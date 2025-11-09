/*
    indBikeSim - An app that simulates a basic FTMS indoor bike

    Copyright (C) 2025  Marcelo Mourier  marcelo_mourier@yahoo.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#ifdef CONFIG_MDNS_AGENT

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "binbuf.h"
#include "dump.h"
#include "fmtbuf.h"
#include "mdns.h"
#include "mlog.h"

/*
4. MESSAGES

4.1. Format

All communications inside of the domain protocol are carried in a single
format called a message.  The top level format of message is divided
into 5 sections (some of which are empty in certain cases) shown below:

    +---------------------+
    |        Header       |
    +---------------------+
    |       Question      | the question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+

The header section is always present.  The header includes fields that
specify which of the remaining sections are present, and also specify
whether the message is a query or a response, a standard query or some
other opcode, etc.

The names of the sections after the header are derived from their use in
standard queries.  The question section contains fields that describe a
question to a name server.  These fields are a query type (QTYPE), a
query class (QCLASS), and a query domain name (QNAME).  The last three
sections have the same format: a possibly empty list of concatenated
resource records (RRs).  The answer section contains RRs that answer the
question; the authority section contains RRs that point toward an
authoritative name server; the additional records section contains RRs
which relate to the query, but are not strictly answers for the
question.

4.1.1. Header section format

The header contains the following fields:

                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

where:

ID              A 16 bit identifier assigned by the program that
                generates any kind of query.  This identifier is copied
                the corresponding reply and can be used by the requester
                to match up replies to outstanding queries.

QR              A one bit field that specifies whether this message is a
                query (0), or a response (1).

OPCODE          A four bit field that specifies kind of query in this
                message.  This value is set by the originator of a query
                and copied into the response.  The values are:

                0               a standard query (QUERY)

                1               an inverse query (IQUERY)

                2               a server status request (STATUS)

                3-15            reserved for future use

AA              Authoritative Answer - this bit is valid in responses,
                and specifies that the responding name server is an
                authority for the domain name in question section.

                Note that the contents of the answer section may have
                multiple owner names because of aliases.  The AA bit
                corresponds to the name which matches the query name, or
                the first owner name in the answer section.

TC              TrunCation - specifies that this message was truncated
                due to length greater than that permitted on the
                transmission channel.

RD              Recursion Desired - this bit may be set in a query and
                is copied into the response.  If RD is set, it directs
                the name server to pursue the query recursively.
                Recursive query support is optional.

RA              Recursion Available - this be is set or cleared in a
                response, and denotes whether recursive query support is
                available in the name server.

Z               Reserved for future use.  Must be zero in all queries
                and responses.

RCODE           Response code - this 4 bit field is set as part of
                responses.  The values have the following
                interpretation:

                0               No error condition

                1               Format error - The name server was
                                unable to interpret the query.

                2               Server failure - The name server was
                                unable to process this query due to a
                                problem with the name server.

                3               Name Error - Meaningful only for
                                responses from an authoritative name
                                server, this code signifies that the
                                domain name referenced in the query does
                                not exist.

                4               Not Implemented - The name server does
                                not support the requested kind of query.

                5               Refused - The name server refuses to
                                perform the specified operation for
                                policy reasons.  For example, a name
                                server may not wish to provide the
                                information to the particular requester,
                                or a name server may not wish to perform
                                a particular operation (e.g., zone
                                transfer) for particular data.

                6-15            Reserved for future use.

QDCOUNT         an unsigned 16 bit integer specifying the number of
                entries in the question section.

ANCOUNT         an unsigned 16 bit integer specifying the number of
                resource records in the answer section.

NSCOUNT         an unsigned 16 bit integer specifying the number of name
                server resource records in the authority records
                section.

ARCOUNT         an unsigned 16 bit integer specifying the number of
                resource records in the additional records section.

 */

struct DnsMesgHdr {
    uint16_t id;
    uint16_t flags;
    uint16_t qdCount;
    uint16_t anCount;
    uint16_t nsCount;
    uint16_t arCount;
} __attribute__((packed));

typedef struct DnsMesgHdr DnsMesgHdr;

