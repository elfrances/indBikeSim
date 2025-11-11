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

#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "dircon.h"
#include "dump.h"
#include "ftms.h"
#include "mlog.h"
#include "server.h"

// GET signed values

int8_t getSINT8(const uint8_t *data)
{
    uint8_t value = data[0];
    return (int8_t) value;
}

int16_t getSINT16(const uint8_t *data)
{
    uint16_t value = ((uint16_t) data[1] << 8) | (uint16_t) data[0];
    return (int16_t) value;
}

int32_t getSINT24(const uint8_t *data)
{
    uint32_t value = ((uint32_t) data[2] <<16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
    return (int32_t) value;
}

int32_t getSINT32(const uint8_t *data)
{
    uint32_t value = ((uint32_t) data[3] << 24) | ((uint32_t) data[2] <<16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
    return (int32_t) value;
}

// GET unsigned values

uint8_t getUINT8(const uint8_t *data)
{
    uint8_t value = data[0];
    return value;
}

uint16_t getUINT16(const uint8_t *data)
{
    uint16_t value = ((uint16_t) data[1] << 8) | (uint16_t) data[0];
    return value;
}

uint32_t getUINT24(const uint8_t *data)
{
    uint32_t value = ((uint32_t) data[2] <<16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
    return value;
}

uint32_t getUINT32(const uint8_t *data)
{
    uint32_t value = ((uint32_t) data[3] << 24) | ((uint32_t) data[2] << 16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
    return value;
}

// PUT signed values

void putSINT16(uint8_t *data, int16_t value)
{
    *data++ = (value & 0xff);
    *data = ((value >> 8) & 0xff);
}

// PUT unsigned values

void putUINT8(uint8_t *data, uint8_t value)
{
    *data = value;
}

void putUINT16(uint8_t *data, uint16_t value)
{
    *data++ = (value & 0xff);
    *data = ((value >> 8) & 0xff);
}

void putUINT24(uint8_t *data, uint32_t value)
{
    *data++ = (value & 0xff);
    *data++ = ((value >> 8) & 0xff);
    *data = ((value >> 16) & 0xff);
}

void putUINT32(uint8_t *data, uint32_t value)
{
    *data++ = (value & 0xff);
    *data++ = ((value >> 8) & 0xff);
    *data++ = ((value >> 16) & 0xff);
    *data = ((value >> 24) & 0xff);
}

static int dirconSendMesg(Server *server, DirconSession *sess, MesgType mesgType, DirconMesg *mesg)
{
    int pduLen = sizeof (DirconMesg) + mesg->mesgLen;
    struct timeval timestamp;

    sess->txMesgCnt++;

    gettimeofday(&timestamp, NULL);

    if (mesgType == request) {
        mlog(debug, "mesgId=%u seqNum=%u mesgLen=%u", mesg->mesgId, mesg->seqNum, mesg->mesgLen);
    } else {
        mlog(debug, "mesgId=%u seqNum=%u respCode=%u mesgLen=%u", mesg->mesgId, mesg->seqNum, mesg->respCode, mesg->mesgLen);
    }

    if (server->dissect)
        dirconDumpMesg(&timestamp, server, sess, TxDir, mesgType, mesg);

    mesg->mesgLen = htons(mesg->mesgLen);

    if (send(sess->cliSockFd, mesg, pduLen, 0) != pduLen) {
        mlog(error, "Failed to send DIRCON message!");
        return -1;
    }

    if (mesgType == request)
        sess->respPend = true;

    return 0;
}

static DirconMesg *dirconInitMesg(Server *server, DirconMesgId mesgId,
                                  uint8_t seqNum, DirconRespCode respCode)
{
    DirconMesg *mesg = (DirconMesg *) server->txMesgBuf;

    mesg->version = DIRCON_VERSION;
    mesg->mesgId = mesgId;
    mesg->seqNum = seqNum;
    mesg->respCode = respCode;
    mesg->mesgLen = 0;  // set by the Rx/Tx handler

    return mesg;
}

int dirconSendDiscoverServicesMesg(Server *server, DirconSession *sess)
{
    DiscSvcsMesg *discServ;

    discServ = (DiscSvcsMesg *) dirconInitMesg(server, DiscoverServices, ++sess->lastTxReqSeqNum, SuccessRequest);

    return dirconSendMesg(server, sess, request, (DirconMesg *) discServ);
}

int dirconSendDiscoverCharacteristicsMesg(Server *server, DirconSession *sess, const Uuid128 *svcUuid)
{
    DiscCharsMesg *discChar;

    discChar = (DiscCharsMesg *) dirconInitMesg(server, DiscoverCharacteristics, ++sess->lastTxReqSeqNum, SuccessRequest);
    discChar->svcUuid = *svcUuid;
    discChar->hdr.mesgLen = sizeof (discChar->svcUuid);

    return dirconSendMesg(server, sess, request, (DirconMesg *) discChar);
}

int dirconSendReadCharacteristicMesg(Server *server, DirconSession *sess, const Uuid128 *uuid)
{
    ReadCharMesg *readChar;

    readChar = (ReadCharMesg *) dirconInitMesg(server, ReadCharacteristic, ++sess->lastTxReqSeqNum, SuccessRequest);
    readChar->charUuid = *uuid;
    readChar->hdr.mesgLen = sizeof (readChar->charUuid);

    return dirconSendMesg(server, sess, request, (DirconMesg *) readChar);
}

static uint16_t initIndoorBikeDataChar(Server *server, IndoorBikeData *ibd)
{
    uint16_t flags = IBD_INSTANTANEOUS_CADENCE | IBD_INSTANTANEOUS_POWER | IBD_HEART_RATE;
    uint16_t speed = server->speed;
    uint16_t cadence = server->cadence;
    uint16_t power = server->power;
    uint8_t heartRate = server->heartRate;
#ifdef CONFIG_FIT_ACTIVITY_FILE
    TrkPt *tp = TAILQ_FIRST(&server->trkPtList);

    if ((tp != NULL) && server->actInProg) {
        speed = tp->speed;
        cadence = tp->cadence;
        power = tp->power;
        heartRate = tp->heartRate;
    }
#endif

    putUINT16(ibd->flags,flags);
    putUINT16(&ibd->data[0], speed);    // speed
    putUINT16(&ibd->data[2], cadence);  // cadence
    putUINT16(&ibd->data[4], power);    // power
    putUINT8(&ibd->data[6], heartRate); // HR

    return (sizeof (IndoorBikeData) + 7);
}

static int dirconSendUnsolicitedCharacteristicNotificationMesg(Server *server, DirconSession *sess, uint16_t charUuid)
{
    UnsCharNot *unsCharNot = (UnsCharNot *) dirconInitMesg(server, UnsolicitedCharacteristicNotification, ++sess->lastTxReqSeqNum, SuccessRequest);

    uint16ToUuid128(&unsCharNot->charUuid, charUuid);
    unsCharNot->hdr.mesgLen = sizeof (unsCharNot->charUuid);

    if (charUuid == indoorBikeData) {
        // Indoor Bike Data
        IndoorBikeData *ibd = (IndoorBikeData *) unsCharNot->data;
        unsCharNot->hdr.mesgLen += initIndoorBikeDataChar(server, ibd);
    } else {
        // Hu?
        return -1;
    }

    return dirconSendMesg(server, sess, request, (DirconMesg *) unsCharNot);
}

int dirconProcTimers(Server *server, const struct timeval *time)
{
    DirconSession *sess = &server->dirconSession;

    if ((sess->nextNotification.tv_sec != 0) &&
        (tvCmp(time, &sess->nextNotification) >= 0)) {
        if (sess->ibdNotificationsEnabled) {
            // Send an Indoor Bike Data notification
            dirconSendUnsolicitedCharacteristicNotificationMesg(server, sess, indoorBikeData);
        }

#ifdef CONFIG_FIT_ACTIVITY_FILE
        {
            TrkPt *tp = TAILQ_FIRST(&server->trkPtList);

            if ((tp != NULL) && server->actInProg) {
                // Remove this trackpoint and move on to
                // the next one...
                TAILQ_REMOVE(&server->trkPtList, tp, tqEntry);
                trkPtFree(tp);
            }
        }
#endif

        // Re-arm the timer with a 1-sec expiry
        sess->nextNotification.tv_sec++;
    }

    return 0;
}

static int addService(Uuid128 *svcUuid, const Service *svc)
{
    *svcUuid = svc->uuid;
    return sizeof (Uuid128);
}

// Common error response handler for ReadCharacteristic,
// WriteCharacteristic, and EnableCharacteristicNotifications.
static int dirconSendErrorResp(Server *server, DirconSession *sess, const DirconMesg *mesg, DirconRespCode respCode, const Uuid128 *uuid)
{
    ReadCharMesg *resp = (ReadCharMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, respCode);
    resp->charUuid = *uuid;

    return dirconSendMesg(server, sess, response, (DirconMesg *) resp);
}

static int dirconProcDiscoverServicesMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    DiscSvcsMesg *resp = (DiscSvcsMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
    Uuid128 *svcUuid = resp->svcUuid;
    Service *svc;

    // Add all the services discovered so far...
    TAILQ_FOREACH(svc, &server->svcList, svcListEnt) {
        resp->hdr.mesgLen += addService(svcUuid++, svc);
    }

    // Send response!
    dirconSendMesg(server, sess, response, (DirconMesg *) resp);

    return 0;
}

static int addCharProp(CharProp *charProp, const Characteristic *chr)
{
    charProp->charUuid = chr->uuid;
    charProp->properties = (chr->properties & DIRCON_CHAR_PROP_MASK);
    return sizeof (CharProp);
}

static int dirconProcDiscoverCharacteristicsMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    DiscCharsMesg *discChars = (DiscCharsMesg *) mesg;
    const Uuid128 *svcUuid = &discChars->svcUuid;
    Service *svc;

    if (mesg->mesgLen < sizeof (discChars->svcUuid))
        return -1;

    {
        DiscCharsMesg *resp = (DiscCharsMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
        resp->svcUuid = discChars->svcUuid;
        resp->hdr.mesgLen = sizeof (resp->svcUuid);

        if ((svc = serverFindService(server, svcUuid)) != NULL) {
            Characteristic *chr;
            CharProp *prop = resp->charProp;

            // Add all the characteristics in this service
            // to the response...
            TAILQ_FOREACH(chr, &svc->charList, charListEnt) {
                resp->hdr.mesgLen += addCharProp(prop++, chr);
            }
        } else {
            // Unsupported service!
            resp->hdr.respCode = ServiceNotFound;
        }

        // Send response!
        dirconSendMesg(server, sess, response, (DirconMesg *) resp);
    }

    return 0;
}

static int dirconProcReadCharacteristicMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    ReadCharMesg *readChar = (ReadCharMesg *) mesg;

    if (mesg->mesgLen < sizeof (readChar->charUuid))
        return -1;

    Characteristic *chr = serverFindCharacteristicByUuid128(server, &readChar->charUuid);

    if (chr == NULL) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicNotFound, &readChar->charUuid);
    }

    if (!(chr->properties & DIRCON_CHAR_PROP_READ)) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicOperationNotSupported, &readChar->charUuid);
    }

    ReadCharMesg *resp = (ReadCharMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
    resp->charUuid = readChar->charUuid;
    resp->hdr.mesgLen = sizeof (resp->charUuid);

    if (chr->uuid16 == fitnessMachineFeature) {
        // Fitness Machine Features (FTMS 4.3.1.1)
        FitMachFeat *fmf = (FitMachFeat *) resp->data;
        uint32_t fmFeat = FMF_CADENCE | FMF_HEART_RATE_MEASURMENT | FMF_POWER_MEASUREMENT;
        uint32_t tsFeat = TSF_POWER | TSF_INDOOR_BIKE_SIM_PARMS;
        putUINT32(fmf->fmFeat, fmFeat);
        putUINT32(fmf->tsFeat, tsFeat);
        resp->hdr.mesgLen += sizeof (FitMachFeat);
    } else if (chr->uuid16 == supportedPowerRange) {
        // Supported Power Range (FTMS 4.14)
        putUINT16(&resp->data[0], server->minPower);
        putUINT16(&resp->data[2], server->maxPower);
        putUINT16(&resp->data[4], server->incPower);
        resp->hdr.mesgLen += 6;
    } else {
        // Hu?
        resp->hdr.respCode = UnexpectedError;
    }

    // Send response!
    dirconSendMesg(server, sess, response, (DirconMesg *) resp);

    return 0;
}

