"use strict";
/* senha.js - Troca de senha (POST /api/trocar-senha). */

Auth.exigir();
Auth.montarMenu(document.getElementById("menu"));

const formulario = document.getElementById("formulario");
const aviso = document.getElementById("aviso");

function mostrar(texto, tipo) {
  aviso.textContent = texto;
  aviso.className = "aviso " + tipo;
  aviso.hidden = false;
}

formulario.addEventListener("submit", async (evento) => {
  evento.preventDefault();
  const atual = document.getElementById("atual").value;
  const nova = document.getElementById("nova").value;
  const confirmacao = document.getElementById("confirmacao").value;

  if (nova.length < 8) return mostrar("A senha precisa ter pelo menos 8 caracteres.", "erro");
  if (nova !== confirmacao) return mostrar("As senhas não conferem.", "erro");

  const botao = formulario.querySelector("button");
  botao.disabled = true;
  try {
    await Auth.chamar("POST", "/api/trocar-senha", { senhaAtual: atual, novaSenha: nova });
    formulario.reset();
    mostrar("Senha alterada com sucesso.", "ok");
  } catch (erro) {
    mostrar(erro.message, "erro");
  } finally {
    botao.disabled = false;
  }
});
