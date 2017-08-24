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

// client.h

#pragma once

#include "quakedef.h"
#include "common.h"
#include "common/mathlib.h"
#include "common/const.h"
#include "common/pmtrace.h"
#include "pm_shared/pm_defs.h"
#include "engine/cdll_int.h"
#include "engine/APIProxy.h"
#include "common/dlight.h"
#include "common/com_model.h"
#include "common/usercmd.h"
#include "common/weaponinfo.h"
#include "eventapi.h"
#include "common/crc.h"
//#include "common/event_state.h"
#include "consistency.h"
#include "common/screenfade.h"
#include "common/cvardef.h"
#include "vid.h"
#include "net_chan.h"
#include "public/FileSystem.h"
#include "common/cl_entity.h"
//#include "sound.h"
//#include "model.h"

/*
typedef struct player_info_s
{
	float	entertime;
	int		frags;
	byte	pl;

	byte	translations[VID_GRADES*256];
} player_info_t;
*/

typedef struct soundfade_s
{
	int nStartPercent;
	int nClientSoundFadePercent;
	double soundFadeStartTime;
	int soundFadeOutTime;
	int soundFadeHoldTime;
	int soundFadeInTime;
} soundfade_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	byte	translations[VID_GRADES*256];
} scoreboard_t;

/*
// unused
typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	NUM_CSHIFTS		4
// unused
*/

const int NAME_LENGTH = 64;

//
// client_state_t should hold all pieces of the client state
//

const int SIGNONS = 4; // signon messages to receive before connected

const int MAX_DLIGHTS = 32;
const int MAX_ELIGHTS = 64;

const int MAX_BEAMS = 24;

typedef struct
{
	int		entity;
	struct model_s	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;

const int MAX_EFRAGS = 640;

const int MAX_MAPSTRING = 2048;

const int MAX_DEMOS = 32;
const int MAX_DEMONAME = 16;

typedef enum cactive_e
{
	ca_dedicated = 0, 	// This is a dedicated server, client code is inactive (no ability to start a client)
	ca_disconnected, 	// full screen console with no connection
	ca_connecting,		// netchan_t established, waiting for svc_serverdata
	ca_connected,		// valid netcon, talking to a server (processing data lists, donwloading, etc)
	ca_uninitialized,
	ca_active			// everything is in, so frames can be rendered
} cactive_t;

typedef struct cmd_s
{
	usercmd_t cmd;
	float senttime;
	float receivedtime;
	float frame_lerp;
	qboolean processedfuncs;
	qboolean heldback;
	int sendsize;
} cmd_t;

/**
*	the client_static_t structure is persistent through an arbitrary number
*	of server connections
*/
typedef struct
{
	// connection information
	cactive_t state;

	// network stuff
	netchan_t netchan;

	sizebuf_t datagram;
	byte datagram_buf[ MAX_DATAGRAM ];

	double connect_time; // for connection retransmits
	int connect_retry;

	int challenge;

	byte authprotocol;

	int userid;

	char trueaddress[ 32 ];

	float slist_time;

	//TODO: define constants - Solokiller
	int signon;

	char servername[ MAX_OSPATH ];	// name of server from original connect
	char mapstring[ 64 ];

	char spawnparms[ 2048 ];

	// private userinfo for sending to masterless servers
	char userinfo[ MAX_INFO_STRING ];

	float nextcmdtime;
	int lastoutgoingcommand;

	// demo loop control
	int demonum;									// -1 = don't play demos
	char demos[ MAX_DEMOS ][ MAX_DEMONAME ];		// when not playing
													// demo recording info must be here, because record is started before
													// entering a map (and clearing client_state_t)
	bool demorecording;
	bool demoplayback;
	bool timedemo;

	float demostarttime;
	int demostartframe;

	int forcetrack;

	FileHandle_t demofile;
	FileHandle_t demoheader;
	
	bool demowaiting;
	bool demoappending;
	char demofilename[ MAX_OSPATH ];
	int demoframecount;

	int td_lastframe;		// to meter out one message a frame
	int td_startframe;		// host_framecount at start
	float td_starttime;		// realtime at second frame of timedemo

	//incomingtransfer_t dl;

	float packet_loss;
	double packet_loss_recalc_time;

	int playerbits;

	soundfade_t soundfade;

	char physinfo[ MAX_PHYSINFO_STRING ];

	byte md5_clientdll[ 16 ];

	netadr_t game_stream;
	netadr_t connect_stream;

	bool passive;
	bool spectator;
	bool director;
	bool fSecureClient;
	bool isVAC2Secure;

	uint64 GameServerSteamID;

	int build_num;
} client_static_t;

extern client_static_t	cls;

typedef struct
{
	double receivedtime;
	double latency;

	qboolean invalid;
	qboolean choked;

	entity_state_t playerstate[ MAX_CLIENTS ];

	double time;
	clientdata_t clientdata;
	weapon_data_t weapondata[ 64 ];
	packet_entities_t packet_entities;

	unsigned short clientbytes;
	unsigned short playerinfobytes;
	unsigned short packetentitybytes;
	unsigned short tentitybytes;
	unsigned short soundbytes;
	unsigned short eventbytes;
	unsigned short usrbytes;
	unsigned short voicebytes;
	unsigned short msgbytes;
} frame_t;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	//TODO: verify contents - Solokiller
	int max_edicts;

	resource_t resourcesonhand;
	resource_t resourcesneeded;
	resource_t resourcelist[ 1280 ]; // MAX_RESOURCES
	int num_resources;

	qboolean need_force_consistency_response;

	char serverinfo[ 512 ]; // MAX_SERVERINFO_STRING

	int servercount;

	int validsequence;

	int parsecount;
	int parsecountmod;

	int stats[ 32 ];

	int weapons;

	usercmd_t cmd;

	vec3_t viewangles;
	vec3_t punchangle;
	vec3_t crosshairangle;
	
	vec3_t simorg;
	vec3_t simvel;
	vec3_t simangles;
	
	vec3_t predicted_origins[ 64 ];
	vec3_t prediction_error;

	float idealpitch;

	vec3_t viewheight;

	screenfade_t sf;

	bool paused;

	int onground;
	int moving;
	int waterlevel;
	int usehull;

	float maxspeed;

	int pushmsec;
	int light_level;
	int intermission;

	double mtime[ 2 ];
	double time;
	double oldtime;

	frame_t frames[ 64 ];
	
	cmd_t commands[ 64 ];

	//local_state_t predicted_frames[ 64 ];
	
	int delta_sequence;

	int playernum;
	event_t event_precache[ EVENT_MAX_EVENTS ];

	model_t* model_precache[ MAX_MODELS ];
	int model_precache_count;

	sfx_t* sound_precache[ MAX_SOUNDS ];

	consistency_t consistency_list[ 512 ];
	int num_consistency;

	int highentity;

	char levelname[ 40 ];

	int maxclients;

	int gametype;
	int viewentity;

	model_t* worldmodel;

	efrag_t* free_efrags;

	int num_entities;
	int num_statics;

	cl_entity_t viewent;

	int cdtrack;
	int looptrack;

	CRC32_t serverCRC;

	byte clientdllmd5[ 16 ];

	float weaponstarttime;
	int weaponsequence;

	int fPrecaching;

	dlight_t* pLight;
	player_info_t players[ 32 ]; // MAX_PLAYERS

	entity_state_t instanced_baseline[ 64 ];

	int instanced_baseline_number;

	CRC32_t mapCRC;

	event_state_t events;

	char downloadUrl[128];
} client_state_t;

