#include <doctest/doctest.h>
#include "BlockType.h"
#include "BlockCompat.h"   // la migracion de las mixtas de mundos v3

// ============================================================================
// TESTS DE LAS CELDAS MIXTAS
// ============================================================================
// La regla que fijan: una celda mixta NUNCA deja hueco. Abajo una capa con su
// nivel, y de ahí al techo el relleno — sin aire en medio y sin nada flotando.
//
// El caso que motivó estos tests: los TRONCOS y TABLONES no tienen niveles
// propios (un tronco en lonchas de 3 px es irreconocible), y por eso no podían
// ser el relleno de una celda. Al poner un tronco sobre una capa de tierra de
// 5 px, el tronco se iba al voxel de arriba: 11 px de aire a la vista y un
// tronco flotando.
//
// El arreglo separa dos conceptos que estaban mezclados:
//   - la capa de ABAJO necesita niveles (su altura ES su nivel)
//   - el material de ARRIBA no, porque siempre llega hasta el techo
//
// ⚠️ COMPATIBILIDAD DE SAVES. El índice del relleno viaja DENTRO del ID mixto,
// que es lo que hay escrito en los mundos guardados. Si alguien reordena
// tablaRelleno() o inserta en medio, las celdas mixtas ya guardadas pasarían a
// leerse como otro material. Los tests de abajo fijan las posiciones exactas
// para que ese error no pueda colarse en silencio.

// ----------------------------------------------------------------------------
// LO QUE MOTIVÓ EL ARREGLO: LA MADERA COMO RELLENO
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: un tronco puede rellenar una celda sin tener niveles") {
    // El TRONCO sigue SIN niveles propios: eso es deliberado y no cambia.
    // (El TABLON si los recupero: ver la nota de tablaNiveles.)
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD));

    // Pero AHORA sí puede ser el relleno de arriba.
    const BlockType celda = mixto(BLOCK_DIRT, 3, BLOCK_WOOD);
    REQUIRE(celda != BLOCK_AIR);          // antes daba BLOCK_AIR: el bug
    CHECK(esMixto(celda));

    // Y se descodifica entero: tronco, no una loncha.
    CHECK(mixtoBase(celda) == BLOCK_DIRT);
    CHECK(mixtoNivel(celda) == 3);
    CHECK(mixtoRelleno(celda) == BLOCK_WOOD);
}

TEST_CASE("Mixta: las seis maderas rellenan") {
    const BlockType maderas[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,
    };
    for (BlockType m : maderas) {
        const BlockType celda = mixto(BLOCK_STONE, 4, m);
        REQUIRE(celda != BLOCK_AIR);
        CHECK(mixtoRelleno(celda) == m);
        CHECK(mixtoBase(celda) == BLOCK_STONE);
        CHECK(mixtoNivel(celda) == 4);
    }
}

// ----------------------------------------------------------------------------
// LA INVARIANTE DE FONDO: NI HUECOS NI BLOQUES FLOTANDO
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: la celda esta LLENA, sin hueco a ninguna altura") {
    // Una celda mixta ocupa su voxel entero: se anda por encima como por un
    // bloque normal. Si esto dejara de valer, habría un escalón invisible.
    for (int nivel = 1; nivel <= 7; ++nivel) {
        const BlockType celda = mixto(BLOCK_DIRT, nivel, BLOCK_WOOD);
        REQUIRE(celda != BLOCK_AIR);
        CHECK(alturaDe(celda) == doctest::Approx(1.0f));

        // El relleno arranca EXACTAMENTE donde acaba la capa de abajo.
        // Ese "exactamente" es lo que impide que quede aire en medio.
        const float finCapa = alturaBaseMixto(celda);
        const float altoNivel = (float)alturaNivelPx(nivel) / 16.0f;
        CHECK(finCapa == doctest::Approx(altoNivel));

        // Y de ahí al techo hay relleno de verdad, no aire.
        CHECK(finCapa < 1.0f);
        CHECK(mixtoRelleno(celda) != BLOCK_AIR);
    }
}

