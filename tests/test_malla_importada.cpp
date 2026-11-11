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
    CHECK(mi.formato == 2);
    CHECK(mi.base.size() < 60000);
    CHECK(mi.indices.size() / 3 <= 80000);
    CHECK(mi.uv.size() == mi.base.size() * 2);
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

TEST_CASE("Modelo importado: el avatar lo moldea y volver atras lo deja igual") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const std::vector<VerticeJ> original = mi.base;
    const V nariz = mi.pt.nariz;
    AjustesMalla a;
    a.narizLargo = 1.25f;
    a.orejas = 1.2f;
    a.busto = 0.8f;
    mi.aplicarAjustes(a);
    // La nariz crece hacia delante; lo de lejos (los pies) no se mueve.
    float zNariz = -1e9f;
    for (const auto& v : mi.base) if (largo(resta(v.p, nariz)) < 0.03f) zNariz = std::fmax(zNariz, v.p.z);
    CHECK(zNariz > nariz.z + 0.002f);
    for (size_t i = 0; i < original.size(); ++i)
        if (original[i].p.y < 0.3f) REQUIRE(largo(resta(mi.base[i].p, original[i].p)) < 1e-6f);
    // Al volver a los valores por defecto queda como vino.
    mi.aplicarAjustes(AjustesMalla{});
    float peor = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) peor = std::fmax(peor, largo(resta(mi.base[i].p, original[i].p)));
    CHECK(peor < 1e-6f);
}

TEST_CASE("Modelo importado: unos brazos mas gruesos engordan el brazo, no la mano") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Pose p;
    p.prop = mi.proporciones();
    limitar(p);
    const Esqueleto s = resolver(p, true);
    auto anchoBrazo = [&](float k) {
        AjustesMalla a;
        a.brazo = k;
        mi.aplicarAjustes(a);
        Malla m;
        mi.deformar(s, m);
        // La distancia media al eje del brazo derecho de lo que es brazo.
        float suma = 0.0f;
        int n = 0;
        for (size_t i = 0; i < m.v.size(); ++i)
            if (mi.h0[i] == huesoDe(1, HL_HUMERO) && mi.w0[i] > 0.9f) {
                suma += detalle::distSegmento(m.v[i].p, s.hombro[1].p, s.codo[1].p);
                ++n;
            }
        return n ? suma / n : 0.0f;
    };
    CHECK(anchoBrazo(1.3f) > anchoBrazo(1.0f) * 1.2f);
}