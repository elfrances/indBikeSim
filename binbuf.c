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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binbuf.h"

BinBuf *binBufNew(size_t bufSize, ByteOrder byteOrder)
{
    BinBuf *binBuf = calloc(1, sizeof (BinBuf));

    if (binBuf != NULL) {
        if ((binBuf->buf = calloc(1, bufSize)) != NULL) {
            binBuf->bufSize = bufSize;
            binBuf->byteOrder = byteOrder;
        } else {
            free(binBuf);
            binBuf = NULL;
        }
    }

    return binBuf;
}

void binBufFree(BinBuf *binBuf)
{
    free(binBuf->buf);
    free(binBuf);
}

void binBufInit(BinBuf *binBuf, uint8_t *buf, size_t bufSize, ByteOrder byteOrder)
{
    binBuf->buf = buf;
    binBuf->bufSize = bufSize;
    binBuf->byteOrder = byteOrder;
    binBufClear(binBuf);
}

void binBufClear(BinBuf *binBuf)
{
    binBuf->offset = 0;
}

uint8_t binBufGetUINT8(BinBuf *binBuf)
{
    uint8_t value;
    int offset = binBuf->offset;
    assert((offset + 1) <= binBuf->bufSize);
    value = binBuf->buf[offset];
    binBuf->offset += 1;
    return value;
}

uint16_t binBufGetUINT16(BinBuf *binBuf)
{
    uint16_t value;
    int offset = binBuf->offset;
    assert((offset + 2) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        value = ((uint16_t) binBuf->buf[offset+1] << 8) | (uint16_t) binBuf->buf[offset];
    } else {
        value = ((uint16_t) binBuf->buf[offset] << 8) | (uint16_t) binBuf->buf[offset+1];
    }
    binBuf->offset += 2;
    return value;
}

uint32_t binBufGetUINT24(BinBuf *binBuf)
{
    uint32_t value;
    int offset = binBuf->offset;
    assert((offset + 3) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        value = ((uint32_t) binBuf->buf[offset+2] << 16) | ((uint32_t) binBuf->buf[offset+1] << 8) | (uint32_t) binBuf->buf[offset];
    } else {
        value = ((uint32_t) binBuf->buf[offset] << 16) | ((uint32_t) binBuf->buf[offset+1] << 8) | (uint32_t) binBuf->buf[offset+2];
    }
    binBuf->offset += 3;
    return value;
}

uint32_t binBufGetUINT32(BinBuf *binBuf)
{
    uint32_t value;
    int offset = binBuf->offset;
    assert((offset + 4) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        value = ((uint32_t) binBuf->buf[offset+3] << 24) | ((uint32_t) binBuf->buf[offset+2] << 16) | ((uint32_t) binBuf->buf[offset+1] << 8) | (uint32_t) binBuf->buf[offset];
    } else {
        value = ((uint32_t) binBuf->buf[offset] << 24) | ((uint32_t) binBuf->buf[offset+1] << 16) | ((uint32_t) binBuf->buf[offset+2] << 8) | (uint32_t) binBuf->buf[offset+3];
    }
    binBuf->offset += 4;
    return value;
}

uint64_t binBufGetUINT64(BinBuf *binBuf)
{
    uint64_t value;
    int offset = binBuf->offset;
    assert((offset + 8) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        value = ((uint64_t) binBuf->buf[offset+7] << 56) | ((uint64_t) binBuf->buf[offset+6] << 48) | ((uint64_t) binBuf->buf[offset+5] << 40) | ((uint64_t) binBuf->buf[offset+4] << 32) |
                ((uint64_t) binBuf->buf[offset+3] << 24) | ((uint64_t) binBuf->buf[offset+2] << 16) | ((uint64_t) binBuf->buf[offset+1] << 8) | (uint64_t) binBuf->buf[offset];
    } else {
        value = ((uint64_t) binBuf->buf[offset] << 56) | ((uint64_t) binBuf->buf[offset+1] << 48) | ((uint64_t) binBuf->buf[offset+2] << 40) | ((uint64_t) binBuf->buf[offset+3] << 32) |
                ((uint64_t) binBuf->buf[offset+4] << 24) | ((uint64_t) binBuf->buf[offset+5] << 16) | ((uint64_t) binBuf->buf[offset+6] << 8) | (uint64_t) binBuf->buf[offset+7];
    }
    binBuf->offset += 8;
    return value;
}

