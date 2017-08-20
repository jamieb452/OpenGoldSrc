/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#pragma once

#include "common/mathlib.h"
#include "common/const.h"
#include "common/enums.h" // netsrc_t
#include "common/netadr.h"
#include "common.h"

extern char gDownloadFile[ 256 ];

extern int net_drop; // packets dropped before this one

extern cvar_t net_log;

extern cvar_t net_showpackets;
extern cvar_t net_showdrop;

extern cvar_t net_drawslider;
extern cvar_t net_chokeloopback;

extern cvar_t sv_filetransfercompression;
extern cvar_t sv_filetransfermaxsize;

typedef struct fragbuf_s fragbuf_t;
typedef struct fragbufwaiting_s fragbufwaiting_t;

// Message data
typedef struct flowstats_s
{
	// Size of message sent/received
	int size;
	// Time that message was sent/received
	double time;
} flowstats_t;

const int MAX_LATENT = 32;

typedef struct flow_s
{
	// Data for last MAX_LATENT messages
	flowstats_t stats[MAX_LATENT];
	// Current message position
	int current;
	// Time when we should recompute k/sec data
	double nextcompute;
	// Average data
	float kbytespersec;
	float avgkbytespersec;
} flow_t;

typedef struct netchan_s
{
	netsrc_t sock;
	
	netadr_t remote_address;
	
	int player_slot;
	
	float last_received; // for timeouts
	float connect_time;

// bandwidth estimator
	double rate; // seconds / byte
	double cleartime; // if realtime > nc->cleartime, free to go

// sequencing variables
	int incoming_sequence;
	int incoming_acknowledged;
	
	int incoming_reliable_acknowledged; // single bit
	int incoming_reliable_sequence; // single bit, maintained local
	
	int outgoing_sequence;
	
	int reliable_sequence; // single bit
	int last_reliable_sequence; // sequence number of last send
	
	void *connection_status;
	
	int( *pfnNetchan_Blocksize )( void * );

// reliable staging and holding areas
	sizebuf_t message; // writing buffer to send to server
	byte message_buf[ 3990 ];
	
	int reliable_length;
	byte reliable_buf[ 3990 ];
	
	fragbufwaiting_t *waitlist[ 2 ];
	
	int reliable_fragment[ 2 ];
	unsigned int reliable_fragid[ 2 ];
	
	fragbuf_t *fragbufs[ 2 ];
	int fragbufcount[ 2 ];
	
	__int16 frag_startpos[ 2 ];
	__int16 frag_length[ 2 ];
	
	fragbuf_t *incomingbufs[ 2 ];
	qboolean incomingready[ 2 ];
	
	char incomingfilename[ 260 ];
	
	void *tempbuffer;
	int tempbuffersize;
	
	flow_t flow[ 2 ];
} netchan_t;

void Netchan_Init();

void Netchan_Setup(netsrc_t socketnumber, netchan_t *chan, netadr_t adr, int player_slot, void *connection_status, qboolean(*pfnNetchan_Blocksize)(void *));

void Netchan_UpdateFlow(netchan_t *chan);
/*NOXREF*/ void Netchan_UpdateProgress(netchan_t *chan);

void Netchan_Transmit(netchan_t *chan, int length, byte *data);

void Netchan_OutOfBand(netsrc_t sock, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint(netsrc_t sock, netadr_t adr, char *format, ...);

qboolean Netchan_Process(netchan_t *chan);

qboolean Netchan_CanPacket(netchan_t *chan);
qboolean Netchan_CanReliable(netchan_t *chan);

qboolean Netchan_Validate(netchan_t *chan, qboolean *frag_message, unsigned int *fragid, int *frag_offset, int *frag_length);

void Netchan_CreateFragments_(qboolean server, netchan_t *chan, sizebuf_t *msg);
void Netchan_CreateFragments(qboolean server, netchan_t *chan, sizebuf_t *msg);

void Netchan_CreateFileFragmentsFromBuffer(qboolean server, netchan_t *chan, const char *filename, unsigned char *uncompressed_pbuf, int uncompressed_size);
int Netchan_CreateFileFragments(qboolean server, netchan_t *chan, const char *filename);

#ifdef REHLDS_FIXES
int Netchan_CreateFileFragments_(qboolean server, netchan_t *chan, const char *filename);
#endif

qboolean Netchan_CopyNormalFragments(netchan_t *chan);
qboolean Netchan_CopyFileFragments(netchan_t *chan);

void Netchan_ClearFragbufs(fragbuf_t **ppbuf);
void Netchan_ClearFragments(netchan_t *chan);
void Netchan_Clear(netchan_t *chan);

void Netchan_CheckForCompletion(netchan_t *chan, int stream, int intotalbuffers);

void Netchan_UnlinkFragment(fragbuf_t *buf, fragbuf_t **list);

/*NOXREF*/ qboolean Netchan_IsSending(netchan_t *chan);
/*NOXREF*/ qboolean Netchan_IsReceiving(netchan_t *chan);

void Netchan_FlushIncoming(netchan_t *chan, int stream);
qboolean Netchan_IncomingReady(netchan_t *chan);

/*NOXREF*/ qboolean Netchan_CompressPacket(sizebuf_t *chan);
/*NOXREF*/ qboolean Netchan_DecompressPacket(sizebuf_t *chan);

void Netchan_FragSend(netchan_t *chan);

fragbuf_t *Netchan_AllocFragbuf();

void Netchan_AddBufferToList(fragbuf_t **pplist, fragbuf_t *pbuf);
void Netchan_AddFragbufToTail(fragbufwaiting_t *wait, fragbuf_t *buf);

fragbuf_t *Netchan_FindBufferById(fragbuf_t **pplist, int id, qboolean allocate);