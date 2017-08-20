#pragma once

#include "pm_defs.h"

void CL_SetSolidPlayers( int playernum );

void CL_SetUpPlayerPrediction( bool dopred, bool bIncludeLocalClient );

void CL_AddStateToEntlist( physent_t* pe, entity_state_t* state );