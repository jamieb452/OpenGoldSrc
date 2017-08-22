#pragma once

//============================================================================

//
// PR STrings stuff
//
#define MAX_PRSTR 1024

extern char *pr_strtbl[MAX_PRSTR];
extern int num_prstr;

const char *PR_GetString(int num);
int PR_SetString(const char *s);