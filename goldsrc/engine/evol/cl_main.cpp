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
// cl_main.c  -- client main loop

#include "quakedef.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

cvar_t	rcon_password = {"rcon_password", "", false};
cvar_t	rcon_address = {"rcon_address", ""};

//cvar_t	entlatency = {"entlatency", "20"};
//cvar_t	cl_predict_players = {"cl_predict_players", "1"};
//cvar_t	cl_predict_players2 = {"cl_predict_players2", "1"};
cvar_t	cl_solid_players = {"cl_solid_players", "1"};

//
// info mirrors
//
cvar_t	password = {"password", "", false, true};
//cvar_t	spectator = {"spectator", "", false, true};
cvar_t	cl_name = {"name", "unnamed", true};
//cvar_t	name = {"name","unnamed", true, true};
cvar_t	team = {"team","", true, true};
cvar_t	skin = {"skin","", true, true};
cvar_t	topcolor = {"topcolor","0", true, true};
cvar_t	bottomcolor = {"bottomcolor","0", true, true};
cvar_t	rate = {"rate","2500", true, true};

cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2
//cvar_t	cl_nolerp = {"cl_nolerp","0"};
cvar_t	cl_nolerp = {"cl_nosmooth","0"};

cvar_t	cl_timeout = {"cl_timeout", "60"};

cvar_t	lookspring = {"lookspring","0", true};
cvar_t	lookstrafe = {"lookstrafe","0", true};
cvar_t	sensitivity = {"sensitivity","3", true};

cvar_t	m_pitch = {"m_pitch","0.022", true};
cvar_t	m_yaw = {"m_yaw","0.022", true};
cvar_t	m_forward = {"m_forward",``"1", true};
cvar_t	m_side = {"m_side","0.8", true};

cvar_t	show_fps = {"cl_showfps","0"}; // set for running times

client_static_t	cls;
client_state_t	cl;

// TODO, allocate dynamically
// FIXME: put these on hunk?
//entity_state_t cl_baselines[MAX_EDICTS] = {};
efrag_t cl_efrags[MAX_EFRAGS] = {};
cl_entity_t cl_entities[MAX_EDICTS] = {}; // instead of cl_baselines
//cl_entity_t cl_static_entities[MAX_STATIC_ENTITIES] = {}; // unused?
lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES] = {};
dlight_t cl_dlights[MAX_DLIGHTS] = {};
dlight_t cl_elights[ MAX_ELIGHTS ] = {};

int cl_numvisedicts = 0;
cl_entity_t *cl_visedicts[MAX_VISEDICTS] = {};

// TODO: implement API and add here - Solokiller
playermove_t g_clmove;

float server_version = 0;	// version of server we connected to

void CL_BeginServerConnect()
{
	cls.connect_time = 0;
	CL_CheckForResend();
}

/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
void CL_SendConnectPacket ()
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;
// JACK: Fixed bug where DNS lookups would cause two connects real fast
//       Now, adds lookup time to the connect time.
//		 Should I add it to realtime instead?!?!

	if (cls.state != ca_disconnected)
		return;

	t1 = Sys_FloatTime ();

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		cls.connect_time = -1;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (27500);
	t2 = Sys_FloatTime ();

	cls.connect_time = realtime+t2-t1;	// for retransmit requests

	cls.qport = Cvar_VariableValue("qport");

	Info_SetValueForStarKey (cls.userinfo, "*ip", NET_AdrToString(adr), MAX_INFO_STRING);

//	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf (data, "%c%c%c%cconnect %i %i %i \"%s\"\n",
		255, 255, 255, 255,	PROTOCOL_VERSION, cls.qport, cls.challenge, cls.userinfo);
	NET_SendPacket (strlen(data), data, adr);
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out

=================
*/
void CL_CheckForResend ()
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;

	if (cls.connect_time == -1)
		return;
	if (cls.state != ca_disconnected)
		return;
	if (cls.connect_time && realtime - cls.connect_time < 5.0)
		return;

	t1 = Sys_FloatTime ();
	
	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		cls.connect_time = -1;
		return;
	};

	if (adr.port == 0)
		adr.port = BigShort (27500);
	t2 = Sys_FloatTime ();

	cls.connect_time = realtime+t2-t1;	// for retransmit requests

	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf (data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);
	NET_SendPacket (strlen(data), data, adr);
}

