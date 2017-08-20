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

// net.h -- quake's interface to the networking layer

#pragma once

#include "common/netadr.h"
#include "common/enums.h"

#define	PORT_ANY	-1

extern	netadr_t	net_local_adr;
extern	netadr_t	net_from;		// address of who sent the packet
extern	sizebuf_t	net_message;

extern	cvar_t	hostname;

extern	int		net_socket;

void		NET_Init (int port);
void		NET_Shutdown ();

void		NET_Config (qboolean multiplayer);

void		NET_Sleep(int msec);

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message);
void		NET_SendPacket(netsrc_t sock, int length, void *data, netadr_t to);

qboolean	NET_IsLocalAddress (netadr_t adr);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);

char		*NET_AdrToString (netadr_t a);
char		*NET_BaseAdrToString (netadr_t a);

qboolean	NET_StringToAdr (char *s, netadr_t *a);

//============================================================================

#define	OLD_AVG		0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

/*
#define	MAX_LATENT	32

typedef struct
{
	qboolean	fatal_error;	

// the statistics are cleared at each client begin, because
// the server connecting process gives a bogus picture of the data
	float		frame_latency;		// rolling average
	float		frame_rate;

	int			drop_count;			// dropped packets, cleared each level
	int			good_count;			// cleared each level

	int			qport;			

	byte		message_buf[MAX_MSGLEN];

	byte		reliable_buf[MAX_MSGLEN];	// unacked reliable message

// time and size data to calculate bandwidth
	int			outgoing_size[MAX_LATENT];
	double		outgoing_time[MAX_LATENT];
} netchan_t;
*/