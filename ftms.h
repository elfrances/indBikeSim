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

// All definitions based on the Bluetooth "Fitness Machine Service" doc:
//
// https://www.bluetooth.com/specifications/specs/fitness-machine-service-1-0/
//

// Fitness Machine Feature (FTMS 4.3)
//
// The Fitness Machine Feature characteristic shall be used to describe the supported features of the
// Server.
//
struct FitMachFeat {
    uint8_t fmFeat[4]; // Fitness Machine Features
    uint8_t tsFeat[4]; // Target Setting Features
} __attribute__((packed));
typedef struct FitMachFeat FitMachFeat;

// Fitness Machine Features (FTMS 4.3.1.1)
#define FMF_AVERAGE_SPEED           bitMask(0)
#define FMF_CADENCE                 bitMask(1)
#define FMF_TOTAL_DISTANCE          bitMask(2)
#define FMF_INCLINATION             bitMask(3)
#define FMF_ELEVATION_GAIN          bitMask(4)
#define FMF_PACE                    bitMask(5)
#define FMF_STEP_COUNT              bitMask(6)
#define FMF_RESISTANCE_LEVEL        bitMask(7)
#define FMF_STRIDE_COUNT            bitMask(8)
#define FMF_EXPENDED_ENERGY         bitMask(9)
#define FMF_HEART_RATE_MEASURMENT   bitMask(10)
#define FMF_METABOLIC_EQUIVALENT    bitMask(11)
#define FMF_ELAPSED_TIME            bitMask(12)
#define FMF_REMAINING_TIME          bitMask(13)
#define FMF_POWER_MEASUREMENT       bitMask(14)
#define FMF_FORCE_ON_BELT           bitMask(15)
#define FMF_USER_DATA_RETENTION     bitMask(16)
#define FMF_RFU                     0xFFFE0000U // bits 17-31

// Target Settings Features (FTMS 4.3.1.2)
//
// When a bit is set to 1 (True) in the Target Setting Features field, the Server supports the associated control
// related feature. If the Server does not support the relevant control related feature, the associated feature
// bit shall be set to 0 (False)
//
#define TSF_SPEED                   bitMask(0)
#define TSF_INCLINATION             bitMask(1)
#define TSF_RESISTANCE              bitMask(2)
#define TSF_POWER                   bitMask(3)
#define TSF_HEART_RATE              bitMask(4)
#define TSF_EXPENDED_ENERGY         bitMask(5)
#define TSF_STEP_NUMBER             bitMask(6)
#define TSF_STRIDE_NUMBER           bitMask(7)
#define TSF_DISTANCE                bitMask(8)
#define TSF_TRAINING_TIME           bitMask(9)
#define TSF_TIME_IN_TWO_HRZ         bitMask(10)
#define TSF_TIME_IN_THREE_HRZ       bitMask(11)
#define TSF_TIME_IN_FIVE_HRZ        bitMask(12)
#define TSF_INDOOR_BIKE_SIM_PARMS   bitMask(13)
#define TSF_WHEEL_CIRCUMFERENCE     bitMask(14)
#define TSF_SPIN_DOWN               bitMask(15)
#define TSF_CADENCE                 bitMask(16)
#define TSF_RFU                     0xFFFE0000U // bits 17-31

// Indoor Bike Data (FTMS 4.9)
//
// The Indoor Bike Data characteristic is used to send training-related data to the Client from an indoor bike
// (Server).
//
struct IndoorBikeData {
    uint8_t flags[2];
    uint8_t data[0];
} __attribute__((packed));
typedef struct IndoorBikeData IndoorBikeData;

// Indoor Bike Data Flags (FTMS 4.9.1.1)
// NOTE: when bit #0 in the flags field is NOT set
// then the 16-bit Instantaneous Speed value is
// present; otherwise the value is NOT present.
#define IBD_INSTANTANEOUS_SPEED     bitMask(0)  // {UINT16, kph, 0.01}
#define IBD_MORE_DATA               bitMask(0)  // NOTE: when bit #0 is 1 -> Inst. Speed is NOT present and more data exists
#define IBD_AVERAGE_SPEED           bitMask(1)  // {UINT16, kph, 0.01}
#define IBD_INSTANTANEOUS_CADENCE   bitMask(2)  // {UINT16, rpm, 0.5}
#define IBD_AVERAGE_CADENCE         bitMask(3)  // {UINT16, rpm, 0.5}
#define IBD_TOTAL_DISTANCE          bitMask(4)  // {UINT24, meters, 1}
#define IBD_RESISTANCE_LEVEL        bitMask(5)  // {UINT8, unitless, 1}
#define IBD_INSTANTANEOUS_POWER     bitMask(6)  // {UINT16, watts, 1}
#define IBD_AVERAGE_POWER           bitMask(7)  // {UINT16, watts, 1}
#define IBD_EXPENDED_ENERGY         bitMask(8)  // {UINT16, kg.cal, 1}
#define IBD_HEART_RATE              bitMask(9)  // {UINT8, bpm, 1}
#define IBD_METABOLIC_EQUIVALENT    bitMask(10) // {UINT8, me, 0.1}
#define IBD_ELAPSED_TIME            bitMask(11) // {UINT16, seconds, 1}
#define IBD_REMAINING_TIME          bitMask(12) // {UINT16, seconds, 1}

// Training Status (FTMS 4.10)
//
// The Training Status characteristic shall be used by the Server to send the training status information to
// the Client.
//
struct TrainingStatus {
    uint8_t flags;
    uint8_t trainingStatus;
    uint8_t data[0];
} __attribute__((packed));
typedef struct TrainingStatus TrainingStatus;

