#include "senha.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr unsigned kIteracoes = 200000;

// ----- SHA-256 (FIPS 180-4) -----
class Sha256 {
public:
    Sha256() { reiniciar(); }

    void reiniciar() {
        estado_ = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        tamanho_ = 0;
        usado_ = 0;
    }

    void atualizar(const uint8_t* dados, size_t n) {
        tamanho_ += n;
        while (n > 0) {
            size_t copiar = std::min(n, bloco_.size() - usado_);
            std::memcpy(bloco_.data() + usado_, dados, copiar);
            usado_ += copiar;
            dados += copiar;
            n -= copiar;
            if (usado_ == bloco_.size()) {
                processar();
                usado_ = 0;
            }
        }
    }

    std::array<uint8_t, 32> finalizar() {
        const uint64_t bits = tamanho_ * 8;
        const uint8_t um = 0x80;
        atualizar(&um, 1);
        const uint8_t zero = 0;
        while (usado_ != 56) {
            atualizar(&zero, 1);
        }
        uint8_t fim[8];
        for (int i = 0; i < 8; ++i) {
            fim[i] = static_cast<uint8_t>(bits >> (56 - 8 * i));
        }
        atualizar(fim, 8);
        std::array<uint8_t, 32> saida{};
        for (int i = 0; i < 8; ++i) {
            saida[i * 4] = static_cast<uint8_t>(estado_[i] >> 24);
            saida[i * 4 + 1] = static_cast<uint8_t>(estado_[i] >> 16);
            saida[i * 4 + 2] = static_cast<uint8_t>(estado_[i] >> 8);
            saida[i * 4 + 3] = static_cast<uint8_t>(estado_[i]);
        }
        return saida;
    }

private:
    static uint32_t rotd(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void processar() {
        static const uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t(bloco_[i * 4]) << 24) | (uint32_t(bloco_[i * 4 + 1]) << 16) |
                   (uint32_t(bloco_[i * 4 + 2]) << 8) | uint32_t(bloco_[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotd(w[i - 15], 7) ^ rotd(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotd(w[i - 2], 17) ^ rotd(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = estado_[0], b = estado_[1], c = estado_[2], d = estado_[3];
        uint32_t e = estado_[4], f = estado_[5], g = estado_[6], h = estado_[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotd(e, 6) ^ rotd(e, 11) ^ rotd(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = h + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotd(a, 2) ^ rotd(a, 13) ^ rotd(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;
            h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        estado_[0] += a; estado_[1] += b; estado_[2] += c; estado_[3] += d;
        estado_[4] += e; estado_[5] += f; estado_[6] += g; estado_[7] += h;
    }

    std::array<uint32_t, 8> estado_{};
    std::array<uint8_t, 64> bloco_{};
    size_t usado_{0};
    uint64_t tamanho_{0};
};

std::string paraHex(const uint8_t* bytes, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        s += d[bytes[i] >> 4];
        s += d[bytes[i] & 15];
    }
    return s;
}

int valorHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::vector<uint8_t> deHex(const std::string& texto) {
    if (texto.size() % 2 != 0) throw std::invalid_argument("hex invalido");
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < texto.size(); i += 2) {
        int a = valorHex(texto[i]), b = valorHex(texto[i + 1]);
        if (a < 0 || b < 0) throw std::invalid_argument("hex invalido");
        bytes.push_back(static_cast<uint8_t>((a << 4) | b));
    }
    return bytes;
}

std::vector<uint8_t> aleatorios(size_t n) {
    std::random_device origem;
    std::vector<uint8_t> bytes(n);
    for (auto& b : bytes) b = static_cast<uint8_t>(origem());
    return bytes;
}

}  // namespace

namespace senha {

std::string gerarSalHex() {
    auto b = aleatorios(16);
    return paraHex(b.data(), b.size());
}

std::string gerarToken() {
    auto b = aleatorios(32);
    return paraHex(b.data(), b.size());
}

std::string derivarHashHex(const std::string& texto, const std::string& salHex) {
    const std::vector<uint8_t> sal = deHex(salHex);

    // Chave do HMAC: se passar de 64 bytes, vira o hash dela.
    std::array<uint8_t, 64> chave{};
    if (texto.size() > chave.size()) {
        Sha256 h;
        h.atualizar(reinterpret_cast<const uint8_t*>(texto.data()), texto.size());
        auto r = h.finalizar();
        std::memcpy(chave.data(), r.data(), r.size());
    } else {
        std::memcpy(chave.data(), texto.data(), texto.size());
    }
    std::array<uint8_t, 64> ipad{}, opad{};
    for (size_t i = 0; i < 64; ++i) {
        ipad[i] = chave[i] ^ 0x36;
        opad[i] = chave[i] ^ 0x5c;
    }
    // Estados já "alimentados" com o ipad/opad: cada iteração só copia e continua.
    Sha256 interno, externo;
    interno.atualizar(ipad.data(), 64);
    externo.atualizar(opad.data(), 64);

    auto hmac = [&](const uint8_t* msg, size_t n) {
        Sha256 i = interno;
        i.atualizar(msg, n);
        auto ri = i.finalizar();
        Sha256 o = externo;
        o.atualizar(ri.data(), ri.size());
        return o.finalizar();
    };

    // Um único bloco de saída (32 bytes): T1 = U1 ^ U2 ^ ... ^ Uc, com U1 = HMAC(senha, sal || 00000001).
    std::vector<uint8_t> primeira = sal;
    primeira.insert(primeira.end(), {0, 0, 0, 1});
    auto u = hmac(primeira.data(), primeira.size());
    auto t = u;
    for (unsigned i = 1; i < kIteracoes; ++i) {
        u = hmac(u.data(), u.size());
        for (size_t j = 0; j < t.size(); ++j) t[j] ^= u[j];
    }
    return paraHex(t.data(), t.size());
}

bool iguais(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char dif = 0;
    for (size_t i = 0; i < a.size(); ++i) dif |= static_cast<unsigned char>(a[i] ^ b[i]);
    return dif == 0;
}

}  // namespace senha
