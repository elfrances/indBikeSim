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

// 128-bit UUID
typedef struct Uuid128 {
    uint8_t data[16];
} Uuid128;

// 16-bit UUID of some relevant BLE Services
extern const uint16_t genericAccessService;
extern const uint16_t genericAttributeService;
extern const uint16_t deviceInfoService;
extern const uint16_t heartRateService;
extern const uint16_t batteryService;
extern const uint16_t runningSpeedAndCadenceService;
extern const uint16_t cyclingSpeedAndCadenceService;
extern const uint16_t cyclingPowerService;
extern const uint16_t userDataService;
extern const uint16_t fitnessMachineService;

// 16-bit UUID of some relevant BLE Characteristics
extern const uint16_t deviceName;
extern const uint16_t deviceAppearance;
extern const uint16_t batteryLevel;
extern const uint16_t modelNumber;
extern const uint16_t serialNumber;
extern const uint16_t firmwareRevision;
extern const uint16_t hardwareRevision;
extern const uint16_t softwareRevision;
extern const uint16_t manufacturerName;
extern const uint16_t heartRateMeasurement;
extern const uint16_t bodySensorLocation;
extern const uint16_t rscMeasurement;
extern const uint16_t scControlPoint;
extern const uint16_t rscFeature;
extern const uint16_t cscMeasurement;
extern const uint16_t cscFeature;
extern const uint16_t sensorLocation;
extern const uint16_t cyclingPowerMeasurement;
extern const uint16_t cyclingPowerFeature;
extern const uint16_t cyclingPowerControlPoint;
extern const uint16_t weight;
extern const uint16_t fitnessMachineFeature;
extern const uint16_t indoorBikeData;
extern const uint16_t trainingStatus;
extern const uint16_t supportedResistanceLevelRange;
extern const uint16_t supportedPowerRange;
extern const uint16_t fitnessMachineControlPoint;
extern const uint16_t fitnessMachineStatus;

// 16-bit UUID of some relevant BLE Characteristic Descriptors
extern const uint16_t clientCharacteristicConfiguration;

// BLE Base 128-bit UUID
extern Uuid128 bleBaseUUID;

__BEGIN_DECLS

extern const char *fmtUuidName(uint16_t uint16);

extern void uint16ToUuid128(Uuid128 *uuid, uint16_t uint16);
extern uint16_t uuid128ToUint16(const Uuid128 *uuid);
extern int scanUuid16(const char *val, uint16_t *uuid);

extern bool uuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2);
extern bool baseUuid128Eq(const Uuid128 *uuid1, const Uuid128 *uuid2);
extern const char *fmtUuid128(const Uuid128 *uuid);
extern const char *fmtUuid128Name(const Uuid128 *uuid);
extern int scanUuid128(const char *str, Uuid128 *uuid);

__END_DECLS
