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
// console.c

#ifdef NeXT
#include <libc.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

/*
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>
*/

#include <fcntl.h>

#ifdef WIN32
//#include <io.h>
#endif

#include "quakedef.h"
/*
#include "client.h"
#include "gl_draw.h"
#include "gl_screen.h"
#include "host.h"
#include "vgui_int.h"
#include "vid.h"
#include "vgui2/text_draw.h"
*/

//qw
//int con_ormask;
/*
console_t	con_main;
console_t	con_chat;
console_t	*con;			// point to either con_main or con_chat

*/

float con_cursorspeed = 4.0f;

#define CON_TEXTSIZE 16384
#define	MAXPRINTMSG	4096
#define CON_MAX_DEBUG_AREAS		32
#define CON_DEBUG_AREA_OFFSET_Y	20

qboolean con_forcedup = false; // because no entities to refresh

int con_linewidth = 1; // characters across screen
int con_totallines = 0;		// total lines in console scrollback
int con_backscroll = 0;		// lines up from bottom to display
int con_current = 0;		// where next message will be printed
int con_x = 0;				// offset in current line for next print

char *con_text = nullptr;

qboolean flIsDebugPrint = false;

cvar_t con_fastmode = { "con_fastmode", "1" };
//cvar_t con_notifytime = { "con_notifytime", "1" };
cvar_t con_notifytime = {"con_notifytime","3"};		//seconds
cvar_t con_color = { "con_color", "255 180 30", FCVAR_ARCHIVE };
cvar_t con_shifttoggleconsole = { "con_shifttoggleconsole", "0" };
cvar_t con_mono = { "con_mono", "0", FCVAR_ARCHIVE };

//#define	NUM_CON_TIMES 4

const int CON_TIMES_MIN = 4;
const int CON_TIMES_MAX = 64;

int con_num_times = 4;

float *con_times = nullptr; // realtime time the line was generated
							// for transparent notify lines
							
void* con_notifypos = nullptr;

//int con_vislines;

qboolean con_debuglog = false;

#define MAXCMDLINE 256

struct da_notify_t
{
	char szNotify[ CON_MAX_NOTIFY_STRING ];
	float expire;
	float color[ 3 ];
};

da_notify_t da_notify[ CON_MAX_DEBUG_AREAS ];

float da_default_color[ 3 ] = { 1, 1, 1 };

static redirect_t sv_redirected = RD_NONE;

static char outputbuf[ NET_MAX_FRAG_BUFFER ] = {};

static char g_szNotifyAreaString[ 256 ] = {};

extern	char key_lines[32][MAXCMDLINE];
extern	int edit_line;
extern	int key_linepos;

qboolean con_initialized = false;

int con_notifylines = 0; // scan lines to clear for notify lines

/*
void Key_ClearTyping ()
{
	key_lines[edit_line][1] = 0;	// clear any typing
	key_linepos = 1;
}
*/

