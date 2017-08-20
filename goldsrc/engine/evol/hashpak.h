#pragma once

#include "custom.h"

void HPAK_FlushHostQueue();

void HPAK_Init();

void HPAK_CheckIntegrity( const char* pakname );

void HPAK_AddLump( bool bUseQueue, const char* pakname, resource_t* pResource, void* pData, FileHandle_t fpSource );

int HPAK_ResourceForHash( const char* pakname, byte* hash, resource_t* pResourceEntry );