//                                 1  1  1  1  1  1
//   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
static __inline__ bool isQueryResp(uint16_t flags) { return !! (flags & 0x8000); }
static __inline__ uint8_t getOpCode(uint16_t flags) { return ((flags >> 11) & 0x000f); }
static __inline__ bool getAaFlag(uint16_t flags) { return !! (flags & 0x0400); }
static __inline__ bool getTcFlag(uint16_t flags) { return !! (flags & 0x0200); }
static __inline__ uint8_t getRespCode(uint16_t flags) { return (flags & 0x000f); }

// TYPE values
#define TYPE_A      ((uint16_t) 1)
#define TYPE_PTR    ((uint16_t) 12)
#define TYPE_HINFO  ((uint16_t) 13)
#define TYPE_TXT    ((uint16_t) 16)
#define TYPE_SRV    ((uint16_t) 33)
#define TYPE_ANY    ((uint16_t) 255)

// CLASS values
#define CLASS_IN    ((uint16_t) 1)

// Cache Flush flag
#define CACHE_FLUSH ((uint16_t) 0x8000)

// Period between mDNS Advertisements
static const struct timeval mdnsAdvPeriod = { .tv_sec = 60, .tv_usec = 0 };

// Time of the last mDNS Advertisement
static struct timeval mdnsLastAdv;

// Device name: ""Wahoo-KICKR-NNNN.local"
static char deviceNameBuf[128];
static FmtBuf mdnsDeviceName;

// Service name: "Wahoo KICKR NNNN._wahoo-fitness-tnp._tcp.local"
static char serviceNameBuf[256];
static FmtBuf mdnsServiceName;

// Wahoo Fitness TNP name: "_wahoo-fitness-tnp._tcp.local"
static char wahooFitnessTnpNameBuf[64];
FmtBuf wahooFitnessTnpName;

// Services DNS-SD name: "_services._dns-sd._udp.local"
static char servicesDnsSdNameBuf[64];
static FmtBuf servicesDnsSdName;

static void mdnsAddName(BinBuf *mesgBuf, const FmtBuf *name)
{
    const char *label = name->buf;
    int labelLen = 0;

    for (const char *p = label; *p != '\0'; p++) {
        if (*p == '.') {
            binBufPutUINT8(mesgBuf, labelLen);
            binBufPutHex(mesgBuf, label, labelLen);
            label = p+1;
            labelLen = 0;
        } else {
            labelLen++;
        }
    }
    binBufPutUINT8(mesgBuf, labelLen);
    binBufPutHex(mesgBuf, label, labelLen);

    binBufPutUINT8(mesgBuf, 0);
}

static int mdnsRemName(BinBuf *mesgBuf, FmtBuf *name)
{
    uint8_t labelLen;
    char label[256];

    while ((labelLen = binBufGetUINT8(mesgBuf)) != 0) {
        if (labelLen & 0xc0) {
            uint8_t flags = (labelLen >> 6);
            if (flags == 0x03) {
                // 4.1.4. Message compression
                //
                // In order to reduce the size of messages, the domain system utilizes a
                // compression scheme which eliminates the repetition of domain names in a
                // message.  In this scheme, an entire domain name or a list of labels at
                // the end of a domain name is replaced with a pointer to a prior occurance
                // of the same name.
                //
                // The pointer takes the form of a two octet sequence:
                //
                //     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                //     | 1  1|                OFFSET                   |
                //     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                //
                // The first two bits are ones.  This allows a pointer to be distinguished
                // from a label, since the label must begin with two zero bits because
                // labels are restricted to 63 octets or less.  (The 10 and 01 combinations
                // are reserved for future use.)  The OFFSET field specifies an offset from
                // the start of the message (i.e., the first octet of the ID field in the
                // domain header).  A zero offset specifies the first byte of the ID field,
                // etc.
                uint16_t offset = ((labelLen &0x3f) << 8) | binBufGetUINT8(mesgBuf);
                //mlog(debug, "OFFSET=%u", offset);
                labelLen = mesgBuf->buf[offset];
                memcpy(label, &mesgBuf->buf[offset+1], labelLen);
                label[labelLen] = '\0';
                (name->offset == 0) ? fmtBufAppend(name, "%s", label) : fmtBufAppend(name, ".%s", label);
                return 0;
            } else {
                mlog(debug, "SPONG! Reserved flags value 0x%02x !", flags);
                return -1;
            }
        }
        if (labelLen > (mesgBuf->bufSize - mesgBuf->offset)) {
            mlog(debug, "SPONG! labelLen=%u mesgBuf.bufSize=%zu mesgBuf.offset=%" PRIu32 ": name=%s",
                 labelLen, mesgBuf->bufSize, mesgBuf->offset, name->buf);
            return -1;
        }
        binBufGetHex(mesgBuf, label, labelLen);
        label[labelLen] = '\0';
        (name->offset == 0) ? fmtBufAppend(name, "%s", label) : fmtBufAppend(name, ".%s", label);
    }

    return 0;
}

