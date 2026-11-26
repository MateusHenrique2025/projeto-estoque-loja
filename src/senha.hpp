// senha.hpp - Hash de senha portátil (PBKDF2-HMAC-SHA256), sem depender de bibliotecas do Windows.
//
// É o mesmo algoritmo do sistema anterior (BCryptDeriveKeyPBKDF2, SHA-256, 200000 iterações, sal de 16 bytes),
// então os hashes antigos continuam válidos. Sal e hash são trocados como texto hexadecimal.
#pragma once

#include <string>

namespace senha {

// 16 bytes aleatórios do sistema, em hexadecimal (32 caracteres).
std::string gerarSalHex();

// PBKDF2-HMAC-SHA256 da senha com o sal (hexadecimal). Devolve 32 bytes em hexadecimal (64 caracteres).
std::string derivarHashHex(const std::string& senha, const std::string& salHex);

// Compara dois textos em tempo constante (não vaza, pelo tempo, em que posição diferem).
bool iguais(const std::string& a, const std::string& b);

// Token aleatório (32 bytes em hexadecimal) para a sessão de login.
std::string gerarToken();

}  // namespace senha
