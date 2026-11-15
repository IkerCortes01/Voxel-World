// Los items dejados en el suelo (src/ItemsTirados.h): cuerpos con fisica.
// PEDIDO (1): "que solo de el item tirado en 3D [...] y que si colocas mas y
// mas en el mismo lugar se acumulan y tienen fisica y gravedad".
// PEDIDO (2): "que el maguey y nopal de Castilla sueltos tengan colision,
// reaccionen a paredes, techo, piso y bloques y al jugador y entidades".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "ItemsTirados.h"

#include <fstream>
#include <iterator>
#include <string>

using namespace ItemsTirados;

namespace {
// Un mundo de prueba: suelo macizo por debajo de y = 10, un escalon de medio
// bloque en x >= 5 (celda y = 10), una PARED en x >= 8 (hasta y = 14) y un
// TECHO en y = 12 sobre la franja z >= 20.
float mundo(int x, int y, int z) {
    if (y < 10) return 1.0f;
    if (x >= 8 && y < 14) return 1.0f;
    if (z >= 20 && y == 12) return 1.0f;
    if (x >= 5 && x < 8 && y == 10) return 0.5f;
    return 0.0f;
}
void dejarQuieto(std::vector<Cuerpo>& c, float segundos = 3.0f, const Empujador* e = nullptr, int n = 0) {
    for (int i = 0; i < (int)(segundos * 60.0f); ++i) paso(c, 1.0f / 60.0f, mundo, e, n);
}
Cuerpo en(float x, float y, float z) { Cuerpo c; c.x = x; c.y = y; c.z = z; return c; }
std::string motor() {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Items tirados: uno suelto en el aire cae y se posa en el suelo") {
    std::vector<Cuerpo> c = { en(2.5f, 13.0f, 2.5f) };
    paso(c, 1.0f / 60.0f, mundo);
    CHECK_FALSE(c[0].enSuelo);
    CHECK(c[0].y < 13.0f);
    CHECK(c[0].vy < 0.0f);
    dejarQuieto(c);
    CHECK(c[0].enSuelo);
    CHECK(c[0].y == doctest::Approx(10.0f));     // sobre el suelo, ni flotando ni hundido
    CHECK(c[0].vy == 0.0f);
    // Sobre el escalon de medio bloque, a su altura de verdad.
    std::vector<Cuerpo> e = { en(6.5f, 12.0f, 2.5f) };
    dejarQuieto(e);
    CHECK(e[0].y == doctest::Approx(10.5f));
}

TEST_CASE("Items tirados: al caer fuerte rebota un poco y luego se queda") {
    std::vector<Cuerpo> c = { en(2.5f, 16.0f, 2.5f) };
    bool subio = false;
    float antes = c[0].y;
    for (int i = 0; i < 240; ++i) {
        paso(c, 1.0f / 60.0f, mundo);
        if (c[0].y > antes + 1e-4f) subio = true;        // tras tocar el suelo vuelve a subir
        CHECK(c[0].y >= 10.0f - 1e-4f);                  // nunca lo atraviesa
        antes = c[0].y;
    }
    CHECK(subio);
    CHECK(c[0].enSuelo);
    CHECK(c[0].y == doctest::Approx(10.0f));
}

TEST_CASE("Items tirados: lanzado contra una pared no la atraviesa") {
    std::vector<Cuerpo> c = { en(6.0f, 11.5f, 2.5f) };
    c[0].vx = 7.0f;                                      // hacia la pared de x = 8
    float masLejos = 0.0f;
    for (int i = 0; i < 300; ++i) {
        paso(c, 1.0f / 60.0f, mundo);
        masLejos = (std::max)(masLejos, c[0].x);
    }
    CHECK(masLejos + MEDIO <= 8.0f + 1e-3f);             // la caja entera se queda antes
    CHECK(masLejos > 7.3f);                              // pero llego hasta ella
    CHECK(c[0].enSuelo);
    CHECK(c[0].vx == 0.0f);                              // y el roce lo paro
}

TEST_CASE("Items tirados: lanzado hacia arriba bajo un techo, se queda debajo") {
    std::vector<Cuerpo> c = { en(2.5f, 10.0f, 22.5f) };
    c[0].vy = 12.0f;
    float masAlto = 0.0f;
    for (int i = 0; i < 200; ++i) {
        paso(c, 1.0f / 60.0f, mundo);
        masAlto = (std::max)(masAlto, c[0].y + GROSOR);
    }
    CHECK(masAlto <= 12.0f + 1e-3f);                     // el techo esta en y = 12
    CHECK(c[0].y == doctest::Approx(10.0f));             // y vuelve al suelo
    // Sin techo, con el mismo impulso sube mucho mas.
    std::vector<Cuerpo> libre = { en(2.5f, 10.0f, 2.5f) };
    libre[0].vy = 12.0f;
    float alto = 0.0f;
    for (int i = 0; i < 200; ++i) { paso(libre, 1.0f / 60.0f, mundo); alto = (std::max)(alto, libre[0].y); }
    CHECK(alto > 12.5f);
}

TEST_CASE("Items tirados: en el suelo el roce lo frena; resbala un poco y se para") {
    std::vector<Cuerpo> c = { en(2.5f, 10.0f, 2.5f) };
    c[0].enSuelo = true;
    c[0].vz = 3.0f;
    dejarQuieto(c, 2.0f);
    CHECK(c[0].z > 2.6f);
    CHECK(c[0].z < 3.5f);
    CHECK(c[0].vz == 0.0f);
}

TEST_CASE("Items tirados: mas y mas en el mismo sitio se amontonan, uno sobre otro") {
    std::vector<Cuerpo> c;
    for (int i = 0; i < 5; ++i) {
        c.push_back(en(2.5f, 10.0f + GROSOR * (float)i + 0.6f, 2.5f));   // se deja caer encima
        dejarQuieto(c, 2.0f);
    }
    for (int i = 0; i < 5; ++i) {
        CHECK(c[(size_t)i].enSuelo);
        CHECK(c[(size_t)i].y == doctest::Approx(10.0f + GROSOR * (float)i));
    }
    for (size_t a = 0; a < c.size(); ++a)
        for (size_t b = a + 1; b < c.size(); ++b)
            CHECK(std::fabs(c[a].y - c[b].y) >= GROSOR - 1e-4f);        // ninguno dentro de otro
    // Al quitar el de abajo, los de arriba bajan.
    c.erase(c.begin());
    paso(c, 1.0f / 60.0f, mundo);
    CHECK_FALSE(c[0].enSuelo);
    dejarQuieto(c);
    CHECK(c[0].y == doctest::Approx(10.0f));
    CHECK(c[1].y == doctest::Approx(10.0f + GROSOR));
}

TEST_CASE("Items tirados: dos al mismo nivel que se pisan se apartan; lejos, ni se enteran") {
    std::vector<Cuerpo> c = { en(2.50f, 10.0f, 2.5f), en(2.62f, 10.0f, 2.5f) };
    dejarQuieto(c, 4.0f);
    CHECK(c[1].x - c[0].x > 0.40f);                      // ya no estan uno dentro del otro
    CHECK(c[0].y == doctest::Approx(10.0f));
    CHECK(c[1].y == doctest::Approx(10.0f));
    std::vector<Cuerpo> l = { en(2.5f, 10.0f, 2.5f), en(4.0f, 10.0f, 2.5f) };
    dejarQuieto(l, 2.0f);
    CHECK(l[0].x == doctest::Approx(2.5f));
    CHECK(l[1].x == doctest::Approx(4.0f));
}

TEST_CASE("Items tirados: el jugador y los animales los empujan, y no a traves de una pared") {
    // El jugador camina hacia +x contra un item: lo lleva por delante.
    std::vector<Cuerpo> c = { en(3.0f, 10.0f, 2.5f) };
    Empujador j;
    j.z = 2.5f; j.yPies = 10.0f;
    for (int i = 0; i < 180; ++i) {
        j.x = 2.2f + 1.5f * (float)i / 60.0f;            // anda a 1,5 bloques por segundo
        paso(c, 1.0f / 60.0f, mundo, &j, 1);
        CHECK(c[0].x + MEDIO <= 5.0f + 1e-3f);           // el escalon de x = 5 lo para
    }
    CHECK(c[0].x > 3.5f);                                // se movio
    // Uno que esta en otra planta no lo toca.
    std::vector<Cuerpo> alto = { en(3.0f, 10.0f, 2.5f) };
    Empujador arriba;
    arriba.x = 3.0f; arriba.z = 2.5f; arriba.yPies = 13.0f;
    dejarQuieto(alto, 1.0f, &arriba, 1);
    CHECK(alto[0].x == doctest::Approx(3.0f));
    // Un animal bajito (0,6 de alto) tambien empuja.
    std::vector<Cuerpo> a = { en(3.0f, 10.0f, 2.5f) };
    Empujador pecari;
    pecari.x = 2.75f; pecari.z = 2.5f; pecari.yPies = 10.0f; pecari.alto = 0.6f; pecari.radio = 0.4f;
    dejarQuieto(a, 1.0f, &pecari, 1);
    CHECK(a[0].x > 3.2f);
}

TEST_CASE("Items tirados: sin mundo cargado se quedan donde estan; un tiron no los hunde") {
    std::vector<Cuerpo> q = { en(2.5f, 40.0f, 2.5f) };
    q[0].vx = 3.0f;
    for (int i = 0; i < 120; ++i) paso(q, 1.0f / 60.0f, [](int, int, int) { return -1.0f; });
    CHECK(q[0].y == doctest::Approx(40.0f));
    CHECK(q[0].x == doctest::Approx(2.5f));
    std::vector<Cuerpo> t = { en(2.5f, 11.0f, 2.5f) };
    for (int i = 0; i < 60; ++i) paso(t, 1.0f, mundo);   // pasos de un segundo entero
    CHECK(t[0].y == doctest::Approx(10.0f));
}

TEST_CASE("Items tirados: con F se coge el que se mira, el mas cercano, y no el de detras") {
    std::vector<Cuerpo> c = { en(0.0f, 0.0f, 3.0f), en(0.0f, 0.0f, 2.0f), en(2.0f, 0.0f, 2.0f) };
    const float cy = 0.5f * GROSOR;
    CHECK(mirado(c, 0.0f, cy, 0.0f, 0.0f, 0.0f, 1.0f) == 1);
    CHECK(mirado(c, 0.0f, cy, 0.0f, 0.0f, 0.0f, -1.0f) == -1);
    CHECK(mirado(c, 0.0f, cy, 0.0f, 1.0f, 0.0f, 0.0f) == -1);
    CHECK(mirado(c, 0.0f, cy, 0.0f, 0.0f, 0.0f, 1.0f, 1.5f) == -1);
    CHECK(mirado({}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) == -1);
}

TEST_CASE("Items tirados: el nopal y el maguey se dejan como item, no como bloque") {
    for (BlockType t : { BLOCK_NOPAL_FRUTO, BLOCK_NOPAL_CLADODIO, BLOCK_IXTLE_HOJA, BLOCK_PENCA_AGAVE_AZUL })
        CHECK(seTiraAlColocar(t));
    for (BlockType t : { BLOCK_STONE, BLOCK_PLANKS, BLOCK_DIRT, BLOCK_NOPAL_PUESTO, BLOCK_MAGUEY_PUESTO })
        CHECK_FALSE(seTiraAlColocar(t));

    const std::string src = motor();
    REQUIRE_FALSE(src.empty());
    const size_t a = src.find("if (result.hit && seTiraAlColocar(selectedBlock)) {");
    REQUIRE(a != std::string::npos);
    const size_t fin = src.find("if (result.hit) {", a + 10);
    REQUIRE(fin != std::string::npos);
    const std::string rama = src.substr(a, fin - a);
    CHECK(rama.find("state->dejarItem(punto, selectedBlock,") != std::string::npos);
    CHECK(rama.find("consumeSelected()") != std::string::npos);
    CHECK(rama.find("setBlock") == std::string::npos);
    CHECK(rama.find("return;") != std::string::npos);
    CHECK(src.find("if (tirado) { lifetime += deltaTime; return; }") != std::string::npos);
    CHECK(src.find("items_tirados.dat") != std::string::npos);
    CHECK(src.find("if (recogerItemTirado(state)) { ++g_recogidas; return; }") != std::string::npos);
    // La fisica recibe el mundo de verdad, al jugador y a los animales.
    CHECK(src.find("return celdaParaItem(x, y, z); },") != std::string::npos);
    CHECK(src.find("for (const auto& a : pecaries->agentes()) {") != std::string::npos);
    CHECK(src.find("empujan.data(), (int)empujan.size());") != std::string::npos);
}
