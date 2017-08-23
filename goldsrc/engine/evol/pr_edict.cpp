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

// sv_edict.c -- entity dictionary

//#include "precompiled.h"
#include "quakedef.h"
//#include "pr_edict.h"
//#include "progs.h"

//char *pr_strings;
//globalvars_t gGlobalVariables;

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict(edict_t *e)
{
	Q_memset(&e->v, 0, sizeof(e->v));
	e->free = false;
	
	ReleaseEntityDLLFields(e);
	InitEntityDLLFields(e);
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc()
{
	int i;
	edict_t *e;

	// Search for free entity
	for (i = svs.maxclients + 1; i < sv.num_edicts; i++)
	{
		e = &sv.edicts[i]; // EDICT_NUM(i)
		
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && (e->freetime <= 2.0 || sv.time - e->freetime >= 0.5))
		{
			ED_ClearEdict(e);
			return e;
		}
	}

	// Check if we are out of free edicts
	if (i >= sv.max_edicts)
	{
		if (!sv.max_edicts)
			Sys_Error("%s: no edicts yet", __func__);
		
		Sys_Error("%s: no free edicts", __func__); // Con_Printf in qw
	}

	// Use new one
	++sv.num_edicts;
	e = &sv.edicts[i]; // EDICT_NUM(i)

	ED_ClearEdict(e);
	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(edict_t *ed)
{
	if(ed->free)
		return;

	// unlink from world bsp
	SV_UnlinkEdict(ed);
	
	FreeEntPrivateData(ed);
	
	ed->serialnumber++;
	
	ed->freetime = (float)sv.time;
	ed->free = true;
	
	ed->v.flags = 0;
	ed->v.model = 0;

	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	ed->v.scale = 0;
	ed->v.gravity = 0;
	ed->v.nextthink = -1.0;
	ed->v.solid = SOLID_NOT;

	//VectorCopy(vec3_origin, ed->v.origin);
	//VectorCopy(vec3_origin, ed->v.angles);
	
	ed->v.origin[0] = vec3_origin[0];
	ed->v.origin[1] = vec3_origin[1];
	ed->v.origin[2] = vec3_origin[2];
	
	ed->v.angles[0] = vec3_origin[0];
	ed->v.angles[1] = vec3_origin[1];
	ed->v.angles[2] = vec3_origin[2];
}

//===========================================================================

/*
=============
ED_Count

For debugging
=============
*/
/*NOXREF*/ void ED_Count()
{
	//NOXREFCHECK;

	int i;
	edict_t *ent;
	int active = 0, models = 0, solid = 0, step = 0;

	for (i = 0; i < sv.num_edicts; i++)
	{
		ent = &sv.edicts[i]; // EDICT_NUM(i)
		
		if (!ent->free)
		{
			++active;
			
			models += (ent->v.model) ? 1 : 0;
			solid += (ent->v.solid) ? 1 : 0;
			step += (ent->v.movetype == MOVETYPE_STEP) ? 1 : 0;
		}
	}

	Con_Printf("num_edicts:%3i\n", sv.num_edicts);
	Con_Printf("active    :%3i\n", active);
	Con_Printf("view      :%3i\n", models);
	Con_Printf("touch     :%3i\n", solid);
	Con_Printf("step      :%3i\n", step);
}

//============================================================================


/*
=============
ED_NewString
=============
*/
char *ED_NewString(const char *string)
{
	char *new_s;

	// Engine string pooling
#ifdef REHLDS_FIXES

	// escaping is done inside Ed_StrPool_Alloc()
	new_s = Ed_StrPool_Alloc(string);

#else // REHLDS_FIXES

	int l = Q_strlen(string);
	new_s = (char *)Hunk_Alloc(l + 1);
	char* new_p = new_s;

	for (int i = 0; i < l; i++, new_p++)
	{
		if (string[i] == '\\')
		{
			if (string[i + 1] == 'n')
				*new_p = '\n';
			else
				*new_p = '\\';
			i++;
		}
		else
			*new_p = string[i];
	}
	
	*new_p = 0;

#endif // REHLDS_FIXES

	return new_s;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char *ED_ParseEdict(char *data, edict_t *ent)
{
	qboolean init = false;
	char keyname[256];
	int n;
	ENTITYINIT pEntityInit;
	char *className;
	KeyValueData kvd;

	// clear it
	if (ent != sv.edicts) // hack
		Q_memset(&ent->v, 0, sizeof(ent->v));

	InitEntityDLLFields(ent);

	if (SuckOutClassname(data, ent))
	{
		className = (char *)(pr_strings + ent->v.classname);

		pEntityInit = GetEntityInit(className);
		if (pEntityInit)
		{
			pEntityInit(&ent->v);
			init = true;
		}
		else
		{
			pEntityInit = GetEntityInit("custom");
			if (pEntityInit)
			{
				pEntityInit(&ent->v);
				kvd.szClassName = "custom";
				kvd.szKeyName = "customclass";
				kvd.szValue = className;
				kvd.fHandled = false;
				gEntityInterface.pfnKeyValue(ent, &kvd);
				init = true;
			}
			else
			{
				Con_DPrintf("Can't init %s\n", className);
				init = false;
			}
		}

		// go through all the dictionary pairs
		while (1)
		{
			// parse key
			data = COM_Parse(data);
			
			if (com_token[0] == '}')
				break;
			
			if (!data)
				Host_Error("%s: EOF without closing brace", __func__); // Sys_Error in qw

			Q_strncpy(keyname, com_token, ARRAYSIZE(keyname) - 1);
			keyname[ARRAYSIZE(keyname) - 1] = 0;
			
			// Remove tail spaces
			for (n = Q_strlen(keyname) - 1; n >= 0 && keyname[n] == ' '; n--)
				keyname[n] = 0;

			// parse value	
			data = COM_Parse(data);
			
			if (!data)
				Host_Error("%s: EOF without closing brace", __func__); // Sys_Error in qw
			
			if (com_token[0] == '}')
				Host_Error("%s: closing brace without data", __func__); // Sys_Error in qw

			if (className != NULL && !Q_strcmp(className, com_token))
				continue;

			if (!Q_strcmp(keyname, "angle"))
			{
				float value = (float)Q_atof(com_token);
				if (value >= 0.0)
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "%f %f %f", ent->v.angles[0], value, ent->v.angles[2]);
				}
				else if ((int)value == -1)
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "-90 0 0");
				}
				else
				{
					Q_snprintf(com_token, COM_TOKEN_LEN, "90 0 0");
				}

				Q_strcpy(keyname, "angles");
			}

			kvd.szClassName = className;
			kvd.szKeyName = keyname;
			kvd.szValue = com_token;
			kvd.fHandled = 0;
			gEntityInterface.pfnKeyValue(ent, &kvd);
		}
	}

	if (!init)
	{
		ent->free = 1;
		ent->serialnumber++;
	}
	return data;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(char *data)
{
	edict_t *ent;
	int inhibit;

	gGlobalVariables.time = (float)sv.time;

	ent = NULL;
	inhibit = 0;
	
	// parse ents
	while (1)
	{
		// parse the opening brace	
		data = COM_Parse(data);
		
		if (!data)
			break;
		
		if (com_token[0] != '{')
			Host_Error("%s: found %s when expecting {", __func__, com_token); // Sys_Error

		if (ent)
			ent = ED_Alloc();
		else
		{
			ent = sv.edicts; // EDICT_NUM(0)
			ReleaseEntityDLLFields(sv.edicts);	// TODO: May be better to call ED_ClearEdict here?
			InitEntityDLLFields(ent);
		}

		data = ED_ParseEdict(data, ent);
		
		if (ent->free)
			continue;

		// remove things from different skill levels or deathmatch
		if (deathmatch.value != 0.0 && (ent->v.spawnflags & SF_NOTINDEATHMATCH))
		{
			ED_Free(ent);
			++inhibit;
			//continue;
		}
		else
		{
			//
			// immediately call spawn function
			//
			if (ent->v.classname)
			{
				if (gEntityInterface.pfnSpawn(ent) < 0 || (ent->v.flags & FL_KILLME))
					ED_Free(ent);
			}
			else
			{
				Con_Printf("No classname for:\n"); // NOTE: WTF??
				//ED_Print(ent); // Should be here???
				ED_Free(ent);
				//continue;
			}
		}
	}
	
	Con_DPrintf("%i entities inhibited\n", inhibit);
}

