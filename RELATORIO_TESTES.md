# Relatório de testes — Switch File Hub

## Código e validações
- O NRO foi atualizado para iniciar um servidor FTP integrado ao próprio homebrew, na porta TCP 5000.
- O servidor implementa login anônimo, PASV/EPSV, MLSD/LIST, PWD/CWD/CDUP, SIZE, STOR/RETR, MKD, DELE, RMD e RNFR/RNTO.
- Os caminhos do FTP são convertidos para o dispositivo `sdmc:/` e rejeitam segmentos de travessia `..` e prefixos de dispositivo.
- Upload usa um arquivo temporário e só renomeia para o destino após o recebimento; o cliente deve confirmar a substituição quando necessário.
- A implementação foi submetida ao workflow **Build Switch Homebrew NRO**. Os logs reais do Actions devem ser consultados para o commit mais recente antes de considerar a compilação validada.

## Testes ainda necessários
- Teste em Nintendo Switch físico: iniciar o NRO, confirmar IP/porta, conectar por Android e Windows, listar arquivos, enviar e baixar arquivos, criar pasta, renomear, excluir, desconectar e fechar o NRO.
- Testar transferência interrompida, pouco espaço no microSD, suspensão/retorno do console e desconexão Wi-Fi.
- Confirmar a compilação Android APK e Windows x64 pelo workflow do cliente.
- Testar descoberta automática e conexão manual por IP/porta com o servidor integrado.
- Confirmar que o aplicativo não exige iniciar ftpd separado.

## Limitações
- FTP anônimo, sem criptografia; somente rede local confiável.
- Uma sessão de controle por vez; não há FTPS/SFTP nem retomada REST.
- A compilação CI não comprova funcionamento em hardware real.
- O cliente Flutter está dentro de `SwitchFileHub_1.0_FINAL.zip`; sua validação precisa ocorrer no workflow Android/Windows.

## Resultado honesto
**Em desenvolvimento.** O servidor FTP integrado foi adicionado ao código do NRO. A entrega só poderá ser considerada validada após compilações relevantes do NRO e do cliente, inspeção dos artefatos/logs e teste funcional no console físico.
