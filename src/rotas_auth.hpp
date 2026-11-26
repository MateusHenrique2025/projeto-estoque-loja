// rotas_auth.hpp - Login, logout e troca de senha. Também protege TODAS as rotas /api/ (exceto login e saúde).
//
//   POST /api/login           {usuario, senha}            -> {token, usuario}
//   POST /api/logout          (com token)                 -> 204
//   POST /api/trocar-senha    {senhaAtual, novaSenha}     -> {mensagem}   (com token)
//
// O token vai no cabeçalho de cada requisição:  Authorization: Bearer <token>
// As sessões ficam só na memória do servidor: reiniciar o servidor exige novo login.
#pragma once

#include <httplib.h>

#include "config.hpp"

void registrarRotasAuth(httplib::Server& servidor, const Config& config);
