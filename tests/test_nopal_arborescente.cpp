#include <doctest/doctest.h>
#include "render/NopalArborescente.h"

#include <cmath>
#include <set>

using namespace Render::Nopal;

// ============================================================================
// LA MALLA DEL NOPAL DE CASTILLA
// ============================================================================
// Un nopal no es una roseta como el agave: es un arbusto ARBORESCENTE donde
// cada penca brota del borde superior de otra. Eso obliga a recursion, y la
// recursion es justo donde es facil colarse -- una planta plana, una hija
// despegada del padre, o un arbol que crece sin limite.
//
// Estos tests fijan las propiedades que de verdad importan, no los numeros
// concretos: la malla es procedural y sus vertices cambiaran al retocarla.

namespace {

// Caja envolvente de la malla, en bloques.
struct Caja {
    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;
    float minZ = 1e9f, maxZ = -1e9f;

    float ancho() const { return maxX - minX; }
    float alto()  const { return maxY - minY; }
    float fondo() const { return maxZ - minZ; }
};

Caja envolvente(const MallaNopal& m) {
    Caja c;
    for (size_t i = 0; i + 2 < m.posiciones.size(); i += 3) {
        const float x = m.posiciones[i];
        const float y = m.posiciones[i + 1];
        const float z = m.posiciones[i + 2];
        if (x < c.minX) c.minX = x;   if (x > c.maxX) c.maxX = x;
        if (y < c.minY) c.minY = y;   if (y > c.maxY) c.maxY = y;
        if (z < c.minZ) c.minZ = z;   if (z > c.maxZ) c.maxZ = z;
    }
    return c;
}

MallaNopal generarCon(uint32_t semilla) {
    Config cfg;
    cfg.semilla = semilla;
    MallaNopal m;
    GeneradorNopal::generar(m, cfg);
    return m;
}

} // namespace

// ----------------------------------------------------------------------------
// VALIDEZ DE LA MALLA
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: genera geometria") {
    const MallaNopal m = generarCon(1234);

    CHECK(m.vertices() > 0);
    CHECK(m.vertices() % 3 == 0);        // triangulos completos
}

TEST_CASE("Nopal: los tres arrays van sincronizados") {
    // posiciones, colores y uvs son paralelos: 3, 4 y 2 floats por vertice.
    // Si se descuadraran, el motor leeria el color de un vertice con la
    // posicion de otro y la planta saldria con los colores corridos.
    const MallaNopal m = generarCon(77);
    const size_t n = m.vertices();

    CHECK(m.posiciones.size() == n * 3);
    CHECK(m.colores.size()    == n * 4);
    CHECK(m.uvs.size()        == n * 2);
}

TEST_CASE("Nopal: nada es NaN ni infinito") {
    // Una rotacion mal construida produce NaN, y un solo NaN en el buffer
    // hace desaparecer el objeto entero al dibujarlo.
    const MallaNopal m = generarCon(4242);

    for (float v : m.posiciones) CHECK(std::isfinite(v));
    for (float v : m.colores)    CHECK(std::isfinite(v));
    for (float v : m.uvs)        CHECK(std::isfinite(v));
}

TEST_CASE("Nopal: los colores estan en rango") {
    const MallaNopal m = generarCon(9);
    for (float v : m.colores) {
        CHECK(v >= 0.0f);
        CHECK(v <= 1.0f);
    }
}

TEST_CASE("Nopal: las UV caen dentro del atlas") {
    // Salirse de 0..1 haria que la penca muestrease la textura del bloque de
    // al lado en el atlas.
    const MallaNopal m = generarCon(555);
    for (float v : m.uvs) {
        CHECK(v >= -0.001f);
        CHECK(v <=  1.001f);
    }
}

// ----------------------------------------------------------------------------
// LA PENCA TIENE VOLUMEN
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: la penca NO es un plano") {
    // ⭐ SE PIDIO GROSOR VOLUMETRICO REAL, de 2 a 4 px.
    //
    // Una sola penca vertical tiene que ocupar profundidad en Z. Si fuera un
    // plano, `fondo` seria cero y la planta desapareceria al mirarla de canto.
    Config cfg;
    cfg.generaciones = 1;       // solo la penca raiz
    cfg.semilla = 1;

    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    const Caja c = envolvente(m);
    const float grosorEsperado = Px::aBloques(cfg.grosorPx);

    INFO("fondo = ", c.fondo(), "  esperado = ", grosorEsperado);
    CHECK(c.fondo() == doctest::Approx(grosorEsperado).epsilon(0.02));

    // Y dentro del rango pedido: 2 a 4 px.
    CHECK(cfg.grosorPx >= 2.0f);
    CHECK(cfg.grosorPx <= 4.0f);
}

