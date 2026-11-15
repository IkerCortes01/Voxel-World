// EL TEXTO DEL JUGADOR EN UTF-8 Y LOS GLIFOS DE LA FUENTE
// (src/ui/TextoUtf8.h y src/ui/FuenteBits.h).
// PEDIDO: "agrega en todo Voxel World que puedas escribir con minusculas, poner
// acentos, simbolos, signos y poner manual mayusculas y el @, y otros mas".
#include "doctest/doctest.h"

#include "Cuenta.h"
#include "WorldName.h"
#include "ui/FuenteBits.h"
#include "ui/TextoUtf8.h"

#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string escribir(std::initializer_list<unsigned> teclas) {
    std::string s;
    for (unsigned cp : teclas) if (Utf8::seEscribe(cp)) Utf8::anadir(s, cp);
    return s;
}
std::string fuente(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Texto: se escribe con minusculas, mayusculas, acentos, enie, arroba y signos") {
    // Tecla a tecla, como llega del teclado (codigos Unicode).
    const std::string s = escribir({ 'N', 'i', 0xF1, 'o', ' ', 0xC1, 'g', 'i', 'l', ' ', '@', '#', '$', '%', '&',
                                     0xBF, '?', 0xA1, '!' });
    CHECK(s == "Ni\xC3\xB1o \xC3\x81gil @#$%&\xC2\xBF?\xC2\xA1!");      // "Niño Ágil @#$%&¿?¡!" en UTF-8
    CHECK(Utf8::largo(s) == 19);                 // caracteres, no bytes
    CHECK(s.size() == 23);
    // Las mayusculas y las minusculas se quedan como se pusieron.
    CHECK(escribir({ 'a', 'B', 'c' }) == "aBc");
    // Lo que no se escribe: controles, el borrar, y lo de fuera de Latin-1.
    for (unsigned cp : { 0u, 9u, 10u, 13u, 27u, 127u, 0x80u, 0x9Fu, 0xA0u, 0x100u, 0x20ACu, 0x1F600u })
        CHECK_FALSE(Utf8::seEscribe(cp));
    for (unsigned cp = 32; cp <= 126; ++cp) CHECK(Utf8::seEscribe(cp));
    for (unsigned cp = 161; cp <= 255; ++cp) CHECK(Utf8::seEscribe(cp));
}

TEST_CASE("Texto: borrar quita el ultimo caracter ENTERO, no medio") {
    std::string s = "ma\xC3\xB1" "ana";          // "mañana"
    Utf8::borrar(s); Utf8::borrar(s); Utf8::borrar(s);
    CHECK(s == "ma\xC3\xB1");                    // "mañ"
    Utf8::borrar(s);
    CHECK(s == "ma");                            // la enie se fue entera (2 bytes)
    Utf8::borrar(s); Utf8::borrar(s);
    CHECK(s.empty());
    Utf8::borrar(s);                             // en vacio no pasa nada
    CHECK(s.empty());
    // Ida y vuelta de todo lo que se puede escribir.
    for (unsigned cp = 32; cp <= 255; ++cp) {
        if (!Utf8::seEscribe(cp)) continue;
        std::string t;
        Utf8::anadir(t, cp);
        size_t i = 0;
        CHECK(Utf8::leer(t, i) == cp);
        CHECK(i == t.size());
        CHECK(Utf8::largo(t) == 1);
    }
    // Los ultimos caracteres, sin partir ninguno.
    CHECK(Utf8::cola("a\xC3\xB1o\xC3\xB1" "e", 3) == "o\xC3\xB1" "e");
    CHECK(Utf8::cola("corto", 20) == "corto");
}

TEST_CASE("Texto: un byte suelto que no es UTF-8 se lee como Latin-1, y nada se cuelga") {
    size_t i = 0;
    const std::string viejo = "\xF1";            // una enie guardada a la antigua
    CHECK(Utf8::leer(viejo, i) == 0xF1);
    CHECK(i == 1);
    // Cualquier basura avanza siempre y termina.
    for (int a = 0; a < 256; ++a)
        for (int b : { 0x00, 0x41, 0x80, 0xBF, 0xC3, 0xE2, 0xF0, 0xFF }) {
            const std::string t = { (char)a, (char)b };
            size_t k = 0, pasos = 0;
            while (k < t.size() && pasos < 10) { Utf8::leer(t, k); ++pasos; }
            CHECK(k == t.size());
            CHECK(pasos <= 2);
        }
    // Una codificacion "larga" de un caracter ASCII no se acepta como tal.
    i = 0;
    CHECK(Utf8::leer("\xC0\x80", i) != 0u);
}

TEST_CASE("Texto: las mayusculas de los rotulos, tambien con acento") {
    CHECK(Utf8::mayuscula('a') == 'A');
    CHECK(Utf8::mayuscula('z') == 'Z');
    CHECK(Utf8::mayuscula(0xF1) == 0xD1);        // ñ -> Ñ
    CHECK(Utf8::mayuscula(0xE1) == 0xC1);        // á -> Á
    CHECK(Utf8::mayuscula(0xFC) == 0xDC);        // ü -> Ü
    CHECK(Utf8::mayuscula('A') == 'A');
    CHECK(Utf8::mayuscula('@') == '@');
    CHECK(Utf8::mayuscula('5') == '5');
    CHECK(Utf8::mayuscula(0xF7) == 0xF7);        // el signo de dividir no es una letra
    CHECK(Utf8::mayuscula(0xBF) == 0xBF);        // ¿
}

