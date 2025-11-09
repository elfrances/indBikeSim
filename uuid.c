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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "uuid.h"

// Well known 16-bit UUID's
Uuid16 deviceInfoUUID = { .data = { 0x18, 0x0a } };
Uuid16 heartRateServiceUUID = { .data = { 0x18, 0x0d } };
Uuid16 batteryServiceUUID = { .data = { 0x18, 0x0f } };
Uuid16 cyclingPowerServiceUUID = { .data = { 0x18, 0x18 } };
Uuid16 userDataUUID = { .data = { 0x18, 0x1c } };
Uuid16 fitnessMachineServiceUUID = { .data = { 0x18, 0x26 } };
Uuid16 primaryServiceUUID = { .data = { 0x28, 0x00 } };
Uuid16 secondaryServiceUUID = { .data = { 0x28, 0x01 } };
Uuid16 includeUUID = { .data = { 0x28, 0x02 } };
Uuid16 characteristicUUID = { .data = { 0x28, 0x03 } };
Uuid16 batteryLevelUUID = { .data = { 0x2a, 0x19 } };
Uuid16 modelNumberUUID = { .data = { 0x2a, 0x24 } };
Uuid16 serialNumberUUID = { .data = { 0x2a, 0x25 } };
Uuid16 firmwareRevisionUUID = { .data = { 0x2a, 0x26 } };
Uuid16 hardwareRevisionUUID = { .data = { 0x2a, 0x27 } };
Uuid16 softwareRevisionUUID = { .data = { 0x2a, 0x28 } };
Uuid16 manufacturerNameUUID = { .data = { 0x2a, 0x29 } };
Uuid16 heartRateMeasurementUUID = { .data = { 0x2a, 0x37 } };
Uuid16 bodySensorLocationUUID = { .data = { 0x2a, 0x38 } };
Uuid16 sensorLocationUUID = { .data = { 0x2a, 0x5d } };
Uuid16 cyclingPowerMeasurementUUID = { .data = { 0x2a, 0x63 } };
Uuid16 cyclingPowerFeatureUUID = { .data = { 0x2a, 0x65 } };
Uuid16 cyclingPowerControlPointUUID = { .data = { 0x2a, 0x66 } };
Uuid16 weightUUID = { .data = { 0x2a, 0x98 } };
Uuid16 fitnessMachineFeatureUUID = { .data = { 0x2a, 0xcc } };
Uuid16 indoorBikeDataUUID = { .data = { 0x2a, 0xd2 } };
Uuid16 trainingStatusUUID = { .data = { 0x2a, 0xd3 } };
Uuid16 supportedResistanceLevelRangeUUID = { .data = { 0x2a, 0xd6 } };
Uuid16 supportedPowerRangeUUID = { .data = { 0x2a, 0xd8 } };
Uuid16 fitnessMachineControlPointUUID = { .data = { 0x2a, 0xd9 } };
Uuid16 fitnessMachineStatusUUID = { .data = { 0x2a, 0xda } };
Uuid16 clientCharacteristicConfigurationUUID = { .data = { 0x29, 0x02 } };
Uuid16 wahooFitnessLlcUUID = { .data = { 0x01, 0xfc } };

// BLE Base 128-bit UUID: 00 00 nn nn 00 00 10 00  80 00 00 80 5f 9b 34 fb
Uuid128 bleBaseUUID = {
        .data = { 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00,
                  0x10, 0x00,
                  0x80, 0x00,
                  0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb} };

// Wahoo Fitness: Cycling Power Service Extension
Uuid128 wahooCyclingPowerServiceExtension = {
        .data = { 0xa0, 0x26, 0xe0, 0x05,
                  0x0a, 0x7d,
                  0x4a, 0xb3,
                  0x97, 0xfa,
                  0xf1, 0x50, 0x0f, 0x9f, 0xeb, 0x8b}
};

// Wahoo Fitness: GEM Module Firmware update Service
Uuid128 wahooGemModuleFirmwareUpdateService = {
        .data = { 0xa0, 0x26, 0xee, 0x01,
                  0x0a, 0x7d,
                  0x4a, 0xb3,
                  0x97, 0xfa,
                  0xf1, 0x50, 0x0f, 0x9f, 0xeb, 0x8b}
};

// Wahoo Fitness: Wahoo Fitness Equipment Service
Uuid128 wahooFitnessEquipmentService = {
        .data = { 0xa0, 0x26, 0xee, 0x07,
                  0x0a, 0x7d,
                  0x4a, 0xb3,
                  0x97, 0xfa,
                  0xf1, 0x50, 0x0f, 0x9f, 0xeb, 0x8b}
};

// Wahoo Fitness: Wahoo Fitness Machine Service
Uuid128 wahooFitnessMachineService = {
        .data = { 0xa0, 0x26, 0xee, 0x0b,
                  0x0a, 0x7d,
                  0x4a, 0xb3,
                  0x97, 0xfa,
                  0xf1, 0x50, 0x0f, 0x9f, 0xeb, 0x8b}
};

// Convert a uint16_t value in host byte order into
// the corresponding Uuid16 value.
void uint16ToUuid16(Uuid16 *uuid16, uint16_t uint16)
{
    uuid16->data[0] = uint16 >> 8;
    uuid16->data[1] = uint16 & 0xff;
}

// Get the 16-bit UUID from the specified 128-bit UUID
void getUuid16(Uuid16 *uuid16, const Uuid128 *uuid128)
{
    uuid16->data[0] = uuid128->data[2];
    uuid16->data[1] = uuid128->data[3];
}