void CL_CreateResourceList()
{
	if( cls.state != ca_dedicated )
	{
		HPAK_FlushHostQueue();

		cl.num_resources = 0;

		char szFileName[ MAX_PATH ];
		snprintf( szFileName, ARRAYSIZE( szFileName ), "tempdecal.wad" );

		byte rgucMD5_hash[ 16 ];
		Q_memset( rgucMD5_hash, 0, sizeof( rgucMD5_hash ) );

		auto hFile = FS_Open( szFileName, "rb" );

		if( FILESYSTEM_INVALID_HANDLE != hFile )
		{
			const auto uiSize = FS_Size( hFile );

			MD5_Hash_File( rgucMD5_hash, szFileName, false, false, nullptr );

			if( uiSize )
			{
				if( cl.num_resources > 1279 )
					Sys_Error( "Too many resources on client." );

				auto pResource = &cl.resourcelist[ cl.num_resources ];

				++cl.num_resources;

				Q_strncpy( pResource->szFileName, szFileName, ARRAYSIZE( pResource->szFileName ) );
				pResource->type = t_decal;
				pResource->nDownloadSize = uiSize;
				pResource->nIndex = 0;
				pResource->ucFlags |= RES_CUSTOM;

				Q_memcpy( pResource->rgucMD5_hash, rgucMD5_hash, sizeof( rgucMD5_hash ) );

				HPAK_AddLump( false, "custom.hpk", pResource, nullptr, hFile );
			}

			FS_Close( hFile );
		}
	}
}

void CL_ClearCaches()
{
	for( int i = 1; i < ARRAYSIZE( cl.event_precache ) && cl.event_precache[ i ].pszScript; ++i )
	{
		Mem_Free( const_cast<char*>( cl.event_precache[ i ].pszScript ) );
		Mem_Free( const_cast<char*>( cl.event_precache[ i ].filename ) );

		Q_memset( &cl.event_precache[ i ], 0, sizeof( cl.event_precache[ i ] ) );
	}
}

