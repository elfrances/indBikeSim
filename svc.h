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

#include <sys/queue.h>

#include "char.h"
#include "uuid.h"

typedef struct Service {
    TAILQ_ENTRY(Service) svcListEnt;    // node in the svcList
    Uuid128 uuid;
    TAILQ_HEAD(CharList, Characteristic) charList;  // list of supported characteristics
} Service;

__BEGIN_DECLS

extern Service *svcNew(const Uuid128 *uuid);
extern void svcFree(Service *svc);

extern Characteristic *svcAddChar(Service *svc, const Uuid128 *uuid, uint8_t properties);
extern Characteristic *svcFindChar(const Service *svc, const Uuid128 *uuid);

__END_DECLS
