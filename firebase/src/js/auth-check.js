// auth-check.js - Versão Corrigida
auth.onAuthStateChanged((user) => {
    console.log("Estado de autenticação alterado. Usuário:", user ? user.uid : "não logado");
    
    // Páginas que não requerem autenticação
    const publicPages = ['login.html', 'cadastro.html'];
    const currentPage = window.location.pathname.split('/').pop();
    
    // Se não está logado e a página atual não é pública
    if (!user && !publicPages.includes(currentPage)) {
        console.log("Usuário não autenticado tentando acessar página protegida. Redirecionando para login.");
        window.location.href = 'login.html';
        return;
    }
    
    // Se está logado e está em página pública
    if (user && publicPages.includes(currentPage)) {
        console.log("Usuário autenticado em página pública. Verificando dispositivos.");
        
        // Verificar se tem dispositivos para decidir para onde redirecionar
        firebase.database().ref('users/' + user.uid + '/dispositivos').once('value')
            .then(snapshot => {
                if (snapshot.exists() && Object.keys(snapshot.val()).length > 0) {
                    console.log("Usuário tem dispositivos. Redirecionando para index.");
                    window.location.href = "index.html";
                } else {
                    console.log("Usuário não tem dispositivos. Redirecionando para cadastro-dispositivo.");
                    window.location.href = "cadastro-dispositivo.html";
                }
            })
            .catch(error => {
                console.error("Erro ao verificar dispositivos:", error);
                window.location.href = "cadastro-dispositivo.html";
            });
    }
});
