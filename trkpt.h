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

#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <time.h>

#include "config.h"

#ifdef CONFIG_FIT_ACTIVITY_FILE

// Activity Track Point
typedef struct TrkPt {
    TAILQ_ENTRY(TrkPt) tqEntry;   // node in the trkPtList

    int index;          // TrkPt index (0..N-1)

    // Timestamp from FIT file
    time_t timestamp;   // in seconds since the Epoch

    // Activity metrics from FIT file
    uint32_t cadence;   // cadence (in RPM)
    uint32_t heartRate; // heart rate (in BPM)
    uint32_t power;     // power (in Watts)
    double speed;       // speed (in m/s)
} TrkPt;

__BEGIN_DECLS

extern TrkPt *trkPtNew(int index);
extern void trkPtFree(TrkPt *tp);

__END_DECLS

#endif  // CONFIG_FIT_ACTIVITY_FILE

