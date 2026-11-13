# Relatório de testes — Switch File Hub

## Resultado dos workflows GitHub Actions

### NRO Nintendo Switch
- Workflow: [Build Switch Homebrew NRO — execução 43478119888](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43478119888)
- Commit validado: [036989b95d10d46a09fe3f515800c5ea11515be3](https://github.com/iikawalucas-13/SwitchFileHub-/commit/036989b95d10d46a09fe3f515800c5ea11515be3)
- Job **Nintendo Switch NRO**: sucesso.
- Log confirma compilação de `ftp_server.c`, assinatura `NRO0` e upload do artifact.
- Artifact: [SwitchFileHub-Nintendo-Switch.zip](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43478119888/artifacts/14064206994) (ID 14064206994).

### Cliente Flutter Android e Windows
- Workflow: [Build Switch File Hub — execução 43477269728](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43477269728)
- Commit validado: [c40ec328f1c7110bcd1668953f6877cf050d5c15](https://github.com/iikawalucas-13/SwitchFileHub-/commit/c40ec328f1c7110bcd1668953f6877cf050d5c15)
- Jobs **Android APK** e **Windows 11 x64**: sucesso.
- `flutter analyze` concluiu sem erros fatais, mas registrou avisos/informações não fatais (incluindo um import não utilizado e avisos de APIs Flutter depreciadas).
- Testes de formato/unidade: 14 aprovados. Teste de widget: 1 aprovado.
- APK: [SwitchFileHub-Android-APK](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43477269728/artifacts/14063797023) (ID 14063797023).
- Windows x64: [SwitchFileHub-Windows-x64](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43477269728/artifacts/14063642032) (ID 14063642032).

## Código integrado no NRO
- O NRO inicia servidor FTP no próprio homebrew, na porta TCP 5000; não é necessário abrir um ftpd separado.
- Implementados PASV/EPSV, login anônimo, MLSD/LIST, PWD/CWD/CDUP, SIZE, STOR/RETR, MKD, DELE, RMD e RNFR/RNTO.
- O servidor está configurado para até quatro sessões de controle simultâneas, compatíveis com a sessão de navegação e as transferências separadas do cliente.
- Caminhos remotos são limitados ao dispositivo `sdmc:/`; segmentos `..` e prefixos de dispositivo são rejeitados.
- Upload é escrito em arquivo temporário e publicado por renomeação após a transferência.

## O que os testes CI NÃO comprovam
- O NRO foi compilado e validado quanto à assinatura do arquivo, mas não foi executado em Nintendo Switch físico.
- O cliente Flutter foi compilado para Android e Windows, mas o CI não conectou esses clientes ao NRO integrado.
- Os testes automatizados do cliente verificam formatos, parsers e widgets; não são um teste de rede com o servidor NRO.
- Permanecem pendentes no hardware: conexão e descoberta na rede local, listagem, upload, download, criação/renomeação/exclusão, quatro sessões, cancelamento, desconexão, suspensão e estabilidade do microSD.

## Limitações e segurança
- FTP anônimo e sem criptografia. Use somente em rede local confiável; não encaminhe a porta 5000 à internet.
- O NRO precisa permanecer aberto durante a conexão.
- Não há FTPS/SFTP nem retomada de transferência interrompida.
- O limite de quatro sessões e a identificação do IP precisam ser confirmados no Switch físico.

## Conclusão
**Entrega de código e CI parcialmente validada; teste funcional no Nintendo Switch físico ainda pendente.** Não considerar o projeto pronto para entrega final até concluir o teste real e confirmar que o cliente Android/Windows conecta ao servidor integrado sem iniciar ftpd separado.


## Atualização — integração GUI Plutonium (13/11/2026)

- Branch: `feat/switch-gui-plutonium`.
- Commit do NRO: `8224651d06516cc095bf307fe38fa70fa2e0ef7e`.
- Workflow NRO: [execução 43486919603](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486919603) — job Nintendo Switch NRO concluído com sucesso.
- Etapas aprovadas: compilação do Plutonium, compilação do homebrew, validação da assinatura `NRO0`, montagem do pacote para microSD e publicação do artifact.
- Artifact: [SwitchFileHub-Nintendo-Switch.zip](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486919603) (ID 14067243923).
- Inspeção do ZIP: contém `switch-homebrew/SwitchFileHub.nro` e `SwitchFileHub-SD.zip`; o pacote SD contém `switch/SwitchFileHub.nro`.
- Tamanho do NRO: 9.383.341 bytes. Assinatura `NRO0` confirmada no deslocamento esperado.
- SHA-256 do NRO: `93048dd07291b3177f6de49c59452875a9719633fa0fcd2e881341ce95480ec8`.
- Cliente Android/Windows: execução [43486780996](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486780996) ainda estava em andamento na última consulta; nenhum resultado final foi declarado.
- Teste em Switch físico: não executado por este ambiente. Conectividade Wi-Fi, operações FTP reais, interface no console e estabilidade permanecem pendentes.

### Cliente Android e Windows — execução 43486780996

- Workflow [Build Switch File Hub](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486780996) — ambos os jobs concluídos com sucesso.
- Windows 11 x64: compilação Release aprovada; artifact [SwitchFileHub-Windows-x64](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486780996/artifacts/14067478694), ID 14067478694. O ZIP interno contém EXE, DLLs e pasta `data`.
- Android: compilação `app-release.apk` aprovada; artifact [SwitchFileHub-Android-APK](https://github.com/iikawalucas-13/SwitchFileHub-/actions/runs/43486780996/artifacts/14066974813), ID 14066974813. APK extraído e estrutura ZIP validada.
- `flutter analyze` concluiu sem erros fatais. Testes registrados no log: 14 testes de unidade/formato e 1 teste de widget aprovados.
- Avisos observados: plugin `desktop_drop` usa Kotlin Gradle Plugin e poderá exigir migração em versões futuras do Flutter; Android também registrou mensagem do watcher de diretório já monitorado, mas a compilação terminou com sucesso.
- Esses resultados validam compilação e testes automatizados, não execução do APK em telefone nem do EXE em outro PC.