TEST_CASE("Texto: la fuente tiene dibujo para todo lo que se puede escribir") {
    for (unsigned cp = 0; cp < 256; ++cp) {
        if (cp == 32) continue;                  // el espacio no pinta nada
        const uint16_t* g = FuenteBits::glifo(cp);
        int encendidos = 0;
        for (int f = 0; f < FuenteBits::ALTO; ++f) {
            CHECK(g[f] < (1u << FuenteBits::ANCHO));     // cabe en su ancho
            for (int c = 0; c < FuenteBits::ANCHO; ++c) encendidos += (g[f] >> c) & 1;
        }
        INFO("caracter " << cp);
        if (Utf8::seEscribe(cp)) {
            CHECK(FuenteBits::tiene(cp));
            CHECK(encendidos >= 3);              // se ve algo
        } else {
            CHECK(encendidos == 0);
        }
    }
    // Una minuscula no es su mayuscula (antes se pintaban igual), ni la enie la ene.
    auto distintos = [](unsigned a, unsigned b) {
        const uint16_t* x = FuenteBits::glifo(a);
        const uint16_t* y = FuenteBits::glifo(b);
        for (int f = 0; f < FuenteBits::ALTO; ++f) if (x[f] != y[f]) return true;
        return false;
    };
    for (unsigned c = 'a'; c <= 'z'; ++c) CHECK(distintos(c, c - 32));
    CHECK(distintos('n', 0xF1));
    CHECK(distintos('a', 0xE1));
    CHECK(distintos('a', '@'));
    // Una mayuscula ocupa las 12 filas de en medio; la "g" baja por debajo.
    const uint16_t* H = FuenteBits::glifo('H');
    CHECK(H[FuenteBits::ARRIBA] != 0);
    CHECK(H[FuenteBits::ARRIBA + FuenteBits::MAYUSCULA - 1] != 0);
    CHECK(H[FuenteBits::ARRIBA - 1] == 0);
    CHECK(H[FuenteBits::ARRIBA + FuenteBits::MAYUSCULA] == 0);
    CHECK(FuenteBits::glifo('g')[FuenteBits::ARRIBA + FuenteBits::MAYUSCULA] != 0);
    {
        // El acento de la A mayuscula va POR ENCIMA de la letra.
        unsigned sobre = 0;
        for (int f = 0; f < FuenteBits::ARRIBA; ++f) sobre |= FuenteBits::glifo(0xC1)[f];
        CHECK(sobre != 0);
    }
}

TEST_CASE("Texto: el nombre de un mundo y el de la cuenta admiten todo el teclado") {
    using WorldName::limpiarTitulo;
    CHECK(limpiarTitulo("  Mi Casa\xC3\xB1" "a: \xC2\xBFS\xC3\xAD? @home #1  ") == "Mi Casa\xC3\xB1" "a: \xC2\xBFS\xC3\xAD? @home #1");
    CHECK(limpiarTitulo("min\xC3\xBAsculas y MAY\xC3\x9ASCULAS") == "min\xC3\xBAsculas y MAY\xC3\x9ASCULAS");
    CHECK(limpiarTitulo("con\nsalto\ty tab") == "consaltoy tab");        // los controles se quitan
    CHECK(limpiarTitulo("   ") == "");
    // El largo maximo, en caracteres: 50 enies son 100 bytes y caben.
    std::string enies;
    for (int i = 0; i < 60; ++i) enies += "\xC3\xB1";
    CHECK(Utf8::largo(limpiarTitulo(enies)) == WorldName::MAX_LENGTH);
    // La cuenta.
    CHECK(Cuenta::nombreVale("Iker Cort\xC3\xA9s"));
    CHECK(Cuenta::nombreVale("i\xC3\xB1" "aki_01 @casa"));
    CHECK_FALSE(Cuenta::nombreVale("dos\nlineas"));
    CHECK_FALSE(Cuenta::nombreVale(" con espacio delante"));
    CHECK(Cuenta::claveVale("contrase\xC3\xB1" "a-con-\xC3\xA1" "cento!"));
    CHECK_FALSE(Cuenta::claveVale("\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1"));  // 4 caracteres (8 bytes): corta
    CHECK(Cuenta::normalizarUsuario("IkerCortes@VoxelWorld") == "ikercortes");
}

TEST_CASE("Texto: el motor escribe y dibuja con todo el teclado en sus campos") {
    const std::string src = fuente("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // Los rotulos ya no se quedan en blanco con lo que la fuente de siempre no tiene.
    CHECK(src.find("else dibujarGlifoBits(cp, currentX, y, size);") != std::string::npos);
    CHECK(src.find("renderChar(toupper(text[i]), currentX, y, size);") == std::string::npos);
    // Lo del jugador se dibuja tal cual.
    CHECK(src.find("void renderTextoTal(const char* text, float x, float y, float size) {") != std::string::npos);
    CHECK(src.find("renderTextoTalConSombra(worldName.c_str(), btn.x + 20, btn.y + 15);") != std::string::npos);
    CHECK(src.find("renderTextoTal(c.reg.nombre.c_str(), tx, i.cy - 18.0f, 16);") != std::string::npos);
    // Se escribe en UTF-8 y se borra por caracteres enteros, en todos los campos.
    CHECK(src.find("Utf8::anadir(g_gameState->editingWorldNewName, codepoint);") != std::string::npos);
    CHECK(src.find("Utf8::anadir(g_gameState->newWorldName, codepoint);") != std::string::npos);
    CHECK(src.find("editingWorldNewName.pop_back()") == std::string::npos);
    CHECK(src.find("newWorldName.pop_back()") == std::string::npos);
    CHECK(src.find("Utf8::borrar(*campo[c.foco]);") != std::string::npos);
    // La arroba entra en la cuenta.
    CHECK(src.find("cp == '_' || cp == '@');") != std::string::npos);
}
