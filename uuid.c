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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "uuid.h"
#include "fmtbuf.h"

// 16-bit UUID of some relevant BLE Services
const uint16_t genericAccessService = 0x1800;
const uint16_t genericAttributeService = 0x1801;
const uint16_t deviceInfoService = 0x180A;
const uint16_t heartRateService = 0x180D;
const uint16_t batteryService = 0x180F;
const uint16_t runningSpeedAndCadenceService = 0x1814;
const uint16_t cyclingSpeedAndCadenceService = 0x1816;
const uint16_t cyclingPowerService = 0x1818;
const uint16_t userDataService = 0x181c;
const uint16_t fitnessMachineService = 0x1826;

// 16-bit UUID of some relevant BLE Characteristics
const uint16_t deviceName = 0x2A00;
const uint16_t deviceAppearance = 0x2A01;
const uint16_t batteryLevel = 0x2A19;
const uint16_t modelNumber = 0x2A24;
const uint16_t serialNumber = 0x2A25;
const uint16_t firmwareRevision = 0x2A26;
const uint16_t hardwareRevision = 0x2A27;
const uint16_t softwareRevision = 0x2A28;
const uint16_t manufacturerName = 0x2A29;
const uint16_t heartRateMeasurement = 0x2A37;
const uint16_t bodySensorLocation = 0x2A38;
const uint16_t rscMeasurement = 0x2A53;
const uint16_t rscFeature = 0x2A54;
const uint16_t scControlPoint = 0x2A55;
const uint16_t cscMeasurement = 0x2A5B;
const uint16_t cscFeature = 0x2A5C;
const uint16_t sensorLocation = 0x2A5D;
const uint16_t cyclingPowerMeasurement = 0x2A63;
const uint16_t cyclingPowerFeature = 0x2A65;
const uint16_t cyclingPowerControlPoint = 0x2A66;
const uint16_t weight = 0x2A98;
const uint16_t fitnessMachineFeature = 0x2ACC;
const uint16_t indoorBikeData = 0x2AD2;
const uint16_t trainingStatus = 0x2AD3;
const uint16_t supportedResistanceLevelRange = 0x2AD6;
const uint16_t supportedPowerRange = 0x2AD8;
const uint16_t fitnessMachineControlPoint = 0x2AD9;
const uint16_t fitnessMachineStatus = 0x2ADA;

// 16-bit UUID of some relevant BLE Characteristic Descriptors
const uint16_t clientCharacteristicConfiguration = 0x2902;

// BLE Base 128-bit UUID: 00 00 nn nn 00 00 10 00  80 00 00 80 5f 9b 34 fb
Uuid128 bleBaseUUID = {
        .data = { 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00,
                  0x10, 0x00,
                  0x80, 0x00,
                  0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb }
};

typedef struct UuidRec {
    const uint16_t uuid;
    const char *name;
} UuidRec;

static UuidRec uuidNameTbl[] = {
        // Service UUID's
        { genericAccessService, "Generic Access Service" },
        { genericAttributeService, "Generic Attribute Service" },
        { deviceInfoService, "Device Information Service" },
        { heartRateService, "Heart Rate Service" },
        { batteryService, "Battery Service" },
        { runningSpeedAndCadenceService, "Running Speed & Cadence Service" },
        { cyclingSpeedAndCadenceService, "Cycling Speed & Cadence Service" },
        { cyclingPowerService, "Cycling Power Service" },
        { userDataService, "User Data Service" },
        { fitnessMachineService, "Fitness Machine Service" },

        // Characteristic UUID's
        { deviceName, "Device Name" },
        { deviceAppearance, "Device Appearance" },
        { batteryLevel, "Battery Level" },
        { modelNumber, "Model Number" },
        { serialNumber, "Serial Number" },
        { firmwareRevision, "Firmware Revision" },
        { hardwareRevision, "Hardware Revision" },
        { softwareRevision, "Software Revision" },
        { manufacturerName, "Manufacturer Name" },
        { heartRateMeasurement, "Heart Rate Measurement" },
        { bodySensorLocation, "Body Sensor Location" },
        { rscMeasurement, "Running Speed & Cadence Measurement" },
        { rscFeature, "Running Speed & Cadence Feature" },
        { scControlPoint, "Speed & Cadence Control Point" },
        { cscMeasurement, "Cycling Speed & Cadence Measurement" },
        { cscFeature, "Cycling Speed & Cadence Feature" },
        { sensorLocation, "Sensor Location" },
        { cyclingPowerMeasurement, "Cycling Power Measurement" },
        { cyclingPowerFeature, "Cycling Power Feature" },
        { cyclingPowerControlPoint, "Cycling Power Control Point" },
        { weight, "Weight" },
        { fitnessMachineFeature, "Fitness Machine Feature" },
        { indoorBikeData, "Indoor Bike Data" },
        { trainingStatus, "Training Status" },
        { supportedResistanceLevelRange, "Supported Resistance Level Range" },
        { supportedPowerRange, "Supported Power Range" },
        { fitnessMachineControlPoint, "Fitness Machine Control Point" },
        { fitnessMachineStatus, "Fitness Machine Status" },

        { 0, NULL }
};

