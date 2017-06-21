/*
 *	This file is part of OGS Engine
 *	Copyright (C) 1996-1997 Id Software, Inc.
 *	Copyright (C) 2016-2017 OGS Dev Team
 *
 *	OGS Engine is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	OGS Engine is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OGS Engine.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	In addition, as a special exception, the author gives permission to
 *	link the code of OGS Engine with the Half-Life Game Engine ("GoldSrc/GS
 *	Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *	L.L.C ("Valve").  You must obey the GNU General Public License in all
 *	respects for all of the code used other than the GoldSrc Engine and MODs
 *	from Valve.  If you modify this file, you may extend this exception
 *	to your version of the file, but you are not obligated to do so.  If
 *	you do not wish to do so, delete this exception statement from your
 *	version.
 */

/// @file
/// @brief general definitions

#pragma once

#include "common/maintypes.h"
#include "public/FileSystem.h"
#include "rehlds/bspfile.h"
#include "rehlds/common_rehlds.h"
#include "system/info.hpp"

#ifdef HOOK_ENGINE
	#define serverinfo (*pserverinfo)

	#define gpszVersionString (*pgpszVersionString)
	#define gpszProductString (*pgpszProductString)

	#define bfread (*pbfread)
	#define bfwrite (*pbfwrite)

	#define msg_badread (*pmsg_badread)
	#define msg_readcount (*pmsg_readcount)

	#define bigendien (*pbigendien)

	#define BigShort (*pBigShort)
	#define LittleShort (*pLittleShort)
	#define BigLong (*pBigLong)
	#define LittleLong (*pLittleLong)
	#define BigFloat (*pBigFloat)
	#define LittleFloat (*pLittleFloat)

	#define com_argc (*pcom_argc)
	#define com_argv (*pcom_argv)
	#define com_token (*pcom_token)
	#define com_ignorecolons (*pcom_ignorecolons)
	#define s_com_token_unget (*ps_com_token_unget)
	#define com_clientfallback (*pcom_clientfallback)
	#define com_gamedir (*pcom_gamedir)
	#define com_cmdline (*pcom_cmdline)

	#define loadcache (*ploadcache)
	#define loadbuf (*ploadbuf)
	#define loadsize (*ploadsize)
#endif // HOOK_ENGINE

extern char serverinfo[MAX_INFO_STRING];

extern char gpszProductString[32];
extern char gpszVersionString[32];

typedef struct bf_read_s bf_read_t;
typedef struct bf_write_s bf_write_t;

extern bf_read_t bfread;
extern bf_write_t bfwrite;

//============================================================================

extern qboolean bigendien;

extern short (*BigShort)(short l);
extern short (*LittleShort)(short l);

extern int (*BigLong)(int l);
extern int (*LittleLong)(int l);

extern float (*BigFloat)(float l);
extern float (*LittleFloat)(float l);

//============================================================================

extern int com_argc;
extern char **com_argv; // const char?

extern char com_token[COM_TOKEN_LEN]; // 1024

/**
*	If true, colons are treated as regular characters, instead of being parsed as single characters.
*/
extern qboolean com_ignorecolons;

extern qboolean s_com_token_unget;

extern char com_clientfallback[MAX_PATH];
extern char com_gamedir[MAX_PATH]; // MAX_OSPATH
extern char com_cmdline[COM_MAX_CMD_LINE];

typedef struct cache_user_s cache_user_t;

extern cache_user_t *loadcache;
extern byte *loadbuf;
extern int loadsize;

//============================================================================

//#define Q_functions
#ifndef Q_functions

#ifndef _WIN32
#define _strlwr(p)                 \
	for(int i = 0; p[i] != 0; i++) \
		p[i] = tolower(p[i]);
#endif

