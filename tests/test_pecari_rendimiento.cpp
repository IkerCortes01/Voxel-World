#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include <vector>

// ============================================================================
// LA FAUNA OPTIMIZADA DA LO MISMO QUE LA DE ANTES
// ============================================================================
// PEDIDO: "haz que se optimice mas todo".
//
// La actualizacion agrupaba por manada recorriendo la lista entera una vez por
// manada y otra por animal (O(n^2)). Ahora se agrupa una vez y cada animal
// solo mira a su manada. Eso solo vale si el resultado es IDENTICO: estos
// tests comparan las dos versiones con las manadas mezcladas en la lista.

using namespace Fauna;

namespace {

// Animales de cinco manadas (y solitarios, idManada -1), intercalados en la
// lista como quedan tras nacer, morir y descargarse.
std::vector<PecariAgente> mezcla() {
    std::vector<PecariAgente> v;
    uint32_t s = 12345u;
    for (int i = 0; i < 120; ++i) {
        PecariAgente p;
        p.id = i;
        s = s * 1664525u + 1013904223u;
        p.idManada = (int)(s % 6u) - 1;          // -1..4
        s = s * 1664525u + 1013904223u;
        p.x = (float)(s % 4000u) * 0.01f;
        s = s * 1664525u + 1013904223u;
        p.z = (float)(s % 4000u) * 0.01f;
        // Algunos en el mismo sitio: los empates tienen que salir igual.
        if (i % 17 == 0 && i > 0) { p.x = v.back().x; p.z = v.back().z; }
        v.push_back(p);
    }
    return v;
}

// Los miembros de una manada, en orden de indice creciente.
std::vector<int> miembrosDe(const std::vector<PecariAgente>& v, int idManada) {
    std::vector<int> m;
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[(size_t)i].idManada == idManada) m.push_back(i);
    return m;
}

} // namespace

TEST_CASE("Rendimiento fauna: los vecinos por manada son los mismos y en el mismo orden") {
    const std::vector<PecariAgente> v = mezcla();
    std::vector<const PecariAgente*> antes, ahora;
    for (size_t i = 0; i < v.size(); ++i) {
        const std::vector<int> m = miembrosDe(v, v[i].idManada);
        BuscarVecinos(v, i, antes);
        BuscarVecinosEnManada(v, m.data(), (int)m.size(), i, ahora);
        REQUIRE(antes.size() == ahora.size());
        for (size_t k = 0; k < antes.size(); ++k) CHECK(antes[k] == ahora[k]);
    }
}

TEST_CASE("Rendimiento fauna: el centro de manada es el mismo, bit a bit") {
    const std::vector<PecariAgente> v = mezcla();
    for (int id = -1; id <= 4; ++id) {
        const std::vector<int> m = miembrosDe(v, id);
        const CentroManada a = CalcularCentro(v, id);
        const CentroManada b = CalcularCentroDeMiembros(v, m.data(), (int)m.size());
        CHECK(a.miembros == b.miembros);
        CHECK(a.x == b.x);     // igualdad exacta: misma suma, mismo orden
        CHECK(a.z == b.z);
    }
}

TEST_CASE("Rendimiento fauna: una manada vacia no rompe nada") {
    const std::vector<PecariAgente> v = mezcla();
    std::vector<const PecariAgente*> salida;
    const int unico = 0;
    BuscarVecinosEnManada(v, &unico, 1, 0, salida);
    CHECK(salida.empty());
    const CentroManada c = CalcularCentroDeMiembros(v, nullptr, 0);
    CHECK(c.miembros == 0);
}

TEST_CASE("Rendimiento fauna: un mundo poblado se actualiza sin perder animales") {
    // De punta a punta por la ruta real: muchas manadas cargadas a la vez y
    // varios segundos de simulacion. Nadie desaparece ni sale disparado.
    struct Llano : IPecariMundo {
        float alturaSuelo(int, int) const override { return 64.0f; }
        BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
        float pendienteEn(int, int) const override { return 0.0f; }
        bool esSolido(int, int y, int) const override { return y < 64; }
    } suelo;
    MundoPecaries mundo(777);
    for (int cx = 0; cx < 24; ++cx)
        for (int cz = 0; cz < 24; ++cz)
            mundo.alCargarChunk(cx, cz, suelo, 500.0f, 0.0);
    const size_t n = mundo.cuantos();
    REQUIRE(n > 20);
    for (int i = 0; i < 300; ++i) mundo.actualizar(1.0f / 60.0f, suelo);
    CHECK(mundo.cuantos() == n);
    for (const PecariAgente& p : mundo.todos()) {
        CHECK(std::isfinite(p.x));
        CHECK(std::isfinite(p.z));
        CHECK(p.y >= 63.0f);
    }
}
