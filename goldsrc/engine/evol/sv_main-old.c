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

// sv_main.c -- server main program

//#include "precompiled.h"
#include "quakedef.h"
/*
#include "client.h"
#include "modinfo.h"
#include "pr_edict.h"
#include "sv_main.h"
#include "sv_phys.h"
#include "server.h"
*/

server_static_t	svs;
server_t sv;

char localmodels[MAX_MODELS][5];			// inline model names for precache

//============================================================================

//TODO: implement functions and add them - Solokiller
playermove_t g_svmove;

globalvars_t gGlobalVariables = {};

bool allow_cheats = false;

cvar_t sv_allow_upload = { "sv_allowupload", "1", FCVAR_SERVER };
cvar_t mapcyclefile = { "mapcyclefile", "mapcycle.txt" };
cvar_t servercfgfile = { "servercfgfile", "server.cfg" };
cvar_t max_queries_sec = { "max_queries_sec", "3.0", FCVAR_PROTECTED | FCVAR_SERVER };
cvar_t max_queries_sec_global = { "max_queries_sec_global", "30", FCVAR_PROTECTED | FCVAR_SERVER };
cvar_t max_queries_window = { "max_queries_window", "60", FCVAR_PROTECTED | FCVAR_SERVER };

int SV_UPDATE_BACKUP = 1 << 3;
int SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

struct GameToAppIDMapItem_t
{
	AppId_t iAppID;
	const char *pGameDir;
};

const AppId_t HALF_LIFE_APPID = 70;

const GameToAppIDMapItem_t g_GameToAppIDMap[] =
{
	{ 10, "cstrike" },
	{ 20, "tfc" },
	{ 30, "dod" },
	{ 40, "dmc" },
	{ 50, "gearbox" },
	{ 60, "ricochet" },
	{ HALF_LIFE_APPID, "valve" },
	{ 80, "czero" },
	{ 100, "czeror" },
	{ 130, "bshift" },
	{ 150, "cstrike_beta" }
};

modinfo_t gmodinfo = {};

AppId_t GetGameAppID()
{
	char gd[ FILENAME_MAX ];
	char arg[ FILENAME_MAX ];

	COM_ParseDirectoryFromCmd( "-game", arg, "valve" );
	COM_FileBase( arg, gd );

	for( const auto& data : g_GameToAppIDMap )
	{
		if( !stricmp( data.pGameDir, gd ) )
		{
			return data.iAppID;
		}
	}

	return HALF_LIFE_APPID;
}

bool IsGameSubscribed( const char *game )
{
	AppId_t appId = HALF_LIFE_APPID;

	for( const auto& data : g_GameToAppIDMap )
	{
		if( !stricmp( data.pGameDir, game ) )
		{
			appId = data.iAppID;
		}
	}

	return ISteamApps_BIsSubscribedApp( appId );
}

bool BIsValveGame()
{
	for( const auto& data : g_GameToAppIDMap )
	{
		if( !stricmp( data.pGameDir, com_gamedir ) )
		{
			return false;
		}
	}

	return true;
}

static bool g_bCS_CZ_Flags_Initialized = false;

bool g_bIsCStrike = false;
bool g_bIsCZero = false;
bool g_bIsCZeroRitual = false;
bool g_bIsTerrorStrike = false;
bool g_bIsTFC = false;

void SetCStrikeFlags()
{
	if( !g_bCS_CZ_Flags_Initialized )
	{
		if( !stricmp( com_gamedir, "cstrike" ) || 
			!stricmp( com_gamedir, "cstrike_beta" ) )
		{
			g_bIsCStrike = true;
		}
		else if( !stricmp( com_gamedir, "czero" ) )
		{
			g_bIsCZero = true;
		}
		else if( !stricmp( com_gamedir, "czeror" ) )
		{
			g_bIsCZeroRitual = true;
		}
		else if( !stricmp( com_gamedir, "terror" ) )
		{
			g_bIsTerrorStrike = true;
		}
		else if( !stricmp( com_gamedir, "tfc" ) )
		{
			g_bIsTFC = true;
		}

		g_bCS_CZ_Flags_Initialized = true;
	}
}

void SV_DeallocateDynamicData()
{
	if (g_moved_edict)
		Mem_Free(g_moved_edict);
	
	if (g_moved_from)
		Mem_Free(g_moved_from);
	
	g_moved_edict = NULL;
	g_moved_from = NULL;
}

void SV_ReallocateDynamicData()
{
	if (!g_psv.max_edicts)
	{
		Con_DPrintf("%s: sv.max_edicts == 0\n", __func__);
		return;
	}

	int nSize = g_psv.max_edicts;

	if (g_moved_edict)
	{
		Con_Printf("Reallocate on moved_edict\n");
#ifdef REHLDS_FIXES
		Mem_Free(g_moved_edict);
#endif
	}
	g_moved_edict = (edict_t **)Mem_ZeroMalloc(sizeof(edict_t *) * nSize);

	if (g_moved_from)
	{
		Con_Printf("Reallocate on moved_from\n");
#ifdef REHLDS_FIXES
		Mem_Free(g_moved_from);
#endif
	}
	g_moved_from = (vec3_t *)Mem_ZeroMalloc(sizeof(vec3_t) * nSize);
}

