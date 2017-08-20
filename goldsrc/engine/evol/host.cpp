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

// host.c -- coordinates spawning and killing of local servers

//#include "precompiled.h"
#include "quakedef.h"
#include "r_local.h"

/*

A server can always be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean	host_initialized = false;		// true if into command execution

double		host_frametime = 0;
double		host_time;
double		realtime = 0;				// without any filtering or bounding
double		oldrealtime = 0;			// last frame run
int			host_framecount;

double rolling_fps = 0;

int host_hunklevel = 0;

int minimum_memory = 0; // unused now?

client_t *host_client = NULL;			// current client

jmp_buf host_abortserver;
jmp_buf host_enddemo;

byte *host_basepal = NULL;
byte *host_colormap = NULL;

cvar_t console = { "console", "0.0", FCVAR_ARCHIVE };

static cvar_t host_profile = { "host_profile", "0" };

cvar_t fps_max = { "fps_max", "100.0", FCVAR_ARCHIVE };
cvar_t fps_override = { "fps_override", "0" };

cvar_t	host_framerate = {"host_framerate","0"};	// set for slow motion
cvar_t	host_speeds = {"host_speeds","0"};			// set for running times

cvar_t sys_ticrate = {"sys_ticrate","100.0"};
cvar_t sys_timescale = { "sys_timescale", "1.0" };

cvar_t	serverprofile = {"serverprofile","0"};

cvar_t	fraglimit = {"fraglimit","0",false,true};
cvar_t	timelimit = {"timelimit","0",false,true};
cvar_t	teamplay = {"teamplay","0",false,true};

cvar_t	developer = {"developer","0"}; // show extra messages

cvar_t	skill = {"skill","1"};						// 0 - 3
cvar_t	deathmatch = {"deathmatch","0"};			// 0, 1, or 2
cvar_t	coop = {"coop","0"};			// 0 or 1

cvar_t	pausable = {"pausable","1"};

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	vsprintf (string,message,argptr);
	va_end (argptr);
	
	//Con_Printf ("\n===========================\n");
	//Con_Printf ("Host_EndGame: %s\n",string);
	Con_DPrintf ("Host_EndGame: %s\n",string);
	//Con_Printf ("===========================\n\n");
	
	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit
	
	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;
	
	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;
	
	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);
	
	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp (host_abortserver, 1);
}

void CheckGore()
{
	char szBuffer[ 128 ];

	Q_memset( szBuffer, 0, sizeof( szBuffer ) );

	//TODO: needs Windows specific code - Solokiller

	if( bLowViolenceBuild )
	{
		Cvar_SetValue( "violence_hblood", 0 );
		Cvar_SetValue( "violence_hgibs", 0 );
		Cvar_SetValue( "violence_ablood", 0 );
		Cvar_SetValue( "violence_agibs", 0 );
	}
	else
	{
		Cvar_SetValue( "violence_hblood", 1 );
		Cvar_SetValue( "violence_hgibs", 1 );
		Cvar_SetValue( "violence_ablood", 1 );
		Cvar_SetValue( "violence_agibs", 1 );
	}
}

void Host_Version()
{
	Q_strcpy( gpszVersionString, "1.0.1.4" );
	Q_strcpy( gpszProductString, "valve" );

	char szFileName[ FILENAME_MAX ];

	strcpy( szFileName, "steam.inf" );

	FileHandle_t hFile = FS_Open( szFileName, "r" );

	if( hFile != FILESYSTEM_INVALID_HANDLE )
	{
		const int iSize = FS_Size( hFile );
		void* pFileData = Mem_Malloc( iSize + 1 );
		FS_Read( pFileData, iSize, hFile );
		FS_Close( hFile );

		char* pBuffer = reinterpret_cast<char*>( pFileData );

		pBuffer[ iSize ] = '\0';

		const int iProductNameLength = Q_strlen( "ProductName=" );
		const int iPatchVerLength = Q_strlen( "PatchVersion=" );

		char szSteamVersionId[ 32 ];

		//Parse out the version and name.
		for( int i = 0; ( pBuffer = COM_Parse( pBuffer ) ) != nullptr && *com_token && i < 2; )
		{
			if( !Q_strnicmp( com_token, "PatchVersion=", iPatchVerLength ) )
			{
				++i;

				Q_strncpy( gpszVersionString, &com_token[ iPatchVerLength ], ARRAYSIZE( gpszVersionString ) );

				if( COM_CheckParm( "-steam" ) )
				{
					FS_GetInterfaceVersion( szSteamVersionId, ARRAYSIZE( szSteamVersionId ) - 1 );
					snprintf( gpszVersionString, ARRAYSIZE( gpszVersionString ), "%s/%s", &com_token[ iPatchVerLength ], szSteamVersionId );
				}
			}
			else if( !Q_strnicmp( com_token, "ProductName=", iProductNameLength ) )
			{
				++i;
				Q_strncpy( gpszProductString, &com_token[ iProductNameLength ], ARRAYSIZE( gpszProductString ) );
			}
		}

		if( pFileData )
			Mem_Free( pFileData );
	}

	if( cls.state != ca_dedicated )
	{
		Con_DPrintf( "Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString );
		Con_DPrintf( "Exe build: " __TIME__ " " __DATE__ " (%i)\n", build_number() );
	}
	else
	{
		Con_Printf( "Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString );
		Con_Printf( "Exe build: " __TIME__ " " __DATE__ " (%i)\n", build_number() );
	}
}

/*
================
Host_FindMaxClients
================
*/
/*
void Host_FindMaxClients ()
{
	int		i;

	svs.maxclients = 1;
		
	i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = Q_atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = Q_atoi (com_argv[i+1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = Hunk_AllocName (svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0);
	else
		Cvar_SetValue ("deathmatch", 0.0);
}
*/

