/*
 *	This file is part of OGS Engine
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

#pragma once

#include "common/commontypes.h"
#include "system/server.hpp"

#define CMD_MAXBACKUP 64

typedef struct command_s command_t;
typedef struct edict_s edict_t;
typedef struct client_s client_t;
typedef struct usercmd_s usercmd_t;
typedef struct pmtrace_s pmtrace_t;
typedef struct entity_state_s entity_state_t;
typedef struct cvar_s cvar_t;
typedef struct consistency_s consistency_t;
typedef struct sizebuf_s sizebuf_t;
typedef struct areanode_s areanode_t;
typedef struct packet_entities_s packet_entities_t;
typedef struct physent_s physent_t;

typedef struct sv_adjusted_positions_s
{
	int active;
	int needrelink;
	vec3_t neworg;
	vec3_t oldorg;
	vec3_t initial_correction_org;
	vec3_t oldabsmin;
	vec3_t oldabsmax;
	int deadflag;
	vec3_t temp_org;
	int temp_org_setflag;
} sv_adjusted_positions_t;

typedef struct clc_func_s
{
	unsigned char opcode;
	char *pszname;
	void (*pfnParse)(client_t *);
} clc_func_t;

#ifdef HOOK_ENGINE
#define sv_player (*psv_player)
#define clcommands (*pclcommands)
#define truepositions (*ptruepositions)
#define g_balreadymoved (*pg_balreadymoved)
#define sv_clcfuncs (*psv_clcfuncs)

#define s_LastFullUpdate (*ps_LastFullUpdate)
#define sv_edgefriction (*psv_edgefriction)
#define sv_maxspeed (*psv_maxspeed)
#define sv_accelerate (*psv_accelerate)
#define sv_footsteps (*psv_footsteps)
#define sv_rollspeed (*psv_rollspeed)
#define sv_rollangle (*psv_rollangle)
#define sv_unlag (*psv_unlag)
#define sv_maxunlag (*psv_maxunlag)
#define sv_unlagpush (*psv_unlagpush)
#define sv_unlagsamples (*psv_unlagsamples)
#define mp_consistency (*pmp_consistency)
#define sv_voiceenable (*psv_voiceenable)

#define nofind (*pnofind)

#endif // HOOK_ENGINE

extern edict_t *sv_player;
extern command_t clcommands[23];
extern sv_adjusted_positions_t truepositions[MAX_CLIENTS];
extern qboolean g_balreadymoved;

#ifdef HOOK_ENGINE
extern clc_func_t sv_clcfuncs[12];
#else
#endif

extern float s_LastFullUpdate[33];
extern cvar_t sv_edgefriction;
extern cvar_t sv_maxspeed;
extern cvar_t sv_accelerate;
extern cvar_t sv_footsteps;
extern cvar_t sv_rollspeed;
extern cvar_t sv_rollangle;
extern cvar_t sv_unlag;
extern cvar_t sv_maxunlag;
extern cvar_t sv_unlagpush;
extern cvar_t sv_unlagsamples;
extern cvar_t mp_consistency;
extern cvar_t sv_voiceenable;

extern qboolean nofind;

void SV_ParseConsistencyResponse(client_t *pSenderClient);
qboolean SV_FileInConsistencyList(const char *filename,
                                  consistency_t **ppconsist);
int SV_TransferConsistencyInfo();
int SV_TransferConsistencyInfo_internal();
void SV_SendConsistencyList(sizebuf_t *msg);
void SV_PreRunCmd();
void SV_CopyEdictToPhysent(physent_t *pe, int e, edict_t *check);
void SV_AddLinksToPM_(areanode_t *node, float *pmove_mins, float *pmove_maxs);
void SV_AddLinksToPM(areanode_t *node, vec_t *origin);
void SV_PlayerRunPreThink(edict_t *player, float time);
qboolean SV_PlayerRunThink(edict_t *ent, float frametime, double clienttimebase);
void SV_CheckMovingGround(edict_t *player, float frametime);
void SV_ConvertPMTrace(trace_t *dest, pmtrace_t *src, edict_t *ent);
void SV_ForceFullClientsUpdate();
void SV_RunCmd(usercmd_t *ucmd, int random_seed);
int SV_ValidateClientCommand(char *pszCommand);
float SV_CalcClientTime(client_t *cl);
void SV_ComputeLatency(client_t *cl);
int SV_UnlagCheckTeleport(vec_t *v1, vec_t *v2);
void SV_GetTrueOrigin(int player, vec_t *origin);
void SV_GetTrueMinMax(int player, float **fmin, float **fmax);
entity_state_t *SV_FindEntInPack(int index, packet_entities_t *pack);
void SV_SetupMove(client_t *_host_client);
void SV_RestoreMove(client_t *_host_client);
void SV_ParseStringCommand(client_t *pSenderClient);
void SV_ParseDelta(client_t *pSenderClient);
void SV_EstablishTimeBase(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds);
void SV_EstablishTimeBase_internal(client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds);
void SV_ParseMove(client_t *pSenderClient);
void SV_ParseVoiceData(client_t *cl);
void SV_IgnoreHLTV(client_t *cl);
void SV_ParseCvarValue(client_t *cl);
void SV_ParseCvarValue2(client_t *cl);
void SV_ExecuteClientMessage(client_t *cl);
qboolean SV_SetPlayer(int idnum);
void SV_ShowServerinfo_f();
void SV_SendEnts_f();
void SV_FullUpdate_f();