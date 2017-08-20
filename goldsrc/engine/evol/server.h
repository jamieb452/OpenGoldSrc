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

// server.h

#pragma once

#include "public/FileSystem.h"
#include "entity_state.h"
#include "eventapi.h"
#include "progs.h"
#include "protocol.h"
#include "pm_defs.h"
#include "pm_info.h"
#include "eiface.h"
#include "usercmd.h"
#include "consistency.h"
#include "inst_baseline.h"

//=============================================================================

/// some commands are only valid before the server has finished
/// initializing (precache commands, static sounds / objects, etc)
typedef enum
{
	ss_dead,			// no map loaded
	ss_loading,			// spawning level edicts
	ss_active			// actively running
} server_state_t;

typedef struct
{
	bool active;					// false when server is going down

	bool paused;
	bool loadgame;

	double time;
	double oldtime;

	int lastcheck;
	double lastchecktime;

	char name[ 64 ];

	char oldname[ 64 ];
	char startspot[ 64 ];
	char modelname[ 64 ];

	model_t* worldmodel;
	CRC32_t worldmapCRC;

	byte clientdllmd5[ 16 ];

	resource_t resourcelist[ 1280 ];
	int num_resources;

	consistency_t consistency_list[ 512 ];
	int num_consistency;

	char* model_precache[ 512 ];
	model_t* models[ 512 ];
	byte model_precache_flags[ 512 ];

	event_t event_precache[ EVENT_MAX_EVENTS ];

	char* sound_precache[ 512 ];
	short sound_precache_hashedlookup[ 1023 ];
	bool sound_precache_hashedlookup_built;

	char* generic_precache[ 512 ];
	char generic_precache_names[ 512 ][ 64 ];
	int num_generic_names;

	char* lightstyles[ 64 ];

	int num_edicts;
	int max_edicts;
	edict_t* edicts;

	entity_state_s* baselines;
	extra_baselines_s* instance_baselines;

	server_state_t state;

	sizebuf_t datagram;
	byte datagram_buf[ 4000 ];

	sizebuf_t reliable_datagram;
	byte reliable_datagram_buf[ 4000 ];

	sizebuf_t multicast;
	byte multicast_buf[ 1024 ];

	sizebuf_t spectator;
	byte spectator_buf[ 1024 ];

	sizebuf_t signon;
	byte signon_data[ 32768 ];
} server_t;

/*
typedef enum
{
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	cs_connected,	// has been assigned to a client_t, but not in game yet
	cs_spawned		// client is fully in game
} client_state_t;
*/

typedef struct client_frame_s
{
	// received from client

	// reply
	double senttime;
	float ping_time;
	clientdata_t clientdata;
	weapon_data_t weapondata[ 64 ];
	packet_entities_t entities;
} client_frame_t;

#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

