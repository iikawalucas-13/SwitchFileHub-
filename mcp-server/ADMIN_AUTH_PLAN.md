# Login Google e perfil ADMIN

## Regra de acesso solicitada

O aplicativo terá um único APK e dois perfis após o login Google:

- **ADMIN:** somente a identidade Google autorizada pelo backend. Identidade administrativa informada pelo proprietário será configurada somente no ambiente privado do backend, nunca publicada no repositório.
- **NORMAL:** qualquer outra conta Google autenticada.

## Configuração privada

O backend deverá receber estas variáveis pelo gerenciador de segredos do serviço de hospedagem:

- `ADMIN_GOOGLE_EMAIL`: endereço do administrador.
- `GOOGLE_OAUTH_CLIENT_ID`: Client ID OAuth esperado para o aplicativo.

O endereço administrativo não deve ser usado como prova de identidade. O servidor precisa validar a assinatura do ID token do Google, conferir o `aud` com o Client ID esperado, conferir `email_verified=true` e só então comparar o e-mail validado com `ADMIN_GOOGLE_EMAIL`. Nunca confiar em um e-mail enviado diretamente pelo APK, em cabeçalho HTTP ou em um campo editável pelo usuário.

## Estado desta etapa

Este documento e o arquivo de exemplo apenas registram a configuração planejada. **O login Google, a verificação de token, a autorização ADMIN/NORMAL e a integração no APK ainda não estão implementados.** O servidor MCP atual continua local/stdio e somente leitura. Não há servidor remoto hospedado.

## Próximos passos técnicos

1. Configurar o projeto Google/Firebase para autenticação no Android.
2. Integrar o login Google ao código Flutter real que está dentro do ZIP do aplicativo.
3. Validar o token no backend e aplicar o papel ADMIN/NORMAL em todas as rotas administrativas.
4. Manter a IA e as ações administrativas fora do alcance de contas NORMAL.
5. Criar builds de teste e validar com duas contas antes de publicar.

Nunca incluir chaves privadas, tokens ou segredos no APK ou no repositório.
