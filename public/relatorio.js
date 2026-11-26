"use strict";
/* relatorio.js - Mostra GET /api/relatorio (totais e produtos com estoque baixo). */

Auth.exigir();
Auth.montarMenu(document.getElementById("menu"));

const moeda = (n) => Number(n).toLocaleString("pt-BR", { style: "currency", currency: "BRL" });

async function carregarRelatorio() {
  const aviso = document.getElementById("aviso");
  try {
    const r = await Auth.chamar("GET", "/api/relatorio");
    document.getElementById("r-produtos").textContent = r.produtos;
    document.getElementById("r-unidades").textContent = r.unidades;
    document.getElementById("r-valor").textContent = moeda(r.valor_total);
    document.getElementById("titulo-baixo").textContent =
      "Produtos com estoque baixo (" + r.limite_estoque_baixo + " unidades ou menos)";

    const corpo = document.getElementById("baixo");
    corpo.textContent = "";
    document.getElementById("vazio").hidden = r.estoque_baixo.length > 0;
    for (const p of r.estoque_baixo) {
      const linha = document.createElement("tr");
      linha.className = "linha-alerta";
      for (const valor of [p.codigo, p.nome, p.categoria, p.quantidade]) {
        const celula = document.createElement("td");
        celula.textContent = valor; // textContent evita XSS
        linha.appendChild(celula);
      }
      corpo.appendChild(linha);
    }
  } catch (erro) {
    aviso.textContent = erro.message;
    aviso.className = "aviso erro";
    aviso.hidden = false;
  }
}

carregarRelatorio();
