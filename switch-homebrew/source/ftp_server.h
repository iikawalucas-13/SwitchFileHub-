#pragma once
#include <switch.h>

Result ftpServerStart(void);
void ftpServerStop(void);
int ftpServerState(void);
const char *ftpServerIp(void);
