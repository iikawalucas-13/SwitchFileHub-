# Manual de conexão com o Nintendo Switch

Requisitos: Switch com homebrew (CFW) e o app **ftpd** (mtheall/ftpd) instalado; Switch e PC/celular na **mesma rede Wi-Fi**.

1. No Switch, abra o **ftpd** pelo Homebrew Menu. A tela mostra o IP (ex.: `192.168.1.25`) e a porta (padrão **5000**).
2. No Switch File Hub, menu **Conectar Switch**: informe IP e porta. Usuário/senha em branco = acesso anônimo (padrão do ftpd; preencha só se você configurou credenciais).
3. Toque em **Testar conexão** (confere IP/porta e lista a raiz) e depois **Conectar**.
4. Em **Gerenciador de arquivos**: navegue, marque itens, **Enviar arquivos/pasta**, **Baixar**, **Nova pasta**, **Renomear**, **Excluir** (sempre com confirmação). No Windows também dá para arrastar arquivos para a janela.
5. Acompanhe em **Transferências** (progresso, velocidade, tempo restante, cancelar) e **Histórico**.
6. Ao terminar, **Desconectar** e feche o ftpd no Switch.

Problemas comuns
- *Tempo esgotado*: IP/porta errados, redes diferentes, ou ftpd fechado. O Switch em modo de espera encerra o FTPD.
- *421 / muitas conexões*: reduza **Transferências simultâneas** para 1 em Configurações.
- *Listagem vazia/erro*: em Configurações troque o comando de listagem para **LIST**.
- Não envie nem apague arquivos essenciais do sistema (ex.: `/atmosphere`, `/switch` de outros apps) sem saber o que faz.
- FTP não é criptografado: use somente em rede doméstica confiável.