void SV_AllocClientFrames()
{
	for( int i = 0; i < svs.maxclientslimit; ++i )
	{
		if( svs.clients[ i ].frames )
			Con_DPrintf( "Allocating over frame pointer?\n" );

		svs.clients[ i ].frames = reinterpret_cast<client_frame_t*>( Mem_ZeroMalloc( sizeof( client_frame_t ) * SV_UPDATE_BACKUP ) );
	}
}

void SV_ClearFrames( client_frame_t** frames )
{
	if( *frames )
	{
		auto pFrame = *frames;

		for( int i = 0; i < SV_UPDATE_BACKUP; ++i, ++pFrame )
		{
			if( pFrame->entities.entities )
			{
				Mem_Free( pFrame->entities.entities );
			}

			pFrame->entities.entities = nullptr;
			pFrame->entities.num_entities = 0;

			pFrame->senttime = 0;
			pFrame->ping_time = -1;
		}

		Mem_Free( *frames );
		*frames = nullptr;
	}
}

void SV_ResetModInfo()
{
	Q_memset( &gmodinfo, 0, sizeof( gmodinfo ) );

	gmodinfo.version = 1;
	gmodinfo.svonly = true;
	gmodinfo.num_edicts = MAX_EDICTS;

	char szDllListFile[ FILENAME_MAX ];
	snprintf( szDllListFile, sizeof( szDllListFile ), "%s", "liblist.gam" );

	FileHandle_t hLibListFile = FS_Open( szDllListFile, "rb" );

	if( hLibListFile != FILESYSTEM_INVALID_HANDLE )
	{
		char szKey[ 64 ];
		char szValue[ 256 ];

		const int iSize = FS_Size( hLibListFile );

		if( iSize > ( 512 * 512 ) || !iSize )
			Sys_Error( "Game listing file size is bogus [%s: size %i]", "liblist.gam", iSize );

		byte* pFileData = reinterpret_cast<byte*>( Mem_Malloc( iSize + 1 ) );

		if( !pFileData )
			Sys_Error( "Could not allocate space for game listing file of %i bytes", iSize + 1 );

		const int iRead = FS_Read( pFileData, iSize, hLibListFile );

		if( iRead != iSize )
			Sys_Error( "Error reading in game listing file, expected %i bytes, read %i", iSize, iRead );

		pFileData[ iSize ] = '\0';

		char* pBuffer = ( char* ) pFileData;

		com_ignorecolons = true;

		while( 1 )
		{
			pBuffer = COM_Parse( pBuffer );

			if( Q_strlen( com_token ) <= 0 )
				break;

			Q_strncpy( szKey, com_token, sizeof( szKey ) - 1 );
			szKey[ sizeof( szKey ) - 1 ] = '\0';

			pBuffer = COM_Parse( pBuffer );

			Q_strncpy( szValue, com_token, sizeof( szValue ) - 1 );
			szValue[ sizeof( szValue ) - 1 ] = '\0';

			if( Q_stricmp( szKey, "gamedll_linux" ) )
				DLL_SetModKey( &gmodinfo, szKey, szValue );
		}

		com_ignorecolons = false;
		Mem_Free( pFileData );
		FS_Close( hLibListFile );
	}
}

void SV_ServerShutdown()
{
	Steam_NotifyOfLevelChange();
	gGlobalVariables.time = sv.time;

	if (svs.dll_initialized)
	{
		if (sv.active)
			gEntityInterface.pfnServerDeactivate();
	}
}

/*
===============
SV_Init
===============
*/
void SV_Init ()
{
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_nostep;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_edgefriction;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_maxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_idealpitchscale;
	extern	cvar_t	sv_aim;

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_edgefriction);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_idealpitchscale);
	Cvar_RegisterVariable (&sv_aim);
	Cvar_RegisterVariable (&sv_nostep);

	Cvar_RegisterVariable( &sv_allow_upload );
	
	Cvar_RegisterVariable( &mapcyclefile );
	Cvar_RegisterVariable( &servercfgfile );
	
	Cvar_RegisterVariable( &max_queries_sec );
	Cvar_RegisterVariable( &max_queries_sec_global );
	Cvar_RegisterVariable( &max_queries_window );
	
	for (int i=0 ; i<MAX_MODELS ; i++)
		sprintf (localmodels[i], "*%i", i);
}

void SV_Shutdown()
{
	g_DeltaJitRegistry.Cleanup();
	delta_info_t *p = g_sv_delta;
	while (p)
	{
		delta_info_t *n = p->next;
		if (p->delta)
			DELTA_FreeDescription(&p->delta);

		Mem_Free(p->name);
		Mem_Free(p->loadfile);
		Mem_Free(p);
		p = n;
	}

	g_sv_delta = NULL;
}