static void schedFmcpNotification(Server *server, const Characteristic *chr, uint8_t opCode, uint8_t resultCode)
{
    server->cpRespInfo.chr = chr;
    server->cpRespInfo.respCode = FMCP_RESPONSE_CODE;
    server->cpRespInfo.reqOpCode = opCode;
    server->cpRespInfo.resultCode = resultCode;
}

static int dirconProcWriteCharacteristicMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    WriteCharMesg *writeChar = (WriteCharMesg *) mesg;

    if (mesg->mesgLen < sizeof (writeChar->charUuid))
        return -1;

    Characteristic *chr = serverFindCharacteristicByUuid128(server, &writeChar->charUuid);

    if (chr == NULL) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicNotFound, &writeChar->charUuid);
    }

    if (!(chr->properties & DIRCON_CHAR_PROP_WRITE)) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicOperationNotSupported, &writeChar->charUuid);
    }

    WriteCharMesg *resp = (WriteCharMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
    resp->charUuid = writeChar->charUuid;
    resp->hdr.mesgLen = sizeof (resp->charUuid);

    if (chr->uuid16 == fitnessMachineControlPoint) {
        // Fitness Machine Control Point
        const FitMachCP *fmcp = (FitMachCP *) writeChar->data;
        int resultCode = FMCP_RC_SUCCESS;
        if ((fmcp->opCode != FMCP_REQUEST_CONTROL) && !server->controlGranted) {
            resultCode = FMCP_RC_CONTROL_NOT_PERMITTED;
        } else if (fmcp->opCode == FMCP_REQUEST_CONTROL) {
            server->controlGranted = true;
        } else if (fmcp->opCode == FMCP_RESET) {
            server->controlGranted = false;
        } else if (fmcp->opCode == FMCP_SET_TGT_POWER) {
            // TBD
        } else if (fmcp->opCode == FMCP_START_OR_RESUME) {
            // TBD
        } else if (fmcp->opCode == FMCP_STOP_OR_PAUSE) {
            // TBD
        } else if (fmcp->opCode == FMCP_SET_INDOOR_BIKE_SIM_PARMS) {
            // TBD
        } else if (fmcp->opCode == FMCP_SET_WHEEL_CIRCUMFERENCE) {
            // TBD
        } else {
            resultCode = FMCP_RC_OP_CODE_NOT_SUPPORTED;
        }

        // Schedule the NOTIFICATION that follows the WRITE Response
        schedFmcpNotification(server, chr, fmcp->opCode, resultCode);
    } else {
        // Hu?
        resp->hdr.respCode = UnexpectedError;
    }

    // Send response!
    dirconSendMesg(server, sess, response, (DirconMesg *) resp);

    return 0;
}

