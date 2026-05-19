#pragma once

#include <Windows.h>

extern bool Reconnect;
extern int ReconnectState;
extern int ReconnectTimer;
extern int nPersonagem;

extern char Numerica[10];

extern short Reconnect_Conn_id_leader;
extern char Reconnect_LeaderName[16];

DWORD WINAPI Timer(LPVOID lpParameter);