TEST_CASE("Mixta: todo nivel de toda familia admite relleno de madera") {
    // Barrido completo: ninguna combinación puede quedarse sin representar,
    // porque la que se quedara fuera volvería a producir un bloque flotando.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    for (int i = 0; i < nNiv; ++i) {
        // ⚠️ LAS FAMILIAS RETIRADAS SE SALTAN.
        //
        // Las posiciones 11-15 eran los cinco minerales de veta. Los bloques
        // se eliminaron del juego, pero su FILA sigue aqui (con `entero` a
        // BLOCK_AIR) porque el indice viaja dentro de cada celda mixta ya
        // guardada: quitarla correria la madera de la 16-21 a la 11-16 y los
        // mundos existentes leerian otro relleno.
        //
        // Una familia sin bloque no puede formar una celda mixta nueva, que
        // es justo lo que comprueba el caso de mas abajo.
        if (T[i].entero == BLOCK_AIR) continue;

        for (int nivel = 1; nivel <= 7; ++nivel) {
            const BlockType celda = mixto(T[i].entero, nivel, BLOCK_WOOD);
            REQUIRE_MESSAGE(celda != BLOCK_AIR,
                            "familia ", i, " nivel ", nivel);
            CHECK(mixtoBase(celda) == T[i].entero);
            CHECK(mixtoNivel(celda) == nivel);
            CHECK(mixtoRelleno(celda) == BLOCK_WOOD);
        }
    }
}

// ----------------------------------------------------------------------------
// COMPATIBILIDAD: LOS MUNDOS YA GUARDADOS SE SIGUEN LEYENDO IGUAL
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: las 16 familias con niveles conservan su indice") {
    // El índice del relleno está escrito dentro del ID de cada celda mixta ya
    // guardada. Las 16 primeras posiciones de tablaRelleno() TIENEN que ser
    // las mismas que las de tablaNiveles(), y en el mismo orden, o los mundos
    // existentes leerían otro material.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);

    // ⚠️ LO QUE IMPORTA NO ES CUANTAS FAMILIAS HAY, SINO QUE LAS 16 PRIMERAS
    // NO SE MUEVAN.
    //
    // Esto decia `REQUIRE(nNiv == 16)` y saltaba al anadir la 17 (el techo de
    // hojas), abortando el test ANTES de comprobar los invariantes de verdad.
    // Era un censo, no una garantia: contar filas no protege ningun save.
    //
    // Lo que rompe mundos guardados es que una familia CAMBIE DE INDICE, y eso
    // es lo que se verifica abajo. Anadir al FINAL es seguro por diseno (ver
    // `familiasCaben`): ocupa un hueco reservado sin mover lo que ya hay.
    //
    // Se exige que quepan en el espacio reservado, que es la condicion real.
    REQUIRE(nNiv >= 16);
    REQUIRE(nNiv <= FAMILIAS_CON_NIVEL);
    REQUIRE(familiasCaben());

    // ⭐ SOLO LAS 16 PRIMERAS COMPARTEN INDICE CON `tablaRelleno`.
    //
    // Son las unicas que viajan dentro de un ID mixto ya guardado. De la 16 en
    // adelante las dos tablas son INDEPENDIENTES: en `tablaRelleno` esas
    // posiciones las ocupa la madera, que no tiene niveles propios.
    //
    // Una familia nueva anadida al final (el techo de hojas, la 16) puede ser
    // la CAPA DE ABAJO de una celda mixta --para eso tiene niveles-- pero no
    // el RELLENO de arriba: `indiceRelleno` le devuelve -1 y `mixto()` rechaza
    // la pareja. Es correcto y no rompe nada guardado; simplemente no se puede
    // rellenar una celda "con techo" por encima de otra capa.
    const int COMPARTIDAS = 16;

    for (int i = 0; i < COMPARTIDAS && i < nNiv; ++i) {
        // Las cinco familias retiradas (11-15, los minerales de veta) ya no
        // tienen bloque, asi que indiceRelleno() las rechaza a proposito. Lo
        // que hay que comprobar de ellas es que SIGAN OCUPANDO SU SITIO, y eso
        // se hace justo debajo: si desaparecieran, la madera se correria.
        if (T[i].entero == BLOCK_AIR) continue;

        CHECK_MESSAGE(indiceRelleno(T[i].entero) == i,
                      "la familia ", i, " cambio de indice: rompe saves");
    }

    // ⭐ LA MADERA NO SE HA MOVIDO.
    //
    // Es la comprobacion que protege de verdad los mundos guardados tras
    // retirar los minerales: los seis rellenos de madera tienen que seguir en
    // las posiciones 16-21. Si alguien borra las filas centinela "porque no
    // sirven para nada", esto salta.
    int nRel = 0; const BlockType* R = tablaRelleno(nRel);
    // 23 desde que el techo de hojas entro al FINAL de la tabla (pos. 22).
    // Lo que importa no es el numero, sino que las 16 primeras no se muevan:
    // es lo que se comprueba justo debajo.
    REQUIRE(nRel == 23);
    CHECK(R[16] == BLOCK_WOOD);
    CHECK(R[17] == BLOCK_WOOD_ENCINO);
    CHECK(R[18] == BLOCK_WOOD_OYAMEL);
    CHECK(R[19] == BLOCK_PLANKS);
    CHECK(R[20] == BLOCK_PLANKS_ENCINO);
    CHECK(R[21] == BLOCK_PLANKS_OYAMEL);
    CHECK(indiceRelleno(BLOCK_WOOD) == 16);
    CHECK(indiceRelleno(BLOCK_PLANKS_OYAMEL) == 21);
}

