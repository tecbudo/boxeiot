// Configuração do Firebase (usando a versão de compatibilidade)
const firebaseConfig = {
  apiKey: "AIzaSyBLsU5CMvKQOm9Qs9mu6X1K27fVI8WuBQg",
  authDomain: "boxeiot.firebaseapp.com",
  databaseURL: "https://boxeiot-default-rtdb.firebaseio.com", // Adicione esta linha
  projectId: "boxeiot",
  storageBucket: "boxeiot.firebasestorage.app",
  messagingSenderId: "510231005229",
  appId: "1:510231005229:web:13e764df17a953ff9d50a4",
  measurementId: "G-0LP6HCJKGR"
};

// Inicializa o Firebase com a versão de compatibilidade
const app = firebase.initializeApp(firebaseConfig);
const auth = firebase.auth();
const database = firebase.database();
