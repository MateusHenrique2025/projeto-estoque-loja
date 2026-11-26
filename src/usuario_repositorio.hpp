// usuario_repositorio.hpp - Acesso à tabela "usuarios" (login e troca de senha).
#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "banco.hpp"

class UsuarioRepositorio {
public:
    explicit UsuarioRepositorio(Conexao& banco) : banco_(banco) {}

    // Devolve {id, usuario, sal, hash} ou null se o usuário não existir.
    nlohmann::json buscarPorNome(const std::string& usuario);

    // Grava novo sal e hash. Devolve true se o usuário existe.
    bool atualizarSenha(const std::string& usuario, const std::string& salHex, const std::string& hashHex);

private:
    Conexao& banco_;
};
