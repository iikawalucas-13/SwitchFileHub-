# Switch File Hub 1.0 (código-fonte)

Gerenciador de arquivos para o microSD do Nintendo Switch (servidor FTPD no console), com apps Flutter para **Windows 11** e **Android**.

> **Status honesto:** o código foi escrito e revisado, mas **ainda não foi compilado nem executado** (o ambiente onde foi produzido não tem Flutter/Dart nem acesso à rede para instalá-los). Os binários (`.exe`, instalador, `.apk`) **não estão neste ZIP**. Veja `RELATORIO_TESTES.md`.

## Como obter o .exe e o .apk (2 caminhos)

### A) GitHub Actions (recomendado — não exige instalar nada)
1. Crie um repositório no GitHub e envie o conteúdo desta pasta (a raiz do repositório deve conter `pubspec.yaml`).
2. Abra a aba **Actions** → workflow **Build Switch File Hub** (roda sozinho no push, ou use *Run workflow*).
3. O workflow: gera a estrutura nativa (`flutter create`), roda `flutter analyze`, os testes unitários, os **testes de integração contra um servidor FTP real (pyftpdlib)** e compila.
4. Baixe os artefatos: `SwitchFileHub-Windows` (ZIP com `SwitchFileHub.exe` + instalador `SwitchFileHub_Setup.exe`) e `SwitchFileHub-Android` (`SwitchFileHub.apk`).
5. Se algum passo falhar, o log mostra o erro — corrija e rode de novo.

### B) Localmente no Windows 11
Pré-requisitos: Flutter SDK no PATH, Visual Studio 2022 com "Desenvolvimento para desktop com C++", Modo de Desenvolvedor ligado. Depois, duplo clique em `build_windows.bat` (cria o projeto nativo, `pub get`, `analyze`, `test`, `build windows --release` e monta `dist\`).

Android local: após `setup_windows.ps1`, entre em `switch_file_hub` e rode `flutter build apk --release`.
(O manifesto com a permissão `INTERNET` fica em `tool/android/AndroidManifest.xml` e é copiado pelo script/CI.)

`backup_original/` guarda o ZIP original recebido, intacto.

## Estrutura
- `lib/services/ftp_client.dart` — cliente FTP próprio (dart:io): modo passivo, MLSD/LIST, STOR/RETR em fluxo, aborto imediato.
- `lib/services/ftp_session.dart` — reconexão automática, validação de nomes/caminhos, operações.
- `lib/services/transfer_manager.dart` — fila, progresso, velocidade, ETA, cancelamento, novas tentativas.
- `lib/state/app_controller.dart` — estado da aplicação.
- `lib/ui/` — tema escuro (preto/azul/roxo), menu lateral e 8 telas.
- `test/` — testes unitários, de widget e de integração (`test/integration`, precisa de servidor FTP).
- `tool/test_ftp_server.py` — servidor FTP de teste (pyftpdlib).
- `installer/SwitchFileHub.iss` — instalador Inno Setup.

## Documentos
`MANUAL_SWITCH.md` · `FUNCIONALIDADES.md` · `LIMITACOES.md` · `RELATORIO_TESTES.md`
