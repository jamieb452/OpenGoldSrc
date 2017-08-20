/*
Copyright (C) 1996-2001 Id Software, Inc.

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

#include "public/keydefs.h"

const size_t MAX_CHAT_BUFFER = 120;

typedef enum
{
	key_game = 0,
	key_message,
	key_menu
} keydest_t;

extern keydest_t	key_dest;
extern char *keybindings[256];
extern	int		key_repeats[256];
//extern	int		key_count;			// incremented every key event
//extern	int		key_lastpress;

extern int toggleconsole_key;

extern char chat_buffer[MAX_CHAT_BUFFER];
extern	int chat_bufferlen;
//extern	qboolean	chat_team;
extern char message_type[ 32 ];

SDL_Keycode GetSDLKeycodeFromEngineKey( int iKey );
int GetEngineKeyFromSDLScancode( SDL_Scancode code );

void Key_Init ();
void Key_Shutdown();

void Key_Event (int key, qboolean down);

bool CheckForCommand();
void CompleteCommand();

int Key_CountBindings();
void Key_WriteBindings (FileHandle_t f);

int Key_StringToKeynum( const char* str );
const char* Key_KeynumToString( int keynum );

void Key_SetBinding (int keynum, const char *binding);
const char* Key_NameForBinding( const char* pBinding );
void Key_EventStub( int key, qboolean down );
void Key_ClearStates ();

const char* Key_BindingForKey( int keynum );