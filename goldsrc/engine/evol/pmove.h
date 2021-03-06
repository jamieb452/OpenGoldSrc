/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#include "cvar.h"
#include "pm_defs.h"
#include "pm_movevars.h"

extern cvar_t pm_showclip;

extern movevars_t movevars;

extern vec3_t player_mins[ 4 ];
extern vec3_t player_maxs[ 4 ];

extern playermove_t *pmove;

//extern int onground;
//extern int waterlevel;
//extern int watertype;

void PM_Init( playermove_t* ppm );

//int PM_HullPointContents (hull_t *hull, int num, vec3_t p);

//int PM_PointContents (vec3_t point);
//qboolean PM_TestPlayerPosition (vec3_t point);
//pmtrace_t PM_PlayerMove (vec3_t start, vec3_t stop);

qboolean PM_AddToTouched(pmtrace_t tr, vec_t *impactvelocity);
void PM_StuckTouch(int hitent, pmtrace_t *ptraceresult);