const char *fmtUuidName(uint16_t uuid)
{
    for (int i = 0; uuidNameTbl[i].uuid != 0; i++) {
        if (uuid == uuidNameTbl[i].uuid)
            return uuidNameTbl[i].name;
    }

    return "???";
}

void uint16ToUuid128(Uuid128 *uuid, uint16_t uint16)
{
    memcpy(uuid, &bleBaseUUID, sizeof (Uuid128));
    uuid->data[2] = (uint16 >> 8);
    uuid->data[3] = (uint16 & 0xff);
}

int scanUuid16(const char *val, uint16_t *uuid)
{
    uint32_t hi, lo;

    if (strlen(val) == 4) {
        if (sscanf(val,
                "%02x%02x",
                &hi, &lo) == 2) {
            *uuid = (hi << 8) | lo;
            return 0;
        }
    }

    return -1;
}

uint16_t uuid128ToUint16(const Uuid128 *uuid)
{
    return ((uuid->data[2] << 8) | (uuid->data[3]));
}

bool uuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2)
{
    return (memcmp(uuid1, uuid2, sizeof (Uuid128)) == 0) ? true : false;
}

bool baseUuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2)
{
    for (int i = 0; i < 16; i++) {
        // Ignore bytes #2 and #3
        if ((i == 2) || (i == 3))
            continue;
        if (uuid1->data[i] != uuid2->data[i])
            return false;
    }

    return true;
}

const char *fmtUuid128Name(const Uuid128 *uuid)
{
    static char strBuf[64];
    FmtBuf fmtBuf;

    fmtBufInit(&fmtBuf, strBuf, sizeof (strBuf));

    if (baseUuid128Eq(uuid, &bleBaseUUID)) {
        return fmtUuidName(uuid128ToUint16(uuid));
    }

    return "???";
}

// Format a 128-bit UUID
const char *fmtUuid128(const Uuid128 *uuid)
{
    static char fmtBuf[38];
    snprintf(fmtBuf, sizeof (fmtBuf), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3],
            uuid->data[4], uuid->data[5],
            uuid->data[6], uuid->data[7],
            uuid->data[8], uuid->data[9],
            uuid->data[10], uuid->data[11], uuid->data[12], uuid->data[13], uuid->data[14], uuid->data[15]);
    return fmtBuf;
}

int scanUuid128(const char *str, Uuid128 *uuid)
{
    uint32_t data[16];

    if (strlen(str) == 36) {
        if (sscanf(str,
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                &data[0], &data[1], &data[2], &data[3],
                &data[4], &data[5],
                &data[6], &data[7],
                &data[8], &data[9],
                &data[10], &data[11], &data[12], &data[13], &data[14], &data[15]) == 16) {
            for (int i = 0; i < 16; i++)
                uuid->data[i] = data[i];
            return 0;
        }

        if (sscanf(str,
                "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                &data[0], &data[1], &data[2], &data[3],
                &data[4], &data[5],
                &data[6], &data[7],
                &data[8], &data[9],
                &data[10], &data[11], &data[12], &data[13], &data[14], &data[15]) == 16) {
            for (int i = 0; i < 16; i++)
                uuid->data[i] = data[i];
            return 0;
        }
    }

    return -1;
}
