/*
 *	This file is part of OGS Engine
 *	Copyright (C) 2015-2017 OGS Dev Team
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
/// @brief old api game module wrapper

#include "precompiled.hpp"
#include "game/HLGame.hpp"
#include <cstring>

CHLGame::CHLGame(CSystem *apSystem) : mpSystem(apSystem)
{
	mpFuncs = new DLL_FUNCTIONS;
	mpNewFuncs = new NEW_DLL_FUNCTIONS;
	
	// Make sure that new dll functions is cleared
	//memset(&mpNewFuncs, 0, sizeof(mpNewFuncs));
};

CHLGame::~CHLGame()
{
	if(mpFuncs)
		delete mpFuncs;
	
	if(mpNewFuncs)
		delete mpNewFuncs;
	
	mpFuncs = nullptr;
	mpNewFuncs = nullptr;
};

bool CHLGame::Load(const APIFUNCTION &afnGetEntityAPI, const APIFUNCTION2 &afnGetEntityAPI2, const NEW_DLL_FUNCTIONS_FN &afnGetNewDllFuncs)
{
	int nVersion;
	
	// Get extended callbacks
	if(afnGetNewDllFuncs)
	{
		nVersion = NEW_DLL_FUNCTIONS_VERSION;
		
		if(!afnGetNewDllFuncs(mpNewFuncs, &nVersion))
		{
			if(nVersion != NEW_DLL_FUNCTIONS_VERSION)
				DevWarning("SV_LoadProgs: new interface version is %d, should be %d", NEW_DLL_FUNCTIONS_VERSION, nVersion);
			
			memset(&mpNewFuncs, 0, sizeof(mpNewFuncs));
		};
	};
	
	nVersion = INTERFACE_VERSION;
	
	if(afnGetEntityAPI2)
	{
		if(!afnGetEntityAPI2(mpFuncs, &nVersion))
		{
			DevWarning("SV_LoadProgs: interface version is %d, should be %d", INTERFACE_VERSION, nVersion);
			
			// Fallback to old API
			if(!afnGetEntityAPI(mpFuncs, nVersion))
				return false;
		}
		else
			DevMsg(eMsgType_AIConsole, "SV_LoadProgs: ^2initailized extended EntityAPI ^7ver. %d", nVersion);
	}
	else
		if(!afnGetEntityAPI(mpFuncs, nVersion))
			return false;
	
	return true;
};

bool CHLGame::Init(CreateInterfaceFn afnEngineFactory)
{
	DevMsg("Initializing the old api game component...");
	
	mpSystem->AddListener(mpHLGameListener); // add a system event listener
	
	mpFuncs->pfnGameInit();
	return true;
};

void CHLGame::Shutdown()
{
	if(mpNewFuncs)
		mpNewFuncs->pfnGameShutdown();
};

// Since the new engine arch isn't oriented on old api
// We need to listen to some events in order to react to them
// Engine is just broadcasting events from now so everybody can 
// listen and react to them (gamedll/plugins/other modules)
void CHLGame::OnEvent(const TEvent &aEvent)
{
	switch(aEvent.type)
	{
	case SystemEvent::Type::Error:
		HandleSysError(aEvent.SysError.asMsg);
		break;
	case UnsortedEvent::Type::LevelInit:
		LevelInit(aEvent.LevelInitData);
		break;
	case UnsortedEvent::Type::LevelShutdown:
		LevelShutdown();
		break;
	default:
		break;
	};
};

const char *CHLGame::GetGameDescription()
{
	return mpFuncs->pfnGetGameDescription();
};

bool CHLGame::AllowLagCompensation()
{
	return mpFuncs->pfnAllowLagCompensation() ? true : false;
};

bool CHLGame::LevelInit(char const *asMapName, char const *asMapEntities, char const *asOldLevel, char const *asLandmarkName, bool abLoadGame, bool abBackground)
{
	return true;
};

void CHLGame::LevelShutdown()
{
};

void CHLGame::RegisterEncoders()
{
	mpFuncs->pfnRegisterEncoders();
};

void CHLGame::PostInit()
{
};

void CHLGame::HandleSysError(const char *asMsg)
{
	mpFuncs->pfnSys_Error(asMsg);
};

void CHLGame::Update()
{
	mpFuncs->pfnStartFrame();
};

void CHLGame::OnServerActivate(edict_t *apEdictList, uint anEdictCount, uint anMaxPlayers)
{
	mpFuncs->pfnServerActivate(apEdictList, anEdictCount, anMaxPlayers);
};

void CHLGame::OnServerDeactivate()
{
	mpFuncs->pfnServerDeactivate();
};

void CHLGame::OnNewLevel()
{
	mpFuncs->pfnParmsNewLevel();
};

void CHLGame::OnChangeLevel()
{
	mpFuncs->pfnParmsChangeLevel();
};

int CHLGame::OnConnectionlessPacket(const netadr_t *apFrom, const char *asArgs, char *asResponseBuffer, int *anBufferSize)
{
	return mpFuncs->pfnConnectionlessPacket(apFrom, asArgs, asResponseBuffer, anBufferSize);
};

void CHLGame::SaveWriteFields(SAVERESTOREDATA *apSaveRestoreData, const char *as, void *ap, TYPEDESCRIPTION *apTypeDesc, int an)
{
	mpFuncs->pfnSaveWriteFields(apSaveRestoreData, as, ap, apTypeDesc, an);
};

void CHLGame::SaveReadFields(SAVERESTOREDATA *apSaveRestoreData, const char *as, void *ap, TYPEDESCRIPTION *apTypeDesc, int an)
{
	mpFuncs->pfnSaveReadFields(apSaveRestoreData, as, ap, apTypeDesc, an);
};

void CHLGame::OnSaveGlobalState(SAVERESTOREDATA *apSaveRestoreData)
{
	mpFuncs->pfnSaveGlobalState(apSaveRestoreData);
};

void CHLGame::OnRestoreGlobalState(SAVERESTOREDATA *apSaveRestoreData)
{
	mpFuncs->pfnRestoreGlobalState(apSaveRestoreData);
};

void CHLGame::OnResetGlobalState()
{
	mpFuncs->pfnResetGlobalState();
};

int CHLGame::AddToFullPack(entity_state_t *apEntityState, int anE, edict_t *apEnt, edict_t *apHost, int anHostFlags, int anPlayer, unsigned char *apSet)
{
	return mpFuncs->pfnAddToFullPack(apEntityState, anE, apEnt, apHost, anHostFlags, anPlayer, apSet);
};

void CHLGame::CreateBaseline(int anPlayer, int anEntIndex, entity_state_t *apBaseline, edict_t *apEntity, int anPlayerModelIndex, vec3_t avPlayerMins, vec3_t avPlayerMaxs)
{
	mpFuncs->pfnCreateBaseline(anPlayer, anEntIndex, apBaseline, apEntity, anPlayerModelIndex, avPlayerMins, avPlayerMaxs);
};

void CHLGame::CreateInstancedBaselines()
{
	mpFuncs->pfnCreateInstancedBaselines();
};

int CHLGame::GetHullBounds(int anHullNumber, float *afMins, float *afMaxs)
{
	return mpFuncs->pfnGetHullBounds(anHullNumber, afMins, afMaxs);
};