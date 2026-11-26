"use strict";
/*
 * auth.js - Guarda o token de login no navegador e protege as telas.
 * Carregado antes do app.js nas telas do sistema. O login em si é feito em login.html.
 */
const Auth = {
  token() {
    try {
      return sessionStorage.getItem("token") || "";
    } catch (e) {
      return "";
    }
  },
  definir(token) {
    try {
      sessionStorage.setItem("token", token);
    } catch (e) {
      /* sem armazenamento: o login não persiste, mas a tela atual continua funcionando */
    }
  },
  limpar() {
    try {
      sessionStorage.removeItem("token");
    } catch (e) {}
  },
  // Sem token -> volta para o login.
  exigir() {
    if (!Auth.token()) location.replace("login.html");
  },
  // Cabeçalho que vai em toda requisição à API.
  cabecalhos() {
    return { Authorization: "Bearer " + Auth.token() };
  },
  // Sessão expirada (resposta 401): limpa e volta para o login.
  expirou() {
    Auth.limpar();
    location.replace("login.html");
  },
  // Monta o menu de navegação (links do CONFIG.menu) nas telas que não usam o app.js.
  montarMenu(nav) {
    const atual = location.pathname.split("/").pop() || "index.html";
    for (const item of CONFIG.menu || []) {
      const link = document.createElement("a");
      link.href = item.href;
      link.textContent = item.texto;
      if (item.href === atual) link.setAttribute("aria-current", "page");
      nav.appendChild(link);
    }
    nav.hidden = false;
  },

  // fetch com o token; devolve o JSON (ou null no 204) e lança Error com a mensagem da API.
  async chamar(metodo, url, corpo) {
    const opcoes = { method: metodo, headers: Auth.cabecalhos() };
    if (corpo !== undefined) {
      opcoes.headers["Content-Type"] = "application/json";
      opcoes.body = JSON.stringify(corpo);
    }
    let resposta;
    try {
      resposta = await fetch(url, opcoes);
    } catch (e) {
      throw new Error("Não foi possível falar com o servidor. Ele está rodando?");
    }
    if (resposta.status === 401) {
      Auth.expirou();
      throw new Error("Sessão expirada. Faça login novamente.");
    }
    if (resposta.status === 204) return null;
    const dados = await resposta.json().catch(() => null);
    if (!resposta.ok) {
      let mensagem = (dados && dados.erro) || "Erro " + resposta.status;
      if (dados && Array.isArray(dados.detalhes)) mensagem += ":\n- " + dados.detalhes.join("\n- ");
      throw new Error(mensagem);
    }
    return dados;
  },
};
