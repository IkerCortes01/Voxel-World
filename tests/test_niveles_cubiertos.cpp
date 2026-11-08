#include <doctest/doctest.h>
#include "NivelesCubiertos.h"
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

using namespace NivelesCubiertos;

// Un nivel cualquiera (el valor exacto da igual al registro).
static const BlockType NIVEL = (BlockType)(BLOCK_SAND + 1);

TEST_CASE("Niveles cubiertos: al romper el bloque entero vuelve el nivel") {
    Registro r;
    r.cubrir(4, 10, -7, NIVEL, BLOCK_STONE);
    CHECK(r.tiene(4, 10, -7));
    // Romper el bloque que lo tapo: la celda vuelve a ser el nivel.
    CHECK(r.destapar(4, 10, -7, BLOCK_STONE) == NIVEL);
    // Y la entrada se olvida: romper otra vez deja aire.
    CHECK_FALSE(r.tiene(4, 10, -7));
    CHECK(r.destapar(4, 10, -7, NIVEL) == BLOCK_AIR);
}

TEST_CASE("Niveles cubiertos: una entrada vieja no hace aparecer niveles") {
    Registro r;
    r.cubrir(0, 5, 0, NIVEL, BLOCK_STONE);
    // La piedra se fue por otro camino y ahora hay tierra: romperla deja aire.
    CHECK(r.destapar(0, 5, 0, BLOCK_DIRT) == BLOCK_AIR);
    CHECK(r.size() == 0);
    // Celdas sin registro: aire, como siempre.
    CHECK(r.destapar(1, 1, 1, BLOCK_STONE) == BLOCK_AIR);
}

TEST_CASE("Niveles cubiertos: se guardan y se cargan con el mundo") {
    Registro r;
    r.cubrir(1, 2, 3, NIVEL, BLOCK_STONE);
    r.cubrir(-100, 64, 250, BLOCK_SAND, BLOCK_DIRT);
    std::stringstream s;
    r.guardar(s);

    Registro leido;
    leido.cubrir(9, 9, 9, NIVEL, BLOCK_STONE);   // de otro mundo: se borra
    leido.cargar(s);
    CHECK(leido.size() == 2);
    CHECK_FALSE(leido.tiene(9, 9, 9));
    CHECK(leido.destapar(1, 2, 3, BLOCK_STONE) == NIVEL);
    CHECK(leido.destapar(-100, 64, 250, BLOCK_DIRT) == BLOCK_SAND);

    // Un archivo vacio o roto no revienta.
    std::stringstream vacio;
    leido.cargar(vacio);
    CHECK(leido.size() == 0);
}

TEST_CASE("Niveles cubiertos: el motor tapa en los dos caminos y destapa al romper") {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    const std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
#else
    const std::string src;
#endif
    REQUIRE_FALSE(src.empty());
    size_t n = 0;
    for (size_t p = src.find("g_nivelesCubiertos.cubrir("); p != std::string::npos;
         p = src.find("g_nivelesCubiertos.cubrir(", p + 1))
        ++n;
    CHECK(n == 2);
    CHECK(src.find("g_nivelesCubiertos.destapar(bx, by, bz, celdaAntes)") != std::string::npos);
    CHECK(src.find("niveles_cubiertos.dat") != std::string::npos);
}
