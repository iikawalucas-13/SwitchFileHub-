#include "ftp_server.h"
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define FTP_PORT 5000
#define FTP_LINE 1024
#define FTP_IO 16384

static Thread server_thread;
static volatile bool server_running = false;
static volatile int server_state = 0; /* 0 stopped, 1 starting, 2 ready, -1 error */
static int listen_fd = -1;
static char server_ip[32] = "IP indisponivel";

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static void reply(int fd, const char *msg) {
    send_all(fd, msg, strlen(msg));
}

static bool read_line(int fd, char *out, size_t cap) {
    size_t n = 0;
    while (n + 1 < cap) {
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r <= 0) return false;
        if (c == '\n') break;
        if (c != '\r') out[n++] = c;
    }
    out[n] = 0;
    return true;
}

/* FTP paths are confined to sdmc:/; dot-dot and device prefixes are rejected. */
static bool ftp_path(const char *cwd, const char *input, char *out, size_t cap) {
    char joined[FTP_LINE];
    char normalized[FTP_LINE];
    size_t used = 0;
    if (!input || !input[0]) input = cwd;
    if (input[0] == '/') {
        if (snprintf(joined, sizeof(joined), "%s", input) >= (int)sizeof(joined)) return false;
    } else {
        if (snprintf(joined, sizeof(joined), "%s/%s", cwd, input) >= (int)sizeof(joined)) return false;
    }
    normalized[used++] = '/';
    const char *p = joined;
    while (*p == '/') p++;
    while (*p) {
        const char *start = p;
        while (*p && *p != '/') p++;
        size_t len = (size_t)(p - start);
        while (*p == '/') p++;
        if (!len || (len == 1 && start[0] == '.')) continue;
        if (len == 2 && start[0] == '.' && start[1] == '.') return false;
        if (memchr(start, ':', len) || memchr(start, '\\', len)) return false;
        if (used > 1) normalized[used++] = '/';
        if (used + len >= sizeof(normalized)) return false;
        memcpy(normalized + used, start, len);
        used += len;
    }
    normalized[used] = 0;
    if (snprintf(out, cap, "sdmc:%s", normalized) >= (int)cap) return false;
    return true;
}

static bool ftp_virtual_path(const char *cwd, const char *input, char *out, size_t cap) {
    char disk[FTP_LINE];
    if (!ftp_path(cwd, input, disk, sizeof(disk))) return false;
    if (snprintf(out, cap, "%s", disk) >= (int)cap) return false;
    return true;
}

static int open_passive(int control, int *data_listener) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(fd, 1) < 0 || getsockname(fd, (struct sockaddr *)&addr, &len) < 0) {
        close(fd);
        return -1;
    }
    *data_listener = fd;
    unsigned int ip[4] = {127,0,0,1};
    struct in_addr parsed;
    if (inet_aton(server_ip, &parsed)) {
        const unsigned char *b = (const unsigned char *)&parsed.s_addr;
        ip[0]=b[0]; ip[1]=b[1]; ip[2]=b[2]; ip[3]=b[3];
    }
    unsigned int port = ntohs(addr.sin_port);
    char msg[128];
    snprintf(msg, sizeof(msg), "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n",
        ip[0],ip[1],ip[2],ip[3],port/256,port%256);
    reply(control, msg);
    return 0;
}

static int accept_data(int *data_listener) {
    if (*data_listener < 0) return -1;
    int fd = accept(*data_listener, NULL, NULL);
    close(*data_listener);
    *data_listener = -1;
    return fd;
}

