#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include "ui/NombreItem.h"
#include <cmath>
#include <cstring>

// ============================================================================
// EL CADAVER DEL PECARI: SE EMPUJA Y SE RECOGE CON F
// ============================================================================
// PEDIDO: "que cuando esta en modo ragdoll el pecari muerto, puedas empujarlo
// con la hitbox del jugador de forma realista, y que si le presionas F, te de
// un nuevo item llamado Cadaver de Pecari".

using namespace Fauna;
using NombreItem::nombreDeItem;

namespace {

class Llano : public IPecariMundo {
public:
    float alturaSuelo(int, int) const override { return 64.0f; }
    BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
    float pendienteEn(int, int) const override { return 0.0f; }
    bool esSolido(int, int y, int) const override { return y < 64; }
};

constexpr float RADIO_JUGADOR  = 0.30f;   // los de main.cpp
constexpr float ALTURA_JUGADOR = 1.80f;
constexpr float DT = 1.0f / 60.0f;

// Un mundo con un solo cadaver adulto, ya quieto en el suelo.
void cadaverQuieto(MundoPecaries& mundo, const Llano& suelo) {
    mundo.soltarCadaver(10.5f, 64.0f, 10.5f, 0.0f, 0.0f, 0.0f, 7u);
    for (int i = 0; i < 600; ++i) mundo.actualizar(DT, suelo);
    REQUIRE(mundo.cuerpos().size() == 1);
}

} // namespace

TEST_CASE("Cadaver: la masa decide cuanto cede cada uno") {
    // Una cria cede casi todo; un adulto, bastante menos. Nunca 0 ni 1: los
    // dos cuerpos se apartan algo.
    const float cria   = CesionAnteJugador(EtapaPecari::NEONATO);
    const float adulto = CesionAnteJugador(EtapaPecari::ADULTO);
    CHECK(cria > adulto);
    CHECK(cria < 1.0f);
    CHECK(adulto > 0.5f);
    CHECK(adulto < 1.0f);
    // El radio del cuerpo tambien sale de la etapa.
    CHECK(RadioCuerpoBloques(EtapaPecari::NEONATO, 1.0f) <
          RadioCuerpoBloques(EtapaPecari::ADULTO, 1.0f));
}

TEST_CASE("Cadaver: tirado vuelve a caer al suelo y se duerme") {
    Llano suelo;
    MundoPecaries mundo(1);
    cadaverQuieto(mundo, suelo);
    const Ragdoll& r = mundo.cuerpos().front();
    CHECK(r.dormido);
    CHECK(r.etapa == (int)EtapaPecari::ADULTO);
    // Sobre el suelo (y = 64), no hundido ni flotando.
    CHECK(r.centro().y >= 64.0f);
    CHECK(r.centro().y < 65.0f);
}

TEST_CASE("Cadaver: el jugador que camina contra el lo arrastra") {
    Llano suelo;
    MundoPecaries mundo(1);
    cadaverQuieto(mundo, suelo);
    const V3 antes = mundo.cuerpos().front().centro();

    // El jugador viene desde -X, a 4 bloques/s, y sigue andando a traves.
    float px = antes.x - 2.0f;
    const float pz = antes.z;
    float devueltoTotalX = 0.0f;
    int framesConChoque = 0;
    for (int i = 0; i < 60; ++i) {
        px += 4.0f * DT;
        float ox = 0.0f, oz = 0.0f;
        if (mundo.jugadorEmpujaCadaveres(px, 64.0f, pz, RADIO_JUGADOR,
                                         ALTURA_JUGADOR, ox, oz) > 0) {
            ++framesConChoque;
            devueltoTotalX += ox;
            px += ox * 0.25f;   // el mismo suavizado que main.cpp
        }
        mundo.actualizar(DT, suelo);
    }
    REQUIRE(framesConChoque > 0);

    const Ragdoll& r = mundo.cuerpos().front();
    const V3 despues = r.centro();
    // Se fue hacia donde andaba el jugador, y bastante: no es un estorbo fijo.
    CHECK(despues.x - antes.x > 0.5f);
    // Sin desviarse a un lado mas que lo que avanzo.
    CHECK(std::fabs(despues.z - antes.z) < (despues.x - antes.x));
    // Y no se metio en el suelo ni salio volando.
    CHECK(despues.y >= 64.0f);
    CHECK(despues.y < 65.0f);
    // El cuerpo devuelve parte del choque al jugador: hacia atras.
    CHECK(devueltoTotalX < 0.0f);
}

