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
    "exit\n"
    "    Exit the tool.\n"
    "\n"
    "help\n"
    "    Print this help.\n"
    "\n"
    "history\n"
    "    Print the command history.\n"
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

//static int invArg(const char *arg)
//{
//    fprintf(stderr, "ERROR: invalid argument \"%s\"\n", arg);
//    return ERROR;
//}

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

static CmdStat cliCmdShow(CliInfo *cliInfo)
{
    return OK;
}

// CLI command table
static CliCmd cliCmdTbl [] = {
    { "exit",           cliCmdExit,                 1,  1, NULL,                false },
    { "help",           cliCmdHelp,                 1,  1, NULL,                false },
    { "history",        cliCmdHistory,              1,  1, NULL,                false },
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
