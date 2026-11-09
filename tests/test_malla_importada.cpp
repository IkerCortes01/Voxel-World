#include <doctest/doctest.h>
#include "player/MallaImportada.h"
#include <cmath>
#include <string>

// ============================================================================
// EL MODELO IMPORTADO (resourcepacks/Modelos/jugador.vwm) CON EL ESQUELETO
// ============================================================================

using namespace ModeloJugador;

namespace {
std::string rutaModelo() {
#ifdef RAIZ_PROYECTO
    return std::string(RAIZ_PROYECTO) + "/resourcepacks/Modelos/jugador.vwm";
#else
    return "resourcepacks/Modelos/jugador.vwm";
#endif
}
} // namespace

TEST_CASE("Modelo importado: se carga y es ligero para la HD 4000") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    CHECK(mi.base.size() < 20000);
    CHECK(mi.indices.size() / 3 < 40000);
    CHECK(mi.indicesSinCabeza.size() < mi.indices.size());
}

TEST_CASE("Modelo importado: en su pose de referencia no se mueve nada") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Malla m;
    mi.deformar(resolver(mi.poseReferencia(), true), m);
    float peor = 0.0f;
    for (size_t i = 0; i < m.v.size(); ++i) peor = std::fmax(peor, largo(resta(m.v[i].p, mi.base[i].p)));
    CHECK(peor < 1e-4f);
}

TEST_CASE("Modelo importado: con los brazos abajo las manos van con el esqueleto") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Pose p;
    p.prop = mi.proporciones();
    limitar(p);
    const Esqueleto s = resolver(p, true);
    Malla m;
    mi.deformar(s, m);
    // Ningun vertice se queda en la cruz: todo queda cerca del cuerpo.
    float ancho = 0.0f, alto = 0.0f;
    for (const auto& v : m.v) { ancho = std::fmax(ancho, std::fabs(v.p.x)); alto = std::fmax(alto, v.p.y); }
    CHECK(ancho < 0.40f);
    CHECK(alto < 1.85f);
    // Y la mano de la malla esta donde la del esqueleto (en la cruz estaba a
    // mas de medio metro; la mano del modelo no mide justo lo mismo que la
    // del esqueleto, de ahi los centimetros de margen).
    float cerca = 1e9f;
    for (const auto& v : m.v) cerca = std::fmin(cerca, largo(resta(v.p, s.mano[1].dedo[1][2].p)));
    CHECK(cerca < 0.07f);
}
