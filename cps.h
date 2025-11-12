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

#include "defs.h"

// All definitions based on the Bluetooth "Cycling Power Service" doc:
//
// https://www.bluetooth.com/specifications/specs/cycling-power-service-1-1/
//

// Cycling Power Feature (CPS 3.1)
//
// The Cycling Power Feature characteristic shall be used to describe the supported features of
// the Server.
//
struct CycPowerFeat {
    uint8_t flags[4];
} __attribute__((packed));
typedef struct CycPowerFeat CycPowerFeat;

#define CPF_PEDAL_POWER_BALANCE             bitMask(0)
#define CPF_ACCUMMULATED_TORQUE             bitMask(1)
#define CPF_WHEEL_REVOLUTION_DATA           bitMask(2)
#define CPF_CRANK_REVOLUTION_DATA           bitMask(3)
#define CPF_EXTREME_MAGNITUDE               bitMask(4)
#define CPF_EXTREME_ANGLES                  bitMask(5)
#define CPF_TOP_BOTTOM_DEAD_SPOT_ANGLES     bitMask(6)
#define CPF_ACCUMMULATED_ENERGY             bitMask(7)
#define CPF_OFFSET_COMPENSTATION_INDICATOR  bitMask(8)
#define CPF_OFFSET_COMPENSATION             bitMask(9)
#define CPF_CONTENT_MASKING                 bitMask(10)
#define CPF_MULTIPLE_SENSOR_LOCATION        bitMask(11)
#define CPF_CRANK_LENGTH_ADJUSTMENT         bitMask(12)
#define CPF_CHAIN_LENGTH_ADJUSTMENT         bitMask(13)
#define CPF_CHAIN_WEIGHT_ADJUSTMENT         bitMask(14)
#define CPF_SPAN_LENGHT_ADJUSTMENT          bitMask(15)
#define CPF_SENSOR_MEASUREMENT_CONTEXT      bitMask(16)
#define CPF_INST_MEASUREMENT_DIRECTION      bitMask(17)
#define CPF_FACTORY_CALIBRATION_DATE        bitMask(18)
#define CPF_ENHANCED_OFFSET_COMPENSATION    bitMask(19)
#define CPF_DISTRIBUTED_SYSTEM_SUPPORT_B0   bitMask(20)
#define CPF_DISTRIBUTED_SYSTEM_SUPPORT_B1   bitMask(21)
#define CPF_RFU                             0xFC000000U // bits 22-31

// Cycling Power Measurement (CPS 3.2)
//
// The Cycling Power Measurement characteristic is used to send power-related data, speed-
// related data, and/or cadence-related data.
//
struct CycPowerMeas {
    uint8_t flags[2];
    uint8_t instPower[2];   // {SINT16, Watts, 1}
    uint8_t data[0];    // depends on the flags
} __attribute__((packed));
typedef struct CycPowerMeas CycPowerMeas;

// Cycling Power Measurement Flags (CPS 3.2.1.1)
#define CPM_PEDAL_POWER_BALANCE             bitMask(0)  // {UINT8, percentage, 1/2}
#define CPM_PEDAL_POWER_BALANCE_REFERENCE   bitMask(1)  // 0=unknown, 1=left
#define CPM_ACCUMMULATED_TORQUE             bitMask(2)  // {UINT16, Nm, 1/32}
#define CPM_ACCUMMULATED_TORQUE_SOURCE      bitMask(3)  // 0=wheel, 1=crank
#define CPM_WHEEL_REVOLUTION_DATA           bitMask(4)  // {UINT32, unitless, 1} {UINT16, seconds, 1/2048}
#define CPM_CRANK_REVOLUTION_DATA           bitMask(5)  // {UINT16, unitless, 1} {UINT16, seconds, 1/1024}
#define CPM_EXTREME_FORCE_MAGNITUDES        bitMask(6)  // {SINT16, Newton, 1} {SINT16, Newton, 1}
#define CPM_EXTREME_TORQUE_MAGNITUDES       bitMask(7)  // {UINT16, Nm, 1/32} {UINT16, Nm, 1/32}
#define CPM_EXTREME_ANGLES                  bitMask(8)
#define CPM_TOP_DEAD_SPOT_ANGLE             bitMask(9)
#define CPM_BOTTOM_DEAD_SPOT_ANGLE          bitMask(10)
#define CPM_ACCUMMULATED_ENERGY             bitMask(11)
#define CPM_OFFSET_COMPENSATION_INDICATOR   bitMask(12)
#define CPM_RFU                             0xE000U     // bits 13-15

