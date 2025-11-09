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

#include "trkpt.h"

#ifdef CONFIG_FIT_ACTIVITY_FILE

TrkPt *trkPtNew(int index)
{
    TrkPt *tp = calloc(1, sizeof (TrkPt));

    if (tp != NULL)
        tp->index = index;

    return tp;
}

void trkPtFree(TrkPt *tp)
{
    free(tp);
}

#endif  // CONFIG_FIT_ACTIVITY_FILE