/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal ()
{
	Host_InitCommands ();

	//
	// register our commands
	//
	//Cvar_RegisterVariable (&serverprofile);

	//Cvar_RegisterVariable (&fraglimit);
	//Cvar_RegisterVariable (&timelimit);
	//Cvar_RegisterVariable (&teamplay);
	
	Cvar_RegisterVariable( &host_killtime );
	Cvar_RegisterVariable( &sys_ticrate );
	
	Cvar_RegisterVariable( &fps_max );
	Cvar_RegisterVariable( &fps_override );
	
	Cvar_RegisterVariable( &host_name );
	Cvar_RegisterVariable( &host_limitlocal );
	
	sys_timescale.value = 1.0f;
	
	Cvar_RegisterVariable( &host_framerate );
	Cvar_RegisterVariable( &host_speeds );
	Cvar_RegisterVariable( &host_profile );
	
	Cvar_RegisterVariable( &mp_logfile );
	Cvar_RegisterVariable( &mp_logecho );
	
	Cvar_RegisterVariable( &sv_log_onefile );
	Cvar_RegisterVariable( &sv_log_singleplayer );
	Cvar_RegisterVariable( &sv_logsecret );
	
	Cvar_RegisterVariable( &sv_stats );
	
	Cvar_RegisterVariable (&developer);
	
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&coop);
	
	Cvar_RegisterVariable (&pausable);
	Cvar_RegisterVariable (&skill);

	SV_SetMaxclients();
	//Host_FindMaxClients ();
	
	//host_time = 1.0;		// so a think at time 0 won't get called
}

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration()
{
	// dedicated servers initialize the host but don't parse and set the config.cfg cvars
	if(host_initialized & !isDedicated)
	{
		FileHandle_t f = FS_Open(va("%s/config.cfg", com_gamedir), "w");
		
		if(!f)
		{
			Con_Printf("Couldn't write config.cfg.\n");
			return;
		};
		
		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		FS_Close(f);
	};
};

