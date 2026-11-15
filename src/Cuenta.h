#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ui/TextoUtf8.h"

// ============================================================================
// ⭐ LA CUENTA DEL JUGADOR
// ============================================================================
// PEDIDO: "anade un sistema de cuentas: antes de que inicies el juego te
// pedira poner en una mini seccion tu nombre, cuenta, contrasena, autenticador
// y si todo esta en orden te deja jugar [...] usa el sistema de Ludora para
// ubicarte".
//
// Como en Ludora, la cuenta es LOCAL: el juego no tiene red ni servidor, asi
// que no hay nadie al otro lado que la compruebe. Vive en un archivo de texto
// (cuenta.dat) junto a las partidas, y lo que guarda es:
//
//   nombre   como te llamas (lo que se ve en el menu).
//   cuenta   tu identificador: "iker@voxelworld".
//   hash     NO la contrasena: PBKDF2-HMAC-SHA256 de ella con una sal al azar
//            y 120.000 vueltas. Del hash no se vuelve a la contrasena, y las
//            vueltas encarecen probar contrasenas una a una. (Ludora guarda
//            SHA-256(sal + contrasena), una sola vuelta; esto es mas duro.)
//   totp     el secreto del AUTENTICADOR (RFC 6238, el de Google Authenticator
//            y similares: 6 digitos que cambian cada 30 s), tapado con una
//            clave que sale de la contrasena. Sin la contrasena no se puede
//            sacar el secreto del archivo.
//
// Para entrar hacen falta las tres cosas: la cuenta, la contrasena y el codigo
// de 6 digitos que marca la aplicacion en ese momento. Los errores son
// genericos ("cuenta o contrasena incorrectas"): no se dice cual fallo.
//
// EL MISMO ARCHIVO LO PUEDE CREAR LA PAGINA WEB (adquirir.html), en el
// navegador y sin mandar nada a ningun sitio: usa estas mismas cuentas
// (PBKDF2 y HMAC de WebCrypto), asi que una cuenta hecha alli entra aqui.
//
// LO QUE ESTO NO ES. Sin servidor, la cuenta protege el juego en ESTE equipo
// como un candado: quien pueda borrar cuenta.dat puede crear otra. No es una
// cuenta en linea ni guarda nada fuera del equipo.
//
// Todo aqui es C++ sin sistema operativo (SHA-1, SHA-256, HMAC, PBKDF2, TOTP,
// Base32), para poder probarlo contra los vectores de sus RFC:
// tests/test_cuenta.cpp.
// ============================================================================

namespace Cuenta {

using Bytes = std::vector<uint8_t>;

// ------------------------------------------------------------------ SHA-256
inline Bytes sha256(const uint8_t* d, size_t n) {
    static const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
    uint32_t h[8] = { 0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19 };
    Bytes m(d, d + n);
    m.push_back(0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    const uint64_t bits = (uint64_t)n * 8;
    for (int i = 7; i >= 0; --i) m.push_back((uint8_t)(bits >> (i * 8)));
    auto rot = [](uint32_t x, int r) { return (x >> r) | (x << (32 - r)); };
    for (size_t o = 0; o < m.size(); o += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t)m[o + i * 4] << 24 | (uint32_t)m[o + i * 4 + 1] << 16 |
                   (uint32_t)m[o + i * 4 + 2] << 8 | m[o + i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rot(w[i - 15], 7) ^ rot(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rot(w[i - 2], 17) ^ rot(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], dd = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t S1 = rot(e, 6) ^ rot(e, 11) ^ rot(e, 25);
            const uint32_t t1 = hh + S1 + ((e & f) ^ (~e & g)) + K[i] + w[i];
            const uint32_t S0 = rot(a, 2) ^ rot(a, 13) ^ rot(a, 22);
            const uint32_t t2 = S0 + ((a & b) ^ (a & c) ^ (b & c));
            hh = g; g = f; f = e; e = dd + t1; dd = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += dd; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    Bytes out(32);
    for (int i = 0; i < 8; ++i)
        for (int k = 0; k < 4; ++k) out[(size_t)(i * 4 + k)] = (uint8_t)(h[i] >> (24 - k * 8));
    return out;
}

// -------------------------------------------------------------------- SHA-1
// Solo para el autenticador: TOTP esta definido sobre HMAC-SHA1 y es lo que
// hablan todas las aplicaciones. La contrasena no pasa por aqui.
inline Bytes sha1(const uint8_t* d, size_t n) {
    uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    Bytes m(d, d + n);
    m.push_back(0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    const uint64_t bits = (uint64_t)n * 8;
    for (int i = 7; i >= 0; --i) m.push_back((uint8_t)(bits >> (i * 8)));
    auto rol = [](uint32_t x, int r) { return (x << r) | (x >> (32 - r)); };
    for (size_t o = 0; o < m.size(); o += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t)m[o + i * 4] << 24 | (uint32_t)m[o + i * 4 + 1] << 16 |
                   (uint32_t)m[o + i * 4 + 2] << 8 | m[o + i * 4 + 3];
        for (int i = 16; i < 80; ++i) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], b = h[1], c = h[2], dd = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & dd);            k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ dd;                     k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & dd) | (c & dd);  k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ dd;                     k = 0xCA62C1D6; }
            const uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = dd; dd = c; c = rol(b, 30); b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += dd; h[4] += e;
    }
    Bytes out(20);
    for (int i = 0; i < 5; ++i)
        for (int k = 0; k < 4; ++k) out[(size_t)(i * 4 + k)] = (uint8_t)(h[i] >> (24 - k * 8));
    return out;
}