static void mdnsAddString(BinBuf *mesgBuf, const char *str)
{
    size_t len = strlen(str) & 0xff;

    binBufPutUINT8(mesgBuf, len);
    binBufPutHex(mesgBuf, str, len);
}

//  4.1.2. Question section format
//
//  The question section is used to carry the "question" in most queries,
//  i.e., the parameters that define what is being asked.  The section
//  contains QDCOUNT (usually 1) entries, each of the following format:
//
//                                      1  1  1  1  1  1
//        0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                                               |
//      /                     QNAME                     /
//      /                                               /
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                     QTYPE                     |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                     QCLASS                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//  where:
//
//  QNAME           a domain name represented as a sequence of labels, where
//                  each label consists of a length octet followed by that
//                  number of octets.  The domain name terminates with the
//                  zero length octet for the null label of the root.  Note
//                  that this field may be an odd number of octets; no
//                  padding is used.
//
//  QTYPE           a two octet code which specifies the type of the query.
//                  The values for this field include all codes valid for a
//                  TYPE field, together with some more general codes which
//                  can match more than one type of RR.
//
//
//  QCLASS          a two octet code that specifies the class of the query.
//                  For example, the QCLASS field is IN for the Internet.
//

static int mdnsAddQuestion(BinBuf *mesgBuf, const FmtBuf *qname, uint16_t qtype, uint16_t qclass)
{
    mdnsAddName(mesgBuf, qname);        // NAME
    binBufPutUINT16(mesgBuf, qtype);    // QTYPE
    binBufPutUINT16(mesgBuf, qclass);   // QCLASS

    return 0;
}

//  4.1.3. Resource record format
//
//  The answer, authority, and additional sections all share the same
//  format: a variable number of resource records, where the number of
//  records is specified in the corresponding count field in the header.
//  Each resource record has the following format:
//
//                                      1  1  1  1  1  1
//        0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                                               |
//      /                                               /
//      /                      NAME                     /
//      |                                               |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                      TYPE                     |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                     CLASS                     |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                      TTL                      |
//      |                                               |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                   RDLENGTH                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
//      /                     RDATA                     /
//      /                                               /
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//  where:
//
//  NAME            an owner name, i.e., the name of the node to which this
//                  resource record pertains.
//
//  TYPE            two octets containing one of the RR TYPE codes.
//
//  CLASS           two octets containing one of the RR CLASS codes.
//
//  TTL             a 32 bit signed integer that specifies the time interval
//                  that the resource record may be cached before the source
//                  of the information should again be consulted.  Zero
//                  values are interpreted to mean that the RR can only be
//                  used for the transaction in progress, and should not be
//                  cached.  For example, SOA records are always distributed
//                  with a zero TTL to prohibit caching.  Zero values can
//                  also be used for extremely volatile data.
//
//  RDLENGTH        an unsigned 16 bit integer that specifies the length in
//                  octets of the RDATA field.
//
//  RDATA           a variable length string of octets that describes the
//                  resource.  The format of this information varies
//                  according to the TYPE and CLASS of the resource record.
//

static int mdnsAddResourceRec(BinBuf *mesgBuf, const FmtBuf *qname, uint16_t type,
                              uint16_t class, uint32_t ttl, const BinBuf *rdata)
{
    size_t rdlen = rdata->offset;

    mdnsAddName(mesgBuf, qname);        // NAME
    binBufPutUINT16(mesgBuf, type);     // TYPE
    binBufPutUINT16(mesgBuf, class);    // CLASS
    binBufPutUINT32(mesgBuf, ttl);      // TTL
    binBufPutUINT16(mesgBuf, rdlen);    // RDLENGTH
    binBufPutHex(mesgBuf, rdata->buf, rdlen); // RDATA

    return 0;
}

