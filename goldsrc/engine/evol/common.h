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
*	general definitions
*/

#pragma once

//#include <cstdio>

#include "common/mathlib.h"
#include "common/const.h"

//#include "tier0/platform.h"

#if !defined BYTE_DEFINED
typedef unsigned char 		byte; // TODO: remove?
#define BYTE_DEFINED 1
#endif

//#define MAX_SERVERINFO_STRING 512
//#define MAX_LOCALINFO_STRING 32768

//============================================================================

enum FSB
{
	/**
	*	If not set, do a Sys_Error
	*/
	FSB_ALLOWOVERFLOW	=	1	<<	0,

	/**
	*	set if the buffer size failed
	*/
	FSB_OVERFLOWED		=	1	<<	1
};

typedef struct sizebuf_s
{
	const char* buffername;
	
	unsigned short flags;
	
	byte *data;
	
	int maxsize;
	int cursize;
} sizebuf_t;

void SZ_Alloc(const char* name, sizebuf_t *buf, int startsize);
void SZ_Free(sizebuf_t *buf);

void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, const char *data);	///< strcats onto the sizebuf

//============================================================================

void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	qboolean bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

//struct usercmd_s;
//extern struct usercmd_s nullcmd;

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

extern	int			msg_readcount;
extern	qboolean	msg_badread; ///< set if a read goes beyond end of message

void MSG_BeginReading ();
int MSG_GetReadCount();
int MSG_ReadChar ();
int MSG_ReadByte ();
int MSG_ReadShort ();
int MSG_ReadLong ();
float MSG_ReadFloat ();
char *MSG_ReadString ();
char *MSG_ReadStringLine ();

float MSG_ReadCoord ();
float MSG_ReadAngle ();
float MSG_ReadAngle16 ();
void MSG_ReadDeltaUsercmd (struct usercmd_s *from, struct usercmd_s *cmd);

//============================================================================

#ifndef OGS_CUSTOM_CRT
	#define Q_memset(d, f, c) memset((d), (f), (c))
	#define Q_memcpy(d, s, c) memcpy((d), (s), (c))
	#define Q_memcmp(m1, m2, c) memcmp((m1), (m2), (c))
	#define Q_strcpy(d, s) strcpy((d), (s))
	#define Q_strncpy(d, s, n) strncpy((d), (s), (n))
	#define Q_strlen(s) ((int)strlen(s))
	#define Q_strrchr(s, c) strrchr((s), (c))
	#define Q_strcat(d, s) strcat((d), (s))
	#define Q_strcmp(s1, s2) strcmp((s1), (s2))
	#define Q_strncmp(s1, s2, n) strncmp((s1), (s2), (n))

	#ifdef _WIN32
		#define Q_strcasecmp(s1, s2) _stricmp((s1), (s2))
		#define Q_strncasecmp(s1, s2, n) _strnicmp((s1), (s2), (n))
	#else
		#define Q_strcasecmp(s1, s2) strcasecmp((s1), (s2))
		#define Q_strncasecmp(s1, s2, n) strncasecmp((s1), (s2), (n))
	#endif
#else
	void Q_memset (void *dest, int fill, int count);
	void Q_memcpy (void *dest, void *src, int count);
	int Q_memcmp (void *m1, void *m2, int count);
	void Q_strcpy (char *dest, char *src);
	void Q_strncpy (char *dest, char *src, int count);
	int Q_strlen (char *str);
	char *Q_strrchr (char *s, char c);
	void Q_strcat (char *dest, char *src);
	int Q_strcmp (char *s1, char *s2);
	int Q_strncmp (char *s1, char *s2, int count);
	int Q_strcasecmp (char *s1, char *s2);
	int Q_strncasecmp (char *s1, char *s2, int n);
	int	Q_atoi (char *str);
	float Q_atof (char *str);
#endif

//============================================================================

extern	char		com_token[1024];
extern	qboolean	com_eof;

extern char gpszProductString[ 32 ];
extern char gpszVersionString[ 32 ];

extern	int		com_argc;
extern	const char	**com_argv;

/**
*	If true, colons are treated as regular characters, instead of being parsed as single characters.
*/
extern bool com_ignorecolons;

void COM_UngetToken();

/**
*	Parse a token out of a string
*/
char *COM_Parse (char *data);

