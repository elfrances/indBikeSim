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

#include <sys/queue.h>

#include "uuid.h"

typedef struct Characteristic {
    TAILQ_ENTRY(Characteristic) charListEnt;    // node in the charList
    Uuid128 uuid;
    uint16_t uuid16;
    uint8_t properties;

    uint16_t connHandle;
    uint16_t valHandle;
    uint16_t dscHandle;
} Characteristic;

__BEGIN_DECLS

extern Characteristic *charNew(const Uuid128 *uuid);
extern void charFree(Characteristic *cha);

__END_DECLS
