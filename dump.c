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

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "binbuf.h"
#include "dircon.h"
#include "dump.h"
#include "fmtbuf.h"
#include "ftms.h"
#include "uuid.h"

// Static FmtBuf used to dump the dissected message data
static char dumpStrBuf[2048];
static FmtBuf dumpFmtBuf;
static FmtBuf *fmtBuf = &dumpFmtBuf;

static const char *fmtMesgDir(MesgDir dir)
{
    return (dir == TxDir) ? "Tx" : "Rx";
}

static const char *fmtMesgId(DirconMesgId mesgId)
{
    const char *mesgIdName = "???";
    static const char *mesgIdNameTable[] = {
        [DiscoverServices] = "Discover Services",
        [DiscoverCharacteristics] = "Discover Characteristics",
        [ReadCharacteristic] = "Read Characteristic",
        [WriteCharacteristic] = "Write Characteristic",
        [EnableCharacteristicNotifications] = "Enable Characteristic Notifications",
        [UnsolicitedCharacteristicNotification] = "Unsolicited Characteristic Notification",
    };

    if ((mesgId >= DiscoverServices) && (mesgId <= UnsolicitedCharacteristicNotification)) {
        mesgIdName = mesgIdNameTable[mesgId];
    } else if (mesgId == Error) {
        mesgIdName = "Error";
    }

    return mesgIdName;
}

static const char *fmtRespCode(DirconRespCode respCode)
{
    const char *respCodeName = "???";
    static const char *respCodeNameTable[] = {
        [SuccessRequest] = "Success Request",
        [UnknownMessageType] = "Unknown Message Type",
        [UnexpectedError] = "Unexpected Error",
        [ServiceNotFound] = "Service Not Found",
        [CharacteristicNotFound] = "Characteristic Not Found",
        [CharacteristicOperationNotSupported] = "Characteristic Operation Not Supported",
        [CharacteristicWriteFailed] = "Characteristic Write Failed",
        [UnknownProtocol] = "Unknown Protocol",
    };

    if ((respCode >= SuccessRequest) && (respCode <= UnknownProtocol)) {
        respCodeName = respCodeNameTable[respCode];
    }

    return respCodeName;
}

static const char *fmtCharProp(uint8_t prop)
{
    static char fmtBuf[64];
    int len = sizeof (fmtBuf);
    int n = 0;
    if (prop & DIRCON_CHAR_PROP_READ)
        n += snprintf((fmtBuf + n), (len - n), "READ,");
    if (prop & DIRCON_CHAR_PROP_WRITE)
        n += snprintf((fmtBuf + n), (len - n), "WRITE,");
    if (prop & DIRCON_CHAR_PROP_NOTIFY)
        n += snprintf((fmtBuf + n), (len - n), "NOTIFY,");
    if (n != 0) {
        fmtBuf[n-1] = '\0';
    }
    return fmtBuf;
}

static const char *fmtIndBikeSimParms(const IndBikeSimParms *ibsp)
{
    static char buf[128];
    FmtBuf fmtBuf;

    fmtBufInit(&fmtBuf, buf, sizeof (buf));

    fmtBufAppend(&fmtBuf, "windSpeed: %.3lf [mps]\n", (getUINT16(ibsp->windSpeed) / 1000.0));
    fmtBufAppend(&fmtBuf, "grade: %.3lf [%%]\n", (getSINT16(ibsp->grade) / 100.0));
    fmtBufAppend(&fmtBuf, "crr: %.5lf\n", (ibsp->crr / 10000.0));
    fmtBufAppend(&fmtBuf, "cw: %.3lf [kg/m]\n", (ibsp->cw / 100.0));

    return fmtBuf.buf;
}