/*
=================
SV_ClientPrintf

Sends text across to be displayed 
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,fmt);
	//Q_vsnprintf(string, ARRAYSIZE(string) - 1, fmt, argptr);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	//string[ARRAYSIZE(string) - 1] = 0;
	
	for (int i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient( client_t* cl, qboolean crash, const char* fmt, ... )
{
	int		saveSelf;
	int		i;
	client_t *client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}
	
		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int		i;
	int		count;
	sizebuf_t	buf;
	char		message[4];
	double	start;

	if (!sv.active)
		return;

	sv.active = false;

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_FloatTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_FloatTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

//
// clear structures
//
	memset (&sv, 0, sizeof(sv));
	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}

void SV_ClearClientStates()
{
	auto pClient = svs.clients;

	for( int i = 0; i < svs.maxclients; ++i, ++pClient )
	{
		COM_ClearCustomizationList( &pClient->customdata, false );
		SV_ClearResourceLists( pClient );
	}
}

//TODO: typo - Solokiller
void Host_CheckDyanmicStructures()
{
	auto pClient = svs.clients;

	// svs.maxclientslimit -> svs.maxclients?
	for( int i = 0; i < svs.maxclientslimit; ++i, ++pClient )
	{
		if( pClient->frames )
			SV_ClearFrames( &pClient->frames );
	}
}

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (qboolean bQuiet)
{
	//Engine string pooling
#ifdef REHLDS_FIXES
	Ed_StrPool_Reset();
#endif //REHLDS_FIXES

	CM_FreePAS();
	SV_ClearEntities();

	if( !bQuiet )
		Con_DPrintf ("Clearing memory\n");
	
	D_FlushCaches ();
	Mod_ClearAll ();
	
	if (host_hunklevel)
	{
		Host_CheckDyanmicStructures();
		
		Hunk_FreeToLowMark (host_hunklevel);
	};

	cls.signon = 0;
	
	SV_ClearCaches();
	
	Q_memset (&sv, 0, sizeof(sv)); // sizeof(server_t)
	
	CL_ClearClientState();

	SV_ClearClientStates();
};

//============================================================================

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime (float time)
{
	
	//if (!cls.timedemo && realtime - oldrealtime < 1.0/72.0)
	//	return false;		// framerate is too high

	if (host_framerate.value > 0)
	{
		if( ( sv.active && svs.maxclients == 1 ) || ( cl.maxclients == 1 ) || cls.demoplayback )
		{
			host_frametime = host_framerate.value * sys_timescale.value;
			realtime = host_frametime + realtime;
			return true;
		};
	};
	/*
	else
	{	// don't allow really long or short frames
		if (host_frametime > 0.1)
			host_frametime = 0.1;
		
		if (host_frametime < 0.001)
			host_frametime = 0.001;
	}
	*/
	
	realtime += time * sys_timescale.value;

	const double flDelta = realtime - oldrealtime;

	if( g_bIsDedicatedServer )
	{
		static int command_line_ticrate = -1;

		if( command_line_ticrate == -1 )
			command_line_ticrate = COM_CheckParm( "-sys_ticrate" );

		double flTicRate = sys_ticrate.value;

		if( command_line_ticrate > 0 )
		{
			flTicRate = strtod( com_argv[ command_line_ticrate + 1 ], nullptr );
		}

		if( flTicRate > 0.0 )
		{
			if( ( 1.0 / ( flTicRate + 1.0 ) ) > flDelta )
				return false;
		}
	}
	else
	{
		double flFPSMax;

		if( sv.active || cls.state == ca_disconnected || cls.state == ca_active )
		{
			flFPSMax = 0.5;
			if( fps_max.value >= 0.5 )
				flFPSMax = fps_max.value;
		}
		else
		{
			flFPSMax = 31.0;
		}

		if( !fps_override.value )
		{
			if( flFPSMax > 100.0 )
				flFPSMax = 100.0;
		}

		if( cl.maxclients > 1 )
		{
			if( flFPSMax < 20.0 )
				flFPSMax = 20.0;
		}

		if( gl_vsync.value )
		{
			if( !fps_override.value )
				flFPSMax = 100.0;
		}

		if( !cls.timedemo )
		{
			if( sys_timescale.value / ( flFPSMax + 0.5 ) > flDelta )
				return false;
		}
	}
	
	host_frametime = flDelta;
	oldrealtime = realtime;

	if( flDelta > 0.25 )
		host_frametime = 0.25;

	return true;
};

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands ()
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}


/*
==================
Host_ServerFrame

==================
*/
#ifdef FPS_20

void _Host_ServerFrame ()
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// read client messages
	SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();
}

