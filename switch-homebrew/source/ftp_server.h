#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <switch.h>

Result ftpServerStart(void);
void ftpServerStop(void);
int ftpServerState(void);
const char *ftpServerIp(void);

#ifdef __cplusplus
}
#endif