void SV_SetMaxclients()
{
	for( int i = 0; i < svs.maxclientslimit; ++i )
		SV_ClearFrames( &svs.clients[ i ].frames );

	svs.maxclients = 1;

	const int iCmdMaxPlayers = COM_CheckParm( "-maxplayers" );

	if( iCmdMaxPlayers )
		svs.maxclients = Q_atoi( com_argv[ iCmdMaxPlayers + 1 ] );

	cls.state = g_bIsDedicatedServer ? ca_dedicated : ca_disconnected;

	if( svs.maxclients <= 0 )
		svs.maxclients = MP_MIN_CLIENTS;
	else if( svs.maxclients > MAX_CLIENTS )
		svs.maxclients = MAX_CLIENTS;

	svs.maxclientslimit = MAX_CLIENTS;

	//If we're a listen server and we're low on memory, reduce maximum player limit.
	if( !g_bIsDedicatedServer && host_parms.memsize < LISTENSERVER_SAFE_MINIMUM_MEMORY )
		svs.maxclientslimit = 4;

	//Increase the number of updates available for multiplayer.
	SV_UPDATE_BACKUP = 8;

	if( svs.maxclients != 1 )
		SV_UPDATE_BACKUP = 64;

	SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

	svs.clients = reinterpret_cast<client_t*>( Hunk_AllocName( sizeof( client_t ) * svs.maxclientslimit, "clients" ) );

	for( int i = 0; i < svs.maxclientslimit; ++i )
	{
		auto& client = svs.clients[ i ];

		Q_memset( &client, 0, sizeof( client ) );
		client.resourcesneeded.pPrev = &client.resourcesneeded;
		client.resourcesneeded.pNext = &client.resourcesneeded;
		client.resourcesonhand.pPrev = &client.resourcesonhand;
		client.resourcesonhand.pNext = &client.resourcesonhand;
	}

	Cvar_SetValue( "deathmatch", svs.maxclients > 1 ? 1 : 0 );

	SV_AllocClientFrames();

	int maxclients = svs.maxclientslimit;

	if( maxclients > svs.maxclients )
		maxclients = svs.maxclients;

	svs.maxclients = maxclients;
}

void SV_HandleRconPacket()
{
	MSG_BeginReading();
	MSG_ReadLong();
	char* s = MSG_ReadStringLine();
	Cmd_TokenizeString(s);
	const char* c = Cmd_Argv(0);
	if (!Q_strcmp(c, "getchallenge"))
	{
		SVC_GetChallenge();
	}
	else if (!Q_stricmp(c, "challenge"))
	{
		SVC_ServiceChallenge();
	}
	else if (!Q_strcmp(c, "rcon"))
	{
		SV_Rcon(&net_from);
	}
}

void SV_CheckCmdTimes()
{
	static double lastreset;

	if (Host_IsSinglePlayerGame())
		return;

	if (realtime - lastreset < 1.0)
		return;

	lastreset = realtime;
	for (int i = g_psvs.maxclients - 1; i >= 0; i--)
	{
		client_t* cl = &g_psvs.clients[i];
		if (!cl->connected || !cl->active)
			continue;

		if (cl->connecttime == 0.0)
			cl->connecttime = realtime;

		float dif = cl->connecttime + cl->cmdtime - realtime;
		if (dif > clockwindow.value)
		{
			cl->ignorecmdtime = clockwindow.value + realtime;
			cl->cmdtime = realtime - cl->connecttime;
		}

		if (dif < -clockwindow.value)
			cl->cmdtime = realtime - cl->connecttime;
	}
}

void SV_CheckForRcon()
{
	if (g_psv.active || g_pcls.state != ca_dedicated || giActive == DLL_CLOSE || !host_initialized)
		return;

	while (NET_GetPacket(NS_SERVER))
	{
		if (SV_FilterPacket())
		{
			SV_SendBan();
		}
		else
		{
			if (*(uint32 *)net_message.data == 0xFFFFFFFF)
				SV_HandleRconPacket();
		}
	}
}

void SV_CountPlayers( int* clients )
{
	*clients = 0;

	for( int i = 0; i < svs.maxclients; ++i )
	{
		auto& client = svs.clients[ i ];

		if( client.active || client.spawned || client.connected )
		{
			++*clients;
		}
	}
}

void SV_CountProxies(int *proxies)
{
	*proxies = 0;
	client_s *cl = g_psvs.clients;

	for (int i = 0; i < g_psvs.maxclients; i++, cl++)
	{
		if (cl->active || cl->spawned || cl->connected)
		{
			if (cl->proxy)
				(*proxies)++;
		}
	}
}

