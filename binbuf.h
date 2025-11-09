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
#include <sys/types.h>

typedef enum ByteOrder {
    undefEndian = 0,
    littleEndian = 1,
    bigEndian = 2
} ByteOrder;

typedef struct BinBuf {
    uint8_t *buf;
    size_t bufSize;
    uint32_t offset;
    ByteOrder byteOrder;
} BinBuf;

__BEGIN_DECLS

extern BinBuf *binBufNew(size_t bufSize, ByteOrder byteOrder);

extern void binBufFree(BinBuf *binBuf);

extern void binBufInit(BinBuf *binBuf, uint8_t *buf, size_t bufSize, ByteOrder byteOrder);

extern void binBufClear(BinBuf *binBuf);

extern uint8_t binBufGetUINT8(BinBuf *binBuf);
extern uint16_t binBufGetUINT16(BinBuf *binBuf);
extern uint32_t binBufGetUINT24(BinBuf *binBuf);
extern uint32_t binBufGetUINT32(BinBuf *binBuf);
extern uint64_t binBufGetUINT64(BinBuf *binBuf);
extern uint8_t *binBufGetHex(BinBuf *binBuf, void *buf, size_t numBytes);

extern void binBufPutUINT8(BinBuf *binBuf, uint8_t value);
extern void binBufPutUINT16(BinBuf *binBuf, uint16_t value);
extern void binBufPutUINT24(BinBuf *binBuf, uint32_t value);
extern void binBufPutUINT32(BinBuf *binBuf, uint32_t value);
extern void binBufPutUINT64(BinBuf *binBuf, uint64_t value);
extern void binBufPutHex(BinBuf *binBuf, const void *buf, size_t numBytes);

__END_DECLS
