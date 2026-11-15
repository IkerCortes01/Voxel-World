#pragma once

#include <string>

// ============================================================================
// ⭐ EL TEXTO QUE ESCRIBE EL JUGADOR, EN UTF-8
// ============================================================================
// PEDIDO: "agrega en todo Voxel World que puedas escribir con minusculas, poner
// acentos, simbolos, signos y poner manual mayusculas y el @, y otros mas".
//
// Antes cada campo guardaba un `char` por tecla y solo dejaba pasar ASCII (y
// casi siempre solo letras, numeros y un par de signos). Una "ñ" no entraba, y
// si hubiera entrado habria ocupado un byte suelto que nadie sabia leer.
//
// Ahora el texto se guarda en UTF-8, que es como lo escribe la pagina web y
// como se guarda en los archivos: una contrasena con acento da el mismo hash
// aqui que en el navegador. Lo que cambia es que un caracter ya no es un byte:
//
//   - para ANADIR una tecla se anade su codigo entero (1 o 2 bytes),
//   - para BORRAR se quita el ultimo caracter ENTERO, no su ultimo byte
//     (quitar medio caracter deja el texto roto),
//   - el LARGO se cuenta en caracteres, no en bytes.
//
// Se admite lo que la fuente sabe dibujar (ver FuenteBits.h): ASCII imprimible
// (32-126) y Latin-1 (161-255), que es todo el teclado espanol.
//
// Sin motor: lo prueba tests/test_texto_utf8.cpp.
// ============================================================================

namespace Utf8 {

// ¿Se puede escribir ese caracter?
inline bool seEscribe(unsigned cp) {
    return (cp >= 32 && cp <= 126) || (cp >= 161 && cp <= 255);
}

// Anade el caracter al final.
inline void anadir(std::string& s, unsigned cp) {
    if (cp < 0x80) {
        s += (char)cp;
    } else if (cp < 0x800) {
        s += (char)(0xC0 | (cp >> 6));
        s += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += (char)(0xE0 | (cp >> 12));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    }
}

// Lee el caracter que empieza en `i` y avanza `i`. Un byte suelto que no es
// UTF-8 valido se toma tal cual (Latin-1): asi un texto viejo guardado de otra
// manera no se queda en blanco.
inline unsigned leer(const std::string& s, size_t& i) {
    const unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { ++i; return c; }
    auto sigue = [&](size_t k) { return k < s.size() && ((unsigned char)s[k] & 0xC0) == 0x80; };
    if ((c & 0xE0) == 0xC0 && sigue(i + 1)) {
        const unsigned cp = ((c & 0x1Fu) << 6) | ((unsigned char)s[i + 1] & 0x3Fu);
        if (cp >= 0x80) { i += 2; return cp; }
    } else if ((c & 0xF0) == 0xE0 && sigue(i + 1) && sigue(i + 2)) {
        const unsigned cp = ((c & 0x0Fu) << 12) | (((unsigned char)s[i + 1] & 0x3Fu) << 6) |
                            ((unsigned char)s[i + 2] & 0x3Fu);
        if (cp >= 0x800) { i += 3; return cp; }
    } else if ((c & 0xF8) == 0xF0 && sigue(i + 1) && sigue(i + 2) && sigue(i + 3)) {
        const unsigned cp = ((c & 0x07u) << 18) | (((unsigned char)s[i + 1] & 0x3Fu) << 12) |
                            (((unsigned char)s[i + 2] & 0x3Fu) << 6) | ((unsigned char)s[i + 3] & 0x3Fu);
        i += 4;
        return cp;
    }
    ++i;
    return c;
}

// Cuantos caracteres tiene (no bytes).
inline size_t largo(const std::string& s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size(); ++n) leer(s, i);
    return n;
}

// Quita el ultimo caracter, entero.
inline void borrar(std::string& s) {
    if (s.empty()) return;
    size_t i = 0, ultimo = 0;
    while (i < s.size()) { ultimo = i; leer(s, i); }
    s.erase(ultimo);
}

// Los ultimos `n` caracteres (para un campo en el que no cabe todo).
inline std::string cola(const std::string& s, size_t n) {
    const size_t total = largo(s);
    if (total <= n) return s;
    size_t i = 0;
    for (size_t k = 0; k < total - n; ++k) leer(s, i);
    return s.substr(i);
}

// La mayuscula de un caracter: tambien las vocales con acento y la enie.
inline unsigned mayuscula(unsigned cp) {
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7) return cp - 32;      // à..þ, menos el signo de dividir
    return cp;
}

// El texto sin lo que no se puede escribir, sin espacios al principio ni al
// final y con `maximo` caracteres como mucho.
inline std::string limpiar(const std::string& s, size_t maximo) {
    std::string o;
    size_t n = 0;
    bool algo = false;
    for (size_t i = 0; i < s.size() && n < maximo;) {
        const unsigned cp = leer(s, i);
        if (!seEscribe(cp)) continue;
        if (cp == ' ' && !algo) continue;
        algo = true;
        anadir(o, cp);
        ++n;
    }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
}

}  // namespace Utf8