/*
===============
PR_Init
===============
*/
/*NOXREF*/ void PR_Init()
{
	//NOXREFCHECK;
	
	//Cmd_AddCommand ("edictcount", ED_Count); // not present
}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Sys_Error("%s: bad number %i", __func__, n);

	return &sv.edicts[n];
}

int NUM_FOR_EDICT(const edict_t *e)
{
	int b = e - sv.edicts;

	if (b < 0 || b >= sv.num_edicts)
		Sys_Error("%s: bad pointer", __func__);

	return b;
}

bool SuckOutClassname(char *szInputStream, edict_t *pEdict)
{
	char szKeyName[256];
	KeyValueData kvd;

	// key
	szInputStream = COM_Parse(szInputStream);
	while (szInputStream && com_token[0] != '}')
	{
		Q_strncpy(szKeyName, com_token, ARRAYSIZE(szKeyName) - 1);
		szKeyName[ARRAYSIZE(szKeyName) - 1] = 0;

		// value
		szInputStream = COM_Parse(szInputStream);

		if (!Q_strcmp(szKeyName, "classname"))
		{
			kvd.szClassName = NULL;
			kvd.szKeyName = szKeyName;
			kvd.szValue = com_token;
			kvd.fHandled = false;

			gEntityInterface.pfnKeyValue(pEdict, &kvd);

			if (kvd.fHandled == false)
			{
				Host_Error("%s: parse error", __func__);
			}

			return true;
		}

		if (!szInputStream)
		{
			break;
		}

		// next key
		szInputStream = COM_Parse(szInputStream);
	}

#ifdef REHLDS_FIXES
	if (pEdict == sv.edicts)
	{
		kvd.szClassName = NULL;
		kvd.szKeyName = "classname";
		kvd.szValue = "worldspawn";
		kvd.fHandled = false;

		gEntityInterface.pfnKeyValue(pEdict, &kvd);

		if (kvd.fHandled == false)
		{
			Host_Error("%s: parse error", __func__);
		}

		return true;
	}
#endif

	// classname not found
	return false;
}