static void list_directory(int control, int *data_listener, const char *path, bool mlsd) {
    DIR *dir = opendir(path);
    if (!dir) { reply(control, "550 Directory unavailable\r\n"); return; }
    reply(control, "150 Opening data connection\r\n");
    int data = accept_data(data_listener);
    if (data < 0) { closedir(dir); reply(control, "425 Data connection failed\r\n"); return; }
    struct dirent *item;
    char full[FTP_LINE], line[FTP_LINE];
    while ((item = readdir(dir)) != NULL) {
        if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, "..")) continue;
        struct stat st;
        if (snprintf(full, sizeof(full), "%s%s%s", path,
            path[strlen(path)-1] == '/' ? "" : "/", item->d_name) >= (int)sizeof(full)) continue;
        if (stat(full, &st) != 0) continue;
        if (mlsd) {
            snprintf(line, sizeof(line), "type=%s;size=%lld;modify=20260101000000; %s\r\n",
                S_ISDIR(st.st_mode) ? "dir" : "file", (long long)st.st_size, item->d_name);
        } else {
            snprintf(line, sizeof(line), "%s 1 switch switch %lld Jan 01 00:00 %s\r\n",
                S_ISDIR(st.st_mode) ? "drwxr-xr-x" : "-rw-r--r--",
                (long long)st.st_size, item->d_name);
        }
        if (send_all(data, line, strlen(line)) != 0) break;
    }
    closedir(dir);
    close(data);
    reply(control, "226 Directory send OK\r\n");
}

