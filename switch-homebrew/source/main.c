#include <switch.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_ENTRIES 128
#define NAME_LENGTH 256
#define PATH_LENGTH 1024
#define PAGE_SIZE 10
#define COPY_BUFFER_SIZE 16384

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

static bool valid_name(const char *name) {
    if (!name || !name[0] || !strcmp(name, ".") || !strcmp(name, "..")) return false;
    if (strchr(name, '/') || strchr(name, '\\') || strchr(name, ':')) return false;
    if (strchr(name, '\n') || strchr(name, '\r')) return false;
    return true;
}

static bool build_path(char *out, size_t out_size, const char *base, const char *name) {
    int n = snprintf(out, out_size, "%s%s%s", base,
        base[strlen(base) - 1] == '/' ? "" : "/", name);
    return n > 0 && (size_t)n < out_size;
}

static bool ask_text(const char *title, const char *initial, char *out, size_t out_size) {
    SwkbdConfig keyboard;
    memset(out, 0, out_size);
    Result rc = swkbdCreate(&keyboard, 0);
    if (R_FAILED(rc)) {
        snprintf(directory_status, sizeof(directory_status), "Nao foi possivel abrir o teclado.");
        return false;
    }
    swkbdConfigMakePresetDefault(&keyboard);
    swkbdConfigSetHeaderText(&keyboard, title);
    if (initial && initial[0]) swkbdConfigSetInitialText(&keyboard, initial);
    rc = swkbdShow(&keyboard, out, out_size);
    swkbdClose(&keyboard);
    return R_SUCCEEDED(rc) && out[0] != '\0';
}

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
                 "Nao foi possivel abrir esta pasta (erro %d).", errno);
        return;
    }

    struct dirent *item;
    while ((item = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) continue;

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
    snprintf(directory_status, sizeof(directory_status),
             entry_count == MAX_ENTRIES ? "Limite de %d itens exibidos." : "Pasta carregada: %d item(ns).",
             entry_count);
}

static bool selected_path(char *out, size_t out_size) {
    if (selected < 0 || selected >= entry_count) return false;
    return build_path(out, out_size, current_path, entries[selected].name);
}

static void go_parent(void) {
    if (strcmp(current_path, "sdmc:/") == 0) return;

    size_t len = strlen(current_path);
    while (len > 6 && current_path[len - 1] == '/') current_path[--len] = '\0';

    char *slash = strrchr(current_path, '/');
    if (slash && slash >= current_path + 6) slash[1] = '\0';
    else strcpy(current_path, "sdmc:/");

    load_directory();
}

static void enter_selected(void) {
    if (selected < 0 || selected >= entry_count || !entries[selected].is_dir) return;
    char next_path[PATH_LENGTH];
    if (!build_path(next_path, sizeof(next_path), current_path, entries[selected].name)) {
        snprintf(directory_status, sizeof(directory_status), "Caminho muito longo.");
        return;
    }
    snprintf(current_path, sizeof(current_path), "%s", next_path);
    load_directory();
}

static void create_folder(void) {
    char name[NAME_LENGTH];
    if (!ask_text("Nome da nova pasta", "", name, sizeof(name))) return;
    if (!valid_name(name)) {
        snprintf(directory_status, sizeof(directory_status), "Nome invalido.");
        return;
    }
    char path[PATH_LENGTH];
    if (!build_path(path, sizeof(path), current_path, name)) {
        snprintf(directory_status, sizeof(directory_status), "Caminho muito longo.");
        return;
    }
    if (mkdir(path, 0777) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Pasta criada: %.100s", name);
        load_directory();
    } else {
        snprintf(directory_status, sizeof(directory_status), "Falha ao criar pasta (erro %d).", errno);
    }
}

static void rename_selected(void) {
    char old_path[PATH_LENGTH], new_name[NAME_LENGTH], new_path[PATH_LENGTH];
    if (!selected_path(old_path, sizeof(old_path))) return;
    if (!ask_text("Novo nome", entries[selected].name, new_name, sizeof(new_name))) return;
    if (!valid_name(new_name)) {
        snprintf(directory_status, sizeof(directory_status), "Nome invalido.");
        return;
    }
    if (!build_path(new_path, sizeof(new_path), current_path, new_name)) {
        snprintf(directory_status, sizeof(directory_status), "Caminho muito longo.");
        return;
    }
    struct stat st;
    if (stat(new_path, &st) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Ja existe um item com esse nome.");
        return;
    }
    if (rename(old_path, new_path) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Item renomeado.");
        load_directory();
    } else {
        snprintf(directory_status, sizeof(directory_status), "Falha ao renomear (erro %d).", errno);
    }
}

