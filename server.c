/*
    indBikeSim - An app that simulates a basic FTMS indoor bike

    Copyright (C) 2023  Marcelo Mourier  marcelo_mourier@yahoo.com

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

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <unistd.h>

#ifdef __DARWIN__
#include <net/if_dl.h>
#include <sys/sysctl.h>
#endif

#include "cli.h"
#include "config.h"
#include "dircon.h"
#include "mdns.h"
#include "mlog.h"
#include "server.h"

#ifdef CONFIG_FIT_ACTIVITY_FILE
// FIT SDK files
#include "fit/fit.c"
#include "fit/fit_example.c"
#include "fit/fit_crc.c"
#include "fit/fit_convert.c"
#include "fit/fit_strings.c"

// FIT uses December 31, 1989 UTC as their Epoch. See below
// for the details:
//   https://developer.garmin.com/fit/cookbook/datetime/
static const time_t fitEpoch = 631065600;

static void fitRecToTrkPt(const FIT_RECORD_MESG *record, TrkPt *tp)
{
    if (record->timestamp != FIT_DATE_TIME_INVALID) {
        tp->timestamp = (double) ((time_t) record->timestamp + fitEpoch);   // in s since UTC Epoch
    }

    if (record->cadence != FIT_UINT8_INVALID) {
        tp->cadence = record->cadence;
    }

    if (record->heart_rate != FIT_UINT8_INVALID) {
        tp->heartRate = record->heart_rate;
    }

    if (record->power != FIT_UINT16_INVALID) {
        tp->power = record->power;
    }

    //mlog(debug, "timestamp=%ld cadence=%u heartRate=%u power=%u",
    //        tp->timestamp, tp->cadence, tp->heartRate, tp->power);
}

// Parse the FIT file and create a list of Track Points (TrkPt's)
static int parseFitFile(Server *server)
{
    FILE *fp = server->actFile;
    FIT_UINT8 inBuf[8];
    FIT_CONVERT_RETURN conRet = FIT_CONVERT_CONTINUE;
    FIT_UINT32 bufSize;
    FIT_MANUFACTURER manufacturer = FIT_MANUFACTURER_INVALID;
    bool timerRunning = true;
    int numTrkPts = 0;

    FitConvert_Init(FIT_TRUE);

    while (!feof(fp) && (conRet == FIT_CONVERT_CONTINUE)) {
        for (bufSize = 0; (bufSize < sizeof (inBuf)) && !feof(fp); bufSize++) {
            inBuf[bufSize] = (FIT_UINT8) getc(fp);
        }

        do {
            if ((conRet = FitConvert_Read(inBuf, bufSize)) == FIT_CONVERT_MESSAGE_AVAILABLE) {
                const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                FIT_MESG_NUM mesgNum = FitConvert_GetMessageNumber();

                switch (mesgNum) {
                    case FIT_MESG_NUM_SPORT: {
                        const FIT_SPORT_MESG *sport = (FIT_SPORT_MESG *) mesg;

                        if (sport->sport != FIT_SPORT_CYCLING) {
                            fprintf(stderr, "Not a cycling activity !!!\n");
                            return -1;
                        }
                        break;
                    }

                    case FIT_MESG_NUM_RECORD: {
                        const FIT_RECORD_MESG *record = (FIT_RECORD_MESG *) mesg;

                        if (timerRunning) {
                            // The Strava app generates a pair of FIT RECORD messages
                            // for each trackpoint (i.e. timestamp). The first one seems
                            // to always have a valid distance value of 0.000, but no
                            // latitude/longitude/altitude values: e.g.
                            //
                            // Mesg 9 (21) - Event: timestamp=1018803532 event=0 event_type=0
                            // Mesg 10 (20) - Record: timestamp=1018803532 distance=0.000
                            // Mesg 11 (20) - Record: timestamp=1018803532 latitude=43.6232699098 longitude=-114.3533090010 enh_altitude=1712.000 speed=0.310
                            // Mesg 12 (20) - Record: timestamp=1018803533 distance=0.000
                            // Mesg 13 (20) - Record: timestamp=1018803533 latitude=43.6232681496 longitude=-114.3533167124 enh_altitude=1712.000 speed=0.112
                            //
                            // So here we detect, and skip, such RECORD messages...
                            if ((manufacturer == FIT_MANUFACTURER_STRAVA) &&
                                ((record->position_lat == FIT_SINT32_INVALID) ||
                                 (record->position_long == FIT_SINT32_INVALID) ||
                                 (record->enhanced_altitude == FIT_UINT32_INVALID))) {
                                //printf(" *** SKIPPED ***");
                            } else {
                                TrkPt *pTrkPt;

                                // Alloc new TrkPt object
                                if ((pTrkPt = trkPtNew(numTrkPts++)) == NULL) {
                                    fprintf(stderr, "Failed to create TrkPt object !!!\n");
                                    return -1;
                                }

                                // Init TrkPt object with the values from the FIT
                                // RECORD message.
                                fitRecToTrkPt(record, pTrkPt);

                                // Insert track point at the tail of the queue
                                TAILQ_INSERT_TAIL(&server->trkPtList, pTrkPt, tqEntry);
                            }
                        } else {
                            fprintf(stderr, "Hu? RECORD message while timer not running !!!\n");
                        }
                        break;
                    }

                    case FIT_MESG_NUM_EVENT: {
                        const FIT_EVENT_MESG *event = (FIT_EVENT_MESG *) mesg;
                        //printf("%s: timestamp=%u event=%s event_type=%s\n",
                        //        fitMesgNum(mesgNum),
                        //        event->timestamp, fitEvent(event->event), fitEventType(event->event_type));
                        if (event->event == FIT_EVENT_TIMER) {
                            if (event->event_type == FIT_EVENT_TYPE_START) {
                                timerRunning = true;
                            } else if (event->event_type == FIT_EVENT_TYPE_STOP) {
                                timerRunning = false;
                            }
                        }
                        break;
                    }

                    default: {
                        if ((mesgNum >= FIT_MESG_NUM_MFG_RANGE_MIN) && (mesgNum <= FIT_MESG_NUM_MFG_RANGE_MAX)) {
                            // TBD
                        } else {
                            //printf("%s: %u\n", mesgNum);
                        }
                        break;
                    }
                }
            }
        } while (conRet == FIT_CONVERT_MESSAGE_AVAILABLE);
    }

    fclose(fp);

    if (conRet != FIT_CONVERT_END_OF_FILE) {
        const char *errMsg = NULL;
        if (conRet == FIT_CONVERT_ERROR) {
            errMsg = "Error decoding file";
        } else if (conRet == FIT_CONVERT_CONTINUE) {
            errMsg = "Unexpected end of file";
        } else if (conRet == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED) {
            errMsg = "File is not FIT";
        } else if (conRet == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED) {
            errMsg = "Protocol version not supported";
        }

        fprintf(stderr, "%s !!!\n", errMsg);

        return -1;
    }

    return 0;
}
#endif  // CONFIG_FIT_ACTIVITY_FILE

Service *serverAddService(Server *server, const Uuid128 *uuid)
{
    Service *svc = svcNew(uuid);

    if (svc != NULL) {
        TAILQ_INSERT_TAIL(&server->svcList, svc, svcListEnt);
        //mlog(trace, "svcUUID=\"%s\"", fmtUuid128Name(uuid));
    }

    return svc;
}

Service *serverFindService(const Server *server, const Uuid128 *uuid)
{
    Service *svc = NULL;

    TAILQ_FOREACH(svc, &server->svcList, svcListEnt) {
        if (uuid128Eq(&svc->uuid, uuid)) {
            // Found it!
            return svc;
        }
    }

    return svc;
}

DirconSessId swapDirconSessId(DirconSessId sessId)
{
    return (sessId == app) ? dev : app;
}

const char *fmtDirconSessId(DirconSessId sessId)
{
    if (sessId == app)
        return "app";
    else if (sessId == dev)
        return "dev";

    return "???";
}

const char *fmtSockaddr(const struct sockaddr_in *sockAddr, bool printPort)
{
    static char fmtBuf[INET_ADDRSTRLEN + 7];    // "AAA.BBB.CCC.DDD[NNNNN]"

    if (printPort) {
        char addrBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sockAddr->sin_addr, addrBuf, sizeof (addrBuf));
        snprintf(fmtBuf, sizeof (fmtBuf), "%s[%hu]", addrBuf, ntohs(sockAddr->sin_port));
    } else {
        inet_ntop(AF_INET, &sockAddr->sin_addr, fmtBuf, sizeof (fmtBuf));
    }

    return fmtBuf;
}

#ifdef __DARWIN__
static int getIntfMacAddr(Server *server, const char *intfName)
{
    int mib[6];
    size_t len;
    uint8_t *buf;
    uint8_t *ptr;
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    if ((mib[5] = if_nametoindex(intfName)) == 0) {
        mlog(error, "if_nametoindex() failed! (%s)", strerror(errno));
        return -1;
    }

    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
        mlog(error, "sysctl() failed! (%s)", strerror(errno));
        return -1;
    }

    if ((buf = malloc(len)) == NULL) {
        mlog(error, "malloc() failed! (%s)", strerror(errno));
        return -1;
    }

    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        mlog(error, "sysctl() failed! (%s)", strerror(errno));
        free(buf);
        return -1;
    }

    ifm = (struct if_msghdr *)buf;
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (unsigned char *)LLADDR(sdl);
    for (int i = 0; i < 6; i++)
        server->macAddr[i] = ptr[i];

    free(buf);

    return 0;
}
#else
static int getIntfMacAddr(Server *server, const char *intfName)
{
    int sd;
    struct ifreq ifReq = {0};

    snprintf(ifReq.ifr_name, sizeof (ifReq.ifr_name), "%s", intfName);

    if ((sd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        mlog(error, "socket() failed! (%s)", strerror(errno));
        return -1;
    }

    if (ioctl(sd, SIOCGIFHWADDR, &ifReq) < 0) {
        mlog(error, "ioctl() failed! (%s)", strerror(errno));
        close(sd);
        return -1;
    }

    if (ifReq.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
        mlog(error, "unexpected hwaddr type %d !", ifReq.ifr_hwaddr.sa_family);
        close(sd);
        return -1;
    }

    memcpy(server->macAddr, ifReq.ifr_hwaddr.sa_data, sizeof (server->macAddr));

    close(sd);

    return 0;
}
#endif

static int findIntfAddr(Server *server)
{
    struct ifaddrs *ifAddrLst = NULL;
    int s = -1;

    if (getifaddrs(&ifAddrLst) == -1) {
        mlog(error, "getifaddrs() failed! (%s)", strerror(errno));
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (struct ifaddrs *ifAddr = ifAddrLst; ifAddr != NULL; ifAddr = ifAddr->ifa_next) {
        const struct sockaddr *sockAddr = ifAddr->ifa_addr;

        if (sockAddr == NULL)
            continue;

        if (sockAddr->sa_family != AF_INET)
            continue;

        {
            const struct sockaddr_in *sin = (struct sockaddr_in *) sockAddr;
            if (ntohl(sin->sin_addr.s_addr) != INADDR_LOOPBACK) {
                // Get the IP address
                server->srvAddr.sin_addr.s_addr = sin->sin_addr.s_addr;

                // Get the MAC address
                s = getIntfMacAddr(server, ifAddr->ifa_name);
                break;
            }
        }
    }

    freeifaddrs(ifAddrLst);

    return s;
}

static int initServerSock(Server *server)
{
    int reuseAddr = 1;
    int sd;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        mlog(error, "socket() failed!");
        return -1;
    }
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof (reuseAddr)) < 0) {
        mlog(error, "setsockopt(SO_REUSEADDR) failed!");
        return -1;
    }
    if (bind(sd, (struct sockaddr *) &server->srvAddr, sizeof (server->srvAddr)) < 0) {
        mlog(error, "bind() failed!");
        return -1;
    }
    if (listen(sd, 5) < 0) {
        mlog(error, "listen() failed!");
        return -1;
    }

    server->srvSockFd = sd;

    return 0;
}

int serverInit(Server *server)
{
    gettimeofday(&server->baseTime, NULL);

    TAILQ_INIT(&server->svcList);

    for (DirconSessId sessId = app; sessId <= dev; sessId++) {
        server->dirconSession[sessId].sessId = sessId;
        server->dirconSession[sessId].lastTxReqSeqNum = 0xff;
    }

#ifdef CONFIG_FIT_ACTIVITY_FILE
    TAILQ_INIT(&server->trkPtList);

    if (server->actFile != NULL) {
        // Load the FIT activity file
        if (parseFitFile(server) != 0) {
            mlog(error, "Failed to load FIT activity file!");
            return -1;
        }
    }
#endif

    if (server->srvAddr.sin_addr.s_addr == 0) {
        // Figure out the interface IP address...
        if (findIntfAddr(server) != 0) {
            mlog(error, "Can't determine interface IP address!");
            return -1;
        }
    }

    mlog(info, "Using socket address: %s at %02x-%02x-%02x-%02x-%02x-%02x",
            fmtSockaddr(&server->srvAddr, true),
            server->macAddr[0], server->macAddr[1], server->macAddr[2],
            server->macAddr[3], server->macAddr[4], server->macAddr[5]);

#ifdef CONFIG_BLE_CONTROLLER
    // Initialize the BLE API
    if (bleInit(server) != 0) {
        mlog(error, "Can't init BLE API!");
        return -1;
    }
#endif

    if (server->dirconSession[dev].remCliAddr.sin_family == 0) {
    	// We don't have a downstream DIRCON indoor
    	// trainer, so we can init the DIRCON server
    	// socket now...
    	if (initServerSock(server) != 0) {
    		mlog(fatal, "Failed to init DIRCON server socket!");
    	}
    }

    return 0;
}

#ifdef CONFIG_MDNS_AGENT
static int discoverIndoorTrainer(Server *server)
{
    // Send an mDNS query for "_wahoo-fitness-tnp._tcp.local"
    if (mdnsSendQuery(server, &wahooFitnessTnpName) != 0) {
        mlog(error, "mdnsSendQuery()!");
        return -1;
    }

    // Wait up to 10 sec for a query response...
    for (int i = 0; i < 10; i++) {
        DirconSession *devSess = &server->dirconSession[dev];
        struct pollfd pollFdSet[1];
        int n;

        pollFdSet[0].fd = server->mdnsSockFd;
        pollFdSet[0].events = POLLIN;
        pollFdSet[0].revents = 0;
        if ((n = poll(pollFdSet, 1, 1000)) == 0) {
            continue;
        } else if (n < 0) {
            mlog(error, "poll()");
            return -1;
        }

        if (mdnsProcMesg(server) != 0) {
            mlog(error, "mdnsProcMesg()!");
            return -1;
        }

        if (devSess->remCliAddr.sin_family != 0) {
            // DIRCON trainer discovered!
            mlog(info, "DIRCON trainer discovered: \"%s\" at %s", devSess->peerName, fmtSockaddr(&devSess->remCliAddr, true));
            break;
        }
    }

    return 0;
}
#endif

static int connToIndoorTrainer(Server *server)
{
    DirconSession *sess = &server->dirconSession[dev];
    int cliSockFd;
    int enable = 1;
    int kaOptVal;
    int connAttempts = 0;
    const int maxConnAttemps = 3;

    // Set up the client (local) address
    sess->locCliAddr.sin_family = AF_INET;
    sess->locCliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sess->locCliAddr.sin_port = htons(0);

    // To protect against trainers that reject a new connection
    // attempt immediately after a connection drop or an app
    // restart, we allow a few connection retries...
    while (connAttempts++ < maxConnAttemps) {
        if ((cliSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            mlog(error, "Can't alloc TCP socket!");
            return -1;
        }

        if (bind(cliSockFd, (struct sockaddr *) &sess->locCliAddr, sizeof (sess->locCliAddr)) < 0) {
            mlog(error, "Can't bind TCP socket!");
            close(cliSockFd);
            return -1;
        }

        // Connect to the smart trainer.
        if (connect(cliSockFd, (struct sockaddr *) &sess->remCliAddr, sizeof (sess->remCliAddr)) == 0) {
            // Success!
            break;
        } else if ((errno == ECONNRESET) && (connAttempts < maxConnAttemps)) {
            mlog(warning, "Connection attempt #%u failed. Retrying in 10 s ...", connAttempts);
            close(cliSockFd);
            sleep(10);
            continue;
        } else {
            mlog(error, "Can't connect TCP socket: sd=%d addr=%s", cliSockFd, fmtSockaddr(&sess->remCliAddr, true));
            close(cliSockFd);
            return -1;
        }
    }

    // Enable TCP Keep-Alive's
    if (setsockopt(cliSockFd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof (enable)) != 0) {
        mlog(error, "Can't set SO_KEEPALIVE option!");
        close(cliSockFd);
        return -1;
    }
#ifndef __DARWIN__
    kaOptVal = 1;   // start sending KA's after being idle for 1 sec
    if (setsockopt(cliSockFd, IPPROTO_TCP, TCP_KEEPIDLE, &kaOptVal, sizeof (kaOptVal)) != 0) {
        mlog(error, "Can't set TCP_KEEPIDLE option!");
        close(cliSockFd);
        return -1;
    }
#endif
    kaOptVal = 1;   // Use a 1 sec KA interval
    if (setsockopt(cliSockFd, IPPROTO_TCP, TCP_KEEPINTVL, &kaOptVal, sizeof (kaOptVal)) != 0) {
        mlog(error, "Can't set TCP_KEEPINTVL option!");
        close(cliSockFd);
        return -1;
    }
    kaOptVal = 2;   // Close connection after 2 unsuccessful KA probes
    if (setsockopt(cliSockFd, IPPROTO_TCP, TCP_KEEPCNT, &kaOptVal, sizeof (kaOptVal)) != 0) {
        mlog(error, "Can't set TCP_KEEPCNT option!");
        close(cliSockFd);
        return -1;
    }

    // Set NODELAY option to reduce the latency of the
    // messages we send out over this DIRCON session.
    if (setsockopt(cliSockFd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof (enable)) != 0) {
        mlog(error, "Can't set TCP_NODELAY option!");
        close(cliSockFd);
        return -1;
    }

    // Figure out the actual local address and port
    {
        socklen_t addrLen = sizeof(sess->locCliAddr);
        if (getsockname(cliSockFd, (struct sockaddr *) &sess->locCliAddr, &addrLen) < 0) {
            mlog(error, "Can't connect TCP socket!");
            close(cliSockFd);
            return -1;
        }
    }

    {
        char locAddrBuf[INET_ADDRSTRLEN];
        char remAddrBuf[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &sess->locCliAddr.sin_addr, locAddrBuf, sizeof (locAddrBuf));
        inet_ntop(AF_INET, &sess->remCliAddr.sin_addr, remAddrBuf, sizeof (remAddrBuf));
        mlog(info, "Connected to indoor trainer: %s[%u] -> %s[%u]",
                locAddrBuf, ntohs(sess->locCliAddr.sin_port),
                remAddrBuf, ntohs(sess->remCliAddr.sin_port));
    }

    sess->cliSockFd = cliSockFd;

    return 0;
}

static int discoverIndoorTrainerServices(Server *server)
{
    DirconSession *sess = &server->dirconSession[dev];
    Uuid128 uuid;
    Service *svc;
    Characteristic *chr;

    // Send trainer a Discover Services messages...
    if (dirconSendDiscoverServicesMesg(server, sess) != 0) {
        mlog(error, "Failed to send Discover Services request message!");
        return -1;
    }

    // ... and wait for its response
    if (dirconProcMesg(server, dev) != 0) {
        mlog(error, "Failed to get Discover Services response message!");
        return -1;
    }

    // Now query the trainer for the characteristics
    // of each of its advertised services...
    {
        TAILQ_FOREACH(svc, &server->svcList, svcListEnt) {
            // Send trainer a Discover Characteristics messages...
            if (dirconSendDiscoverCharacteristicsMesg(server, sess, &svc->uuid) != 0) {
                mlog(error, "Failed to send Discover Characteristics request message!");
                return -1;
            }

            // ... and wait for its response
            if (dirconProcMesg(server, dev) != 0) {
                mlog(error, "Failed to get Discover Characteristics response message!");
                return -1;
            }
        }
    }

    // Check that the trainer supports the Fitness Machine Service
    buildUuid128(&uuid, &fitnessMachineServiceUUID);
    if ((svc = serverFindService(server, &uuid)) == NULL) {
        mlog(error, "Indoor trainer doesn't support the Fitness Machine Service!");
        return -1;
    }

    // Now check that the Fitness Machine Service supports the
    // Indoor Bike Data characteristic ...
    buildUuid128(&uuid, &indoorBikeDataUUID);
    if ((chr = svcFindChar(svc, &uuid)) == NULL) {
        mlog(error, "Fitness Machine Service doesn't support the Indoor Bike Data characteristic!");
        return -1;
    }

    // ... and that the Indoor Bike Data characteristic supports
    // unsolicited notifications.
    if (!(chr->properties & DIRCON_CHAR_PROP_NOTIFY)) {
        mlog(error, "Indoor Bike Data characteristic doesn't support notifications!");
        return -1;
    }

    // We are good to go!
    mlog(info, "Indoor trainer meets requirements!");

    return 0;
}

int serverConnectToDirconTrainer(Server *server)
{
#ifdef CONFIG_MDNS_AGENT
    if (server->dirconSession[dev].remCliAddr.sin_family == 0) {
        // Try to discover the downstream DIRCON
        // indoor trainer via mDNS...
        if (discoverIndoorTrainer(server) != 0) {
            mlog(warning, "No DIRCON indoor trainer discovered...");
        }
    }
#endif

    if (server->dirconSession[dev].remCliAddr.sin_family != 0) {
        // Connect to the downstream indoor trainer
        if (connToIndoorTrainer(server) != 0) {
            mlog(error, "Failed to connect to indoor trainer!");
            return -1;
        }

        // Discover the services and characteristics of the
        // downstream indoor trainer, and check that it meets
        // the minimum requirements.
        if (discoverIndoorTrainerServices(server) != 0) {
            mlog(error, "Unsupported indoor trainer!");
            return -1;
        }

        // Set up TCP server (listening) socket
        if (initServerSock(server) != 0) {
            mlog(error, "Failed to init server socket!");
            return -1;
        }

        // If bridge mode is enabled, at this point we can
        // start doing the DIRCON message forwarding...
        if (server->bridgeMode) {
            server->mesgForwarding = true;
            mlog(trace, "DIRCON message forwarding enabled ...");
        }
    }

    return 0;
}

static int serverProcConnReq(Server *server)
{
    DirconSession *sess = &server->dirconSession[app];
    int cliSockFd;
    socklen_t addrLen = sizeof (sess->remCliAddr);

    // Accept the connection from the upstream
    // client app.
    if ((cliSockFd = accept(server->srvSockFd, (struct sockaddr *) &sess->remCliAddr, &addrLen)) < 0) {
        mlog(error, "accept() failed!");
        return -1;
    }

    if (sess->cliSockFd == 0) {
        int enable = true;
        char locAddrBuf[INET_ADDRSTRLEN];
        char remAddrBuf[INET_ADDRSTRLEN];

        // Set NODELAY option to reduce the latency of the
        // messages we send out over this DIRCON session.
        if (setsockopt(cliSockFd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof (enable)) != 0) {
            mlog(error, "setsockopt(TCP_NODELAY) failed!");
            close(cliSockFd);
            return -1;
        }

        // Get our local socket address
        if (getsockname(cliSockFd, (struct sockaddr *) &sess->locCliAddr, &addrLen) < 0) {
            mlog(error, "getsockname() failed!");
            close(cliSockFd);
            return -1;
        }

        inet_ntop(AF_INET, &sess->remCliAddr.sin_addr, remAddrBuf, sizeof (remAddrBuf));
        inet_ntop(AF_INET, &sess->locCliAddr.sin_addr, locAddrBuf, sizeof (locAddrBuf));
        mlog(info, "Client connection established: %s[%u] -> %s[%u]",
                remAddrBuf, ntohs(sess->remCliAddr.sin_port),
                locAddrBuf, ntohs(sess->locCliAddr.sin_port));

        sess->cliSockFd = cliSockFd;
        sess->rxMesgCnt = 0;
        sess->txMesgCnt = 0;
    } else {
        // The server supports only one client at a
        // time!
        mlog(info, "Server supports a single client app connection at a time!");
        close(cliSockFd);
    }

    return 0;
}

int serverProcConnDrop(Server *server, DirconSessId sessId)
{
    DirconSession *sess = &server->dirconSession[sessId];

    mlog(info, "Client disconnected!");

    // Dis-arm the DIRCON timers
    sess->nextNotification.tv_sec = 0;

    // Clean up
    sess->cpmNotificationsEnabled = false;
    sess->ibdNotificationsEnabled = false;
    sess->rxMesgCnt = 0;
    sess->txMesgCnt = 0;
    if (sessId == app)
        server->weight = 0;

    // Close our end of the socket
    close(sess->cliSockFd);
    sess->cliSockFd = 0;
    memset(&sess->locCliAddr, 0, sizeof (sess->locCliAddr));
    memset(&sess->remCliAddr, 0, sizeof (sess->remCliAddr));

    return 0;
}

int serverRun(Server *server)
{
    const struct timeval pollInt = { .tv_sec = 0, .tv_usec = 10000 };   // 10 ms
    struct timeval start = {0};
    struct timeval end = {0};

    // Main work loop
    while (true) {
        struct pollfd pollFdSet[6]; // max 6 fd: stdin, srv, app, dev, mdns, msgq
        struct timeval delta, timeout;
        int numFds, numEnt, msTimeo;

        numFds = 0;

#ifdef CONFIG_CLI
        // CLI console input
        pollFdSet[numFds].fd = server->stdinFd;
        pollFdSet[numFds].events = POLLIN;
        pollFdSet[numFds++].revents = 0;
#endif

        // Server socket input
        pollFdSet[numFds].fd = server->srvSockFd;
        pollFdSet[numFds].events = POLLIN;
        pollFdSet[numFds++].revents = 0;

        // App client socket input
        if (server->dirconSession[app].cliSockFd) {
            pollFdSet[numFds].fd = server->dirconSession[app].cliSockFd;
            pollFdSet[numFds].events = POLLIN | POLLRDHUP;
            pollFdSet[numFds++].revents = 0;
        }

        // Device client socket input
        if (server->dirconSession[dev].cliSockFd) {
            pollFdSet[numFds].fd = server->dirconSession[dev].cliSockFd;
            pollFdSet[numFds].events = POLLIN | POLLRDHUP;
            pollFdSet[numFds++].revents = 0;
        }

#ifdef CONFIG_MDNS_AGENT
        // mDNS socket input
        if (server->mdnsSockFd) {
            pollFdSet[numFds].fd = server->mdnsSockFd;
            pollFdSet[numFds].events = POLLIN;
            pollFdSet[numFds++].revents = 0;
        }
#endif

#ifdef CONFIG_BLE_CONTROLLER
        // BLE socket input
        if (server->bleSockFd) {
            pollFdSet[numFds].fd = server->bleSockFd;
            pollFdSet[numFds].events = POLLIN;
            pollFdSet[numFds++].revents = 0;
        }
#endif

        // Wait for data on any of the specified file
        // descriptors, or until the timeout expires.
        // To keep the timers accurate, we adjust the
        // nominal poll interval by the amount of time
        // spent in the last iteration of the work
        // loop...
        tvSub(&delta, &end, &start);
        if (tvCmp(&pollInt, &delta) > 0) {
            tvSub(&timeout, &pollInt, &delta);
        } else {
            timeout.tv_usec = 0;
        }
        msTimeo = (timeout.tv_usec / 1000);
        if ((numEnt = poll(pollFdSet, numFds, msTimeo)) < 0) {
            if (errno != EINTR) {
                mlog(fatal, "poll() failed!");
                return -1;
            }
            numEnt = 0;
        }

        gettimeofday(&start, NULL);

        // Process DIRCON timers
        dirconProcTimers(server, &start);

#ifdef CONFIG_MDNS_AGENT
        // Process mDNS timers
        mdnsProcTimers(server, &start);
#endif

        if (numEnt) {
            for (int n = 0; (n < numFds); n++) {
                if (pollFdSet[n].revents & POLLIN) {
                    int fd = pollFdSet[n].fd;
#ifdef CONFIG_CLI
                    if (fd == server->stdinFd) {
                        // Process CLI console input
                        cliReadChar();
                    } else
#endif
                    if (fd == server->srvSockFd) {
                        // Process connection request
                        serverProcConnReq(server);
                    } else if (fd == server->dirconSession[app].cliSockFd) {
                        if (pollFdSet[n].revents & POLLRDHUP) {
                            // Process connection drop
                            serverProcConnDrop(server, app);
                        } else {
                            // Process DIRCON message from app
                            dirconProcMesg(server, app);
                        }
                    } else if (fd == server->dirconSession[dev].cliSockFd) {
                        if (pollFdSet[n].revents & POLLRDHUP) {
                            // Process connection drop
                            serverProcConnDrop(server, dev);
                        } else {
                            // Process DIRCON message from device
                            dirconProcMesg(server, dev);
                        }
#ifdef CONFIG_MDNS_AGENT
                    } else if (fd == server->mdnsSockFd) {
                        // Process mDNS message
                        mdnsProcMesg(server);
#endif
                    }
#ifdef CONFIG_BLE_CONTROLLER
                    else if (fd == server->bleSockFd) {
                        // Process BLE message
                        bleProcMesg(server);
                    }
#endif
                }
            }
        }

        gettimeofday(&end, NULL);

        // Exit the tool?
        if (server->exit) {
            cliPreExitCleanup(server);
            break;
        }
    }

    return 0;
}