extern	client_state_t	cl;

extern playermove_t g_clmove;

extern cl_enginefunc_t cl_enginefuncs;

extern char g_szfullClientName[512];

class CSysModule;
extern CSysModule* hClientDLL;

//
// cvars
//
extern	cvar_t	cl_name;
extern	cvar_t	cl_color;

extern cvar_t cl_mousegrab; // shouldn't be here?
extern cvar_t m_rawinput; // shouldn't be here?
extern cvar_t rate;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_autofire;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

const int MAX_TEMP_ENTITIES = 500;   // lightning bolts, etc
const int MAX_STATIC_ENTITIES = 128; // torches, etc

// FIXME, allocate dynamically
//extern	entity_state_t	cl_baselines[MAX_EDICTS];
extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	cl_entity_t		cl_entities[MAX_EDICTS];
extern	cl_entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];
extern	cl_entity_t		cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t			cl_beams[MAX_BEAMS];

//=============================================================================

//
// cl_main
//
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights ();

void CL_Init();
void CL_Shutdown();

void CL_ShutDownClientStatic();

void CL_Disconnect ();
void CL_Disconnect_f ();
void CL_NextDemo ();
//qboolean CL_DemoBehind();

const int MAX_VISEDICTS = 256;

extern	int				cl_numvisedicts;
extern	cl_entity_t		*cl_visedicts[MAX_VISEDICTS];

//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput ();
void CL_SendCmd ();
void CL_SendMove (usercmd_t *cmd);

void CL_ParseTEnt ();
void CL_UpdateTEnts ();

void CL_ClearState ();

void CL_ReadPackets ();

void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);

float CL_KeyState (kbutton_t *key);

//
// cl_demo.c
//
void CL_StopPlayback ();
//qboolean CL_GetMessage ();
int CL_GetMessage ();
//void CL_WriteDemoCmd (usercmd_t *pcmd);

void CL_Stop_f ();
void CL_Record_f ();
//void CL_ReRecord_f ();
void CL_PlayDemo_f ();
void CL_TimeDemo_f ();

//
// cl_parse.c
//
void CL_ParseServerMessage ();
void CL_NewTranslation (int slot);

//
// cl_tent
//
void CL_InitTEnts ();
void CL_SignonReply ();
//void CL_ClearTEnts ();

//
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities ();
void CL_ClearProjectiles ();
void CL_ParseProjectiles ();
void CL_ParsePacketEntities (qboolean delta);
void CL_SetSolidEntities ();
void CL_ParsePlayerinfo ();

//
// cl_pred.c
//
void CL_InitPrediction ();
void CL_PredictMove ();
void CL_PredictUsercmd (local_state_t *from, local_state_t *to, usercmd_t *u, qboolean spectator);

//
// cl_cam.c
//
#define CAM_NONE	0
#define CAM_TRACK	1

extern int autocam;
extern int spec_track; // player# of who we are tracking

qboolean Cam_DrawViewModel();
qboolean Cam_DrawPlayer(int playernum);

void Cam_Track(usercmd_t *cmd);
void Cam_FinishMove(usercmd_t *cmd);
void Cam_Reset();

void CL_InitCam();