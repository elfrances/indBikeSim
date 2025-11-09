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

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli.h"
#include "dircon.h"
#include "mdns.h"

#ifdef CONFIG_CLI

#include <readline/readline.h>
#include <readline/history.h>

#ifdef __DARWIN__
// These are missing in macOS
static HIST_ENTRY **history_list (void)
{
    // TBD
    return NULL;
} 
#endif

typedef struct CliInfo {
    Server *server;
    char *cmdLine;
    int argc;
    char *argv[MAX_ARGS];
} CliInfo;

static CliInfo cliInfo = {0};

// Command status code
typedef enum CmdStat {
    OK = 0,
    ERROR = 1,
    EXIT = 2
} CmdStat;

static const char *cliHelp = \
    "Supported CLI commands:\n"
    "\n"
    "disc-char {app|dev} <uuid>\n"
    "    Send out a DIRCON 'Discover Characteristics' message\n"
    "    to the specified target.\n"
    "\n"
    "disc-serv {app|dev}\n"
    "    Send out a DIRCON 'Discover Services' message to the.\n"
    "    specified target.\n"
    "\n"
    "exit\n"
    "    Exit the tool.\n"
    "\n"
    "help\n"
    "    Print this help.\n"
    "\n"
    "history\n"
    "    Print the command history.\n"
    "\n"
    "mdns-query <qname>\n"
    "    Send out an mDNS query for the given name.\n"
    "\n"
    "read-char {app|dev} <uuid>\n"
    "    Send out a DIRCON 'Read Characteristic' message to\n"
    "    the specified target.\n"
    "\n"
    "show\n"
    "    Show seesion information.\n"
    "\n"
    "NOTES:\n"
    "\n"
    "\n";

typedef struct CliCmd {
    const char *name;
    CmdStat (*handler)(CliInfo *);
    int minArgCnt;
    int maxArgCnt;
    const char *args;
    bool needConn;
} CliCmd;

static int invArg(const char *arg)
{
    fprintf(stderr, "ERROR: invalid argument \"%s\"\n", arg);
    return ERROR;
}

static int sessIsDown(void)
{
    fprintf(stderr, "ERROR: DIRCON session is down\n");
    return ERROR;
}

static DirconSessId scanDirconSessId(const char *val)
{
    DirconSessId sessId = inv;

    if (strcmp(val, "app") == 0) {
        sessId = app;
    } else if (strcmp(val, "dev") == 0) {
        sessId = dev;
    }

    return sessId;
}

static int scanUuid16(Uuid16 *uuid, const char *val)
{
    uint32_t data[2];

    if (strlen(val) == 4) {
        if (sscanf(val, "%02x%02x", &data[0], &data[1]) == 2) {
            uuid->data[0] = data[0];
            uuid->data[1] = data[1];
            return 0;
        }
    }

    return -1;
}

static CmdStat cliCmdDiscChar(CliInfo *cliInfo)
{
    Server *server = cliInfo->server;
    DirconSessId sessId;
    Uuid16 uuid = {0};
    Uuid128 svcUuid = {0};

    if ((sessId = scanDirconSessId(cliInfo->argv[1])) == inv) {
        return invArg(cliInfo->argv[1]);
    }

    if (scanUuid16(&uuid, cliInfo->argv[2]) != 0) {
        return invArg(cliInfo->argv[2]);
    }

    if (server->dirconSession[sessId].cliSockFd == 0) {
        return sessIsDown();
    }

    buildUuid128(&svcUuid, &uuid);
    dirconSendDiscoverCharacteristicsMesg(server, &server->dirconSession[sessId], &svcUuid);

    return OK;
}

static CmdStat cliCmdDiscServ(CliInfo *cliInfo)
{
    Server *server = cliInfo->server;
    DirconSessId sessId;

    if ((sessId = scanDirconSessId(cliInfo->argv[1])) == inv) {
        return invArg(cliInfo->argv[1]);
    }

    if (server->dirconSession[sessId].cliSockFd == 0) {
        return sessIsDown();
    }

    dirconSendDiscoverServicesMesg(server, &server->dirconSession[sessId]);

    return OK;
}

static CmdStat cliCmdExit(CliInfo *cliInfo)
{
    // Exit the work loop
    cliInfo->server->exit = 1;

    return OK;
}

static CmdStat cliCmdHelp(CliInfo *cliInfo)
{
    printf("%s", cliHelp);
    return OK;
}

