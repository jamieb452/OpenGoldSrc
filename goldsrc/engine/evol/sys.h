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

/**
*	@file
*
*	non-portable functions
*/

#pragma once

#include <SDL2/SDL.h>

const int MAX_EXT_DLLS = 50;

extern qboolean gfBackground;

extern int giActive;
extern int giStateInfo;
extern int giSubState;

extern qboolean gfExtendedError;
extern char gszDisconnectReason[ 256 ];
extern char gszExtendedDisconnectReason[ 256 ];

extern qboolean g_bIsDedicatedServer;

extern SDL_Window* pmainwindow;

extern qboolean gHasMMXTechnology;

typedef struct functiontable_s
{
	uint32 pFunction;
	char* pFunctionName;
} functiontable_t;

typedef struct extensiondll_s
{
	CSysModule* pDLLHandle;
	functiontable_t* functionTable;
	int functionCount;
} extensiondll_t;

void Sys_Init();
void Sys_Shutdown();

void Sys_ShutdownFloatTime();

//
// memory protection
//
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length);

/// Send text to the console
void Sys_Printf(const char *fmt, ...);

void Sys_Warning(const char *pszWarning, ...);

/// An error will cause the entire program to exit
void Sys_Error(const char *error, ...);

double Sys_FloatTime();

/// Called to yield for a little bit so as
/// not to hog cpu when paused or debugging
void Sys_Sleep(int msec);

void Sys_Quit();

void Sys_CheckOSVersion();

qboolean Sys_IsWin95();
qboolean Sys_IsWin98();

bool Sys_InitGame( char *lpOrgCmdLine, char *pBaseDir, void *pwnd, bool bIsDedicated );
void Sys_ShutdownGame();

///////////////////////////////
// OLD

//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
int Sys_FileOpenRead (char *path, int *hndl);

int Sys_FileOpenWrite (char *path);
void Sys_FileClose (int handle);
void Sys_FileSeek (int handle, int position);
int Sys_FileRead (int handle, void *dest, int count);
int Sys_FileWrite (int handle, void *data, int count);
int	Sys_FileTime (char *path);

void Sys_mkdir (char *path);

//
// system IO
//
void Sys_DebugLog(char *file, char *fmt, ...);

char *Sys_ConsoleInput ();

void Sys_SendKeyEvents ();
// Perform Key_Event () callbacks until the input que is empty

void Sys_LowFPPrecision ();
void Sys_HighFPPrecision ();
void Sys_SetFPCW ();