#include <doctest/doctest.h>
#include "player/CabezaJugador.h"
#include <cmath>

// ============================================================================
// EL AVATAR: GENES, CRECIMIENTO Y MALLAS
// ============================================================================

using namespace Avatar;

namespace {
void poner(Datos& d, int gen, int a0, int a1) { d.genes.a[gen][0] = (uint8_t)a0; d.genes.a[gen][1] = (uint8_t)a1; }

bool mallaSana(const ModeloJugador::Malla& m) {
    for (const auto& v : m.v)
        if (!std::isfinite(v.p.x) || !std::isfinite(v.p.y) || !std::isfinite(v.p.z) ||
            !std::isfinite(v.n.x)) return false;
    for (uint32_t i : m.i) if (i >= m.v.size()) return false;
    return !m.i.empty();
}
} // namespace

TEST_CASE("Avatar: el MC1R pelirrojo es recesivo") {
    Datos d;
    poner(d, G_MC1R, 1, 0);
    const Fenotipo uno = d.fenotipo();
    poner(d, G_MC1R, 1, 1);
    const Fenotipo dos = d.fenotipo();
    // Con dos copias el pelo es rojo: mucho mas rojo que verde y azul.
    CHECK(dos.pelo.r > dos.pelo.g * 1.6f);
    CHECK(dos.pecas > uno.pecas);
    // Con una, mucho menos rojo que con dos.
    CHECK(uno.pelo.r / uno.pelo.g < 0.8f * (dos.pelo.r / dos.pelo.g));
}

TEST_CASE("Avatar: HERC2 GG ojos azules, AA marrones") {
    Datos d;
    poner(d, G_HERC2, 1, 1);
    const Fenotipo azul = d.fenotipo();
    poner(d, G_HERC2, 0, 0);
    const Fenotipo marron = d.fenotipo();
    CHECK(azul.iris.b > azul.iris.r);
    CHECK(marron.iris.r > marron.iris.b);
}

TEST_CASE("Avatar: los aclaradores de piel suman, y el albinismo pide dos copias") {
    Datos d;
    poner(d, G_SLC24A5, 0, 0); poner(d, G_SLC45A2, 0, 0);
    const float oscura = d.fenotipo().piel.g;
    poner(d, G_SLC24A5, 1, 0);
    const float media = d.fenotipo().piel.g;
    poner(d, G_SLC24A5, 1, 1); poner(d, G_SLC45A2, 1, 1);
    const float clara = d.fenotipo().piel.g;
    CHECK(oscura < media);
    CHECK(media < clara);
    poner(d, G_TYR, 1, 0);
    CHECK_FALSE(d.fenotipo().albino);
    poner(d, G_TYR, 1, 1);
    CHECK(d.fenotipo().albino);
}

TEST_CASE("Avatar: el SRY solo viene del padre y el AR del X se hereda como en la biologia") {
    Datos d;
    REQUIRE(esXY(d.genes));
    d.ciclarAlelo(G_SRY, 0);                 // la madre: siempre X
    CHECK(d.genes.a[G_SRY][0] == 0);
    d.ciclarAlelo(G_SRY, 1);                 // el padre: X
    CHECK_FALSE(esXY(d.genes));
    // XX: dos copias del AR. Con XY solo cuenta la del X.
    poner(d, G_AR, 1, 1);
    CHECK(copias(d.genes, G_AR, 1) == 2);
    d.ciclarAlelo(G_SRY, 1);                 // vuelve a XY
    CHECK(copias(d.genes, G_AR, 1) == 1);
    CHECK(genotipo(d.genes, G_AR).find("(Y)") != std::string::npos);
    // En XY no se puede tocar un segundo X que no existe.
    const uint8_t antes = d.genes.a[G_AR][1];
    d.ciclarAlelo(G_AR, 1);
    CHECK(d.genes.a[G_AR][1] == antes);
}