typedef struct client_s
{
	bool active;						// false = client is free
	bool spawned;						// false = don't send datagrams
	bool fully_connected;				// true = client has fully connected, set after sendents command is received
	bool connected;						// Has been assigned to a client_t, but not in game yet
	bool uploading;						// true = client uploading custom resources
	bool hasusrmsgs;					// Whether this client has received the list of user messages
	bool has_force_unmodified;			// true = mp_consistency is set and at least one file is forced to be consistent

	//===== NETWORK ============
	netchan_t netchan;

	int chokecount;						// amount of choke since last client message
	int delta_sequence;					// -1 = no compression

	bool fakeclient;					// Bot
	bool proxy;							// HLTV proxy

	usercmd_t lastcmd;					// for filling in big drops and partial predictions

	double connecttime;					// Time at which client connected, this is the time after "spawn" is sent, not initial connection
	double cmdtime;						// Time since connecttime that last usercmd was received
	double ignorecmdtime;				// Time until which usercmds are ignored

	float latency;						// Average latency
	float packet_loss;					// Packet loss suffered by this client

	double localtime;					// of last message
	double nextping;					// next time to recalculate ping for this client
	double svtimebase;					// Server timebase for the client when running movement

	// the datagram is written to after every frame, but only cleared
	// when it is sent out to the client.  overflow is tolerated.
	sizebuf_t datagram;
	byte datagram_buf[ MAX_DATAGRAM ];

	double connection_started;			// or time of disconnect for zombies TODO verify that zombies still exist - Solokiller
	double next_messagetime;			// Earliest time to send another message
	double next_messageinterval;		// Minimum interval between messages

	bool send_message;					// set on frames a datagram arived on
	bool skip_message;					// Skip sending message next frame

	client_frame_t* frames;				// updates can be deltad from here

	event_state_t events;

	edict_t* edict;						// EDICT_NUM(clientnum+1)
	const edict_t* pViewEntity;			// View entity, equal to edict if not overridden

	int userid;							// identifying number
	//USERID_t network_userid;

	char userinfo[ MAX_INFO_STRING ];	// infostring
	bool sendinfo;						// at end of frame, send info to all
										// this prevents malicious multiple broadcasts
	float sendinfo_time;				// Time when userinfo was sent

	char hashedcdkey[ 64 ];				// Hashed cd key from user. Really the user's IP address in IPv4 form
	char name[ 32 ];					// for printing to other people
										// extracted from userinfo

	int topcolor;						// top color for model
	int bottomcolor;					// bottom color for model

	int entityId;						// unused TODO verify - Solokiller

	resource_t resourcesonhand;			// Head of resources accounted for list
	resource_t resourcesneeded;			// Head of resources to download list

	FileHandle_t upload;				// Handle of file being uploaded

	bool uploaddoneregistering;			// If client files have finished uploading

	customization_t customdata;			// Head of custom client data list

	int crcValue;						// checksum for calcs

	int lw;								// If user is predicting weapons locally (cl_lw)
	int lc;								// If user is lag compensating (cl_lc)

	char physinfo[ MAX_PHYSINFO_STRING ];	//Physics info string

	bool m_bLoopback;					// True if client has voice loopback enabled

	uint32 m_VoiceStreams[ 2 ];			// Bit mask for whether client is listening to other client TODO 64 clients? - Solokiller
	double m_lastvoicetime;				// Last time client voice data was processed on server

	int m_sendrescount;					// Count of times resources sent to client
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

/**
*	log messages are used so that fraglog processes can get stats
*/
struct server_log_t
{
	/**
	*	Is the log file active?
	*/
	bool active;

	/**
	*	Are we logging to a remote address?
	*/
	bool net_log;

	/**
	*	Remote address to log to
	*/
	netadr_t net_address;

	/**
	*	Handle to the log file
	*/
	void* file;
};

typedef struct
{
	/**
	*	Total number of samples taken over server lifetime
	*/
	int num_samples;

	/**
	*	Number of samples where server was filled to capacity (numclients == maxclients)
	*/
	int at_capacity;

	/**
	*	Number of samples where server was empty (numclients <= 1, singleplayer counts as empty)
	*/
	int at_empty;

	/**
	*	Percentage of time that server was at capacity
	*/
	float capacity_percent;

	/**
	*	Percentage of time that server was empty
	*/
	float empty_percent;

	/**
	*	Lowest number of players on server at any time
	*/
	int minusers;

	/**
	*	Highest number of players on server at any time
	*/
	int maxusers;

	/**
	*	Cumulative occupancy level over time
	*/
	float cumulative_occupancy;

	/**
	*	Average occupancy
	*/
	float occupancy;

	/**
	*	Number of client sessions (clients that joined and left, and were on server for more than a minute)
	*/
	int num_sessions;

	/**
	*	Total amount of time spent on server by all clients with recorded session
	*/
	float cumulative_sessiontime;

	/**
	*	Average length of a single client session
	*/
	float average_session_len;

	/**
	*	Cumulation of average latency for all clients per sample
	*/
	float cumulative_latency;

	/**
	*	Average latency for all clients over server lifetime
	*/
	float average_latency;
} server_stats_t;

typedef struct
{
	/**
	*	Whether the server dll has been loaded and initialized
	*/
	bool dll_initialized;

	/**
	*	Array of maxclientslimit clients
	*/
	client_t* clients;

	/**
	*	Maximum number of players on this server as defined by host
	*/
	int maxclients;

	/**
	*	Maximum number of players supported on this server as dictated by memory limits
	*/
	int maxclientslimit;

	/**
	*	number of servers spawned since start,
	*	used to check late spawns
	*/
	int spawncount;

	/**
	*	episode completion information
	*	TODO: unused? - Solokiller
	*/
	int serverflags;

	server_log_t log;

	/**
	*	Next time to clear stats
	*/
	double next_cleartime;

	/**
	*	Next time to gather stat samples
	*/
	double next_sampletime;

	/**
	*	Server statistics
	*/
	server_stats_t stats;

	/**
	*	Whether server is secure
	*	TODO: unused? - Solokiller
	*/
	bool isSecure;
} server_static_t;

//=============================================================================

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

#define	MULTICAST_ALL			0
#define	MULTICAST_PHS			1
#define	MULTICAST_PVS			2

#define	MULTICAST_ALL_R			3
#define	MULTICAST_PHS_R			4
#define	MULTICAST_PVS_R			5

//============================================================================

extern	cvar_t	teamplay;
extern	cvar_t	skill;
extern	cvar_t	deathmatch;
extern	cvar_t	coop;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				///< persistant server info
extern	server_t		sv;					///< local server

extern playermove_t g_svmove;

extern DLL_FUNCTIONS gEntityInterface;

extern	client_t	*host_client;

extern	jmp_buf 	host_abortserver;

extern	double		host_time;

extern	edict_t		*sv_player;

//===========================================================

//
// sv_main.c
//
void SV_Init ();
void SV_Shutdown ();
void SV_Frame (float time);
void SV_FinalMessage (char *message);
//void SV_DropClient (client_t *drop);
void SV_DropClient (qboolean crash);

int SV_CalcPing (client_t *cl);
void SV_FullClientUpdate (client_t *client, sizebuf_t *buf);

int SV_ModelIndex (char *name);

qboolean SV_CheckBottom (edict_t *ent);
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink);