static int mdnsSendMesg(Server *server, const BinBuf *mesgBuf)
{
    size_t mesgLen;
    ssize_t n;

    if (server->noMdns) {
        // We are not using mDNS, so just return
        return 0;
    }

    server->txMdnsMesgCnt++;

    mesgLen = mesgBuf->offset;

    mlog(debug, "mesgLen=%zu", mesgLen);
    //hexDump(mesgBuf->buf, mesgLen);

    if ((n = sendto(server->mdnsSockFd, mesgBuf->buf, mesgLen, 0, (struct sockaddr *) &server->mdnsAddr, sizeof (server->mdnsAddr))) != mesgLen) {
        mlog(error, "Failed to send MDNS query! sd=%d mesgLen=%zd n=%zd",
                server->mdnsSockFd, mesgLen, n);
        return -1;
    }

    return 0;
}

int mdnsSendQuery(Server *server, const FmtBuf *qname)
{
    BinBuf mesgBuf;

    mlog(debug, "mdnsSockFd=%d", server->mdnsSockFd);

    binBufInit(&mesgBuf, server->txMesgBuf, sizeof (server->txMesgBuf), bigEndian);

    // Init message header
    binBufPutUINT16(&mesgBuf, 0);    // ID
    binBufPutUINT16(&mesgBuf, 0);    // QR=0, OPCODE=QUERY
    binBufPutUINT16(&mesgBuf, 1);    // QDCOUNT=1
    binBufPutUINT16(&mesgBuf, 0);    // ANCOUNT=0
    binBufPutUINT16(&mesgBuf, 0);    // NSCOUNT=0
    binBufPutUINT16(&mesgBuf, 0);    // ARCOUNT=0

    // Add question #1
    mdnsAddQuestion(&mesgBuf, qname, TYPE_PTR, CLASS_IN);

    return mdnsSendMesg(server, &mesgBuf);
}

static int mdnsSendAdv(Server *server, const struct timeval *time)
{
    BinBuf mesgBuf;
    uint8_t buf[512];
    BinBuf rdata;
    int s;

    mlog(debug, "mdnsSockFd=%d", server->mdnsSockFd);

    binBufInit(&mesgBuf, server->txMesgBuf, sizeof (server->txMesgBuf), bigEndian);

    // Init message header
    binBufPutUINT16(&mesgBuf, 0);    // ID
    binBufPutUINT16(&mesgBuf, 0);    // QR=0, OPCODE=QUERY
    binBufPutUINT16(&mesgBuf, 3);    // QDCOUNT=3
    binBufPutUINT16(&mesgBuf, 0);    // ANCOUNT=0
    binBufPutUINT16(&mesgBuf, 3);    // NSCOUNT=3
    binBufPutUINT16(&mesgBuf, 0);    // ARCOUNT=0

    // Add question #1
    mdnsAddQuestion(&mesgBuf, &mdnsDeviceName, TYPE_ANY, CLASS_IN);

    // Add question #2
    mdnsAddQuestion(&mesgBuf, &mdnsDeviceName, TYPE_ANY, CLASS_IN);

    // Add question #3
    mdnsAddQuestion(&mesgBuf, &mdnsServiceName, TYPE_ANY, CLASS_IN);

    // Add address resource record: TYPE=A, CLASS=IN, TTL=120
    {
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutHex(&rdata, &server->srvAddr.sin_addr, sizeof (server->srvAddr.sin_addr));
        mdnsAddResourceRec(&mesgBuf, &mdnsDeviceName, TYPE_A, CLASS_IN, 120, &rdata);
    }

    // Add host info record: TYPE=HINFO, CLASS=IN, TTL=7200
    {
        const char *wftnp = "WFTNP";
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        mdnsAddString(&rdata, wftnp);   // CPU=<name>
        mdnsAddString(&rdata, wftnp);   // OS=<name>
        mdnsAddResourceRec(&mesgBuf, &mdnsDeviceName, TYPE_HINFO, CLASS_IN, 7200, &rdata);
    }

    // Add service record: TYPE=SRV, CLASS=IN, TTL=120
    {
        uint16_t port = ntohs(server->srvAddr.sin_port);
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutUINT16(&rdata, 0);     // PRIORITY=0
        binBufPutUINT16(&rdata, 0);     // WEIGHT=0
        binBufPutUINT16(&rdata, port);  // PORT=<port>
        mdnsAddName(&rdata, &mdnsDeviceName); // TARGET=<name>
        mdnsAddResourceRec(&mesgBuf, &mdnsServiceName, TYPE_SRV, CLASS_IN, 120, &rdata);
    }

    if ((s = mdnsSendMesg(server, &mesgBuf)) == 0) {
        mdnsLastAdv = *time;
    }

    return s;
}

