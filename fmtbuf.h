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
#include <stdio.h>
#include <sys/types.h>

typedef struct FmtBuf {
    char *buf;          // buffer where the string is stored
    size_t bufSize;     // size of the string buffer
    uint32_t offset;    // offset of the next available char in the string buffer
} FmtBuf;

__BEGIN_DECLS

// Allocate a FmtBuf object of the given size
extern FmtBuf *fmtBufNew(size_t bufSize);

// Free allocated FmtBuf object
extern void fmtBufFree(FmtBuf *fmtBuf);

// Initialize a static FmtBuf object
extern void fmtBufInit(FmtBuf *fmtBuf, char *buf, size_t bufSize);

// Append the formatted string to the FmtBuf object
extern int fmtBufAppend(FmtBuf *fmtBuf, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 3)));

// Append a formatted hex dump to the FmtBuf object
extern int fmtBufHexDump(FmtBuf *fmtBuf, const uint8_t *data, size_t len);

// Print the string in the FmtBuf object
extern void fmtBufPrint(const FmtBuf *fmtBuf, FILE *fp);

// Clear/reset the FmtBuf object
extern void fmtBufClear(FmtBuf *fmtBuf);

// Compare two FmtBuf objects
extern int fmtBufComp(const FmtBuf *fmtBuf1, const FmtBuf *fmtBuf2);

__END_DECLS
