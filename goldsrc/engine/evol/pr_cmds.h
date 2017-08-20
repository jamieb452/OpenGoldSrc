#pragma once

#include "tier0/platform.h"

void SeedRandomNumberGenerator();

float RandomFloat( float flLow, float flHigh );
int32 RandomLong( int32 lLow, int32 lHigh );

int hudCheckParm( char* parm, char** ppnext );