#include "rotas_auth.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "http_util.hpp"
#include "senha.hpp"
#include "usuario_repositorio.hpp"

using nlohmann::json;
using Relogio = std::chrono::steady_clock;

namespace {

constexpr auto kValidadeSessao = std::chrono::hours(8);
constexpr int kMaxFalhas = 5;                              // falhas seguidas antes de bloquear o usuário
constexpr auto kTempoBloqueio = std::chrono::seconds(60);  // por quanto tempo
constexpr size_t kTamanhoMinimoSenha = 8;

struct Sessao {
    std::string usuario;
    Relogio::time_point expira;
};
struct Tentativas {
    int falhas{0};
    Relogio::time_point bloqueadoAte{};
};

std::mutex g_trava;
std::map<std::string, Sessao> g_sessoes;
std::map<std::string, Tentativas> g_tentativas;

std::string tokenDaRequisicao(const httplib::Request& req) {
    const std::string cabecalho = req.get_header_value("Authorization");
    const std::string prefixo = "Bearer ";
    return cabecalho.rfind(prefixo, 0) == 0 ? cabecalho.substr(prefixo.size()) : "";
}

// Devolve o nome do usuário dono do token, ou "" se o token não existe / expirou.
std::string usuarioDoToken(const std::string& token) {
    if (token.empty()) return "";
    std::lock_guard<std::mutex> guarda(g_trava);
    auto it = g_sessoes.find(token);
    if (it == g_sessoes.end()) return "";
    if (Relogio::now() > it->second.expira) {
        g_sessoes.erase(it);
        return "";
    }
    return it->second.usuario;
}

std::string criarSessao(const std::string& usuario) {
    std::string token = senha::gerarToken();
    std::lock_guard<std::mutex> guarda(g_trava);
    const auto agora = Relogio::now();
    for (auto it = g_sessoes.begin(); it != g_sessoes.end();) {  // limpa as vencidas
        it = (agora > it->second.expira) ? g_sessoes.erase(it) : std::next(it);
    }
    g_sessoes[token] = {usuario, agora + kValidadeSessao};
    return token;
}

}  // namespace

void registrarRotasAuth(httplib::Server& servidor, const Config& config) {
    // Antes de qualquer rota: /api/* exige token, menos o login e as rotas de saúde. Arquivos do front-end são livres
    // (o login.html precisa carregar); os DADOS é que ficam protegidos.
    servidor.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        const bool api = req.path.rfind("/api/", 0) == 0;
        const bool livre = req.path == "/api/login" || req.path == "/api/saude" || req.path == "/api/saude/banco";
        if (api && !livre && usuarioDoToken(tokenDaRequisicao(req)).empty()) {
            util::enviarErro(res, 401, "Sessao invalida ou expirada. Faca login novamente");
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // LOGIN
    servidor.Post("/api/login", [&config](const httplib::Request& req, httplib::Response& res) {
        json dados;
        if (!util::lerCorpoObjeto(req, res, dados)) {
            return;
        }
        if (!dados.contains("usuario") || !dados["usuario"].is_string() || !dados.contains("senha") ||
            !dados["senha"].is_string() || dados["usuario"].get<std::string>().empty() ||
            dados["senha"].get<std::string>().empty()) {
            util::enviarErro(res, 422, "Dados invalidos", json::array({"usuario e senha sao obrigatorios"}));
            return;
        }
        const std::string usuario = dados["usuario"].get<std::string>();
        const std::string digitada = dados["senha"].get<std::string>();

        {
            std::lock_guard<std::mutex> guarda(g_trava);
            auto it = g_tentativas.find(usuario);
            if (it != g_tentativas.end() && Relogio::now() < it->second.bloqueadoAte) {
                util::enviarErro(res, 429, "Muitas tentativas. Aguarde 1 minuto e tente de novo");
                return;
            }
        }

        util::comBanco(config, res, [&](Conexao& banco) {
            json registro = UsuarioRepositorio(banco).buscarPorNome(usuario);
            // Mesmo com usuário inexistente calculamos um hash, para não revelar (pelo tempo) quais usuários existem.
            const std::string sal = registro.is_null() ? senha::gerarSalHex() : registro["sal"].get<std::string>();
            const std::string calculado = senha::derivarHashHex(digitada, sal);
            const bool ok = !registro.is_null() && senha::iguais(calculado, registro["hash"].get<std::string>());

            if (!ok) {
                std::lock_guard<std::mutex> guarda(g_trava);
                Tentativas& t = g_tentativas[usuario];
                if (++t.falhas >= kMaxFalhas) {
                    t.falhas = 0;
                    t.bloqueadoAte = Relogio::now() + kTempoBloqueio;
                }
                util::enviarErro(res, 401, "Usuario ou senha incorretos");
                return;
            }
            {
                std::lock_guard<std::mutex> guarda(g_trava);
                g_tentativas.erase(usuario);
            }
            util::enviarJson(res, {{"token", criarSessao(usuario)}, {"usuario", usuario}});
        });
    });

    // LOGOUT
    servidor.Post("/api/logout", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> guarda(g_trava);
        g_sessoes.erase(tokenDaRequisicao(req));
        res.status = 204;
    });

    // TROCAR SENHA
    servidor.Post("/api/trocar-senha", [&config](const httplib::Request& req, httplib::Response& res) {
        const std::string usuario = usuarioDoToken(tokenDaRequisicao(req));
        json dados;
        if (!util::lerCorpoObjeto(req, res, dados)) {
            return;
        }
        json erros = json::array();
        if (!dados.contains("senhaAtual") || !dados["senhaAtual"].is_string() ||
            dados["senhaAtual"].get<std::string>().empty()) {
            erros.push_back("senhaAtual e obrigatoria");
        }
        if (!dados.contains("novaSenha") || !dados["novaSenha"].is_string() ||
            dados["novaSenha"].get<std::string>().size() < kTamanhoMinimoSenha) {
            erros.push_back("novaSenha deve ter pelo menos 8 caracteres");
        }
        if (!erros.empty()) {
            util::enviarErro(res, 422, "Dados invalidos", erros);
            return;
        }
        const std::string atual = dados["senhaAtual"].get<std::string>();
        const std::string nova = dados["novaSenha"].get<std::string>();

        util::comBanco(config, res, [&](Conexao& banco) {
            UsuarioRepositorio repositorio(banco);
            json registro = repositorio.buscarPorNome(usuario);
            if (registro.is_null() ||
                !senha::iguais(senha::derivarHashHex(atual, registro["sal"].get<std::string>()),
                               registro["hash"].get<std::string>())) {
                util::enviarErro(res, 403, "Senha atual incorreta");
                return;
            }
            const std::string novoSal = senha::gerarSalHex();
            repositorio.atualizarSenha(usuario, novoSal, senha::derivarHashHex(nova, novoSal));
            util::enviarJson(res, {{"mensagem", "Senha alterada com sucesso"}});
        });
    });
}