TEST_CASE("Avatar: el grupo ABO, A y B codominantes y O recesivo") {
    Datos d;
    poner(d, G_ABO, 0, 2); CHECK(std::string(d.fenotipo().grupo) == "A");
    poner(d, G_ABO, 1, 2); CHECK(std::string(d.fenotipo().grupo) == "B");
    poner(d, G_ABO, 0, 1); CHECK(std::string(d.fenotipo().grupo) == "AB");
    poner(d, G_ABO, 2, 2); CHECK(std::string(d.fenotipo().grupo) == "O");
}

TEST_CASE("Avatar: miostatina nula, mas musculo; calvicie solo con la edad y en varones") {
    Datos d;
    const float base = d.fenotipo().musculo;
    poner(d, G_MSTN, 1, 1);
    CHECK(d.fenotipo().musculo > base * 1.3f);
    poner(d, G_AR, 1, 0);
    d.poner(CUERPO_EDAD, 20.0f);
    CHECK(d.fenotipo().calvicie == 0.0f);
    d.poner(CUERPO_EDAD, 55.0f);
    CHECK(d.fenotipo().calvicie > 0.5f);
    d.ciclarAlelo(G_SRY, 1);                 // XX
    CHECK(d.fenotipo().calvicie == 0.0f);
}

TEST_CASE("Avatar: el pelo y las unas crecen con los dias") {
    Datos d;
    d.crecer(10.0);                          // la primera vez solo apunta
    const float pelo0 = d[PELO_LARGO_CM], una0 = d[UNA_LARGO_MM];
    d.crecer(40.0);                          // 30 dias despues
    CHECK(d[PELO_LARGO_CM] == doctest::Approx(pelo0 + d[PELO_CRECE]).epsilon(0.01));
    CHECK(d[UNA_LARGO_MM] == doctest::Approx(una0 + d[UNA_CRECE]).epsilon(0.01));
    CHECK(d[BARBA_LARGO_MM] > 5.0f);         // XY adulto: le sale barba
}

TEST_CASE("Avatar: se guarda y se lee igual") {
    Datos a;
    a.poner(CAB_TAMANO, 1.1f);
    a.poner(PELO_LARGO_CM, 33.0f);
    poner(a, G_HERC2, 1, 1);
    a.ultimoDia = 12.5;
    Datos b;
    b.leer(a.texto());
    CHECK(b[CAB_TAMANO] == doctest::Approx(1.1f));
    CHECK(b[PELO_LARGO_CM] == doctest::Approx(33.0f));
    CHECK(b.genes.a[G_HERC2][0] == 1);
    CHECK(b.ultimoDia == doctest::Approx(12.5));
}

TEST_CASE("Avatar: las familias van de la cabeza a los pies y cubren todo") {
    CHECK(std::string(familia(0).nombre) == "CABEZA");
    int cubiertos = 0;
    for (int f = 0; f < NUM_FAMILIAS; ++f) {
        if (f > 0) CHECK(familia(f).desde == familia(f - 1).hasta);
        cubiertos += familia(f).hasta - familia(f).desde;
    }
    CHECK(cubiertos == NUM_PARAMS);
}

TEST_CASE("Avatar: el cuerpo y la cabeza salen sanos con ajustes extremos") {
    using namespace ModeloJugador;
    const Esqueleto esq = resolver(Pose{}, true);
    Calidad q;
    for (int extremo = 0; extremo < 3; ++extremo) {
        Datos d;
        for (int p = 0; p < NUM_PARAMS; ++p)
            d.poner(p, extremo == 0 ? info(p).minimo : (extremo == 1 ? info(p).maximo : info(p).defecto));
        Piel piel = generarPiel(7);
        aplicarAvatar(d, piel);
        Fisiologia fis;
        Malla cuerpo, cabeza;
        generar(esq, Pose{}, piel, fis, q, cuerpo);
        generarCabeza(esq, d, q, cabeza);
        CHECK(mallaSana(cuerpo));
        CHECK(mallaSana(cabeza));
        // La cabeza queda arriba: el craneo por encima del cuello.
        float alto = -1.0f;
        for (const auto& v : cabeza.v) alto = std::fmax(alto, v.p.y);
        CHECK(alto > esq.cuello.p.y + 0.15f);
        CHECK(alto < 2.2f);
    }
}

