# Configuração do Projeto BoxeIoT

## Configuração das Credenciais

1. Faça uma cópia do arquivo `config.example.h` e renomeie para `config.h`
2. Preencha o arquivo `config.h` com suas credenciais do Firebase:

```cpp
#define FIREBASE_PROJECT_ID "seu_project_id"
#define API_KEY "sua_api_key"
#define DATABASE_URL "sua_database_url"
#define USER_EMAIL "seu_email"
#define USER_PASSWORD "sua_senha"