TEST_CASE("Mixta: un ID guardado con el orden viejo se MIGRA bien") {
    // ⭐⭐ ESTE TEST CAMBIO DE SENTIDO, Y ES EL QUE PROTEGE LOS MUNDOS.
    //
    // ANTES comprobaba que un ID viejo significara LO MISMO hoy, porque la
    // codificacion no habia cambiado.
    //
    // AHORA si cambio: el relleno gano una altura propia dentro del ID (para
    // que una celda pueda estar a medio llenar con dos materiales a la
    // vista). Los dos esquemas comparten rango, asi que un numero escrito con
    // el viejo se decodifica MAL con el nuevo.
    //
    // Por eso ya no se exige que signifique lo mismo, sino que la MIGRACION
    // lo traduzca correctamente. Sin ella, cada celda mixta de un mundo
    // guardado se convertiria en otro par de materiales al cargarlo.
    const int ib = indiceFamilia(BLOCK_DIRT);
    const int ir = 3;                       // arena: 4ª de la tabla, índice 3
    REQUIRE(ib >= 0);
    REQUIRE(indiceFamilia(BLOCK_SAND) == ir);

    // El ID tal y como lo escribia el codigo anterior.
    //
    // ⚠️ CON EL ANCHO VIEJO (22), NO con `FAMILIAS_CON_NIVEL`. Esa constante
    // subio a 24 al entrar el techo de hojas en la tabla de relleno, y usarla
    // aqui construiria un ID que NUNCA estuvo en ningun mundo guardado -- el
    // test comprobaria la migracion de un numero inventado.
    //
    // Este 22 describe lo que hay en disco y no se toca aunque la constante
    // vuelva a crecer. Es la misma razon por la que BlockCompat lleva su
    // propia copia (FAMILIAS_VIEJAS).
    constexpr int FAMILIAS_VIEJAS = 22;
    const BlockType guardado = (BlockType)(BLOCK_MIXTO_BASE +
                               (ib * 7 + (3 - 1)) * FAMILIAS_VIEJAS + ir);
    REQUIRE(esMixto(guardado));

    // Al cargarlo se traduce, y SOLO entonces significa lo que significaba.
    const BlockType migrado = BlockCompat::mixtoAlturaRelleno(guardado);

    CHECK(esMixto(migrado));
    CHECK(mixtoBase(migrado) == BLOCK_DIRT);
    CHECK(mixtoNivel(migrado) == 3);
    CHECK(mixtoRelleno(migrado) == BLOCK_SAND);

    // El relleno viejo llegaba al TECHO, asi que su altura real eran los 5
    // octavos que quedaban libres. La celda sigue estando llena.
    CHECK(mixtoNivelRelleno(migrado) == 5);
    CHECK(mixtoOctavosTotales(migrado) == 8);
    CHECK(alturaDe(migrado) == doctest::Approx(1.0f));

    // Y es exactamente el que produce el codigo de hoy para ese bloque.
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_SAND, 5) == migrado);
}