TEST_CASE("Nopal: la penca es mas larga que ancha") {
    // Un cladodio es un ovalo de pie, no un circulo.
    Config cfg;
    cfg.generaciones = 1;
    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    const Caja c = envolvente(m);
    INFO("alto = ", c.alto(), "  ancho = ", c.ancho());
    CHECK(c.alto() > c.ancho());
}

TEST_CASE("Nopal: el perfil NO es un rectangulo") {
    // ⭐ SE PIDIO UN POLIGONO OVALADO, no un quad crudo.
    //
    // En un rectangulo solo hay 4 valores distintos de X. En un octagono
    // escalado hay mas, porque cada vertice esta a un angulo distinto.
    Config cfg;
    cfg.generaciones = 1;
    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    std::set<int> xs;   // redondeadas a milesimas
    for (size_t i = 0; i + 2 < m.posiciones.size(); i += 3)
        xs.insert((int)std::lround(m.posiciones[i] * 1000.0f));

    INFO("valores distintos de X = ", xs.size());
    CHECK(xs.size() > 4);
}

// ----------------------------------------------------------------------------
// LA RAMIFICACION
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: mas generaciones dan mas geometria") {
    // La comprobacion de que la recursion de verdad ramifica.
    Config uno;  uno.generaciones = 1;  uno.semilla = 7;
    Config tres; tres.generaciones = 3; tres.semilla = 7;

    MallaNopal m1, m3;
    GeneradorNopal::generar(m1, uno);
    GeneradorNopal::generar(m3, tres);

    INFO("1 generacion = ", m1.vertices(), "  3 generaciones = ", m3.vertices());
    CHECK(m3.vertices() > m1.vertices());
}

TEST_CASE("Nopal: la planta NO es plana") {
    // ⭐⭐ EL PUNTO DEL GIRO EN Y (yaw).
    //
    // Sin el, todas las hijas abririan en el mismo plano y la planta seria una
    // lamina vertical. Es el error clasico de un generador de ramas: se ve
    // bien de frente y desaparece de perfil.
    //
    // Con volumen 3D, la caja envolvente tiene profundidad de VARIAS pencas,
    // no del grosor de una.
    Config cfg;
    cfg.generaciones = 3;
    cfg.semilla = 31337;

    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    const Caja c = envolvente(m);
    const float grosorUnaPenca = Px::aBloques(cfg.grosorPx);

    INFO("fondo de la planta = ", c.fondo(),
         "  grosor de una penca = ", grosorUnaPenca);
    CHECK(c.fondo() > grosorUnaPenca * 2.5f);
}

TEST_CASE("Nopal: crece HACIA ARRIBA desde el origen") {
    // La planta nace en (0,0,0) y sube. Nada puede quedar muy por debajo del
    // origen, o se hundiria en el suelo.
    const MallaNopal m = generarCon(2024);
    const Caja c = envolvente(m);

    INFO("minY = ", c.minY, "  maxY = ", c.maxY);
    CHECK(c.minY > -0.15f);     // algo de margen por el hundido de las tunas
    CHECK(c.maxY > 0.5f);       // y sube de verdad
}

TEST_CASE("Nopal: cabe en el volumen declarado (2x3x2 m)") {
    // ⭐ EL LIMITE QUE PROTEGE EL RENDERIZADO ENTRE CHUNKS.
    //
    // La planta se dibuja entera desde su celda origen, asi que su tamano
    // tiene que ser acotado y conocido. Si creciera sin limite, un nopal
    // podria asomar en un chunk que no sabe nada de el.
    for (uint32_t s = 1; s <= 40; ++s) {
        const MallaNopal m = generarCon(s * 7919u);
        const Caja c = envolvente(m);

        INFO("semilla ", s, ": ", c.ancho(), " x ", c.alto(), " x ", c.fondo());
        CHECK(c.ancho() <= 2.0f);
        CHECK(c.alto()  <= 3.0f);
        CHECK(c.fondo() <= 2.0f);
    }
}

TEST_CASE("Nopal: la recursion termina") {
    // Con 3 hijas por nivel y 3 generaciones el peor caso son 1+3+9 = 13
    // pencas. Un tope generoso que solo salta si la recursion se desbocara.
    Config cfg;
    cfg.generaciones = 3;
    cfg.hijasMin = 3;
    cfg.hijasMax = 3;            // el maximo en cada nivel
    cfg.semilla = 99;

    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    INFO("vertices con ramificacion maxima = ", m.vertices());
    CHECK(m.vertices() < 20000);
}

