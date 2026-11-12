# Política de segurança do servidor MCP

- Escopo inicial: leitura do estado do repositório e de arquivos públicos selecionados.
- Não registrar nem imprimir tokens.
- Token GitHub deve ser fornecido por variável de ambiente/secret manager.
- Nunca aceitar caminhos arbitrários em leitura de arquivos.
- Nunca disponibilizar endpoint remoto sem autenticação forte e HTTPS.
- Antes de habilitar operações de escrita, exigir confirmação explícita, criar branch e abrir PR.
- Não mesclar PRs nem publicar Releases automaticamente.
