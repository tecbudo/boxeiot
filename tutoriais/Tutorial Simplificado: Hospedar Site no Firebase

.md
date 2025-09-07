# Tutorial Simplificado: Hospedar Site no Firebase

Este guia vai te ajudar a publicar seu site de monitoramento de sensores no Firebase.
Você já tem o arquivo index.html pronto - vamos apenas configurar e publicar.
📝 Passo 1: Instalar o Firebase Tools

Abra o terminal no Manjaro Linux e execute:
bash

sudo pacman -S nodejs npm  # Instala Node.js e npm
sudo npm install -g firebase-tools  # Instala o Firebase CLI
firebase login  # Faça login com sua conta Google

🔥 Passo 2: Configurar o Firebase no seu projeto

    Navegue até a pasta do seu projeto:
    bash

cd ~/caminho/para/sua/pasta

Inicie o Firebase:
bash

    firebase init hosting

    Siga estas opções:

        Selecione seu projeto existente (niveltec-2d647).

        Public directory: Digite . (ponto) se o index.html já está na pasta.

        Single-page app? Não.

        GitHub automatic deploys? Não.

🔑 Passo 3: Entendendo o arquivo index.html

Seu arquivo já está configurado para:

    Exibir dados em uma tabela HTML.

    Conectar-se ao Firebase Realtime Database usando estas chaves (já inclusas no código):

javascript

const firebaseConfig = {
  apiKey: "AIzaSyCHp7E1frHdznEcNVAcHpwqho_PWVFdLCA",
  authDomain: "niveltec-2d647.firebaseapp.com",
  databaseURL: "https://niveltec-2d647-default-rtdb.firebaseio.com",
  projectId: "niveltec-2d647",
  storageBucket: "niveltec-2d647.appspot.com",
  messagingSenderId: "235731676914",
  appId: "1:235731676914:web:8f0c5c316065d5e8f56a6b"
};

    Observação: Essas chaves já estão no seu código. NÃO as compartilhe publicamente!

🌐 Passo 4: Publicar o site (Deploy)

Execute no terminal:
bash

firebase deploy --only hosting

No final, aparecerá um link como:
text

✔  Deploy complete!

Project Console: https://console.firebase.google.com/project/niveltec-2d647/overview
Hosting URL: https://niveltec-2d647.web.app

Pronto! Seu site já está no ar e atualizando os dados em tempo real. 🚀
🔧 Problemas comuns?

    Dados não aparecem? Verifique as Regras do Banco de Dados:

        Acesse Firebase Console > Realtime Database > Regras.

        Altere para:
        json

    {
      "rules": {
        ".read": true,
        ".write": true
      }
    }

    (Isso libera acesso público. Para produção, restrinja depois.)

Erro de billing? Você selecionou App Hosting sem querer. Use:
bash

    firebase init hosting  # E selecione apenas "Hosting" tradicional.

🔄 Como atualizar o site depois?

Sempre que modificar o index.html, repita:
bash

firebase deploy --only hosting

Resumo do que você fez:

    Instalou o Firebase Tools.

    Configurou o projeto com firebase init hosting.

    Publicou com firebase deploy.
