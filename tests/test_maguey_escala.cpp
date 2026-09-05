#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

using namespace Compuesto;
namespace M = Compuesto::Maguey;

// ============================================================================
// EL TAMAÑO DEL MAGUEY POR ETAPA
// ============================================================================
// Lo que se pidio: que los magueyes MADUROS -- los que dan aguamiel -- sean
// mucho mas grandes, para reconocerlos de lejos sin tener que acercarse a
// probar cual sirve.
//
// ⚠️ EL MAGUEY SE SALE DEL VOXEL A PROPOSITO.
//
// Hubo una tanda de trabajo dedicada a acotarlo al bloque, persiguiendo un bug
// real: con unas escalas mal calculadas las hojas asomaban CINCO voxels por
// encima de su celda y se veian como cubos flotando.
//
// Pero acotarlo lo dejaba raquitico. Mirando la version de agosto -- la que se
// veia bien -- resulto que sus magueyes tambien se salen: un ejemplar viejo
// llega a ~3 bloques de alto y ~2 de ancho. Eso es lo que los hace frondosos,
// y la contrapartida asumida es que sus hojas cruzan el terreno de al lado.
//
// Asi que estos tests YA NO comprueban que quepa. Comprueban que CREZCA bien:
// cada etapa mayor que la anterior, con proporciones de planta, y sin
// dispararse a tamaños absurdos.

TEST_CASE("Escala: los que dan aguamiel son mas grandes") {
    // La diferencia que se pidio: un maduro tiene que despegarse de un
    // adulto, no ser "un poco mayor".
    //
    // Se pide un 40% mas, no el doble. Las escalas son las de la version de
    // referencia (0.45 / 0.70 / 1.00 / 1.40 / 1.90), que crecen de forma
    // regular en vez de dar un salto brusco en el maduro. Exigir el doble
    // ataria el test a unos numeros concretos en lugar de a la regla: lo que
    // importa es que un maguey productivo se distinga de un adulto.
    const float adulto = M::escalaDeEtapa(M::ADULTO);
    const float maduro = M::escalaDeEtapa(M::MADURO);
    const float prod   = M::escalaDeEtapa(M::PRODUCTOR);

    CHECK(maduro >= adulto * 1.35f);
    // Y el productor destaca claramente sobre el adulto.
    CHECK(prod   >= adulto * 1.8f);
}

TEST_CASE("Escala: crece en todas las etapas, sin retrocesos") {
    // Si una etapa fuera mas pequeña que la anterior, un maguey ENCOGERIA al
    // crecer, que es lo contrario de lo que espera el jugador.
    float anterior = 0.0f;
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float s = M::escalaDeEtapa(e);
        INFO("etapa ", e, " escala ", s);
        CHECK(s > anterior);
        anterior = s;
    }
}

TEST_CASE("Escala: los que producen se distinguen de los que no") {
    // La regla de juego: si da aguamiel, se ve grande. Es la pista visual que
    // permite elegir a cual acercarse.
    const float mayorSinJugo = M::escalaDeEtapa(M::ADULTO);

    for (uint16_t e = M::MADURO; e <= M::PRODUCTOR; ++e) {
        INFO("etapa productiva ", e);
        CHECK(M::capacidad(e) > 0);                     // da jugo
        CHECK(M::escalaDeEtapa(e) > mayorSinJugo);      // y se nota
    }

    // Y al reves: las que no producen son las pequeñas.
    for (uint16_t e = M::BROTE; e <= M::ADULTO; ++e) {
        INFO("etapa esteril ", e);
        CHECK(M::capacidad(e) == 0);
        CHECK(M::escalaDeEtapa(e) <= mayorSinJugo);
    }
}


TEST_CASE("Escala: la roseta crece de verdad con la etapa") {
    // ⚠️ ESTE TEST CAMBIO DE CRITERIO, Y CONVIENE SABER POR QUE.
    //
    // Antes comprobaba que la planta CABIA EN EL VOXEL. Se escribio
    // persiguiendo un bug real (hojas asomando cinco bloques por encima de su
    // celda), pero acotar el maguey al bloque lo dejaba raquitico -- se probo
    // en el juego y no valia.
    //
    // La decision, tomada mirando la version de referencia: el maguey SE SALE
    // del voxel a proposito. Un ejemplar viejo llega a ~3 bloques de alto y
    // ~2 de ancho, y eso es lo que lo hace frondoso. La contrapartida asumida
    // es que sus hojas pueden cruzar el terreno de al lado.
    //
    // Asi que lo que se comprueba ya no es que quepa, sino que CREZCA de
    // forma coherente: cada etapa mas grande que la anterior y con
    // proporciones de planta, no de rascacielos.
    constexpr float PXL     = 1.0f / 16.0f;
    constexpr float VAR_MAX = 1.14f;
    constexpr float SUBIDA  = 0.90f;
    constexpr float SALIDA  = 0.6375f;

    float altoAnterior = 0.0f;

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc   = M::escalaDeEtapa(e);
        const float largo = 26.0f * PXL * esc;   // el largo de referencia
        const float alto  = largo * VAR_MAX * SUBIDA;
        const float ancho = largo * VAR_MAX * SALIDA;

        INFO("etapa ", e, " escala ", esc, " alto ", alto, " ancho ", ancho);

        // Crece en cada etapa: un maguey nunca encoge al madurar.
        CHECK(alto > altoAnterior);
        altoAnterior = alto;

        // Proporciones de agave: mas ancho que alto no, pero tampoco un
        // palo. La referencia da ancho/alto ~0.7.
        CHECK(ancho > alto * 0.5f);
        CHECK(ancho < alto * 1.2f);

        // Y un tope de cordura: por muy frondoso que sea, un maguey no puede
        // medir media pantalla. Si alguien sube las escalas por error, esto
        // lo caza.
        CHECK(alto < 5.0f);
        CHECK(ancho < 4.0f);
    }
}

TEST_CASE("Escala: el ejemplar viejo es el mas grande, con diferencia") {
    // La pista visual que permite elegir a cual acercarse: el que da aguamiel
    // se ve claramente mayor que un brote.
    const float brote = M::escalaDeEtapa(M::BROTE);
    const float prod  = M::escalaDeEtapa(M::PRODUCTOR);

    CHECK(prod > brote * 3.0f);
}
TEST_CASE("Escala: el cuenco conserva hueco dentro para el jugo") {
    // Si al crecer las paredes se comieran el interior, el aguamiel no
    // tendria donde dibujarse.
    constexpr float PARED = 0.05f;
    for (uint16_t e = M::MADURO; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        INFO("etapa ", e);
        CHECK(M::radioCajete(esc) - PARED > 0.0f);
        CHECK(M::hondoCajete(esc) - PARED > 0.0f);
    }
}

// ----------------------------------------------------------------------------
// LAS PIEZAS SIGUEN ENCAJANDO
// ----------------------------------------------------------------------------

TEST_CASE("Escala: el cajete sigue hundido en la roseta a cualquier tamaño") {
    // El bug del "cubo verde flotando" era que el cuenco estaba a una altura
    // fija. Al cambiar las escalas hay que comprobar que sigue dentro.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        INFO("etapa ", e);
        CHECK(M::alturaCajete(esc) < M::altoHoja(esc));   // hundido
        CHECK(M::alturaCajete(esc) > 0.0f);               // sobre el suelo
    }
}

TEST_CASE("Escala: las espinas siguen por encima del cajete") {
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        INFO("etapa ", e);
        CHECK(M::alturaEspina(esc) > M::alturaCajete(esc));
    }
}