// ----------------------------------------------------------------------------
// DETERMINISMO
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: la misma semilla da la misma planta") {
    // ⭐ IMPRESCINDIBLE, y no es un detalle.
    //
    // La planta se regenera cada vez que su chunk se vuelve a mallar. Si
    // dependiera de rand() global, cambiaria de forma al recargar el mundo --
    // y peor: dos hilos de mallado darian resultados distintos para el mismo
    // nopal.
    const MallaNopal a = generarCon(12345);
    const MallaNopal b = generarCon(12345);

    REQUIRE(a.posiciones.size() == b.posiciones.size());
    for (size_t i = 0; i < a.posiciones.size(); ++i)
        CHECK(a.posiciones[i] == doctest::Approx(b.posiciones[i]));
}

TEST_CASE("Nopal: semillas distintas dan plantas distintas") {
    // Si no, un nopalera entera saldria clonada.
    const MallaNopal a = generarCon(1);
    const MallaNopal b = generarCon(2);

    bool alguna = (a.posiciones.size() != b.posiciones.size());
    if (!alguna) {
        for (size_t i = 0; i < a.posiciones.size(); ++i)
            if (std::fabs(a.posiciones[i] - b.posiciones[i]) > 1e-4f) {
                alguna = true;
                break;
            }
    }
    CHECK(alguna);
}

// ----------------------------------------------------------------------------
// EL MAPEO UV DE DOS ZONAS
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: el canto usa una FRANJA, no el rectangulo de la cara") {
    // ⭐⭐ LA RAZON TECNICA, Y ES LA QUE OBLIGA.
    //
    // El canto mide 3 px de grosor. Con el rectangulo de la cara --pensado
    // para 13x9 px-- la textura se comprimiria a un tercio de pixel de alto:
    // el filtrado mezclaria filas enteras y saldria una banda de color
    // indefinido, distinta en cada penca segun su angulo.
    //
    // Se comprueba que la franja declarada es efectivamente estrecha.
    Config cfg;
    const float altoFranja = cfg.uvCantoV1 - cfg.uvCantoV0;
    const float altoCara   = cfg.uvPencaV1 - cfg.uvPencaV0;

    INFO("franja = ", altoFranja, "  cara = ", altoCara);
    CHECK(altoFranja < altoCara * 0.2f);
    CHECK(altoFranja > 0.0f);
}

TEST_CASE("Nopal: el canto es mas oscuro que la cara") {
    // Un filo iluminado igual que la cara ancha aplana la silueta: es lo que
    // hace que una penca se lea como volumen y no como una calcomania.
    Config cfg;
    const float lumCara  = cfg.verdeCara[0]  * 0.3f + cfg.verdeCara[1]  * 0.6f
                         + cfg.verdeCara[2]  * 0.1f;
    const float lumCanto = cfg.verdeCanto[0] * 0.3f + cfg.verdeCanto[1] * 0.6f
                         + cfg.verdeCanto[2] * 0.1f;

    INFO("cara = ", lumCara, "  canto = ", lumCanto);
    CHECK(lumCanto < lumCara);
}

// ----------------------------------------------------------------------------
// LAS TUNAS
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: las tunas son rojas o naranjas, nunca verdes") {
    // Se buscan vertices cuyo rojo domine claramente al verde. Si no hubiera
    // ninguno en varias semillas, el fruto no se estaria emitiendo.
    int conFruto = 0;

    for (uint32_t s = 1; s <= 30; ++s) {
        const MallaNopal m = generarCon(s * 104729u);
        for (size_t i = 0; i + 3 < m.colores.size(); i += 4) {
            const float r = m.colores[i], g = m.colores[i + 1];
            if (r > g * 1.3f) { ++conFruto; break; }
        }
    }

    INFO("plantas con fruto visible: ", conFruto, " de 30");
    CHECK(conFruto > 0);
}

TEST_CASE("Nopal: sin tunas la malla sigue siendo valida") {
    // `tunasMax = 0` es un caso real: un nopal fuera de temporada. No puede
    // dejar triangulos a medias ni arrays descuadrados.
    Config cfg;
    cfg.tunasMax = 0;
    cfg.semilla = 808;

    MallaNopal m;
    GeneradorNopal::generar(m, cfg);

    CHECK(m.vertices() > 0);
    CHECK(m.vertices() % 3 == 0);
    CHECK(m.colores.size() == m.vertices() * 4);
}