uint8_t *binBufGetHex(BinBuf *binBuf, void *buf, size_t numBytes)
{
    int offset = binBuf->offset;
    assert((offset + numBytes) <= binBuf->bufSize);
    memcpy(buf, &binBuf->buf[offset], numBytes);
    binBuf->offset += numBytes;
    return buf;
}

void binBufPutUINT8(BinBuf *binBuf, uint8_t value)
{
    int offset = binBuf->offset;
    assert((offset + 1) <= binBuf->bufSize);
    binBuf->buf[offset] = value;
    binBuf->offset += 1;
}

void binBufPutUINT16(BinBuf *binBuf, uint16_t value)
{
    int offset = binBuf->offset;
    assert((offset + 2) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        binBuf->buf[offset] = (value & 0xff);
        binBuf->buf[offset+1] = (value >> 8) & 0xff;
    } else {
        binBuf->buf[offset] = (value >> 8) & 0xff;
        binBuf->buf[offset+1] = (value & 0xff);
    }
    binBuf->offset += 2;
}

void binBufPutUINT24(BinBuf *binBuf, uint32_t value)
{
    int offset = binBuf->offset;
    assert((offset + 3) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        binBuf->buf[offset] = (value & 0xff);
        binBuf->buf[offset+1] = (value >> 8) & 0xff;
        binBuf->buf[offset+2] = (value >> 16) & 0xff;
    } else {
        binBuf->buf[offset] = (value >> 16) & 0xff;
        binBuf->buf[offset+1] = (value >> 8) & 0xff;
        binBuf->buf[offset+2] = (value & 0xff);
    }
    binBuf->offset += 3;
}

void binBufPutUINT32(BinBuf *binBuf, uint32_t value)
{
    int offset = binBuf->offset;
    assert((offset + 4) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        binBuf->buf[offset] = (value & 0xff);
        binBuf->buf[offset+1] = (value >> 8) & 0xff;
        binBuf->buf[offset+2] = (value >> 16) & 0xff;
        binBuf->buf[offset+3] = (value >> 24) & 0xff;
    } else {
        binBuf->buf[offset] = (value >> 24) & 0xff;
        binBuf->buf[offset+1] = (value >> 16) & 0xff;
        binBuf->buf[offset+2] = (value >> 8) & 0xff;
        binBuf->buf[offset+3] = (value & 0xff);
    }
    binBuf->offset += 4;
}

void binBufPutUINT64(BinBuf *binBuf, uint64_t value)
{
    int offset = binBuf->offset;
    assert((offset + 8) <= binBuf->bufSize);
    if (binBuf->byteOrder == littleEndian) {
        binBuf->buf[offset] = (value & 0xff);
        binBuf->buf[offset+1] = (value >> 8) & 0xff;
        binBuf->buf[offset+2] = (value >> 16) & 0xff;
        binBuf->buf[offset+3] = (value >> 24) & 0xff;
        binBuf->buf[offset+4] = (value >> 32) & 0xff;
        binBuf->buf[offset+5] = (value >> 40) & 0xff;
        binBuf->buf[offset+6] = (value >> 48) & 0xff;
        binBuf->buf[offset+7] = (value >> 56) & 0xff;
    } else {
        binBuf->buf[offset] = (value >> 56) & 0xff;
        binBuf->buf[offset+1] = (value >> 48) & 0xff;
        binBuf->buf[offset+2] = (value >> 40) & 0xff;
        binBuf->buf[offset+3] = (value >> 32) & 0xff;
        binBuf->buf[offset+4] = (value >> 24) & 0xff;
        binBuf->buf[offset+5] = (value >> 16) & 0xff;
        binBuf->buf[offset+6] = (value >> 8) & 0xff;
        binBuf->buf[offset+7] = (value & 0xff);
    }
    binBuf->offset += 8;
}

void binBufPutHex(BinBuf *binBuf, const void *buf, size_t numBytes)
{
    int offset = binBuf->offset;
    assert((offset + numBytes) <= binBuf->bufSize);
    memcpy(&binBuf->buf[offset], buf, numBytes);
    binBuf->offset += numBytes;
}





