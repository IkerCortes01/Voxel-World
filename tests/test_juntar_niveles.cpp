#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <iterator>
#include <map>
#include <string>

// ============================================================================
// JUNTAR NIVELES DESDE CUALQUIER ANGULO Y POSICION
// ============================================================================
// PEDIDO: "que en cualquier direccion de mirada y posicion del jugador puedas
// juntar niveles sin limites".

namespace {

struct Columna {
    std::map<int, BlockType> celdas;
    BlockType get(int y) const {
        auto it = celdas.find(y);
        return it == celdas.end() ? BLOCK_AIR : it->second;
    }
    // Aplica el plan y devuelve si se pudo.
    bool depositar(int y, BlockType material) {
        EscrituraColumna esc[MAX_ESCRITURAS_COLUMNA];
        int n = 0;
        const bool ok = planearDeposito(
            [&](int cy) { return get(cy); },
            [](BlockType t) { return t == BLOCK_AIR; },
            y, 512, material, esc, n);
        if (ok) for (int i = 0; i < n; ++i) celdas[esc[i].y] = esc[i].tipo;
        return ok;
    }
    int octavos() const {
        int s = 0;
        for (const auto& c : celdas) s += octavosOcupados(c.second);
        return s;
    }
};

std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}

} // namespace

TEST_CASE("Juntar: lo que sobra se funde con la capa de arriba, no se pierde") {
    // Antes, si la celda de arriba no estaba vacia, ese material desaparecia.
    Columna c;
    c.celdas[10] = conNivel(BLOCK_DIRT, 7);
    c.celdas[11] = conNivel(BLOCK_SAND, 2);

    REQUIRE(c.depositar(10, conNivel(BLOCK_STONE, 3)));
    CHECK(c.octavos() == 7 + 2 + 3);
    CHECK(octavosOcupados(c.get(10)) == 8);
    CHECK(octavosOcupados(c.get(11)) == 4);
}

TEST_CASE("Juntar: lo que sobra no atraviesa un techo") {
    // Bloque entero bajo un techo: su sobrante no aparece encima del techo.
    Columna c;
    c.celdas[10] = conNivel(BLOCK_DIRT, 3);
    c.celdas[11] = BLOCK_STONE;
    const auto antes = c.celdas;
    CHECK_FALSE(c.depositar(10, BLOCK_COBBLESTONE));
    CHECK(c.celdas == antes);
}

TEST_CASE("Juntar: apuntando a una celda llena, va a la de encima") {
    Columna c;
    c.celdas[10] = BLOCK_STONE;
    REQUIRE(c.depositar(10, conNivel(BLOCK_DIRT, 2)));
    CHECK(c.get(10) == BLOCK_STONE);
    CHECK(c.get(11) == conNivel(BLOCK_DIRT, 2));
}

TEST_CASE("Juntar: si no tiene donde acabar, no escribe nada") {
    // Un item que no es capa corta la columna: el clic no gasta nada.
    Columna c;
    c.celdas[10] = conNivel(BLOCK_DIRT, 7);
    c.celdas[11] = BLOCK_TALLGRASS;
    const auto antes = c.celdas;
    CHECK_FALSE(c.depositar(10, conNivel(BLOCK_STONE, 3)));
    CHECK(c.celdas == antes);
}

TEST_CASE("Juntar: no hay tope de clics, la columna crece sin perder material") {
    const BlockType mats[] = { BLOCK_DIRT, BLOCK_SAND, BLOCK_STONE, BLOCK_GRAVEL };
    Columna c;
    int cima = 0;   // la celda a la que apunta el jugador: la mas alta con algo
    for (int i = 0; i < 200; ++i) {
        INFO("clic ", i);
        REQUIRE(c.depositar(cima, conNivel(mats[i % 4], 1)));
        CHECK(c.octavos() == i + 1);
        if (celdaLlena(c.get(cima))) ++cima;
    }
}

TEST_CASE("Juntar: el motor ya no pierde el clic segun el angulo") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Destino = capa con sitio: se junta en vez de salir sin hacer nada.
    const size_t m = src.find("SI EL DESTINO ES UNA CAPA CON SITIO, SE JUNTAN");
    REQUIRE(m != std::string::npos);
    CHECK(src.substr(m, 1800).find("depositarEnColumna(placePos.x, placePos.y,")
          != std::string::npos);

    // La prueba contra el jugador mira donde va a caer la capa.
    CHECK(src.find("Render::celdaPisaAlJugador(\n            placePos.x, yPrueba")
          != std::string::npos);
}