/**
*	Parse a line out of a string. Used to parse out lines out of cfg files
*/
char* COM_ParseLine( char* data );

bool COM_TokenWaiting( const char* buffer );

/**
*	Returns the position (1 to argc-1) in the program's argument list
*	where the given parameter apears, or 0 if not present
*/
int COM_CheckParm (const char *parm);

//void COM_AddParm (const char *parm);

void COM_Init (/*const char *path*/);
void COM_Shutdown();

void COM_InitArgv (int argc, const char **argv);

bool COM_SetupDirectories();

void COM_ParseDirectoryFromCmd( const char *pCmdName, char *pDirName, const char *pDefault );

void COM_FixSlashes( char *pname );

void COM_AddDefaultDir( const char* pszDir );

void COM_AddAppDirectory( const char* pszBaseDir );

const char* COM_FileExtension( const char* in );

void COM_DefaultExtension (char *path, char *extension);

void COM_StripExtension (char *in, char *out);
void COM_StripTrailingSlash( char* ppath );

/**
*	Creates a hierarchy of directories specified by path
*	Modifies the given string while performing this operation, but restores it to its original state
*/
void COM_CreatePath( const char* path );

uint COM_GetApproxWavePlayLength( const char* filepath );

char* Info_Serverinfo();

char *COM_SkipPath (const char *pathname);

void COM_FileBase (const char *in, char *out);

/**
*	does a varargs printf into a temp buffer, so I don't need to have
*	varargs versions of all text functions.
*	FIXME: make this buffer size safe someday
*/
char *va(const char *format, ...);

/**
*	Converts a vector to a string representation.
*/
char* vstr( vec_t* v );

/**
*	Searches for a byte of data in a binary buffer
*/
int memsearch( byte* start, int count, int search );

/**
*	Compares filenames
*	@return -1 if file1 is not equal to file2, 0 otherwise
*/
int Q_FileNameCmp( const char* file1, const char* file2 );

//============================================================================

extern int com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

byte* COM_LoadFile( const char* path, int usehunk, int* pLength );
void COM_FreeFile( void *buffer );

byte* COM_LoadFileForMe( const char* filename, int* pLength );
byte* COM_LoadFileLimit( const char* path, int pos, int cbmax, int* pcbread, FileHandle_t* phFile );

void COM_WriteFile (const char *filename, void *data, int len);
int COM_OpenFile (const char *filename, int *hndl);
int COM_FOpenFile (const char *filename, FILE **file);
void COM_CloseFile (int h);

byte* COM_LoadStackFile( const char* path, void* buffer, int bufsize, int* length );
byte* COM_LoadTempFile( const char* path, int* pLength );
byte *COM_LoadHunkFile (const char *path);
void COM_LoadCacheFile( const char* path, cache_user_t* cu );

void COM_WriteFile( char* filename, void* data, int len );

int COM_FileSize( const char* filename );

bool COM_ExpandFilename( char* filename );

int COM_CompareFileTime( const char* filename1, const char* filename2, int* iCompare );

/**
*	@param cachepath Modified by the function but restored before returning
*/
void COM_CopyFile( const char* netpath, char* cachepath );

void COM_CopyFileChunk( FileHandle_t dst, FileHandle_t src, int nSize );

void COM_Log( const char* pszFile, const char* fmt, ... );

void COM_ListMaps( const char* pszSubString );

void COM_GetGameDir( char* szGameDir );

const char* COM_SkipPath( const char* pathname );

char* COM_BinPrintf( byte* buf, int nLen );

byte COM_Nibble( char c );

void COM_HexConvert( const char* pszInput, int nInputLength, byte* pOutput );

/**
*	Normalizes the angles to a range of [ -180, 180 ]
*/
void COM_NormalizeAngles( vec_t* angles );

int COM_EntsForPlayerSlots( int nPlayers );

/**
*	Set explanation for disconnection
*	@param bPrint Whether to print the explanation to the console
*/
void COM_ExplainDisconnection( bool bPrint, const char* fmt, ... );

/**
*	Set extended explanation for disconnection
*	Only used if COM_ExplainDisconnection has been called as well
*	@param bPrint Whether to print the explanation to the console
*/
void COM_ExtendedExplainDisconnection( bool bPrint, const char* fmt, ... );