void SV_KickPlayer( int nPlayerSlot, int nReason )
{
	if (nPlayerSlot < 0 || nPlayerSlot >= g_psvs.maxclients)
		return;

	client_t * client = &g_psvs.clients[nPlayerSlot];
	if (!client->connected || !g_psvs.isSecure)
		return;

	USERID_t id;
	Q_memcpy(&id, &client->network_userid, sizeof(id));

	Log_Printf("Secure: \"%s<%i><%s><>\" was detected cheating and dropped from the server.\n", client->name, client->userid, SV_GetIDString(&id), nReason);

	char rgchT[1024];
	rgchT[0] = svc_print;
	Q_sprintf(
		&rgchT[1],
		"\n********************************************\nYou have been automatically disconnected\nfrom this secure server because an illegal\ncheat was detected on your computer.\nRepeat violators may be permanently banned\nfrom all secure servers.\n\nFor help cleaning your system of cheats, visit:\nhttp://www.counter-strike.net/cheat.html\n********************************************\n\n"
	);
	Netchan_Transmit(&g_psvs.clients[nPlayerSlot].netchan, Q_strlen(rgchT) + 1, (byte *)rgchT);

	Q_sprintf(rgchT, "%s was automatically disconnected\nfrom this secure server.\n", client->name);
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		if (!g_psvs.clients[i].active && !g_psvs.clients[i].spawned || g_psvs.clients[i].fakeclient)
			continue;

		MSG_WriteByte(&g_psvs.clients[i].netchan.message, svc_centerprint);
		MSG_WriteString(&g_psvs.clients[i].netchan.message, rgchT);
	}

	SV_DropClient(&g_psvs.clients[nPlayerSlot], FALSE, "Automatically dropped by cheat detector");
}

void SV_LoadEntities()
{
	ED_LoadFromFile(g_psv.worldmodel->entities);
}

void SV_ClearEntities()
{
	for( int i = 0; i < sv.num_edicts; ++i )
	{
		if( !sv.edicts[ i ].free )
		{
			//TODO: need to increment the serial number so EHANDLE works properly - Solokiller
			ReleaseEntityDLLFields( &sv.edicts[ i ] );
		}
	}
}

int EXT_FUNC RegUserMsg(const char *pszName, int iSize)
{
	if (giNextUserMsg > 255 || !pszName || Q_strlen(pszName) > 11 || iSize > 192)
		return 0;

	UserMsg *pUserMsgs = sv_gpUserMsgs;
	while (pUserMsgs)
	{
		if (!Q_strcmp(pszName, pUserMsgs->szName))
			return pUserMsgs->iMsg;

		pUserMsgs = pUserMsgs->next;
	}

	UserMsg *pNewMsg = (UserMsg *)Mem_ZeroMalloc(sizeof(UserMsg));
	pNewMsg->iMsg = giNextUserMsg++;
	pNewMsg->iSize = iSize;
	Q_strcpy(pNewMsg->szName, pszName);
	pNewMsg->next = sv_gpNewUserMsgs;
	sv_gpNewUserMsgs = pNewMsg;

	return pNewMsg->iMsg;
}

void SV_InactivateClients()
{
	int i;
	client_t *cl;
	for (i = 0, cl = g_psvs.clients; i < g_psvs.maxclients; i++, cl++)
	{
		if (!cl->active && !cl->connected && !cl->spawned)
			continue;

		if (cl->fakeclient)
			SV_DropClient(cl, FALSE, "Dropping fakeclient on level change");
		else
		{
			cl->active = FALSE;
			cl->connected = TRUE;
			cl->spawned = FALSE;
			cl->fully_connected = FALSE;

			SZ_Clear(&cl->netchan.message);
			SZ_Clear(&cl->datagram);

			COM_ClearCustomizationList(&cl->customdata, FALSE);
			Q_memset(cl->physinfo, 0, MAX_PHYSINFO_STRING);
		}
	}
}

void SV_FailDownload(const char *filename)
{
	if (filename && *filename)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_filetxferfailed);
		MSG_WriteString(&host_client->netchan.message, filename);
	}
}

void SV_ClearCaches()
{
	for( int i = 1; i < ARRAYSIZE( sv.event_precache ) && sv.event_precache[ i ].filename; ++i )
	{
		auto& event = sv.event_precache[ i ];

		event.filename = nullptr;

		if( event.pszScript )
			Mem_Free( const_cast<char*>( event.pszScript ) );

		event.pszScript = nullptr;
	}
}

