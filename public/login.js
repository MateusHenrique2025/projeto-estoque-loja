"use strict";
/* login.js - Envia usuário e senha para POST /api/login e guarda o token. */

const formulario = document.getElementById("formulario-login");
const mensagem = document.getElementById("mensagem");
const botao = formulario.querySelector("button");

function mostrar(texto, ok) {
  mensagem.style.color = ok ? "#39ff14" : "#ff6b6b";
  mensagem.textContent = texto;
}

// login.html?sair=1  ->  encerra a sessão (o menu "Sair" aponta para cá).
if (new URLSearchParams(location.search).has("sair")) {
  const token = Auth.token();
  Auth.limpar();
  if (token) {
    fetch("/api/logout", { method: "POST", headers: { Authorization: "Bearer " + token } }).catch(() => {});
  }
  mostrar("Você saiu do sistema.", true);
} else if (Auth.token()) {
  location.replace("index.html"); // já está logado
}

formulario.addEventListener("submit", async (evento) => {
  evento.preventDefault();

  const usuario = document.getElementById("usuario").value.trim();
  const senha = document.getElementById("senha").value;
  if (!usuario || !senha) {
    mostrar("Preencha o login e a senha.", false);
    return;
  }

  botao.disabled = true;
  try {
    const resposta = await fetch("/api/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ usuario, senha }),
    });
    const dados = await resposta.json().catch(() => ({}));
    if (!resposta.ok) {
      mostrar(dados.erro || "Erro " + resposta.status, false);
      return;
    }
    Auth.definir(dados.token);
    mostrar("Login realizado com sucesso.", true);
    location.replace("index.html");
  } catch (e) {
    mostrar("Não foi possível falar com o servidor. Ele está rodando?", false);
  } finally {
    botao.disabled = false;
  }
});