void ReleaseEntityDLLFields(edict_t *pEdict)
{
	FreeEntPrivateData(pEdict);
}

void InitEntityDLLFields(edict_t *pEdict)
{
	pEdict->v.pContainingEntity = pEdict;
}

void* /*EXT_FUNC*/ PvAllocEntPrivateData(edict_t *pEdict, int32 cb)
{
	FreeEntPrivateData(pEdict);

	if (cb <= 0)
	{
		return NULL;
	}

	pEdict->pvPrivateData = Mem_Calloc(1, cb);

#ifdef REHLDS_FLIGHT_REC
	if (rehlds_flrec_pvdata.string[0] != '0') {
		FR_AllocEntPrivateData(pEdict->pvPrivateData, cb);
	}
#endif // REHLDS_FLIGHT_REC

	return pEdict->pvPrivateData;
}

void* /*EXT_FUNC*/ PvEntPrivateData(edict_t *pEdict)
{
	if (!pEdict)
	{
		return NULL;
	}

	return pEdict->pvPrivateData;
}

void /*EXT_FUNC*/ FreeEntPrivateData(edict_t *pEdict)
{
	if (pEdict->pvPrivateData)
	{
		if (gNewDLLFunctions.pfnOnFreeEntPrivateData)
			gNewDLLFunctions.pfnOnFreeEntPrivateData(pEdict);

#ifdef REHLDS_FLIGHT_REC
		if (rehlds_flrec_pvdata.string[0] != '0')
			FR_FreeEntPrivateData(pEdict->pvPrivateData);
#endif // REHLDS_FLIGHT_REC

		Mem_Free(pEdict->pvPrivateData);
		//pEdict->pvPrivateData = NULL;
	}
	
	pEdict->pvPrivateData = NULL;
}

