#pragma once

// Some functions declared here
#include "cdll_int.h"

const size_t MODULE_MAX_SIZE = 0x10000U;

void* GetModuleHandle( char* name );