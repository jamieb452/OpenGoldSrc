#pragma once

extern bool m_bDrawInitialized;

void Draw_DecalShutdown();

void Decal_Init();

int Draw_DecalIndex( int id );
int Draw_DecalIndexFromName( char* name );