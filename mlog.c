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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "defs.h"
#include "fmtbuf.h"
#include "mlog.h"

#ifdef CONFIG_MSGLOG

static const char *logLevelName[] = {
    [none] = "",
    [info] = "INFO",
    [trace] = "\033[0;32mTRACE\033[0m",     // green
    [debug] = "\033[0;36mDEBUG\033[0m",     // cyan
    [warning] = "\033[0;33mWARNING\033[0m", // yellow
    [error] = "\033[0;31mERROR\033[0m",     // red
    [fatal] = "\033[0;31mFATAL\033[0m",     // red
};

static LogDest msgLogDest = console;
static LogLevel msgLogLevel = info;
static FILE *logFile = NULL;

#ifdef ESP_PLATFORM
static const char *fmtTimestamp(void)
{
    struct timeval now;
    unsigned days, hr, min, sec;
    static char tsBuf[24];  // DDD HH:MM:SS.xxxxxx

    gettimeofday(&now, NULL);
    sec = now.tv_sec;
    days = sec / 86400;
    sec = sec % 86400;
    hr = sec / 3600;
    sec = sec % 3600;
    min = sec / 60;
    sec = sec % 60;
    snprintf(tsBuf, sizeof (tsBuf), "%03u %02u:%02u:%02u.%06u", days, hr, min, sec, (unsigned) now.tv_usec);

    return tsBuf;
}
#else
static const char *fmtTimestamp(void)
{
    struct timeval now;
    struct tm brkDwnTime;
    static char tsBuf[32];  // YYYY-MM-DDTHH:MM:SS.xxxxxx
    size_t bufLen = sizeof (tsBuf);
    int n;

    gettimeofday(&now, NULL);
    n = strftime(tsBuf, bufLen, "%Y-%m-%dT%H:%M:%S", gmtime_r(&now.tv_sec, &brkDwnTime));
    snprintf((tsBuf + n), (bufLen - n), ".%06u", (unsigned) now.tv_usec);

    return tsBuf;
}
#endif  // ESP_PLATFORM

void msgLog(LogLevel logLevel, const char *funcName, int lineNum, int errNo, const char *fmt, ...)
{
    // Everything at or above "warning" is
    // always printed...
    if ((logLevel <= msgLogLevel) || (logLevel >= warning)) {
        static char msgLogBuf[1024];
        FmtBuf fmtBuf;
        va_list ap;

        fmtBufInit(&fmtBuf, msgLogBuf, sizeof (msgLogBuf));

        fmtBufAppend(&fmtBuf, "%s %s ", fmtTimestamp(), logLevelName[logLevel]);

        if (logLevel >= trace) {
#ifdef ESP_PLATFORM
            fmtBufAppend(&fmtBuf, "%s:", pcTaskGetName(NULL));
#endif
            fmtBufAppend(&fmtBuf, "%s:%d: ", funcName, lineNum);
        }
        va_start(ap, fmt);
        fmtBuf.offset += vsnprintf((fmtBuf.buf + fmtBuf.offset), (fmtBuf.bufSize - fmtBuf.offset), fmt, ap);
        va_end(ap);
        if ((logLevel >= warning) && (errno != 0)) {
            fmtBufAppend(&fmtBuf, " errno=%d (%s)", errNo, strerror(errNo));
        }
        fmtBufAppend(&fmtBuf, "\n");

        if ((msgLogDest == both) || (msgLogDest == console)) {
            fmtBufPrint(&fmtBuf, stdout);
        }
        if ((msgLogDest == both) || (msgLogDest == file)) {
            fmtBufPrint(&fmtBuf, logFile);
        }

        if (logLevel == fatal) {
#ifdef ESP_PLATFORM
            while (true) {
                usleep(100000);
            }
#else
            assert(false);
#endif
        }
    }
}

LogDest msgLogSetDest(LogDest logDest)
{
    LogDest prevLogDest = msgLogDest;
    if ((msgLogDest = logDest) != prevLogDest) {
        if ((msgLogDest == console) && (logFile != NULL)) {
            fclose(logFile);
            logFile = NULL;
        } else if (prevLogDest == console) {
            time_t now = time(NULL);
            struct tm brkDwnTime;
            char logFileName[128];
            // Log file name: "YYYY-MM-DDTHH:MM:SS.txt"
            strftime(logFileName, sizeof (logFileName), "%Y-%m-%dT%H:%M:%S.txt", gmtime_r(&now, &brkDwnTime));
            logFile = fopen(logFileName, "w");
        }
    }
    return prevLogDest;
}

LogLevel msgLogSetLevel(LogLevel logLevel)
{
    LogLevel prevLogLevel = msgLogLevel;
    msgLogLevel = logLevel;
    return prevLogLevel;
}
#else
// Stub functions
void msgLog(LogLevel logLevel, const char *funcName, int lineNum, const char *fmt, ...)
{
}

LogDest msgLogSetDest(LogDest logDest)
{
    return undef;
}

LogLevel msgLogSetLevel(LogLevel logLevel)
{
    return none;
}
#endif


