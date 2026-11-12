#include <switch.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_ENTRIES 128
#define NAME_LENGTH 256
#define PATH_LENGTH 1024
#define PAGE_SIZE 10

typedef struct {
    char name[NAME_LENGTH];
    bool is_dir;
} Entry;

static Entry entries[MAX_ENTRIES];
static int entry_count = 0;
static int selected = 0;
static char current_path[PATH_LENGTH] = "sdmc:/";
static char directory_status[160] = "Aguardando montagem do microSD...";
static Result mount_result = 0;
static bool exfat_query_ok = false;
static bool exfat_supported = false;
static u32 mount_module = 0;
static u32 mount_description = 0;

static void diagnose_mount_error(Result rc) {
    Result exfat_rc = fsIsExFatSupported(&exfat_supported);
    exfat_query_ok = R_SUCCEEDED(exfat_rc);
    mount_module = (u32)R_MODULE(rc);
    mount_description = (u32)R_DESCRIPTION(rc);
    snprintf(directory_status, sizeof(directory_status),
             "Nao foi possivel acessar o microSD.");
}

static void load_directory(void) {
    entry_count = 0;
    selected = 0;

    DIR *dir = opendir(current_path);
    if (!dir) {
        snprintf(directory_status, sizeof(directory_status),
                 "Nao foi possivel abrir esta pasta.");
        return;
    }

    struct dirent *item;
    while ((item = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        if (strcmp(item->d_name, ".") == 0) continue;

        Entry *entry = &entries[entry_count];
        snprintf(entry->name, sizeof(entry->name), "%s", item->d_name);

        char full_path[PATH_LENGTH];
        struct stat st;
        int written = snprintf(full_path, sizeof(full_path), "%s%s%s",
            current_path,
            current_path[strlen(current_path) - 1] == '/' ? "" : "/",
            item->d_name);
        entry->is_dir = written > 0 && (size_t)written < sizeof(full_path)
            && stat(full_path, &st) == 0 && S_ISDIR(st.st_mode);
        entry_count++;
    }

    closedir(dir);

    if (entry_count == MAX_ENTRIES) {
        snprintf(directory_status, sizeof(directory_status),
                 "Mostrando ate %d itens nesta pasta.", MAX_ENTRIES);
    } else {
        snprintf(directory_status, sizeof(directory_status),
                 "Pasta carregada: %d item(ns).", entry_count);
    }
}

static void go_parent(void) {
    if (strcmp(current_path, "sdmc:/") == 0) return;

    size_t len = strlen(current_path);
    while (len > 6 && current_path[len - 1] == '/') current_path[--len] = '\0';

    char *slash = strrchr(current_path, '/');
    if (slash && slash >= current_path + 6) {
        slash[1] = '\0';
    } else {
        strcpy(current_path, "sdmc:/");
    }

    load_directory();
}

static void enter_selected(void) {
    if (selected < 0 || selected >= entry_count || !entries[selected].is_dir) return;

    char next_path[PATH_LENGTH];
    int written = snprintf(next_path, sizeof(next_path), "%s%s%s",
        current_path,
        current_path[strlen(current_path) - 1] == '/' ? "" : "/",
        entries[selected].name);

    if (written <= 0 || (size_t)written >= sizeof(next_path)) {
        snprintf(directory_status, sizeof(directory_status),
                 "Caminho muito longo para abrir.");
        return;
    }

    snprintf(current_path, sizeof(current_path), "%s", next_path);
    load_directory();
}

static void draw_ui(void) {
    consoleClear();

    printf("  +------------------------------------------------------------------+\n");
    printf("  |                         SWITCH FILE HUB                          |\n");
    printf("  |                         NAVEGADOR DE SD                          |\n");
    printf("  +------------------------------------------------------------------+\n");

    if (R_SUCCEEDED(mount_result)) {
        printf("  MICROSD  [ CONECTADO ]       ITENS: %-4d       VERSAO: 0.1.3\n", entry_count);
    } else {
        printf("  MICROSD  [ ERRO ]            VERSAO: 0.1.3\n");
    }
    printf("  LOCAL: %.62s\n", current_path);
    printf("  STATUS: %.65s\n", directory_status);

    if (R_FAILED(mount_result)) {
        printf("  CODIGO: 0x%08X  MODULO: %u  DESCRICAO: %u\n",
               (unsigned int)mount_result, mount_module, mount_description);
        if (exfat_query_ok) {
            printf("  exFAT: %s\n", exfat_supported ? "suportado pelo sistema" : "nao suportado pelo sistema");
        } else {
            printf("  exFAT: consulta indisponivel\n");
        }
        printf("  Verifique se o console reconhece o cartao. Nao formate por este erro.\n");
    } else {
        printf("  ------------------------------------------------------------------\n");
        printf("  %c  TIPO   NOME\n", ' ');
        printf("  ------------------------------------------------------------------\n");

        int page_start = (selected / PAGE_SIZE) * PAGE_SIZE;
        int page_end = page_start + PAGE_SIZE;
        if (page_end > entry_count) page_end = entry_count;

        if (entry_count == 0) {
            printf("     Esta pasta esta vazia.\n");
        } else {
            for (int i = page_start; i < page_end; i++) {
                printf("  %c  %-5s  %.54s\n",
                    i == selected ? '>' : ' ',
                    entries[i].is_dir ? "PASTA" : "ARQ.",
                    entries[i].name);
            }
        }

        printf("  ------------------------------------------------------------------\n");
        if (entry_count > PAGE_SIZE) {
            printf("  PAGINA: %d/%d  |  TOTAL: %d itens\n",
                selected / PAGE_SIZE + 1,
                (entry_count + PAGE_SIZE - 1) / PAGE_SIZE,
                entry_count);
        }
    }

    printf("\n  CONTROLES  [A] Abrir  [B] Voltar  [Y] Atualizar  [+] Sair\n");
    printf("  Switch File Hub | Navegacao local do microSD\n");
    consoleUpdate(NULL);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    mount_result = fsdevMountSdmc();

    if (R_SUCCEEDED(mount_result)) {
        load_directory();
    } else {
        diagnose_mount_error(mount_result);
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus) break;
        if (R_SUCCEEDED(mount_result)) {
            if ((down & HidNpadButton_Down) && selected < entry_count - 1) selected++;
            if ((down & HidNpadButton_Up) && selected > 0) selected--;
            if (down & HidNpadButton_A) enter_selected();
            if (down & HidNpadButton_B) go_parent();
            if (down & HidNpadButton_Y) load_directory();
        }

        draw_ui();
    }

    if (R_SUCCEEDED(mount_result)) fsdevUnmountDevice("sdmc");
    consoleExit(NULL);
    return 0;
}