TEST_CASE("Cadaver: pasar lejos, o por encima, no lo toca") {
    Llano suelo;
    MundoPecaries mundo(1);
    cadaverQuieto(mundo, suelo);
    const V3 c = mundo.cuerpos().front().centro();
    float ox = 0.0f, oz = 0.0f;
    // A tres bloques de lado.
    CHECK(mundo.jugadorEmpujaCadaveres(c.x, 64.0f, c.z + 3.0f, RADIO_JUGADOR,
                                       ALTURA_JUGADOR, ox, oz) == 0);
    // Justo encima, con los pies por encima del cuerpo.
    CHECK(mundo.jugadorEmpujaCadaveres(c.x, 66.0f, c.z, RADIO_JUGADOR,
                                       ALTURA_JUGADOR, ox, oz) == 0);
    CHECK(mundo.cuerpos().front().dormido);
}

TEST_CASE("Cadaver: F lo apunta de frente y no de espaldas") {
    Llano suelo;
    MundoPecaries mundo(1);
    cadaverQuieto(mundo, suelo);
    const V3 c = mundo.cuerpos().front().centro();

    // El ojo a 3 bloques, apuntando al centro del cuerpo.
    const float ox = c.x - 3.0f, oy = c.y + 1.0f, oz = c.z;
    float dx = c.x - ox, dy = c.y - oy, dz = c.z - oz;
    const float l = std::sqrt(dx * dx + dy * dy + dz * dz);
    dx /= l; dy /= l; dz /= l;

    float dist = 0.0f;
    CHECK(mundo.cadaverApuntado(ox, oy, oz, dx, dy, dz, 5.0f, dist) == 0);
    CHECK(dist > 2.0f);
    CHECK(dist < l);

    // Mirando al lado contrario no hay nada.
    CHECK(mundo.cadaverApuntado(ox, oy, oz, -dx, -dy, -dz, 5.0f, dist) == -1);
    // Y fuera de alcance tampoco.
    CHECK(mundo.cadaverApuntado(ox - 10.0f, oy, oz, 1.0f, 0.0f, 0.0f, 5.0f, dist) == -1);
}

TEST_CASE("Cadaver: recogerlo lo saca del mundo") {
    Llano suelo;
    MundoPecaries mundo(1);
    cadaverQuieto(mundo, suelo);
    CHECK_FALSE(mundo.retirarCadaver(5));
    CHECK(mundo.retirarCadaver(0));
    CHECK(mundo.cuerpos().empty());
}

TEST_CASE("Cadaver: el pecari que muere de un golpe queda para recoger") {
    Llano suelo;
    MundoPecaries mundo(4242);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, suelo, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() > 0);
    const PecariAgente a = mundo.todos().front();
    REQUIRE(mundo.recibirDano(a.id, 1000, a.x - 3.0f, a.z, suelo));
    REQUIRE(mundo.cuerpos().size() == 1);
    // Se apunta por donde haya caido: el rayo va en vertical, desde arriba.
    const V3 c = mundo.cuerpos().front().centro();
    float dist = 0.0f;
    CHECK(mundo.cadaverApuntado(c.x, c.y + 3.0f, c.z, 0.0f, -1.0f, 0.0f,
                                5.0f, dist) == 0);
}

TEST_CASE("Cadaver: el item existe, tiene nombre y cierra el enum") {
    CHECK((int)BLOCK_CADAVER_PECARI == BLOCK_TYPE_MAX);
    const char* n = nombreDeItem(BLOCK_CADAVER_PECARI);
    REQUIRE(n != nullptr);
    CHECK(std::strcmp(n, "CADAVER DE PECARI") == 0);
}
