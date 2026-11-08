#include <doctest/doctest.h>
#include "fauna/AnimalMallaCache.h"
#include <cstdio>

// ============================================================================
// LA POSTURA: SOLO LAS PATAS TOCAN EL SUELO
// ============================================================================
// PEDIDO: "que los pecaris no esten tan pegados al suelo ... y que solo las
// patas rozen con el suelo".
//
// El vientre tiene que quedar despegado y las pezunas, justo en y = 0.
// ============================================================================

using namespace Fauna;

namespace {
struct Alturas { float pezuna, vientre, pata; };

Alturas medir(int etapa) {
    MallaAnimal m;
    ParametrosPecari p = Especies::aplicarEdad(Especies::pecariDeCollar(), etapa);
    ConstructorPecari::generar(m, p, 0);
    Alturas a{ 1e9f, 1e9f, 1e9f };
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::PEZUNA) a.pezuna = (std::min)(a.pezuna, v.pos.y);
        else if (v.zona == ZonaCuerpo::PATA) a.pata = (std::min)(a.pata, v.pos.y);
        else a.vientre = (std::min)(a.vientre, v.pos.y);
    }
    return a;
}
} // namespace

TEST_CASE("Pecari: las pezunas en el suelo y el vientre bien despegado") {
    for (int etapa = 0; etapa <= 4; ++etapa) {
        const Alturas a = medir(etapa);
        std::printf("etapa %d: pezuna %.3f  pata %.3f  cuerpo %.3f\n", etapa, a.pezuna, a.pata, a.vientre);
        CHECK(a.pezuna > -0.01f);
        CHECK(a.pezuna < 0.01f);
        // Nada que no sea pata o pezuna llega al suelo.
        CHECK(a.vientre > 0.04f);
    }
    // El adulto: el vientre a mas de 21 cm (antes, 16).
    CHECK(medir(3).vientre > 0.21f);
}