static void delete_selected(void) {
    char path[PATH_LENGTH];
    if (!selected_path(path, sizeof(path))) return;
    bool is_dir = entries[selected].is_dir;
    char prompt[NAME_LENGTH + 64];
    snprintf(prompt, sizeof(prompt), "Digite APAGAR para confirmar: %.180s", entries[selected].name);
    char answer[NAME_LENGTH];
    if (!ask_text(prompt, "", answer, sizeof(answer)) || strcmp(answer, "APAGAR") != 0) {
        snprintf(directory_status, sizeof(directory_status), "Exclusao cancelada.");
        return;
    }
    int rc = is_dir ? rmdir(path) : remove(path);
    if (rc == 0) {
        snprintf(directory_status, sizeof(directory_status), "Item excluido.");
        load_directory();
    } else if (is_dir && errno == ENOTEMPTY) {
        snprintf(directory_status, sizeof(directory_status), "Pasta nao esta vazia; exclua o conteudo primeiro.");
    } else {
        snprintf(directory_status, sizeof(directory_status), "Falha ao excluir (erro %d).", errno);
    }
}

static bool safe_destination(const char *path) {
    if (!path || strncmp(path, "sdmc:/", 6) != 0) return false;
    if (strstr(path, "/../") || strstr(path, "/./") || strstr(path, "//")) return false;
    size_t len = strlen(path);
    if (len >= 3 && (!strcmp(path + len - 3, "/..") || !strcmp(path + len - 2, "/."))) return false;
    return true;
}

static void copy_file_to(const char *source, const char *destination) {
    struct stat st;
    if (stat(source, &st) != 0 || !S_ISREG(st.st_mode)) {
        snprintf(directory_status, sizeof(directory_status), "Copiar: selecione um arquivo comum.");
        return;
    }
    if (stat(destination, &st) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Destino ja existe; nada foi sobrescrito.");
        return;
    }
    FILE *in = fopen(source, "rb");
    if (!in) {
        snprintf(directory_status, sizeof(directory_status), "Nao foi possivel abrir origem (erro %d).", errno);
        return;
    }
    FILE *out = fopen(destination, "wbx");
    if (!out) {
        fclose(in);
        snprintf(directory_status, sizeof(directory_status), "Nao foi possivel criar destino (erro %d).", errno);
        return;
    }
    char buffer[COPY_BUFFER_SIZE];
    size_t count;
    bool ok = true;
    while ((count = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, count, out) != count) { ok = false; break; }
    }
    if (ferror(in)) ok = false;
    if (fclose(in) != 0) ok = false;
    if (fclose(out) != 0) ok = false;
    if (!ok) {
        remove(destination);
        snprintf(directory_status, sizeof(directory_status), "Falha na copia; arquivo parcial removido.");
    } else {
        snprintf(directory_status, sizeof(directory_status), "Arquivo copiado com sucesso.");
    }
}

static void copy_selected(void) {
    char source[PATH_LENGTH], destination[PATH_LENGTH];
    if (!selected_path(source, sizeof(source))) return;
    if (entries[selected].is_dir) {
        snprintf(directory_status, sizeof(directory_status), "Copiar pasta inteira ainda nao esta disponivel.");
        return;
    }
    char initial[PATH_LENGTH];
    if (!build_path(initial, sizeof(initial), current_path, entries[selected].name)) return;
    if (!ask_text("Caminho completo do destino no microSD", initial, destination, sizeof(destination))) return;
    if (!safe_destination(destination)) {
        snprintf(directory_status, sizeof(directory_status), "Destino invalido: use sdmc:/... sem ..");
        return;
    }
    copy_file_to(source, destination);
    load_directory();
}