TEST_CASE("Mixta: migrar es seguro para TODOS los IDs viejos") {
    // Se barre el rango mixto VIEJO entero. Ninguna traduccion puede salirse
    // del rango ni producir un bloque imposible: si una sola fallara, esa
    // celda se leeria como basura en todos los mundos guardados.
    // ⚠️ El rango VIEJO se calcula con el ancho VIEJO (22). Con el de hoy
    // (24) se barrerian IDs que nunca existieron en ningun mundo guardado.
    constexpr int FAMILIAS_VIEJAS = 22;
    const int FIN_VIEJO = BLOCK_MIXTO_BASE +
                          FAMILIAS_VIEJAS * 7 * FAMILIAS_VIEJAS;

    int traducidos = 0;
    for (int id = BLOCK_MIXTO_BASE; id < FIN_VIEJO; ++id) {
        const BlockType migrado = BlockCompat::mixtoAlturaRelleno((BlockType)id);

        // O se traduce a una mixta valida, o se deja igual (par imposible).
        if (migrado == (BlockType)id) continue;

        INFO("id viejo ", id);
        CHECK(esMixto(migrado));
        CHECK((int)migrado < BLOCK_MIXTO_FIN);
        // La celda migrada sigue llena: era lo que significaba el ID viejo.
        CHECK(mixtoOctavosTotales(migrado) == 8);
        ++traducidos;
    }

    // Si no se tradujo nada, el test no estaria comprobando nada.
    CHECK(traducidos > 100);
}

TEST_CASE("Mixta: el relleno puede ocupar menos que el techo") {
    // ⭐ LA NOVEDAD, y lo que arregla el bug reportado.
    //
    // Antes "3 de pasto + 2 de arena" no se podia representar: una mixta era
    // siempre 8 octavos, asi que la fusion devolvia 5 de PASTO y el jugador
    // veia su pasto estirarse sin que la arena apareciera hasta completar el
    // bloque.
    const BlockType m = mixto(BLOCK_GRASS, 3, BLOCK_SAND, 2);
    REQUIRE(m != BLOCK_AIR);
    REQUIRE(esMixto(m));

    CHECK(mixtoBase(m) == BLOCK_GRASS);
    CHECK(mixtoNivel(m) == 3);
    CHECK(mixtoRelleno(m) == BLOCK_SAND);
    CHECK(mixtoNivelRelleno(m) == 2);

    // Cinco octavos en total: la celda NO esta llena.
    CHECK(mixtoOctavosTotales(m) == 5);
    CHECK(alturaDe(m) < 1.0f);

    // Y las dos capas van PEGADAS: el relleno empieza justo donde acaba la
    // base. Sin aire entre ellas, que es lo que se pidio.
    CHECK(alturaBaseMixto(m) == doctest::Approx(alturaNivelPx(3) / 16.0f));
}