static void transfer_file(int control, int *data_listener, const char *path, bool upload) {
    struct stat st;
    char temporary[FTP_LINE];
    const char *write_path = path;
    if (upload) {
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            reply(control, "550 Destination is a directory\r\n");
            return;
        }
        if (snprintf(temporary, sizeof(temporary), "%s.sfh-uploading", path) >= (int)sizeof(temporary) ||
            stat(temporary, &st) == 0) {
            reply(control, "550 Upload temporary file already exists\r\n");
            return;
        }
        write_path = temporary;
    }
    FILE *file = upload ? fopen(write_path, "wbx") : fopen(path, "rb");
    if (!file) { reply(control, "550 File unavailable\r\n"); return; }
    reply(control, "150 Opening data connection\r\n");
    int data = accept_data(data_listener);
    if (data < 0) {
        fclose(file);
        if (upload) remove(write_path);
        reply(control, "425 Data connection failed\r\n");
        return;
    }
    char buffer[FTP_IO];
    bool ok = true;
    if (upload) {
        ssize_t n;
        while ((n = recv(data, buffer, sizeof(buffer), 0)) > 0) {
            if (fwrite(buffer, 1, (size_t)n, file) != (size_t)n) { ok = false; break; }
        }
    } else {
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            if (send_all(data, buffer, n) != 0) { ok = false; break; }
        }
        if (ferror(file)) ok = false;
    }
    if (fclose(file) != 0) ok = false;
    shutdown(data, SHUT_RDWR);
    close(data);
    if (upload) {
        if (ok) {
            if (rename(write_path, path) != 0) { remove(write_path); ok = false; }
        } else {
            remove(write_path);
        }
    }
    reply(control, ok ? "226 Transfer complete\r\n" : "451 Transfer failed; partial upload removed\r\n");
}
static void serve_client(int control) {
    char line[FTP_LINE], cwd[FTP_LINE] = "/", path[FTP_LINE], second[FTP_LINE];
    char rename_from[FTP_LINE] = "";
    int data_listener = -1;
    reply(control, "220 Switch File Hub FTP ready\r\n");
    while (server_running && read_line(control, line, sizeof(line))) {
        char *arg = strchr(line, ' ');
        if (arg) *arg++ = 0; else arg = "";
        for (char *p=line; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
        if (!strcmp(line,"USER")) reply(control,"331 Password not required; send PASS\r\n");
        else if (!strcmp(line,"PASS")) reply(control,"230 Login successful\r\n");
        else if (!strcmp(line,"SYST")) reply(control,"215 UNIX Type: L8\r\n");
        else if (!strcmp(line,"FEAT")) reply(control,"211-Features\r\n MLSD\r\n SIZE\r\n UTF8\r\n211 End\r\n");
        else if (!strcmp(line,"OPTS")) reply(control,"200 Options accepted\r\n");
        else if (!strcmp(line,"TYPE")) reply(control,"200 Type set\r\n");
        else if (!strcmp(line,"NOOP")) reply(control,"200 OK\r\n");
        else if (!strcmp(line,"PWD") || !strcmp(line,"XPWD")) {
            snprintf(second,sizeof(second),"257 \"%s\" is current directory\r\n",cwd); reply(control,second);
        } else if (!strcmp(line,"CWD") || !strcmp(line,"CDUP")) {
            const char *target = !strcmp(line,"CDUP") ? "/" : arg;
            char candidate[FTP_LINE];
            if (!strcmp(line,"CDUP")) {
                snprintf(candidate,sizeof(candidate),"%s",cwd);
                char *parent=strrchr(candidate, '/');
                if (parent && parent!=candidate) *parent=0; else strcpy(candidate,"/");
                target=candidate;
            }
            if (!ftp_virtual_path(cwd,target,path,sizeof(path))) { reply(control,"550 Invalid path\r\n"); continue; }
            struct stat st;
            if (stat(path,&st)==0 && S_ISDIR(st.st_mode)) {
                if (target[0]=='/') snprintf(cwd,sizeof(cwd),"%s",target);
                else if (!strcmp(cwd,"/")) snprintf(cwd,sizeof(cwd),"/%s",target);
                else { strncat(cwd,"/",sizeof(cwd)-strlen(cwd)-1); strncat(cwd,target,sizeof(cwd)-strlen(cwd)-1); }
                reply(control,"250 Directory changed\r\n");
            } else reply(control,"550 Directory unavailable\r\n");
        } else if (!strcmp(line,"PASV")) {
            if (data_listener >= 0) close(data_listener);
            data_listener=-1;
            if (open_passive(control,&data_listener)!=0) reply(control,"425 Cannot open passive listener\r\n");
        } else if (!strcmp(line,"EPSV")) {
            if (data_listener >= 0) close(data_listener);
            data_listener=-1;
            struct sockaddr_in a; socklen_t alen=sizeof(a);
            int fd=socket(AF_INET,SOCK_STREAM,0);
            memset(&a,0,sizeof(a)); a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_ANY);
            if (fd<0 || bind(fd,(struct sockaddr*)&a,sizeof(a))<0 || listen(fd,1)<0 || getsockname(fd,(struct sockaddr*)&a,&alen)<0) {
                if (fd >= 0) close(fd);
                reply(control,"425 Cannot open passive listener\r\n");
            } else {
                data_listener=fd; snprintf(second,sizeof(second),"229 Entering Extended Passive Mode (|||%u|)\r\n",ntohs(a.sin_port)); reply(control,second);
            }
        } else if (!strcmp(line,"LIST") || !strcmp(line,"MLSD")) {
            if (!ftp_virtual_path(cwd,arg,path,sizeof(path))) { reply(control,"550 Invalid path\r\n"); continue; }
            list_directory(control,&data_listener,path,!strcmp(line,"MLSD"));
        } else if (!strcmp(line,"RETR") || !strcmp(line,"STOR")) {
            if (!ftp_virtual_path(cwd,arg,path,sizeof(path))) { reply(control,"550 Invalid path\r\n"); continue; }
            transfer_file(control,&data_listener,path,!strcmp(line,"STOR"));
        } else if (!strcmp(line,"MKD") || !strcmp(line,"XMKD")) {
            if (!ftp_virtual_path(cwd,arg,path,sizeof(path))) { reply(control,"550 Invalid path\r\n"); continue; }
            reply(control,mkdir(path,0777)==0 ? "257 Directory created\r\n" : "550 Cannot create directory\r\n");
        } else if (!strcmp(line,"DELE") || !strcmp(line,"RMD") || !strcmp(line,"XRMD")) {
            if (!ftp_virtual_path(cwd,arg,path,sizeof(path))) { reply(control,"550 Invalid path\r\n"); continue; }
            struct stat st;
            if (stat(path,&st)!=0 || (!strcmp(line,"DELE") && S_ISDIR(st.st_mode)) || ((!strcmp(line,"RMD") || !strcmp(line,"XRMD")) && !S_ISDIR(st.st_mode))) reply(control,"550 Invalid target\r\n");
            else reply(control,(S_ISDIR(st.st_mode) ? rmdir(path) : remove(path))==0 ? "250 Deleted\r\n" : "550 Delete failed\r\n");
        } else if (!strcmp(line,"SIZE")) {
            struct stat st;
            if (ftp_virtual_path(cwd,arg,path,sizeof(path)) && stat(path,&st)==0 && S_ISREG(st.st_mode)) { snprintf(second,sizeof(second),"213 %lld\r\n",(long long)st.st_size); reply(control,second); }
            else reply(control,"550 File unavailable\r\n");
        } else if (!strcmp(line,"RNFR")) {
            if (ftp_virtual_path(cwd,arg,rename_from,sizeof(rename_from))) reply(control,"350 Ready for RNTO\r\n");
            else reply(control,"550 Invalid path\r\n");
        } else if (!strcmp(line,"RNTO")) {
            if (!rename_from[0] || !ftp_virtual_path(cwd,arg,path,sizeof(path))) reply(control,"503 Send RNFR first\r\n");
            else { struct stat st; if(stat(path,&st)==0) reply(control,"550 Destination exists\r\n"); else reply(control,rename(rename_from,path)==0 ? "250 Rename successful\r\n" : "550 Rename failed\r\n"); rename_from[0]=0; }
        } else if (!strcmp(line,"QUIT")) { reply(control,"221 Goodbye\r\n"); break; }
        else if (!strcmp(line,"ABOR")) reply(control,"226 Abort acknowledged\r\n");
        else reply(control,"502 Command not implemented\r\n");
    }
    if (data_listener >= 0) close(data_listener);
    shutdown(control,SHUT_RDWR);
    close(control);
}

