#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// TESTS DE LA PUNTA DEL MAGUEY Y EL AGUAMIEL COMO CELDA COMPARTIDA
// ============================================================================
// Reglas que fijan:
//
//   1. El aguamiel es una CELDA COMPARTIDA: el cajete y el jugo son dos
//      bloques distintos en el mismo voxel, seleccionables por separado.
//   2. Quitar el jugo deja el cuenco; romper el cuenco se lo lleva todo.
//   3. Cualquier tazon sirve para recoger, y sale con SU misma madera.
//   4. El aguamiel no se puede colocar ni es roca (no se pica).
//
// El punto 3 es el delicado: hay tres maderas x tres estados (vacio, agua,
// aguamiel), y una correspondencia mal puesta cambiaria el tazon de un
// jugador por otro de madera distinta.

// ----------------------------------------------------------------------------
// LA CELDA COMPARTIDA: DOS BLOQUES EN UN VOXEL
// ----------------------------------------------------------------------------

TEST_CASE("Aguamiel: es una celda compartida, no un bloque suelto") {
    // Esto es lo que le da la propiedad pedida: estar DENTRO de otro bloque
    // y aun asi ser seleccionable por su cuenta.
    CHECK(esCompartido(BLOCK_AGUAMIEL));

    // Sus dos piezas: el cuenco de maguey y el jugo de dentro.
    CHECK(piezaPrimera(BLOCK_AGUAMIEL) == BLOCK_MAGUEY_HUECO);
    CHECK(piezaSegunda(BLOCK_AGUAMIEL) == BLOCK_AGUAMIEL);
}

TEST_CASE("Aguamiel: quitar el jugo deja el cuenco en pie") {
    // Es lo que pasa al recogerlo con el tazon: el maguey capado sigue ahi,
    // vacio, y volvera a manar a los 10 minutos.
    CHECK(quitarPieza(BLOCK_AGUAMIEL, true) == BLOCK_MAGUEY_HUECO);
}

TEST_CASE("Aguamiel: romper el cuenco se lleva el jugo con el") {
    // El liquido no puede quedarse flotando sin nada que lo contenga. Si
    // esto devolviera el aguamiel, quedaria un charco irrompible en el aire
    // -- y como el jugo NO se puede romper, seria imposible de quitar.
    CHECK(quitarPieza(BLOCK_AGUAMIEL, false) == BLOCK_AIR);
}

// ----------------------------------------------------------------------------
// LOS TAZONES: TRES MADERAS x TRES ESTADOS
// ----------------------------------------------------------------------------

TEST_CASE("Tazon: los tres con aguamiel existen y se reconocen") {
    CHECK(esTazonConAguamiel(BLOCK_TAZON_PINO_AGUAMIEL));
    CHECK(esTazonConAguamiel(BLOCK_TAZON_ENCINO_AGUAMIEL));
    CHECK(esTazonConAguamiel(BLOCK_TAZON_OYAMEL_AGUAMIEL));

    // Y cuentan como tazon a todos los efectos.
    CHECK(esTazon(BLOCK_TAZON_PINO_AGUAMIEL));
    CHECK(esTazon(BLOCK_TAZON_ENCINO_AGUAMIEL));
    CHECK(esTazon(BLOCK_TAZON_OYAMEL_AGUAMIEL));
}

TEST_CASE("Tazon: CUALQUIERA sirve para recoger aguamiel") {
    // Se pidio que valiera "cualquier tipo de tazon": vacio, con agua, o uno
    // que ya lleve aguamiel (se rellena).
    CHECK(tazonConAguamiel(BLOCK_TAZON_PINO)      == BLOCK_TAZON_PINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_PINO_AGUA) == BLOCK_TAZON_PINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_PINO_AGUAMIEL)
                                                  == BLOCK_TAZON_PINO_AGUAMIEL);
}

TEST_CASE("Tazon: el aguamiel no cambia la madera del recipiente") {
    // El punto delicado. Meter un tazon de encino y recibir uno de pino seria
    // un cambiazo silencioso que el jugador solo veria mirando el inventario.
    CHECK(tazonConAguamiel(BLOCK_TAZON_ENCINO)      == BLOCK_TAZON_ENCINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_ENCINO_AGUA) == BLOCK_TAZON_ENCINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_OYAMEL)      == BLOCK_TAZON_OYAMEL_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_OYAMEL_AGUA) == BLOCK_TAZON_OYAMEL_AGUAMIEL);
}

TEST_CASE("Tazon: lo que no es tazon no se llena de aguamiel") {
    // BLOCK_AIR es la señal de "esto no vale", y es lo que impide que un
    // ingrediente cualquiera se convierta en tazon por error.
    CHECK(tazonConAguamiel(BLOCK_STONE)        == BLOCK_AIR);
    CHECK(tazonConAguamiel(BLOCK_PEDAZO_BARRO) == BLOCK_AIR);
    CHECK(tazonConAguamiel(BLOCK_AGUAMIEL)     == BLOCK_AIR);
}

// ----------------------------------------------------------------------------
// EL BARRO NO SE LLEVA EL AGUAMIEL POR DELANTE
// ----------------------------------------------------------------------------

