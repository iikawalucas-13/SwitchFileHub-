# Switch File Hub

Gerenciador de arquivos para o microSD do Nintendo Switch, com cliente Flutter para Windows 11 e Android.

O NRO na branch `feat/switch-gui-plutonium` já compila com interface Plutonium e servidor FTP integrado na porta TCP 5000. O artifact do NRO e do pacote para microSD foi gerado pelo GitHub Actions; ainda falta validar o funcionamento em um Nintendo Switch físico.

- Manual: `MANUAL_SWITCH.md`
- Funcionalidades: `FUNCIONALIDADES.md`
- Limitações: `LIMITACOES.md`
- Relatório de testes: `RELATORIO_TESTES.md`

**Status:** compilação do NRO validada no GitHub Actions (execução 43486919603). O teste no console físico ainda está pendente. O cliente Android/Windows tem workflow próprio em execução; não confunda build com teste de conexão real. FTP é anônimo e sem criptografia nesta versão; use somente em rede local confiável.