static int mdnsSendAdvResp(Server *server)
{
    BinBuf mesgBuf;
    uint8_t buf[512];
    BinBuf rdata;

    mlog(debug, "mdnsSockFd=%d", server->mdnsSockFd);

    binBufInit(&mesgBuf, server->txMesgBuf, sizeof (server->txMesgBuf), bigEndian);

    // Init message header
    binBufPutUINT16(&mesgBuf, 0);       // ID
    binBufPutUINT16(&mesgBuf, 0x8000);  // QR=1, OPCODE=QUERY
    binBufPutUINT16(&mesgBuf, 0);       // QDCOUNT=0
    binBufPutUINT16(&mesgBuf, 3);       // ANCOUNT=3
    binBufPutUINT16(&mesgBuf, 0);       // NSCOUNT=0
    binBufPutUINT16(&mesgBuf, 0);       // ARCOUNT=0

    // Add address resource record: TYPE=A, CLASS=IN, CACHE-FLUSH=1, TTL=120
    {
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutHex(&rdata, &server->srvAddr.sin_addr, sizeof (server->srvAddr.sin_addr));
        mdnsAddResourceRec(&mesgBuf, &mdnsDeviceName, TYPE_A, (CLASS_IN | CACHE_FLUSH), 120, &rdata);
    }

    // Add host info record: TYPE=HINFO, CLASS=IN, TTL=7200
    {
        const char *wftnp = "WFTNP";
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        mdnsAddString(&rdata, wftnp);   // CPU=<name>
        mdnsAddString(&rdata, wftnp);   // OS=<name>
        mdnsAddResourceRec(&mesgBuf, &mdnsDeviceName, TYPE_HINFO, (CLASS_IN | CACHE_FLUSH), 7200, &rdata);
    }

    // Add service record: TYPE=SRV, CLASS=IN, CACHE-FLUSH=1, TTL=120
    {
        uint16_t port = ntohs(server->srvAddr.sin_port);
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutUINT16(&rdata, 0);     // PRIORITY=0
        binBufPutUINT16(&rdata, 0);     // WEIGHT=0
        binBufPutUINT16(&rdata, port);  // PORT=<port>
        mdnsAddName(&rdata, &mdnsDeviceName); // TARGET=<name>
        mdnsAddResourceRec(&mesgBuf, &mdnsServiceName, TYPE_SRV, (CLASS_IN | CACHE_FLUSH), 120, &rdata);
    }

    return mdnsSendMesg(server, &mesgBuf);
}

