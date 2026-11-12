# Instruções para finalizar o Switch File Hub

Você está recebendo o projeto-fonte Switch File Hub 0.2.1. NÃO comece do zero e NÃO remova funcionalidades existentes. Trabalhe diretamente nesta base.

## Objetivo
Entregar uma versão Windows 11 Release funcional, com pasta de distribuição completa e ZIP, mantendo o caminho aberto para Android. O app conecta a um Nintendo Switch que esteja executando o homebrew FTPD, na mesma rede local.

## Funcionalidades que devem ser preservadas
- Interface em português e tema escuro.
- Conexão FTP com IP e porta configuráveis.
- Modo passivo e listagem de diretórios.
- Navegação entre pastas e retorno à pasta anterior.
- Envio de arquivos do PC/celular para o Switch.
- Download de arquivos do Switch para uma pasta local escolhida.
- Progresso de transferência.
- Criação de pastas remotas.
- Confirmação antes de sobrescrever arquivos.

## Trabalho obrigatório
1. Leia todo o código, pubspec, testes, README e scripts antes de alterar.
2. Execute `flutter --version`, `flutter doctor -v` e confirme os requisitos Windows Desktop.
3. Execute `flutter pub get`, `flutter analyze` e `flutter test`. Corrija todos os erros e avisos relevantes; não esconda erros com supressões desnecessárias.
4. Verifique a API e as versões reais de `ftpconnect` e `file_picker`. Ajuste o código se necessário, sem remover upload/download.
5. Revise tratamento de desconexão, falhas de rede, callbacks de progresso, estado após `await`, nomes de arquivos/pastas e caminhos locais. Evite que uma falha deixe o app preso em estado ocupado.
6. Confira a compatibilidade da listagem MLSD com o FTPD. Se necessário, implemente fallback seguro para LIST ou documente claramente a limitação; não invente compatibilidade.
7. Gere a estrutura nativa com Flutter se estiver faltando e compile com `flutter build windows --release`.
8. Verifique a pasta Release: EXE, DLLs, `data` e dependências necessárias. Teste o EXE em outro diretório ou PC Windows quando possível.
9. Se houver Switch real disponível, teste conexão, listagem, navegação, upload, download e criação de pasta. Não faça operações destrutivas nem apague conteúdo do microSD.
10. Revise o fluxo Android e permissões, mas não declare APK testado se não compilar e executar em dispositivo/emulador.
11. Atualize README e relatório com resultados verdadeiros, incluindo comandos executados, versões, erros encontrados e testes que não puderam ser feitos.

## Entrega obrigatória
- Projeto-fonte completo atualizado.
- Pasta `SwitchFileHub_Windows_Release` completa.
- `SwitchFileHub_Windows_Release.zip`.
- `RELATORIO_FINAL.md` com resultados verificáveis.
- Instruções curtas para executar e conectar ao Switch.

## Regras importantes
- Não diga que compilou ou testou se não executou de fato.
- Não entregue apenas trechos de código: devolva os arquivos completos.
- Não remova funcionalidades para fazer o build passar sem explicar e pedir aprovação.
- Preserve o nome Switch File Hub e o idioma português.
