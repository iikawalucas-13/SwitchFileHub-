#include <switch.h>
#include <pu/Plutonium>
#include <string>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "ftp_server.h"

/* Avoid an extra heap allocation for fsdev current-directory tracking.
   Every filesystem path in this app is absolute (sdmc:/...). */
extern "C" bool __nx_fsdev_support_cwd;

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
    if (R_MODULE(rc) == Module_Libnx && R_DESCRIPTION(rc) == LibnxError_OutOfMemory) {
        snprintf(directory_status, sizeof(directory_status),
                 "Falha de memoria ao registrar o dispositivo SD (libnx).");
    } else {
        snprintf(directory_status, sizeof(directory_status),
                 "Falha ao abrir o sistema de arquivos do microSD.");
    }
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


static bool g_show_transfers = false;
class HubLayout : public pu::ui::Layout {
public:
    pu::ui::elm::TextBlock::Ref header, subtitle, path, files, shortcuts, footer;
    pu::ui::elm::Button::Ref status, ftp, transfer_button;
    PU_SMART_CTOR(HubLayout)
    HubLayout() {
        this->SetBackgroundColor(pu::ui::Color(10, 14, 25, 255));

        header = pu::ui::elm::TextBlock::New(42, 24, "SWITCH FILE HUB");
        header->SetColor(pu::ui::Color(248, 250, 255, 255));
        subtitle = pu::ui::elm::TextBlock::New(44, 68, "CENTRAL DE ARQUIVOS  |  NINTENDO SWITCH");
        subtitle->SetColor(pu::ui::Color(135, 155, 190, 255));

        status = pu::ui::elm::Button::New(42, 112, 430, 66,
            "MICROSD  |  VERIFICANDO...", pu::ui::Color(245,248,255,255),
            pu::ui::Color(27,43,62,255));
        status->SetOnClick([]() {});
        ftp = pu::ui::elm::Button::New(490, 112, 738, 66,
            "FTP  |  DESLIGADO", pu::ui::Color(245,248,255,255),
            pu::ui::Color(31,39,67,255));
        ftp->SetOnClick([]() {});

        path = pu::ui::elm::TextBlock::New(48, 204, "LOCAL  /  sdmc:/");
        path->SetColor(pu::ui::Color(130, 205, 255, 255));

        files = pu::ui::elm::TextBlock::New(54, 250, "ARQUIVOS E PASTAS\n\nCarregando...");
        files->SetColor(pu::ui::Color(238, 242, 250, 255));

        shortcuts = pu::ui::elm::TextBlock::New(870, 252,
            "ATALHOS\n\nA  Abrir item\nB  Voltar pasta\nY  Atualizar lista\nX  Nova pasta\nL  Renomear\nR  Excluir\nZL  Copiar arquivo\nZR  Mover arquivo\n-  Informacoes\n\nDICA\nUse o FTP na mesma rede Wi-Fi para acessar os arquivos pelo celular ou computador.");
        shortcuts->SetColor(pu::ui::Color(150, 170, 205, 255));

        footer = pu::ui::elm::TextBlock::New(44, 662,
            "SWITCH FILE HUB  0.6.0     •     A: SELECIONAR     B: VOLTAR     +: SAIR");
        footer->SetColor(pu::ui::Color(125, 145, 175, 255));

        transfer_button = pu::ui::elm::Button::New(940, 626, 288, 58,
            "VER TRANSFERENCIAS", pu::ui::Color(255,255,255,255),
            pu::ui::Color(63,88,180,255));
        transfer_button->SetOnClick([this]() {
            g_show_transfers = !g_show_transfers;
            transfer_button->SetContent(g_show_transfers ? "VER ARQUIVOS" : "VER TRANSFERENCIAS");
        });

        this->Add(header);
        this->Add(subtitle);
        this->Add(status);
        this->Add(ftp);
        this->Add(path);
        this->Add(files);
        this->Add(shortcuts);
        this->Add(footer);
        this->Add(transfer_button);
    }
};
class HubApplication : public pu::ui::Application {
    HubLayout::Ref layout;
public:
    using Application::Application;
    PU_SMART_CTOR(HubApplication)
    void refreshText() {
        if (R_SUCCEEDED(mount_result)) {
            char s[180];
            snprintf(s, sizeof(s), "MICROSD  |  CONECTADO  |  %d ITENS", entry_count);
            layout->status->SetContent(s);
        } else {
            char s[180];
            snprintf(s, sizeof(s), "MICROSD  |  ERRO 0x%08X", (unsigned int)mount_result);
            layout->status->SetContent(s);
        }

        int fs = ftpServerState();
        if (fs == 2) {
            char ftp_text[180];
            snprintf(ftp_text, sizeof(ftp_text), "FTP  |  ATIVO  |  %s:5000  |  ANONIMO", ftpServerIp());
            layout->ftp->SetContent(ftp_text);
        } else if (fs == 1) {
            layout->ftp->SetContent("FTP  |  INICIANDO SERVIDOR...");
        } else if (fs < 0) {
            layout->ftp->SetContent("FTP  |  FALHA AO INICIAR");
        } else {
            layout->ftp->SetContent("FTP  |  DESLIGADO");
        }

        layout->path->SetText(std::string("LOCAL  /  ") + current_path);

        std::string content;
        if (g_show_transfers) {
            FtpTransferStats stats = {};
            ftpServerGetStats(&stats);
            char summary[768];
            snprintf(summary, sizeof(summary),
                "PAINEL DE TRANSFERENCIAS\n\n"
                "STATUS DO SERVIDOR   %s\n\n"
                "TRANSFERENCIAS ATIVAS       %u\n"
                "CONCLUIDAS                  %llu\n"
                "COM FALHA                   %llu\n\n"
                "DADOS RECEBIDOS NO SWITCH   %llu bytes\n"
                "DADOS ENVIADOS PELO SWITCH  %llu bytes\n\n"
                "Os numeros mostram somente esta sessao e reiniciam ao fechar o app.",
                fs == 2 ? "ONLINE" : (fs < 0 ? "ERRO" : "OFFLINE"),
                (unsigned int)stats.active,
                (unsigned long long)stats.completed,
                (unsigned long long)stats.failed,
                (unsigned long long)stats.bytes_received,
                (unsigned long long)stats.bytes_sent);
            content = summary;
            layout->shortcuts->SetText("TRANSFERENCIAS FTP\n\nO Switch File Hub contabiliza os arquivos enviados e recebidos durante esta sessao.\n\nDICA DE CONEXAO\nDigite o endereco IP exibido no cartao FTP em um cliente FTP conectado a mesma rede Wi-Fi.\n\nSeguranca\nO FTP e anonimo e sem criptografia. Use somente em rede local confiavel.");
        } else {
            content = "ARQUIVOS E PASTAS\n\n";
            if (R_FAILED(mount_result)) {
                content += directory_status;
                content += "\n\nO cartao nao foi montado. Reinicie o console e tente novamente. Se continuar, envie o codigo de erro para diagnostico.";
            } else if (entry_count == 0) {
                content += "ESTA PASTA ESTA VAZIA.";
            } else {
                int first = (selected / PAGE_SIZE) * PAGE_SIZE;
                int last = std::min(first + PAGE_SIZE, entry_count);
                for (int i = first; i < last; i++) {
                    content += (i == selected ? "  >  " : "     ");
                    content += entries[i].is_dir ? "[PASTA]  " : "[ARQUIVO]  ";
                    content += entries[i].name;
                    content += "\n";
                }
                char page[80];
                snprintf(page, sizeof(page), "\nPagina %d  |  %d item(ns)", selected / PAGE_SIZE + 1, entry_count);
                content += page;
            }
            content += "\n\n";
            content += directory_status;
            layout->shortcuts->SetText("ATALHOS\n\nA  Abrir item\nB  Voltar pasta\nY  Atualizar lista\nX  Nova pasta\nL  Renomear\nR  Excluir\nZL  Copiar arquivo\nZR  Mover arquivo\n-  Informacoes\n\nDICA\nUse o FTP na mesma rede Wi-Fi para acessar os arquivos pelo celular ou computador.");
        }
        layout->files->SetText(content);
    }
    void OnLoad() override {
        layout = HubLayout::New();
        this->LoadLayout(layout);
        this->SetOnInput([&](const u64 down,const u64,const u64,const pu::ui::TouchPoint) {
            const bool was_showing_transfers = g_show_transfers;
            if(down & HidNpadButton_Plus) { this->Close(); return; }
            if(down & HidNpadButton_X) {
                if(g_show_transfers) g_show_transfers = false;
                else if(R_SUCCEEDED(mount_result)) create_folder();
            }
            if(down & HidNpadButton_B) {
                if(g_show_transfers) g_show_transfers = false;
                else if(R_SUCCEEDED(mount_result)) go_parent();
            }
            if(down & HidNpadButton_Y) {
                if(!g_show_transfers && R_SUCCEEDED(mount_result)) load_directory();
            }
            if(down & HidNpadButton_Up && !g_show_transfers && selected > 0) selected--;
            if(down & HidNpadButton_Down && !g_show_transfers && selected < entry_count - 1) selected++;
            if(down & HidNpadButton_A) {
                if(g_show_transfers) g_show_transfers = false;
                else if(R_SUCCEEDED(mount_result)) enter_selected();
            }
            if(down & HidNpadButton_L && !g_show_transfers && R_SUCCEEDED(mount_result)) rename_selected();
            if(down & HidNpadButton_R && !g_show_transfers && R_SUCCEEDED(mount_result)) delete_selected();
            if(down & HidNpadButton_ZL && !g_show_transfers && R_SUCCEEDED(mount_result)) copy_selected();
            if(down & HidNpadButton_ZR && !g_show_transfers && R_SUCCEEDED(mount_result)) move_selected();
            if(down & HidNpadButton_Minus && !g_show_transfers && R_SUCCEEDED(mount_result)) show_info();
            if(down & HidNpadButton_StickL) g_show_transfers = !g_show_transfers;
            if(was_showing_transfers && !g_show_transfers && R_SUCCEEDED(mount_result))
                load_directory();
            refreshText();
        });
        refreshText();
    }
};
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    __nx_fsdev_support_cwd = false;
    mount_result = fsdevMountSdmc();
    if(R_SUCCEEDED(mount_result)) { load_directory(); ftpServerStart(); }
    else diagnose_mount_error(mount_result);
    auto opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    opts.UseImage(pu::ui::render::ImgAllFlags);
    opts.SetPlServiceType(PlServiceType_User);
    opts.AddDefaultAllSharedFonts();
    opts.SetInputPlayerCount(1);
    opts.AddInputNpadStyleTag(HidNpadStyleSet_NpadStandard);
    opts.AddInputNpadIdType(HidNpadIdType_Handheld);
    opts.AddInputNpadIdType(HidNpadIdType_No1);
    auto renderer = pu::ui::render::Renderer::New(opts);
    auto app = HubApplication::New(renderer);
    Result rc = app->Load();
    if(R_FAILED(rc)) diagAbortWithResult(rc);
    app->Show();
    ftpServerStop();
    if(R_SUCCEEDED(mount_result)) fsdevUnmountDevice("sdmc");
    return 0;
}
