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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "dircon.h"
#include "mdns.h"
#include "mlog.h"
#include "server.h"

// Program's major/minor version numbers
#define PROG_VER_MAJOR  0
#define PROG_VER_MINOR  0

typedef enum PollFdIndex {
    connSock = 0,
    mdnsSock = 1,
    srvSock = 2,
    stdInput = 3,
} PollFdIndex;

// The main Server object
static Server serverObj = {0};

static const char *help =
        "SYNTAX:\n"
        "    indBikeSim [OPTIONS]\n"
        "\n"
        "OPTIONS:\n"
        "    --activity <file>\n"
        "        Specifies the FIT file of the cycling activity to be used to\n"
        "        get the metrics sent in the 'Indoor Bike Data' notification\n"
        "        messages.\n"
        "    --cadence <val>\n"
        "        Specifies a fixed cadence value (in RPM) to be sent in the\n"
        "        periodic 'Cycling Power Measurement' and 'Indoor Bike Data'\n"
        "        notifications.\n"
        "    --dissect <mesg-id>\n"
        "        Dissect the WFTNP messages that match the specified message ID\n"
        "        Valid values are:\n"
        "          0 - any\n"
        "          1 - Discover Services\n"
        "          2 - Discover Characteristics\n"
        "          3 - Read Characteristic\n"
        "          4 - Write Characteristic\n"
        "          5 - Enable Characteristic Notifications\n"
        "          6 - Unsolicited Characteristic Notification\n"
        "    --heart-rate <val>\n"
        "        Specifies a fixed heart rate value (in BPM) to be sent\n"
        "        in the periodic 'Indoor Bike Data' notifications.\n"
        "    --help\n"
        "        Show this help and exit.\n"
        "    --hex-dump\n"
        "        Do a hex dump of the DIRCON messages sent and received.\n"
        "    --ip-address <addr>\n"
        "        Specifies the interface IP address to use to advertise the\n"
        "        WFTNP mDNS service.\n"
        "    --log-dest {both|console|file}\n"
        "        Specifies the destination of the log messages. The default is\n"
        "        'console'.\n"
        "    --log-level {none|info|trace|debug}\n"
        "        Set the specified message log level. The default level is\n"
        "        \"info\".\n"
        "    --no-mdns\n"
        "        Don't use mDNS to advertise the WFTNP service on the local\n"
        "        network.\n"
        "    --power <val>\n"
        "        Specifies a fixed pedal power value (in Watts) to be sent\n"
        "        in the periodic 'Indoor Bike Data' notifications.\n"
        "    --speed <val>\n"
        "        Specifies a fixed speed value (in km/h) to be sent\n"
        "        in the periodic 'Indoor Bike Data' notifications.\n"
        "    --supported-power-range <min,max,inc>\n"
        "        Specifies the minimum, maximum, and increment power values\n"
        "        (in Watts) used by the Supported Power Range characteristic.\n"
        "        Default is 0,1500,1."
        "    --tcp-port <num>\n"
        "        Specifies the TCP port to use. Default is 36866.\n"
        "    --version\n"
        "        Show version information and exit.\n"
        "\n"
        "BUGS:\n"
        "    Report bugs and enhancement requests to: marcelo_mourier@yahoo.com\n";

static int invalidArgument(const char *arg, const char *val)
{
    fprintf(stderr, "Invalid argument: %s %s\n", arg, (val != NULL) ? val : "");
    return -1;
}

static int missingArgValue(const char *arg)
{
    fprintf(stderr, "Option %s requires a value.\n", arg);
    return -1;
}

