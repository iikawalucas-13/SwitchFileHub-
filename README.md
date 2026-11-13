# Switch File Hub

Gerenciador de arquivos para o microSD do Nintendo Switch, com cliente Flutter para Windows 11 e Android.

O NRO na branch `feat/switch-logo-nro` está sendo atualizado para iniciar um servidor FTP integrado ao homebrew na porta TCP 5000. A meta é conectar o cliente Android/Windows diretamente ao Switch, sem iniciar ftpd separado.

- Manual: `MANUAL_SWITCH.md`
- Funcionalidades: `FUNCIONALIDADES.md`
- Limitações: `LIMITACOES.md`
- Relatório de testes: `RELATORIO_TESTES.md`

**Status:** o código integrado está em desenvolvimento. Não confunda compilação CI com teste em console real. FTP é anônimo e sem criptografia nesta versão; use somente em rede local confiável.