void SV_PropagateCustomizations()
{
	//TODO: implement - Solokiller
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*  
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;	
	MSG_WriteByte (&sv.datagram, svc_particle);
	MSG_WriteCoord (&sv.datagram, org[0]);
	MSG_WriteCoord (&sv.datagram, org[1]);
	MSG_WriteCoord (&sv.datagram, org[2]);
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar (&sv.datagram, v);
	}
	MSG_WriteByte (&sv.datagram, count);
	MSG_WriteByte (&sv.datagram, color);
}           

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation)
{       
    int         sound_num;
    int field_mask;
    int			i;
	int			ent;
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;	

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS
        && sv.sound_precache[sound_num] ; sound_num++)
        if (!strcmp(sample, sv.sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS || !sv.sound_precache[sound_num] )
    {
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

// directed messages go only to the entity the are targeted on
	MSG_WriteByte (&sv.datagram, svc_sound);
	MSG_WriteByte (&sv.datagram, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&sv.datagram, volume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&sv.datagram, attenuation*64);
	MSG_WriteShort (&sv.datagram, channel);
	MSG_WriteByte (&sv.datagram, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&sv.datagram, entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]));
}           

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	char			**s;
	char			message[2048];

	MSG_WriteByte (&client->message, svc_print);
	sprintf (message, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, VERSION, pr_crc);
	MSG_WriteString (&client->message,message);

	MSG_WriteByte (&client->message, svc_serverinfo);
	MSG_WriteLong (&client->message, PROTOCOL_VERSION);
	MSG_WriteByte (&client->message, svs.maxclients);

	if (!coop.value && deathmatch.value)
		MSG_WriteByte (&client->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->message, GAME_COOP);

	sprintf (message, pr_strings+sv.edicts->v.message);

	MSG_WriteString (&client->message,message);

	for (s = sv.model_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

	for (s = sv.sound_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

// send music
	MSG_WriteByte (&client->message, svc_cdtrack);
	MSG_WriteByte (&client->message, sv.edicts->v.sounds);
	MSG_WriteByte (&client->message, sv.edicts->v.sounds);

// set view	
	MSG_WriteByte (&client->message, svc_setview);
	MSG_WriteShort (&client->message, NUM_FOR_EDICT(client->edict));

	MSG_WriteByte (&client->message, svc_signonnum);
	MSG_WriteByte (&client->message, 1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum)
{
	edict_t			*ent;
	client_t		*client;
	int				edictnum;
	struct qsocket_s *netconnection;
	int				i;
	float			spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients + clientnum;

	Con_DPrintf ("Client %s connected\n", client->netconnection->address);

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);
	
// set up the client_t
	netconnection = client->netconnection;
	
	if (sv.loadgame)
		memcpy (spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	memset (client, 0, sizeof(*client));
	client->netconnection = netconnection;

	strcpy (client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;
	client->message.data = client->msgbuf;
	client->message.maxsize = sizeof(client->msgbuf);
	client->message.allowoverflow = true;		// we can catch it


	if (sv.loadgame)
		memcpy (client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	else
	{
	// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram (pr_global_struct->SetNewParms);
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients ()
{
	struct qsocket_s	*ret;
	int				i;
		
//
// check for new connections
//
	while (1)
	{
		ret = NET_CheckNewConnections ();
		if (!ret)
			break;

	// 
	// init a new client structure
	//	
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("Host_CheckForNewClients: no free clients");
		
		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);	
	
		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram ()
{
	SZ_Clear (&sv.datagram);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS/8];

void SV_AddToFatPVS (vec3_t org, mnode_t *node)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS ( (mleaf_t *)node, sv.worldmodel);
				for (i=0 ; i<fatbytes ; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}
	
		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS (org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *SV_FatPVS (vec3_t org)
{
	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	Q_memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, sv.worldmodel->nodes);
	return fatpvs;
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a svc_packetentities messages and possibly
a svc_nails message and
svc_playerinfo messages
=============
*/
void SV_WriteEntitiesToClient (edict_t	*clent, sizebuf_t *msg)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	edict_t	*ent;

// find the client's PVS
	VectorAdd (clent->v.origin, clent->v.view_ofs, org);
	pvs = SV_FatPVS (org);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
#ifdef QUAKE2
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->v.effects == EF_NODRAW)
			continue;
#endif

// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
// ignore ents without visible models
			if (!ent->v.modelindex || !pr_strings[ent->v.model])
				continue;

			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;
				
			if (i == ent->num_leafs)
				continue;		// not visible
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			Con_Printf ("packet overflow\n");
			return;
		}

// send an update
		bits = 0;
		
		for (i=0 ; i<3 ; i++)
		{
			miss = ent->v.origin[i] - ent->baseline.origin[i];
			if ( miss < -0.1 || miss > 0.1 )
				bits |= U_ORIGIN1<<i;
		}

		if ( ent->v.angles[0] != ent->baseline.angles[0] )
			bits |= U_ANGLE1;
			
		if ( ent->v.angles[1] != ent->baseline.angles[1] )
			bits |= U_ANGLE2;
			
		if ( ent->v.angles[2] != ent->baseline.angles[2] )
			bits |= U_ANGLE3;
			
		if (ent->v.movetype == MOVETYPE_STEP)
			bits |= U_NOLERP;	// don't mess up the step animation
	
		if (ent->baseline.colormap != ent->v.colormap)
			bits |= U_COLORMAP;
			
		if (ent->baseline.skin != ent->v.skin)
			bits |= U_SKIN;
			
		if (ent->baseline.frame != ent->v.frame)
			bits |= U_FRAME;
		
		if (ent->baseline.effects != ent->v.effects)
			bits |= U_EFFECTS;
		
		if (ent->baseline.modelindex != ent->v.modelindex)
			bits |= U_MODEL;

		if (e >= 256)
			bits |= U_LONGENTITY;
			
		if (bits >= 256)
			bits |= U_MOREBITS;

	//
	// write the message
	//
		MSG_WriteByte (msg,bits | U_SIGNAL);
		
		if (bits & U_MOREBITS)
			MSG_WriteByte (msg, bits>>8);
		if (bits & U_LONGENTITY)
			MSG_WriteShort (msg,e);
		else
			MSG_WriteByte (msg,e);

		if (bits & U_MODEL)
			MSG_WriteByte (msg,	ent->v.modelindex);
		if (bits & U_FRAME)
			MSG_WriteByte (msg, ent->v.frame);
		if (bits & U_COLORMAP)
			MSG_WriteByte (msg, ent->v.colormap);
		if (bits & U_SKIN)
			MSG_WriteByte (msg, ent->v.skin);
		if (bits & U_EFFECTS)
			MSG_WriteByte (msg, ent->v.effects);
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (msg, ent->v.origin[0]);		
		if (bits & U_ANGLE1)
			MSG_WriteAngle(msg, ent->v.angles[0]);
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (msg, ent->v.origin[1]);
		if (bits & U_ANGLE2)
			MSG_WriteAngle(msg, ent->v.angles[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (msg, ent->v.origin[2]);
		if (bits & U_ANGLE3)
			MSG_WriteAngle(msg, ent->v.angles[2]);
	}
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts ()
{
	edict_t	*ent = NEXT_EDICT(sv.edicts);
	
	for (int e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
		ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg)
{
	int		bits;
	int		i;
	edict_t	*other;
	int		items;
#ifndef QUAKE2
	eval_t	*val;
#endif

//
// send a damage message
//
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = PROG_TO_EDICT(ent->v.dmg_inflictor);
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, ent->v.dmg_save);
		MSG_WriteByte (msg, ent->v.dmg_take);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));
	
		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->v.fixangle )
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ent->v.angles[i] );
		ent->v.fixangle = 0;
	}

	bits = 0;
	
	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;
		
	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
#ifdef QUAKE2
	items = (int)ent->v.items | ((int)ent->v.items2 << 23);
#else
	val = GetEdictFieldValue(ent, "items2");

	if (val)
		items = (int)ent->v.items | ((int)val->_float << 23);
	else
		items = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);
#endif

	bits |= SU_ITEMS;
	
	if ( (int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;
	
	if ( ent->v.waterlevel >= 2)
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1<<i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

//	if (ent->v.weapon)
		bits |= SU_WEAPON;

// send the data

	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ent->v.idealpitch);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			MSG_WriteChar (msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ent->v.velocity[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v.weaponframe);
	if (bits & SU_ARMOR)
		MSG_WriteByte (msg, ent->v.armorvalue);
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(pr_strings+ent->v.weaponmodel));
	
	MSG_WriteShort (msg, ent->v.health);
	MSG_WriteByte (msg, ent->v.currentammo);
	MSG_WriteByte (msg, ent->v.ammo_shells);
	MSG_WriteByte (msg, ent->v.ammo_nails);
	MSG_WriteByte (msg, ent->v.ammo_rockets);
	MSG_WriteByte (msg, ent->v.ammo_cells);

	if (standard_quake)
	{
		MSG_WriteByte (msg, ent->v.weapon);
	}
	else
	{
		for(i=0;i<32;i++)
		{
			if ( ((int)ent->v.weapon) & (1<<i) )
			{
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteByte (&msg, svc_time);
	MSG_WriteFloat (&msg, sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client->edict, &msg);

	SV_WriteEntitiesToClient (client->edict, &msg);

// copy the server datagram if there is space
	if (msg.cursize + sv.datagram.cursize < msg.maxsize)
		SZ_Write (&msg, sv.datagram.data, sv.datagram.cursize);

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}
	
	return true;
}

void SV_UpdateUserInfo(client_t *client)
{
#ifndef REHLDS_FIXES
	client->sendinfo = FALSE;
	client->sendinfo_time = realtime + 1.0;
#endif
	SV_ExtractFromUserinfo(client);
	SV_SendFullClientUpdateForAll(client);
#ifdef REHLDS_FIXES
	client->sendinfo = FALSE;
	client->sendinfo_time = realtime + 1.0;
#endif
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages ()
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->old_frags != host_client->edict->v.frags)
		{
			for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;
				MSG_WriteByte (&client->message, svc_updatefrags);
				MSG_WriteByte (&client->message, i);
				MSG_WriteShort (&client->message, host_client->edict->v.frags);
			}

			host_client->old_frags = host_client->edict->v.frags;
		}
	}
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		SZ_Write (&client->message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
	}

	SZ_Clear (&sv.reliable_datagram);
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop (client_t *client)
{
	sizebuf_t	msg;
	byte		buf[4];
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = realtime;
}

void SV_SkipUpdates()
{
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *client = &g_psvs.clients[i];
		if (!client->active && !client->connected && !client->spawned)
			continue;

		if (!host_client->fakeclient) //TODO: should be client, not host_client; investigation needed
			client->skip_message = TRUE;
	}
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages ()
{
	int			i;
	
// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate 
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}
			
		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
				SV_DropClient (false);	// went to another level
			else
			{
				if (NET_SendMessage (host_client->netconnection
				, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				SZ_Clear (&host_client->message);
				host_client->last_message = realtime;
				host_client->sendsignon = false;
			}
		}
	}
	
	
// clear muzzle flashes
	SV_CleanupEnts ();
}

void SV_ExtractFromUserinfo(client_t *cl)
{
	const char *val;
	int i;
	char newname[MAX_NAME];
	char *userinfo = cl->userinfo;

	val = Info_ValueForKey(userinfo, "name");
#ifdef REHLDS_FIXES
	SV_ReplaceSpecialCharactersInName(newname, val);
#else // REHLDS_FIXES
	Q_strncpy(newname, val, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = '\0';

	for (char *p = newname; *p; p++)
	{
		if (*p == '%'
			|| *p == '&'
			)
			*p = ' ';
	}

	// Fix name to not start with '#', so it will not resemble userid
	for (char *p = newname; *p == '#'; p++) *p = ' ';
#endif // REHLDS_FIXES

#ifdef REHLDS_FIXES
	Q_StripUnprintableAndSpace(newname);
#else // REHLDS_FIXES
	TrimSpace(newname, newname);
#endif // REHLDS_FIXES

	if (!Q_UnicodeValidate(newname))
	{
		Q_UnicodeRepair(newname);
	}

	if (newname[0] == '\0' || !Q_stricmp(newname, "console")
#ifdef REHLDS_FIXES
		|| Q_strstr(newname, "..") != NULL)
#else // REHLDS_FIXES
		)
#endif // REHLDS_FIXES
	{
		Info_SetValueForKey(userinfo, "name", "unnamed", MAX_INFO_STRING);
	}
	else if (Q_strcmp(val, newname))
	{
		Info_SetValueForKey(userinfo, "name", newname, MAX_INFO_STRING);
	}

	// Check for duplicate names
	SV_CheckForDuplicateNames(userinfo, TRUE, cl - g_psvs.clients);

	gEntityInterface.pfnClientUserInfoChanged(cl->edict, userinfo);

	val = Info_ValueForKey(userinfo, "name");
	Q_strncpy(cl->name, val, sizeof(cl->name) - 1);
	cl->name[sizeof(cl->name) - 1] = '\0';

	ISteamGameServer_BUpdateUserData(cl->network_userid.m_SteamID, cl->name, 0);

	val = Info_ValueForKey(userinfo, "rate");
	if (val[0] != 0)
	{
		i = Q_atoi(val);
		cl->netchan.rate = Q_clamp(float(i), MIN_RATE, MAX_RATE);
	}

	val = Info_ValueForKey(userinfo, "topcolor");
	if (val[0] != 0)
		cl->topcolor = Q_atoi(val);
	else
		Con_DPrintf("topcolor unchanged for %s\n", cl->name);

	val = Info_ValueForKey(userinfo, "bottomcolor");
	if (val[0] != 0)
		cl->bottomcolor = Q_atoi(val);
	else
		Con_DPrintf("bottomcolor unchanged for %s\n", cl->name);

	val = Info_ValueForKey(userinfo, "cl_updaterate");
	if (val[0] != 0)
	{
		i = Q_atoi(val);
		if (i >= 10)
			cl->next_messageinterval = 1.0 / i;
		else
			cl->next_messageinterval = 0.1;
	}

	val = Info_ValueForKey(userinfo, "cl_lw");
	cl->lw = val[0] != 0 ? Q_atoi(val) != 0 : 0;

	val = Info_ValueForKey(userinfo, "cl_lc");
	cl->lc = val[0] != 0 ? Q_atoi(val) != 0 : 0;

	val = Info_ValueForKey(userinfo, "*hltv");
	cl->proxy = val[0] != 0 ? Q_atoi(val) == TYPE_PROXY : 0;

	SV_CheckUpdateRate(&cl->next_messageinterval);
	SV_CheckRate(cl);
}

/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (const char *name)
{
	int		i;
	
	if (!name || !name[0])
		return 0;

	for (i=0 ; i<MAX_MODELS && sv.model_precache[i] ; i++)
		if (!strcmp(sv.model_precache[i], name))
			return i;
	
	if (i==MAX_MODELS || !sv.model_precache[i])
		Sys_Error ("SV_ModelIndex: model %s not precached", name);
	
	return i;
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline ()
{
	int			i;
	edict_t			*svent;
	int				entnum;	
		
	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

	//
	// create entity baseline
	//
		VectorCopy (svent->v.origin, svent->baseline.origin);
		VectorCopy (svent->v.angles, svent->baseline.angles);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(pr_strings + svent->v.model);
		}
		
	//
	// add to the message
	//
		MSG_WriteByte (&sv.signon,svc_spawnbaseline);		
		MSG_WriteShort (&sv.signon,entnum);

		MSG_WriteByte (&sv.signon, svent->baseline.modelindex);
		MSG_WriteByte (&sv.signon, svent->baseline.frame);
		MSG_WriteByte (&sv.signon, svent->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->baseline.skin);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
			MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect ()
{
	char	data[128];
	sizebuf_t	msg;

	msg.data = data;
	msg.cursize = 0;
	msg.maxsize = sizeof(data);

	MSG_WriteChar (&msg, svc_stufftext);
	MSG_WriteString (&msg, "reconnect\n");
	NET_SendToAll (&msg, 5);
	
	if (cls.state != ca_dedicated)
#ifdef QUAKE2
		Cbuf_InsertText ("reconnect\n");
#else
		Cmd_ExecuteString ("reconnect\n", src_command);
#endif
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms ()
{
	int		i, j;

	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float scr_centertime_off;

void SV_SpawnServer (char *server, char *startspot)
{
	edict_t		*ent;
	int			i;

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_Set ("hostname", "UNNAMED");
	scr_centertime_off = 0;

	Con_DPrintf ("SpawnServer: %s\n",server);
	svs.changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (sv.active)
		SV_SendReconnect ();

//
// make cvars consistant
//
	if (coop.value)
		Cvar_SetValue ("deathmatch", 0);
	
	current_skill = (int)(skill.value + 0.5);
	
	if (current_skill < 0)
		current_skill = 0;
	if (current_skill > 3)
		current_skill = 3;

	Cvar_SetValue ("skill", (float)current_skill);
	
//
// set up the new server
//
	Host_ClearMemory ();

	memset (&sv, 0, sizeof(sv));

	strcpy (sv.name, server);

	if (startspot)
		strcpy(sv.startspot, startspot);

// load progs to get entity field count
	PR_LoadProgs ();

// allocate server memory
	sv.max_edicts = MAX_EDICTS;
	
	sv.edicts = Hunk_AllocName (sv.max_edicts*pr_edict_size, "edicts");

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;
	
	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;
	
	sv.signon.maxsize = sizeof(sv.signon_buf);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buf;
	
// leave slots at start for clients only
	sv.num_edicts = svs.maxclients+1;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		ent = EDICT_NUM(i+1);
		svs.clients[i].edict = ent;
	}
	
	//sv.state = ss_loading;
	//sv.paused = false;

	sv.time = 1.0;
	
	strcpy (sv.name, server);
	sprintf (sv.modelname,"maps/%s.bsp", server);
	sv.worldmodel = Mod_ForName (sv.modelname, false);
	if (!sv.worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", sv.modelname);
		sv.active = false;
		return;
	}
	SV_CalcPHS ();
	
//
// clear world interaction links
//
	SV_ClearWorld ();
	
	sv.sound_precache[0] = pr_strings;

	sv.model_precache[0] = pr_strings;
	sv.model_precache[1] = sv.modelname;
	sv.models[1] = sv.worldmodel;
	for (i=1 ; i<sv.worldmodel->numsubmodels ; i++)
	{
		sv.model_precache[1+i] = localmodels[i];
		sv.models[i+1] = Mod_ForName (localmodels[i], false);
	}
	
	//check player/eyes models for hacks
	sv.model_player_checksum = SV_CheckModel("progs/player.mdl");
	sv.eyes_player_checksum = SV_CheckModel("progs/eyes.mdl");
	
	//
	// spawn the rest of the entities on the map
	//	

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;

//
// load the rest of the entities
//	
	ent = EDICT_NUM(0);
	memset (&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->v.model = PR_SetString(sv.worldmodel->name);
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (coop.value)
		pr_global_struct->coop = coop.value;
	else
		pr_global_struct->deathmatch = deathmatch.value;

	pr_global_struct->mapname = PR_SetString(sv.name);
	pr_global_struct->startspot = PR_SetString(sv.startspot);

// serverflags are for cross level information
	pr_global_struct->serverflags = svs.serverflags;
	
	// run the frame start qc function to let progs check cvars
	SV_ProgStartFrame ();
	
	// load and spawn all other entities
	ED_LoadFromFile (sv.worldmodel->entities);

	// look up some model indexes for specialized message compression
	SV_FindModelNumbers ();
	
	sv.active = true;

// all setup is completed, any further precache statements are errors
	sv.state = ss_active;
	
// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// save movement vars
	SV_SetMoveVars();
	
// create a baseline for more efficient communications
	SV_CreateBaseline ();

	sv.signon_buffer_size[sv.num_signon_buffers-1] = sv.signon.cursize;
	
// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_SendServerinfo (host_client);
	
	Info_SetValueForKey (svs.info, "map", sv.name, MAX_SERVERINFO_STRING);
	Con_DPrintf ("Server spawned.\n");
}

