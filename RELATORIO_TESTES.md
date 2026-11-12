# Relatório de testes — Switch File Hub 1.0

## Executado neste ambiente
- Leitura e análise completa do ZIP original (586 linhas em `main.dart`).
- Conferência de balanceamento de chaves/parênteses nos 17 arquivos Dart.
- Conferência em Python das expressões regulares de MLSD/LIST/PASV usadas pelo cliente FTP.

## NÃO executado (ambiente sem Flutter/Dart e sem rede para instalar)
- `flutter pub get`, `flutter analyze`, `flutter test`
- Compilação Windows (`.exe`) e Android (`.apk`), geração do instalador
- Testes de integração contra servidor FTP (existem em `test/integration`, rodam no CI com pyftpdlib)
- Conexão com Nintendo Switch real

## Testes escritos (para rodar no CI ou localmente)
| Arquivo | Cobre |
|---|---|
| `test/format_test.dart` | validação de nomes/caminhos, anti-traversal, tamanhos/ETA, parsers MLSD/LIST, JSON do histórico |
| (em `format_test.dart`) | cálculo da faixa de IPs da detecção automática |
| `test/widget_test.dart` | abertura do app, navegação Início → Conectar Switch |
| `test/integration/ftp_integration_test.dart` | conexão, listagem, criar pasta, renomear, excluir, upload/download de 20 MB com comparação byte a byte, não-substituição silenciosa, cancelamento de upload de 400 MB com remoção do parcial, conexão recusada |

## Mudança de arquitetura que você deve saber
O pacote `ftpconnect` foi **removido**: sua documentação pública não confirmou a API que o protótipo usava e ele não permite cancelar transferências. Foi substituído por um cliente FTP próprio (`ftp_client.dart`). Isso é código novo e ainda sem execução.

## Próximo passo
Rodar o workflow `build.yml` (ou `build_windows.bat`), corrigir o que o `flutter analyze`/testes apontarem e testar com o Switch real (o ftpd aberto na rede local).