void Host_ServerFrame ()
{
	float	save_host_frametime;
	float	temp_host_frametime;

// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();
	
// check for new clients
	SV_CheckForNewClients ();

	temp_host_frametime = save_host_frametime = host_frametime;
	while(temp_host_frametime > (1.0/72.0))
	{
		if (temp_host_frametime > 0.05)
			host_frametime = 0.05;
		else
			host_frametime = temp_host_frametime;
		temp_host_frametime -= host_frametime;
		_Host_ServerFrame ();
	}
	host_frametime = save_host_frametime;

// send all messages to the clients
	SV_SendClientMessages ();
}

#else

void Host_ServerFrame ()
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();
	
// check for new clients
	SV_CheckForNewClients ();

// read client messages
	SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();

// send all messages to the clients
	SV_SendClientMessages ();
}

#endif

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (float time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;
	int pass1, pass2, pass3;

	//if (setjmp (host_abortserver) )
		//return;			// something bad happened, or the server disconnected

	// keep the random time dependent
	//rand ();
	
	if( setjmp( host_enddemo ) || !Host_FilterTime( time ) )
		return;
	
	// decide the simulation time
	//if(!Host_FilterTime (time))
		//return;			// don't run too fast, or packets will flood out
	
	SystemWrapper_RunFrame( host_frametime );

	if( g_modfuncs.m_pfnFrameBegin )
		g_modfuncs.m_pfnFrameBegin();

	rolling_fps = 0.6 + rolling_fps + 0.4 * host_frametime;
	
	// get new key events
	Sys_SendKeyEvents();

	// allow mice or other external controllers to add commands
	IN_Commands();

	// process console commands
	Cbuf_Execute();

	//NET_Poll();

	// if running the server locally, make intentions now
	//if (sv.active)
		//CL_SendCmd ();
	
//-------------------
//
// server operations
//
//-------------------

	// check for commands typed to the host
	//Host_GetConsoleCommands ();
	
	//if (sv.active)
		//Host_ServerFrame ();

//-------------------
//
// client operations
//
//-------------------

	// fetch results from server
	CL_ReadPackets();
	
	// if running the server remotely, send intentions now after
	// the incoming messages have been read
	//if (!sv.active)
		//CL_SendCmd ();

	//host_time += host_frametime;

	// fetch results from server
	//if (cls.state == ca_connected)
		//CL_ReadFromServer ();
	
	// Set up prediction for other players
	CL_SetUpPlayerPrediction(false);

	// do client side motion prediction
	CL_PredictMove ();

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(true);

	// build a refresh entity list
	CL_EmitEntities ();

	// update video
	if(host_speeds.value)
		time1 = Sys_FloatTime ();
	
	if(!gfBackground )
	{
		SCR_UpdateScreen();
	};
	
	if(host_speeds.value)
		time2 = Sys_FloatTime ();
		
	// update audio
	if(cls.signon == SIGNONS)
	//if(cls.state == ca_active)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	
	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_FloatTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}
	
	host_framecount++;
	//fps_count++;
};

int Host_Frame (float time, int iState, int* stateInfo)
{
	if( setjmp( host_abortserver ) )
		return giActive;
	
	if( giActive != DLL_CLOSE || !g_iQuitCommandIssued )
		giActive = iState;
	
	*stateInfo = 0;

	double time1, time2;
	
	if( host_profile.value )
		time1 = Sys_FloatTime();
	
	_Host_Frame (time);
	
	if( host_profile.value )
		time2 = Sys_FloatTime();
	
	if( giStateInfo )
	{
		*stateInfo = giStateInfo;
		giStateInfo = 0;
		Cbuf_Execute();
	};
	
	if( host_profile.value )
	{
		static double timetotal = 0;
		static int timecount = 0;
		
		timetotal += time2 - time1;
		++timecount;
		
		// Print status every 1000 frames
		if (timecount < 1000)
			return;

		int iActiveClients = 0;
		
		for (int i = 0; i < svs.maxclients; ++i)
			if (svs.clients[i].active)
				++iActiveClients;

		Con_Printf ("host_profile: %2i clients %2i msec\n", iActiveClients, timetotal * 1000 / timecount);
		
		timecount = 0;
		timetotal = 0;
	};
	
	return giActive;
}

qboolean Host_IsServerActive()
{
	return sv.active;
}

qboolean Host_IsSinglePlayerGame()
{
	if( sv.active )
		return svs.maxclients == 1;
	else
		return cl.maxclients == 1;
}