static const char *fmtFitMachCPOpCode(uint8_t opCode)
{
    const char *name = "???";

    static const char *opCodeNameTbl[] = {
        [FMCP_REQUEST_CONTROL] = "Request Control",
        [FMCP_RESET] = "Reset",
        [FMCP_SET_TGT_SPEED] = "Set Target Speed",
        [FMCP_SET_TGT_INCLINATION] = "Set Target Inclination",
        [FMCP_SET_TGT_RESISTANCE] = "Set Target Resistance",
        [FMCP_SET_TGT_POWER] = "Set Target Power",
        [FMCP_SET_TGT_HEART_RATE] = "Set Target Heart Rate",
        [FMCP_START_OR_RESUME] = "Start or Resume",
        [FMCP_STOP_OR_PAUSE] = "Stop or Pause",
        [FMCP_SET_TGT_EXP_ENERGY] = "Set Target Exp Energy",
        [FMCP_SET_TGT_NUM_STEPS] = "Set Target Number of Steps",
        [FMCP_SET_TGT_NUM_STRIDES] = "Set Target Number of Strides",
        [FMCP_SET_TGT_DISTANCE] = "Set Target Distance",
        [FMCP_SET_TGT_TRAINING_TIME] = "Set Target Training Time",
        [FMCP_SET_TGT_TIME_2_HRZ] = "Set Target Time in Two HR Zone",
        [FMCP_SET_TGT_TIME_3_HRZ] = "Set Target Time in Three HR Zone",
        [FMCP_SET_TGT_TIME_5_HRZ] = "Set Target Time in Five HR Zone",
        [FMCP_SET_INDOOR_BIKE_SIM_PARMS] = "Set Indoor Bike Simulation Parameters",
        [FMCP_SET_WHEEL_CIRCUMFERENCE] = "Set Wheel Circumference",
        [FMCP_SET_SPIN_DOWN_CONTROL] = "Set Spin Down Control",
        [FMCP_SET_TGT_CADENCE] = "Set Target Cadence",
    };

    if (opCode <= FMCP_SET_TGT_CADENCE)
        name = opCodeNameTbl[opCode];
    else if (opCode == FMCP_SET_TGT_RESPONSE_CODE)
        name = "Response Code";

    return name;
}

static const char *fmtFitMachCP(const FitMachCP *fmcp, int fmcpLen)
{
    static char buf[512];
    FmtBuf fmtBuf;
    int parmLen = fmcpLen - sizeof (FitMachCP);

    fmtBufInit(&fmtBuf, buf, sizeof (buf));

    fmtBufAppend(&fmtBuf, "opCode: 0x%02x (%s)\n", fmcp->opCode, fmtFitMachCPOpCode(fmcp->opCode));
    if (fmcp->opCode == FMCP_SET_TGT_RESISTANCE) {
        fmtBufAppend(&fmtBuf, "targetResistance: %u\n", getUINT16(fmcp->parm));
    } else if (fmcp->opCode == FMCP_SET_TGT_POWER) {
        fmtBufAppend(&fmtBuf, "targetPower: %u [W]\n", getUINT16(fmcp->parm));
    } else if (fmcp->opCode == FMCP_SET_INDOOR_BIKE_SIM_PARMS) {
        const IndBikeSimParms *ibsp = (IndBikeSimParms *) fmcp->parm;
        fmtBufAppend(&fmtBuf, "%s", fmtIndBikeSimParms(ibsp));
    } else if (fmcp->opCode == FMCP_SET_WHEEL_CIRCUMFERENCE) {
        fmtBufAppend(&fmtBuf, "wheelCircumference: %u [mm]\n", getUINT16(fmcp->parm));
    } else if (parmLen != 0) {
        fmtBufAppend(&fmtBuf, "parm: ");
        fmtBufHexDump(&fmtBuf, fmcp->parm, parmLen);
        fmtBufAppend(&fmtBuf, "\n");
    }

    return fmtBuf.buf;
}

