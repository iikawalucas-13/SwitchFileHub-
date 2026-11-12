#include <switch.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_ENTRIES 128
#define NAME_LENGTH 256
#define PATH_LENGTH 1024

typedef struct {
    char name[NAME_LENGTH];
    bool is_dir;
} Entry;

static Entry entries[MAX_ENTRIES];
static int entry_count = 0;
static int selected = 0;
static char current_path[PATH_LENGTH] = "sdmc:/";
static char directory_status[256] = "Aguardando montagem do microSD...";
static Result mount_result = 0;
static bool exfat_query_ok = false;
static bool exfat_supported = false;

static void diagnose_mount_error(Result rc) {
    Result exfat_rc = fsIsExFatSupported(&exfat_supported);
    exfat_query_ok = R_SUCCEEDED(exfat_rc);
    snprintf(directory_status, sizeof(directory_status),
        "Falha microSD: 0x%08X | Modulo: %u | Descricao: %u | exFAT: %s",
        (unsigned int)rc, (unsigned int)R_MODULE(rc),
        (unsigned int)R_DESCRIPTION(rc),
        exfat_query_ok ? (exfat_supported ? "suportado" : "nao suportado") : "consulta indisponivel");
}

static void load_directory(void) {
    entry_count = 0;
    selected = 0;

    DIR *dir = opendir(current_path);
    if (!dir) {
        snprintf(directory_status, sizeof(directory_status),
                 "ERRO: nao foi possivel abrir esta pasta.");
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
                 "Listagem limitada aos primeiros %d itens.", MAX_ENTRIES);
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
                 "ERRO: caminho muito longo.");
        return;
    }

    snprintf(current_path, sizeof(current_path), "%s", next_path);
    load_directory();
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
        if ((down & HidNpadButton_Down) && selected < entry_count - 1) selected++;
        if ((down & HidNpadButton_Up) && selected > 0) selected--;
        if (down & HidNpadButton_A) enter_selected();
        if (down & HidNpadButton_B) go_parent();
        if ((down & HidNpadButton_Y) && R_SUCCEEDED(mount_result)) load_directory();

        consoleClear();
        printf("========================================\n");
        printf("          SWITCH FILE HUB 0.1.1         \n");
        printf("========================================\n\n");
        printf("MicroSD: %s\n", R_SUCCEEDED(mount_result) ? "montado" : "erro ao montar");
        printf("Pasta: %s\n", current_path);
        printf("Estado: %s\n", directory_status);
        if (R_FAILED(mount_result)) {
            printf("Diagnostico: confira se o console reconhece o microSD.\n");
            printf("Nao formate o cartao por causa deste erro.\n\n");
        } else {
            printf("\n");
        }
        printf("Arquivos e pastas (max. %d):\n", MAX_ENTRIES);

        for (int i = 0; i < entry_count; i++) {
            printf("%s %s %s\n",
                i == selected ? ">" : " ",
                entries[i].is_dir ? "[DIR]" : "     ",
                entries[i].name);
        }

        printf("\nControles: A abrir pasta | B voltar | Y atualizar | + sair\n");
        printf("Aviso: esta versao navega no microSD; nao transfere arquivos.\n");
        printf("Build diagnostico: 0.1.2\n");
        consoleUpdate(NULL);
    }

    if (R_SUCCEEDED(mount_result)) fsdevUnmountDevice("sdmc");
    consoleExit(NULL);
    return 0;
}