// Training Status Flags (FTMS 4.10.1.1)
#define TSF_TRAINING_STATUS_STRING  bitMask(0)
#define TSF_EXTENDED_STRING         bitMask(1)

// Training Status Field (FTMS 4.10.1.2)
#define TSF_OTHER                   0x00U
#define TSF_IDLE                    0x01U
#define TSF_WARMING_UP              0x02U
#define TSF_LOW_INTENSITY_INTERVAL  0x03U
#define TSF_HIGH_INTENSITY_INTERVAL 0x04U
#define TSF_RECOVERY_INTERVAL       0x05U
#define TSF_ISOMETRIC               0x06U
#define TSF_HEART_RATE_CONTROL      0x07U
#define TSF_FITNESS_TEST            0x08U
#define TSF_SPEED_OCR_LOW           0x09U
#define TSF_SPEED_OCR_HIGH          0x0aU
#define TSF_COOL_DOWN               0x0bU
#define TSF_WATT_CONTROL            0x0cU
#define TSF_MANUAL_MODE             0x0dU
#define TSF_PRE_WORKOUT             0x0eU
#define TSF_POST_WORKOUT            0x0fU
#define TST_RFU                     0xF0U

// Supported Resistance Level Range (FTMS 4.13)
//
// The Supported Resistance Level Range characteristic is used to send the supported resistance level
// range as well as the minimum resistance increment supported by the Server. Included in the
// characteristic value are a Minimum Resistance Level field, a Maximum Resistance Level field, and a
// Minimum Increment field
//
struct ResistLevelRange {
    uint8_t minResLevel;    // {UINT8, [unitless], 1}
    uint8_t maxResLevel;    // {UINT8, [unitless], 1}
    uint8_t incResLevel;    // {UINT8, [unitless], 1}
} __attribute__((packed));
typedef struct ResistLevelRange ResistLevelRange;

// Supported Power Range (FTMS 4.14)
//
// The Supported Power Range characteristic is used to send the supported power range as well as the
// minimum power increment supported by the Server. Included in the characteristic value are a Minimum
// Power field, a Maximum Power field, and a Minimum Increment field
//
struct PowerRange {
    uint8_t minPower[2];    // {SINT16, [watts], 1}
    uint8_t maxPower[2];    // {SINT16, [watts], 1}
    uint8_t incPower[2];    // {UINT16, [watts], 1}
} __attribute__((packed));
typedef struct PowerRange PowerRange;

// Fitness Machine Control Point (FTMS 4.16)
//
// The Fitness Machine Control Point characteristic is used to request a specific function to be executed on
// the Server.
//
struct FitMachCP {
    uint8_t opCode;
    uint8_t parm[0];    // 0-18 octets
} __attribute__((packed));
typedef struct FitMachCP FitMachCP;

// Fitness Machine Control Point Op Codes (FTMS 4.16.1)
#define FMCP_REQUEST_CONTROL            0x00U   // parm: n/a
#define FMCP_RESET                      0x01U   // parm: n/a
#define FMCP_SET_TGT_SPEED              0x02U   // parm: UINT16 (km/h)
#define FMCP_SET_TGT_INCLINATION        0x03U   // parm: SINT16 (%)
#define FMCP_SET_TGT_RESISTANCE         0x04U   // parm: UINT8 (unitless)
#define FMCP_SET_TGT_POWER              0x05U   // parm: SINT16 (watts)
#define FMCP_SET_TGT_HEART_RATE         0x06U   // parm: UINT8 (bpm)
#define FMCP_START_OR_RESUME            0x07U   // parm: n/a
#define FMCP_STOP_OR_PAUSE              0x08U   // parm: UINT8
#define FMCP_SET_TGT_EXP_ENERGY         0x09U   // parm: UINT16 (calories)
#define FMCP_SET_TGT_NUM_STEPS          0x0aU   // parm: UINT16 (steps)
#define FMCP_SET_TGT_NUM_STRIDES        0x0bU   // parm: UINT16 (strides)
#define FMCP_SET_TGT_DISTANCE           0x0cU   // parm: UINT24 (meters)
#define FMCP_SET_TGT_TRAINING_TIME      0x0dU   // parm: UINT16 (seconds)
#define FMCP_SET_TGT_TIME_2_HRZ         0x0eU
#define FMCP_SET_TGT_TIME_3_HRZ         0x0fU
#define FMCP_SET_TGT_TIME_5_HRZ         0x10U
#define FMCP_SET_INDOOR_BIKE_SIM_PARMS  0x11U   // parm: {SINT16 (mps), SINT16 (%), UINT8 (unitless), UINT8 (kg/m)}
#define FMCP_SET_WHEEL_CIRCUMFERENCE    0x12U   // parm: UINT16 (mm)
#define FMCP_SET_SPIN_DOWN_CONTROL      0x13U
#define FMCP_SET_TGT_CADENCE            0x14U   // parm: UINT16 (rpm)
#define FMCP_SET_TGT_RESPONSE_CODE      0x80U

// Indoor Bike Simulation Parameters (FTMS 4.16.2.18)
struct IndBikeSimParms {
    uint8_t windSpeed[2];   // {SINT16, mps, 0.001}
    uint8_t grade[2];       // {SINT16, percentage, 0.01}
    uint8_t crr;            // {UINT8, unitless, 0.0001}
    uint8_t cw;             // {UINT8, kg/m, 0.01}
} __attribute__((packed));
typedef struct IndBikeSimParms IndBikeSimParms;