// Sensor Location (GATT Spec Supp 3.195)
//
// The Sensor Location characteristic is used to represent the location of the sensor.
//
struct SensorLocation {
    uint8_t sensorLocation;
} __attribute__((packed));
typedef struct SensorLocation SensorLocation;

// Sensor Location field (GATT Spec Supp 3.195.1)
#define SL_OTHER            0x00U
#define SL_TOP_OF_SHOE      0x01U
#define SL_IN_SHOE          0x02U
#define SL_HIP              0x03U
#define SL_FRONT_WHEEL      0x04U
#define SL_LEFT_CRANK       0x05U
#define SL_RIGHT_CRANK      0x06U
#define SL_LEFT_PEDAL       0x07U
#define SL_RIGHT_PEDAL      0x08U
#define SL_FRONT_HUB        0x09U
#define SL_REAR_DROPOUT     0x0aU
#define SL_CHAINSTAY        0x0bU
#define SL_REAR_WHEEL       0x0cU
#define SL_REAR_HUB         0x0dU
#define SL_CHEST            0x0eU
#define SL_SPIDER           0x0fU
#define SL_CHAIN_RING       0x10U

// Cycling Power Control Point (GATT Spec Supp 3.63)
//
// The Cycling Power Control Point characteristic is used to enable device-specific procedures related to a
// cycling power sensor.
//
struct CycPowerCP {
    uint8_t opCode;
    uint8_t parm[0];    // 0-18 octets
} __attribute__((packed));
typedef struct CycPowerCP CycPowerCP;

// Cycling Power Control Point Op Code and Param fields (GATT Spec Supp 3.63.1)
#define CPCP_RFU                                0x00U
#define CPCP_SET_CUMULATIVE_VALUE               0x01U
#define CPCP_UPDATE_SENSOR_LOCATION             0x02U   // {UINT8}
#define CPCP_REQ_SUPPORTED_SENSOR_LOCATIONS     0x03U
#define CPCP_SET_CRANK_LENGTH                   0x04U
#define CPCP_REQ_CRANK_LENGTH                   0x05U
#define CPCP_SET_CHAIN_LENGTH                   0x06U
#define CPCP_REQ_CHAIN_LENGTH                   0x07U
#define CPCP_SET_CHAIN_WEIGHT                   0x08U
#define CPCP_REQ_CHAIN_WEIGHT                   0x09U
#define CPCP_SET_SPAN_LENGTH                    0x0aU
#define CPCP_REQ_SPAN_LENGTH                    0x0bU
#define CPCP_START_OFFSET_COMPENSATION          0x0cU
#define CPCP_MASK_CPM_CHARACTERISTICS           0x0dU
#define CPCP_REQ_SAMPLING_RATE                  0x0eU
#define CPCP_REQ_FACTORY_CALIBRATION_DATE       0x0fU
#define CPCP_START_ENH_OFFSET_COMPENSATION      0x10U
#define CPCP_RESPONSE_CODE                      0x20U   // {UINT8, UINT8, Params}

// Cycling Power Control Point Response Code Values (GATT Spec Supp 3.63.2)
#define CPCP_RCV_RFU                    0x00U
#define CPCP_RCV_SUCCESS                0x01U
#define CPCP_RCV_OP_CODE_NOT_SUPPORTED  0x02U
#define CPCP_RCV_INVALID_OPERAND        0x03U
#define CPCP_RCV_OPERATION_FAILED       0x05U



