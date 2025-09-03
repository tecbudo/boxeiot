# Documentação Completa do Sistema BoxeIoT

## Índice
1. [Introdução](#introdução)
2. [Arquitetura do Sistema](#arquitetura-do-sistema)
3. [Estrutura de Dados](#estrutura-de-dados)
4. [Fluxo de Usuário](#fluxo-de-usuário)
5. [Funcionalidades por Página](#funcionalidades-por-página)
6. [Diagramas Técnicos](#diagramas-técnicos)
7. [Configuração e Implantação](#configuração-e-implantação)
8. [Considerações de Segurança](#considerações-de-segurança)

## Introdução

O BoxeIoT é um sistema completo de monitoramento de sensores para equipamentos de boxe, composto por:

1. **Aplicação Web**: Interface para usuários interagirem com os dispositivos
2. **Dispositivos IoT**: Sensores acoplados a sacos de boxe para capturar dados
3. **Backend Firebase**: Processamento e armazenamento em tempo real

### Funcionalidades Principais
- **Cadastro e autenticação** de usuários
- **Gerenciamento** de dispositivos IoT
- **Teste de tempo de reação**
- **Teste de precisão/foco** 
- **Teste de força** de golpes
- **Calibração** de sensores
- **Histórico** e relatórios de desempenho

## Arquitetura do Sistema

```mermaid
graph TB
    subgraph Frontend
        A[Login/Cadastro] --> B[Seleção de Dispositivos]
        B --> C[Modos de Treino]
        C --> D[Tempo de Reação]
        C --> E[Precisão/Foco]
        C --> F[Força]
        C --> G[Calibração]
    end

    subgraph Backend
        H[Firebase Auth] --> I[Autenticação]
        J[Firebase Realtime DB] --> K[Armazenamento]
        L[Firebase Hosting] --> M[Deploy]
    end

    subgraph Dispositivos IoT
        N[Saco de Boxe] --> O[Sensores de Força]
        N --> P[LEDs de Precisão]
        N --> Q[Sensor de Toque/Reação]
        R[Microcontrolador] --> S[Conexão WiFi]
    end

    D --> J
    E --> J
    F --> J
    G --> J
    O --> J
    P --> J
    Q --> J
    I --> A
```

## Estrutura de Dados

### Estrutura no Firebase Realtime Database

```
root/
├── devices/
│   └── [device_id]/
│       ├── estado: "disponivel" | "ocupado" | "manutencao"
│       ├── usuario: [user_id] (se ocupado)
│       ├── timestampConexao: [timestamp]
│       ├── medicoes/
│       │   └── [measurement_id]/
│       │       ├── tipo: "tempo_reacao" | "precisao" | "forca" | "tCalibrar"
│       │       ├── estado: "solicitada" | "executando" | "concluida"
│       │       ├── usuario: [user_id]
│       │       ├── timestamp: [timestamp]
│       │       ├── valor: [number] (resultado)
│       │       └── ledIndex: [number] (apenas para precisão)
│       └── usuarios/
│           └── [user_id]: [timestamp] (timestamp da última conexão)
│
└── users/
    └── [user_id]/
        ├── nome: [string]
        ├── email: [string]
        ├── createdAt: [timestamp]
        └── dispositivos/
            └── [device_id]/
                ├── apelido: [string]
                └── timestamp: [timestamp]
        └── medicoes/
            ├── tempo_reacao/
            │   └── [timestamp_id]/
            │       ├── valor: [number]
            │       ├── timestamp: [timestamp]
            │       └── dispositivo: [device_id]
            ├── forca/
            │   └── [timestamp_id]/
            │       ├── valor: [number]
            │       ├── timestamp: [timestamp]
            │       └── dispositivo: [device_id]
            └── precisao/
                └── [session_id]/
                    ├── acertos: [number]
                    ├── erros: [number]
                    ├── pontuacao: [number]
                    ├── tempoTotal: [number]
                    ├── timestampInicio: [timestamp]
                    ├── timestampFim: [timestamp]
                    └── dispositivo: [device_id]
```

## Fluxo de Usuário

```mermaid
flowchart TD
    A[Acesso Inicial] --> B{Usuário autenticado?}
    B -->|Não| C[Página de Login]
    B -->|Sim| D[Página de Dispositivos]
    
    C --> E{Novo usuário?}
    E -->|Sim| F[Cadastro]
    E -->|Não| G[Login]
    
    F --> H[Cadastro de Dispositivo]
    G --> D
    
    H --> D
    
    D --> I[Selecionar Dispositivo]
    I --> J[Página Principal]
    
    J --> K{Escolher Modo de Treino}
    K --> L[Tempo de Reação]
    K --> M[Precisão/Foco]
    K --> N[Força]
    K --> O[Calibração]
    
    L --> P[Execução] --> Q[Resultados]
    M --> R[Execução] --> S[Resultados]
    N --> T[Execução] --> U[Resultados]
    O --> V[Execução] --> W[Confirmação]
    
    Q --> J
    S --> J
    U --> J
    W --> J
```

## Funcionalidades por Página

### 1. Página de Login (`login.html`)
- Autenticação por email/senha
- Validação de campos e exibição de mensagens de erro
- Redirecionamento automático se já autenticado

### 2. Página de Cadastro (`cadastro.html`)
- Registro de novos usuários
- Validação de senha (mínimo 6 caracteres)
- Armazenamento de dados do usuário no Realtime Database

### 3. Cadastro de Dispositivo (`cadastro-dispositivo.html`)
- Associação de dispositivos ao usuário
- Leitura de QR Code para facilitar o cadastro
- Nome personalizado para dispositivos
- Verificação de existência do dispositivo

### 4. Lista de Dispositivos (`dispositivos.html`)
- Visualização de todos os dispositivos do usuário
- Status em tempo real (disponível/ocupado/manutenção)
- Acesso rápido aos modos de treino
- Logout da aplicação

### 5. Página Principal (`index.html`)
- Dashboard com os quatro modos de treino
- Informações do dispositivo selecionado
- Status de conexão em tempo real
- Gerenciamento de conexão com dispositivos

### 6. Tempo de Reação (`tempodereacao.html`)
- Medição do tempo de resposta do usuário
- Temporizador visual durante a medição
- Histórico de resultados
- Reinício e cancelamento de medições

### 7. Precisão/Foco (`foco.html`)
- Teste de acuidade com LEDs
- Pontuação baseada em acertos/erros
- Treino contínuo até interrupção manual
- Registro de sessões de treino

### 8. Força (`forca.html`)
- Medição da força aplicada no saco de boxe
- Contador regressivo de 30 segundos
- Barra de progresso e valor numérico da força
- Registro da força máxima aplicada

## Diagramas Técnicos

### Diagrama de Sequência - Processo de Medição

```mermaid
sequenceDiagram
    participant U as Usuário
    participant A as Aplicação Web
    participant F as Firebase
    participant D as Dispositivo IoT

    U->>A: Clica em "Iniciar Medição"
    A->>F: Solicita medição (estado: "solicitada")
    F->>D: Notifica dispositivo via DB update
    D->>D: Prepara sensors e inicia medição
    D->>F: Atualiza estado para "executando"
    F->>A: Notifica estado atualizado
    A->>U: Mostra feedback visual
    
    alt Modo Tempo de Reação
        D->>D: Aguarda toque no sensor
        D->>F: Registra valor e estado "concluida"
    else Modo Precisão
        D->>D: Acende LED aleatório
        D->>D: Aguarda toque no sensor correto
        D->>F: Registra resultado (1 ou -1)
        loop Até parada manual
            D->>D: Repete processo automaticamente
        end
    else Modo Força
        D->>D: Inicia contagem regressiva (30s)
        D->>D: Mede força continuamente
        D->>F: Atualiza valor de força em tempo real
        D->>F: Registra força máxima ao final
    end
    
    F->>A: Notifica resultado final
    A->>U: Exibe resultado e atualiza histórico
    A->>F: Salva resultado no histórico do usuário
```

### Diagrama de Estados - Dispositivo IoT

```mermaid
stateDiagram-v2
    [*] --> Disponivel
    Disponivel --> Ocupado: Usuário conecta
    Ocupado --> Disponivel: Usuário desconecta
    
    state Ocupado {
        [*] --> EsperandoComando
        EsperandoComando --> MedicaoSolicitada: Recebe solicitação
        MedicaoSolicitada --> EmExecucao: Inicia medição
        
        state EmExecucao {
            [*] --> ColetandoDados
            ColetandoDados --> Processando: Dados capturados
            Processando --> ResultadoPronto: Processamento completo
        }
        
        ResultadoPronto --> EsperandoComando: Entrega resultado
    }
    
    Ocupado --> Manutencao: Erro detectado
    Manutencao --> Disponivel: Reparo concluído
```

### Diagrama de Componentes

```mermaid
graph TB
    subgraph ClientSide
        A[Browser] --> B[HTML/CSS/JS]
        B --> C[Tailwind CSS]
        B --> D[Chart.js]
        B --> E[jsQR]
    end

    subgraph Firebase
        F[Firebase Auth] --> G[Autenticação]
        H[Firebase Realtime DB] --> I[Armazenamento]
        J[Firebase Hosting] --> K[Deploy]
    end

    subgraph APIs
        L[Webcam API] --> M[Leitura QR Code]
    end

    B --> G
    B --> I
    B --> M
```

## Configuração e Implantação

### Pré-requisitos
1. Conta no Firebase (https://firebase.google.com)
2. Projeto Firebase criado
3. Dispositivos IoT configurados e conectados à mesma base de dados

### Configuração do Firebase
1. Criar projeto no console Firebase
2. Ativar Authentication (com provedor Email)
3. Criar Realtime Database com regras de segurança:

```javascript
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    },
    "devices": {
      "$device_id": {
        ".read": true,
        ".write": "auth != null"
      }
    }
  }
}
```

4. Registrar aplicativo web no projeto e obter configuração
5. Substituir configuração no código de todas as páginas:

```javascript
const firebaseConfig = {
  apiKey: "SUA_API_KEY",
  authDomain: "SEU_PROJETO.firebaseapp.com",
  databaseURL: "https://SEU_PROJETO.firebaseio.com",
  projectId: "SEU_PROJETO",
  storageBucket: "SEU_PROJETO.appspot.com",
  messagingSenderId: "123456789",
  appId: "SEU_APP_ID"
};
```

### Implantação
1. Instalar Firebase CLI: `npm install -g firebase-tools`
2. Fazer login: `firebase login`
3. Inicializar projeto: `firebase init`
   - Selecionar Hosting e Realtime Database
   - Especificar pasta pública (ex: `public`)
   - Configurar como SPA (single page app)
4. Fazer deploy: `firebase deploy`

## Considerações de Segurança

1. **Autenticação**: Todos os usuários devem se autenticar para acessar o sistema
2. **Autorização**: Usuários só acessam seus próprios dados e dispositivos
3. **Validação**: Todas as entradas são validadas no frontend e backend
4. **Conexão Segura**: Firebase usa HTTPS para todas as comunicações
5. **Controle de Acesso**: Dispositivos usam sistema de bloqueio para acesso exclusivo

### Melhorias Futuras
- Implementar sistema de recuperação de senha
- Adicionar mais provedores de autenticação
- Implementar notificações para eventos importantes
- Desenvolver aplicativo mobile nativo

Esta documentação cobre todos os aspectos principais do sistema BoxeIoT conforme implementado atualmente. Para informações mais detalhadas sobre implementação específica, consulte os comentários no código-fonte de cada página.
