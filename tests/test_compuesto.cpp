#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

using namespace Compuesto;

// ============================================================================
// TESTS DEL SISTEMA DE BLOQUES COMPUESTOS
// ============================================================================
// El sistema mete el ESTADO de un bloque dentro de su propio ID, apoyandose en
// que BlockType se serializa como 4 bytes y el motor solo usa ~168 valores.
//
// Eso da persistencia gratis (el save no cambia), pero a cambio TODO depende
// de que el empaquetado de bits sea exacto: un campo que pise a otro no da un
// error de compilacion, da un maguey que cambia de etapa al llenarse de jugo.
// Estos tests son la red que lo impide.
//
// Lo que fijan:
//   1. Componer y descomponer un ID es reversible SIEMPRE.
//   2. Los campos no se pisan entre si.
//   3. Escribir un campo no toca los demas.
//   4. Los valores fuera de rango se acotan, no desbordan al vecino.
//   5. El espacio de IDs no choca con los bloques reales ni con las mixtas.
//   6. Las reglas del maguey (produccion, capado, crecimiento) se cumplen.

// ----------------------------------------------------------------------------
// EL ESPACIO DE IDs: SIN CHOQUES
// ----------------------------------------------------------------------------

TEST_CASE("Compuesto: el rango no pisa a los bloques normales") {
    // Si se solapara con el enum, un maguey se leeria como piedra.
    CHECK(COMPUESTO_BASE > BLOCK_TYPE_MAX);

    // Ni con las celdas mixtas, que tienen su propio rango calculado.
    CHECK(COMPUESTO_BASE > BLOCK_MIXTO_FIN);
}

TEST_CASE("Compuesto: un bloque normal no se confunde con un compuesto") {
    CHECK_FALSE(esCompuesto(BLOCK_STONE));
    CHECK_FALSE(esCompuesto(BLOCK_AIR));
    CHECK_FALSE(esCompuesto(BLOCK_AGUAMIEL));
    CHECK_FALSE(esCompuesto((BlockType)BLOCK_TYPE_MAX));
    // Ni una celda mixta, que vive en otro rango calculado.
    CHECK_FALSE(esCompuesto(mixto(BLOCK_DIRT, 3, BLOCK_SAND)));
}

TEST_CASE("Compuesto: cada familia tiene su hueco, sin solaparse") {
    // Dos familias distintas nunca pueden dar el mismo ID: seria un maguey
    // que se lee como un arbol.
    for (int f1 = 0; f1 < FAM_COUNT; ++f1)
        for (int f2 = f1 + 1; f2 < FAM_COUNT; ++f2) {
            // El ultimo ID de f1 va antes que el primero de f2.
            const BlockType finF1 = hacer((Familia)f1, ESTADOS_POR_FAMILIA - 1);
            const BlockType iniF2 = hacer((Familia)f2, 0);
            CHECK((int)finF1 < (int)iniF2);
        }
}

TEST_CASE("Compuesto: componer y descomponer es reversible") {
    // La propiedad de fondo: lo que se guarda es lo que se lee. Si esto
    // fallara, un mundo cargado tendria magueyes distintos a los guardados.
    for (int f = 0; f < FAM_COUNT; ++f)
        for (uint16_t e = 0; e < 2000; e += 7) {
            const BlockType t = hacer((Familia)f, e);
            REQUIRE(esCompuesto(t));
            CHECK(familiaDe(t) == (Familia)f);
            CHECK(estadoDe(t)  == e);
        }
}

// ----------------------------------------------------------------------------
// EL EMPAQUETADO DE BITS
// ----------------------------------------------------------------------------

TEST_CASE("Campos: ninguno pisa a otro") {
    // El fallo mas peligroso del diseño: dos campos que comparten un bit.
    // No da error de compilacion -- da un maguey que cambia de etapa al
    // llenarse de aguamiel. Se comprueba que las mascaras son disjuntas.
    const Campo TODOS[] = {
        Maguey::ETAPA, Maguey::GIRO, Maguey::PUNTAS,
        Maguey::AGUAMIEL, Maguey::CAPADO
    };
    const int N = (int)(sizeof(TODOS) / sizeof(TODOS[0]));

    for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j) {
            INFO("campos ", i, " y ", j);
            CHECK((TODOS[i].mascara() & TODOS[j].mascara()) == 0);
        }
}

