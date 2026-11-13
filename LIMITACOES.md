# Limitações conhecidas — Switch File Hub

## Nintendo Switch / servidor FTP integrado
- O NRO agora contém código de servidor FTP integrado, iniciado ao abrir o Switch File Hub; não deve ser necessário abrir um ftpd separado.
- O servidor usa a porta TCP 5000, modo passivo FTP e uma sessão de controle por vez.
- O acesso é anônimo e o FTP não é criptografado. Use apenas em rede local confiável; não encaminhe a porta para a internet.
- O NRO precisa permanecer aberto durante o uso. Fechar o homebrew encerra o servidor.
- A identificação automática do IP depende da interface de rede ativa; se o endereço não aparecer, consulte o IP nas configurações de Internet do Switch ou use a descoberta de dispositivos do cliente.
- A implementação foi compilada em CI, mas ainda não foi validada em um Switch físico. Operações reais de listagem, upload, download, renomeação, exclusão, desconexão e suspensão do console continuam pendentes de teste no hardware.
- O servidor atende até quatro sessões FTP simultâneas no código (pendente de validação em hardware); transferências concorrentes e vários clientes simultâneos não foram validados.
- FTP transmite dados sem criptografia e sem autenticação. Não é adequado para redes públicas ou não confiáveis.
- O servidor não implementa FTPS/SFTP, retomada de transferência (REST), nem cópia/movimentação recursiva pelo próprio NRO.
- O upload usa arquivo temporário antes de publicar o destino; ainda precisa ser testado em interrupções reais e em cartões microSD com pouco espaço.

## Cliente Android / Windows
- O código Flutter está empacotado em `SwitchFileHub_1.0_FINAL.zip`; o cliente usa FTP passivo, MLSD/LIST, STOR/RETR e descoberta de servidores por banner FTP na porta configurada.
- A compilação e os testes do cliente Android/Windows precisam ser confirmados pelos workflows GitHub Actions após a inclusão da branch de integração.
- Android: seleção de arquivos e destino de downloads dependem das restrições de armazenamento do Android (Scoped Storage); validar em dispositivo real.
- Windows: alvo x64.
- Sem retomada automática de transferência interrompida.
- FTP sem criptografia: não use em redes públicas nem exponha o serviço à internet.

## Testes de entrega
- Uma compilação CI bem-sucedida confirma somente a geração e validação do artefato no runner.
- O funcionamento real depende de teste no Switch físico e de teste do cliente em Android/Windows na mesma rede.
