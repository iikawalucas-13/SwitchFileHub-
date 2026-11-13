# Funcionalidades no código — Switch File Hub

> As funcionalidades descritas abaixo estão presentes no código-fonte. A validação depende dos workflows e de testes em dispositivos reais; consulte `RELATORIO_TESTES.md`.

## Nintendo Switch — NRO
- Navegação e gerenciamento local do microSD.
- Criar pastas, renomear, excluir, copiar e mover arquivos.
- Servidor FTP iniciado pelo próprio NRO, sem abrir um homebrew ftpd separado.
- Porta TCP 5000; FTP passivo (PASV/EPSV); sessão anônima.
- Listagem MLSD/LIST, PWD/CWD/CDUP, SIZE, STOR/RETR, MKD, DELE, RMD e RNFR/RNTO.
- Upload para arquivo temporário, com renomeação ao concluir; caminhos limitados a `sdmc:/`.

## Cliente Flutter — Android e Windows
- Conexão por IP e porta, teste de conexão e desconexão.
- Descoberta automática de servidores FTP na sub-rede privada /24 pela porta escolhida (banner 220).
- MLSD com fallback para LIST; modo passivo FTP; upload/download em fluxo.
- Reconexão em falha de rede, progresso, velocidade, tempo restante, cancelamento e histórico.
- Gerenciamento remoto: navegar, criar pasta, renomear, excluir e transferir arquivos/pastas.
- Windows: navegação no computador e arrastar/soltar.
- Android: seletor de arquivos e downloads para pasta do aplicativo ou destino escolhido.

## Limitações
- FTP anônimo e sem criptografia; use somente em rede local confiável.
- NRO precisa permanecer aberto; até quatro sessões FTP simultâneas no código (pendente de validação em hardware).
- Não há FTPS/SFTP nem retomada de transferência interrompida.
- A descoberta depende de dispositivos na mesma sub-rede privada /24.
- Compatibilidade e estabilidade precisam ser verificadas em Switch físico, Android e Windows.
