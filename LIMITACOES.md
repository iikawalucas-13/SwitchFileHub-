# Limitações conhecidas

- **Nada foi compilado ou executado ainda**; podem existir erros de compilação a corrigir no primeiro `flutter analyze`.
- **Teste com Nintendo Switch real: PENDENTE.**
- **Windows ↔ Android não é direto**: passa pelo microSD do Switch (pasta `/SwitchFileHub_Troca`).
- Sem retomada de transferência interrompida (recomeça do início).
- Pastas locais vazias não são recriadas no envio de pastas; arquivos vazios são enviados.
- Espaço livre do microSD não é exibido (FTP padrão não informa; o ftpd não fornece).
- FTP sem criptografia (ftpd não suporta FTPS).
- Android: `file_picker` copia arquivos escolhidos para o cache do app antes do envio (usa espaço temporário); envio de pasta e destino de download fora da pasta do app dependem das restrições de armazenamento do Android (Scoped Storage) e não foram validados.
- Exclusão no computador é definitiva (não usa a Lixeira).
- Credenciais: a senha não é persistida (precisa ser digitada a cada sessão) por decisão de segurança.
- Instalador e .exe dependem de rodar o workflow/script (não incluídos).
- Windows 11 apenas x64.