void Con_HideConsole_f()
{
	VGuiWrap2_HideConsole();
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f ()
{
	if( VGuiWrap2_IsConsoleVisible() )
		VGuiWrap2_HideConsole();
	else
		VGuiWrap2_ShowConsole();

	//Key_ClearTyping ();
	
/*
	if (key_dest == key_console)
	{
		extern void M_Menu_Main_f ();
		
		if (cls.state == ca_connected) // ca_active
		{
			key_dest = key_game;
			key_lines[edit_line][1] = 0;	// clear any typing
			key_linepos = 1;
		}
		else
			M_Menu_Main_f ();
	}
	else
		key_dest = key_console;
	
	SCR_EndLoadingPlaque ();
	memset (con_times, 0, sizeof(con_times));
*/
	
	//Con_ClearNotify ();
}

/*
================
Con_ToggleChat_f
================
*/
/*
void Con_ToggleChat_f ()
{
	Key_ClearTyping ();

	if (key_dest == key_console)
	{
		if (cls.state == ca_active)
			key_dest = key_game;
	}
	else
		key_dest = key_console;
	
	Con_ClearNotify ();
}
*/

/*
================
Con_Clear_f
================
*/
void Con_Clear_f ()
{
	if (con_text)
		Q_memset (con_text, ' ', CON_TEXTSIZE); // netquake/gs
	
	// qw
	//Q_memset (con_main.text, ' ', CON_TEXTSIZE);
	//Q_memset (con_chat.text, ' ', CON_TEXTSIZE);
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify ()
{
	if( con_times )
	{
		for (int i=0 ; i < con_num_times ; i++)
			con_times[i] = 0;
		
		g_szNotifyAreaString[ 0 ] = '\0';
	};
}

/*
================
Con_MessageMode_f
================
*/
extern qboolean team_message;

void Con_MessageMode_f ()
{
	if( VGuiWrap2_IsInCareerMatch() == CAREER_NONE )
	{
		//chat_team = false;
		key_dest = key_message;
		//team_message = false;
		
		if( Cmd_Argc() == 2 )
		{
			Q_strncpy( message_type, Cmd_Argv( 1 ), ARRAYSIZE( message_type ) - 1 );
			message_type[ ARRAYSIZE( message_type ) - 1 ] = '\0';
		}
		else
			Q_strcpy( message_type, "say" );
	};
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f ()
{
	if( VGuiWrap2_IsInCareerMatch() == CAREER_NONE )
	{
		//chat_team = true;
		key_dest = key_message;
		//team_message = true;
		Q_strcpy( message_type, "say_team" );
	};
}

void Con_Debug_f()
{
	if( con_debuglog )
	{
		Con_Printf( "condebug disabled\n" );
		con_debuglog = false;
	}
	else
	{
		con_debuglog = true;
		Con_Printf( "condebug enabled\n" );
	}
}

void Con_SetTimes_f()
{
	if( Cmd_Argc() == 2 )
	{
		con_num_times = clamp( Q_atoi( Cmd_Argv( 1 ) ), CON_TIMES_MIN, CON_TIMES_MAX );

		if( con_times )
			Mem_Free( con_times );
		if( con_notifypos )
			Mem_Free( con_notifypos );

		con_times = reinterpret_cast<float*>( Mem_Malloc( sizeof( float ) * con_num_times ) );
		con_notifypos = reinterpret_cast<int*>( Mem_Malloc( sizeof( int ) * con_num_times ) );
		
		if( !con_times || !con_notifypos )
			Sys_Error( "Couldn't allocate space for %i console overlays.", con_num_times );

		Con_Printf( "%i lines will overlay.\n", con_num_times );
	}
	else
	{
		Con_Printf( "contimes <n>\nShow <n> overlay lines [4-64].\n%i current overlay lines.\n", con_num_times );
	}
}

/*
================
Con_Resize

================
*/
/*
void Con_Resize (console_t *con)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = (vid.width >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		Q_memset (con->text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;
	
		if (con_linewidth < numchars)
			numchars = con_linewidth;

		Q_memcpy (tbuf, con->text, CON_TEXTSIZE);
		Q_memset (con->text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con->text[(con_totallines - 1 - i) * con_linewidth + j] =
						tbuf[((con->current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con->current = con_totallines - 1;
	con->display = con->current;
}

void Con_CheckResize ()
{
	Con_Resize (&con_main);
	Con_Resize (&con_chat);
}
*/

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize ()
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = (vid.width >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		Q_memset (con_text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;
	
		if (con_linewidth < numchars)
			numchars = con_linewidth;

		Q_memcpy (tbuf, con_text, CON_TEXTSIZE);
		Q_memset (con_text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linewidth + j] =
						tbuf[((con_current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}

/*
================
Con_Init
================
*/
void Con_Init ()
{
	con_debuglog = COM_CheckParm("-condebug"); // != 0;

	if (con_debuglog)
		FS_RemoveFile( "qconsole.log", nullptr );
	
	con_text = (char*)Hunk_AllocName (CON_TEXTSIZE, "context");
	Q_memset (con_text, ' ', CON_TEXTSIZE);
	
	//con = &con_main;
	
	con_linewidth = -1;
	
	con_times = reinterpret_cast<float*>( Mem_Malloc( sizeof( float ) * con_num_times ) );
	con_notifypos = reinterpret_cast<int*>( Mem_Malloc( sizeof( int ) * con_num_times ) );

	if( !con_times || !con_notifypos )
		Sys_Error( "Couldn't allocate space for %i console overlays.", con_num_times );
	
	Con_CheckResize ();
	
	Con_DPrintf ("Console initialized.\n");

//
// register our commands
//
	Cvar_RegisterVariable( &con_fastmode );
	Cvar_RegisterVariable (&con_notifytime);
	Cvar_RegisterVariable( &con_color );
	Cvar_RegisterVariable( &con_shifttoggleconsole );
	Cvar_RegisterVariable( &con_mono );

	Cmd_AddCommand( "contimes", Con_SetTimes_f );
	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand( "hideconsole", Con_HideConsole_f );
	//Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand( "condebug", Con_Debug_f );
	
	con_initialized = true;
}

void Con_Shutdown()
{
	if( con_times )
		Mem_Free( con_times );

	if( con_notifypos )
		Mem_Free( con_notifypos );

	con_times = nullptr;
	con_notifypos = nullptr;
	con_initialized = false;
}

/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed ()
{
	// NOTE: commented are qw
	
	con_x = 0; // con->x
	//if (con->display == con->current)
		//con->display++;
	con_current++; // con->current
	//Q_memset (&con->text[(con->current%con_totallines)*con_linewidth]
	//, ' ', con_linewidth);
	Q_memset (&con_text[(con_current%con_totallines)*con_linewidth]
	, ' ', con_linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
/*
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;
	
	con_backscroll = 0;

	if (txt[0] == 1)
	{
		mask = 128;		// go to colored text
		S_LocalSound ("misc/talk.wav");
	// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con_linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con_linewidth && (con_x + l > con_linewidth) )
			con_x = 0; // con->x = 0;

		txt++;

		if (cr)
		{
			con_current--; // con->current--;
			cr = false;
		}
		
		if (!con_x) // !con->x
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con_current >= 0) // con->current
				con_times[con_current % NUM_CON_TIMES] = realtime;
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			break;

		case '\r':
			con_x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con_current % con_totallines;
			con_text[y*con_linewidth+con_x] = c | mask; // | con_ormask
			con_x++;
			if (con_x >= con_linewidth)
				con_x = 0;
			break;
		}
		
	}
}
*/

/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char *file, const char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    
    va_start(argptr, fmt);
    vsnprintf(data, ARRAYSIZE( data ), fmt, argptr);
    va_end(argptr);
	
	data[ ARRAYSIZE( data ) - 1 ] = '\0';
	
	// TODO: get rid of this ugly stuff and use fopen or IFileSystem - Solokiller
    int fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    write(fd, data, strlen(data));
    close(fd);
}

/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsnprintf (msg, sizeof( msg ), fmt,argptr);
	va_end (argptr);

	// also echo to debugging console
	Sys_Printf ("%s", msg);

	// add to redirected message
	if (sv_redirected != RD_NONE)
	{
		const size_t uiLength = Q_strlen( msg );
		
		//if (uiLength + Q_strlen(outputbuf) > sizeof(outputbuf) - 1)
		if( ( Q_strlen( outputbuf ) + uiLength ) >= NET_MAX_FRAG_BUFFER )
			SV_FlushRedirect ();
		
		strncat (outputbuf, msg, NET_MAX_FRAG_BUFFER - 1);
		//return;
	}
	else
	{
		// log all messages to file
		if (con_debuglog)
			Con_DebugLog( "qconsole.log", "%s", msg );
			//Con_DebugLog(va("%s/qconsole.log",com_gamedir), "%s", msg);
		
		//if (!con_initialized)
			//return;
		
		//if (cls.state == ca_dedicated)
			//return;		// no graphics mode
		
		if( host_initialized && con_initialized && cls.state )
		{
			if( developer.value != 0.0 )
			{
				strncpy( g_szNotifyAreaString, msg, ARRAYSIZE( g_szNotifyAreaString ) );
				g_szNotifyAreaString[ ARRAYSIZE( g_szNotifyAreaString ) - 1 ] = '\0';

				*con_times = realtime;
			}

			VGuiWrap2_ConPrintf( msg );
		}
	};
	
	//if(sv_logfile)
		//fprintf (sv_logfile, "%s", msg);

	

// write it to the scrollable buffer
	//Con_Print (msg);
	
// update the screen immediately if the console is displayed
	//if (cls.state != ca_active)
/*	
	if (cls.signon != SIGNONS && !scr_disabled_for_loading )
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls Con_Printf
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen ();
			inupdate = false;
		}
	}
*/
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (const char *fmt, ...)
{
	// don't confuse non-developers with techie stuff...
	//if (!developer.value)
		//return;	

	va_list argptr;
	
	va_start (argptr,fmt);
	
	if( developer.value != 0.0 && ( scr_con_current == 0.0 || cls.state != 5 ) )
	{
		char msg[MAXPRINTMSG];
		
		vsnprintf (msg, sizeof( msg ), fmt,argptr);
		
		//Con_Printf ("%s", msg); // NetQuake
		
		if( con_debuglog )
			Con_DebugLog( "qconsole.log", "%s", msg );

		VGuiWrap2_ConDPrintf( msg );
	};
	
	va_end (argptr);
}

int Con_IsVisible()
{
	g_engdstAddrs.Con_IsVisible();

	return static_cast<int>( scr_con_current );
}

/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
		
	va_start (argptr,fmt);
	vsnprintf (msg, ARRAYSIZE( msg ), fmt, argptr);
	va_end (argptr);
	
	flIsDebugPrint = true;
	
	Con_Printf ("%s", msg);
	
	flIsDebugPrint = false;
}

void Con_NPrintf( int idx, const char* fmt, ... )
{
	va_list va;

	va_start( va, fmt );

	g_engdstAddrs.Con_NPrintf( &idx, const_cast<char**>( &fmt ) );

	if( 0 <= idx && idx < CON_MAX_DEBUG_AREAS )
	{
		vsnprintf( da_notify[ idx ].szNotify, ARRAYSIZE( da_notify[ idx ].szNotify ), fmt, va );
	
		da_notify[ idx ].expire = realtime + 4.0;
		da_notify[ idx ].color[ 0 ] = da_default_color[ 0 ];
		da_notify[ idx ].color[ 1 ] = da_default_color[ 1 ];
		da_notify[ idx ].color[ 2 ] = da_default_color[ 2 ];
	}

	va_end( va );
}

void Con_NXPrintf( con_nprint_t* info, const char* fmt, ... )
{
	va_list va;

	va_start( va, fmt );

	g_engdstAddrs.Con_NXPrintf( &info, const_cast<char**>( &fmt ) );

	if( info )
	{
		//TODO: doesn't check if < 0 - Solokiller
		if( info->index < CON_MAX_DEBUG_AREAS )
		{
			vsnprintf( da_notify[ info->index ].szNotify, ARRAYSIZE( da_notify[ info->index ].szNotify ), fmt, va );
			da_notify[ info->index ].szNotify[ ARRAYSIZE( da_notify[ info->index ].szNotify ) - 1 ] = '\0';
			
			da_notify[ info->index ].expire = info->time_to_live + realtime;
			da_notify[ info->index ].color[ 0 ] = info->color[ 0 ];
			da_notify[ info->index ].color[ 1 ] = info->color[ 1 ];
			da_notify[ info->index ].color[ 2 ] = info->color[ 2 ];
		}
	}

	va_end( va );
}

/*
==============================================================================

DRAWING

==============================================================================
*/

void Con_DrawDebugArea( int idx )
{
	if( 0 <= idx && idx < CON_MAX_DEBUG_AREAS )
	{
		const auto iOffset = idx * VGUI2_GetFontTall( VGUI2_GetConsoleFont() );

		const auto iWidth = Draw_StringLen( da_notify[ idx ].szNotify, VGUI2_GetConsoleFont() );

		if( ( iOffset + CON_DEBUG_AREA_OFFSET_Y - 1 ) < static_cast<int>( vid.height - CON_DEBUG_AREA_OFFSET_Y ) )
		{
			Draw_SetTextColor( da_notify[ idx ].color[ 0 ], da_notify[ idx ].color[ 1 ], da_notify[ idx ].color[ 2 ] );
			Draw_String( vid.width - 10 - iWidth, iOffset + CON_DEBUG_AREA_OFFSET_Y, da_notify[ idx ].szNotify );
		}
	}
}

void Con_DrawDebugAreas()
{
	for( int i = 0; i < CON_MAX_DEBUG_AREAS; ++i )
	{
		auto& notify = da_notify[ i ];

		if( notify.expire > realtime )
		{
			Con_DrawDebugArea( i );
		}
	}
}

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
/*
void Con_DrawInput ()
{
	int		y;
	int		i;
	char	*text;

	//if (key_dest != key_console && cls.state == ca_active)
	if (key_dest != key_console && !con_forcedup)
		return;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];
	
// add the cursor frame
	text[key_linepos] = 10+((int)(realtime*con_cursorspeed)&1);
	
// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con_linewidth ; i++)
		text[i] = ' ';
		
//	prestep if horizontally scrolling
	if (key_linepos >= con_linewidth)
		text += 1 + key_linepos - con_linewidth;
		
// draw it
	y = con_vislines-16; // 22 in qw

	for (i=0 ; i<con_linewidth ; i++)
		Draw_Character ( (i+1)<<3, con_vislines - 16, text[i]); // 16->22 in qw

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}
*/

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
/*
void Con_DrawNotify ()
{
	int		x, v;
	char	*text;
	int		i;
	float	time;
	extern char chat_buffer[];

	v = 0;
	for (i= con_current-NUM_CON_TIMES+1 ; i<=con_current ; i++)
	{
		if (i < 0)
			continue;
		time = con_times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = realtime - time;
		if (time > con_notifytime.value)
			continue;
		text = con_text + (i % con_totallines)*con_linewidth;
		
		clearnotify = 0;
		scr_copytop = 1;

		for (x = 0 ; x < con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, v, text[x]);

		v += 8;
	}

	if (key_dest == key_message)
	{
		clearnotify = 0;
		scr_copytop = 1;
	
		x = 0;
		
		Draw_String (8, v, "say:");
		while(chat_buffer[x])
		{
			Draw_Character ( (x+5)<<3, v, chat_buffer[x]);
			x++;
		}
		Draw_Character ( (x+5)<<3, v, 10+((int)(realtime*con_cursorspeed)&1));
		v += 8;
	}
	
	if (v > con_notifylines)
		con_notifylines = v;
}
*/
void Con_DrawNotify()
{
	Con_DrawDebugAreas();

	int v = 0;

	Draw_ResetTextColor();

	for( int i = con_current - con_num_times + 1; i <= con_current; ++i )
	{
		if( i >= 0 && con_times[ i % con_num_times ] )
		{
			if( ( realtime - con_times[ i % con_num_times ] ) <= con_notifytime.value )
			{
				clearnotify = false;
				scr_copytop = true;

				int x = 8;

				for( int j = 0; j < con_linewidth; ++j )
				{
					x += Draw_Character( x, v, g_szNotifyAreaString[ x ], VGUI2_GetConsoleFont() );
				}

				v += VGUI2_GetFontTall( VGUI2_GetConsoleFont() );
			}
		}
	}

	if( key_dest == key_message )
	{
		clearnotify = false;
		scr_copytop = true;

		int xOffset = 8;

		ClientDLL_ChatInputPosition( &xOffset, &v );

		auto pszBuffer = chat_buffer;

		if( static_cast<int>( vid.width / 10 ) < chat_bufferlen )
			pszBuffer = Q_UnicodeAdvance( chat_buffer, chat_bufferlen - vid.width / 10 );

		xOffset = Draw_String( xOffset, v, message_type );
		xOffset = Draw_String( xOffset + 3, v, ":" );
		xOffset = Draw_String( xOffset + 3, v, pszBuffer );

		Draw_Character( xOffset + 1, v, ( static_cast<int>( con_cursorspeed * realtime ) % 2 ) ? '\v' : '\n', VGUI2_GetConsoleFont() );
		v += VGUI2_GetFontTall( VGUI2_GetConsoleFont() );
	}

	if( con_notifylines < v )
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
/*
void Con_DrawConsole (int lines, qboolean drawinput)
{
	int				i, x, y;
	int				rows;
	char			*text;
	int				j;
	
	if (lines <= 0)
		return;

// draw the background
	Draw_ConsoleBackground (lines);

// draw the text
	con_vislines = lines;

	rows = (lines-16)>>3;		// rows of text to draw
	y = lines - 16 - (rows<<3);	// may start slightly negative

	for (i= con_current - rows + 1 ; i<=con_current ; i++, y+=8 )
	{
		j = i - con_backscroll;
		if (j<0)
			j = 0;
		text = con_text + (j % con_totallines)*con_linewidth;

		for (x=0 ; x<con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, y, text[x]);
	}

// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput ();
}
*/

/*
==================
Con_NotifyBox
==================
*/
/*
void Con_NotifyBox (char *text)
{
	double		t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf (text);

	Con_Printf ("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	key_dest = key_console;

	do
	{
		t1 = Sys_FloatTime ();
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();
		t2 = Sys_FloatTime ();
		realtime += t2-t1;		// make the cursor blink
	} while (key_count < 0);

	Con_Printf ("\n");
	key_dest = key_game;
	realtime = 0;				// put the cursor back to invisible
}
*/
