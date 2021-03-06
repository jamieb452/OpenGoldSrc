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

#include "common/con_nprint.h"

//
// console
//

// qw
/*
#define CON_TEXTSIZE 16384 // 32768

typedef struct
{
	char	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line
} console_t;

extern console_t con_main;
extern console_t con_chat;
extern console_t *con; // point to either con_main or con_chat

extern int con_ormask;
*/

extern cvar_t con_fastmode;
extern cvar_t con_notifytime;
extern cvar_t con_color;
extern cvar_t con_shifttoggleconsole;
extern cvar_t con_mono;

//extern int con_totallines;
//extern int con_backscroll;

extern qboolean con_forcedup; // because no entities to refresh
extern qboolean con_initialized;

//extern byte *con_chars;
//extern int con_notifylines; // scan lines to clear for notify lines

void Con_ToggleConsole_f ();

void Con_Init ();
void Con_Shutdown();

void Con_Printf (const char *fmt, ...);
void Con_DPrintf (const char *fmt, ...);
void Con_SafePrintf (const char *fmt, ...);

void Con_CheckResize ();

void Con_DrawNotify ();
void Con_ClearNotify ();

/*qboolean*/ int Con_IsVisible();

////////////////

/*
void Con_DrawCharacter (int cx, int line, int num);

//void Con_DrawConsole (int lines); // qw
void Con_DrawConsole (int lines, qboolean drawinput);
void Con_Print (const char *txt);
void Con_Clear_f ();

void Con_NotifyBox (const char *text); // during startup for sound / cd warnings
*/