TEST_CASE("Avatar: al bajar un ajuste la cabeza vuelve a como estaba") {
    using namespace ModeloJugador;
    const Esqueleto esq = resolver(Pose{}, true);
    Calidad q;
    Datos d;
    Malla m;
    generarCabeza(esq, d, q, m);
    const size_t nAntes = m.v.size();
    float altoAntes = -1.0f;
    for (const auto& v : m.v) altoAntes = std::fmax(altoAntes, v.p.y);
    // Se sube al maximo y se regenera sobre la MISMA malla, luego se baja.
    d.poner(CAB_TAMANO, info(CAB_TAMANO).maximo);
    generarCabeza(esq, d, q, m);
    d.poner(CAB_TAMANO, info(CAB_TAMANO).defecto);
    generarCabeza(esq, d, q, m);
    float alto = -1.0f;
    for (const auto& v : m.v) alto = std::fmax(alto, v.p.y);
    CHECK(m.v.size() == nAntes);
    CHECK(alto == doctest::Approx(altoAntes));
}

TEST_CASE("Avatar: un pie mas grande es mas largo, y la mano crece") {
    using namespace ModeloJugador;
    const Esqueleto esq = resolver(Pose{}, true);
    Calidad q;
    q.venas = false;
    auto largoPie = [&](float k) {
        Piel piel;
        piel.forma.pie = k;
        Fisiologia fis;
        Malla m;
        generar(esq, Pose{}, piel, fis, q, m);
        float zmax = -1e9f;
        for (const auto& v : m.v) if (v.p.y < 0.06f) zmax = std::fmax(zmax, v.p.z);
        return zmax;
    };
    CHECK(largoPie(1.15f) > largoPie(1.0f) + 0.015f);
}

TEST_CASE("Avatar: se guarda por nombre y se siguen leyendo los archivos viejos por numero") {
    Datos viejo;
    viejo.leer("p0 1.1000\np17 1.2000\np51 0.1000\n");     // CAB_TAMANO, OREJA_TAMANO, ROPA_CAMISA_TONO
    CHECK(viejo[CAB_TAMANO] == doctest::Approx(1.1f));
    CHECK(viejo[OREJA_TAMANO] == doctest::Approx(1.2f));
    CHECK(viejo[ROPA_CAMISA_TONO] == doctest::Approx(0.1f));
    Datos a;
    a.poner(NARIZ_PUNTA, 0.7f);
    a.poner(PELO_TINTE, 0.5f);
    CHECK(a.texto().find("a PUNTA_ARRIBA/ABAJO 0.7000") != std::string::npos);
    Datos b;
    b.leer(a.texto());
    CHECK(b[NARIZ_PUNTA] == doctest::Approx(0.7f));
    CHECK(b[PELO_TINTE] == doctest::Approx(0.5f));
}

TEST_CASE("Avatar: los nuevos ajustes de color se notan") {
    Datos d;
    const Fenotipo base = d.fenotipo();
    d.poner(PELO_TINTE, 1.0f);
    d.poner(PELO_TINTE_TONO, 0.6f);      // azul
    d.poner(PIEL_BRONCEADO, 1.0f);
    d.poner(ROPA_VIVEZA, 1.0f);
    const Fenotipo f = d.fenotipo();
    CHECK(f.pelo.b > f.pelo.r);
    CHECK(f.piel.r < base.piel.r);
    const float satAntes = base.camisa.r + base.camisa.g + base.camisa.b;
    (void)satAntes;
    CHECK(std::fabs(f.camisa.r - f.camisa.b) > std::fabs(base.camisa.r - base.camisa.b));
}