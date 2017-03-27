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
/// @brief primary header for host

#pragma once

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

//=============================================================================

#include <memory>
#include "system/GameServer.hpp"

typedef struct quakeparms_s
{
	char *basedir;
	char *cachedir; // for development over ISDN lines

	int argc;
	char **argv;

	void *membase;
	int memsize;
} quakeparms_t;

struct IConsole;
class CFileSystem;

class CHost
{
public:
	int Init(quakeparms_t *parms);
	void InitLocal();
	
	void Shutdown();
	
	void _Frame(float time);
	int Frame(float time, int iState, int *stateInfo);
	
	void EndGame(const char *message, ...);

	void NORETURN Error(const char *error, ...);

	void WriteConfig();
	void WriteCustomConfig();

	void ClientCommands(const char *fmt, ...);
	void ClearClients(bool bFramesOnly);
	void ShutdownServer(bool crash);

	void CheckDynamicStructures();
	void ClearMemory(bool bQuiet);
	bool FilterTime(float time);

	void ComputeFPS(double frametime);
	void GetInfo(float *fps, int *nActive, int *unused, int *nMaxPlayers, char *pszMap);
	void PrintSpeeds(double *time); // CalcSpeeds

	void UpdateScreen();
	void UpdateSounds();

	void CheckConnectionFailure();

	void CheckGore();

	bool IsSinglePlayerGame();
	bool IsServerActive();

	void PrintVersion();
	
	bool IsInitialized() const {return host_initialized;}
	
	int GetStartTime();
private:
	void ClearIOStates();
	
	void InitCommands();
	
	std::unique_ptr<IConsole> mpConsole;
	//std::unique_ptr<CCmdBuffer> mpCmdBuffer;
	
	//std::unique_ptr<CNetwork> mpNetwork;
	
	//std::unique_ptr<ISound> mpSound;
	
	std::unique_ptr<CGameServer> mpServer;
	
	CFileSystem *mpFS{nullptr};

	quakeparms_t *host_params{nullptr};
	
	bool host_initialized{false}; // true if into command execution
	
	double realtime{0.0f};  // without any filtering or bounding;
							// not bounded in any way, changed at
							// start of every frame, never reset
	double oldrealtime{0.0f}; // last frame run
	
	double host_frametime{0.0f};
	
	double rolling_fps{0.0f};
	
	int32 startTime{0};
	
	int host_framecount{0}; // incremented every frame, never reset
	
	int host_hunklevel{0};

	//jmp_buf host_abortserver;
	//jmp_buf host_enddemo;
};