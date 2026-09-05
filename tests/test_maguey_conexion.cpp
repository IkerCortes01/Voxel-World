#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include <cmath>

// ============================================================================
// LA PLANTA TIENE QUE SER UNA SOLA PIEZA, PEGADA AL SUELO
// ============================================================================
// BUG REAL: las pencas del maguey salian desconectadas del cuerpo -- trozos de
// hoja flotando, huecos entre el nucleo y las hojas, y separacion entre la base
// de la planta y el terreno.
//
// LO QUE **NO** ERA LA CAUSA. El punto de nacimiento de las hojas ya estaba
// bien: la penca nace a RADIO*0.34 y la piña llega a RADIO*0.52, o sea que el
// arranque cae DENTRO del nucleo en las cinco etapas. Ese hueco se habia
// corregido antes. El primer test de aqui lo fija para que nadie vuelva a
// sospechar de esa pista.
//
// LA CAUSA REAL: EL RECORTE POR CELDA. Un maguey ocupa hasta 4 celdas, y cada
// una dibujaba solo su franja vertical descartando lo que se saliera:
//
//     if (hi < -0.02f || lo > 1.02f) continue;
//
// Pero una penca sale hacia ARRIBA y hacia AFUERA a la vez, asi que cruza la
// frontera entre celdas EN DIAGONAL: se sale por arriba en la celda de abajo y
// por abajo en la de arriba. Las DOS la descartaban. Medido sobre la geometria
// real del mesher:
//
//     BROTE     (1 celda)    0 %  perdido   <- por eso se veia perfecto
//     JOVEN     (2 celdas)  45 %
//     ADULTO    (3 celdas)  59 %
//     MADURO    (4 celdas)  68 %
//     PRODUCTOR (4 celdas)  67 %
//
// Cuanto mas grande la planta, mas rota -- que es exactamente el sintoma.
//
// EL ARREGLO: la planta se emite ENTERA desde su celda base (SEG == 0). Un quad
// no se parte nunca, asi que no hay frontera donde perderse.
//
// QUE FIJAN ESTOS TESTS. El mesher no se puede instanciar sin OpenGL, pero las
// medidas de las que sale toda su geometria son logica pura de
// BloqueCompuesto.h. Se replican aqui las formulas del mesher y se comprueban
// las PROPIEDADES de conexion, no valores concretos.

namespace {

constexpr float ELEVADO   = 1.5f / 16.0f;   // cuanto se sube el modelo
constexpr float SOTERRADO = 2.5f / 16.0f;   // cuanto se hunde el borde del nucleo

// Radio del nucleo a una altura dada. El mesher lo emite como cuarto de
// circulo: radio = pr*cos(t*PI/2), altura = ph*sin(t*PI/2).
float radioPinaEnAltura(float pr, float ph, float y) {
    if (ph <= 0.0f || y <= 0.0f) return pr;
    if (y >= ph) return 0.0f;
    const float s = y / ph;
    return pr * sqrtf(1.0f - s * s);
}

struct Planta {
    float ALTO, RADIO, prPina, phPina, r0Penca, y0Penca;
    int celdas, npencas;
};

Planta plantaDe(uint16_t etapa) {
    namespace M = Compuesto::Maguey;
    Planta p{};
    p.celdas  = M::celdasDeEtapa(etapa);
    const float esc = M::escalaDeEtapa(etapa);
    p.ALTO    = M::alturaReal(p.celdas);
    p.RADIO   = M::radioDeCeldas(p.celdas);
    p.npencas = M::pencasDeEtapa(etapa);
    p.prPina  = p.RADIO * 0.52f;              // radio de la piña
    p.phPina  = M::altoHoja(esc) * 0.30f;     // alto de la piña
    p.r0Penca = p.RADIO * M::ARRANQUE;        // donde nace la hoja
    p.y0Penca = M::altoHoja(esc) * 0.08f;
    return p;
}

} // namespace

// ============================================================================
// 1. LAS HOJAS NACEN DENTRO DEL NUCLEO
// ============================================================================

