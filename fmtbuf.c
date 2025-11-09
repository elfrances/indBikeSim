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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmtbuf.h"

FmtBuf *fmtBufNew(size_t bufSize)
{
    FmtBuf *fmtBuf = calloc(1, sizeof (FmtBuf));

    if (fmtBuf != NULL) {
        if ((fmtBuf->buf = calloc(1, bufSize)) != NULL) {
            fmtBuf->bufSize = bufSize;
        } else {
            free(fmtBuf);
            fmtBuf = NULL;
        }
    }

    return fmtBuf;
}

void fmtBufFree(FmtBuf *fmtBuf)
{
    free(fmtBuf->buf);
    free(fmtBuf);
}

void fmtBufInit(FmtBuf *fmtBuf, char *buf, size_t bufSize)
{
    fmtBuf->buf = buf;
    fmtBuf->bufSize = bufSize;
    fmtBufClear(fmtBuf);
}

int fmtBufAppend(FmtBuf *fmtBuf, const char *fmt, ...)
{
    va_list ap;

    if (fmtBuf->offset >= fmtBuf->bufSize)
        return -1;

    va_start(ap, fmt);
    fmtBuf->offset += vsnprintf((fmtBuf->buf + fmtBuf->offset), (fmtBuf->bufSize - fmtBuf->offset), fmt, ap);
    va_end(ap);

    return 0;
}

int fmtBufHexDump(FmtBuf *fmtBuf, const uint8_t *data, size_t len)
{
    for (int i = 0; i < len; i++) {
        if (fmtBufAppend(fmtBuf, "%02x", data[i]) != 0)
            return -1;
    }

    return 0;
}

void fmtBufPrint(const FmtBuf *fmtBuf, FILE *fp)
{
    fprintf(fp, "%s", fmtBuf->buf);
}

void fmtBufClear(FmtBuf *fmtBuf)
{
    fmtBuf->buf[0] = '\0';
    fmtBuf->offset = 0;
}

int fmtBufComp(const FmtBuf *fmtBuf1, const FmtBuf *fmtBuf2)
{
    int diff = (fmtBuf1->offset - fmtBuf2->offset);
    if (diff == 0)
        diff = strncmp(fmtBuf1->buf, fmtBuf2->buf, fmtBuf1->offset);
    return diff;
}