static CmdStat cliCmdHistory(CliInfo *cliInfo)
{
    HIST_ENTRY **histList = history_list();

    if (histList != NULL) {
        for (int n = 0; histList[n] != NULL; n++) {
            HIST_ENTRY *histEntry = histList[n];
            if (histEntry != NULL) {
                printf("%s\n", histEntry->line);
            }
        }
    }

    return OK;
}

static CmdStat cliCmdMdnsQuery(CliInfo *cliInfo)
{
    FmtBuf qname;
    char buf[256];

    fmtBufInit(&qname, buf, sizeof (buf));
    fmtBufAppend(&qname, "%s", cliInfo->argv[1]);
    mdnsSendQuery(cliInfo->server, &qname);

    return OK;
}

static CmdStat cliCmdReadChar(CliInfo *cliInfo)
{
    Server *server = cliInfo->server;
    DirconSessId sessId;
    Uuid16 uuid = {0};
    Uuid128 charUuid = {0};

    if ((sessId = scanDirconSessId(cliInfo->argv[1])) == inv) {
        return invArg(cliInfo->argv[1]);
    }

    if (scanUuid16(&uuid, cliInfo->argv[2]) != 0) {
        return invArg(cliInfo->argv[2]);
    }

    if (server->dirconSession[sessId].cliSockFd == 0) {
        return sessIsDown();
    }

    buildUuid128(&charUuid, &uuid);
    dirconSendReadCharacteristicMesg(server, &server->dirconSession[sessId], &charUuid);

    return OK;
}

static CmdStat cliCmdShow(CliInfo *cliInfo)
{
    Server *server = cliInfo->server;
    DirconSession *sess;
    Service *svc;
    Characteristic *chr;

    printf("Discovered Services:\n");
    TAILQ_FOREACH(svc, &server->svcList, svcListEnt) {
        printf("Service: uuid=%s (%s) {\n", fmtUuid128(&svc->uuid), fmtUuid128Name(&svc->uuid));
        TAILQ_FOREACH(chr, &svc->charList, charListEnt) {
            printf("    Characteristic: uuid=%s (%s)\n", fmtUuid128(&chr->uuid), fmtUuid128Name(&chr->uuid));
        }
        printf("}\n");
    }
    printf("\n");

    printf("APP Session:\n");
    sess = &server->dirconSession[app];
    if (sess->remCliAddr.sin_family != 0) {
        char remAddrBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sess->remCliAddr.sin_addr, remAddrBuf, sizeof (remAddrBuf));
        printf("App Address:            %s[%u]\n", remAddrBuf, ntohs(sess->remCliAddr.sin_port));
        printf("Tx App DIRCON Messages: %u\n", sess->txMesgCnt);
        printf("Rx App DIRCON Messages: %u\n", sess->rxMesgCnt);
    }
    printf("\n");

    printf("DEV Session:\n");
    sess = &server->dirconSession[dev];
    if (sess->remCliAddr.sin_family != 0) {
        char remAddrBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sess->remCliAddr.sin_addr, remAddrBuf, sizeof (remAddrBuf));
        printf("Device Address:         %s[%u]\n", remAddrBuf, ntohs(sess->remCliAddr.sin_port));
        printf("Tx Dev DIRCON Messages: %u\n", sess->txMesgCnt);
        printf("Rx Dev DIRCON Messages: %u\n", sess->rxMesgCnt);
    }
    printf("\n");

    if (server->mdnsAddr.sin_family != 0) {
        printf("Tx mDNS Messages:       %u\n", server->txMdnsMesgCnt);
        printf("Rx mDNS Messages:       %u\n", server->rxMdnsMesgCnt);
    }

    return OK;
}

// CLI command table
static CliCmd cliCmdTbl [] = {
    { "disc-char",      cliCmdDiscChar,             3,  3, "{app|dev} <uuid>",  true },
    { "disc-serv",      cliCmdDiscServ,             2,  2, NULL,                true },
    { "exit",           cliCmdExit,                 1,  1, NULL,                false },
    { "help",           cliCmdHelp,                 1,  1, NULL,                false },
    { "history",        cliCmdHistory,              1,  1, NULL,                false },
    { "mdns-query",     cliCmdMdnsQuery,            2,  2, "{app|dev} <qname>", false },
    { "read-char",      cliCmdReadChar,             3,  3, "{app|dev} <uuid>",  true },
    { "show",           cliCmdShow,                 1,  1, NULL,                false },
    { NULL,             NULL,                       0,  0, NULL,                false },
};

