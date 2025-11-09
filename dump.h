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

#include <sys/time.h>

#include "dircon.h"
#include "server.h"

__BEGIN_DECLS

extern void dirconDumpMesg(const struct timeval *pTs, Server *server, const DirconSession *sess,
                           MesgDir dir, MesgType mesgType, const DirconMesg *mesg);

extern void hexDump(const void *pBuf, int bufLen);

__END_DECLS