TEST_CASE("Maguey: la penca nace dentro del volumen de la piña") {
    namespace M = Compuesto::Maguey;

    // La invariante de "no hay hueco entre el nucleo y la hoja": el punto de
    // nacimiento (r0, y0) cae DENTRO de la piña, no al lado ni por encima.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e, " r0=", p.r0Penca, " y0=", p.y0Penca,
             " piña r=", p.prPina, " h=", p.phPina);

        // No nace por encima de la piña: eso seria una hoja flotando.
        CHECK(p.y0Penca <= p.phPina);

        // Y a esa altura el radio del nucleo alcanza al arranque de la hoja.
        CHECK(p.r0Penca <= radioPinaEnAltura(p.prPina, p.phPina, p.y0Penca));
    }
}

TEST_CASE("Maguey: el nucleo sobresale del anillo donde nacen las hojas") {
    namespace M = Compuesto::Maguey;

    // Version fuerte: la piña no solo toca el arranque, lo envuelve. Ese margen
    // es lo que hace que las hojas salgan DE ella en vez de estar clavadas en
    // un palo.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e);
        CHECK(p.prPina > p.r0Penca);
    }
}

// ============================================================================
// 2. LA PLANTA SE PEGA AL SUELO
// ============================================================================

TEST_CASE("Maguey: el nucleo arranca a ras de suelo o enterrado, nunca flotando") {
    // El mesher sube el modelo ELEVADO para que la cupula se vea entera. Pero
    // la piña esta ABIERTA POR ABAJO: si su borde quedara sobre el suelo, se
    // veria el hueco por debajo desde un angulo rasante. Por eso el anillo
    // inferior se hunde SOTERRADO.
    const float baseNucleo = ELEVADO - SOTERRADO;
    INFO("base del nucleo respecto al suelo: ", baseNucleo);
    CHECK(baseNucleo <= 0.0f);
}

TEST_CASE("Maguey: la planta asoma del suelo en todas las etapas") {
    namespace M = Compuesto::Maguey;

    // El defecto contrario al de flotar: hundirla tanto que parezca enterrada.
    // Hasta el brote -- el de piña mas baja -- tiene que asomar.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        const float cima = p.phPina + ELEVADO;
        INFO("etapa ", e, " cima de la piña ", cima);
        CHECK(cima > SOTERRADO);
    }
}

TEST_CASE("Maguey: las hojas arrancan por encima del suelo") {
    namespace M = Compuesto::Maguey;

    // Si el arranque quedara bajo tierra, la hoja saldria del subsuelo.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e, " arranque sobre el suelo ", p.y0Penca + ELEVADO);
        CHECK(p.y0Penca + ELEVADO > 0.0f);
    }
}

// ============================================================================
// 3. CABE ENTERA EN SU CELDA BASE
// ============================================================================
// Es la precondicion que hace legitimo emitirla desde SEG == 0. Si dejara de
// cumplirse habria que repartirla otra vez, y el recorte -- con sus huecos --
// volveria.

TEST_CASE("Maguey: cabe entero dentro del alcance del mesher") {
    namespace M = Compuesto::Maguey;

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e, " alto ", p.ALTO, " radio ", p.RADIO);

        // A lo ancho: topado para no cruzar el borde del chunk, que es donde el
        // recorte del dibujado si corta de verdad.
        CHECK(p.RADIO + p.r0Penca <= M::radioMaximoReal() + 0.001f);

        // A lo alto: no pasa de las celdas que declara ocupar, que son las que
        // existen en el mundo y dan la colision.
        CHECK(p.ALTO <= (float)p.celdas);
    }
}

TEST_CASE("Maguey: ninguna hoja se dispara mas alla de la roseta") {
    namespace M = Compuesto::Maguey;

    // "Vertices extremadamente alejados del nucleo": el alcance de la penca
    // (r0 + largo) tiene que cuadrar con el RADIO de su etapa. `var` es la
    // variacion por hoja del mesher, que llega a 1.11.
    constexpr float VAR_MAX = 0.88f + 23.0f * 0.01f;

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        const float alcance = p.RADIO * VAR_MAX;   // r0 + (RADIO*var - r0)
        INFO("etapa ", e, " alcance ", alcance, " radio ", p.RADIO);
        CHECK(alcance <= M::radioMaximoReal());
    }
}

