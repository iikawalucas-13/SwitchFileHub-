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

static void load_directory(void) {
    entry_count = 0;
    selected = 0;

    DIR *dir = opendir(current_path);
    if (!dir) return;

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

    if (written > 0 && (size_t)written < sizeof(next_path)) {
        snprintf(current_path, sizeof(current_path), "%s", next_path);
        load_directory();
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    Result mount_result = fsdevMountSdmc();
    load_directory();

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
        if (down & HidNpadButton_Y) load_directory();

        consoleClear();
        printf("========================================\n");
        printf("          SWITCH FILE HUB 0.1.0         \n");
        printf("========================================\n\n");
        printf("MicroSD: %s\n", R_SUCCEEDED(mount_result) ? "montado" : "erro ao montar");
        printf("Pasta: %s\n\n", current_path);
        printf("Arquivos e pastas (max. %d):\n", MAX_ENTRIES);

        if (entry_count == 0) {
            printf("  (pasta vazia ou nao foi possivel ler)\n");
        }

        for (int i = 0; i < entry_count; i++) {
            printf("%s %s %s\n",
                i == selected ? ">" : " ",
                entries[i].is_dir ? "[DIR]" : "     ",
                entries[i].name);
        }

        printf("\nControles: A abrir pasta | B voltar | Y atualizar | + sair\n");
        consoleUpdate(NULL);
    }

    fsdevUnmountDevice("sdmc");
    consoleExit(NULL);
    return 0;
}