static void move_selected(void) {
    char source[PATH_LENGTH], destination[PATH_LENGTH];
    if (!selected_path(source, sizeof(source))) return;
    if (entries[selected].is_dir) {
        snprintf(directory_status, sizeof(directory_status), "Mover pasta inteira ainda nao esta disponivel.");
        return;
    }
    if (!ask_text("Caminho completo do destino no microSD", source, destination, sizeof(destination))) return;
    if (!safe_destination(destination)) {
        snprintf(directory_status, sizeof(directory_status), "Destino invalido: use sdmc:/... sem ..");
        return;
    }
    struct stat st;
    if (stat(destination, &st) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Destino ja existe; nada foi substituido.");
        return;
    }
    if (rename(source, destination) == 0) {
        snprintf(directory_status, sizeof(directory_status), "Arquivo movido.");
        load_directory();
    } else {
        snprintf(directory_status, sizeof(directory_status), "Falha ao mover (erro %d).", errno);
    }
}

static void show_info(void) {
    char path[PATH_LENGTH];
    struct stat st;
    if (!selected_path(path, sizeof(path))) return;
    if (stat(path, &st) != 0) {
        snprintf(directory_status, sizeof(directory_status), "Nao foi possivel ler informacoes.");
        return;
    }
    if (S_ISDIR(st.st_mode))
        snprintf(directory_status, sizeof(directory_status), "PASTA: %.75s | tamanho: %lld bytes", entries[selected].name, (long long)st.st_size);
    else
        snprintf(directory_status, sizeof(directory_status), "ARQUIVO: %.65s | %lld bytes", entries[selected].name, (long long)st.st_size);
}

static void draw_ui(void) {
    consoleClear();
    printf("  +------------------------------------------------------------------+\n");
    printf("  |                         SWITCH FILE HUB                          |\n");
    printf("  |                    GERENCIADOR DO MICROSD                        |\n");
    printf("  +------------------------------------------------------------------+\n");
    if (R_SUCCEEDED(mount_result))
        printf("  MICROSD [ CONECTADO ]       ITENS: %-4d       VERSAO: 0.2.0\n", entry_count);
    else
        printf("  MICROSD [ ERRO ]            VERSAO: 0.2.0\n");
    printf("  LOCAL: %.62s\n", current_path);
    printf("  STATUS: %.65s\n", directory_status);

    if (R_FAILED(mount_result)) {
        printf("  CODIGO: 0x%08X  MODULO: %u  DESCRICAO: %u\n",
               (unsigned int)mount_result, mount_module, mount_description);
        if (exfat_query_ok)
            printf("  exFAT: %s\n", exfat_supported ? "suportado pelo sistema" : "nao suportado pelo sistema");
        else
            printf("  exFAT: consulta indisponivel\n");
        printf("  Verifique se o console reconhece o cartao. Nao formate por este erro.\n");
    } else {
        printf("  ------------------------------------------------------------------\n");
        printf("     TIPO   NOME\n");
        printf("  ------------------------------------------------------------------\n");
        int page_start = (selected / PAGE_SIZE) * PAGE_SIZE;
        int page_end = page_start + PAGE_SIZE;
        if (page_end > entry_count) page_end = entry_count;
        if (entry_count == 0) printf("     Esta pasta esta vazia.\n");
        else for (int i = page_start; i < page_end; i++)
            printf("  %c  %-5s  %.54s\n", i == selected ? '>' : ' ',
                   entries[i].is_dir ? "PASTA" : "ARQ.", entries[i].name);
        printf("  ------------------------------------------------------------------\n");
        if (entry_count > PAGE_SIZE)
            printf("  PAGINA: %d/%d | TOTAL: %d itens\n",
                selected / PAGE_SIZE + 1, (entry_count + PAGE_SIZE - 1) / PAGE_SIZE, entry_count);
    }
    printf("\n  A Abrir | B Voltar | Y Atualizar | X Nova pasta\n");
    printf("  L Renomear | R Excluir | ZL Copiar arquivo | ZR Mover arquivo\n");
    printf("  R3 Informacoes | + Sair\n");
    consoleUpdate(NULL);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    consoleInit(NULL);
    mount_result = fsdevMountSdmc();
    if (R_SUCCEEDED(mount_result)) load_directory();
    else diagnose_mount_error(mount_result);

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
            if (down & HidNpadButton_X) create_folder();
            if (down & HidNpadButton_L) rename_selected();
            if (down & HidNpadButton_R) delete_selected();
            if (down & HidNpadButton_ZL) copy_selected();
            if (down & HidNpadButton_ZR) move_selected();
            if (down & HidNpadButton_R3) show_info();
        }
        draw_ui();
    }
    if (R_SUCCEEDED(mount_result)) fsdevUnmountDevice("sdmc");
    consoleExit(NULL);
    return 0;
}