static int parseArgs(int argc, char **argv, Server *server)
{
    int n, numArgs;

    // Set defaults
    server->srvAddr.sin_family = 0;
    server->srvAddr.sin_port = htons(DIRCON_TCP_PORT);
    server->minPower = 0;
    server->maxPower = 1500;
    server->incPower = 1;

    for (n = 1, numArgs = argc -1; n <= numArgs; n++) {
        const char *arg;
        const char *val;

        arg = argv[n];

        if (strcmp(arg, "--activity") == 0) {
#ifdef CONFIG_FIT_ACTIVITY_FILE
            FILE *fp;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if ((fp = fopen(val, "r")) == NULL) {
                return invalidArgument(arg, val);
            }
            server->actFile = fp;
#else
            return invalidArgument(arg, NULL);
#endif
        } else if (strcmp(arg, "--cadence") == 0) {
            uint16_t cadence;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (sscanf(val, "%hu", &cadence) != 1) {
                return invalidArgument(arg, val);
            }
            server->cadence = cadence * 2;  // FTMS cadence unit is 0.5 RPM
        } else if (strcmp(arg, "--dissect") == 0) {
            int dissectMesgId;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if ((sscanf(val, "%d", &dissectMesgId) != 1) ||
                (dissectMesgId < 0) ||
                (dissectMesgId > UnsolicitedCharacteristicNotification)) {
                return invalidArgument(arg, val);
            }
            server->dissectMesgId = dissectMesgId;
            server->dissect = true;
        } else if (strcmp(arg, "--heart-rate") == 0) {
            uint16_t heartRate;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (sscanf(val, "%hu", &heartRate) != 1) {
                return invalidArgument(arg, val);
            }
            server->heartRate = heartRate;
        } else if (strcmp(arg, "--help") == 0) {
            fprintf(stdout, "%s\n", help);
            exit(0);
        } else if (strcmp(arg, "--hex-dump") == 0) {
            server->hexDumpMesg = true;
        } else if (strcmp(arg, "--ip-address") == 0) {
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (inet_pton(AF_INET, val, &server->srvAddr.sin_addr) != 1) {
                return invalidArgument(arg, val);
            }
#ifdef CONFIG_MSGLOG
        } else if (strcmp(arg, "--log-dest") == 0) {
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            LogDest dest;
            if (strcmp(val, "both") == 0) {
                dest = both;
            } else if (strcmp(val, "console") == 0) {
                dest = console;
            } else if (strcmp(val, "file") == 0) {
                dest = file;
            } else {
                return invalidArgument(arg, val);
            }
            msgLogSetDest(dest);
        } else if (strcmp(arg, "--log-level") == 0) {
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            LogLevel level;
            if (strcmp(val, "none") == 0) {
                level = none;
            } else if (strcmp(val, "info") == 0) {
                level = info;
            } else if (strcmp(val, "trace") == 0) {
                level = trace;
            } else if (strcmp(val, "debug") == 0) {
                level = debug;
            } else {
                return invalidArgument(arg, val);
            }
            msgLogSetLevel(level);
#endif
        } else if (strcmp(arg, "--no-mdns") == 0) {
            server->noMdns = true;
        } else if (strcmp(arg, "--power") == 0) {
            uint16_t power;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (sscanf(val, "%hu", &power) != 1) {
                return invalidArgument(arg, val);
            }
            server->power = power;
        } else if (strcmp(arg, "--speed") == 0) {
            uint16_t speed;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (sscanf(val, "%hu", &speed) != 1) {
                return invalidArgument(arg, val);
            }
            server->speed = speed * 100;    // FTMS speed unit is 0.01 km/h
        } else if (strcmp(arg, "--supported-power-range") == 0) {
            uint16_t minPwr, maxPwr, incPwr;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if (sscanf(val, "%hu,%hu,%hu", &minPwr, &maxPwr, &incPwr) != 3) {
                return invalidArgument(arg, val);
            }
            server->minPower = minPwr;
            server->maxPower = maxPwr;
            server->incPower = incPwr;
        } else if (strcmp(arg, "--tcp-port") == 0) {
            uint16_t tcpPort;
            if ((val = argv[++n]) == NULL) {
                return missingArgValue(arg);
            }
            if ((sscanf(val, "%hu", &tcpPort) != 1) ||
                (tcpPort < 1024) ||
                (tcpPort > 49151)) {
                return invalidArgument(arg, val);
            }
            server->srvAddr.sin_port = htons(tcpPort);
        } else if (strcmp(arg, "--version") == 0) {
            fprintf(stdout, "Program version %d.%d built on %s %s\n", PROG_VER_MAJOR, PROG_VER_MINOR, __DATE__, __TIME__);
            exit(0);
        } else if (strncmp(arg, "--", 2) == 0) {
            return invalidArgument(arg, NULL);
        }
    }

    return 0;
}

static void sigHandler(int sigNum, siginfo_t *sigInfo, void *ucontext)
{
    // Gracefully exit the app
    serverObj.exit = true;
}

int main(int argc, char *argv[])
{
    Server *server = &serverObj;
    struct sigaction sigAct = {{0}};

    // Install SIGINT handler
    sigAct.sa_sigaction = sigHandler;
    sigAct.sa_flags = SA_SIGINFO;
    if (sigaction(SIGINT, &sigAct, NULL) != 0) {
        return -1;
    }

    // Parse the command line arguments
    if (parseArgs(argc, argv, server) != 0) {
        return -1;
    }

    mlog(info, "dirconServer version %d.%d built on %s %s", PROG_VER_MAJOR, PROG_VER_MINOR, __DATE__, __TIME__);

#ifdef CONFIG_CLI
    // Initialize CLI
    if (cliInit(server) != 0) {
        return -1;
    }
#endif

    // Initialize Server
    if (serverInit(server) != 0) {
        cliPreExitCleanup(server);
        return -1;
    }

#ifdef CONFIG_MDNS_AGENT
    // Initialize mDNS
    if (mdnsInit(server) != 0) {
        cliPreExitCleanup(server);
        return -1;
    }
#endif

    // Run server's work loop
    if (serverRun(server) != 0) {
        cliPreExitCleanup(server);
        return -1;
    }

    return 0;
}