static int mdnsSendResp(Server *server, uint16_t id, const FmtBuf *qname)
{
    BinBuf mesgBuf;
    uint8_t buf[512];
    BinBuf rdata;

    binBufInit(&mesgBuf, server->txMesgBuf, sizeof (server->txMesgBuf), bigEndian);

    // Init message header
    binBufPutUINT16(&mesgBuf, 0);       // ID
    binBufPutUINT16(&mesgBuf, 0x8000);  // QR=1, OPCODE=QUERY
    binBufPutUINT16(&mesgBuf, 0);       // QDCOUNT=0
    binBufPutUINT16(&mesgBuf, 4);       // ANCOUNT=4
    binBufPutUINT16(&mesgBuf, 0);       // NSCOUNT=0
    binBufPutUINT16(&mesgBuf, 0);       // ARCOUNT=0

    // Add pointer record: TYPE=PTR, CLASS=IN, TTL=4500
    {
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        if (fmtBufComp(qname, &servicesDnsSdName) == 0) {
            // qname="_services._dns-sd._udp.local"
            mdnsAddName(&rdata, &wahooFitnessTnpName); // TARGET="_wahoo-fitness-tnp._tcp.local"
        } else {
            // qname="_wahoo-fitness-tnp._tcp.local"
            mdnsAddName(&rdata, &mdnsServiceName);  // TARGET="Wahoo KICKR NNNN._wahoo-fitness-tnp._tcp.local"
        }
        mdnsAddResourceRec(&mesgBuf, qname, TYPE_PTR, CLASS_IN, 4500, &rdata);
    }

    // Add address resource record: TYPE=A, CLASS=IN, CACHE-FLUSH=1, TTL=120
    {
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutHex(&rdata, &server->srvAddr.sin_addr, sizeof (server->srvAddr.sin_addr));
        mdnsAddResourceRec(&mesgBuf, &mdnsDeviceName, TYPE_A, (CLASS_IN | CACHE_FLUSH), 120, &rdata);
    }

    // Add service record: TYPE=SRV, CLASS=IN, CACHE-FLUSH=1, TTL=120
    {
        uint16_t port = ntohs(server->srvAddr.sin_port);
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        binBufPutUINT16(&rdata, 0);     // PRIORITY=0
        binBufPutUINT16(&rdata, 0);     // WEIGHT=0
        binBufPutUINT16(&rdata, port);  // PORT=<port>
        mdnsAddName(&rdata, &mdnsDeviceName); // TARGET=<name>
        mdnsAddResourceRec(&mesgBuf, &mdnsServiceName, TYPE_SRV, (CLASS_IN | CACHE_FLUSH), 120, &rdata);
    }

    // Add text record: TYPE=TXT, CLASS=IN, TTL=4500
    {
        char macAddr[64];
        binBufInit(&rdata, buf, sizeof (buf), bigEndian);
        mdnsAddString(&rdata, "serial-number=123456789");
        snprintf(macAddr, sizeof (macAddr), "mac-address=%02X-%02X-%02X-%02X-%02X-%02X",
                 server->macAddr[0], server->macAddr[1], server->macAddr[2],
                 server->macAddr[3], server->macAddr[4], server->macAddr[5]);
        mdnsAddString(&rdata, macAddr);
        mdnsAddString(&rdata, "ble-service-uuids=0x1818,0x1826");
        mdnsAddResourceRec(&mesgBuf, &mdnsServiceName, TYPE_TXT, CLASS_IN, 120, &rdata);
    }

    return mdnsSendMesg(server, &mesgBuf);
}

int mdnsInit(Server *server)
{
    int sd;
    struct sockaddr_in locAddr = {0};
    struct ip_mreq mreq = {{0},{0}};

    if (server->noMdns) {
        // We are not using mDNS, so just return
        return 0;
    }

    fmtBufInit(&mdnsDeviceName, deviceNameBuf, sizeof (deviceNameBuf));
    fmtBufAppend(&mdnsDeviceName, "Wahoo-KICKR-%02X%02X.local",
                 server->macAddr[4], server->macAddr[5]);

    fmtBufInit(&mdnsServiceName, serviceNameBuf, sizeof (serviceNameBuf));
    fmtBufAppend(&mdnsServiceName, "Wahoo KICKR %02X%02X._wahoo-fitness-tnp._tcp.local",
                 server->macAddr[4], server->macAddr[5]);

    fmtBufInit(&wahooFitnessTnpName, wahooFitnessTnpNameBuf, sizeof (wahooFitnessTnpNameBuf));
    fmtBufAppend(&wahooFitnessTnpName, "_wahoo-fitness-tnp._tcp.local");

    fmtBufInit(&servicesDnsSdName, servicesDnsSdNameBuf, sizeof (servicesDnsSdNameBuf));
    fmtBufAppend(&servicesDnsSdName, "_services._dns-sd._udp.local");

    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        mlog(error, "Failed to open UDP socket!");
        return -1;
    }

    locAddr.sin_family = AF_INET;
    locAddr.sin_port = htons(5353);
    if (bind(sd, (struct sockaddr *) &locAddr, sizeof (locAddr)) < 0) {
        int errNo = errno;
        mlog(error, "Failed to bind UDP socket!");
        if (errNo == EADDRINUSE) {
            mlog(error, "Make sure there is no Zeroconf/Bonjour service running on this system...");
        }
        close(sd);
        return -1;
    }

    // Initialize well-known mDNS socket address
    server->mdnsAddr.sin_family = AF_INET;
    server->mdnsAddr.sin_addr.s_addr = htonl(0xe00000fb);   // 224.0.0.251
    server->mdnsAddr.sin_port = htons(MDNS_UDP_PORT);

    mreq.imr_multiaddr = server->mdnsAddr.sin_addr;
    mreq.imr_interface = server->srvAddr.sin_addr;
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        mlog(error, "Failed to join MDNS mcast group!");
        close(sd);
        return -1;
    }

    server->mdnsSockFd = sd;

    // Send the initial mDNS advertisements...
    for (int i = 0; i < 3; i++) {
        struct timeval now;
        gettimeofday(&now, NULL);
        mdnsSendAdv(server, &now);
        usleep(250000); // 250 ms delay
    }

    // ... and the matching responses
    for (int i = 0; i < 3; i++) {
        mdnsSendAdvResp(server);
        usleep(10000); // 10 ms delay
    }

    return 0;
}