// --------------------------------------------------------------------- HMAC
template <typename THash>
inline Bytes hmac(THash hash, const Bytes& clave, const uint8_t* d, size_t n) {
    Bytes k = clave;
    if (k.size() > 64) k = hash(k.data(), k.size());
    k.resize(64, 0);
    Bytes dentro(64 + n), fuera(64);
    for (int i = 0; i < 64; ++i) { dentro[(size_t)i] = k[(size_t)i] ^ 0x36; fuera[(size_t)i] = k[(size_t)i] ^ 0x5c; }
    if (n) std::memcpy(dentro.data() + 64, d, n);
    const Bytes h1 = hash(dentro.data(), dentro.size());
    fuera.insert(fuera.end(), h1.begin(), h1.end());
    return hash(fuera.data(), fuera.size());
}
inline Bytes hmacSha256(const Bytes& k, const uint8_t* d, size_t n) { return hmac(sha256, k, d, n); }
inline Bytes hmacSha1(const Bytes& k, const uint8_t* d, size_t n) { return hmac(sha1, k, d, n); }

// ------------------------------------------------------------------- PBKDF2
// PBKDF2-HMAC-SHA256 (RFC 8018). `largo` bytes de salida.
inline Bytes pbkdf2(const std::string& clave, const Bytes& sal, int vueltas, size_t largo) {
    const Bytes k(clave.begin(), clave.end());
    Bytes out;
    for (uint32_t bloque = 1; out.size() < largo; ++bloque) {
        Bytes s = sal;
        for (int i = 3; i >= 0; --i) s.push_back((uint8_t)(bloque >> (i * 8)));
        Bytes u = hmacSha256(k, s.data(), s.size());
        Bytes t = u;
        for (int v = 1; v < vueltas; ++v) {
            u = hmacSha256(k, u.data(), u.size());
            for (size_t i = 0; i < t.size(); ++i) t[i] ^= u[i];
        }
        out.insert(out.end(), t.begin(), t.end());
    }
    out.resize(largo);
    return out;
}

// --------------------------------------------------------------------- TOTP
constexpr int TOTP_PASO = 30;       // segundos que dura cada codigo
constexpr int TOTP_DIGITOS = 6;
// El codigo del contador `c` (RFC 4226).
inline int hotp(const Bytes& secreto, uint64_t c) {
    uint8_t m[8];
    for (int i = 7; i >= 0; --i) { m[i] = (uint8_t)(c & 0xff); c >>= 8; }
    const Bytes h = hmacSha1(secreto, m, 8);
    const int o = h[19] & 0x0f;
    const uint32_t v = ((uint32_t)(h[(size_t)o] & 0x7f) << 24) | ((uint32_t)h[(size_t)o + 1] << 16) |
                       ((uint32_t)h[(size_t)o + 2] << 8) | h[(size_t)o + 3];
    return (int)(v % 1000000u);
}
// El codigo que toca en el instante `unix` (segundos).
inline int totp(const Bytes& secreto, long long unix) {
    return hotp(secreto, (uint64_t)(unix / TOTP_PASO));
}
// ¿Vale ese codigo ahora? Se acepta el paso anterior y el siguiente: el reloj
// del telefono y el del equipo nunca van iguales.
inline bool totpVale(const Bytes& secreto, int codigo, long long unix) {
    if (unix < TOTP_PASO) return false;
    int ok = 0;
    for (int d = -1; d <= 1; ++d) ok |= (totp(secreto, unix + d * TOTP_PASO) == codigo) ? 1 : 0;
    return ok != 0;
}

