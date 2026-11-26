// produto_repositorio.hpp - Acesso à tabela "produtos". TODO o SQL de produtos fica aqui.
#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "banco.hpp"

// Produto com quantidade menor ou igual a este valor aparece como "estoque baixo".
constexpr int kEstoqueBaixo = 5;

class ProdutoRepositorio {
public:
    explicit ProdutoRepositorio(Conexao& banco) : banco_(banco) {}

    // Lista (por código). "busca" filtra por nome, categoria ou código; "somenteBaixo" mostra só o estoque baixo.
    nlohmann::json listar(const std::string& busca = "", bool somenteBaixo = false);

    // Devolve o produto com o id informado, ou null se não existir.
    nlohmann::json buscar(long long id);

    nlohmann::json criar(const nlohmann::json& dados);

    // Atualiza. Devolve o produto atualizado, ou null se o id não existir.
    nlohmann::json atualizar(long long id, const nlohmann::json& dados);

    // Remove. Devolve true se existia.
    bool remover(long long id);

    // Soma "quantidade" ao estoque. Devolve o produto atualizado, ou null se o id não existir.
    nlohmann::json registrarEntrada(long long id, long long quantidade);

    // Baixa "quantidade" do estoque, de forma atômica (nunca deixa o estoque negativo).
    // Devolve o produto atualizado, ou null se o id não existir. Se não houver estoque suficiente,
    // devolve false em "suficiente" e não altera nada.
    nlohmann::json registrarVenda(long long id, long long quantidade, bool& suficiente);

    // {produtos, unidades, valor_total, estoque_baixo: [...]}
    nlohmann::json relatorio();

private:
    Conexao& banco_;
};