int mdnsProcTimers(Server *server, const struct timeval *time)
{
    struct timeval delta;

    if (server->noMdns) {
        // We are not using mDNS, so just return
        return 0;
    }

    tvSub(&delta, time, &mdnsLastAdv);

    if (tvCmp(&delta, &mdnsAdvPeriod) >= 0) {
        // Time to send a new mDNS advertisement!
        mdnsSendAdv(server, time);
        mdnsSendAdvResp(server);
    }

    return 0;
}

int mdnsProcQueryMesg(Server *server, const DnsMesgHdr *hdr, BinBuf *mesgBuf)
{
    mlog(debug, "id=0x%04x opcode=%u tc=%u qdcnt=%u ancnt=%u nscnt=%u arcnt=%u",
                 hdr->id, getOpCode(hdr->flags), getTcFlag(hdr->flags),
                 hdr->qdCount, hdr->anCount, hdr->nsCount, hdr->arCount);

    for (int i = 0; i < hdr->qdCount; i++) {
        FmtBuf qnameBuf;
        char name[256];
        uint16_t qtype;
        uint16_t qclass;

        // Get the QNAME string
        fmtBufInit(&qnameBuf, name, sizeof (name));
        if (mdnsRemName(mesgBuf, &qnameBuf) != 0) {
            // Ignore message...
            mlog(debug, "Ignoring malformed/corrupted message...");
            //hexDump(mesgBuf->buf, mesgBuf->bufSize);
            return 0;
        }

        // Get the QTYPE and QCLASS values
        qtype = binBufGetUINT16(mesgBuf);
        qclass = binBufGetUINT16(mesgBuf) & ~CACHE_FLUSH;

        mlog(debug, "#%d: qname=%s qtype=%u qclass=%u", i, qnameBuf.buf, qtype, qclass);

        // We only care about QTYPE=PTR and QCLASS=IN
        if ((qtype != TYPE_PTR) || (qclass != CLASS_IN)) {
            // Not interested
            continue;
        }

        // We only care about queries for the following
        // PTR domains:
        //   "_services._dns-sd._udp.local"
        //   "_wahoo-fitness-tnp._tcp.local"
        if ((fmtBufComp(&qnameBuf, &servicesDnsSdName) != 0) &&
            (fmtBufComp(&qnameBuf, &wahooFitnessTnpName) != 0)) {
            // Not interested
            continue;
        }

        // Send the query response
        return mdnsSendResp(server, hdr->id, &qnameBuf);
    }

    return 0;
}