static const char *fmtIndoorBikeData(const IndoorBikeData *ibd, size_t ibdLen)
{
    static char buf[512];
    FmtBuf fmtBuf;
    uint16_t flags = getUINT16(ibd->flags);
    BinBuf binBuf;

    fmtBufInit(&fmtBuf, buf, sizeof (buf));
    binBufInit(&binBuf, (uint8_t *) ibd->data, (ibdLen - sizeof (ibd->flags)), littleEndian);

    fmtBufAppend(&fmtBuf, "flags: 0x%04x\n", flags);
    if (!(flags & IBD_MORE_DATA)) {
        fmtBufAppend(&fmtBuf, "instSpeed: %.3lf [kph]\n", (binBufGetUINT16(&binBuf) / 100.0));
    }
    if (flags & IBD_AVERAGE_SPEED) {
        fmtBufAppend(&fmtBuf, "avgSpeed: %.3lf [kph]\n", (binBufGetUINT16(&binBuf) / 100.0));
    }
    if (flags & IBD_INSTANTANEOUS_CADENCE) {
        fmtBufAppend(&fmtBuf, "instCadence: %u [rpm]\n", (binBufGetUINT16(&binBuf) / 2));
    }
    if (flags & IBD_AVERAGE_CADENCE) {
        fmtBufAppend(&fmtBuf, "avgCadence: %u [rpm]\n", (binBufGetUINT16(&binBuf) / 2));
    }
    if (flags & IBD_TOTAL_DISTANCE) {
        fmtBufAppend(&fmtBuf, "totalDistance: %" PRIu32 " [m]\n", binBufGetUINT24(&binBuf));
    }
    if (flags & IBD_RESISTANCE_LEVEL) {
        fmtBufAppend(&fmtBuf, "resistanceLevel: %u\n", binBufGetUINT8(&binBuf));
    }
    if (flags & IBD_INSTANTANEOUS_POWER) {
        fmtBufAppend(&fmtBuf, "instPower: %u [W]\n", binBufGetUINT16(&binBuf));
    }
    if (flags & IBD_AVERAGE_POWER) {
        fmtBufAppend(&fmtBuf, "avgPower: %u [W]\n", binBufGetUINT16(&binBuf));
    }
    if (flags & IBD_EXPENDED_ENERGY) {
        fmtBufAppend(&fmtBuf, "expEnergy: %u [kg.cal]\n", binBufGetUINT16(&binBuf));
    }
    if (flags & IBD_HEART_RATE) {
        fmtBufAppend(&fmtBuf, "heartRate: %u [bpm]\n", binBufGetUINT8(&binBuf));
    }
    if (flags & IBD_METABOLIC_EQUIVALENT) {
        fmtBufAppend(&fmtBuf, "metabEquiv: %.3lf [me]\n", (binBufGetUINT8(&binBuf) / 10.0));
    }
    if (flags & IBD_ELAPSED_TIME) {
        fmtBufAppend(&fmtBuf, "elapsedTime: %u [s]\n", binBufGetUINT16(&binBuf));
    }
    if (flags & IBD_REMAINING_TIME) {
        fmtBufAppend(&fmtBuf, "remainTime: %u [s]\n", binBufGetUINT16(&binBuf));
    }

    return fmtBuf.buf;
}

void hexDump(const void *pBuf, int bufLen)
{
    const uint8_t *p = pBuf;
    char ascii[17];

    while (bufLen) {
        int numBytes = (bufLen >= 16) ? 16 : bufLen;
        int n;
        bufLen -= numBytes;
        for (n = 0; n < numBytes; n++) {
            uint8_t byte = *p++;
            printf("%02x ", byte);
            ascii[n] = isprint(byte) ? byte : '.';
            if (n == 7)
                printf(" ");
        }
        ascii[n] = '\0';
        while (n < 16) {
            printf("   ");
            if (n++ == 7)
                printf(" ");
        }
        printf("   %s\n", ascii);
    }
}

static void dumpDiscoverServicesMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    if (mesgType == response) {
        const DiscSvcsMesg *mesg = (DiscSvcsMesg *) pMesg;
        const Uuid128 *svcUuid = mesg->svcUuid;

        while (mesgLen >= sizeof (Uuid128)) {
            fmtBufAppend(fmtBuf, "svcUUID: %s (%s)\n", fmtUuid128(svcUuid), fmtUuid128Name(svcUuid));
            svcUuid++;
            mesgLen -= sizeof (Uuid128);
        }
    }
}

static void dumpDiscoverCharacteristicsMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    const DiscCharsMesg *mesg = (DiscCharsMesg *) pMesg;

    fmtBufAppend(fmtBuf, "svcUUID: %s (%s)\n", fmtUuid128(&mesg->svcUuid), fmtUuid128Name(&mesg->svcUuid));
    mesgLen -= sizeof (Uuid128);

    if (mesgType == response) {
        const CharProp *charProp = mesg->charProp;

        while (mesgLen >= sizeof (CharProp)) {
            fmtBufAppend(fmtBuf, "charUUID: %s (%s) [%s]\n", fmtUuid128(&charProp->charUuid), fmtUuid128Name(&charProp->charUuid), fmtCharProp(charProp->properties));
            charProp++;
            mesgLen -= sizeof (CharProp);
        }
    }
}

static void dumpReadCharacteristicMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    const ReadCharMesg *mesg = (ReadCharMesg *) pMesg;
    fmtBufAppend(fmtBuf, "charUUID: %s (%s)\n", fmtUuid128(&mesg->charUuid), fmtUuid128Name(&mesg->charUuid));

    if (mesgType == response) {
        Uuid16 uuid16;
        size_t dataLen = (mesgLen - sizeof (Uuid128));

        getUuid16(&uuid16, &mesg->charUuid);
        if (uuid16Eq(&uuid16, &fitnessMachineFeatureUUID)) {
            const FitMachFeat *fmf = (FitMachFeat *) mesg->data;
            fmtBufAppend(fmtBuf, "fitnessMachineFeatures: 0x%08" PRIx32 "\n", (getUINT32(fmf->fmFeat) & ~FMF_RFU));
            fmtBufAppend(fmtBuf, "targetSettingFeatures: 0x%08" PRIx32 "\n", (getUINT32(fmf->tsFeat) & ~TSF_RFU));
        } else if (uuid16Eq(&uuid16, &indoorBikeDataUUID)) {
            const IndoorBikeData *ibd = (IndoorBikeData *) mesg->data;
            fmtBufAppend(fmtBuf, "%s", fmtIndoorBikeData(ibd, dataLen));
        } else if (uuid16Eq(&uuid16, &trainingStatusUUID)) {
            const TrainingStatus *trStat = (TrainingStatus *) mesg->data;
            int strLen = (dataLen - sizeof (TrainingStatus));
            fmtBufAppend(fmtBuf, "flags: 0x%02x\n", trStat->flags);
            fmtBufAppend(fmtBuf, "trainingStatus: 0x%02x\n", (trStat->trainingStatus & ~TST_RFU));
            if (strLen != 0) {
                for (int i = 0; i < strLen; i++) {
                    fmtBufAppend(fmtBuf, "%c", mesg->data[i]);
                }
            }
        } else if (uuid16Eq(&uuid16, &supportedResistanceLevelRangeUUID)) {
            const ResistLevelRange *rlr = (ResistLevelRange *) mesg->data;
            fmtBufAppend(fmtBuf, "minimum: %u\n", rlr->minResLevel);
            fmtBufAppend(fmtBuf, "maximum: %u\n", rlr->maxResLevel);
            fmtBufAppend(fmtBuf, "minIncr: %u\n", rlr->incResLevel);
        } else if (uuid16Eq(&uuid16, &supportedPowerRangeUUID)) {
            const PowerRange *spr = (PowerRange *) mesg->data;
            fmtBufAppend(fmtBuf, "minimum: %d [W]\n", getSINT16(spr->minPower));
            fmtBufAppend(fmtBuf, "maximum: %d [W]\n", getSINT16(spr->maxPower));
            fmtBufAppend(fmtBuf, "minIncr: %u [W]\n", getUINT16(spr->incPower));
        } else {
            fmtBufHexDump(fmtBuf, mesg->data, dataLen);
        }
    }
}

static void dumpWriteCharacteristicMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    const WriteCharMesg *mesg = (WriteCharMesg *) pMesg;

    fmtBufAppend(fmtBuf, "charUUID: %s (%s)\n", fmtUuid128(&mesg->charUuid), fmtUuid128Name(&mesg->charUuid));

    if (mesgType == request) {
        Uuid16 uuid16;
        size_t dataLen = (mesgLen - sizeof (Uuid128));

        getUuid16(&uuid16, &mesg->charUuid);
        if (uuid16Eq(&uuid16, &fitnessMachineControlPointUUID)) {
            const FitMachCP *fmcp = (FitMachCP *) mesg->data;
            fmtBufAppend(fmtBuf, "%s", fmtFitMachCP(fmcp, dataLen));
        } else {
            fmtBufHexDump(fmtBuf, mesg->data, dataLen);
        }
    }
}

static void dumpEnableCharacteristicNotificationsMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    const EnCharNotMesg *mesg = (EnCharNotMesg *) pMesg;
    fmtBufAppend(fmtBuf, "charUUID: %s (%s)\n", fmtUuid128(&mesg->charUuid), fmtUuid128Name(&mesg->charUuid));
    fmtBufAppend(fmtBuf, "enable: %u\n", (mesg->enable & 0x01));
}

static void dumpUnsolicitedCharacteristicNotificationMesg(const DirconMesg *pMesg, int mesgLen, MesgType mesgType)
{
    const UnsCharNot *mesg = (UnsCharNot *) pMesg;
    Uuid16 uuid16;
    size_t dataLen = (mesgLen - sizeof (Uuid128));

    fmtBufAppend(fmtBuf, "charUUID: %s (%s)\n", fmtUuid128(&mesg->charUuid), fmtUuid128Name(&mesg->charUuid));
    getUuid16(&uuid16, &mesg->charUuid);
    if (uuid16Eq(&uuid16, &indoorBikeDataUUID)) {
        // Indoor Bike Data
        const IndoorBikeData *ibd = (IndoorBikeData *) mesg->data;
        fmtBufAppend(fmtBuf, "%s", fmtIndoorBikeData(ibd, dataLen));
    } else {
        fmtBufHexDump(fmtBuf, mesg->data, dataLen);
    }
}

void dirconDumpMesg(const struct timeval *pTs, Server *server, const DirconSession *sess,
                    MesgDir dir, MesgType mesgType, const DirconMesg *mesg)
{
    if ((server->dissectMesgId == 0) || (mesg->mesgId == server->dissectMesgId)) {
        struct timeval relTs;
        int mesgLen = mesg->mesgLen;

        printf("\n");

        fmtBufInit(fmtBuf, dumpStrBuf, sizeof (dumpStrBuf));

        tvSub(&relTs, pTs, &server->baseTime);
        fmtBufAppend(fmtBuf, "%s: %d.%06d\n", fmtMesgDir(dir), (int) relTs.tv_sec, (int) relTs.tv_usec);
        fmtBufAppend(fmtBuf, "mesgId: %s (0x%02x)\n", fmtMesgId(mesg->mesgId), mesg->mesgId);
        fmtBufAppend(fmtBuf, "seqNum: %u\n", mesg->seqNum);
        if (mesgType == response) {
            fmtBufAppend(fmtBuf, "respCode: %s\n", fmtRespCode(mesg->respCode));
        }
        fmtBufAppend(fmtBuf, "mesgLen: %u\n", mesgLen);

        switch (mesg->mesgId) {
            case DiscoverServices: { dumpDiscoverServicesMesg(mesg, mesgLen, mesgType); break; }
            case DiscoverCharacteristics: { dumpDiscoverCharacteristicsMesg(mesg, mesgLen, mesgType); break; }
            case ReadCharacteristic: { dumpReadCharacteristicMesg(mesg, mesgLen, mesgType); break; }
            case WriteCharacteristic: { dumpWriteCharacteristicMesg(mesg, mesgLen, mesgType); break; }
            case EnableCharacteristicNotifications: { dumpEnableCharacteristicNotificationsMesg(mesg, mesgLen, mesgType); break; }
            case UnsolicitedCharacteristicNotification: { dumpUnsolicitedCharacteristicNotificationMesg(mesg, mesgLen, mesgType); break; }
            default: break;
        }

        // Print the dissected message data
        fmtBufPrint(fmtBuf, stdout);

        if (server->hexDumpMesg) {
            printf("\n");
            hexDump(mesg, (sizeof (DirconMesg) + mesgLen));
        }

        printf("\n");
    }
}


