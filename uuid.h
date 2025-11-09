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

#pragma once

#include <stdint.h>

#include "defs.h"

// 16-bit UUID
typedef struct Uuid16 {
    uint8_t data[2];
} Uuid16;

// 128-bit UUID
typedef struct Uuid128 {
    uint8_t data[16];
} Uuid128;

// Well known 16-bit UUID's
extern Uuid16 deviceInfoUUID;
extern Uuid16 heartRateServiceUUID;
extern Uuid16 batteryServiceUUID;
extern Uuid16 cyclingPowerServiceUUID;
extern Uuid16 userDataUUID;
extern Uuid16 fitnessMachineServiceUUID;
extern Uuid16 primaryServiceUUID;
extern Uuid16 secondaryServiceUUID;
extern Uuid16 includeUUID;
extern Uuid16 characteristicUUID;
extern Uuid16 batteryLevelUUID;
extern Uuid16 modelNumberUUID;
extern Uuid16 serialNumberUUID;
extern Uuid16 firmwareRevisionUUID;
extern Uuid16 hardwareRevisionUUID;
extern Uuid16 softwareRevisionUUID;
extern Uuid16 manufacturerNameUUID;
extern Uuid16 heartRateMeasurementUUID;
extern Uuid16 bodySensorLocationUUID;
extern Uuid16 sensorLocationUUID;
extern Uuid16 cyclingPowerMeasurementUUID;
extern Uuid16 cyclingPowerFeatureUUID;
extern Uuid16 cyclingPowerControlPointUUID;
extern Uuid16 weightUUID;
extern Uuid16 fitnessMachineFeatureUUID;
extern Uuid16 indoorBikeDataUUID;
extern Uuid16 trainingStatusUUID;
extern Uuid16 supportedResistanceLevelRangeUUID;
extern Uuid16 supportedPowerRangeUUID;
extern Uuid16 fitnessMachineControlPointUUID;
extern Uuid16 fitnessMachineStatusUUID;
extern Uuid16 clientCharacteristicConfigurationUUID;
extern Uuid16 wahooFitnessLlcUUID;

// BLE Base 128-bit UUID
extern Uuid128 bleBaseUUID;

// Wahoo Fitness custom 128-bit UUID's
extern Uuid128 wahooCyclingPowerServiceExtension;
extern Uuid128 wahooGemModuleFirmwareUpdateService;
extern Uuid128 wahooFitnessEquipmentService;
extern Uuid128 wahooFitnessMachineService;

__BEGIN_DECLS

extern void uint16ToUuid16(Uuid16 *uuid, uint16_t uint16);
extern void getUuid16(Uuid16 *uuid16, const Uuid128 *uuid128);
extern bool uuid16Eq(const Uuid16 *uuid1, const Uuid16 *uuid2);
extern const char *fmtUuid16(const Uuid16 *uuid);
extern const char *fmtUuid16Name(const Uuid16 *uuid);

extern void uint16ToUuid128(Uuid128 *uuid, uint16_t uint16);
extern bool uuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2);
extern void buildUuid128(Uuid128 *uuid128, const Uuid16 *uuid16);
extern bool isBleUuid128(const Uuid128 *uuid128);
extern const char *fmtUuid128(const Uuid128 *uuid);
extern const char *fmtUuid128Name(const Uuid128 *uuid);

__END_DECLS