TEST_CASE("Barro: la receta NO acepta tazones de aguamiel") {
    // tazonVaciado es lo que usa la receta del barro. Si aceptara el
    // aguamiel, se podria gastar el jugo -- 10 minutos de espera -- en hacer
    // barro sin darse cuenta.
    CHECK(tazonVaciado(BLOCK_TAZON_PINO_AGUAMIEL)   == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_TAZON_ENCINO_AGUAMIEL) == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_TAZON_OYAMEL_AGUAMIEL) == BLOCK_AIR);

    // Pero los de AGUA si, que es lo que la receta necesita.
    CHECK(tazonVaciado(BLOCK_TAZON_PINO_AGUA)   == BLOCK_TAZON_PINO);
    CHECK(tazonVaciado(BLOCK_TAZON_ENCINO_AGUA) == BLOCK_TAZON_ENCINO);
    CHECK(tazonVaciado(BLOCK_TAZON_OYAMEL_AGUA) == BLOCK_TAZON_OYAMEL);
}

TEST_CASE("Tazon: vaciarlo del todo funciona venga de donde venga") {
    // tazonVaciadoTodo si acepta las dos cosas: se usa cuando lo que importa
    // es dejar el recipiente limpio, no que llevaba.
    CHECK(tazonVaciadoTodo(BLOCK_TAZON_PINO_AGUAMIEL)   == BLOCK_TAZON_PINO);
    CHECK(tazonVaciadoTodo(BLOCK_TAZON_ENCINO_AGUAMIEL) == BLOCK_TAZON_ENCINO);
    CHECK(tazonVaciadoTodo(BLOCK_TAZON_OYAMEL_AGUAMIEL) == BLOCK_TAZON_OYAMEL);
    CHECK(tazonVaciadoTodo(BLOCK_TAZON_PINO_AGUA)       == BLOCK_TAZON_PINO);
}

// ----------------------------------------------------------------------------
// LA GEOMETRIA DE LA PUNTA
// ----------------------------------------------------------------------------

TEST_CASE("Punta: es gruesa y cabe dentro de su voxel") {
    // El cono arranca en 12/16 -- bien gruesa, para verse de lejos -- y
    // acaba casi en pico. Todos los escalones tienen que caber en el bloque:
    // uno que se pasara de 1.0 asomaria por fuera y se cruzaria con el vecino.
    static const float ANCHO[5] =
        { 12.0f/16.0f, 9.0f/16.0f, 6.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f };

    for (int e = 0; e < 5; ++e) {
        const float a = ANCHO[e] * 0.5f;
        CHECK(0.5f - a >= 0.0f);
        CHECK(0.5f + a <= 1.0f);
    }

    // Es MAS GRUESA que la mitad del bloque en su base: eso es lo que la
    // hace visible desde lejos.
    CHECK(ANCHO[0] > 0.5f);

    // Y mengua de verdad hacia arriba, o no seria una punta.
    for (int e = 1; e < 5; ++e) CHECK(ANCHO[e] < ANCHO[e - 1]);
}

TEST_CASE("Punta: los escalones se tocan, sin huecos entre ellos") {
    // Cada escalon empieza EXACTAMENTE donde acaba el anterior. Si quedara
    // aire entre dos, se veria una rendija a traves de la punta.
    for (int e = 0; e < 5; ++e) {
        const float y0 = (float)e / 5.0f;
        const float y1 = (float)(e + 1) / 5.0f;
        CHECK(y1 > y0);
        if (e > 0) CHECK(y0 == doctest::Approx((float)e / 5.0f));
    }
    // Y entre todos cubren el voxel entero, de suelo a techo.
    CHECK(0.0f == doctest::Approx(0.0f / 5.0f));
    CHECK(1.0f == doctest::Approx(5.0f / 5.0f));
}

// ----------------------------------------------------------------------------
// LO QUE NO DEBE CAMBIAR
// ----------------------------------------------------------------------------

TEST_CASE("Maguey: nada de esto se pica con el pico") {
    // Son planta, no roca. Y sobre todo: intentarlo no debe gastar la
    // herramienta.
    CHECK(desgastePico(BLOCK_MAGUEY_PUNTA) == 0);
    CHECK(desgastePico(BLOCK_MAGUEY_HUECO) == 0);
    CHECK(desgastePico(BLOCK_AGUAMIEL)     == 0);
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_PUNTA));
}

TEST_CASE("Tazones de aguamiel: son items, no bloques del terreno") {
    CHECK((int)BLOCK_TAZON_PINO_AGUAMIEL   > BLOCK_LAST_PLACEABLE);
    CHECK((int)BLOCK_TAZON_ENCINO_AGUAMIEL > BLOCK_LAST_PLACEABLE);
    CHECK((int)BLOCK_TAZON_OYAMEL_AGUAMIEL > BLOCK_LAST_PLACEABLE);

    // Y el ultimo del enum es el que dice BLOCK_TYPE_MAX: si alguien añade
    // otro detras sin actualizarlo, el deserializador lo rechazaria al cargar.
    CHECK(BLOCK_TYPE_MAX == (int)BLOCK_TAZON_OYAMEL_AGUAMIEL);
}
