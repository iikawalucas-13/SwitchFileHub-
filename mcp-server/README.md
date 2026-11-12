# Switch File Hub — servidor MCP

Servidor MCP inicial, **somente leitura**, para consultar o repositório e acompanhar GitHub Actions.

## O que já oferece

- `get_repository_status`: branch padrão, branches e PRs abertas.
- `list_recent_workflow_runs`: execuções recentes do Actions.
- `get_workflow_run_details`: conclusão e jobs de uma execução.
- `read_project_file`: leitura de uma lista restrita de documentação/configuração.

Não há ferramentas para editar arquivos, criar releases, mesclar PRs, alterar configurações ou acessar segredos.

## Requisitos

- Node.js 20 ou superior.
- Um token GitHub com acesso **somente de leitura** ao repositório. Para repositório público, parte das consultas funciona sem token, mas o token ajuda com limites de API.

## Executar localmente

1. Copie `.env.example` para `.env`.
2. Preencha `GITHUB_TOKEN` com um token de leitura. Não compartilhe esse token e não o envie ao app.
3. Execute:

```bash
npm install
npm run check
npm run build
npm start
```

O servidor usa transporte MCP stdio. Para clientes locais compatíveis, configure o comando Node para executar `mcp-server/dist/server.js` e forneça as variáveis de ambiente no ambiente local seguro.

## Conexão remota ao ChatGPT

Este commit **não publica um servidor remoto** e ainda não conecta automaticamente sua conta ChatGPT. Para uma conexão remota, será necessário hospedar o servidor em HTTPS e implementar autenticação OAuth 2.1 compatível com o fluxo MCP do ChatGPT. Não exponha o servidor na internet sem autenticação.

A disponibilidade de servidores MCP personalizados depende do plano e da interface do ChatGPT. A configuração de apps MCP personalizados é feita na web, não no aplicativo móvel; consulte a documentação oficial antes de tentar conectar.

## Segurança

- Nunca coloque tokens em código, commits, APKs ou arquivos de exemplo.
- Use o menor escopo de GitHub possível.
- Mantenha o serviço privado e autentique todos os usuários.
- As ferramentas desta primeira versão são somente leitura.
- Qualquer futura função de escrita deverá criar uma branch e um Pull Request para revisão; nunca mesclar automaticamente.

## Limitações atuais

- Não controla o Nintendo Switch nem o DBI.
- Não instala NRO/APK no dispositivo.
- Não publica APK nem cria GitHub Releases.
- Não altera o código do app Flutter que está empacotado no ZIP.
- Não faz conexão direta entre o app Android e o ChatGPT.
