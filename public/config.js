"use strict";
/*
 * config.js - A ÚNICA parte do front-end que a equipe precisa editar para mudar de tema.
 * O app.js lê o CONFIG abaixo e monta a tela (tabela, formulário, busca, botões) sozinho.
 *
 * Tipos de campo aceitos em "tipo":
 *   "texto"         linha de texto            (opções: max = tamanho máximo)
 *   "textarea"      texto longo               (opções: max = tamanho máximo)
 *   "numero"        número inteiro            (opções: min, max)
 *   "decimal"       número com casas decimais (opções: min, max, passo, formato: "moeda")
 *   "data"          data (AAAA-MM-DD)
 *   "select"        lista de opções           (opções: opcoes: [{valor, rotulo}]
 *                                              ou opcoesDe: {recurso, valor, rotulo} para buscar na API)
 *   "somenteLeitura" aparece na tabela, mas não no formulário (ex.: data de criação)
 * Opções comuns: obrigatorio (true/false), padrao (valor inicial), largo (ocupa a linha toda),
 *   rotuloTabela (título mais curto para a coluna da tabela, quando o rótulo do formulário é longo).
 *
 * Opções do CONFIG:
 *   semEditar: true / semExcluir: true   escondem os botões da tabela quando a API ainda não tem PUT / DELETE.
 *   classeLinha: (registro) => "linha-alerta"   devolve uma classe CSS para destacar a linha
 *                                               (prontas: "linha-alerta" e "linha-apagada"; veja o estilo.css).
 *   acoesExtras: [{ rotulo, caminho: (registro) => "/api/.../acao", metodo: "POST", mensagem, mostrarSe: (registro) => bool }]
 *                                               botões extras na linha que chamam uma rota da API.
 *                                               Com perguntar: "texto", pede um número ao usuário e envia {quantidade: N};
 *                                               "mensagem" pode ser um texto ou uma função (resposta) => texto.
 *   filtros: [{ nome: "status", rotulo: "Status", opcoes: [{valor, rotulo}] }  (ou opcoesDe: {recurso, valor, rotulo})]
 *                                               listas de filtro ao lado da busca; cada uma envia ?nome=valor à API.
 * No campo "decimal" use casas: N para escolher quantas casas decimais aparecem na tabela (padrão 2).
 *
 * Várias telas: copie index.html para outro arquivo (ex.: autores.html), troque nele o
 *   <script src="config.js"> por um novo arquivo de configuração (ex.: config-autores.js)
 * e use "menu" para ligar as páginas entre si.
 */

const CONFIG = {
  titulo: "Sistema de Estoque",
  recurso: "produtos", // usa as rotas /api/produtos
  campos: [
    { nome: "codigo", rotulo: "Código", tipo: "numero", obrigatorio: true, min: 1 },
    { nome: "nome", rotulo: "Nome", tipo: "texto", obrigatorio: true, max: 100 },
    { nome: "categoria", rotulo: "Categoria", tipo: "texto", obrigatorio: true, max: 50 },
    { nome: "preco", rotulo: "Preço (R$)", tipo: "decimal", formato: "moeda", obrigatorio: true, min: 0 },
    { nome: "quantidade", rotulo: "Quantidade", tipo: "numero", min: 0, padrao: 0 },
    { nome: "criado_em", rotulo: "Cadastrado em", tipo: "somenteLeitura" },
  ],
  // Estoque baixo (5 unidades ou menos), como no relatório do sistema anterior.
  classeLinha: (produto) => (produto.quantidade <= 5 ? "linha-alerta" : ""),
  filtros: [{ nome: "situacao", rotulo: "Estoque", opcoes: [{ valor: "baixo", rotulo: "Estoque baixo (até 5)" }] }],
  // Botões extras em cada linha. "perguntar" abre uma caixa pedindo a quantidade e envia {quantidade: N}.
  acoesExtras: [
    {
      rotulo: "Entrada",
      caminho: (produto) => "/api/produtos/" + produto.id + "/entrada",
      perguntar: "Quantidade de entrada:",
      mensagem: (r) => "Entrada registrada. Estoque atual: " + r.quantidade,
    },
    {
      rotulo: "Venda",
      caminho: (produto) => "/api/produtos/" + produto.id + "/venda",
      perguntar: "Quantidade vendida:",
      mostrarSe: (produto) => produto.quantidade > 0,
      mensagem: (r) =>
        "Venda realizada: " + r.quantidade_vendida + " x " + r.produto + " = " +
        Number(r.total).toLocaleString("pt-BR", { style: "currency", currency: "BRL" }),
    },
  ],
  menu: [
    { texto: "Produtos", href: "index.html" },
    { texto: "Relatório", href: "relatorio.html" },
    { texto: "Trocar senha", href: "senha.html" },
    { texto: "Sair", href: "login.html?sair=1" },
  ],
};