static int dirconProcEnableCharacteristicNotificationsMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    EnCharNotMesg *enCharNot = (EnCharNotMesg *) mesg;
    struct timeval now;

    if (mesg->mesgLen < sizeof (Uuid128))
        return -1;

    Characteristic *chr = serverFindCharacteristicByUuid128(server, &enCharNot->charUuid);

    if (chr == NULL) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicNotFound, &enCharNot->charUuid);
    }

    if (!(chr->properties & DIRCON_CHAR_PROP_NOTIFY)) {
        return dirconSendErrorResp(server, sess, mesg, CharacteristicOperationNotSupported, &enCharNot->charUuid);
    }

    gettimeofday(&now, NULL);

    bool enable = (enCharNot->enable & 0x01) ? true : false;
    EnCharNotMesg *resp = (EnCharNotMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
    resp->charUuid = enCharNot->charUuid;
    resp->enable = enCharNot->enable;
    resp->hdr.mesgLen = sizeof (resp->charUuid);

    if (chr->uuid16 == indoorBikeData) {
        // Indoor Bike Data
        if (enable) {
            if (sess->nextNotification.tv_sec == 0) {
                // Start the notification timer with a 1-sec expiry
                sess->nextNotification = now;
                sess->nextNotification.tv_sec++;
            }
        } else {
            sess->nextNotification.tv_sec = 0;
            sess->nextNotification.tv_usec = 0;
        }
        sess->ibdNotificationsEnabled = enable;
    } else if (chr->uuid16 == fitnessMachineControlPoint) {
        // Fitness Machine Control Point
        sess->fmcpNotificationsEnabled = enable;
    } else if (chr->uuid16 == fitnessMachineStatus) {
        // Fitness Machine Status
        //   TBD
    } else {
        // Hu?
        resp->hdr.respCode = UnexpectedError;
    }

    // Send response!
    dirconSendMesg(server, sess, response, (DirconMesg *) resp);

    return 0;
}

