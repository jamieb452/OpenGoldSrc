#pragma once

extern cvar_t sv_log_onefile;
extern cvar_t sv_log_singleplayer;
extern cvar_t sv_logsecret;

extern cvar_t mp_logecho;
extern cvar_t mp_logfile;

void Log_Open();
void Log_Close();

void Log_Printf( const char* fmt, ... );