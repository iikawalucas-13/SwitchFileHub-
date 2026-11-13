#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <switch.h>

Result ftpServerStart(void);
void ftpServerStop(void);
int ftpServerState(void);
const char *ftpServerIp(void);

typedef struct {
    u32 active;
    u64 completed;
    u64 failed;
    u64 bytes_sent;
    u64 bytes_received;
} FtpTransferStats;

void ftpServerGetStats(FtpTransferStats *out);

#ifdef __cplusplus
}
#endif
