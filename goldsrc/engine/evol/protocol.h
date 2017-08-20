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
*	communications protocols
*/

#pragma once

typedef struct entity_state_s entity_state_t;

#define	PROTOCOL_VERSION 48

//=========================================

#define	PORT_CLIENT	27005
#define	PORT_MASTER	27010
#define	PORT_SERVER	27015

//=========================================

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define	U_MOREBITS	(1<<0)
#define	U_ORIGIN1	(1<<1)
#define	U_ORIGIN2	(1<<2)
#define	U_ORIGIN3	(1<<3)
#define	U_ANGLE2	(1<<4)
#define	U_NOLERP	(1<<5)		// don't interpolate movement
#define	U_FRAME		(1<<6)
#define U_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	U_ANGLE1	(1<<8)
#define	U_ANGLE3	(1<<9)
#define	U_MODEL		(1<<10)
#define	U_COLORMAP	(1<<11)
#define	U_SKIN		(1<<12)
#define	U_EFFECTS	(1<<13)
#define	U_LONGENTITY	(1<<14)


#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)

// a sound with no channel is a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_LOOPING		(1<<2)		// a long


// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22


// game types sent by serverinfo
// these determine which intermission screen plays
#define	GAME_COOP			0
#define	GAME_DEATHMATCH		1

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
typedef enum svc_commands_e
{
	svc_bad = 0,
	
	svc_nop,
	
	svc_disconnect,
	svc_event,
	svc_version,			// [long] server version
	svc_setview,			// [short] entity number
	svc_sound,				// <see code>
	svc_time,				// [float] server time
	svc_print,				// ([byte] id) [string] null terminated string
	svc_stufftext,			// [string] stuffed into client's console buffer
							// the string should be \n terminated
	svc_setangle,			// [angle3] set the view angle to this absolute value
	svc_serverinfo,			// serverinfo
							// [string] signon string
							// [string]..[0]model cache
							// [string]...[0]sounds cache
	svc_lightstyle,			// [byte] [string]
	svc_updateuserinfo,		// [byte] slot [long] uid
							// [string] userinfo
	svc_deltadescription,
	svc_clientdata,			// <shortbits + data>
	svc_stopsound,			// <see code>
	svc_pings,
	svc_particle,			// [vec3] <variable>
	svc_damage,
	svc_spawnstatic,
	svc_event_reliable,
	svc_spawnbaseline,
	svc_temp_entity,		// variable
	svc_setpause,			// [byte] on / off
	svc_signonnum,			// [byte]  used for the signon sequence
	svc_centerprint,		// [string] to put in center of the screen
	svc_killedmonster,
	svc_foundsecret,
	svc_spawnstaticsound,	// [coord3] [byte] samp [byte] vol [byte] aten
	svc_intermission,		// [vec3_t] origin [vec3_t] angle
	svc_finale,				// ([string] music) [string] text
	svc_cdtrack,			// [byte] track ([byte] looptrack)
	svc_restore,
	svc_cutscene,
	svc_weaponanim,
	svc_decalname,
	svc_roomtype,
	svc_addangle,
	svc_newusermsg,
	svc_packetentities,		// [...]
	svc_deltapacketentities, // [...]
	svc_choke,
	svc_resourcelist,		// [strings]
	svc_newmovevars,
	svc_resourcerequest,
	svc_customization,
	svc_crosshairangle,
	svc_soundfade,
	svc_filetxferfailed,
	svc_hltv,
	svc_director,
	
	svc_voiceinit,
	svc_voicedata,
	
	svc_sendextrainfo,
	svc_timescale,
	svc_resourcelocation,
	
	svc_sendcvarvalue,
	svc_sendcvarvalue2,
	
	svc_startofusermessages = svc_sendcvarvalue2,
	
	svc_endoflist = 255,
} svc_commands_t;

//==============================================

//
// client to server
//
typedef enum clc_commands_e
{
	clc_bad = 0,
	
	clc_nop,
	
	clc_move,				// [usercmd_t]
	clc_stringcmd,			// [string] message
	clc_delta,				// [byte] sequence number, requests delta compression of message
	clc_resourcelist,
	clc_tmove,				// teleport request, spectator only
	clc_fileconsistency,
	clc_voicedata,
	clc_hltv,
	
	clc_cvarvalue,
	clc_cvarvalue2,
	
	clc_endoflist = 255,
} clc_commands_t;	

//==============================================

/*
==========================================================

ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

/**
*	Default minimum number of clients for multiplayer servers
*/
#define MP_MIN_CLIENTS 6

// See com_model.h for MAX_CLIENTS

#define	MAX_PACKET_ENTITIES	64 // doesn't count nails

struct packet_entities_t
{
	int num_entities;

	byte flags[ 32 ];
	entity_state_t* entities;
};