#include <doctest/doctest.h>
#include "AguaFlujo.h"
#include <map>
#include <set>
#include <tuple>
#include <vector>

// ============================================================================
// EL AGUA COMO EN MINECRAFT
// ============================================================================
// Ejecuta las reglas reales de AguaFlujo.h sobre una rejilla, con la misma cola
// y el mismo orden de escritura que World::updateWaterFlow.

using namespace Compuesto;

namespace {

using Pos = std::tuple<int, int, int>;

struct Mundo {
    std::map<Pos, BlockType> celdas;
    std::vector<Pos> cola;

    BlockType get(int x, int y, int z) const {
        auto it = celdas.find({ x, y, z });
        return it == celdas.end() ? BLOCK_AIR : it->second;
    }
    void set(int x, int y, int z, BlockType b) {
        if (b == BLOCK_AIR) celdas.erase({ x, y, z });
        else celdas[{ x, y, z }] = b;
    }
    void suelo(int y, int radio = 30) {
        for (int x = -radio; x <= radio; ++x)
            for (int z = -radio; z <= radio; ++z) set(x, y, z, BLOCK_STONE);
    }
    void despertar(int x, int y, int z) {
        static const int D[7][3] = { {0,0,0}, {1,0,0}, {-1,0,0}, {0,1,0},
                                     {0,-1,0}, {0,0,1}, {0,0,-1} };
        for (const auto& d : D) {
            const Pos p{ x + d[0], y + d[1], z + d[2] };
            if (esAguaCualquiera(get(x + d[0], y + d[1], z + d[2]))) cola.push_back(p);
        }
    }
    void asentar() {
        for (int tick = 0; tick < 2000 && !cola.empty(); ++tick) {
            std::vector<Pos> ahora;
            ahora.swap(cola);
            std::set<Pos> vistos;
            for (const Pos& p : ahora) {
                if (!vistos.insert(p).second) continue;
                const int x = std::get<0>(p), y = std::get<1>(p), z = std::get<2>(p);
                const BlockType actual = get(x, y, z);
                if (!esAguaCualquiera(actual)) continue;
                const AguaFlujo::Plan plan = AguaFlujo::planear(
                    [&](int dx, int dy, int dz) { return get(x + dx, y + dy, z + dz); });
                if (plan.propia != actual) {
                    set(x, y, z, plan.propia);
                    despertar(x, y, z);
                }
                if (plan.propia == BLOCK_AIR) continue;
                for (int i = 0; i < plan.n; ++i) {
                    const auto& e = plan.esc[i];
                    set(x + e.dx, y + e.dy, z + e.dz, e.tipo);
                    cola.push_back({ x + e.dx, y + e.dy, z + e.dz });
                }
            }
        }
        REQUIRE(cola.empty());   // la simulacion termina
    }
    int nivel(int x, int y, int z) const { return (int)Agua::nivelDe(get(x, y, z)); }
    int conAgua() const {
        int n = 0;
        for (const auto& c : celdas) if (esAguaCualquiera(c.second)) ++n;
        return n;
    }
};

} // namespace

TEST_CASE("Agua: la fuente es un bloque completo y no se gasta") {
    Mundo m;
    m.suelo(0);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.asentar();
    CHECK(AguaFlujo::esFuente(m.get(0, 1, 0)));
    CHECK(Agua::alturaVisual(m.get(0, 1, 0)) == doctest::Approx(1.0f));
}

TEST_CASE("Agua: se expande 7 bloques con un desnivel por bloque") {
    Mundo m;
    m.suelo(0);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.asentar();
    for (int d = 1; d <= 7; ++d) {
        INFO("a ", d, " bloques");
        CHECK(m.nivel(d, 1, 0) == 8 - d);
        CHECK(m.nivel(-d, 1, 0) == 8 - d);
        CHECK(m.nivel(0, 1, d) == 8 - d);
    }
    CHECK(m.get(8, 1, 0) == BLOCK_AIR);
    // El alcance es un rombo: distancia en pasos, no en linea recta.
    CHECK(m.nivel(3, 1, 3) == 2);
    CHECK(m.get(4, 1, 4) == BLOCK_AIR);
    // Rombo de radio 7: 1 + 2*7*8 = 113 celdas.
    CHECK(m.conAgua() == 113);
}

TEST_CASE("Agua: quitar la fuente seca todo lo que alimentaba") {
    Mundo m;
    m.suelo(0);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.asentar();
    m.set(0, 1, 0, BLOCK_AIR);
    m.despertar(0, 1, 0);
    m.asentar();
    CHECK(m.conAgua() == 0);
}

