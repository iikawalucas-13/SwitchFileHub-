# Funcionalidades implementadas no código (não validadas por compilação — ver RELATORIO_TESTES.md)

**Conexão FTP:** IP, porta, usuário/senha opcionais; testar conexão; conectar/desconectar; MLSD com fallback para LIST; reconexão automática em falha de rede (restaura a pasta); novas tentativas de transferência (até 2).
**Arquivos do Switch:** navegar, listar com tamanhos, criar pasta, renomear, excluir (arquivos e pastas, com confirmação), seleção múltipla.
**Transferências:** envio e download de arquivos e pastas inteiras; em fluxo (sem carregar na memória); fila com até 3 simultâneas (conexão própria por transferência); progresso, velocidade, tempo restante; cancelamento (remove parcial); download via arquivo `.sfhpart` renomeado só ao concluir; nunca substitui sem perguntar (substituir / pular existentes / cancelar).
**Windows:** painel do computador (unidades, navegar, criar/renomear/excluir com confirmação), arrastar e soltar, escolher destino de download.
**Android:** envio por seletor de arquivos/pasta, download para pasta do app (ou escolhida), layout com menu lateral recolhível.
**Detecção automática:** botão "Procurar Switch na rede" varre a sub-rede /24 privada na porta escolhida e lista servidores FTP (banner 220).
**Histórico** persistente (500 itens); **configurações** persistentes (a senha nunca é salva).
**Segurança:** validação de nomes (FAT32/exFAT), bloqueio de `..` e CR/LF em comandos, download restrito à pasta escolhida, senha fora de logs.
**Windows↔Android:** via microSD do Switch como intermediário, claramente identificado (tela Dispositivos).