TEST_CASE("Mixta: a medio llenar NO tapa las caras de al lado") {
    // Si una mixta de 5 octavos dijera que es opaca, taparia la cara del
    // bloque vecino y se verian agujeros en el terreno -- el mismo motivo
    // por el que un nivel parcial tampoco tapa.
    const BlockType media = mixto(BLOCK_GRASS, 3, BLOCK_SAND, 2);
    const BlockType llena = mixto(BLOCK_GRASS, 3, BLOCK_SAND, 5);
    REQUIRE(media != BLOCK_AIR);
    REQUIRE(llena != BLOCK_AIR);

    CHECK(mixtoOctavosTotales(media) == 5);
    CHECK(mixtoOctavosTotales(llena) == 8);
    // La comprobacion de opacidad vive en main.cpp y el test no lo enlaza;
    // aqui se fija el DATO del que depende, que es lo que ese codigo mira.
    CHECK(alturaDe(media) < 1.0f);
    CHECK(alturaDe(llena) == doctest::Approx(1.0f));
}

TEST_CASE("Mixta: las tablas caben en el hueco reservado") {
    // Si una tabla creciera por encima de FAMILIAS_CON_NIVEL, dos parejas
    // distintas de materiales darían el MISMO ID y los mundos se corromperían.
    CHECK(familiasCaben());

    int nRel = 0; const BlockType* R = tablaRelleno(nRel);
    CHECK(nRel <= FAMILIAS_CON_NIVEL);

    // ⭐ ESTE AVISO SE CUMPLIO, Y CONVIENE QUE QUEDE ESCRITO COMO FUE.
    //
    // Decia: "para anadir otro relleno hay que ampliar el hueco, y eso SI
    // exige migrar los saves". Es exactamente lo que paso al entrar el TECHO
    // DE HOJAS en la posicion 22: se amplio FAMILIAS_CON_NIVEL de 22 a 24 y
    // se escribio la migracion (BlockCompat::mixtoAlturaRelleno), que ademas
    // lleva su propia copia del ancho viejo para no decodificar mal.
    //
    // Hoy son 23 entradas con hueco para una mas antes de la proxima.
    CHECK(nRel == 23);
    CHECK(nRel <= FAMILIAS_CON_NIVEL);
    CHECK(R[22] == BLOCK_TECHO_HOJAS);
}

TEST_CASE("Mixta: todo ID generado cae dentro del rango reservado") {
    // Un ID que se saliera del rango dejaría de reconocerse como mixto y la
    // celda se leería como un bloque cualquiera.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    int nRel = 0; const BlockType* R = tablaRelleno(nRel);

    // Las familias y rellenos retirados (los cinco minerales de veta) se
    // saltan: ya no tienen bloque, asi que mixto() los rechaza a proposito.
    // Su HUECO sigue en las tablas y eso se comprueba aparte.
    for (int i = 0; i < nNiv; ++i) {
        if (T[i].entero == BLOCK_AIR) continue;
        for (int nivel = 1; nivel <= 7; ++nivel)
            for (int j = 0; j < nRel; ++j) {
                if (R[j] == BLOCK_AIR) continue;
                const BlockType celda = mixto(T[i].entero, nivel, R[j]);
                REQUIRE(celda != BLOCK_AIR);
                CHECK((int)celda >= BLOCK_MIXTO_BASE);
                CHECK((int)celda < BLOCK_MIXTO_FIN);
                CHECK(esMixto(celda));
            }
    }
}

