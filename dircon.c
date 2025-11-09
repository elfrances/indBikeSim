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
        mlog(debug, "sessId=%s mesgId=%u seqNum=%u mesgLen=%u", fmtDirconSessId(sess->sessId), mesg->mesgId, mesg->seqNum, mesg->mesgLen);
    } else {
        mlog(debug, "sessId=%s mesgId=%u seqNum=%u respCode=%u mesgLen=%u", fmtDirconSessId(sess->sessId), mesg->mesgId, mesg->seqNum, mesg->respCode, mesg->mesgLen);
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
    uint16_t speed = 0;
    uint16_t cadence = server->cadence;
    uint16_t power = server->power;
    uint8_t heartRate = server->heartRate;
#ifdef CONFIG_FIT_ACTIVITY_FILE
    TrkPt *tp = TAILQ_FIRST(&server->trkPtList);

    if ((tp != NULL) && server->actInProg) {
        cadence = tp->cadence;
        power = tp->power;
        heartRate = tp->heartRate;
    }
#endif

    putUINT16(ibd->flags,flags);
    putUINT16(&ibd->data[0], speed);            // speed
    putUINT16(&ibd->data[2], (cadence / 0.5));  // cadence
    putUINT16(&ibd->data[4], power);            // power
    putUINT8(&ibd->data[6], heartRate);         // HR

    return (sizeof (IndoorBikeData) + 7);
}

