#pragma once

#include "pmtrace.h"

int PM_PointContents( vec_t* p, int* truecontents );

int PM_WaterEntity( vec_t* p );

pmtrace_t* PM_TraceLine( float* start, float* end, int flags, int usehull, int ignore_pe );

pmtrace_t PM_PlayerTrace( vec_t* start, vec_t* end, int traceFlags, int ignore_pe );