void FreeAllEntPrivateData()
{
	for (int i = 0; i < sv.num_edicts; i++)
	{
		FreeEntPrivateData(&sv.edicts[i]);
	}
}

edict_t* /*EXT_FUNC*/ PEntityOfEntOffset(int iEntOffset)
{
	return (edict_t *)((char *)sv.edicts + iEntOffset);
}

int /*EXT_FUNC*/ EntOffsetOfPEntity(const edict_t *pEdict)
{
	return (char *)pEdict - (char *)sv.edicts;
}

int /*EXT_FUNC*/ IndexOfEdict(const edict_t *pEdict)
{
	int index = 0;
	if (pEdict)
	{
		index = pEdict - sv.edicts;
#ifdef REHLDS_FIXES
		if (index < 0 || index >= sv.max_edicts) // max_edicts is not valid entity index
#else // REHLDS_FIXES
		if (index < 0 || index > sv.max_edicts)
#endif // REHLDS_FIXES
		{
			Sys_Error("%s: bad entity", __func__);
		}
	}
	return index;
}

edict_t* /*EXT_FUNC*/ PEntityOfEntIndex(int iEntIndex)
{
	if (iEntIndex < 0 || iEntIndex >= sv.max_edicts)
	{
		return NULL;
	}

	edict_t *pEdict = EDICT_NUM(iEntIndex);

#ifdef REHLDS_FIXES
	if (pEdict && (pEdict->free || (iEntIndex > svs.maxclients && !pEdict->pvPrivateData))) // Simplified confition
	{
		return NULL;
	}
#else // REHLDS_FIXES
	if ((!pEdict || pEdict->free || !pEdict->pvPrivateData) && (iEntIndex >= svs.maxclients || pEdict->free))
	{
		if (iEntIndex >= svs.maxclients || pEdict->free)
		{
			pEdict = NULL;
		}
	}
#endif // REHLDS_FIXES

	return pEdict;
}

const char* /*EXT_FUNC*/ SzFromIndex(int iString)
{
	return (const char *)(pr_strings + iString);
}

entvars_t* /*EXT_FUNC*/ GetVarsOfEnt(edict_t *pEdict)
{
	return &pEdict->v;
}

edict_t* /*EXT_FUNC*/ FindEntityByVars(entvars_t *pvars)
{
	for (int i = 0; i < sv.num_edicts; i++)
	{
		edict_t *pEdict = &sv.edicts[i];
		if (&pEdict->v == pvars)
		{
			return pEdict;
		}
	}
	return NULL;
}

float /*EXT_FUNC*/ CVarGetFloat(const char *szVarName)
{
	return Cvar_VariableValue(szVarName);
}

const char* /*EXT_FUNC*/ CVarGetString(const char *szVarName)
{
	return Cvar_VariableString(szVarName);
}

cvar_t* /*EXT_FUNC*/ CVarGetPointer(const char *szVarName)
{
	return Cvar_FindVar(szVarName);
}

void /*EXT_FUNC*/ CVarSetFloat(const char *szVarName, float flValue)
{
	Cvar_SetValue(szVarName, flValue);
}

void /*EXT_FUNC*/ CVarSetString(const char *szVarName, const char *szValue)
{
	Cvar_Set(szVarName, szValue);
}

void /*EXT_FUNC*/ CVarRegister(cvar_t *pCvar)
{
	if (pCvar)
	{
		pCvar->flags |= FCVAR_EXTDLL;
		Cvar_RegisterVariable(pCvar);
	}
}

int /*EXT_FUNC*/ AllocEngineString(const char *szValue)
{
	return ED_NewString(szValue) - pr_strings;
}

void /*EXT_FUNC*/ SaveSpawnParms(edict_t *pEdict)
{
	int eoffset = NUM_FOR_EDICT(pEdict);
	if (eoffset < 1 || eoffset > svs.maxclients)
	{
		Host_Error("%s: Entity is not a client", __func__);
	}
	// Nothing more for this function even on client-side
}

void* /*EXT_FUNC*/ GetModelPtr(edict_t *pEdict)
{
	if (!pEdict)
	{
		return NULL;
	}

	return Mod_Extradata(Mod_Handle(pEdict->v.modelindex));
}