TEST_CASE("Agua: cae en columna y abajo vuelve a extenderse 7") {
    Mundo m;
    m.suelo(0);
    m.set(0, 6, 0, BLOCK_STONE);
    m.set(0, 7, 0, Agua::nuevo(Agua::LLENA));
    m.set(1, 6, 0, BLOCK_AIR);
    m.cola.push_back({ 0, 7, 0 });
    m.asentar();
    // Sale por +X, encuentra el hueco y baja.
    CHECK(Agua::estaCayendo(m.get(1, 6, 0)));
    CHECK(Agua::estaCayendo(m.get(1, 1, 0)));
    CHECK(m.nivel(2, 1, 0) == 7);
    CHECK(m.nivel(8, 1, 0) == 1);
    CHECK(m.get(9, 1, 0) == BLOCK_AIR);
}

TEST_CASE("Agua: va directo al hueco mas cercano") {
    Mundo m;
    m.suelo(0);
    m.set(2, 0, 0, BLOCK_AIR);                  // hueco a 2 bloques, en +X
    m.set(2, -1, 0, BLOCK_STONE);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.asentar();
    CHECK(m.nivel(1, 1, 0) == 7);
    CHECK(m.get(-1, 1, 0) == BLOCK_AIR);        // no se extiende al otro lado
}

TEST_CASE("Agua: dos fuentes con apoyo crean otra, sin apoyo no") {
    Mundo m;
    m.suelo(0);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.set(2, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.cola.push_back({ 2, 1, 0 });
    m.asentar();
    CHECK(AguaFlujo::esFuente(m.get(1, 1, 0)));

    const BlockType F = Agua::nuevo(Agua::LLENA);
    BlockType aire = BLOCK_AIR;
    auto sinApoyo = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return Agua::nuevo(3);
        if (dy == 0 && dz == 0 && (dx == 1 || dx == -1)) return F;
        return aire;
    };
    CHECK_FALSE(AguaFlujo::esFuente(AguaFlujo::estadoNuevo(sinApoyo)));
}

TEST_CASE("Agua: solo entra en el aire; un nopal o un maguey le cortan el paso") {
    for (BlockType planta : { BLOCK_NOPAL_PUESTO, BLOCK_MAGUEY_PUESTO, BLOCK_STONE }) {
        Mundo m;
        m.suelo(0);
        m.set(1, 1, 0, planta);
        m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
        m.cola.push_back({ 0, 1, 0 });
        m.asentar();
        CHECK(m.get(1, 1, 0) == planta);        // sigue ahi
        CHECK(m.nivel(-1, 1, 0) == 7);          // y por el otro lado fluye
    }
}

TEST_CASE("Agua: poner un bloque en el flujo no crea agua en otro sitio") {
    // Antes el agua tapada subia encima del bloque.
    Mundo m;
    m.suelo(0);
    m.set(0, 1, 0, Agua::nuevo(Agua::LLENA));
    m.cola.push_back({ 0, 1, 0 });
    m.asentar();
    m.set(1, 1, 0, BLOCK_NOPAL_PUESTO);
    m.despertar(1, 1, 0);
    m.asentar();
    CHECK(m.get(1, 2, 0) == BLOCK_AIR);
    CHECK(m.get(1, 1, 0) == BLOCK_NOPAL_PUESTO);
    // Lo que hay detras se alimenta rodeandolo.
    CHECK(m.nivel(2, 1, 0) == 4);
    // Y al quitarlo, el flujo vuelve.
    m.set(1, 1, 0, BLOCK_AIR);
    m.despertar(0, 1, 0);
    m.asentar();
    CHECK(m.nivel(1, 1, 0) == 7);
    CHECK(m.nivel(2, 1, 0) == 6);
}

TEST_CASE("Agua: el mar y los rios no se mueven ni se secan") {
    CHECK(AguaFlujo::esFuente(BLOCK_WATER));
    // Un rio de un mundo viejo: nivel 2 con corriente.
    const BlockType rio = Agua::conCorriente(Agua::nuevo(2), Agua::HACIA_ESTE);
    CHECK(AguaFlujo::esFuente(rio));
    CHECK(Agua::alturaVisual(rio) == doctest::Approx(1.0f));
    auto solo = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return rio;
        return BLOCK_STONE;
    };
    CHECK(AguaFlujo::estadoNuevo(solo) == rio);

    // Un charco sin fuente de un mundo viejo si se seca.
    auto charco = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return Agua::nuevo(5);
        return BLOCK_STONE;
    };
    CHECK(AguaFlujo::estadoNuevo(charco) == BLOCK_AIR);
}

TEST_CASE("Agua: un oceano quieto no hace nada") {
    // Rodeada de fuentes y con fuente debajo, una fuente no escribe nada.
    auto mar = [](int, int dy, int) -> BlockType {
        return dy > 0 ? BLOCK_AIR : BLOCK_WATER;
    };
    const AguaFlujo::Plan p = AguaFlujo::planear(mar);
    CHECK(p.n == 0);
    CHECK(p.propia == BLOCK_WATER);
}