TEST_CASE("Campos: todos caben en los bits del estado") {
    // Un campo que se saliera de los 16 bits perderia sus bits altos al
    // guardarse, y el bloque se leeria mal al cargar el mundo.
    const Campo TODOS[] = {
        Maguey::ETAPA, Maguey::GIRO, Maguey::PUNTAS,
        Maguey::AGUAMIEL, Maguey::CAPADO
    };
    for (const Campo& c : TODOS) {
        CHECK((int)c.desplaz + (int)c.ancho <= BITS_ESTADO);
    }
}

TEST_CASE("Campos: escribir uno no toca los demas") {
    // Es lo que permite cambiar el aguamiel sin reescribir la planta entera.
    uint16_t e = 0;
    e = escribir(e, Maguey::ETAPA,    Maguey::PRODUCTOR);
    e = escribir(e, Maguey::GIRO,     2);
    e = escribir(e, Maguey::PUNTAS,   5);
    e = escribir(e, Maguey::AGUAMIEL, 9);
    e = escribir(e, Maguey::CAPADO,   1);

    // Se cambia SOLO el aguamiel...
    const uint16_t e2 = escribir(e, Maguey::AGUAMIEL, 3);

    CHECK(leer(e2, Maguey::AGUAMIEL) == 3);
    // ...y el resto sigue igual.
    CHECK(leer(e2, Maguey::ETAPA)  == Maguey::PRODUCTOR);
    CHECK(leer(e2, Maguey::GIRO)   == 2);
    CHECK(leer(e2, Maguey::PUNTAS) == 5);
    CHECK(leer(e2, Maguey::CAPADO) == 1);
}

TEST_CASE("Campos: un valor pasado de rango se acota, no desborda") {
    // Si un valor demasiado grande se colara, sus bits altos irian a parar al
    // campo de al lado: llenar de aguamiel cambiaria las puntas.
    uint16_t e = 0;
    e = escribir(e, Maguey::PUNTAS, 3);
    e = escribir(e, Maguey::AGUAMIEL, 999);      // el maximo es 15

    CHECK(leer(e, Maguey::AGUAMIEL) == 15);      // acotado
    CHECK(leer(e, Maguey::PUNTAS)   == 3);       // el vecino, intacto
}

TEST_CASE("Campos: ida y vuelta con todos los valores posibles") {
    // Barrido exhaustivo de cada campo en todo su rango.
    const Campo TODOS[] = {
        Maguey::ETAPA, Maguey::GIRO, Maguey::PUNTAS,
        Maguey::AGUAMIEL, Maguey::CAPADO
    };
    for (const Campo& c : TODOS)
        for (uint16_t v = 0; v <= c.maximo(); ++v) {
            const uint16_t e = escribir(0, c, v);
            CHECK(leer(e, c) == v);
        }
}

// ----------------------------------------------------------------------------
// LAS REGLAS DEL MAGUEY
// ----------------------------------------------------------------------------

TEST_CASE("Maguey: crear y leer devuelve lo mismo que se puso") {
    const BlockType m = Maguey::crear(Maguey::MADURO, 2, 4, 6, true);

    REQUIRE(esCompuesto(m));
    CHECK(familiaDe(m) == FAM_MAGUEY);
    CHECK(Maguey::etapaDe(m)    == Maguey::MADURO);
    CHECK(Maguey::giroDe(m)     == 2);
    CHECK(Maguey::puntasDe(m)   == 4);
    CHECK(Maguey::aguamielDe(m) == 6);
    CHECK(Maguey::capadoDe(m)   == true);
}

TEST_CASE("Maguey: uno recien nacido no tiene jugo ni esta capado") {
    for (uint16_t etapa = 0; etapa <= Maguey::PRODUCTOR; ++etapa) {
        const BlockType m = Maguey::nuevo(etapa, 0);
        CHECK(Maguey::aguamielDe(m) == 0);
        CHECK_FALSE(Maguey::capadoDe(m));
        // Y le salen las puntas que le tocan por edad.
        CHECK(Maguey::puntasDe(m) == Maguey::puntasDeEtapa(etapa));
    }
}

