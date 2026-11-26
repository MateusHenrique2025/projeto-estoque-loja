#include "rotas_produtos.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include "http_util.hpp"
#include "produto_repositorio.hpp"

using nlohmann::json;

namespace {

bool inteiroEntre(const json& v, long long min, long long max) {
    return v.is_number_integer() && v.get<long long>() >= min && v.get<long long>() <= max;
}

bool textoComTamanho(const json& dados, const char* campo, size_t max) {
    return dados.contains(campo) && dados[campo].is_string() && !dados[campo].get<std::string>().empty() &&
           dados[campo].get<std::string>().size() <= max;
}

// Confere os campos recebidos. Devolve uma lista de mensagens; vazia = tudo certo.
json validarProduto(const json& dados) {
    json erros = json::array();

    if (!dados.contains("codigo") || !inteiroEntre(dados["codigo"], 1, 4294967295LL)) {
        erros.push_back("codigo e obrigatorio e deve ser um numero inteiro positivo");
    }
    if (!textoComTamanho(dados, "nome", 100)) {
        erros.push_back("nome e obrigatorio (ate 100 caracteres)");
    }
    if (!textoComTamanho(dados, "categoria", 50)) {
        erros.push_back("categoria e obrigatoria (ate 50 caracteres)");
    }
    if (!dados.contains("preco") || !dados["preco"].is_number() || dados["preco"].get<double>() < 0 ||
        dados["preco"].get<double>() > 99999999.99) {
        erros.push_back("preco e obrigatorio e deve ser um numero maior ou igual a zero");
    }
    if (dados.contains("quantidade") && !dados["quantidade"].is_null() &&
        !inteiroEntre(dados["quantidade"], 0, 2000000000LL)) {
        erros.push_back("quantidade deve ser um numero inteiro maior ou igual a zero");
    }
    return erros;
}

// Lê {"quantidade": N} (N inteiro > 0) das rotas de entrada e venda. Em caso de erro já responde 422.
bool lerQuantidade(const httplib::Request& req, httplib::Response& res, long long& quantidade) {
    json dados;
    if (!util::lerCorpoObjeto(req, res, dados)) {
        return false;
    }
    if (!dados.contains("quantidade") || !inteiroEntre(dados["quantidade"], 1, 2000000000LL)) {
        util::enviarErro(res, 422, "Dados invalidos",
                         json::array({"quantidade deve ser um numero inteiro maior que zero"}));
        return false;
    }
    quantidade = dados["quantidade"].get<long long>();
    return true;
}

}  // namespace

void registrarRotasProdutos(httplib::Server& servidor, const Config& config) {
    // LISTAR (?busca=texto  e/ou  ?situacao=baixo)
    servidor.Get("/api/produtos", [&config](const httplib::Request& req, httplib::Response& res) {
        util::comBanco(config, res, [&](Conexao& banco) {
            ProdutoRepositorio repositorio(banco);
            const std::string busca = req.has_param("busca") ? req.get_param_value("busca") : "";
            const bool baixo = req.has_param("situacao") && req.get_param_value("situacao") == "baixo";
            util::enviarJson(res, repositorio.listar(busca, baixo));
        });
    });

    // RELATÓRIO (declarada antes das rotas com :id, só por clareza)
    servidor.Get("/api/relatorio", [&config](const httplib::Request&, httplib::Response& res) {
        util::comBanco(config, res, [&](Conexao& banco) {
            util::enviarJson(res, ProdutoRepositorio(banco).relatorio());
        });
    });

    // BUSCAR UM
    servidor.Get("/api/produtos/:id", [&config](const httplib::Request& req, httplib::Response& res) {
        long long id = 0;
        if (!util::lerId(req, res, id)) {
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            json produto = ProdutoRepositorio(banco).buscar(id);
            if (produto.is_null()) {
                util::enviarErro(res, 404, "Produto nao encontrado");
                return;
            }
            util::enviarJson(res, produto);
        });
    });

    // CADASTRAR (codigo repetido -> 409)
    servidor.Post("/api/produtos", [&config](const httplib::Request& req, httplib::Response& res) {
        json dados;
        if (!util::lerCorpoObjeto(req, res, dados)) {
            return;
        }
        json erros = validarProduto(dados);
        if (!erros.empty()) {
            util::enviarErro(res, 422, "Dados invalidos", erros);
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            util::enviarJson(res, ProdutoRepositorio(banco).criar(dados), 201);
        });
    });

    // EDITAR (substitui todos os campos)
    servidor.Put("/api/produtos/:id", [&config](const httplib::Request& req, httplib::Response& res) {
        long long id = 0;
        if (!util::lerId(req, res, id)) {
            return;
        }
        json dados;
        if (!util::lerCorpoObjeto(req, res, dados)) {
            return;
        }
        json erros = validarProduto(dados);
        if (!erros.empty()) {
            util::enviarErro(res, 422, "Dados invalidos", erros);
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            json atualizado = ProdutoRepositorio(banco).atualizar(id, dados);
            if (atualizado.is_null()) {
                util::enviarErro(res, 404, "Produto nao encontrado");
                return;
            }
            util::enviarJson(res, atualizado);
        });
    });

    // EXCLUIR
    servidor.Delete("/api/produtos/:id", [&config](const httplib::Request& req, httplib::Response& res) {
        long long id = 0;
        if (!util::lerId(req, res, id)) {
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            if (!ProdutoRepositorio(banco).remover(id)) {
                util::enviarErro(res, 404, "Produto nao encontrado");
                return;
            }
            res.status = 204;  // sem conteudo
        });
    });

    // ENTRADA DE MERCADORIA
    servidor.Post("/api/produtos/:id/entrada", [&config](const httplib::Request& req, httplib::Response& res) {
        long long id = 0;
        long long quantidade = 0;
        if (!util::lerId(req, res, id) || !lerQuantidade(req, res, quantidade)) {
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            json produto = ProdutoRepositorio(banco).registrarEntrada(id, quantidade);
            if (produto.is_null()) {
                util::enviarErro(res, 404, "Produto nao encontrado");
                return;
            }
            util::enviarJson(res, produto);
        });
    });

    // VENDA (baixa o estoque; sem estoque suficiente -> 422)
    servidor.Post("/api/produtos/:id/venda", [&config](const httplib::Request& req, httplib::Response& res) {
        long long id = 0;
        long long quantidade = 0;
        if (!util::lerId(req, res, id) || !lerQuantidade(req, res, quantidade)) {
            return;
        }
        util::comBanco(config, res, [&](Conexao& banco) {
            bool suficiente = false;
            json produto = ProdutoRepositorio(banco).registrarVenda(id, quantidade, suficiente);
            if (produto.is_null()) {
                util::enviarErro(res, 404, "Produto nao encontrado");
                return;
            }
            if (!suficiente) {
                util::enviarErro(res, 422, "Estoque insuficiente",
                                 json::array({"disponivel: " + std::to_string(produto["quantidade"].get<long long>())}));
                return;
            }
            const double total = static_cast<double>(quantidade) * produto["preco"].get<double>();
            util::enviarJson(res, {{"produto", produto["nome"]},
                                   {"quantidade_vendida", quantidade},
                                   {"total", total},
                                   {"estoque_restante", produto["quantidade"]}});
        });
    });
}