void Host_GetHostInfo( float* fps, int* nActive, int* unused, int* nMaxPlayers, char* pszMap )
{
	int clients = 0;

	if( rolling_fps > 0.0 )
	{
		*fps = 1.0 / rolling_fps;
	}
	else
	{
		rolling_fps = 0.0;
		*fps = rolling_fps;
	}

	SV_CountPlayers( &clients );
	*nActive = clients;

	if( unused )
		*unused = 0;

	if( pszMap )
	{
		if( sv.name[ 0 ] )
			Q_strcpy( pszMap, sv.name );
		else
			*pszMap = '\0';
	}

	*nMaxPlayers = svs.maxclients;
}

//============================================================================

extern int vcrFile;
#define	VCR_SIGNATURE	0x56435231
// "VCR1"

void Host_InitVCR (quakeparms_t *parms)
{
	int		i, len, n;
	char	*p;
	
	if (COM_CheckParm("-playback"))
	{
		if (com_argc != 2)
			Sys_Error("No other parameters allowed with -playback\n");

		Sys_FileOpenRead("quake.vcr", &vcrFile);
		if (vcrFile == -1)
			Sys_Error("playback file not found\n");

		Sys_FileRead (vcrFile, &i, sizeof(int));
		if (i != VCR_SIGNATURE)
			Sys_Error("Invalid signature in vcr file\n");

		Sys_FileRead (vcrFile, &com_argc, sizeof(int));
		com_argv = malloc(com_argc * sizeof(char *));
		com_argv[0] = parms->argv[0];
		for (i = 0; i < com_argc; i++)
		{
			Sys_FileRead (vcrFile, &len, sizeof(int));
			p = malloc(len);
			Sys_FileRead (vcrFile, p, len);
			com_argv[i+1] = p;
		}
		com_argc++; /* add one for arg[0] */
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ( (n = COM_CheckParm("-record")) != 0)
	{
		vcrFile = Sys_FileOpenWrite("quake.vcr");

		i = VCR_SIGNATURE;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		i = com_argc - 1;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				Sys_FileWrite(vcrFile, &len, sizeof(int));
				Sys_FileWrite(vcrFile, "-playback", len);
				continue;
			}
			len = Q_strlen(com_argv[i]) + 1;
			Sys_FileWrite(vcrFile, &len, sizeof(int));
			Sys_FileWrite(vcrFile, com_argv[i], len);
		}
	}
	
}

