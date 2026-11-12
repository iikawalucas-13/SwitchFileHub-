# Switch File Hub — Homebrew para Nintendo Switch

## Estado desta versão

Esta pasta contém o cliente homebrew nativo do Switch. A versão 0.1.1 oferece somente:
- montagem do microSD;
- navegação entre diretórios;
- atualização da listagem;
- mensagens visíveis quando a montagem ou abertura de uma pasta falha.

**Ainda não oferece transferência de arquivos, servidor FTP, cliente MTP ou integração com o DBI.** A compilação bem-sucedida do NRO confirma apenas que o binário foi gerado; não confirma funcionamento em um console real.

## Como compilar

O projeto usa devkitPro, devkitA64 e libnx:

```bash
cd switch-homebrew
make
```

O resultado esperado é `SwitchFileHub.nro`. O workflow do GitHub Actions verifica se o arquivo não está vazio e se contém a assinatura `NRO0` no cabeçalho.

## Instalar pelo cartão SD

O pacote `SwitchFileHub-SD.zip` publicado pelo GitHub Actions já contém a estrutura:

```
switch/
└── SwitchFileHub.nro
```

Extraia o conteúdo na raiz do microSD, mantendo a pasta `switch`. O caminho final recomendado é:

```
/switch/SwitchFileHub.nro
```

Também é válido usar uma subpasta com o mesmo nome do executável:

```
/switch/SwitchFileHub/SwitchFileHub.nro
```

O Homebrew Menu procura arquivos `.nro` dentro de `/switch`. A ausência de ícone pode afetar a aparência da entrada, mas não é, por si só, prova de que o executável não foi encontrado. Se o aplicativo não aparecer, confira primeiro o caminho e se o Homebrew Menu está lendo o mesmo microSD.

## Controles

- Direcional para cima/baixo: selecionar item.
- A: abrir pasta.
- B: voltar uma pasta.
- Y: atualizar listagem.
- +: sair.

## Próximas etapas

1. Testar a navegação em um Switch real e registrar a mensagem de estado exibida.
2. Definir uma arquitetura de comunicação compatível entre o cliente desktop/mobile e o console.
3. Implementar transferências com validação de caminhos, confirmação de sobrescrita e tratamento de interrupções.
4. Testar cada operação em hardware antes de declarar a versão pronta.

O cliente Flutter documentado em `MANUAL_SWITCH.md` usa um servidor FTP separado (por exemplo, o homebrew ftpd). Esse servidor não é iniciado por este NRO.