bool uuid16Eq(const Uuid16 *uuid1, const Uuid16 *uuid2)
{
    return !! ((uuid1->data[0] == uuid2->data[0]) && (uuid1->data[1] == uuid2->data[1]));
}

void uint16ToUuid128(Uuid128 *uuid, uint16_t uint16)
{
    Uuid16 uuid16;
    uint16ToUuid16(&uuid16, uint16);
    buildUuid128(uuid, &uuid16);
}

bool uuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2)
{
    return (memcmp(uuid1, uuid2, sizeof (Uuid128)) == 0) ? true : false;
}

// Build a 128-bit UUID from the specified 16-bit UUID
void buildUuid128(Uuid128 *uuid128, const Uuid16 *uuid16)
{
    memcpy(uuid128, &bleBaseUUID, sizeof (Uuid128));
    uuid128->data[2] = uuid16->data[0];
    uuid128->data[3] = uuid16->data[1];
}

bool isBleUuid128(const Uuid128 *uuid128)
{
    Uuid128 uuid = *uuid128;
    uuid.data[2] = 0x00;
    uuid.data[3] = 0x00;
    return uuid128Eq(&uuid, &bleBaseUUID);
}

static struct UuidNameRec {
    const Uuid16 *uuid;
    const char *name;
} uuidNameTbl[] = {
    { &deviceInfoUUID, "Device Information" },
    { &heartRateServiceUUID, "Heart Rate Service" },
    { &batteryServiceUUID, "Battery Service" },
    { &cyclingPowerServiceUUID, "Cycling Power Service" },
    { &userDataUUID, "User Data" },
    { &fitnessMachineServiceUUID, "Fitness Machine Service" },
    { &primaryServiceUUID, "Primary Service" },
    { &secondaryServiceUUID, "Secondary Service" },
    { &includeUUID, "Include" },
    { &characteristicUUID, "Characteristic" },
    { &batteryLevelUUID, "Battery Level" },
    { &modelNumberUUID, "Model Number" },
    { &serialNumberUUID, "Serial Number" },
    { &firmwareRevisionUUID, "Firmware Revision" },
    { &hardwareRevisionUUID, "Hardware Revision" },
    { &softwareRevisionUUID, "Software Revision" },
    { &manufacturerNameUUID, "Manufacturer Name" },
    { &heartRateMeasurementUUID, "Heart Rate Measurement" },
    { &bodySensorLocationUUID, "Body Sensor Location" },
    { &sensorLocationUUID, "Sensor Location" },
    { &cyclingPowerMeasurementUUID, "Cycling Power Measurement" },
    { &cyclingPowerFeatureUUID, "Cycling Power Feature" },
    { &cyclingPowerControlPointUUID, "Cycling Power Control Point" },
    { &weightUUID, "Weight" },
    { &fitnessMachineFeatureUUID, "Fitness Machine Feature" },
    { &indoorBikeDataUUID, "Indoor Bike Data" },
    { &trainingStatusUUID, "Training Status" },
    { &supportedResistanceLevelRangeUUID, "Supported Resistance Level Range" },
    { &supportedPowerRangeUUID, "Supported Power Range" },
    { &fitnessMachineControlPointUUID, "Fitness Machine Control Point" },
    { &fitnessMachineStatusUUID, "Fitness Machine Status" },
    { &clientCharacteristicConfigurationUUID, "Client Characteristic Configuration" },
    { NULL, NULL }
};

const char *fmtUuid16Name(const Uuid16 *uuid)
{
    for (int i = 0; uuidNameTbl[i].uuid != NULL; i++) {
        if ((uuid->data[0] == uuidNameTbl[i].uuid->data[0]) &&
            (uuid->data[1] == uuidNameTbl[i].uuid->data[1])) {
            return uuidNameTbl[i].name;
        }
    }

    return "???";
}

const char *fmtUuid128Name(const Uuid128 *uuid)
{
    if (isBleUuid128(uuid)) {
        for (int i = 0; uuidNameTbl[i].uuid != NULL; i++) {
            if ((uuid->data[2] == uuidNameTbl[i].uuid->data[0]) &&
                (uuid->data[3] == uuidNameTbl[i].uuid->data[1])) {
                return uuidNameTbl[i].name;
            }
        }
    } else if (uuid128Eq(uuid, &wahooCyclingPowerServiceExtension)) {
        return "Wahoo Fitness Cycling Power Service Extension";
    } else if (uuid128Eq(uuid, &wahooGemModuleFirmwareUpdateService)) {
        return "Wahoo GEM Module Firmware Update Service";
    } else if (uuid128Eq(uuid, &wahooFitnessEquipmentService)) {
        return "Wahoo Fitness Equipment Service";
    } else if (uuid128Eq(uuid, &wahooFitnessMachineService)) {
        return "Wahoo Fitness Machine Service";
    }

    return "???";
}

// Format a 128-bit UUID
const char *fmtUuid128(const Uuid128 *uuid)
{
    static char fmtBuf[64];
    snprintf(fmtBuf, sizeof (fmtBuf), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3],
            uuid->data[4], uuid->data[5],
            uuid->data[6], uuid->data[7],
            uuid->data[8], uuid->data[9],
            uuid->data[10], uuid->data[11], uuid->data[12], uuid->data[13], uuid->data[14], uuid->data[15]);
    return fmtBuf;
}


