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
*	defs common to client and server
*/

#pragma once

// For backwards compatibility only, SDK headers use it - Solokiller
#define QUAKEDEF_H

//#define	GLTEST			// experimental stuff

#define	QUAKE_GAME			// as opposed to utilities

#define	VERSION				1.09 // 2.40 for qw
#define	GLQUAKE_VERSION		1.00
#define	D3DQUAKE_VERSION	0.01
#define	WINQUAKE_VERSION	0.996
#define	LINUX_VERSION		1.30 // 0.98 for qw
#define	X11_VERSION			1.10

//define	PARANOID			// speed sapping error checking

#define	GAMENAME	"valve"		// directory to look in by default

#ifdef _WIN32
#pragma warning( disable : 4244 4127 4201 4214 4514 4305 4115 4018)
#endif

#include <cmath>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

void	VID_LockBuffer ();
void	VID_UnlockBuffer ();

#else

#define	VID_LockBuffer()
#define	VID_UnlockBuffer()

#endif

#if defined __i386__ // && !defined __sun__
//#if (defined(_M_IX86) || defined(__i386__)) && !defined(id386)
#define id386	1
#else
#define id386	0
#endif

#ifdef SERVERONLY		// no asm in dedicated server
#undef id386
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY			0x550000
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_MSGLEN		8000		//1450 // max length of a reliable message
#define	MAX_DATAGRAM	1024		//1450 // max length of unreliable message

//
// per-level limits
//
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		512			// these are sent over the net as bytes
#define	MAX_SOUNDS		512			// so they cannot be blindly increased
#define	MAX_STYLESTRING	64

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
#define	STAT_HEALTH			0
//#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
//#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define	STAT_ITEMS			15
//define	STAT_VIEWHEIGHT		16

// stock defines

#define	MAX_SCOREBOARD		16		// max numbers of players

#define	SOUND_CHANNELS		8

//
// print flags
//
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages

//TODO: tidy these includes - Solokiller
//#include "tier0/platform.h"
//#include "commonmacros.h"
#include "filesystem.h"
#include "common/mathlib.h"
#include "common/const.h"
#include "cvar.h"
#include "cmd.h"
#include "mem.h"
#include "zone.h"
#include "common.h"
#include "net.h"
#include "mathlib.h" // common/mathlib
#include "vid.h"
#include "sys.h"

#include "common/entity_state.h"

#include "wad.h"
#include "decals.h"
#include "draw.h"
#include "screen.h"
#include "protocol.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "client.h"
#include "progs.h"
#include "server.h"

#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
//#include "d_iface.h" // TODO: fix
#endif

#include "input.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"
#include "pmove.h"

#include "world.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

//=============================================================================

/**
*	the host system specifies the base of the directory tree, the
*	command line parms passed to the program, and the amount of memory
*	available for the program to use
*/
typedef struct
{
	/*const*/ char	*basedir;
	/*const*/ char	*cachedir;		// for development over ISDN lines
	int		argc;
	/*const*/ char	**argv;
	void	*membase;
	int		memsize;
} quakeparms_t;


//=============================================================================

//#define MAX_NUM_ARGVS 50

extern qboolean noclip_anglehack;

//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

//extern cvar_t password;

extern	qboolean	host_initialized;		///< true if into command execution
extern	double		host_frametime;
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	///< incremented every frame, never reset
extern	double		realtime;			///< not bounded in any way, changed at
										/// start of every frame, never reset

void Host_ClearMemory ();
void Host_ServerFrame ();
void Host_InitCommands ();

qboolean Host_Init (quakeparms_t *parms);
void Host_Shutdown();

void Host_Error (const char *error, ...);

void Host_EndGame (const char *message, ...);

void Host_Frame (float time);

qboolean Host_SimulationTime(float time);

void Host_Quit_f ();
void Host_ClientCommands (const char *fmt, ...);
void Host_ShutdownServer (qboolean crash);

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss
extern int			current_skill;		// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)

extern qboolean		isDedicated;

extern int			minimum_memory;