// --------------------------------------------------------- Texto <-> bytes
inline std::string aHex(const Bytes& b) {
    static const char* H = "0123456789abcdef";
    std::string s;
    for (uint8_t v : b) { s += H[v >> 4]; s += H[v & 15]; }
    return s;
}
inline Bytes deHex(const std::string& s) {
    auto v = [](char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                                 (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1)); };
    Bytes b;
    if (s.size() % 2) return b;
    for (size_t i = 0; i < s.size(); i += 2) {
        const int a = v(s[i]), c = v(s[i + 1]);
        if (a < 0 || c < 0) return Bytes();
        b.push_back((uint8_t)(a * 16 + c));
    }
    return b;
}
// Base32 (RFC 4648, sin relleno): asi se escribe el secreto en la aplicacion
// del autenticador. Solo letras A-Z y numeros 2-7.
inline std::string aBase32(const Bytes& b) {
    static const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string s;
    uint32_t buf = 0;
    int bits = 0;
    for (uint8_t v : b) {
        buf = (buf << 8) | v;
        bits += 8;
        while (bits >= 5) { s += A[(buf >> (bits - 5)) & 31]; bits -= 5; }
    }
    if (bits > 0) s += A[(buf << (5 - bits)) & 31];
    return s;
}
// Comparar sin que el tiempo delate donde esta la primera diferencia.
inline bool iguales(const Bytes& a, const Bytes& b) {
    if (a.size() != b.size()) return false;
    uint8_t d = 0;
    for (size_t i = 0; i < a.size(); ++i) d |= a[i] ^ b[i];
    return d == 0;
}

// ------------------------------------------------------------------ Reglas
constexpr const char* DOMINIO = "@voxelworld";
constexpr int VUELTAS = 120000;
constexpr size_t NOMBRE_MAX = 24, USUARIO_MIN = 3, USUARIO_MAX = 24, CLAVE_MIN = 8, CLAVE_MAX = 64;

// El nombre: cualquier cosa que se pueda escribir (minusculas, acentos, enie,
// signos), en UTF-8, hasta 24 caracteres y sin espacios en las puntas. Nada de
// saltos de linea ni caracteres de control: no se cuela otra linea del archivo.
inline bool todoSeEscribe(const std::string& s) {
    for (size_t i = 0; i < s.size();) if (!Utf8::seEscribe(Utf8::leer(s, i))) return false;
    return true;
}
inline bool nombreVale(const std::string& n) {
    if (n.empty() || n.front() == ' ' || n.back() == ' ') return false;
    return Utf8::largo(n) <= NOMBRE_MAX && todoSeEscribe(n);
}
// El usuario de la cuenta (lo de antes de la arroba): minusculas, numeros,
// punto, guion y guion bajo.
inline std::string normalizarUsuario(std::string u) {
    const size_t a = u.find('@');
    if (a != std::string::npos) u = u.substr(0, a);
    for (char& c : u) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return u;
}
inline bool usuarioVale(const std::string& u) {
    if (u.size() < USUARIO_MIN || u.size() > USUARIO_MAX) return false;
    for (char c : u)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return false;
    return true;
}
// La contrasena: de 8 a 64 caracteres de los que se pueden escribir, acentos
// y signos incluidos. Va en UTF-8, igual que la codifica el navegador.
inline bool claveVale(const std::string& c) {
    const size_t n = Utf8::largo(c);
    return n >= CLAVE_MIN && n <= CLAVE_MAX && todoSeEscribe(c);
}

// ---------------------------------------------------------------- El registro
struct Registro {
    std::string nombre;
    std::string cuenta;        // "usuario@voxelworld"
    Bytes sal, hash;           // de la contrasena
    Bytes salTotp, totp;       // el secreto del autenticador, tapado
    int vueltas = VUELTAS;
    std::string foto;          // archivo de la foto ("" si no hay)
    bool valido() const {
        return !nombre.empty() && !cuenta.empty() && sal.size() >= 8 && hash.size() == 32 &&
               salTotp.size() >= 8 && totp.size() == 20 && vueltas >= 1000;
    }
};

// Tapar y destapar el secreto del autenticador con la contrasena (es la misma
// operacion: XOR con 20 bytes que salen de ella).
inline Bytes taparSecreto(const Bytes& secreto, const std::string& clave, const Bytes& salTotp, int vueltas) {
    const Bytes k = pbkdf2(clave, salTotp, vueltas, secreto.size());
    Bytes o(secreto.size());
    for (size_t i = 0; i < o.size(); ++i) o[i] = secreto[i] ^ k[i];
    return o;
}

// Crear una cuenta. `sal` y `salTotp` (16 bytes) y `secreto` (20 bytes) los da
// quien llama, al azar. `vueltas` solo se baja en las pruebas.
inline Registro crear(const std::string& nombre, const std::string& usuario, const std::string& clave,
                      const Bytes& sal, const Bytes& salTotp, const Bytes& secreto, int vueltas = VUELTAS) {
    Registro r;
    r.nombre = nombre;
    r.cuenta = normalizarUsuario(usuario) + DOMINIO;
    r.sal = sal;
    r.salTotp = salTotp;
    r.vueltas = vueltas;
    r.hash = pbkdf2(clave, sal, vueltas, 32);
    r.totp = taparSecreto(secreto, clave, salTotp, vueltas);
    return r;
}