int mdnsProcQueryRespMesg(Server *server, const DnsMesgHdr *hdr, BinBuf *mesgBuf)
{
    DirconSession *devSess = &server->dirconSession[dev];
    FmtBuf nameBuf;
    char name[256];
    uint16_t qtype, qclass, rdlen;
    uint32_t ttl;

    mlog(debug, "id=0x%04x opcode=%u aa=%u rcode=%u qdcnt=%u ancnt=%u nscnt=%u arcnt=%u",
                 hdr->id, getOpCode(hdr->flags), getAaFlag(hdr->flags), getRespCode(hdr->flags),
                 hdr->qdCount, hdr->anCount, hdr->nsCount, hdr->arCount);

    // Process each Answer RR in the response...
    for (int n = 0; n < hdr->anCount; n++) {
        // Get the QNAME string
        fmtBufInit(&nameBuf, name, sizeof (name));
        if (mdnsRemName(mesgBuf, &nameBuf) != 0) {
            // Ignore message...
            mlog(debug, "Ignoring malformed/corrupted message...");
            //hexDump(mesgBuf->buf, mesgBuf->bufSize);
            return 0;
        }

        // Get the QTYPE and QCLASS values
        qtype = binBufGetUINT16(mesgBuf);   // Get QTYPE
        qclass = binBufGetUINT16(mesgBuf) & ~CACHE_FLUSH;   // Get QCLASS
        ttl = binBufGetUINT32(mesgBuf);     // Get TTL
        rdlen = binBufGetUINT16(mesgBuf);   // Get RDLENGTH

        mlog(debug, "    qname=%s qtype=%u qclass=%u ttl=%" PRIu32 " rdlen=%u", nameBuf.buf, qtype, qclass, ttl, rdlen);

        if (qtype == TYPE_PTR) {
            // We only care about query responses for wahooFitnessTnpName
            if (fmtBufComp(&nameBuf, &wahooFitnessTnpName) != 0) {
                // Not interested!
                return 0;
            }
            fmtBufInit(&nameBuf, name, sizeof (name));
            mdnsRemName(mesgBuf, &nameBuf);     // Get RDATA (service domain name)
            if (devSess->remCliAddr.sin_family == 0) {
                devSess->remCliAddr.sin_family = AF_INET;
            }
        } else if (qtype == TYPE_A) {
            memcpy(devSess->peerName, nameBuf.buf, nameBuf.offset); // save the indoor trainer device name
            devSess->peerName[nameBuf.offset] = '\0';
            devSess->remCliAddr.sin_addr.s_addr = htonl(binBufGetUINT32(mesgBuf));   // Get RDATA (IPv4 address)
        } else if (qtype == TYPE_SRV) {
            mesgBuf->offset += 4;   // skip over the priority (2) and weight (2) values
            devSess->remCliAddr.sin_port = htons(binBufGetUINT16(mesgBuf));
        } else if (qtype == TYPE_TXT) {

        }
    }

    return 0;
}

int mdnsProcMesg(Server *server)
{
    struct sockaddr_in fromAddr = {0};
    socklen_t fromAddrLen = sizeof (fromAddr);
    ssize_t mesgLen;
    BinBuf mesgBuf;
    DnsMesgHdr hdr;
    int s;

    if ((mesgLen = recvfrom(server->mdnsSockFd, server->rxMesgBuf, sizeof (server->rxMesgBuf),
                            0, (struct sockaddr *) &fromAddr, &fromAddrLen)) < 0) {
        mlog(error, "Failed to read MDNS message!");
        return -1;
    }

    if (mesgLen < sizeof (DnsMesgHdr)) {
        mlog(error, "Runt message: mesgLen=%zd", mesgLen);
        return -1;
    }

    // Ignore mDNS messages sourced by us...
    if (fromAddr.sin_addr.s_addr == server->srvAddr.sin_addr.s_addr) {
        //mlog(debug, "Ignoring own mDNS message...");
        return 0;
    }

    server->rxMdnsMesgCnt++;

    {
        mlog(debug, "sender=%s mesgLen=%zd", fmtSockaddr(&fromAddr, true), mesgLen);
        //hexDump(server->rxMesgBuf, mesgLen);
    }

    // Get the fixed-size header
    binBufInit(&mesgBuf, server->rxMesgBuf, mesgLen, bigEndian);
    binBufGetHex(&mesgBuf, &hdr, sizeof (hdr));
    hdr.id = ntohs(hdr.id);
    hdr.flags = ntohs(hdr.flags);
    hdr.qdCount = ntohs(hdr.qdCount);
    hdr.anCount = ntohs(hdr.anCount);
    hdr.nsCount = ntohs(hdr.nsCount);
    hdr.arCount = ntohs(hdr.arCount);

    if (isQueryResp(hdr.flags)) {
        s = mdnsProcQueryRespMesg(server, &hdr, &mesgBuf);
    } else {
        s = mdnsProcQueryMesg(server, &hdr, &mesgBuf);
    }

    return s;
}

#endif  // CONFIG_MDNS_AGENT
