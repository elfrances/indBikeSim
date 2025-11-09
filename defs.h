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

#include <stdbool.h>
#include <sys/time.h>

// Pull in the app's configuration options
#include "config.h"

#ifdef ESP_PLATFORM
// Pull in the required ESP IDF header files
#include "esp32.h"
#endif

// Convert a bit number 'n' to its corresponding 32-bit mask
#define bitMask(n)  (0x01U << (n))

// Test whether bit number 'n' is set in the given 32-bit mask
#define bitTest(n, mask)    (!!((mask) & bitMask(n)))

__BEGIN_DECLS

// Compare two timeval values. Return 1 when 'x' is later than
// 'y', -1 when 'x' is earlier than 'y', and 0 when the values
// are equal.
static __inline__ int tvCmp(const struct timeval *x, const struct timeval *y)
{
    if (x->tv_sec > y->tv_sec) {
        return 1;
    } else if (x->tv_sec < y->tv_sec) {
        return -1;
    } else if (x->tv_usec > y->tv_usec) {
        return 1;
    } else if (x->tv_usec < y->tv_usec) {
        return -1;
    }

    return 0;
}

// Subtract two timeval values. It is assumed that timeval 'x'
// is equal or later than timeval 'y'.
static __inline__ void tvSub(struct timeval *result, const struct timeval *x, const struct timeval *y)
{
    if (x->tv_usec >= y->tv_usec) {
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
    } else {
        result->tv_sec = x->tv_sec - 1 - y->tv_sec;
        result->tv_usec = x->tv_usec + 1000000L - y->tv_usec;
    }
}

__END_DECLS

