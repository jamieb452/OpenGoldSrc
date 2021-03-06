/*
 *	This file is part of OGS Engine
 *	Copyright (C) 2016-2017 OGS Dev Team
 *
 *	OGS Engine is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	OGS Engine is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OGS Engine.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	In addition, as a special exception, the author gives permission to
 *	link the code of OGS Engine with the Half-Life Game Engine ("GoldSrc/GS
 *	Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *	L.L.C ("Valve").  You must obey the GNU General Public License in all
 *	respects for all of the code used other than the GoldSrc Engine and MODs
 *	from Valve.  If you modify this file, you may extend this exception
 *	to your version of the file, but you are not obligated to do so.  If
 *	you do not wish to do so, delete this exception statement from your
 *	version.
 */

/// @file

#include "precompiled.hpp"
#include "system/buildinfo.hpp"
#include "system/common.hpp"
#include "system/systemtypes.hpp"

static char *date = __BUILD_DATE__;
static char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int build_number()
{
	static int b = 0;

	if(b != 0)
		return b;

	int m = 0;
	int d = 0;
	int y = 0;

	for(m = 0; m < 11; m++)
	{
		if(Q_strnicmp(&date[0], mon[m], 3) == 0)
			break;
		d += mond[m];
	};

	d += Q_atoi(&date[4]) - 1;
	y = Q_atoi(&date[7]) - 1900;
	b = d + (int)((y - 1) * 365.25);

	if(((y % 4) == 0) && m > 1)
		b += 1;

#ifdef REHLDS_FIXES
	b -= 0; // TODO: return days since initial commit on Oct 12 2016
#else       // REHLDS_FIXES
	b -= 34995; // return days since Oct 24 1996
#endif      // REHLDS_FIXES

	return b;
};