void CL_ClearClientState()
{
	for( int i = 0; i < CL_UPDATE_BACKUP; ++i )
	{
		if( cl.frames[ i ].packet_entities.entities )
		{
			Mem_Free( cl.frames[ i ].packet_entities.entities );
		}

		cl.frames[ i ].packet_entities.entities = nullptr;
		cl.frames[ i ].packet_entities.num_entities = 0;
	}

	CL_ClearResourceLists();

	for( int i = 0; i < MAX_CLIENTS; ++i )
		COM_ClearCustomizationList( &cl.players[ i ].customdata, false );

	CL_ClearCaches();

	// wipe the entire cl structure
	Q_memset( &cl, 0, sizeof( cl ) );

	cl.resourcesneeded.pPrev = &cl.resourcesneeded;
	cl.resourcesneeded.pNext = &cl.resourcesneeded;
	cl.resourcesonhand.pPrev = &cl.resourcesonhand;
	cl.resourcesonhand.pNext = &cl.resourcesonhand;

	CL_CreateResourceList();
}

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (qboolean bQuiet)
{
	if(!Host_IsServerActive()) // !sv.active
		Host_ClearMemory (bQuiet);

	CL_ClearClientState();

	SZ_Clear (&cls.netchan.message);

	// clear other arrays	
	Q_memset (cl_efrags, 0, sizeof(cl_efrags));
	//Q_memset (cl_entities, 0, sizeof(cl_entities));
	Q_memset (cl_dlights, 0, sizeof(cl_dlights));
	Q_memset( cl_elights, 0, sizeof( cl_elights ) );
	Q_memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	//Q_memset (cl_temp_entities, 0, sizeof(cl_temp_entities));
	//Q_memset (cl_beams, 0, sizeof(cl_beams));

	CL_TempEntInit();

	//
	// allocate the efrags and chain together into a free list
	//
	cl.free_efrags = cl_efrags;
	
	int i = 0;
	
	for(i = 0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;
}

/*
=====================
CL_Connect_f

User command to connect to server
=====================
*/
void CL_Connect_f ()
{
	//char name[MAX_QPATH];
	
	if(Cmd_Argc() != 2)
	{
		Con_Printf ("usage: connect <server>\n");
		return;	
	}
	
	cls.demonum = -1; // stop demo loop in case this fails
	
	if (cls.demoplayback)
	{
		CL_StopPlayback ();
		CL_Disconnect ();
	}
	
	// old NetQuake behavior
	//strcpy (name, Cmd_Argv(1));
	//CL_EstablishConnection (name);
	//Host_Reconnect_f ();
	
	char *server = Cmd_Argv(1);
	
	//CL_Disconnect ();
	
	strncpy (cls.servername, server, sizeof(cls.servername)-1);
	CL_BeginServerConnect();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect ()
{
	//cls.connect_time = -1;
	
// stop sounds (especially looping!)
	S_StopAllSounds (true);
	
// bring the console down and fade the colors back to normal
//	SCR_BringDownConsole ();

// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected) // != ca_disconnected
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;
		
		if (sv.active)
			Host_ShutdownServer(false);
	};
	
	Cam_Reset();

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
}

void CL_Disconnect_f ()
{
	CL_Disconnect();
	
	if (sv.active) // duplicated?
		Host_ShutdownServer (false);
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f ()
{
	char	message[1024];
	int		i;
	netadr_t	to;

	if (!rcon_password.string)
	{
		Con_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255;
	message[3] = 255;
	message[4] = 0;

	strcat (message, "rcon ");

	strcat (message, rcon_password.string);
	strcat (message, " ");

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		strcat (message, Cmd_Argv(i));
		strcat (message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_Printf ("You must either be connected,\n"
						"or set the 'rcon_address' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address.string, &to);
	}
	
	NET_SendPacket (strlen(message)+1, message
		, to);
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f ()
{
	int		uid;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == uid
		|| !strcmp(cl.players[i].name, Cmd_Argv(1)) )
		{
			Info_Print (cl.players[i].userinfo);
			return;
		}
	}
	Con_Printf ("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f ()
{
	int		i;
	int		c;

	c = 0;
	Con_Printf ("userid frags name\n");
	Con_Printf ("------ ----- ----\n");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_Printf ("%6i %4i %s\n", cl.players[i].userid, cl.players[i].frags, cl.players[i].name);
			c++;
		}
	}

	Con_Printf ("%i total users\n", c);
}

void CL_Color_f ()
{
	char	num[16];

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%s %s\"\n",
			Info_ValueForKey (cls.userinfo, "topcolor"),
			Info_ValueForKey (cls.userinfo, "bottomcolor") );
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}
	
	// QW
	
	sprintf (num, "%i", top);
	Cvar_Set ("topcolor", num);
	sprintf (num, "%i", bottom);
	Cvar_Set ("bottomcolor", num);
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
void CL_FullServerinfo_f ()
{
	char *p;
	float v;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: fullserverinfo <complete info string>\n");
		return;
	}

	strcpy (cl.serverinfo, Cmd_Argv(1));

	if ((p = Info_ValueForKey(cl.serverinfo, "*vesion")) && *p) {
		v = Q_atof(p);
		if (v) {
			if (!server_version)
				Con_Printf("Version %1.2f Server\n", v);
			server_version = v;
		}
	}
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
void CL_FullInfo_f ()
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (!stricmp(key, pmodel_name) || !stricmp(key, emodel_name))
			continue;

		Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING);
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f ()
{
	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!stricmp(Cmd_Argv(1), pmodel_name) || !strcmp(Cmd_Argv(1), emodel_name))
		return;

	Info_SetValueForKey (cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
	if (cls.state >= ca_connected)
		Cmd_ForwardToServer ();
}

/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (const char *host) // REMOVE?
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon)
		Host_Error ("CL_Connect: connect failed\n");
	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);
	
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply () // REMOVE?
{
	char 	str[8192];

	Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		break;
		
	case 2:		
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("name \"%s\"\n", cl_name.string));
	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("color %i %i\n", ((int)cl_color.value)>>4, ((int)cl_color.value)&15));
	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		sprintf (str, "spawn %s", cls.spawnparms);
		MSG_WriteString (&cls.message, str);
		break;
		
	case 3:	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory
		break;
		
	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo ()
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f ()
{
	cl_entity_t	*ent;
	int			i;
	
	for (i=0,ent=cl_entities ; i<cl.num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->model)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,ent->model->name,ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}

/*
===============
SetPal

Debugging tool, just flashes the screen
===============
*/
void SetPal (int i)
{
	// Unused by both Quake and GS
	
#if 0
	static int old;
	byte	pal[768];
	int		c;
	
	if (i == old)
		return;
	old = i;

	if (i==0)
		VID_SetPalette (host_basepal);
	else if (i==1)
	{
		for (c=0 ; c<768 ; c+=3)
		{
			pal[c] = 0;
			pal[c+1] = 255;
			pal[c+2] = 0;
		}
		VID_SetPalette (pal);
	}
	else
	{
		for (c=0 ; c<768 ; c+=3)
		{
			pal[c] = 0;
			pal[c+1] = 0;
			pal[c+2] = 255;
		}
		VID_SetPalette (pal);
	}
#endif
}

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				Q_memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			Q_memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	Q_memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