TEST_CASE("Mixta: no hay dos combinaciones con el mismo ID") {
    // Colisión de IDs = dos materiales distintos que se leen como el mismo.
    // Se recorre el producto completo y se comprueba que la ida y la vuelta
    // coinciden siempre, que es lo mismo que decir que la codificación es
    // inyectiva.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    int nRel = 0; const BlockType* R = tablaRelleno(nRel);

    for (int i = 0; i < nNiv; ++i) {
        if (T[i].entero == BLOCK_AIR) continue;   // familia retirada
        for (int nivel = 1; nivel <= 7; ++nivel)
            for (int j = 0; j < nRel; ++j) {
                if (R[j] == BLOCK_AIR) continue;  // relleno retirado
                const BlockType celda = mixto(T[i].entero, nivel, R[j]);
                REQUIRE(celda != BLOCK_AIR);
                CHECK(mixtoBase(celda)    == T[i].entero);
                CHECK(mixtoNivel(celda)   == nivel);
                CHECK(mixtoRelleno(celda) == R[j]);
            }
    }
}

// ----------------------------------------------------------------------------
// LO QUE NO DEBE CAMBIAR
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: la madera sigue sin partirse en lonchas") {
    // El motivo por el que la madera salió de tablaNiveles() sigue vigente:
    // un tronco es un tronco entero. Poder RELLENAR no es tener niveles.
    CHECK(primerNivelDe(BLOCK_WOOD) == BLOCK_AIR);
    // ⚠️ EL TABLON YA NO: volvio a tablaNiveles para poder juntarse con las
    // capas (ver la nota alli). El argumento de "un tronco entero" vale para
    // el TRONCO, no para una tabla de construccion.
    CHECK(primerNivelDe(BLOCK_PLANKS) == BLOCK_PLANKS_L1);
    CHECK(indiceFamilia(BLOCK_WOOD) == -1);   // no puede ser la capa de abajo
    CHECK(indiceRelleno(BLOCK_WOOD) >= 0);    // pero sí el relleno de arriba

    // conNivel sobre madera devuelve el bloque entero, nunca una loncha.
    CHECK(conNivel(BLOCK_WOOD, 1) == BLOCK_WOOD);
    CHECK(conNivel(BLOCK_WOOD, 4) == BLOCK_WOOD);
    CHECK(alturaDe(BLOCK_WOOD) == doctest::Approx(1.0f));
}

TEST_CASE("Mixta: lo que no puede rellenar sigue sin poder") {
    // Plantas, agua y lava no rellenan: no son macizos. Devolver BLOCK_AIR es
    // la señal de "esta pareja no se representa", y el código que coloca la
    // usa para caer a su camino alternativo.
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_WATER)     == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_LAVA)      == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_TALLGRASS) == BLOCK_AIR);

    // Y la capa de abajo sigue exigiendo niveles: un tronco no puede serlo.
    CHECK(mixto(BLOCK_WOOD, 3, BLOCK_DIRT) == BLOCK_AIR);

    // El nivel 8 es la celda llena, no una mixta.
    CHECK(mixto(BLOCK_DIRT, 8, BLOCK_WOOD) == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 0, BLOCK_WOOD) == BLOCK_AIR);
}

// ============================================================================
// LA HITBOX DE UNA MIXTA SIGUE SU ALTURA REAL
// ============================================================================
// BUG REPORTADO: "las hitboxes de los niveles se convierten en la hitbox de
// bloque completo".
//
// LA CAUSA. La rama que calcula la caja entraba solo con `esNivelParcial`, y
// una celda MIXTA no es un nivel parcial: es su propia categoria. Asi que al
// combinar dos materiales --3 de tierra + 2 de arena = 5 octavos-- la celda
// caia al caso por defecto y recibia la caja del CUBO ENTERO.
//
// Lo que se VE son 5 octavos; lo que se TOCA era un bloque completo. El
// jugador se subia a una altura que no existe y lo que ponia encima flotaba.
// ============================================================================

TEST_CASE("Hitbox: una mixta a medio llenar mide lo que ocupa") {
    // ⭐ De `alturaDe` sale la caja: es la colision EXACTA del bloque.
    const BlockType m5 = mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2);   // 5 octavos
    REQUIRE(m5 != BLOCK_AIR);

    CHECK(mixtoOctavosTotales(m5) == 5);
    CHECK(alturaDe(m5) == doctest::Approx(alturaNivelPx(5) / 16.0f));

    // Y NO es la del cubo entero, que es justo el bug.
    CHECK(alturaDe(m5) < 1.0f);
}