static void server_main(void *unused) {
    (void)unused;
    struct sockaddr_in addr;
    listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd<0) { server_state=-1; return; }
    int yes=1; setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    memset(&addr,0,sizeof(addr)); addr.sin_family=AF_INET; addr.sin_addr.s_addr=htonl(INADDR_ANY); addr.sin_port=htons(FTP_PORT);
    if (bind(listen_fd,(struct sockaddr*)&addr,sizeof(addr))<0 || listen(listen_fd,2)<0) {
        close(listen_fd); listen_fd=-1; server_state=-1; return;
    }
    server_state=2;
    while (server_running) {
        int client=accept(listen_fd,NULL,NULL);
        if (client<0) { if (!server_running) break; continue; }
        serve_client(client);
    }
    if (listen_fd>=0) { close(listen_fd); listen_fd=-1; }
    server_state=0;
    return;
}

Result ftpServerStart(void) {
    if (server_running) return 0;
    server_state=1;
    Result rc=socketInitializeDefault();
    if (R_FAILED(rc)) { server_state=-1; return rc; }
    /* Determine the LAN interface address without sending any packets. */
    int probe=socket(AF_INET,SOCK_DGRAM,0);
    if (probe>=0) {
        struct sockaddr_in remote; memset(&remote,0,sizeof(remote));
        remote.sin_family=AF_INET; remote.sin_port=htons(53); inet_aton("1.1.1.1",&remote.sin_addr);
        if (connect(probe,(struct sockaddr*)&remote,sizeof(remote))==0) {
            struct sockaddr_in local; socklen_t len=sizeof(local);
            if (getsockname(probe,(struct sockaddr*)&local,&len)==0)
                snprintf(server_ip,sizeof(server_ip),"%s",inet_ntoa(local.sin_addr));
        }
        close(probe);
    }
    server_running=true;
    rc=threadCreate(&server_thread,server_main,NULL,NULL,0x10000,0x2C,-2);
    if (R_FAILED(rc)) { server_running=false; socketExit(); server_state=-1; return rc; }
    rc=threadStart(&server_thread);
    if (R_FAILED(rc)) { server_running=false; threadClose(&server_thread); socketExit(); server_state=-1; return rc; }
    return 0;
}

void ftpServerStop(void) {
    if (!server_running) return;
    server_running=false;
    if (listen_fd>=0) shutdown(listen_fd,SHUT_RDWR);
    threadWaitForExit(&server_thread);
    threadClose(&server_thread);
    socketExit();
}

int ftpServerState(void) { return server_state; }
const char *ftpServerIp(void) { return server_ip; }