TEST_CASE("Maguey: no produce hasta estar maduro Y capado") {
    // Las dos condiciones. Un maguey joven capado no da nada, y uno productor
    // sin capar tampoco: hay que abrirlo primero.
    for (uint16_t etapa = 0; etapa <= Maguey::PRODUCTOR; ++etapa) {
        const BlockType sinCapar = Maguey::nuevo(etapa, 0);
        CHECK_FALSE(Maguey::produce(estadoDe(sinCapar)));

        const BlockType capado = Maguey::capar(sinCapar);
        const bool deberia = (etapa == Maguey::MADURO ||
                              etapa == Maguey::PRODUCTOR);
        INFO("etapa ", etapa);
        CHECK(Maguey::produce(estadoDe(capado)) == deberia);
    }
}

TEST_CASE("Maguey: capar le quita una punta") {
    // Es la punta que se corta para abrir la planta.
    const BlockType m = Maguey::nuevo(Maguey::PRODUCTOR, 0);
    const uint16_t antes = Maguey::puntasDe(m);
    const BlockType c = Maguey::capar(m);

    CHECK(Maguey::capadoDe(c));
    CHECK(Maguey::puntasDe(c) == antes - 1);
    // Y no ha tocado nada mas.
    CHECK(Maguey::etapaDe(c) == Maguey::etapaDe(m));
    CHECK(Maguey::giroDe(c)  == Maguey::giroDe(m));
}

TEST_CASE("Maguey: capar uno sin puntas no baja de cero") {
    // Un contador sin suelo daria la vuelta a 7 por el empaquetado.
    const BlockType m = Maguey::crear(Maguey::PRODUCTOR, 0, 0, 0, false);
    const BlockType c = Maguey::capar(m);
    CHECK(Maguey::puntasDe(c) == 0);
    CHECK(Maguey::capadoDe(c));
}

TEST_CASE("Maguey: la capacidad crece con la etapa") {
    // Un maguey joven no da nada; el productor llena del todo. Es lo que hace
    // que buscar un buen ejemplar tenga sentido.
    CHECK(Maguey::capacidad(Maguey::BROTE)     == 0);
    CHECK(Maguey::capacidad(Maguey::JOVEN)     == 0);
    CHECK(Maguey::capacidad(Maguey::ADULTO)    == 0);
    CHECK(Maguey::capacidad(Maguey::MADURO)    > 0);
    CHECK(Maguey::capacidad(Maguey::PRODUCTOR) >
          Maguey::capacidad(Maguey::MADURO));

    // Y la del productor cabe en el campo.
    CHECK(Maguey::capacidad(Maguey::PRODUCTOR) <= Maguey::AGUAMIEL.maximo());
}

TEST_CASE("Maguey: hace falta un minimo de jugo para llenar un tazon") {
    const BlockType base = Maguey::capar(
        Maguey::nuevo(Maguey::PRODUCTOR, 0));

    CHECK_FALSE(Maguey::hayParaTazon(estadoDe(
        Maguey::conAguamiel(base, 0))));
    CHECK_FALSE(Maguey::hayParaTazon(estadoDe(
        Maguey::conAguamiel(base, Maguey::AGUAMIEL_PARA_TAZON - 1))));
    CHECK(Maguey::hayParaTazon(estadoDe(
        Maguey::conAguamiel(base, Maguey::AGUAMIEL_PARA_TAZON))));
    CHECK(Maguey::hayParaTazon(estadoDe(
        Maguey::conAguamiel(base, 15))));
}

TEST_CASE("Maguey: vaciarlo deja la planta intacta y lista para volver") {
    // Recoger el jugo NO puede llevarse el capado ni la etapa: el maguey
    // sigue ahi y vuelve a producir.
    const BlockType lleno = Maguey::conAguamiel(
        Maguey::capar(Maguey::nuevo(Maguey::PRODUCTOR, 1)), 15);
    const BlockType v = Maguey::vaciado(lleno);

    CHECK(Maguey::aguamielDe(v) == 0);
    CHECK(Maguey::capadoDe(v));                       // sigue capado
    CHECK(Maguey::etapaDe(v) == Maguey::PRODUCTOR);   // y en su etapa
    CHECK(Maguey::giroDe(v)  == 1);
    CHECK(Maguey::produce(estadoDe(v)));              // volvera a manar
}