dlight_t* CL_AllocElight( int key )
{
	// TODO: implement - Solokiller
	return nullptr;
}

/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights ()
{
	int			i;
	dlight_t	*dl;
	float		time;
	
	time = cl.time - cl.oldtime;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		dl->radius -= time*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	CL_LerpPoint ()
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];
	
	if (!f || cl_nolerp.value || cls.timedemo || sv.active)
	{
		cl.time = cl.mtime[0];
		return 1;
	}
		
	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.time - cl.mtime[1]) / f;
//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
SetPal(1);
			cl.time = cl.mtime[1];
//				Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
SetPal(2);
			cl.time = cl.mtime[0];
//				Con_Printf ("high frac\n");
		}
		frac = 1;
	}
	else
		SetPal(0);
		
	return frac;
}


/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities ()
{
	cl_entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	float		bobjrotate;
	vec3_t		oldorg;
	dlight_t	*dl;

// determine partial update time	
	frac = CL_LerpPoint ();

	cl_numvisedicts = 0;

//
// interpolate player info
//
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + 
			frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

	if (cls.demoplayback)
	{
	// interpolate the angles	
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	
	bobjrotate = anglemod(100*cl.time);
	
// start on the entity after the world
	for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
	{
		if (!ent->model)
		{	// empty slot
			if (ent->forcelink)
				R_RemoveEfrags (ent);	// just became empty
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0])
		{
			ent->model = NULL;
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink)
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
				if (delta[j] > 100 || delta[j] < -100)
					f = 1;		// assume a teleportation, not a motion
			}

		// interpolate the origin and angles
			for (j=0 ; j<3 ; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}
			
		}

// rotate binary objects locally
		if (ent->model->flags & EF_ROTATE)
			ent->angles[1] = bobjrotate;

		if (ent->effects & EF_BRIGHTFIELD)
			R_EntityParticles (ent);
#ifdef QUAKE2
		if (ent->effects & EF_DARKFIELD)
			R_DarkFieldParticles (ent);
#endif
		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;

			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			AngleVectors (ent->angles, fv, rv, uv);
			 
			VectorMA (dl->origin, 18, fv, dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->minlight = 32;
			dl->die = cl.time + 0.1;
		}
		if (ent->effects & EF_BRIGHTLIGHT)
		{			
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			dl->radius = 400 + (rand()&31);
			dl->die = cl.time + 0.001;
		}
		if (ent->effects & EF_DIMLIGHT)
		{			
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->die = cl.time + 0.001;
		}
#ifdef QUAKE2
		if (ent->effects & EF_DARKLIGHT)
		{			
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200.0 + (rand()&31);
			dl->die = cl.time + 0.001;
			dl->dark = true;
		}
		if (ent->effects & EF_LIGHT)
		{			
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.001;
		}
#endif

		if (ent->model->flags & EF_GIB)
			R_RocketTrail (oldorg, ent->origin, 2);
		else if (ent->model->flags & EF_ZOMGIB)
			R_RocketTrail (oldorg, ent->origin, 4);
		else if (ent->model->flags & EF_TRACER)
			R_RocketTrail (oldorg, ent->origin, 3);
		else if (ent->model->flags & EF_TRACER2)
			R_RocketTrail (oldorg, ent->origin, 5);
		else if (ent->model->flags & EF_ROCKET)
		{
			R_RocketTrail (oldorg, ent->origin, 0);
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.01;
		}
		else if (ent->model->flags & EF_GRENADE)
			R_RocketTrail (oldorg, ent->origin, 1);
		else if (ent->model->flags & EF_TRACER3)
			R_RocketTrail (oldorg, ent->origin, 6);

		ent->forcelink = false;

		if (i == cl.viewentity && !chase_active.value)
			continue;

#ifdef QUAKE2
		if ( ent->effects & EF_NODRAW )
			continue;