#if defined(REHLDS_OPT_PEDANTIC) || defined(REHLDS_FIXES)
#define Q_memset A_memset
#define Q_memcpy A_memcpy
#define Q_memmove A_memmove
#define Q_strlen A_strlen
#define Q_memcmp A_memcmp
#define Q_strcpy A_strcpy
#define Q_strncpy strncpy
#define Q_strrchr strrchr
#define Q_strcat A_strcat
#define Q_strncat strncat
#define Q_strcmp A_strcmp
#define Q_strncmp strncmp
//#define Q_strcasecmp _stricmp		// Use Q_stricmp
//#define Q_strncasecmp _strnicmp	// Use Q_strnicmp
#define Q_stricmp A_stricmp
#define Q_strnicmp _strnicmp
#define Q_strstr A_strstr
#define Q_strchr strchr
#define Q_strlwr A_strtolower
#define Q_sprintf sprintf
#define Q_snprintf _snprintf
#define Q_atoi atoi
#define Q_atof atof
#define Q_sqrt M_sqrt
#define Q_min M_min
#define Q_max M_max
#define Q_clamp M_clamp
//#define Q_strtoull strtoull
//#define Q_FileNameCmp FileNameCmp
#define Q_vsnprintf _vsnprintf
#else
#define Q_memset memset
#define Q_memcpy memcpy
#define Q_memmove memmove
#define Q_strlen strlen
#define Q_memcmp memcmp
#define Q_strcpy strcpy
#define Q_strncpy strncpy
#define Q_strrchr strrchr
#define Q_strcat strcat
#define Q_strncat strncat
#define Q_strcmp strcmp
#define Q_strncmp strncmp
//#define Q_strcasecmp _stricmp		// Use Q_stricmp
//#define Q_strncasecmp _strnicmp	// Use Q_strnicmp
#define Q_stricmp _stricmp
#define Q_strnicmp _strnicmp
#define Q_strstr strstr
#define Q_strchr strchr
#define Q_strlwr _strlwr
#define Q_sprintf sprintf
#define Q_snprintf _snprintf
#define Q_atoi atoi
#define Q_atof atof
#define Q_sqrt sqrt
#define Q_min min
#define Q_max max
#define Q_clamp clamp
//#define Q_strtoull strtoull
//#define Q_FileNameCmp FileNameCmp
#define Q_vsnprintf _vsnprintf
#endif // defined(REHLDS_OPT_PEDANTIC) || defined(REHLDS_FIXES)

#else // Q_functions

void Q_strcpy(char *dest, const char *src);
int Q_strlen(const char *str);
NOBODY void Q_memset(void *dest, int fill, int count);
NOBODY void Q_memcpy(void *dest, const void *src, int count);
NOBODY int Q_memcmp(void *m1, void *m2, int count);
NOBODY void Q_strncpy(char *dest, const char *src, int count);
NOBODY char *Q_strrchr(char *s, char c);
NOBODY void Q_strcat(char *dest, char *src);
NOBODY int Q_strcmp(const char *s1, const char *s2);
NOBODY int Q_strncmp(const char *s1, const char *s2, int count);
NOBODY int Q_strncasecmp(const char *s1, const char *s2, int n);
NOBODY int Q_strcasecmp(const char *s1, const char *s2);
NOBODY int Q_stricmp(const char *s1, const char *s2);
NOBODY int Q_strnicmp(const char *s1, const char *s2, int n);
NOBODY int Q_atoi(const char *str);
NOBODY float Q_atof(const char *str);
NOBODY char *Q_strlwr(char *src);
NOBODY int Q_FileNameCmp(char *file1, char *file2);
NOBODY char *Q_strstr(const char *s1, const char *search);
NOBODY uint64 Q_strtoull(char *str);

#endif // Q_functions

// strcpy that works correctly with overlapping src and dst buffers
char *strcpy_safe(char *dst, char *src);

byte COM_Nibble(char c);
void COM_HexConvert(const char *pszInput, int nInputLength, byte *pOutput);
NOXREF char *COM_BinPrintf(byte *buf, int nLen);

/**
*	Set explanation for disconnection
*	@param bPrint Whether to print the explanation to the console
*/
void COM_ExplainDisconnection(qboolean bPrint, const char *fmt, ...);

