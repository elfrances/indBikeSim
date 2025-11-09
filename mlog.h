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

#include <sys/cdefs.h>
#include <errno.h>

typedef enum LogDest {
    undef = 0,
    both = 1,
    console = 2,
    file = 3,
} LogDest;

typedef enum LogLevel {
    none = 0,
    info,
    trace,
    debug,
    warning,
    error,
    fatal
} LogLevel;

// This macro is used to pick up the file and line number from
// where msgLog() is called.
#define mlog(lvl, fmt, args...) msgLog((lvl), __func__, __LINE__, errno, (fmt), ##args)

__BEGIN_DECLS

extern void msgLog(LogLevel logLevel, const char *funcName, int lineNum, int errNo, const char *fmt, ...)  __attribute__ ((__format__ (__printf__, 5, 6)));

extern LogDest msgLogSetDest(LogDest logDest);
extern LogLevel msgLogSetLevel(LogLevel logLevel);

__END_DECLS