#endif
		if (cl_numvisedicts < MAX_VISEDICTS)
		{
			cl_visedicts[cl_numvisedicts] = ent;
			cl_numvisedicts++;
		}
	}

}

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets ()
{
//	while (NET_GetPacket ())
	while (CL_GetMessage())
	{
		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (net_message.cursize < 8)
		{
			Con_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!cls.demoplayback && 
			!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Con_DPrintf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&cls.netchan))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage ();

//		if (cls.demoplayback && cls.state >= ca_active && !CL_DemoBehind())
//			return;
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && realtime - cls.netchan.last_received > cl_timeout.value)
	{
		Con_Printf ("\nServer connection timed out.\n");
		CL_Disconnect ();
		return;
	}
	
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer () // REMOVE?
{
	int		ret;

	cl.oldtime = cl.time;
	cl.time += host_frametime;
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);
	
	if (cl_shownet.value)
		Con_Printf ("\n");

	CL_RelinkEntities ();
	CL_UpdateTEnts ();

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd () // REMOVE?
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);
	
	// allow mice or other external controllers to add to the move
		IN_Move (&cmd);
	
	// send the unreliable message
		CL_SendMove (&cmd);
	
	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}
	
// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all
	
	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	SZ_Clear (&cls.message);
}

/*
=================
CL_Init
=================
*/
void CL_Init ()
{
	cls.state = ca_disconnected;
	
	SZ_Alloc(&cls.message, 1024); // cls.netchan.message? I think it should be removed now
	
	/*
	char st[80];

	Info_SetValueForKey (cls.userinfo, "name", "unnamed", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "topcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "rate", "2500", MAX_INFO_STRING);
	sprintf (st, "%4.2f-%04d", VERSION, build_number());
	Info_SetValueForStarKey (cls.userinfo, "*ver", st, MAX_INFO_STRING);
	*/

	CL_InitInput ();
	CL_InitTEnts ();
	//CL_InitPrediction ();
	//CL_InitCam ();
	//Pmove_Init ();
	
	TextMessageInit();
	
	//
	// register our variables
	//
	Cvar_RegisterVariable (&show_fps);

	Cvar_RegisterVariable (&cl_warncmd);
	
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_sbar);
	Cvar_RegisterVariable (&cl_hudswap);
	Cvar_RegisterVariable (&cl_maxfps);
	Cvar_RegisterVariable (&cl_timeout);

	Cvar_RegisterVariable (&rcon_password);
	Cvar_RegisterVariable (&rcon_address);

	Cvar_RegisterVariable (&entlatency);
	Cvar_RegisterVariable (&cl_predict_players2);
	Cvar_RegisterVariable (&cl_predict_players);
	Cvar_RegisterVariable (&cl_solid_players);

	Cvar_RegisterVariable (&localid);
	
	//
	// info mirrors
	//
	Cvar_RegisterVariable (&cl_name); // name
	Cvar_RegisterVariable (&password);
	Cvar_RegisterVariable (&spectator);
	Cvar_RegisterVariable (&skin);
	Cvar_RegisterVariable (&team);
	Cvar_RegisterVariable (&topcolor);
	Cvar_RegisterVariable (&bottomcolor);
	Cvar_RegisterVariable (&rate);

/*
	Cvar_RegisterVariable (&cl_upspeed);
	Cvar_RegisterVariable (&cl_forwardspeed);
	Cvar_RegisterVariable (&cl_backspeed);
	Cvar_RegisterVariable (&cl_sidespeed);
	
	Cvar_RegisterVariable (&cl_movespeedkey);
	
	Cvar_RegisterVariable (&cl_yawspeed);
	Cvar_RegisterVariable (&cl_pitchspeed);
	
	Cvar_RegisterVariable (&cl_anglespeedkey);
*/
	
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_nolerp);

/*	
	Cvar_RegisterVariable (&lookspring);
	Cvar_RegisterVariable (&lookstrafe);
	Cvar_RegisterVariable (&sensitivity);

	Cvar_RegisterVariable (&m_pitch);
	Cvar_RegisterVariable (&m_yaw);
	Cvar_RegisterVariable (&m_forward);
	Cvar_RegisterVariable (&m_side);
*/
	
	//
	// register our commands
	//
	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	
	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
	
	Cmd_AddCommand ("rcon", CL_Rcon_f);
	
	Cmd_AddCommand ("user", CL_User_f);
	Cmd_AddCommand ("users", CL_Users_f);
	
	Cmd_AddCommand ("setinfo", CL_SetInfo_f);
	Cmd_AddCommand ("fullinfo", CL_FullInfo_f);
	Cmd_AddCommand ("fullserverinfo", CL_FullServerinfo_f);
	
	//Cmd_AddCommand("upload", CL_Upload_f);

	Cmd_AddCommand ("serverinfo", NULL);
};

void CL_Shutdown()
{
	TextMessageShutdown();
};

void CL_ShutDownUsrMessages()
{
};

void CL_ShutDownClientStatic()
{
	//TODO: implement - Solokiller
}