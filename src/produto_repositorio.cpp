#include "produto_repositorio.hpp"

using nlohmann::json;

namespace {

std::string comoTexto(const json& valor) {
    return valor.is_string() ? valor.get<std::string>() : valor.dump();
}

// Escapa % e _ para que a busca trate o texto digitado literalmente dentro do LIKE.
std::string escaparLike(const std::string& texto) {
    std::string saida;
    for (char c : texto) {
        if (c == '\\' || c == '%' || c == '_') {
            saida += '\\';
        }
        saida += c;
    }
    return saida;
}

const char* const kColunas = "id, codigo, nome, categoria, preco, quantidade, criado_em";

}  // namespace

json ProdutoRepositorio::listar(const std::string& busca, bool somenteBaixo) {
    std::string sql = std::string("SELECT ") + kColunas + " FROM produtos WHERE 1 = 1";
    Conexao::Parametros parametros;
    if (!busca.empty()) {
        const std::string padrao = "%" + escaparLike(busca) + "%";
        sql += " AND (nome LIKE ? OR categoria LIKE ? OR CAST(codigo AS CHAR) LIKE ?)";
        parametros = {padrao, padrao, padrao};
    }
    if (somenteBaixo) {
        sql += " AND quantidade <= ?";
        parametros.push_back(std::to_string(kEstoqueBaixo));
    }
    sql += " ORDER BY codigo";
    return banco_.consultar(sql, parametros);
}

json ProdutoRepositorio::buscar(long long id) {
    json linhas = banco_.consultar(std::string("SELECT ") + kColunas + " FROM produtos WHERE id = ?",
                                   {std::to_string(id)});
    return linhas.empty() ? json(nullptr) : linhas.front();
}

json ProdutoRepositorio::criar(const json& dados) {
    banco_.executar("INSERT INTO produtos (codigo, nome, categoria, preco, quantidade) VALUES (?, ?, ?, ?, ?)",
                    {comoTexto(dados.at("codigo")), comoTexto(dados.at("nome")), comoTexto(dados.at("categoria")),
                     comoTexto(dados.at("preco")),
                     dados.contains("quantidade") && !dados["quantidade"].is_null() ? comoTexto(dados["quantidade"])
                                                                                    : "0"});
    return buscar(static_cast<long long>(banco_.ultimoId()));
}

json ProdutoRepositorio::atualizar(long long id, const json& dados) {
    // Conferimos antes: um UPDATE que não muda nada devolve 0 linhas afetadas no MariaDB.
    if (buscar(id).is_null()) {
        return json(nullptr);
    }
    banco_.executar(
        "UPDATE produtos SET codigo = ?, nome = ?, categoria = ?, preco = ?, quantidade = ? WHERE id = ?",
        {comoTexto(dados.at("codigo")), comoTexto(dados.at("nome")), comoTexto(dados.at("categoria")),
         comoTexto(dados.at("preco")),
         dados.contains("quantidade") && !dados["quantidade"].is_null() ? comoTexto(dados["quantidade"]) : "0",
         std::to_string(id)});
    return buscar(id);
}

bool ProdutoRepositorio::remover(long long id) {
    return banco_.executar("DELETE FROM produtos WHERE id = ?", {std::to_string(id)}) > 0;
}

json ProdutoRepositorio::registrarEntrada(long long id, long long quantidade) {
    banco_.executar("UPDATE produtos SET quantidade = quantidade + ? WHERE id = ?",
                    {std::to_string(quantidade), std::to_string(id)});
    return buscar(id);
}

json ProdutoRepositorio::registrarVenda(long long id, long long quantidade, bool& suficiente) {
    // Uma única instrução: só baixa se ainda houver estoque (evita vender o mesmo item duas vezes ao mesmo tempo).
    const auto afetadas = banco_.executar(
        "UPDATE produtos SET quantidade = quantidade - ? WHERE id = ? AND quantidade >= ?",
        {std::to_string(quantidade), std::to_string(id), std::to_string(quantidade)});
    suficiente = afetadas > 0;
    return buscar(id);  // null se o id não existe; se existe mas faltou estoque, vem o produto sem alteração
}

json ProdutoRepositorio::relatorio() {
    json totais = banco_.consultar(
        "SELECT COUNT(*) AS produtos, COALESCE(SUM(quantidade), 0) AS unidades, "
        "COALESCE(SUM(quantidade * preco), 0) AS valor_total FROM produtos");
    json resultado = totais.front();
    resultado["estoque_baixo"] = listar("", true);
    resultado["limite_estoque_baixo"] = kEstoqueBaixo;
    return resultado;
}
