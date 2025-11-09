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

#pragma once

#include <stdint.h>
#include <sys/time.h>

#include "server.h"
#include "uuid.h"

// Default TCP port used by DIRCON
#define DIRCON_TCP_PORT 36866

// Message Type
typedef enum MesgType {
    request = 1,
    response = 2,
} MesgType;

// DIRCON messages have the following format:
//
//       7 6 5 4 3 2 1 0
//      +-+-+-+-+-+-+-+-+
//    0 |    Version    |
//      +---------------+
//    1 |    Mesg ID    | (see below)
//      +---------------+
//    2 |    Seq Num    |
//      +---------------+
//    3 |   Resp Code   | (see below)
//      +---------------+
//    4 |               |
//      +  Data Length  + (network byte order)
//    5 |               |
//      +---------------+
//    6 |       .       |
//      |       .       |
//      | Optional Data |
//      |       .       |
//  N-1 |       .       |
//      +---------------+
//
//  Message ID:
//    0x01 - Discover Services
//    0x02 - Discover Characteristics
//    0x03 - Read Characteristic
//    0x04 - Write Characteristic
//    0x05 - Enable Characteristic Notifications
//    0x06 - Unsolicited Characteristic Notification
//    0xff - Error
//
//  Response Code:
//    0x00 - Success Request
//    0x01 - Unknown Message Type
//    0x02 - Unexpected Error
//    0x03 - Service Not Found
//    0x04 - Characteristic Not Found
//    0x05 - Characteristic Operation Not Supported
//    0x06 - Characteristic Write Failed
//    0x07 - Unknown Protocol
//

#define DIRCON_VERSION  0x01

typedef enum DirconMesgId {
    DiscoverServices = 0x01,
    DiscoverCharacteristics = 0x02,
    ReadCharacteristic = 0x03,
    WriteCharacteristic = 0x04,
    EnableCharacteristicNotifications = 0x05,
    UnsolicitedCharacteristicNotification = 0x06,
    Error = 0xff,
} DirconMesgId;

typedef enum DirconRespCode {
    SuccessRequest = 0x00,
    UnknownMessageType = 0x01,
    UnexpectedError = 0x02,
    ServiceNotFound = 0x03,
    CharacteristicNotFound = 0x04,
    CharacteristicOperationNotSupported = 0x05,
    CharacteristicWriteFailed = 0x06,
    UnknownProtocol = 0x07,
} DirconRespCode;

// Generic DIRCON message
struct DirconMesg {
    uint8_t version;
    uint8_t mesgId;
    uint8_t seqNum;
    uint8_t respCode;
    uint16_t mesgLen;
    uint8_t data[0];
} __attribute__((packed));
typedef struct DirconMesg DirconMesg;

// Discover Services message
struct DiscSvcsMesg {
    DirconMesg hdr;
    Uuid128 svcUuid[0]; // response message only
} __attribute__((packed));
typedef struct DiscSvcsMesg DiscSvcsMesg;

// Characteristic Properties
struct CharProp {
    Uuid128 charUuid;
    uint8_t properties;
} __attribute__((packed));
typedef struct CharProp CharProp;

// Properties of a Characteristic
#define DIRCON_CHAR_PROP_READ       0x01U
#define DIRCON_CHAR_PROP_WRITE      0x02U
#define DIRCON_CHAR_PROP_NOTIFY     0x04U
#define DIRCON_CHAR_PROP_MASK       0x07U

// Discover Characteristics message
struct DiscCharsMesg {
    DirconMesg hdr;
    Uuid128 svcUuid;
    CharProp charProp[0];    // response message only
} __attribute__((packed));
typedef struct DiscCharsMesg DiscCharsMesg;

// Read Characteristics message
struct ReadCharMesg {
    DirconMesg hdr;
    Uuid128 charUuid;
    uint8_t data[0];    // response message only
} __attribute__((packed));
typedef struct ReadCharMesg ReadCharMesg;

// Write Characteristics message
struct WriteCharMesg {
    DirconMesg hdr;
    Uuid128 charUuid;
    uint8_t data[0];    // format depends on the characteristic
} __attribute__((packed));
typedef struct WriteCharMesg WriteCharMesg;

// Enable Characteristic Notifications message
struct EnCharNotMesg {
    DirconMesg hdr;
    Uuid128 charUuid;
    uint8_t enable;
} __attribute__((packed));
typedef struct EnCharNotMesg EnCharNotMesg;

// UnsolicitedCharacteristicNotification message
struct UnsCharNot {
    DirconMesg hdr;
    Uuid128 charUuid;
    uint8_t data[0];    // format depends on the characteristic
} __attribute__((packed));
typedef struct UnsCharNot UnsCharNot;

__BEGIN_DECLS

extern int8_t getSINT8(const uint8_t *data);
extern int16_t getSINT16(const uint8_t *data);
extern int32_t getSINT24(const uint8_t *data);
extern int32_t getSINT32(const uint8_t *data);

extern uint8_t getUINT8(const uint8_t *data);
extern uint16_t getUINT16(const uint8_t *data);
extern uint32_t getUINT24(const uint8_t *data);
extern uint32_t getUINT32(const uint8_t *data);

extern void putSINT8(uint8_t *data, int8_t value);
extern void putSINT16(uint8_t *data, int16_t value);
extern void putSINT24(uint8_t *data, int32_t value);
extern void putSINT32(uint8_t *data, int32_t value);

extern void putUINT8(uint8_t *data, uint8_t value);
extern void putUINT16(uint8_t *data, uint16_t value);
extern void putUINT24(uint8_t *data, uint32_t value);
extern void putUINT32(uint8_t *data, uint32_t value);

extern int dirconInit(Server *server);
extern int dirconProcConnReq(Server *server);
extern int dirconProcConnDrop(Server *server);
extern int dirconProcTimers(Server *server, const struct timeval *time);
extern int dirconProcMesg(Server *server, DirconSessId sessId);
extern int dirconSendDiscoverServicesMesg(Server *server, DirconSession *sess);
extern int dirconSendDiscoverCharacteristicsMesg(Server *server, DirconSession *sess, const Uuid128 *svcUuid);
extern int dirconSendEnableCharacteristicNotificationsMesg(Server *server, DirconSession *sess, const Uuid128 *charUuid);
extern int dirconSendReadCharacteristicMesg(Server *server, DirconSession *sess, const Uuid128 *uuid);
extern int dirconSendWriteCharacteristicMesg(Server *server, DirconSession *sess, const Uuid128 *uuid);

__END_DECLS
