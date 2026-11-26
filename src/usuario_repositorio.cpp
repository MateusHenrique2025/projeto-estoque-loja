#include "usuario_repositorio.hpp"

using nlohmann::json;

json UsuarioRepositorio::buscarPorNome(const std::string& usuario) {
    json linhas = banco_.consultar("SELECT id, usuario, sal, hash FROM usuarios WHERE usuario = ?", {usuario});
    return linhas.empty() ? json(nullptr) : linhas.front();
}

bool UsuarioRepositorio::atualizarSenha(const std::string& usuario, const std::string& salHex,
                                        const std::string& hashHex) {
    if (buscarPorNome(usuario).is_null()) {
        return false;
    }
    banco_.executar("UPDATE usuarios SET sal = ?, hash = ? WHERE usuario = ?", {salHex, hashHex, usuario});
    return true;
}