/**
*	Set extended explanation for disconnection
*	Only used if COM_ExplainDisconnection has been called as well
*	@param bPrint Whether to print the explanation to the console
*/
NOXREF void COM_ExtendedExplainDisconnection(qboolean bPrint, const char *fmt, ...);

int LongSwap(int l);
short ShortSwap(short l);
short ShortNoSwap(short l);
int LongNoSwap(int l);
float FloatSwap(float f);
float FloatNoSwap(float f);

NOXREF const char *COM_SkipPath(const char *pathname);

void COM_StripExtension(char *in, char *out);
char *COM_FileExtension(char *in);
void COM_DefaultExtension(char *path, /*const*/ char *extension);

void COM_FileBase(const char *in, char *out);

char *COM_GetToken();
void COM_UngetToken();

/// Parse a token out of a string
char *COM_Parse(char *data); // const char all?

/// Parse a line out of a string. Used to parse out lines out of cfg files
char *COM_ParseLine(char *data);

//const char *COM_ParseFile(const char *data, char *token, int maxtoken);

int COM_TokenWaiting(char *buffer); // return bool? const char?

/**
*	Returns the position (1 to argc-1) in the program's argument list
*	where the given parameter apears, or 0 if not present
*/
int COM_CheckParm(const char *parm);

void COM_Init(char *basedir); // is basedir really present as arg in gs?
void COM_InitArgv(int argc, char *argv[]); // const char?

void COM_Shutdown();

// does a varargs printf into a temp buffer
char *va(char *format, ...);

/**
*	Converts a vector to a string representation.
*/
char *vstr(vec_t *v);

/**
*	Searches for a byte of data in a binary buffer
*/
NOXREF int memsearch(byte *start, int count, int search);

NOXREF void COM_WriteFile(char *filename, void *data, int len);
NOXREF void COM_CopyFile(char *netpath, char *cachepath);

void COM_FixSlashes(char *pname);

/**
*	Creates a hierarchy of directories specified by path
*	Modifies the given string while performing this operation, but restores it to its original state
*/
void COM_CreatePath(const char *path);

NOXREF int COM_ExpandFilename(char *filename); // return bool?
int COM_FileSize(const char *filename);

byte *COM_LoadFile(char *path, int usehunk, int *pLength); // was const char *path
void COM_FreeFile(void *buffer);

void COM_CopyFileChunk(FileHandle_t dst, FileHandle_t src, int nSize);
NOXREF byte *COM_LoadFileLimit(char *path, int pos, int cbmax, int *pcbread, FileHandle_t *phFile);
byte *COM_LoadHunkFile(char *path);
byte *COM_LoadTempFile(char *path, int *pLength);
void COM_LoadCacheFile(char *path, struct cache_user_s *cu);
NOXREF byte *COM_LoadStackFile(char *path, void *buffer, int bufsize, int *length);

NOXREF void COM_AddAppDirectory(char *pszBaseDir, const char *appName);
void COM_AddDefaultDir(const char *pszDir);
void COM_StripTrailingSlash(char *ppath);
void COM_ParseDirectoryFromCmd(const char *pCmdName, char *pDirName, const char *pDefault);
qboolean COM_SetupDirectories(); // bool?
void COM_CheckPrintMap(dheader_t *header, const char *mapname, qboolean bShowOutdated);
void COM_ListMaps(char *pszSubString);
void COM_Log(char *pszFile, char *fmt, ...);
byte *COM_LoadFileForMe(char *filename, int *pLength);
int COM_CompareFileTime(char *filename1, char *filename2, int *iCompare);
void COM_GetGameDir(char *szGameDir);
int COM_EntsForPlayerSlots(int nPlayers);
void COM_NormalizeAngles(vec_t *angles);

void COM_Munge(byte *data, int len, int seq);
void COM_UnMunge(byte *data, int len, int seq);

void COM_Munge2(byte *data, int len, int seq);
void COM_UnMunge2(byte *data, int len, int seq);

void COM_Munge3(byte *data, int len, int seq);
NOXREF void COM_UnMunge3(byte *data, int len, int seq);

uint COM_GetApproxWavePlayLength(const char *filepath);