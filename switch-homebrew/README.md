# Switch File Hub — Homebrew para Nintendo Switch

## Estado desta versao

Esta pasta contem o cliente homebrew nativo do Switch. A versao 0.1.1 oferece somente:
- montagem do microSD;
- navegacao entre diretorios;
- atualizacao da listagem;
- mensagens visiveis quando a montagem ou abertura de uma pasta falha.

**Ainda nao oferece transferencia de arquivos, servidor FTP, cliente MTP ou integracao com o DBI.** A compilacao bem-sucedida do NRO confirma apenas que o binario foi gerado; nao confirma funcionamento em um console real.

## Como compilar

O projeto usa devkitPro, devkitA64 e libnx:

```bash
cd switch-homebrew
make
```

O resultado esperado e `SwitchFileHub.nro`.

## Instalar

Copie o NRO para:

```
/switch/SwitchFileHub/SwitchFileHub.nro
```

Abra pelo Homebrew Menu. Faca backup dos dados importantes antes de testar homebrew.

## Controles

- Direcional para cima/baixo: selecionar item.
- A: abrir pasta.
- B: voltar uma pasta.
- Y: atualizar listagem.
- +: sair.

## Proximas etapas

1. Testar a navegacao em um Switch real e registrar a mensagem de estado exibida.
2. Definir uma arquitetura de comunicacao compativel entre o cliente desktop/mobile e o console.
3. Implementar transferencias com validacao de caminhos, confirmacao de sobrescrita e tratamento de interrupcoes.
4. Testar cada operacao em hardware antes de declarar a versao pronta.

O cliente Flutter documentado em `MANUAL_SWITCH.md` usa um servidor FTP separado (por exemplo, o homebrew ftpd). Esse servidor nao e iniciado por este NRO.
