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

// common.c -- misc functions used in client and server

#include "quakedef.h"
/*
#include "bspfile.h"
#include "cdll_int.h"
#include "modinfo.h"
*/

#define NUM_SAFE_ARGVS  7

char gpszProductString[ 32 ] = {};
char gpszVersionString[ 32 ] = {};

static const char     *largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static const char     *argvdummy = " ";

static const char     *safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"};

qboolean        com_modified;   // set true if using non-id files

qboolean		proghack;

qboolean		msg_suppress_1 = 0;

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT              339
#define PAK0_CRC                32981

//TODO: on Windows com_token seems to be 2048 characters large. - Solokiller
char	com_token[1024] = {};

int com_argc = 0;
const char **com_argv = nullptr;

char com_cachedir[MAX_OSPATH] = {};
char com_clientfallback[ MAX_OSPATH ] = {};
char com_gamedir[MAX_OSPATH] = {};

static qboolean s_com_token_unget = false;

qboolean com_ignorecolons = false;

#define CMDLINE_LENGTH	256
char	com_cmdline[CMDLINE_LENGTH];

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of engine's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.
	
*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Q_memset (void *dest, int fill, int count)
{
	int             i;
	
	if ( (((long)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i=0 ; i<count ; i++)
			((byte *)dest)[i] = fill;
}

void Q_memcpy (void *dest, void *src, int count)
{
	int             i;
	
	if (( ( (long)dest | (long)src | count) & 3) == 0 )
	{
		count>>=2;
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i=0 ; i<count ; i++)
			((byte *)dest)[i] = ((byte *)src)[i];
}

int Q_memcmp (void *m1, void *m2, int count)
{
	while(count)
	{
		count--;
		if (((byte *)m1)[count] != ((byte *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy (char *dest, char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen (char *str)
{
	int             count;
	
	count = 0;
	while (str[count])
		count++;

	return count;
}

char *Q_strrchr(char *s, char c)
{
    int len = Q_strlen(s);
    s += len;
    while (len--)
	if (*--s == c) return s;
    return 0;
}

void Q_strcat (char *dest, char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp (char *s1, char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int Q_strncmp (char *s1, char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int Q_strncasecmp (char *s1, char *s2, int n)
{
	int             c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}
	
	return -1;
}

int Q_strcasecmp (char *s1, char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

int Q_atoi (char *str)
{
	int             val;
	int             sign;
	int             c;
	
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}
	
	return 0;
}


float Q_atof (char *str)
{
	double			val;
	int             sign;
	int             c;
	int             decimal, total;
	
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}
	
	return val*sign;
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean        bigendien;

short   (*BigShort) (short l);
short   (*LittleShort) (short l);
int     (*BigLong) (int l);
int     (*LittleLong) (int l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int     LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		byte    b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte    *buf;
	
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte    *buf;
	
	buf = SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float   f;
		int     l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, (int)(f*8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, ((int)f*256/360) & 255);
}

//
// reading functions
//
int                     msg_readcount;
qboolean        msg_badread;

void MSG_BeginReading ()
{
	msg_readcount = 0;
	msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadByte ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadShort ()
{
	int     c;
	
	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8));
	
	msg_readcount += 2;
	
	return c;
}

int MSG_ReadLong ()
{
	int     c;
	
	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8)
	+ (net_message.data[msg_readcount+2]<<16)
	+ (net_message.data[msg_readcount+3]<<24);
	
	msg_readcount += 4;
	
	return c;
}

float MSG_ReadFloat ()
{
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	
	dat.b[0] =      net_message.data[msg_readcount];
	dat.b[1] =      net_message.data[msg_readcount+1];
	dat.b[2] =      net_message.data[msg_readcount+2];
	dat.b[3] =      net_message.data[msg_readcount+3];
	msg_readcount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;   
}

char *MSG_ReadString ()
{
	static char     string[2048];
	int             l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

float MSG_ReadCoord ()
{
	return MSG_ReadShort() * (1.0/8);
}

float MSG_ReadAngle ()
{
	return MSG_ReadChar() * (360.0/256);
}



//===========================================================================

void SZ_Alloc (const char* name, sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;

	buf->buffername = name;
	buf->data = (byte*)Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
	buf->flags = 0;
}

void SZ_Free (sizebuf_t *buf)
{
//      Z_Free (buf->data);
//      buf->data = NULL;
//      buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->flags &= ~FSB_OVERFLOWED;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void    *data;
	
	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Sys_Error ("SZ_GetSpace: overflow without allowoverflow set");
		
		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);
			
		buf->overflowed = true;
		Con_Printf ("SZ_GetSpace: overflow");
		SZ_Clear (buf); 
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void SZ_Write (sizebuf_t *buf, void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);         
}

void SZ_Print (sizebuf_t *buf, char *data)
{
	int             len;
	
	len = Q_strlen(data)+1;

// byte * cast to keep VC++ happy
	if (buf->data[buf->cursize-1])
		Q_memcpy ((byte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
	else
		Q_memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len); // write over trailing 0
}


//============================================================================


/*
============
COM_SkipPath
============
*/
const char *COM_SkipPath (const char *pathname)
{
	auto pszLast = pathname;

	for( auto pszPath = pathname; *pszPath; ++pszPath )
	{
		if( *pszPath == '\\' || *pszPath == '/' )
		{
			pszLast = pszPath;
		}
	}

	return pszLast;
}

void COM_CopyFileChunk( FileHandle_t dst, FileHandle_t src, int nSize )
{
	char copybuf[ 1024 ];

	auto iSizeLeft = nSize;

	if( iSizeLeft > sizeof( copybuf ) )
	{
		while( iSizeLeft > sizeof( copybuf ) )
		{
			FS_Read( copybuf, sizeof( copybuf ), src );
			FS_Write( copybuf, sizeof( copybuf ), dst );
			iSizeLeft -= sizeof( copybuf );
		}

		//Compute size left
		iSizeLeft = nSize - ( ( nSize - ( sizeof( copybuf ) + 1 ) ) & ~( sizeof( copybuf ) - 1 ) ) - sizeof( copybuf );
	}

	FS_Read( copybuf, iSizeLeft, src );
	FS_Write( copybuf, iSizeLeft, dst );
	FS_Flush( src );
	FS_Flush( dst );
}

void COM_Log( const char* pszFile, const char* fmt, ... )
{
	char string[ 1024 ];
	va_list va;

	va_start( va, fmt );

	auto pszFileName = pszFile ? pszFile : "c:\\hllog.txt";

	vsnprintf( string, ARRAYSIZE( string ), fmt, va );
	va_end( va );

	auto hFile = FS_Open( pszFileName, "a+t" );

	if( FILESYSTEM_INVALID_HANDLE != hFile )
	{
		FS_FPrintf( hFile, "%s", string );
		FS_Close( hFile );
	}
}

void COM_ListMaps( const char* pszSubString )
{
	const size_t uiSubStrLength = pszSubString && *pszSubString ? strlen( pszSubString ) : 0;

	Con_Printf( "-------------\n" );
	int showOutdated = 1;

	char curDir[ MAX_PATH ];
	char mapwild[ 64 ];
	char sz[ 64 ];

	dheader_t header;

	do
	{
		strcpy( mapwild, "maps/*.bsp" );
		for( auto i = Sys_FindFirst( mapwild, nullptr ); i; i = Sys_FindNext( nullptr ) )
		{
			snprintf( curDir, ARRAYSIZE( curDir ), "maps/%s", i );
			FS_GetLocalPath( curDir, curDir, ARRAYSIZE( curDir ) );
			if( strstr( curDir, com_gamedir ) && ( !uiSubStrLength || !strnicmp( i, pszSubString, uiSubStrLength ) ) )
			{
				memset( &header, 0, sizeof( header ) );

				sprintf( sz, "maps/%s", i );

				auto hFile = FS_Open( sz, "rb" );

				if( hFile )
				{
					FS_Read( &header, sizeof( header ), hFile );
					FS_Close( hFile );
				}

				//TODO: shouldn't this be calling LittleLong? - Solokiller
				if( header.version == BSPVERSION )
				{
					if( !showOutdated )
						Con_Printf( "%s\n", i );
				}
				else if( showOutdated )
				{
					Con_Printf( "OUTDATED:  %s\n", i );
				}
			}
		}
		Sys_FindClose();
		--showOutdated;
	}
	while( showOutdated != -1 );
}

void COM_GetGameDir( char* szGameDir )
{
	//TODO: define this particular limit. It's 260 for Linux as well - Solokiller
	if( szGameDir )
		snprintf( szGameDir, 259U, "%s", com_gamedir );
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (char *in, char *out)
{
	if( !in || !( *in ) )
		return;

	const auto uiLength = strlen( in );

	auto pszSrc = in + uiLength - 1;
	auto pszDest = out + uiLength - 1;

	qboolean bHandledExt = false;

	for( ; in <= pszSrc; --pszSrc, --pszDest )
	{
		if( bHandledExt || *pszSrc != '.' )
		{
			if( *pszSrc == '\\' || *pszSrc == '/' )
				bHandledExt = true;

			*pszSrc = *pszDest;
		}
		else
		{
			*pszSrc = '\0';
			bHandledExt = true;
		}
	}
}

qboolean COM_SetupDirectories()
{
	com_clientfallback[ 0 ] = '\0';
	com_gamedir[ 0 ] = '\0';

	char basedir[ 512 ];

	COM_ParseDirectoryFromCmd( "-basedir", basedir, "valve" );
	COM_ParseDirectoryFromCmd( "-game", com_gamedir, basedir );

	qboolean bResult = FileSystem_SetGameDirectory( basedir, com_gamedir[ 0 ] ? com_gamedir : nullptr );

	if( bResult )
	{
		//TODO: serverinfo is only 256 characters large, but 512 is passed in. - Solokiller
		Info_SetValueForStarKey( serverinfo, "*gamedir", com_gamedir, 512 );
	}

	return bResult;
}

void COM_ParseDirectoryFromCmd( const char *pCmdName, char *pDirName, const char *pDefault )
{
	const char* pszResult = nullptr;

	if( com_argc > 1 )
	{
		for( int arg = 1; arg < com_argc; ++arg )
		{
			auto pszArg = com_argv[ arg ];

			if( pszArg )
			{
				if( pCmdName && *pCmdName == *pszArg )
				{
					if( *pCmdName && !strcmp( pCmdName, pszArg ) )
					{
						if( arg < com_argc - 1 )
						{
							auto pszValue = com_argv[ arg + 1 ];

							if( *pszValue != '+' && *pszValue != '-' )
							{
								strcpy( pDirName, pszValue );
								pszResult = pDirName;
								break;
							}
						}
					}
				}
			}
		}
	}

	if( !pszResult )
	{
		if( pDefault )
		{
			strcpy( pDirName, pDefault );
			pszResult = pDirName;
		}
		else
		{
			pszResult = pDirName;
			*pDirName = '\0';
		}
	}

	const auto uiLength = strlen( pszResult );

	if( uiLength > 0 )
	{
		auto pszEnd = &pDirName[ uiLength - 1 ];
		if( *pszEnd == '/' || *pszEnd == '\\' )
			*pszEnd = '\0';
	}
}

void COM_FixSlashes( char *pname )
{
	for( char* pszNext = pname; *pszNext; ++pszNext )
	{
		if( *pszNext == '\\' )
			*pszNext = '/';
	}
}

void COM_AddDefaultDir( const char* pszDir )
{
	if( pszDir && *pszDir )
	{
		FileSystem_AddFallbackGameDir( pszDir );
	}
}

void COM_AddAppDirectory( const char* pszBaseDir )
{
	FS_AddSearchPath( pszBaseDir, "PLATFORM" );
}

/*
============
COM_FileExtension
============
*/
const char *COM_FileExtension (const char *in)
{
	static char exten[ 8 ];

	for( const char* pszExt = in; *pszExt; ++pszExt )
	{
		if( *pszExt == '.' )
		{
			//Skip the '.'
			++pszExt;

			size_t uiIndex;

			for( uiIndex = 0; *pszExt && uiIndex < ARRAYSIZE( exten ) - 1; ++pszExt, ++uiIndex )
			{
				exten[ uiIndex ] = *pszExt;
			}

			exten[ uiIndex ] = '\0';
			return exten;
		}
	}

	return "";
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (const char *in, char *out)
{
	if( !in )
		return;

	if( !*in )
		return;

	size_t uiLength = strlen( in );

	// scan backward for '.'
	size_t end = uiLength - 1;
	while( end && in[ end ] != '.' && in[ end ] != '/' && in[ end ] != '\\' )
		--end;

	if( in[ end ] != '.' )	// no '.', copy to end
		end = uiLength - 1;
	else
		--end;				// Found ',', copy to left of '.'


	// Scan backward for '/'
	size_t start = uiLength;
	while( start > 0 && in[ start - 1 ] != '/' && in[ start - 1 ] != '\\' )
		--start;

	// Length of new string
	uiLength = end - start + 1;

	// Copy partial string
	strncpy( out, &in[ start ], uiLength );
	// Terminate it
	out[ uiLength ] = '\0';
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension)
{
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	char* src = path + strlen( path ) - 1;

	while( *src != '/' && src != path )
	{
		if( *src == '.' )
			return;                 // it has an extension
		src--;
	}

	//TODO: define this constant, it's 260 on Linux as well - Solokiller
	strncat( path, extension, 260 - strlen( src ) );
}

void COM_UngetToken()
{
	s_com_token_unget = true;
}

/*
==============
COM_Parse

Parse a token out of a string

Updated version of COM_Parse from Quake:
Allows retrieving the last token by calling COM_UngetToken to mark it
Supports Unicode
Has buffer overflow checks
Allows colons to be treated as regular characters using com_ignorecolons
==============
*/
char *COM_Parse (char *data)
{
	if( s_com_token_unget )
	{
		s_com_token_unget = false;

		return data;
	}

	int len;

	len = 0;
	com_token[ 0 ] = 0;

	if( !data )
		return NULL;

	uchar32 wchar;

	// skip whitespace
skipwhite:
	while( !V_UTF8ToUChar32( data, wchar ) && wchar <= ' ' )
	{
		if( wchar == 0 )
			return NULL;                    // end of file;
		data = Q_UnicodeAdvance( data, 1 );
	}

	// skip // comments
	if( *data == '/' && data[ 1 ] == '/' )
	{
		while( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if( *data == '\"' )
	{
		data++;
		char c;
		while( len != ( ARRAYSIZE( com_token ) - 1 ) )
		{
			c = *data++;
			if( c == '\"' || !c )
			{
				break;
			}
			com_token[ len ] = c;
			len++;
		}

		com_token[ len ] = 0;
		return data;
	}

	// parse single characters
	if( *data == '{' || 
		*data == '}' || 
		*data == ')' ||
		*data == '(' ||
		*data == '\'' || 
		( !com_ignorecolons && *data == ':' ) )
	{
		com_token[ len ] = *data;
		len++;
		com_token[ len ] = 0;
		return data + 1;
	}

	char c;
	// parse a regular word
	do
	{
		com_token[ len ] = *data;
		data++;
		len++;
		c = *data;
		if( c == '{' || 
			c == '}' || 
			c == ')' || 
			c == '(' || 
			c == '\'' || 
			( !com_ignorecolons && c == ':' ) ||
			len >= ( ARRAYSIZE( com_token ) - 1 ) )
			break;
	}
	while( c>' ' );

	com_token[ len ] = 0;
	return data;
}

char* COM_ParseLine( char* data )
{
	if( s_com_token_unget )
	{
		s_com_token_unget = false;

		return data;
	}

	com_token[ 0 ] = 0;

	if( !data )
		return nullptr;

	int len = 0;

	char c = *data;

	do
	{
		com_token[ len ] = c;
		data++;
		len++;
		c = *data;
	}
	while( c>=' ' && len < ( ARRAYSIZE( com_token ) - 1 ) );

	com_token[ len ] = 0;

	//Skip unprintable characters.
	while( *data && *data < ' ' )
	{
		++data;
	}

	//End of data.
	if( *data == '\0' )
		return nullptr;

	return data;
}

//From SharedTokenWaiting - Solokiller
qboolean COM_TokenWaiting( const char* buffer )
{
	for( auto p = buffer; *p && *p != '\n'; ++p )
	{
		if( !isspace( *p ) || isalnum( *p ) )
			return true;
	}

	return false;
}

/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (const char *parm)
{
	for( int i = 1; i<com_argc; i++ )
	{
		if( !com_argv[ i ] )
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if( !Q_strcmp( parm, com_argv[ i ] ) )
			return i;
	}

	return 0;
}

void COM_Path_f ();

/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, const char **argv)
{
	int i, n;

	// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for( int j = 0; ( j<MAX_NUM_ARGVS ) && ( j< argc ); j++ )
	{
		i = 0;

		while( ( n < ( CMDLINE_LENGTH - 1 ) ) && argv[ j ][ i ] )
		{
			com_cmdline[ n++ ] = argv[ j ][ i++ ];
		}

		if( n < ( CMDLINE_LENGTH - 1 ) )
			com_cmdline[ n++ ] = ' ';
		else
			break;
	}

	com_cmdline[ n ] = 0;

	qboolean safe = false;

	for( com_argc = 0; ( com_argc<MAX_NUM_ARGVS ) && ( com_argc < argc );
		 com_argc++ )
	{
		largv[ com_argc ] = argv[ com_argc ];
		if( !Q_strcmp( "-safe", argv[ com_argc ] ) )
			safe = true;
	}

	if( safe )
	{
		// force all the safe-mode switches. Note that we reserved extra space in
		// case we need to add these, so we don't need an overflow check
		for( i = 0; i<NUM_SAFE_ARGVS; i++ )
		{
			largv[ com_argc ] = safeargvs[ i ];
			com_argc++;
		}
	}

	largv[ com_argc ] = argvdummy;
	com_argv = largv;
}


/*
================
COM_Init
================
*/
void COM_Init (/*char *basedir*/)
{
	byte    swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner 
	if ( *(short *)swaptest == 1)
	{
		bigendien = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	Cmd_AddCommand ("path", COM_Path_f);
}

void COM_Shutdown()
{
	// Nothing
}

unsigned int COM_GetApproxWavePlayLength( const char* filepath )
{
	//TODO: implement - Solokiller
	return 0;
}

char* Info_Serverinfo()
{
	return serverinfo;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char *va(const char *format, ...)
{
	static char string[ 1024 ];

	va_list argptr;

	va_start( argptr, format );
	vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	return string;  
}

const int MAX_VEC_STRINGS = 16;

char* vstr( vec_t* v )
{
	static char string[ MAX_VEC_STRINGS ][ 1024 ];
	static int idx = 0;

	idx = ( idx + 1 ) % MAX_VEC_STRINGS;

	snprintf( string[ idx ], ARRAYSIZE( string[ idx ] ), "%.4f %.4f %.4f", v[ 0 ], v[ 1 ], v[ 2 ] );

	return string[ idx ];
}

/// just for debugging
int memsearch (byte *start, int count, int search)
{
	if( count <= 0 )
		return -1;

	int result = 0;

	if( *start != search )
	{
		while( 1 )
		{
			++result;
			if( result == count )
				break;

			if( start[ result ] == search )
				return result;
		}

		result = -1;
	}

	return result;
}

/*
=============================================================================

FILESYSTEM

=============================================================================
*/

int com_filesize;

//
// in memory
//

typedef struct
{
	char    name[MAX_QPATH];
	int             filepos, filelen;
} packfile_t;

typedef struct pack_s
{
	char    filename[MAX_OSPATH];
	int             handle;
	int             numfiles;
	packfile_t      *files;
} pack_t;

//
// on disk
//
typedef struct
{
	char    name[56];
	int             filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int             dirofs;
	int             dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK       2048

typedef struct searchpath_s
{
	char    filename[MAX_OSPATH];
	pack_t  *pack;          // only one of filename / pack will be used
	struct searchpath_s *next;
} searchpath_t;

searchpath_t    *com_searchpaths;

/*
============
COM_Path_f

============
*/
void COM_Path_f ()
{
	searchpath_t    *s;
	
	Con_Printf ("Current search path:\n");
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
		{
			Con_Printf ("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf ("%s\n", s->filename);
	}
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile (char *filename, void *data, int len)
{
	char name[ MAX_OSPATH ];
	snprintf( name, ARRAYSIZE( name ), "%s", filename );

	COM_FixSlashes( name );
	COM_CreatePath( name );

	auto hFile = FS_Open( name, "wb" );

	if( FILESYSTEM_INVALID_HANDLE != hFile )
	{
		Sys_Printf( "COM_WriteFile: %s\n", name );
		FS_Write( data, len, hFile );
		FS_Close( hFile );
	}
	else
	{
		Sys_Printf( "COM_WriteFile: failed on %s\n", name );
	}
}

void COM_StripTrailingSlash( char* ppath )
{
	const auto uiLength = strlen( ppath );

	if( uiLength > 0 )
	{
		auto pszEnd = &ppath[ uiLength - 1 ];

		if( *pszEnd == '/' || *pszEnd == '\\' )
		{
			*pszEnd = '\0';
		}
	}
}

/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void    COM_CreatePath (char *path)
{
	//TODO: check if null or empty - Solokiller
	//TODO: copy into temp buffer - Solokiller
	auto pszNext = path + 1;

	while( *pszNext )
	{
		if( *pszNext == '\\' || *pszNext == '/' )
		{
			const auto cSave = *pszNext;
			*pszNext = '\0';

			FS_CreateDirHierarchy( path, nullptr );

			*pszNext = cSave;
		}

		++pszNext;
	}
}

int COM_FileSize( const char* filename )
{
	int result;

	auto hFile = FS_Open( filename, "rb" );

	if( FILESYSTEM_INVALID_HANDLE != hFile )
	{
		result = FS_Size( hFile );
		FS_Close( hFile );
	}
	else
	{
		result = -1;
	}

	return result;
}

qboolean COM_ExpandFilename( char* filename )
{
	char netpath[ MAX_PATH ];

	FS_GetLocalPath( filename, netpath, ARRAYSIZE( netpath ) );
	strcpy( filename, netpath );

	return *filename != '\0';
}

int COM_CompareFileTime( const char* filename1, const char* filename2, int* iCompare )
{
	*iCompare = 0;

	if( filename1 && filename2 )
	{
		const auto iLhs = FS_GetFileTime( filename1 );
		const auto iRhs = FS_GetFileTime( filename2 );

		if( iLhs < iRhs )
		{
			*iCompare = -1;
		}
		else if( iLhs > iRhs )
		{
			*iCompare = 1;
		}

		return true;
	}

	return false;
}

/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
void COM_CopyFile (const char *netpath, char *cachepath)
{
	auto hSrcFile = FS_Open( netpath, "rb" );

	if( FILESYSTEM_INVALID_HANDLE != hSrcFile )
	{
		auto uiSize = FS_Size( hSrcFile );

		//TODO: copy path instead of modifying original - Solokiller

		COM_CreatePath( cachepath );

		auto hDestFile = FS_Open( cachepath, "wb" );

		//TODO: check if file failed to open - Solokiller

		char buf[ 4096 ];

		while( uiSize >= sizeof( buf ) )
		{
			FS_Read( buf, sizeof( buf ), hSrcFile );
			FS_Write( buf, sizeof( buf ), hDestFile );

			uiSize -= sizeof( buf );
		}

		if( uiSize )
		{
			FS_Read( buf, uiSize, hSrcFile );
			FS_Write( buf, uiSize, hDestFile );
		}

		FS_Close( hSrcFile );
		FS_Close( hDestFile );
	}  
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
int COM_FindFile (char *filename, int *handle, FILE **file)
{
	searchpath_t    *search;
	char            netpath[MAX_OSPATH];
	char            cachepath[MAX_OSPATH];
	pack_t          *pak;
	int                     i;
	int                     findtime, cachetime;

	if (file && handle)
		Sys_Error ("COM_FindFile: both handle and file set");
	if (!file && !handle)
		Sys_Error ("COM_FindFile: neither handle or file set");
		
//
// search through the path, one element at a time
//
	search = com_searchpaths;
	if (proghack)
	{	// gross hack to use quake 1 progs with quake 2 maps
		if (!strcmp(filename, "progs.dat"))
			search = search->next;
	}

	for ( ; search ; search = search->next)
	{
	// is the element a pak file?
		if (search->pack)
		{
		// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!strcmp (pak->files[i].name, filename))
				{       // found it!
					Sys_Printf ("PackFile: %s : %s\n",pak->filename, filename);
					if (handle)
					{
						*handle = pak->handle;
						Sys_FileSeek (pak->handle, pak->files[i].filepos);
					}
					else
					{       // open a new file on the pakfile
						*file = fopen (pak->filename, "rb");
						if (*file)
							fseek (*file, pak->files[i].filepos, SEEK_SET);
					}
					com_filesize = pak->files[i].filelen;
					return com_filesize;
				}
		}
		else
		{               
	// check a file in the directory tree
			if (!static_registered)
			{       // if not a registered version, don't ever go beyond base
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}
			
			sprintf (netpath, "%s/%s",search->filename, filename);
			
			findtime = Sys_FileTime (netpath);
			if (findtime == -1)
				continue;
				
		// see if the file needs to be updated in the cache
			if (!com_cachedir[0])
				strcpy (cachepath, netpath);
			else
			{	
#if defined(_WIN32)
				if ((strlen(netpath) < 2) || (netpath[1] != ':'))
					sprintf (cachepath,"%s%s", com_cachedir, netpath);
				else
					sprintf (cachepath,"%s%s", com_cachedir, netpath+2);
#else
				sprintf (cachepath,"%s%s", com_cachedir, netpath);
#endif

				cachetime = Sys_FileTime (cachepath);
			
				if (cachetime < findtime)
					COM_CopyFile (netpath, cachepath);
				strcpy (netpath, cachepath);
			}	

			Sys_Printf ("FindFile: %s\n",netpath);
			com_filesize = Sys_FileOpenRead (netpath, &i);
			if (handle)
				*handle = i;
			else
			{
				Sys_FileClose (i);
				*file = fopen (netpath, "rb");
			}
			return com_filesize;
		}
		
	}
	
	Sys_Printf ("FindFile: can't find %s\n", filename);
	
	if (handle)
		*handle = -1;
	else
		*file = NULL;
	com_filesize = -1;
	return -1;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int COM_OpenFile (char *filename, int *handle)
{
	return COM_FindFile (filename, handle, NULL);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile (char *filename, FILE **file)
{
	return COM_FindFile (filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile (int h)
{
	searchpath_t    *s;
	
	for (s = com_searchpaths ; s ; s=s->next)
		if (s->pack && s->pack->handle == h)
			return;
			
	Sys_FileClose (h);
}

int Q_FileNameCmp( const char* file1, const char* file2 )
{
	for( auto pszLhs = file1, pszRhs = file2; *pszLhs && *pszRhs; ++pszLhs, ++pszRhs )
	{
		if( ( *pszLhs != '/' || *pszRhs != '\\' ) &&
			( *pszLhs != '\\' || *pszRhs != '/' ) )
		{
			if( tolower( *pszLhs ) != tolower( *pszRhs ) )
				return -1;
		}
	}

	return 0;
}

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
static cache_user_t *loadcache = nullptr;
static byte *loadbuf = nullptr;
static int loadsize = 0;

byte *COM_LoadFile (const char* path, int usehunk, int* pLength)
{
	g_engdstAddrs.COM_LoadFile( const_cast<char**>( &path ), &usehunk, &pLength );

	if( pLength )
		*pLength = 0;

	FileHandle_t hFile = FS_Open( path, "rb" );

	if( hFile == FILESYSTEM_INVALID_HANDLE )
		return nullptr;

	const int iSize = FS_Size( hFile );

	char base[ 4096 ];
	COM_FileBase( path, base );
	base[ 32 ] = '\0';

	void* pBuffer = 0;

	if( usehunk == 0 )
	{
		pBuffer = Z_Malloc( iSize + 1 );
	}
	else if( usehunk == 1 )
	{
		pBuffer = Hunk_AllocName( iSize + 1, base );
	}
	else if( usehunk == 2 )
	{
		pBuffer = Hunk_TempAlloc( iSize + 1 );
	}
	else if( usehunk == 3 )
	{
		pBuffer = Cache_Alloc( loadcache, iSize + 1, base );
	}
	else if( usehunk == 4 )
	{
		pBuffer = loadbuf;

		if( iSize >= loadsize )
		{
			pBuffer = Hunk_TempAlloc( iSize + 1 );
		}
	}
	else if( usehunk == 5 )
	{
		pBuffer = Mem_Malloc( iSize + 1 );
	}
	else
	{
		Sys_Error( "COM_LoadFile: bad usehunk" );
	}

	if( !pBuffer )
		Sys_Error( "COM_LoadFile: not enough space for %s", path );

	*( ( byte* ) pBuffer + iSize ) = '\0';

	FS_Read( pBuffer, iSize, hFile );
	FS_Close( hFile );

	if( pLength )
		*pLength = iSize;

	return ( byte * ) pBuffer;
}

void COM_FreeFile( void *buffer )
{
	g_engdstAddrs.COM_FreeFile( &buffer );

	if( buffer )
		Mem_Free( buffer );
}

byte *COM_LoadHunkFile (const char *path)
{
	return COM_LoadFile (path, 1, nullptr);
}

byte *COM_LoadTempFile (const char* path, int* pLength)
{
	return COM_LoadFile (path, 2, pLength);
}

void COM_LoadCacheFile (const char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	COM_LoadFile (path, 3, nullptr);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize, int *length)
{
	loadbuf = reinterpret_cast<byte*>( buffer );
	loadsize = bufsize;

	return COM_LoadFile( path, 4, length );
}

byte* COM_LoadFileForMe( const char* filename, int* pLength )
{
	return COM_LoadFile( filename, 5, pLength );
}

byte* COM_LoadFileLimit( const char* path, int pos, int cbmax, int* pcbread, FileHandle_t* phFile )
{
	auto hFile = *phFile;

	if( FILESYSTEM_INVALID_HANDLE == *phFile )
	{
		hFile = FS_Open( path, "rb" );
	}

	byte* pData = nullptr;

	if( FILESYSTEM_INVALID_HANDLE != hFile )
	{
		const auto uiSize = FS_Size( hFile );

		if( uiSize < static_cast<decltype( uiSize )>( pos ) )
			Sys_Error( "COM_LoadFileLimit: invalid seek position for %s", path );

		FS_Seek( hFile, pos, FILESYSTEM_SEEK_HEAD );

		*pcbread = min( cbmax, static_cast<int>( uiSize ) );

		char base[ 32 ];

		if( path )
		{
			COM_FileBase( path, base );
		}

		pData = reinterpret_cast<byte*>( Hunk_TempAlloc( *pcbread + 1 ) );

		if( pData )
		{
			pData[ uiSize ] = '\0';

			FS_Read( pData, uiSize, hFile );
		}
		else
		{
			if( path )
				Sys_Error( "COM_LoadFileLimit: not enough space for %s", path );

			FS_Close( hFile );
		}
	}

	*phFile = hFile;

	return pData;
}


/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t *COM_LoadPackFile (char *packfile)
{
	dpackheader_t   header;
	int                             i;
	packfile_t              *newfiles;
	int                             numpackfiles;
	pack_t                  *pack;
	int                             packhandle;
	dpackfile_t             info[MAX_FILES_IN_PACK];
	unsigned short          crc;

	if (Sys_FileOpenRead (packfile, &packhandle) == -1)
	{
//              Con_Printf ("Couldn't open %s\n", packfile);
		return NULL;
	}
	Sys_FileRead (packhandle, (void *)&header, sizeof(header));
	if (header.id[0] != 'P' || header.id[1] != 'A'
	|| header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %i files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = true;    // not the original file

	newfiles = Hunk_AllocName (numpackfiles * sizeof(packfile_t), "packfile");

	Sys_FileSeek (packhandle, header.dirofs);
	Sys_FileRead (packhandle, (void *)info, header.dirlen);

// crc the directory to check for modifications
	CRC_Init (&crc);
	for (i=0 ; i<header.dirlen ; i++)
		CRC_ProcessByte (&crc, ((byte *)info)[i]);
	if (crc != PAK0_CRC)
		com_modified = true;

// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		strcpy (newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = Hunk_Alloc (sizeof (pack_t));
	strcpy (pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	
	Con_Printf ("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 
================
*/
void COM_AddGameDirectory (char *dir)
{
	int                             i;
	searchpath_t    *search;
	pack_t                  *pak;
	char                    pakfile[MAX_OSPATH];

	strcpy (com_gamedir, dir);

//
// add the directory to the search path
//
	search = Hunk_Alloc (sizeof(searchpath_t));
	strcpy (search->filename, dir);
	search->next = com_searchpaths;
	com_searchpaths = search;

//
// add any pak files in the format pak0.pak pak1.pak, ...
//
	for (i=0 ; ; i++)
	{
		sprintf (pakfile, "%s/pak%i.pak", dir, i);
		pak = COM_LoadPackFile (pakfile);
		if (!pak)
			break;
		search = (searchpath_t*)Hunk_Alloc (sizeof(searchpath_t));
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;               
	}

//
// add the contents of the parms.txt file to the end of the command line
//

}

char* COM_BinPrintf( byte* buf, int nLen )
{
	static char szReturn[ 4096 ];

	memset( szReturn, 0, sizeof( szReturn ) );

	char szChunk[ 10 ];
	for( int i = 0; i < nLen; ++i )
	{
		snprintf( szChunk, ARRAYSIZE( szChunk ), "%02x", buf[ i ] );

		strncat( szReturn, szChunk, ARRAYSIZE( szReturn ) - 1 - strlen( szReturn ) );
	}

	return szReturn;
}

unsigned char COM_Nibble( char c )
{
	if( ( c >= '0' ) &&
		( c <= '9' ) )
	{
		return ( unsigned char ) ( c - '0' );
	}

	if( ( c >= 'A' ) &&
		( c <= 'F' ) )
	{
		return ( unsigned char ) ( c - 'A' + 0x0a );
	}

	if( ( c >= 'a' ) &&
		( c <= 'f' ) )
	{
		return ( unsigned char ) ( c - 'a' + 0x0a );
	}

	return '0';
}

void COM_HexConvert( const char* pszInput, int nInputLength, byte* pOutput )
{
	const auto iBytes = ( ( max( nInputLength - 1, 0 ) ) / 2 ) + 1;

	auto p = pszInput;

	for( int i = 0; i < iBytes; ++i, ++p )
	{
		pOutput[ i ] = ( 0x10 * COM_Nibble( p[ 0 ] ) ) | COM_Nibble( p[ 1 ] );
	}
}

void COM_NormalizeAngles( vec_t* angles )
{
	for( int i = 0; i < 3; ++i )
	{
		if( angles[ i ] < -180.0 )
		{
			angles[ i ] = fmod( angles[ i ], 360.0 ) + 360.0;
		}
		else if( angles[ i ] > 180.0 )
		{
			angles[ i ] = fmod( angles[ i ], 360.0 ) - 360.0;
		}
	}
}

int COM_EntsForPlayerSlots( int nPlayers )
{
	int num_edicts = gmodinfo.num_edicts;

	const auto parm = COM_CheckParm( "-num_edicts" );

	if( parm != 0 )
	{
		const auto iSetting = atoi( com_argv[ parm + 1 ] );

		if( num_edicts <= iSetting )
			num_edicts = iSetting;
	}

	//TODO: this can exceed MAX_EDICTS - Solokiller
	return num_edicts + 15 * ( nPlayers - 1 );
}

void COM_ExplainDisconnection( qboolean bPrint, const char* fmt, ... )
{
	static char string[ 1024 ];

	va_list va;

	va_start( va, fmt );
	vsnprintf( string, ARRAYSIZE( string ), fmt, va );
	va_end( va );

	strncpy( gszDisconnectReason, string, ARRAYSIZE( gszDisconnectReason ) - 1 );
	gszDisconnectReason[ ARRAYSIZE( gszDisconnectReason ) - 1 ] = '\0';

	gfExtendedError = true;

	if( bPrint )
	{
		if( gszDisconnectReason[ 0 ] != '#' )
			Con_Printf( "%s\n", gszDisconnectReason );
	}
}

void COM_ExtendedExplainDisconnection( qboolean bPrint, const char* fmt, ... )
{
	static char string[ 1024 ];

	va_list va;

	va_start( va, fmt );
	vsnprintf( string, ARRAYSIZE( string ), fmt, va );
	va_end( va );

	strncpy( gszExtendedDisconnectReason, string, ARRAYSIZE( gszExtendedDisconnectReason ) - 1 );
	gszExtendedDisconnectReason[ ARRAYSIZE( gszExtendedDisconnectReason ) - 1 ] = '\0';

	if( bPrint )
	{
		if( gszExtendedDisconnectReason[ 0 ] != '#' )
			Con_Printf( "%s\n", gszExtendedDisconnectReason );
	}
}