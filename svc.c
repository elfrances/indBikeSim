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

#include <stdlib.h>
#include <sys/queue.h>

#include "char.h"
#include "mlog.h"
#include "svc.h"

Service *svcNew(const Uuid128 *uuid)
{
    Service *svc = calloc(1, sizeof (Service));

    if (svc != NULL) {
        svc->uuid = *uuid;
        TAILQ_INIT(&svc->charList);
    }

    return svc;
}

void svcFree(Service *svc)
{
    Characteristic *cha;

    while ((cha = TAILQ_FIRST(&svc->charList)) != NULL) {
        charFree(cha);
    }

    free(svc);
}

Characteristic *svcAddChar(Service *svc, const Uuid128 *uuid, uint8_t properties)
{
    Characteristic *ch = charNew(uuid);

    if (ch != NULL) {
        ch->properties = properties;
        TAILQ_INSERT_TAIL(&svc->charList, ch, charListEnt);
        //mlog(trace, "svc=\"%s\" char=\"%s\" prop=0x%02x", fmtUuid128Name(&svc->uuid), fmtUuid128Name(uuid), properties);
    }

    return ch;
}

Characteristic *svcFindChar(const Service *svc, const Uuid128 *uuid)
{
    Characteristic *ch;

    TAILQ_FOREACH(ch, &svc->charList, charListEnt) {
        if (uuid128Eq(&ch->uuid, uuid)) {
            // Found it!
            return ch;
        }
    }

    return ch;
}