/*
====================
Host_Init
====================
*/
qboolean Host_Init (quakeparms_t *parms)
{
	srand( time( nullptr ) );

	//if (COM_CheckParm ("-minmemory"))
		//parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	//if (parms->memsize < MINIMUM_MEMORY)
		//Sys_Error ("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

	realtime = 0;
	
	com_argc = parms->argc;
	com_argv = parms->argv;

	Memory_Init (parms->membase, parms->memsize);
	
	Voice_RegisterCvars();
	Cvar_RegisterVariable( &console );

	if( COM_CheckParm( "-console" ) || COM_CheckParm( "-toconsole" ) || COM_CheckParm( "-dev" ) )
		Cvar_DirectSet( &console, "1.0" );
	
	Host_InitLocal ();
	
	if( COM_CheckParm( "-dev" ) )
		Cvar_SetValue( "developer", 1.0 );
	
	Cbuf_Init ();
	Cmd_Init ();
	
	Cvar_Init();
	Cvar_CmdInit();
	
	V_Init ();
	Chase_Init ();
	
	//Host_InitVCR (parms);
	
	COM_Init (parms->basedir);
	
	Host_ClearSaveDirectory();
	HPAK_Init();
	
	W_LoadWadFile ("gfx.wad");
	W_LoadWadFile( "fonts.wad" );
	
	Key_Init ();
	Con_Init ();	
	//M_Init ();	
	//PR_Init ();
	Decal_Init();
	Mod_Init ();
	
	NET_Init ();
	Netchan_Init();
	DELTA_Init();
	
	SV_Init ();
	
	SystemWrapper_Init();
	
	Host_Version();

	char versionString[ 256 ];
	snprintf( versionString, ARRAYSIZE( versionString ), "%s,%i,%i", gpszVersionString, PROTOCOL_VERSION, build_number() );

	Cvar_Set( "sv_version", versionString );

	//Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	//Con_Printf ("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));
	//Con_Printf ("%4.1f megs RAM used.\n",parms->memsize/ (1024*1024.0));
	Con_DPrintf( "%4.1f Mb heap\n", parms->memsize / (1024 * 1024.0 ) );

	R_InitTextures ();		// needed even for dedicated servers
 	HPAK_CheckIntegrity( "custom" );

	Q_memset( &g_module, 0, sizeof( g_module ) );

	if (cls.state != ca_dedicated)
	{
		host_basepal = (byte *)COM_LoadHunkFile ("gfx/palette.lmp");
		
		if (!host_basepal)
			Sys_Error ("Host_Init: Couldn't load gfx/palette.lmp");
		
		host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp");
		
		if (!host_colormap)
			Sys_Error ("Couldn't load gfx/colormap.lmp");

		GL_Init();
		PM_Init( &g_clmove );
		CL_InitEventSystem();
		ClientDLL_Init();
		VGui_Startup();

		if( !VID_Init( host_basepal ) )
		{
			VGui_Shutdown();
			return false;
		}
		
#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
		IN_Init ();
#endif
		VID_Init (host_basepal);

		Draw_Init ();
		SCR_Init ();
		R_Init ();
#ifndef	_WIN32
	// on Win32, sound initialization has to come before video initialization, so we
	// can put up a popup if the sound hardware is in use
		S_Init ();
#else

#ifdef	GLQUAKE
	// FIXME: doesn't use the new one-window approach yet
		S_Init ();
#endif

#endif	// _WIN32

		//cls.state = ca_disconnected;
		
		CDAudio_Init ();
		Voice_Init( "voice_speex", 1 );
		DemoPlayer_Init();
		//Sbar_Init ();
		CL_Init ();
		
#ifdef _WIN32 // on non win32, mouse comes before video for security reasons
		IN_Init ();
#endif
	}
	else
		Cvar_RegisterVariable( &suitvolume );

	Cbuf_InsertText ("exec valve.rc\n");
	
	if( cls.state != ca_dedicated )
		GL_Config();

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();
	
	giActive = DLL_ACTIVE;
	scr_skipupdate = false;

	CheckGore();

	host_initialized = true;
	return true;
}

/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown()
{
	static qboolean isdown = false;
	
	if (isdown)
	{
		printf ("recursive shutdown\n"); // puts
		return;
	}
	
	isdown = true;

	// keep Con_Printf from trying to update the screen
	//scr_disabled_for_loading = true;

	if( host_initialized )
		Host_WriteConfiguration (); 
	
	SV_ServerShutdown();
	Voice_Deinit();
	
	host_initialized = false;

	CDAudio_Shutdown ();
	VGui_Shutdown();
	
	if( cls.state != ca_dedicated )
		ClientDLL_Shutdown();
	
	Cmd_RemoveGameCmds();
	Cmd_Shutdown();
	Cvar_Shutdown();
	
	HPAK_FlushHostQueue();
	SV_DeallocateDynamicData();
	
	for( int i = 0; i < svs.maxclientslimit; ++i )
		SV_ClearFrames( &svs.clients[ i ].frames );

	SV_Shutdown();
	SystemWrapper_ShutDown();
	
	NET_Shutdown();
	S_Shutdown();
	IN_Shutdown(); // ?
	
	Con_Shutdown();
	
	//if(cls.state != ca_dedicated) // if(host_basepal)
		//VID_Shutdown();
	
	ReleaseEntityDlls();

	CL_ShutDownClientStatic();
	CM_FreePAS();
	
	if( wadpath )
	{
		Mem_Free( wadpath );
		wadpath = nullptr;
	};

	if( cls.state != ca_dedicated )
		Draw_Shutdown();

	Draw_DecalShutdown();

	W_Shutdown();
	
	Log_Printf( "Server shutdown\n" );
	Log_Close();

	COM_Shutdown();
	CL_Shutdown();
	DELTA_Shutdown();
	
	Key_Shutdown();

	realtime = 0;

	sv.time = 0;
	cl.time = 0;
}