//void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg);
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg);

void SV_MoveToGoal ();

void SV_SaveSpawnparms ();

void SV_Physics_Client (edict_t	*ent);

void SV_ExecuteUserCommand (char *s);
void SV_InitOperatorCommands ();

void SV_SendServerinfo (client_t *client);
void SV_ExtractFromUserinfo (client_t *cl);

void Master_Heartbeat ();
void Master_Packet ();

//
// sv_init.c
//
void SV_SpawnServer (char *server, char *startspot);
void SV_FlushSignon ();

//
// sv_phys.c
//
void SV_ProgStartFrame ();
void SV_Physics ();
void SV_CheckVelocity (edict_t *ent);
void SV_AddGravity (edict_t *ent, float scale);
qboolean SV_RunThink (edict_t *ent);
void SV_Physics_Toss (edict_t *ent);
void SV_RunNewmis ();
void SV_Impact (edict_t *e1, edict_t *e2);
void SV_SetMoveVars();

//
// sv_send.c
//
void SV_SendClientMessages ();

void SV_Multicast (vec3_t origin, int to);
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation);
//void SV_ClientPrintf (client_t *cl, int level, char *fmt, ...);
void SV_ClientPrintf (char *fmt, ...);
//void SV_BroadcastPrintf (int level, char *fmt, ...);
void SV_BroadcastPrintf (char *fmt, ...);
void SV_BroadcastCommand (char *fmt, ...);
void SV_SendMessagesToAll ();
void SV_FindModelNumbers ();

//
// sv_user.c
//
void SV_ExecuteClientMessage (client_t *cl);
void SV_UserInit ();
void SV_TogglePause (const char *msg);

//
// svonly.c
//
typedef enum
{
	RD_NONE,
	RD_CLIENT,
	RD_PACKET
} redirect_t;

void SV_BeginRedirect (redirect_t rd);
void SV_EndRedirect ();

//
// sv_ccmds.c
//
void SV_Status_f ();

//
// sv_ents.c
//
void SV_WriteEntitiesToClient (client_t *client, sizebuf_t *msg);

////

void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation);

void SV_ClearDatagram ();

void SV_SetIdealPitch ();

void SV_AddUpdates ();

void SV_ClientThink ();
void SV_AddClientToServer (struct qsocket_s	*ret);

void SV_CheckForNewClients ();
void SV_RunClients ();