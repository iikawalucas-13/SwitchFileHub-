# Switch File Hub — Homebrew para Nintendo Switch

## Estado desta versão

O NRO contém:
- navegação e gerenciamento local do microSD;
- criação de pastas, renomeação e exclusão;
- cópia e movimentação de arquivos dentro do microSD;
- servidor FTP integrado ao próprio homebrew, iniciado automaticamente na porta TCP 5000;
- suporte FTP passivo, MLSD/LIST, STOR/RETR e operações básicas de diretório/arquivo.

**Não é necessário iniciar o ftpd separado.** Mantenha o Switch File Hub aberto durante a conexão. O acesso FTP desta versão é anônimo e sem criptografia; use somente em rede local confiável.

A compilação do NRO pelo GitHub Actions valida a geração do binário, mas não substitui teste em um Nintendo Switch físico. O teste em hardware permanece pendente.

## Como compilar

O projeto usa devkitPro, devkitA64 e libnx:

```bash
cd switch-homebrew
make
```

O resultado esperado é `SwitchFileHub.nro`. O workflow verifica se o arquivo não está vazio e se contém a assinatura `NRO0`.

## Instalar pelo cartão SD

Extraia o conteúdo de `SwitchFileHub-SD.zip` na raiz do microSD. O caminho final recomendado é:

```
/switch/SwitchFileHub.nro
```

## Usar o servidor FTP integrado

1. Abra o NRO pelo Homebrew Menu.
2. Confira na tela o estado do FTP e o endereço IP (porta 5000).
3. No aplicativo Android/Windows, procure o Switch na rede ou informe o IP e a porta 5000.
4. Deixe usuário e senha em branco nesta versão.
5. Faça as transferências e mantenha o NRO aberto até terminar.

Se o IP não aparecer no NRO, consulte o IP do Switch nas configurações de Internet ou use a descoberta automática no cliente.

## Controles locais

- Direcional para cima/baixo: selecionar item.
- A: abrir pasta.
- B: voltar uma pasta.
- Y: atualizar listagem.
- X: criar pasta.
- L: renomear.
- R: excluir (confirmação por texto).
- ZL: copiar arquivo.
- ZR: mover arquivo.
- -: informações.
- +: sair (encerra o servidor FTP).

## Limitações importantes

- Até quatro sessões FTP simultâneas estão previstas no código; esse limite ainda precisa ser validado no console físico.
- FTP anônimo e sem criptografia; não exponha a porta 5000 à internet.
- O NRO precisa permanecer aberto.
- O teste em console físico ainda é necessário para confirmar rede e operações de arquivos.
