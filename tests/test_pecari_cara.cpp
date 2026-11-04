#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include <cmath>

using namespace Fauna;

namespace {
struct Rig {
    const MallaAnimal* malla;
    const std::vector<PesoVertice>* pesos;
    EsqueletoReposo esq;
};
Rig rigAdulto(CacheMallas& cache, int lod) {
    Rig r;
    r.malla = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, lod);
    r.pesos = cache.pesosDe(EspecieAnimal::PECARI_COLLAR, 3, lod);
    r.esq = EsqueletoDePecari(Especies::aplicarEdad(Especies::pecariDeCollar(), 3));
    return r;
}
int contar(const MallaAnimal& m, ZonaCuerpo z) {
    int n = 0;
    for (const VerticeAnimal& v : m.vertices) if (v.zona == z) ++n;
    return n;
}
} // namespace

TEST_CASE("Cara: de cerca hay parpados, boca y mandibula; de lejos no") {
    CacheMallas cache;
    const Rig r = rigAdulto(cache, 0);
    REQUIRE(r.malla);
    REQUIRE(r.pesos);
    CHECK(contar(*r.malla, ZonaCuerpo::PARPADO) > 0);
    CHECK(contar(*r.malla, ZonaCuerpo::BOCA) > 0);

    int mandibula = 0, ojoI = 0, ojoD = 0;
    for (const PesoVertice& w : *r.pesos) {
        if (w.hueso == (uint8_t)HuesoAnimal::MANDIBULA) ++mandibula;
        if (w.hueso == (uint8_t)HuesoAnimal::OJO_I) ++ojoI;
        if (w.hueso == (uint8_t)HuesoAnimal::OJO_D) ++ojoD;
    }
    CHECK(mandibula > 10);
    CHECK(ojoI > 0);
    CHECK(ojoI == ojoD);

    const MallaAnimal* lejos = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 2);
    REQUIRE(lejos);
    CHECK(contar(*lejos, ZonaCuerpo::PARPADO) == 0);
}

TEST_CASE("Cara: el hueso de cada ojo esta en su ojo") {
    CacheMallas cache;
    const Rig r = rigAdulto(cache, 0);
    V3 suma[2] = { V3(0, 0, 0), V3(0, 0, 0) };
    int n[2] = { 0, 0 };
    for (const VerticeAnimal& v : r.malla->vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        const int l = v.pos.x < 0 ? 0 : 1;
        suma[l] = V3(suma[l].x + v.pos.x, suma[l].y + v.pos.y, suma[l].z + v.pos.z);
        ++n[l];
    }
    REQUIRE(n[0] > 0);
    const HuesoAnimal H[2] = { HuesoAnimal::OJO_I, HuesoAnimal::OJO_D };
    for (int l = 0; l < 2; ++l) {
        const V3 c(suma[l].x / n[l], suma[l].y / n[l], suma[l].z / n[l]);
        const V3& piv = r.esq.huesos[(int)H[l]].pivote;
        CHECK(std::fabs(c.y - piv.y) < 0.02f);
        CHECK(std::fabs(c.z - piv.z) < 0.02f);
        CHECK(std::fabs(std::fabs(c.x) - std::fabs(piv.x)) < 0.03f);
    }
}

TEST_CASE("Cara: abrir la boca baja la mandibula y el resto de la cabeza no se mueve") {
    CacheMallas cache;
    const Rig r = rigAdulto(cache, 0);
    PoseEsqueleto pose;
    GestoCara g;
    g.boca = 1.0f;
    AplicarGesto(g, pose);
    TransHueso t[NUM_HUESOS];
    ResolverPose(r.esq, pose, t);
    std::vector<VerticeAnimal> salida;
    DeformarMalla(*r.malla, *r.pesos, t, salida);
    float bajaMand = 0.0f, mueveCabeza = 0.0f;
    for (size_t i = 0; i < salida.size(); ++i) {
        const float dy = salida[i].pos.y - r.malla->vertices[i].pos.y;
        if ((*r.pesos)[i].hueso == (uint8_t)HuesoAnimal::MANDIBULA) {
            if (-dy > bajaMand) bajaMand = -dy;
        } else if (r.malla->vertices[i].zona == ZonaCuerpo::HOCICO) {
            mueveCabeza = std::fmax(mueveCabeza, std::fabs(dy));
        }
    }
    CHECK(bajaMand > 0.01f);       // la boca se abre de verdad
    CHECK(mueveCabeza < 1e-4f);    // el hocico de arriba se queda
}

TEST_CASE("Cara: los parpados se abren hacia arriba y se cierran sobre el ojo") {
    CacheMallas cache;
    const Rig r = rigAdulto(cache, 0);
    TransHueso t[NUM_HUESOS];
    PoseEsqueleto pose;
    ResolverPose(r.esq, pose, t);
    std::vector<VerticeAnimal> abierto, cerrado;
    DeformarMalla(*r.malla, *r.pesos, t, abierto);
    cerrado = abierto;
    GestoCara ga;                          // abiertos
    GestoCara gc; gc.parpadoI = gc.parpadoD = 1.0f;
    AnimarParpados(*r.malla, ga, t, abierto);
    AnimarParpados(*r.malla, gc, t, cerrado);
    float minAbierto = 1e9f, minCerrado = 1e9f;
    for (size_t i = 0; i < abierto.size(); ++i) {
        if (r.malla->vertices[i].zona != ZonaCuerpo::PARPADO) continue;
        minAbierto = std::fmin(minAbierto, abierto[i].pos.y);
        minCerrado = std::fmin(minCerrado, cerrado[i].pos.y);
    }
    // Cerrado baja mas: tapa el ojo.
    CHECK(minCerrado < minAbierto - 0.005f);
}

TEST_CASE("Cara: parpadea cada pocos segundos y el gesto sigue la conducta") {
    int cerrados = 0;
    for (int i = 0; i < 600; ++i)
        if (parpadeo(i * 0.01f, 1234u) > 0.5f) ++cerrados;
    CHECK(cerrados > 0);           // en 6 s parpadea
    CHECK(cerrados < 60);          // pero casi siempre esta abierto

    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 300; ++i) {
            const GestoCara g = GestoDe(c, i * 0.037f, 99u, 0.0f);
            CHECK(g.boca >= 0.0f); CHECK(g.boca <= 1.0f);
            CHECK(g.parpadoI >= 0.0f); CHECK(g.parpadoI <= 1.0f);
        }
    // Defendiendo, castañetea: en algun momento abre mucho la boca.
    float maxDef = 0.0f;
    for (int i = 0; i < 300; ++i) maxDef = std::fmax(maxDef, GestoDe(3, i * 0.01f, 5u, 0.0f).boca);
    CHECK(maxDef > 0.5f);
    CHECK(GestoMuerto().parpadoI > 0.3f);
}
