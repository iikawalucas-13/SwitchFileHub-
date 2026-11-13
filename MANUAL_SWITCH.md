# Manual de conexão com o Nintendo Switch

## Requisitos
- Nintendo Switch com homebrew/CFW e microSD acessível.
- Switch e celular/PC conectados à mesma rede Wi-Fi.
- Switch File Hub instalado no microSD em `/switch/SwitchFileHub.nro`.

## Iniciar o servidor integrado
1. Abra o **Switch File Hub** pelo Homebrew Menu.
2. Mantenha o aplicativo aberto. Ele inicia o servidor FTP automaticamente na porta **5000**.
3. Na tela do NRO, confira a linha **FTP: ATIVO** e o endereço IP mostrado. Se o IP aparecer como indisponível, consulte o endereço IP do Switch nas configurações de Internet do console ou use a busca de dispositivos no aplicativo.
4. No Android/Windows, abra **Conectar Switch** e toque em **Procurar Switch na rede**. Se preferir, informe manualmente o IP do console e a porta **5000**.
5. Deixe usuário e senha em branco (servidor anônimo desta versão), toque em **Testar conexão** e depois em **Conectar**.
6. Use o gerenciador para listar pastas, criar pastas, renomear, excluir, enviar e baixar arquivos.
7. Ao terminar, desconecte no cliente e feche o Switch File Hub no console. O servidor é encerrado junto com o NRO.

## Segurança e limitações
- O FTP desta versão não usa criptografia nem autenticação. Use somente em uma rede local confiável; não exponha a porta 5000 à internet.
- O servidor aceita uma sessão de controle por vez.
- Upload é gravado em arquivo temporário e finalizado por renomeação; arquivos parciais são removidos quando a transferência falha.
- A substituição de arquivo deve ser confirmada no aplicativo antes do envio.
- O servidor integrado foi compilado no GitHub Actions, mas ainda precisa de teste em um Nintendo Switch físico. A compilação não comprova conectividade Wi-Fi, estabilidade ou compatibilidade completa.
- O NRO precisa permanecer aberto durante a conexão e a transferência.
- Se a conexão falhar, confirme que o Switch e o dispositivo estão na mesma rede, que o servidor mostra **FTP: ATIVO** e que a porta 5000 não está bloqueada.