TEST_CASE("Maguey: crecer conserva lo que ya tenia") {
    const BlockType joven = Maguey::nuevo(Maguey::JOVEN, 3);
    const BlockType adulto = Maguey::conEtapa(joven, Maguey::ADULTO);

    CHECK(Maguey::etapaDe(adulto) == Maguey::ADULTO);
    CHECK(Maguey::giroDe(adulto)  == 3);           // el giro no cambia
    // Y le salen mas puntas al crecer.
    CHECK(Maguey::puntasDe(adulto) >= Maguey::puntasDe(joven));
}

TEST_CASE("Maguey: crecer no le devuelve la punta al que ya esta capado") {
    // Si al crecer recuperara puntas, capar dejaria de ser permanente.
    const BlockType capado = Maguey::capar(
        Maguey::nuevo(Maguey::MADURO, 0));
    const uint16_t antes = Maguey::puntasDe(capado);
    const BlockType crecido = Maguey::conEtapa(capado, Maguey::PRODUCTOR);

    CHECK(Maguey::capadoDe(crecido));
    CHECK(Maguey::puntasDe(crecido) == antes);
}

TEST_CASE("Maguey: la escala crece con la etapa, sin saltos raros") {
    // Es lo que hace que un productor se reconozca de lejos.
    float anterior = 0.0f;
    for (uint16_t e = 0; e <= Maguey::PRODUCTOR; ++e) {
        const float s = Maguey::escalaDeEtapa(e);
        INFO("etapa ", e, " escala ", s);
        CHECK(s > anterior);      // siempre a mas
        CHECK(s > 0.0f);
        CHECK(s < 4.0f);          // ni tan grande que invada el terreno
        anterior = s;
    }
}

// ----------------------------------------------------------------------------
// LOS COMPONENTES
// ----------------------------------------------------------------------------

TEST_CASE("Componentes: un maguey pelado solo tiene cuerpo") {
    // Sin puntas y sin jugo: una sola parte que tocar.
    const BlockType m = Maguey::crear(Maguey::BROTE, 0, 0, 0, false);
    CHECK(componentesDe(m) == 1);
    CHECK(componenteN(m, 0) == COMP_CUERPO);
}

TEST_CASE("Componentes: con puntas aparece el adorno") {
    const BlockType m = Maguey::crear(Maguey::ADULTO, 0, 4, 0, false);
    CHECK(componentesDe(m) == 2);
    CHECK(componenteN(m, 1) == COMP_ADORNO);
}

TEST_CASE("Componentes: con jugo aparece el fluido") {
    const BlockType m = Maguey::crear(Maguey::PRODUCTOR, 0, 3, 9, true);
    CHECK(componentesDe(m) == 3);
    CHECK(componenteN(m, 0) == COMP_CUERPO);
    CHECK(componenteN(m, 1) == COMP_ADORNO);
    CHECK(componenteN(m, 2) == COMP_FLUIDO);
}

TEST_CASE("Componentes: el raycast no prueba partes que no existen") {
    // Es la razon de que componentesDe dependa del estado: sin esto, apuntar
    // a un maguey sin jugo podria seleccionar un liquido invisible.
    const BlockType seco = Maguey::crear(Maguey::PRODUCTOR, 0, 3, 0, true);
    CHECK(componentesDe(seco) == 2);          // cuerpo + puntas, sin fluido

    const BlockType lleno = Maguey::conAguamiel(seco, 10);
    CHECK(componentesDe(lleno) == 3);         // ahora si
}

TEST_CASE("Componentes: el fluido NUNCA se puede romper a golpes") {
    // Es liquido: solo se saca con un recipiente. Las otras partes si.
    const BlockType m = Maguey::crear(Maguey::PRODUCTOR, 0, 3, 9, true);
    CHECK(componenteRompible(m, 0));          // el cuerpo si
    CHECK(componenteRompible(m, 1));          // las puntas si
    CHECK_FALSE(componenteRompible(m, 2));    // el jugo no
}

TEST_CASE("Componentes: un bloque normal es una sola parte rompible") {
    // El caso general tiene que seguir funcionando sin tratarlo aparte.
    CHECK(componentesDe(BLOCK_STONE) == 1);
    CHECK(componenteRompible(BLOCK_STONE, 0));
}