enum class Entrada { DENTRO, CREDENCIALES, CODIGO, SIN_CUENTA };

// Intentar entrar. `codigo` es el de 6 digitos del autenticador y `unix` la
// hora (segundos).
inline Entrada entrar(const Registro& r, const std::string& usuario, const std::string& clave, int codigo,
                      long long unix) {
    if (!r.valido()) return Entrada::SIN_CUENTA;
    // Se calcula el hash SIEMPRE, coincida o no la cuenta: tarda lo mismo.
    const Bytes h = pbkdf2(clave, r.sal, r.vueltas, 32);
    const bool cuentaOk = (normalizarUsuario(usuario) + DOMINIO) == r.cuenta;
    const bool claveOk = iguales(h, r.hash);
    if (!(cuentaOk && claveOk)) return Entrada::CREDENCIALES;
    const Bytes secreto = taparSecreto(r.totp, clave, r.salTotp, r.vueltas);
    return totpVale(secreto, codigo, unix) ? Entrada::DENTRO : Entrada::CODIGO;
}

inline const char* mensaje(Entrada e) {
    switch (e) {
        case Entrada::DENTRO:       return "";
        case Entrada::CREDENCIALES: return "CUENTA O CONTRASENA INCORRECTAS";
        case Entrada::CODIGO:       return "EL CODIGO DEL AUTENTICADOR NO VALE";
        default:                    return "NO HAY NINGUNA CUENTA EN ESTE EQUIPO";
    }
}

// -------------------------------------------------------------- El archivo
inline std::string aTexto(const Registro& r) {
    std::string s;
    s += "# Cuenta de Voxel World. La contrasena NO esta aqui: solo su hash\n";
    s += "# (PBKDF2-HMAC-SHA256). El secreto del autenticador va tapado con ella.\n";
    s += "version=1\n";
    s += "nombre=" + r.nombre + "\n";
    s += "cuenta=" + r.cuenta + "\n";
    s += "vueltas=" + std::to_string(r.vueltas) + "\n";
    s += "sal=" + aHex(r.sal) + "\n";
    s += "hash=" + aHex(r.hash) + "\n";
    s += "saltotp=" + aHex(r.salTotp) + "\n";
    s += "totp=" + aHex(r.totp) + "\n";
    if (!r.foto.empty()) s += "foto=" + r.foto + "\n";
    return s;
}
inline Registro deTexto(const std::string& texto) {
    Registro r;
    r.vueltas = 0;
    size_t i = 0;
    while (i < texto.size()) {
        size_t f = texto.find('\n', i);
        if (f == std::string::npos) f = texto.size();
        std::string l = texto.substr(i, f - i);
        i = f + 1;
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        const size_t ig = l.find('=');
        if (ig == std::string::npos) continue;
        const std::string k = l.substr(0, ig), v = l.substr(ig + 1);
        if (k == "nombre") r.nombre = v;
        else if (k == "cuenta") r.cuenta = v;
        else if (k == "vueltas") { r.vueltas = 0; for (char c : v) if (c >= '0' && c <= '9' && r.vueltas < 100000000) r.vueltas = r.vueltas * 10 + (c - '0'); }
        else if (k == "sal") r.sal = deHex(v);
        else if (k == "hash") r.hash = deHex(v);
        else if (k == "saltotp") r.salTotp = deHex(v);
        else if (k == "totp") r.totp = deHex(v);
        else if (k == "foto") r.foto = v;
    }
    // Un archivo retocado no se da por bueno a medias.
    if (!nombreVale(r.nombre) || r.cuenta.size() <= std::strlen(DOMINIO) ||
        r.cuenta.compare(r.cuenta.size() - std::strlen(DOMINIO), std::string::npos, DOMINIO) != 0 ||
        !usuarioVale(r.cuenta.substr(0, r.cuenta.size() - std::strlen(DOMINIO))) || r.vueltas > 5000000 ||
        !r.valido())
        return Registro();
    return r;
}

// El secreto escrito para teclearlo en la aplicacion: de cuatro en cuatro.
inline std::string secretoLegible(const Bytes& secreto) {
    const std::string b = aBase32(secreto);
    std::string s;
    for (size_t i = 0; i < b.size(); ++i) {
        if (i && i % 4 == 0) s += ' ';
        s += b[i];
    }
    return s;
}

}  // namespace Cuenta