static CmdStat cliProcCmd(CliInfo *cliInfo)
{
    CmdStat s = OK;
    CliCmd *pCmd = &cliCmdTbl[0];

    for (int i = 0; pCmd->name != NULL; i++, pCmd++) {
        const char *name = pCmd->name;
        if (strncmp(cliInfo->argv[0], name, strlen(name)) == 0) {
            // Validate the argument count
            if ((cliInfo->argc >= cliCmdTbl[i].minArgCnt) &&
                (cliInfo->argc <= cliCmdTbl[i].maxArgCnt)) {
                // Call the command handler
                s = (*cliCmdTbl[i].handler)(cliInfo);
            } else {
                fprintf(stderr, "SYNTAX: %s %s\n", name, cliCmdTbl[i].args);
            }
            break;
        }
    }

    if (pCmd->name == NULL) {
        fprintf(stderr, "ERROR: Unsupported command. Use 'help' for the list of valid commands.\n");
    }

    return s;
}

static int cliParseCmdLine(CliInfo *cliInfo)
{
    char **argv = cliInfo->argv;
    char *cmdLine;
    int argc = 0;

    // Clone the entire command line string because we'll
    // modify it while we parse it...
    if ((cmdLine = strdup(cliInfo->cmdLine)) != NULL) {
        char *p = cmdLine;
        char *pArg = NULL;
        int c;

        // Break down the command line into single-word
        // arguments...
        while ((c = *p++) != '\0') {
            //printf("%s: [%d] = \'%c\'\n", __func__, argc, c);
            if (isspace(c)) {
                if (pArg != NULL) {
                    // This whitespace char completes the argument
                    *(p-1) = '\0';
                    if ((argv[argc] = strdup(pArg)) == NULL) {
                        fprintf(stderr, "Failed to duplicate: \"%s\" !\n", pArg);
                        return -1;
                    }
                    pArg = NULL;
                    if (++argc == MAX_ARGS)
                        break;
                } else {
                    // Skip white space
                    continue;
                }
            } else {
                if (pArg == NULL) {
                    // This char starts a new argument
                    pArg = p-1;
                } else {
                    // This char is part of the current argument
                }
            }
        }

        if (pArg != NULL) {
            // Complete the current (last) argument
            *(p-1) = '\0';
            if ((argv[argc] = strdup(pArg)) == NULL) {
                fprintf(stderr, "Failed to duplicate: \"%s\" !\n", pArg);
                return -1;
            }
            argc++;
        }

        //for (int n = 0; n < argc; n++) {
        //    printf("%s: [%d] = \"%s\"\n", __func__, n, argv[n]);
        //}

        for (int n = argc; n < MAX_ARGS; n++)
            argv[n] = NULL;

        cliInfo->argc = argc;

        free(cmdLine);
    } else {
    	fprintf(stderr, "Failed to clone \"%s\" !\n", cliInfo->cmdLine);
    	return -1;
    }

    return argc;
}

int cliCmdHandler(CliInfo *cliInfo)
{
    CmdStat s;

    if ((cliInfo->cmdLine != NULL) && (cliInfo->cmdLine[0] != '\0')) {
        // Parse the command line into tokens
        if (cliParseCmdLine(cliInfo) > 0) {
            // Add command to the history
            add_history(cliInfo->cmdLine);

            // Go process the command!
            if ((s = cliProcCmd(cliInfo)) == ERROR) {
                printf("Invalid command: %s\n", cliInfo->cmdLine);
            }

            // Free the argv[] strings
            for (int n = 0; n < cliInfo->argc; n++) {
            	if (cliInfo->argv[n] != NULL) {
					free(cliInfo->argv[n]);
					cliInfo->argv[n] = NULL;
            	}
            }
            cliInfo->argc = 0;
        }
    }

    return 0;
}

void cliReadChar(void)
{
    rl_callback_read_char();
}

// This handler is called by the readline API whenever
// a new input line is available for the CLI to process.
static void rlCbHandler(char *line)
{
    cliInfo.cmdLine = line;
    cliCmdHandler(&cliInfo);
    free(cliInfo.cmdLine);
    cliInfo.cmdLine = NULL;
}

int cliInit(Server *server)
{
    // Only once!
    if (cliInfo.server != NULL)
        return -1;

    // Install the readline callback handler
    rl_callback_handler_install("CLI> " , rlCbHandler);

    // Init readline's history
    using_history();

    // Ready to roll!
    server->stdinFd = fileno(stdin);
    cliInfo.server = server;

    return 0;
}

void cliPreExitCleanup(Server *server)
{
    rl_callback_handler_remove();
}

#else

// Stubs

void cliReadChar(void)
{
}

void cliPreExitCleanup(Server *server)
{
}

#endif  // CONFIG_CLI