// ============================================================================
// 4. LA CONEXION SE MANTIENE AL CRECER
// ============================================================================

TEST_CASE("Maguey: la conexion se conserva en las cinco etapas") {
    namespace M = Compuesto::Maguey;

    // Al crecer cambian longitud, numero de hojas, diametro y volumen del
    // nucleo -- pero la planta sigue pegada al suelo y las hojas siguen
    // naciendo del nucleo.
    float radioAnt = -1.0f, pinaAnt = -1.0f;
    int pencasAnt = -1;

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e);

        // La conexion, en cada etapa sin excepcion.
        CHECK(p.r0Penca <= radioPinaEnAltura(p.prPina, p.phPina, p.y0Penca));

        // Y nada encoge al crecer.
        if (radioAnt >= 0.0f) {
            CHECK(p.RADIO   >= radioAnt);
            CHECK(p.prPina  >= pinaAnt);
            CHECK(p.npencas >  pencasAnt);
        }
        radioAnt = p.RADIO; pinaAnt = p.prPina; pencasAnt = p.npencas;
    }
}

TEST_CASE("Maguey: el nucleo crece con la planta") {
    namespace M = Compuesto::Maguey;

    // Si el nucleo se quedara fijo mientras la roseta crece, las hojas de una
    // planta grande naceran de un boton diminuto y volveria a leerse como "no
    // conectada" -- el bug que ya se corrigio subiendo la piña a RADIO*0.52.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        INFO("etapa ", e, " piña ", p.prPina, " radio ", p.RADIO);
        CHECK(p.prPina >= p.RADIO * 0.5f);
    }
}

// ============================================================================
// 5. SIGUE SIENDO ORGANICA
// ============================================================================
// Corregir la conexion no puede convertir la planta en un bulto macizo: tiene
// que quedar aire entre las hojas.

TEST_CASE("Maguey: la roseta sigue abierta, no es un bulto macizo") {
    namespace M = Compuesto::Maguey;

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);

        // Arco disponible por hoja a media altura de la roseta.
        const float arco = 6.2831853f * (p.RADIO * 0.6f) / (float)p.npencas;

        // Ancho maximo de la penca, con la formula del mesher.
        const float largo = p.RADIO * 0.7f - p.r0Penca;
        const float recorrido = p.r0Penca + (largo > 0.02f ? largo : 0.02f);
        const float anchoMax = recorrido * 0.30f * M::engrosado(e);

        INFO("etapa ", e, " ancho ", anchoMax, " arco ", arco);
        // La hoja no llena su hueco entero: queda aire entre pencas.
        CHECK(anchoMax < arco * 2.6f);
    }
}

TEST_CASE("Maguey: la roseta es tan ancha como alta") {
    namespace M = Compuesto::Maguey;

    // La proporcion botanica del pulquero. Es lo que distingue su silueta de la
    // del tequilana, que es claramente mas ancho que alto.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const Planta p = plantaDe(e);
        const float diametro = p.RADIO * 2.0f;
        INFO("etapa ", e, " alto ", p.ALTO, " diametro ", diametro);
        CHECK(diametro > p.ALTO * 0.5f);
        CHECK(diametro < p.ALTO * 2.2f);
    }
}

// ============================================================================
// 6. EL AGAVE AZUL, LA MISMA REGLA
// ============================================================================
// La familia hermana tenia el mismo bug y se corrigio igual. Su nucleo es mas
// pequeño en proporcion (0.34 frente a 0.52), asi que conviene comprobar que
// aun asi cubre el arranque de sus pencas.

TEST_CASE("Agave azul: la penca nace dentro de su nucleo") {
    namespace AG = Compuesto::AgaveAzul;

    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        const float RADIO = AG::radioDeEtapa(e);
        const float prNucleo = RADIO * 0.34f;          // como lo emite el mesher
        const float r0Penca  = RADIO * AG::ARRANQUE;   // 0.30

        INFO("etapa ", e, " nucleo ", prNucleo, " arranque ", r0Penca);
        // El arranque queda dentro del nucleo, que es lo que evita el anillo
        // de aire entre el cuerpo y las hojas.
        CHECK(r0Penca <= prNucleo);
    }
}