TEST_CASE("Hitbox: la altura crece con las capas, como si fueran niveles") {
    // "que solamente se aumente el tamaño de las combinaciones de niveles y
    // se mantenga como si fuera solamente niveles acumulados".
    //
    // Una mixta de N octavos tiene que medir EXACTAMENTE lo mismo que un
    // nivel parcial de N octavos: la combinacion no cambia las reglas, solo
    // permite dos materiales.
    for (int base = 1; base <= 6; ++base) {
        for (int rel = 1; base + rel <= 8; ++rel) {
            const BlockType m = mixto(BLOCK_DIRT, base, BLOCK_SAND, rel);
            if (m == BLOCK_AIR) continue;

            const int oct = base + rel;
            INFO("base ", base, " + relleno ", rel, " = ", oct, " octavos");

            CHECK(mixtoOctavosTotales(m) == oct);

            // La misma altura que tendria un nivel suelto de ese tamano.
            const float esperada = (oct >= 8)
                                 ? 1.0f
                                 : alturaNivelPx(oct) / 16.0f;
            CHECK(alturaDe(m) == doctest::Approx(esperada));
        }
    }
}

TEST_CASE("Hitbox: una mixta LLENA si mide un bloque entero") {
    // La contrapartida: lo que ya funcionaba no se rompe. Si las dos capas
    // suman 8, la celda esta llena y se anda por encima como por un cubo.
    const BlockType lleno = mixto(BLOCK_DIRT, 3, BLOCK_SAND, 5);
    REQUIRE(lleno != BLOCK_AIR);

    CHECK(mixtoOctavosTotales(lleno) == 8);
    CHECK(alturaDe(lleno) == doctest::Approx(1.0f));
    CHECK(celdaLlena(lleno));
}

TEST_CASE("Hitbox: lo que se ve y lo que se toca coinciden") {
    // La propiedad de fondo: la caja sale de `alturaDe`, y el mesher dibuja
    // la capa de arriba hasta `alturaDe` tambien (ver "EL RELLENO YA NO LLEGA
    // SIEMPRE AL TECHO"). Si las dos cuentas divergieran, volveria el bug.
    for (int base = 1; base <= 7; ++base) {
        for (int rel = 1; base + rel <= 8; ++rel) {
            const BlockType m = mixto(BLOCK_GRAVEL, base, BLOCK_STONE, rel);
            if (m == BLOCK_AIR) continue;

            INFO("base ", base, " relleno ", rel);

            // Donde acaba la capa de abajo = donde empieza el relleno.
            // Sin aire entre ellas.
            CHECK(alturaBaseMixto(m) ==
                  doctest::Approx(alturaNivelPx(base) / 16.0f));

            // Y el tope de la celda es donde acaba el relleno.
            CHECK(alturaDe(m) > alturaBaseMixto(m));
        }
    }
}

TEST_CASE("Hitbox: las dos capas conservan su material por separado") {
    // "que se renderizen las texturas sin problemas y por separado del
    // bloque": cada capa tiene que poder decir cual es su material, o el
    // mesher no sabria que textura pedir.
    const BlockType m = mixto(BLOCK_GRASS, 2, BLOCK_GRAVEL, 3);
    REQUIRE(m != BLOCK_AIR);

    CHECK(mixtoBase(m) == BLOCK_GRASS);
    CHECK(mixtoNivel(m) == 2);
    CHECK(mixtoRelleno(m) == BLOCK_GRAVEL);
    CHECK(mixtoNivelRelleno(m) == 3);

    // Y son materiales distintos: si colapsaran a uno, se veria una sola
    // textura estirada -- el sintoma viejo.
    CHECK(mixtoBase(m) != mixtoRelleno(m));
}
