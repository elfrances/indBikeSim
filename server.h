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

#include <netinet/in.h>
#include <stdio.h>
#include <sys/queue.h>

#include "binbuf.h"
#include "defs.h"
#include "svc.h"
#include "trkpt.h"

// DIRCON Session ID
typedef enum DirconSessId {
    inv = 0,
    app = 1,    // DIRCON session with the virtual cycling app
    dev = 2,    // DIRCON session with the indoor bike trainer
    maxSess = 3
} DirconSessId;

// DIRCON Session Info
typedef struct DirconSession {
    DirconSessId sessId;                    // back reference
    char peerName[64];                      // name of the DIRCON peer device
    int cliSockFd;                          // file descriptor of the client (connected) DIRCON socket
    struct sockaddr_in locCliAddr;          // local-end of client socket address
    struct sockaddr_in remCliAddr;          // remote-end of client socket address
    struct timeval nextNotification;        // when next notifications are due
    uint32_t rxMesgCnt;
    uint32_t txMesgCnt;
    uint8_t lastTxReqSeqNum;
    uint8_t lastRxReqSeqNum;
    bool fmcpNotificationsEnabled;          // Fitness Machine Control Point notifications enabled
    bool ibdNotificationsEnabled;           // Indoor Bike Data notifications enabled
    bool respPend;                          // server-initiated DIRCON transaction in progress
} DirconSession;

// Connection Type
typedef enum ConnType {
    dircon = 1,
    ble = 2,
} ConnType;

// Selected Characteristic Info
typedef struct SelChrInfo {
    ConnType connType;
    union {
       struct {
           uint16_t connHandle;
           uint16_t valHandle;
           uint16_t dscHandle;
       } ble;
       struct {
           int sockFd;
       } dircon;
    } u;
} SelChrInfo;

// Message direction
typedef enum MesgDir {
    TxDir = 1,
    RxDir = 2,
} MesgDir;

// Max Rx/Tx message length
#define MAX_MESG_LEN    512

// Max number of arguments in a CLI command
#define MAX_ARGS    8

// DIRCON Server
typedef struct Server {
    int stdinFd;                    // file descriptor of stdin stream
    int srvSockFd;                  // file descriptor of the server (listening) DIRCON socket
    int mdnsSockFd;                 // file descriptor of the MDNS UDP socket

    struct sockaddr_in srvAddr;     // listening socket address
    struct sockaddr_in mdnsAddr;    // mDNS socket address

    uint8_t macAddr[6];             // MAC address of the local network interface

    // DIRCON sessions
    DirconSession dirconSession[maxSess];

    // List of supported services/characteristics
    TAILQ_HEAD(SvcList, Service) svcList;

#ifdef CONFIG_FIT_ACTIVITY_FILE
    FILE *actFile;                  // FIT/TCX activity file
    TAILQ_HEAD(TrkPtList, TrkPt) trkPtList; // list of trackpoints from the activity file
#endif

    struct timeval baseTime;        // base time used to generate relative timestamps

    // Rx/Tx message buffers
    uint8_t rxMesgBuf[MAX_MESG_LEN];
    uint8_t txMesgBuf[MAX_MESG_LEN];

    uint32_t rxMdnsMesgCnt;
    uint32_t txMdnsMesgCnt;

    uint16_t cadence;               // Cadence [RPM]
    uint16_t heartRate;             // Heart Rate [BPM]
    uint16_t power;                 // Power [Watts]

    int dissectMesgId;

    bool actInProg;                 // activity in progress
    bool dissect;
    bool exit;
    bool hexDumpMesg;
    bool mesgForwarding;
    bool noMdns;
} Server;

__BEGIN_DECLS

extern int serverInit(Server *server);
extern int serverConnectToDirconTrainer(Server *server);
extern int serverProcConnDrop(Server *server, DirconSessId sessId);
extern int serverRun(Server *server);

extern Service *serverAddService(Server *server, const Uuid128 *uuid);
extern Service *serverFindService(const Server *server, const Uuid128 *uuid);

extern DirconSessId swapDirconSessId(DirconSessId sessId);
extern const char *fmtDirconSessId(DirconSessId sessId);

extern const char *fmtSockaddr(const struct sockaddr_in *sockAddr, bool printPort);

__END_DECLS
