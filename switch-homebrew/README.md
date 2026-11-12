# Switch File Hub — Homebrew para Nintendo Switch

Este diretório contém o aplicativo nativo para Nintendo Switch, separado dos clientes Flutter para Android e Windows.

## Estado atual

**Protótipo inicial:** interface de console, montagem do microSD e navegação básica entre diretórios. Ainda não implementa servidor de rede nem transferência de arquivos. Não considerar esta versão como final.

## Compilar

O projeto usa devkitPro, devkitA64 e libnx:

```bash
cd switch-homebrew
make
```

O resultado esperado é `SwitchFileHub.nro`.

## Instalar no cartão microSD

Depois de compilar, copie o arquivo para:

```
/switch/SwitchFileHub/SwitchFileHub.nro
```

Abra pelo Homebrew Menu. Faça backup dos dados importantes antes de testar qualquer homebrew.

## Controles previstos

- Direcional para cima/baixo: selecionar item.
- A: abrir pasta.
- B: voltar uma pasta.
- Y: atualizar listagem.
- +: sair.

## Próximas etapas

1. Validar compilação e navegação em hardware.
2. Implementar serviço de rede para comunicação com os clientes Android e Windows.
3. Adicionar operações de transferência com confirmação e tratamento de erros.
4. Integrar o protocolo e os estados de conexão com os clientes existentes.
