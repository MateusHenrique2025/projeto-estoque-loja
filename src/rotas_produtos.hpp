// rotas_produtos.hpp - Rotas HTTP do estoque (todas exigem login).
//
//   GET    /api/produtos                 lista (aceita ?busca=texto e ?situacao=baixo)
//   GET    /api/produtos/:id             busca um
//   POST   /api/produtos                 cadastra
//   PUT    /api/produtos/:id             edita
//   DELETE /api/produtos/:id             exclui
//   POST   /api/produtos/:id/entrada     {quantidade}  entrada de mercadoria
//   POST   /api/produtos/:id/venda       {quantidade}  registra venda (baixa o estoque)
//   GET    /api/relatorio                totais do estoque e lista de estoque baixo
#pragma once

#include <httplib.h>

#include "config.hpp"

void registrarRotasProdutos(httplib::Server& servidor, const Config& config);