static int dirconProcUnsolicitedCharacteristicNotificationMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    return 0;
}

typedef int (*RxMesgHandler)(Server *, DirconSession *, MesgType, const DirconMesg *);

static RxMesgHandler rxMesgHandler[] = {
        [DiscoverServices] = dirconProcDiscoverServicesMesg,
        [DiscoverCharacteristics] = dirconProcDiscoverCharacteristicsMesg,
        [ReadCharacteristic] = dirconProcReadCharacteristicMesg,
        [WriteCharacteristic] = dirconProcWriteCharacteristicMesg,
        [EnableCharacteristicNotifications] = dirconProcEnableCharacteristicNotificationsMesg,
        [UnsolicitedCharacteristicNotification] = dirconProcUnsolicitedCharacteristicNotificationMesg,
};

int dirconProcMesg(Server *server)
{
    DirconSession *sess = &server->dirconSession;
    DirconMesg *mesg = (DirconMesg *) server->rxMesgBuf;
    struct timeval timestamp;
    int n, mesgLen;
    MesgType mesgType;

    gettimeofday(&timestamp, NULL);

    // Read the message header
    if ((n = recv(sess->cliSockFd, mesg, sizeof (DirconMesg), 0)) != sizeof (DirconMesg)) {
        if ((n == 0) && (errno == 0)) {
            // Connection dropped
            return serverProcConnDrop(server);
        } else if ((n == -1) && (errno = ETIMEDOUT)) {
            // TCP KA timeout
            return serverProcConnDrop(server);
        } else {
            mlog(fatal, "Failed to receive message header! fd=%d n=%d", sess->cliSockFd, n);
            return -1;
        }
    }

    mesgLen = ntohs(mesg->mesgLen);

    if ((sizeof (DirconMesg) + mesgLen) > sizeof (server->rxMesgBuf)) {
        mlog(error, "DIRCON message length (%zu) is way too large!", (sizeof (DirconMesg) + mesgLen));
        return -1;
    }

    if (mesgLen != 0) {
        // Read enough data to complete the full message
        if ((n = recv(sess->cliSockFd, mesg->data, mesgLen, 0)) != mesgLen) {
            mlog(fatal, "Failed to receive message data! fd=%d n=%d", sess->cliSockFd, n);
            return -1;
        }
    }

    if (mesg->version != DIRCON_VERSION) {
        mlog(error, "Unexpected protocol version %u!\n", mesg->version);
        return -1;
    }

    if ((mesg->mesgId < DiscoverServices) || (mesg->mesgId > UnsolicitedCharacteristicNotification)) {
        mlog(error, "Invalid DIRCON message type %u!", mesg->mesgId);
        return -1;
    }

    // Figure out if this is a request or a response message.
    // If we have a response pending and the sequence number
    // matches that of the last request we sent out, then it
    // is a response message.
    if (sess->respPend && (mesg->seqNum == sess->lastTxReqSeqNum)) {
        mesgType = response;
    } else {
        mesgType = request;
    }

    sess->rxMesgCnt++;

    mesg->mesgLen = mesgLen;

    if (mesgType == request) {
        mlog(debug, "mesgId=%u seqNum=%u mesgLen=%u", mesg->mesgId, mesg->seqNum, mesg->mesgLen);
    } else {
        mlog(debug, "mesgId=%u seqNum=%u respCode=%u mesgLen=%u", mesg->mesgId, mesg->seqNum, mesg->respCode, mesg->mesgLen);
    }

    if (server->dissect) {
        dirconDumpMesg(&timestamp, server, sess, RxDir, mesgType, mesg);
    }

    // We only expect request messages from the virtual cycling
    // app, so we silently drop any unsolicited response messages.
    if (mesgType != request) {
        mlog(warning, "Unsolicited response message: mesgId=%u seqNum=%u respCode=%u mesgLen=%u (lastRxReqSeqNum=%u)",
                mesg->mesgId, mesg->seqNum, mesg->respCode, mesg->mesgLen, sess->lastRxReqSeqNum);
        return 0;
    }

    // Call the Rx message handler to do the work!
    (*rxMesgHandler[mesg->mesgId])(server, sess, mesgType, mesg);

    // Do we have to send a NOTIFY to complete a WRITE to a
    // Control Point characteristic? This happens whenever:
    //   (1) it is a mock characteristic
    //   (2) it is a real characteristic but the WRITE was suppressed
    //
    if (server->cpRespInfo.chr != NULL) {
        UnsCharNot *unsCharNot = (UnsCharNot *) dirconInitMesg(server, UnsolicitedCharacteristicNotification, ++sess->lastTxReqSeqNum, SuccessRequest);
        unsCharNot->charUuid = server->cpRespInfo.chr->uuid;
        unsCharNot->data[0] = server->cpRespInfo.respCode;
        unsCharNot->data[1] = server->cpRespInfo.reqOpCode;
        unsCharNot->data[2] = server->cpRespInfo.resultCode;
        unsCharNot->hdr.mesgLen = sizeof (unsCharNot->charUuid) + 3;
        if (dirconSendMesg(server, sess, request, &unsCharNot->hdr) != 0) {
            mlog(error, "Failed to send NOTIFY message!");
        }
        server->cpRespInfo.chr = NULL;
    }

    return 0;
}