static int dirconSendUnsolicitedCharacteristicNotificationMesg(Server *server, DirconSession *sess, const Uuid16 *charUuid)
{
    UnsCharNot *unsCharNot = (UnsCharNot *) dirconInitMesg(server, UnsolicitedCharacteristicNotification, ++sess->lastTxReqSeqNum, SuccessRequest);

    buildUuid128(&unsCharNot->charUuid, charUuid);
    unsCharNot->hdr.mesgLen = sizeof (unsCharNot->charUuid);

    if (uuid16Eq(charUuid, &indoorBikeDataUUID)) {
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
    DirconSession *sess;

    // App session
    sess = &server->dirconSession[app];
    if ((sess->nextNotification.tv_sec != 0) &&
        (tvCmp(time, &sess->nextNotification) >= 0)) {
        if (sess->cpmNotificationsEnabled) {
            // Send a Cycling Power Measurement notification
            dirconSendUnsolicitedCharacteristicNotificationMesg(server, sess, &cyclingPowerMeasurementUUID);
        }

        if (sess->ibdNotificationsEnabled) {
            // Send an Indoor Bike Data notification
            dirconSendUnsolicitedCharacteristicNotificationMesg(server, sess, &indoorBikeDataUUID);
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

    // Device session
    sess = &server->dirconSession[dev];
    if ((sess->nextNotification.tv_sec != 0) &&
        (tvCmp(time, &sess->nextNotification) >= 0)) {

        // TBD

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

static int dirconProcDiscoverServicesMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    DirconSessId sessId = sess->sessId;
    Service *svc;

    if ((sessId == app) && (mesgType == request)) {
        DiscSvcsMesg *resp = (DiscSvcsMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
        Uuid128 *svcUuid = resp->svcUuid;

        // Add all the services discovered so far...
        TAILQ_FOREACH(svc, &server->svcList, svcListEnt) {
            resp->hdr.mesgLen += addService(svcUuid++, svc);
        }

        // Send response!
        dirconSendMesg(server, sess, response, (DirconMesg *) resp);
    } else if ((sessId == dev) && (mesgType == response)) {
        // Check the response code
        if (mesg->respCode == SuccessRequest) {
            const DiscSvcsMesg *discSvcsMesg = (DiscSvcsMesg *) mesg;
            const Uuid128 *svcUuid = discSvcsMesg->svcUuid;
            int mesgLen = mesg->mesgLen;

            // Process each characteristic in the list
            while (mesgLen >= sizeof (Uuid128)) {
                // We only care about a few standard services...
                Uuid16 uuid;
                getUuid16(&uuid, svcUuid);
                if (uuid16Eq(&uuid, &deviceInfoUUID) ||
                    uuid16Eq(&uuid, &heartRateServiceUUID) ||
                    uuid16Eq(&uuid, &cyclingPowerServiceUUID) ||
                    uuid16Eq(&uuid, &fitnessMachineServiceUUID)) {
                    if (serverFindService(server, svcUuid) == NULL) {
                        if (serverAddService(server, svcUuid) == NULL) {
                            mlog(error, "Failed to add service: sessId=%s svcUuid=%s", fmtDirconSessId(sessId), fmtUuid128(svcUuid));
                        }
                    } else {
                        mlog(warning, "Ignoring duplicate service: svcUuid=%s", fmtUuid128(svcUuid));
                    }
                }
                svcUuid++;
                mesgLen -= sizeof (Uuid128);
            }
        } else {
            mlog(error, "Transaction failed: sessId=%s seqNum=%u respCode=%u", fmtDirconSessId(sessId), mesg->seqNum, mesg->respCode);
        }

        sess->respPend = false;
    }

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
    DirconSessId sessId = sess->sessId;
    DiscCharsMesg *discChars = (DiscCharsMesg *) mesg;
    const Uuid128 *svcUuid = &discChars->svcUuid;
    Service *svc = serverFindService(server, svcUuid);

    if (mesg->mesgLen < sizeof (discChars->svcUuid))
        return -1;

    if ((sessId == app) && (mesgType == request)) {
        DiscCharsMesg *resp = (DiscCharsMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);

        if (svc != NULL) {
            Characteristic *chr;
            CharProp *prop = resp->charProp;
            resp->svcUuid = discChars->svcUuid;
            resp->hdr.mesgLen = sizeof (resp->svcUuid);

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
    } else if ((sessId == dev) && (mesgType == response)) {
        // Check the response code
        if (mesg->respCode == SuccessRequest) {
            const DiscCharsMesg *resp = (DiscCharsMesg *) mesg;
            const Uuid128 *svcUuid = &resp->svcUuid;

            if (svc != NULL) {
                const CharProp *charProp = resp->charProp;
                int mesgLen = mesg->mesgLen;

                // Process each characteristic in the list
                while (mesgLen >= sizeof (CharProp)) {
                    if (svcFindChar(svc, &charProp->charUuid) == NULL) {
                        if (svcAddChar(svc, &charProp->charUuid, charProp->properties) == NULL) {
                            mlog(error, "Failed to add characteristic: sessId=%s svcUuid=%s charUuid=%s",
                                    fmtDirconSessId(sessId), fmtUuid128(svcUuid), fmtUuid128(&charProp->charUuid));
                        }
                    } else {
                        mlog(warning, "Ignoring duplicate characteristic: charUuid=%s", fmtUuid128(&charProp->charUuid));
                    }
                    charProp++;
                    mesgLen -= sizeof (CharProp);
                }
            } else {
                mlog(error, "Hu? Can't find service! sessId=%s svcUuid=%s", fmtDirconSessId(sessId), fmtUuid128(svcUuid));
            }
        } else {
            mlog(error, "Transaction failed: seqNum=%u respCode=%u", mesg->seqNum, mesg->respCode);
        }

        sess->respPend = false;
    }

    return 0;
}

static int dirconProcReadCharacteristicMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    ReadCharMesg *readChar = (ReadCharMesg *) mesg;

    Uuid16 charUuid;

    if (mesg->mesgLen < sizeof (readChar->charUuid))
        return -1;

    getUuid16(&charUuid, &readChar->charUuid);

    if (mesgType == request) {
        ReadCharMesg *resp = (ReadCharMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
        resp->charUuid = readChar->charUuid;
        resp->hdr.mesgLen = sizeof (resp->charUuid);

        if (uuid16Eq(&charUuid, &fitnessMachineFeatureUUID)) {
            // Fitness Machine Features (FTMS 4.3.1.1)
            FitMachFeat *fmf = (FitMachFeat *) resp->data;
            uint32_t fmFeat = FMF_CADENCE | FMF_HEART_RATE_MEASURMENT | FMF_POWER_MEASUREMENT;
            uint32_t tsFeat = TSF_RESISTANCE | TSF_POWER | TSF_INDOOR_BIKE_SIM_PARMS;
            putUINT32(fmf->fmFeat, fmFeat);
            putUINT32(fmf->tsFeat, tsFeat);
            resp->hdr.mesgLen += sizeof (FitMachFeat);
        } else if (uuid16Eq(&charUuid, &trainingStatusUUID)) {

        } else if (uuid16Eq(&charUuid, &supportedResistanceLevelRangeUUID)) {

        } else if (uuid16Eq(&charUuid, &supportedPowerRangeUUID)) {

        } else if (uuid16Eq(&charUuid, &fitnessMachineStatusUUID)) {

        } else {
            // This characteristic is not readable!
            resp->hdr.respCode = CharacteristicOperationNotSupported;
        }

        // Send response!
        dirconSendMesg(server, sess, response, (DirconMesg *) resp);
    } else {
        // Check the response code
        if (mesg->respCode != SuccessRequest) {
            mlog(error, "Transaction failed: seqNum=%u respCode=%u", mesg->seqNum, mesg->respCode);
        }

        sess->respPend = false;
    }

    return 0;
}

static int dirconProcWriteCharacteristicMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    WriteCharMesg *writeChar = (WriteCharMesg *) mesg;

    Uuid16 charUuid;

    if (mesg->mesgLen < sizeof (writeChar->charUuid))
        return -1;

    getUuid16(&charUuid, &writeChar->charUuid);

    if (mesgType == request) {
        WriteCharMesg *resp = (WriteCharMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
        resp->charUuid = writeChar->charUuid;
        resp->hdr.mesgLen = sizeof (resp->charUuid);

        if (uuid16Eq(&charUuid, &cyclingPowerControlPointUUID)) {
            // Cycling Power Control Point
        } else if (uuid16Eq(&charUuid, &weightUUID)) {
            // Weight
            server->weight = getUINT16(writeChar->data);
        } else if (uuid16Eq(&charUuid, &fitnessMachineControlPointUUID)) {
            // Fitness Machine Control Point
            const FitMachCP *fmcp = (FitMachCP *) writeChar->data;
            if (fmcp->opCode == FMCP_SET_INDOOR_BIKE_SIM_PARMS) {
                // If the Cycling Power Measurement notifications
                // have been enabled, and the Weight characteristic
                // has been set, then the activity has started...
                if (sess->cpmNotificationsEnabled && (server->weight != 0)) {
                    server->actInProg = true;
                }
            }
        } else {
            // This characteristic is not writable!
            resp->hdr.respCode = CharacteristicOperationNotSupported;
        }

        // Send response!
        dirconSendMesg(server, sess, response, (DirconMesg *) resp);
    } else {
        // Check the response code
        if (mesg->respCode != SuccessRequest) {
            mlog(error, "Transaction failed: seqNum=%u respCode=%u", mesg->seqNum, mesg->respCode);
        }

        sess->respPend = false;
    }

    return 0;
}

static int dirconProcEnableCharacteristicNotificationsMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg)
{
    EnCharNotMesg *req = (EnCharNotMesg *) mesg;
    Uuid16 charUuid;
    struct timeval now;

    if (mesg->mesgLen < sizeof (Uuid128))
        return -1;

    getUuid16(&charUuid, &req->charUuid);

    gettimeofday(&now, NULL);

    if (mesgType == request) {
        bool enable = (req->enable & 0x01) ? true : false;
        EnCharNotMesg *resp = (EnCharNotMesg *) dirconInitMesg(server, mesg->mesgId, mesg->seqNum, SuccessRequest);
        resp->charUuid = req->charUuid;
        resp->enable = req->enable;
        resp->hdr.mesgLen = sizeof (resp->charUuid);
        if (uuid16Eq(&charUuid, &cyclingPowerMeasurementUUID)) {
            // Cycling Power Measurement
            if (enable) {
                if (sess->nextNotification.tv_sec == 0) {
                    // Start the notification timer with a 1-sec expiry
                    sess->nextNotification = now;
                    sess->nextNotification.tv_sec++;
                }
            }
            sess->cpmNotificationsEnabled = enable;
        } else if (uuid16Eq(&charUuid, &cyclingPowerControlPointUUID)) {
            // Cycling Power Control Point
            //   TBD
        } else if (uuid16Eq(&charUuid, &indoorBikeDataUUID)) {
            // Indoor Bike Data
            if (enable) {
                if (sess->nextNotification.tv_sec == 0) {
                    // Start the notification timer with a 1-sec expiry
                    sess->nextNotification = now;
                    sess->nextNotification.tv_sec++;
                }
            }
            sess->ibdNotificationsEnabled = enable;
        } else if (uuid16Eq(&charUuid, &trainingStatusUUID)) {
            // Training Status
            //   TBD
        } else if (uuid16Eq(&charUuid, &fitnessMachineControlPointUUID)) {
            // Fitness Machine Control Point
            //   TBD
        } else if (uuid16Eq(&charUuid, &fitnessMachineStatusUUID)) {
            // Fitness Machine Status
            //   TBD
        } else {
            // Notifications not supported on this
            // characteristic!
            resp->hdr.respCode = CharacteristicOperationNotSupported;
        }

        // Send response!
        dirconSendMesg(server, sess, response, (DirconMesg *) resp);
    } else {
        // Check the response code
        if (mesg->respCode != SuccessRequest) {
            mlog(error, "Transaction failed: seqNum=%u respCode=%u", mesg->seqNum, mesg->respCode);
        }

        sess->respPend = false;
    }

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

static int dirconForwardMesg(Server *server, DirconSession *sess, MesgType mesgType, const DirconMesg *mesg, int mesgLen)
{
    DirconSessId otherSessId = swapDirconSessId(sess->sessId);
    DirconMesg *fwdMesg = (DirconMesg *) server->txMesgBuf;

    memcpy(fwdMesg, mesg, (sizeof (DirconMesg) + mesgLen));

    return dirconSendMesg(server, &server->dirconSession[otherSessId], mesgType, fwdMesg);
}

int dirconProcMesg(Server *server, DirconSessId sessId)
{
    DirconSession *sess = &server->dirconSession[sessId];
    DirconMesg *mesg = (DirconMesg *) server->rxMesgBuf;
    struct timeval timestamp;
    int n, mesgLen;
    MesgType mesgType;

    gettimeofday(&timestamp, NULL);

    // Read the message header
    if ((n = recv(sess->cliSockFd, mesg, sizeof (DirconMesg), 0)) != sizeof (DirconMesg)) {
        if ((n == 0) && (errno == 0)) {
            // Connection dropped
            return serverProcConnDrop(server, sessId);
#ifdef ESP_PLATFORM
        } else if ((n == 0) && (errno = ENOTCONN)) {
            // Connection dropped
            return serverProcConnDrop(server, sessId);
#endif
        } else if ((n == -1) && (errno = ETIMEDOUT)) {
            // TCP KA timeout
            return serverProcConnDrop(server, sessId);
        } else {
            mlog(fatal, "Failed to receive message header! sessId=%s fd=%d n=%d",
                    fmtDirconSessId(sessId), sess->cliSockFd, n);
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
            mlog(fatal, "Failed to receive message data! sessId=%s fd=%d n=%d",
                    fmtDirconSessId(sessId), sess->cliSockFd, n);
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
        mlog(debug, "sessId=%s mesgId=%u seqNum=%u mesgLen=%u", fmtDirconSessId(sessId), mesg->mesgId, mesg->seqNum, mesg->mesgLen);
    } else {
        mlog(debug, "sessId=%s mesgId=%u seqNum=%u respCode=%u mesgLen=%u", fmtDirconSessId(sessId), mesg->mesgId, mesg->seqNum, mesg->respCode, mesg->mesgLen);
    }

    if (server->dissect) {
        dirconDumpMesg(&timestamp, server, sess, RxDir, mesgType, mesg);
    }

    // If message forwarding is enabled, and this message is not a
    // DiscoverServices or DiscoverCharacteristics, then just relay
    // the DIRCON message to the other session...
    if (server->mesgForwarding && (mesg->mesgId != DiscoverServices) && (mesg->mesgId != DiscoverCharacteristics)) {
        return dirconForwardMesg(server, sess, mesgType, mesg, mesgLen);
    }

    // Call the Rx message handler to do the work!
    return (*rxMesgHandler[mesg->mesgId])(server, sess, mesgType, mesg);
}

