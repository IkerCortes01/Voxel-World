#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include <cmath>

using namespace Fauna;

// ============================================================================
// FASES 4 Y 5: MUSCULOS, PANZA Y PELO
// ============================================================================

TEST_CASE("Musculos: la panza se queda atras al arrancar y vuelve a su sitio") {
    ResortePanza r;
    r.actualizar(1.0f / 60.0f, 0.0f);
    float maximo = 0.0f;
    for (int i = 0; i < 30; ++i) {           // arranca a 3 bloques/s
        r.actualizar(1.0f / 60.0f, 3.0f);
        maximo = std::fmax(maximo, std::fabs(r.x));
    }
    CHECK(maximo > 0.01f);
    for (int i = 0; i < 600; ++i) r.actualizar(1.0f / 60.0f, 3.0f);
    CHECK(std::fabs(r.x) < 0.01f);           // a velocidad constante se calma
    // Estable aunque la fauna vaya a 20 Hz.
    ResortePanza lento;
    for (int i = 0; i < 200; ++i) lento.actualizar(0.05f, (i % 20 < 10) ? 4.0f : 0.0f);
    CHECK(std::isfinite(lento.x));
    CHECK(std::fabs(lento.x) <= 1.0f);
}

TEST_CASE("Musculos: respirar mueve el torso un poco y nada se rompe") {
    CacheMallas cache;
    const MallaAnimal* m = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 0);
    const auto* pesos = cache.pesosDe(EspecieAnimal::PECARI_COLLAR, 3, 0);
    REQUIRE(m); REQUIRE(pesos);
    const EsqueletoReposo esq =
        EsqueletoDePecari(Especies::aplicarEdad(Especies::pecariDeCollar(), 3));
    PoseEsqueleto pose;
    pose.giroX[(int)HuesoAnimal::HOMBRO_DI] = 0.6f;
    TransHueso t[NUM_HUESOS];
    ResolverPose(esq, pose, t);
    std::vector<VerticeAnimal> base, vivo;
    DeformarMalla(*m, *pesos, t, base);
    vivo = base;
    DinamicaCuerpo d;
    d.tiempo = 0.55f;
    d.panza = 0.8f;
    d.rapidez = 2.0f;
    d.fasePaso = 0.7f;
    AnimarMusculos(*m, *pesos, pose, t, d, vivo);
    float maxTorso = 0.0f, maxCara = 0.0f;
    for (size_t i = 0; i < vivo.size(); ++i) {
        const V3& a = base[i].pos; const V3& b = vivo[i].pos;
        REQUIRE(std::isfinite(b.x)); REQUIRE(std::isfinite(b.y));
        const float dd = V3(a.x - b.x, a.y - b.y, a.z - b.z).longitud();
        if (esTroncoBlando(m->vertices[i].zona)) maxTorso = std::fmax(maxTorso, dd);
        if (m->vertices[i].zona == ZonaCuerpo::OJO) maxCara = std::fmax(maxCara, dd);
    }
    CHECK(maxTorso > 0.002f);     // se nota
    CHECK(maxTorso < 0.04f);      // sin deformarlo
    CHECK(maxCara == doctest::Approx(0.0f));   // el ojo no respira
}

TEST_CASE("Pelo: cerdas de cerca, con tope, y que reaccionan") {
    CacheMallas cache;
    const MallaAnimal* m = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 0);
    REQUIRE(m);
    const std::vector<Cerda>& cerdas = CerdasDe(*m);
    CHECK(cerdas.size() > 50);
    CHECK(cerdas.size() <= 260);

    // Una cerda de la crin: erizada se levanta hacia la normal.
    const Cerda* crin = nullptr;
    for (const Cerda& c : cerdas) if (c.crin) { crin = &c; break; }
    REQUIRE(crin);
    const VerticeAnimal& b = m->vertices[crin->vert];
    EntornoPelo calma, alarma;
    alarma.erizado = 1.0f;
    const auto alineada = [&](const V3& p) {
        const V3 d(p.x - b.pos.x, p.y - b.pos.y, p.z - b.pos.z);
        return (d.x * b.normal.x + d.y * b.normal.y + d.z * b.normal.z) / crin->largo;
    };
    CHECK(alineada(PuntaCerda(*crin, b, alarma)) > alineada(PuntaCerda(*crin, b, calma)) + 0.2f);

    // Al correr se tumban hacia atras (-Z).
    EntornoPelo corre;
    corre.rapidez = 4.0f;
    CHECK(PuntaCerda(*crin, b, corre).z < PuntaCerda(*crin, b, calma).z);

    // El largo se respeta.
    const V3 p = PuntaCerda(*crin, b, calma);
    CHECK(V3(p.x - b.pos.x, p.y - b.pos.y, p.z - b.pos.z).longitud() ==
          doctest::Approx(crin->largo).epsilon(0.01));
}
