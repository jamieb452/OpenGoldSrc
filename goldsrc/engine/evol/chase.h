#pragma once

//
// chase
//

extern cvar_t chase_active;

void Chase_Init();
void Chase_Reset();

void Chase_Update();

void TraceLine( vec_t* start, vec_t* end, vec_t* impact );