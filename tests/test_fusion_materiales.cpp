#include <doctest/doctest.h>
#include "BlockType.h"
#include <set>
#include <vector>

// ============================================================================
// JUNTAR NIVELES DE MATERIALES DISTINTOS
// ============================================================================
// LO QUE SE PIDIO, en dos partes:
//
//   1. "que cuando dejas caer 2 o miles mas de tipos de niveles, puedan
//      juntarse infinitamente en millones de combinaciones"
//   2. "que cuando colocas un nivel diferente en un nivel distinto, el nivel
//      colocado al de abajo no se convierta en bloque, que aun sea el mismo
//      nivel"
//
// ----------------------------------------------------------------------------
// LOS DOS BUGS QUE HABIA
// ----------------------------------------------------------------------------
// `fusionarNiveles` hacia, para CUALQUIER pareja:
//
//     const BlockType material = bloqueBaseDe(enDestino);
//
// o sea, el resultado siempre llevaba el material de ABAJO. Arena cayendo
// sobre tierra daba mas tierra: el material que caia se perdia. Y cuando la
// suma daba 8 devolvia `bloqueBaseDe(material)` -- un BLOQUE ENTERO, que es
// justo lo que se pidio evitar.
//
// Al colocar pasaba lo mismo por otro camino (main.cpp): se llamaba a
//
//     mixto(baseAbajo, nivelAbajo, bloqueBaseDe(blockToPlace))
//
// y el relleno de una celda mixta va SIEMPRE de la capa al techo, asi que un
// nivel 1 de arena se estiraba hasta ocupar los 7 octavos libres.
//
// ----------------------------------------------------------------------------
// POR QUE ESTO NO LO CAZO NADIE
// ----------------------------------------------------------------------------
// `fusionarNiveles` no tenia NI UN TEST. Este archivo es el que faltaba.
// ============================================================================

namespace {

// Los materiales con nivel que se usan en las pruebas. Se eligen de familias
// distintas a proposito: el bug solo aparece cuando los dos NO coinciden.
constexpr BlockType TIERRA = BLOCK_DIRT;
constexpr BlockType ARENA  = BLOCK_SAND;
constexpr BlockType GRAVA  = BLOCK_GRAVEL;

} // namespace

TEST_CASE("Fusion: mismo material, se suman las alturas") {
    // El caso que ya funcionaba. Se fija para que el cambio no lo rompa.
    const Fusion f = fusionarNiveles(conNivel(TIERRA, 3), conNivel(TIERRA, 2));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(nivelDe(f.resultado) == 5);
    CHECK(bloqueBaseDe(f.resultado) == TIERRA);
    CHECK(f.sobrante == BLOCK_AIR);
}

TEST_CASE("Fusion: mismo material, lo que pasa de 8 sube") {
    const Fusion f = fusionarNiveles(conNivel(TIERRA, 5), conNivel(TIERRA, 6));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    // 11 = una celda llena (8) + 3 arriba.
    CHECK(f.resultado == bloqueBaseDe(TIERRA));
    CHECK(nivelDe(f.sobrante) == 3);
    CHECK(bloqueBaseDe(f.sobrante) == TIERRA);
}

TEST_CASE("Fusion: materiales distintos, CADA UNO conserva el suyo") {
    // ⭐ EL BUG PRINCIPAL. Antes esto daba tierra y la arena desaparecia.
    const Fusion f = fusionarNiveles(conNivel(ARENA, 3), conNivel(TIERRA, 5));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    REQUIRE(esMixto(f.resultado));

    // Abajo la tierra, con SU nivel intacto.
    CHECK(mixtoBase(f.resultado) == TIERRA);
    CHECK(mixtoNivel(f.resultado) == 5);
    // Y arriba la arena, que es lo que cayo.
    CHECK(mixtoRelleno(f.resultado) == ARENA);

    // 5 + 3 = 8: cabe justo, no sobra nada.
    CHECK(f.sobrante == BLOCK_AIR);
}

TEST_CASE("Fusion: materiales distintos, el nivel NO se vuelve bloque entero") {
    // ⭐ LA SEGUNDA MITAD DE LO QUE SE PIDIO.
    //
    // Con la suma justo en 8, el codigo viejo devolvia `bloqueBaseDe(...)`, o
    // sea un bloque entero de un solo material. Ahora devuelve una celda mixta
    // que sigue teniendo las dos capas por separado.
    const Fusion f = fusionarNiveles(conNivel(ARENA, 4), conNivel(TIERRA, 4));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(esMixto(f.resultado));
    CHECK_FALSE(f.resultado == bloqueBaseDe(TIERRA));
    CHECK_FALSE(f.resultado == bloqueBaseDe(ARENA));

    // Y las dos capas siguen siendo distinguibles dentro de la celda.
    CHECK(mixtoBase(f.resultado)    == TIERRA);
    CHECK(mixtoNivel(f.resultado)   == 4);
    CHECK(mixtoRelleno(f.resultado) == ARENA);
}

TEST_CASE("Fusion: por debajo de 8 se SUMAN en la misma celda, sin flotar") {
    // ⭐⭐ ESTE TEST CAMBIO DE SENTIDO POR UN BUG VISIBLE EN PANTALLA.
    //
    // La version anterior separaba los materiales en DOS celdas para que cada
    // uno conservara su identidad: 4 de tierra abajo, 3 de arena arriba. El
    // volumen se conservaba y sobre el papel estaba bien.
    //
    // En pantalla estaba mal. Los niveles de una celda se dibujan desde SU
    // PROPIO SUELO, asi que esos 3 de arena no se apoyaban sobre los 4 de
    // tierra: nacian en el suelo de la celda de arriba, 16 px mas alto. Entre
    // los dos quedaban 13 px de aire y la arena se veia FLOTANDO.
    //
    // AHORA se suman en la MISMA celda y el resultado conserva el material de
    // abajo, que es el que estaba primero:
    //
    //     4 de tierra + 3 de arena  ->  7 de TIERRA, pegado al suelo
    //
    // De las tres salidas posibles --flotar, inventar material para llenar la
    // mixta, o perder la identidad del de arriba-- es la unica que no rompe
    // ni lo que se ve ni el invariante del volumen.
    const Fusion f = fusionarNiveles(conNivel(ARENA, 3), conNivel(TIERRA, 4));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

    // ⭐⭐ ACTUALIZADO: AHORA LOS DOS MATERIALES SE CONSERVAN.
    //
    // Cuando se escribio este caso, una celda mixta solo sabia representar 8
    // octavos exactos, asi que con total 7 habia que elegir: o flotar, o
    // inventar un octavo, o perder la identidad del de arriba. Se eligio lo
    // ultimo y el resultado era "7 de TIERRA".
    //
    // Desde que el relleno lleva su propia altura dentro del ID, la celda
    // puede guardar las dos capas tal cual. Ya no hay que elegir: la arena se
    // ve, nada flota y el volumen se conserva.
    REQUIRE(esMixto(f.resultado));

    CHECK(mixtoBase(f.resultado) == TIERRA);
    CHECK(mixtoNivel(f.resultado) == 4);
    CHECK(mixtoRelleno(f.resultado) == ARENA);
    CHECK(mixtoNivelRelleno(f.resultado) == 3);

    // Siete octavos en total: la celda NO esta llena, y es correcto.
    CHECK(mixtoOctavosTotales(f.resultado) == 7);

    // ⭐ Y NADA SUBE: si subiera algo, volveria a flotar.
    CHECK_MESSAGE(f.sobrante == BLOCK_AIR,
                  "si sube un sobrante con la celda a medio llenar, "
                  "vuelve el nivel flotante");
}

TEST_CASE("Fusion: nada flota NUNCA en toda la matriz de niveles") {
    // ⭐ LA PROPIEDAD QUE PROTEGE EL BUG REPORTADO.
    //
    // La regla es simple: solo puede subir un sobrante si la celda de abajo
    // queda LLENA. Si queda a medias y ademas sube algo, ese algo nace en el
    // suelo de la celda de arriba y se ve flotando con aire debajo.
    //
    // Se barren todas las combinaciones de materiales y niveles.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA, BLOCK_STONE };

    for (BlockType a : MATS) {
        for (BlockType b : MATS) {
            for (int na = 1; na <= 8; ++na) {
                for (int nb = 1; nb <= 8; ++nb) {
                    const BlockType cae     = (na == 8) ? a : conNivel(a, na);
                    const BlockType destino = (nb == 8) ? b : conNivel(b, nb);

                    const Fusion f = fusionarNiveles(cae, destino);
                    if (f.tipo != ResultadoFusion::FUSIONADOS) continue;
                    if (f.sobrante == BLOCK_AIR) continue;   // no sube nada: ok

                    // Si sube algo, lo de abajo TIENE que estar lleno.
                    const bool abajoLleno = esMixto(f.resultado) ||
                                            !esNivelParcial(f.resultado);

                    INFO("cae ", (int)a, " nivel ", na,
                         " sobre ", (int)b, " nivel ", nb);
                    CHECK_MESSAGE(abajoLleno,
                                  "sube un sobrante con la celda de abajo a "
                                  "medio llenar: el nivel de arriba flota");
                }
            }
        }
    }
}

TEST_CASE("Fusion: lo que no cabe SUBE con su material, no se estira") {
    // tierra(6) abajo deja hueco para 2. Caen 5 de arena: 2 se quedan, 3 suben.
    const Fusion f = fusionarNiveles(conNivel(ARENA, 5), conNivel(TIERRA, 6));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    REQUIRE(esMixto(f.resultado));
    CHECK(mixtoBase(f.resultado) == TIERRA);
    CHECK(mixtoNivel(f.resultado) == 6);
    CHECK(mixtoRelleno(f.resultado) == ARENA);

    // El sobrante conserva el material del que CAE, no el de abajo.
    REQUIRE(f.sobrante != BLOCK_AIR);
    CHECK(bloqueBaseDe(f.sobrante) == ARENA);
    CHECK(nivelDe(f.sobrante) == 3);
}

TEST_CASE("Fusion: ni un octavo se pierde ni se inventa") {
    // ⭐ LA CONSERVACION DE MATERIA, sobre TODAS las combinaciones.
    //
    // Es la propiedad que de verdad importa: apilar no puede crear ni destruir
    // material. Si se pierde, el jugador tira bloques y desaparecen; si se
    // crea, se duplican.
    for (int nAbajo = 1; nAbajo <= 7; ++nAbajo) {
        for (int nCae = 1; nCae <= 7; ++nCae) {
            const Fusion f = fusionarNiveles(conNivel(ARENA, nCae),
                                             conNivel(TIERRA, nAbajo));
            REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

            // Cuanto material hay en la celda de destino tras fusionar.
            //
            // ⚠️ UNA MIXTA YA NO ES SIEMPRE 8. Desde que el relleno lleva su
            // propia altura, una celda puede guardar "3 de tierra + 2 de
            // arena" = 5 octavos, con hueco por arriba. Contarla como 8 aqui
            // daba un falso fallo de conservacion.
            int enCelda = 0;
            if (esMixto(f.resultado)) {
                enCelda = mixtoOctavosTotales(f.resultado);
            } else if (esNivelParcial(f.resultado)) {
                enCelda = nivelDe(f.resultado);
            } else {
                enCelda = 8;                       // bloque entero
            }

            int arriba = 0;
            if (f.sobrante != BLOCK_AIR) {
                arriba = esNivelParcial(f.sobrante) ? nivelDe(f.sobrante) : 8;
            }

            INFO("abajo=", nAbajo, " cae=", nCae);
            CHECK(enCelda + arriba == nAbajo + nCae);
        }
    }
}

TEST_CASE("Fusion: apilar tres materiales distintos llena y desborda") {
    // ⭐ EL APILADO "INFINITO".
    //
    // Una celda solo guarda DOS materiales. El tercero no se pierde: sube. Eso
    // es lo que permite apilar cuantos tipos se quiera -- el limite es la
    // altura del mundo, no el formato de la celda.
    //
    // Se simula una columna: cada material que cae se fusiona con la celda de
    // abajo, y lo que sobra pasa a ser lo que cae en la celda siguiente.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA, BLOCK_SNOW };

    BlockType celda = conNivel(MATS[0], 4);
    BlockType subiendo = BLOCK_AIR;
    int celdasUsadas = 1;

    for (int i = 1; i < 4; ++i) {
        const Fusion f = fusionarNiveles(conNivel(MATS[i], 5), celda);
        if (f.tipo != ResultadoFusion::FUSIONADOS) break;

        subiendo = f.sobrante;
        if (subiendo != BLOCK_AIR) {
            // La pila crece: lo que no cupo abre celda nueva.
            ++celdasUsadas;
            celda = subiendo;
        }
    }

    // Con cuatro materiales de 4-5 octavos cada uno, la pila NO cabe en una
    // sola celda: tiene que haber crecido.
    CHECK(celdasUsadas > 1);
}

TEST_CASE("Fusion: muchas parejas distintas dan celdas distintas") {
    // "millones de combinaciones": lo que se comprueba aqui es que el sistema
    // no colapsa parejas distintas en el mismo ID. Cada (material abajo, nivel,
    // material arriba) tiene que ser una celda propia y distinguible.
    //
    // No son millones porque las tablas tienen 22 familias, pero la propiedad
    // que importa --que no se pisen-- es la misma a cualquier escala.
    std::set<int> vistos;
    int parejas = 0;

    int nNiv = 0; const FamiliaNivel* TN = tablaNiveles(nNiv);
    int nRel = 0; const BlockType*    TR = tablaRelleno(nRel);

    for (int i = 0; i < nNiv; ++i) {
        for (int nivel = 1; nivel <= 7; ++nivel) {
            for (int j = 0; j < nRel; ++j) {
                const BlockType m = mixto(TN[i].entero, nivel, TR[j]);
                if (m == BLOCK_AIR) continue;
                ++parejas;

                // Ni un ID repetido: dos parejas distintas no pueden dar la
                // misma celda, o al cargar el mundo saldria otro material.
                INFO("base=", (int)TN[i].entero, " nivel=", nivel,
                     " relleno=", (int)TR[j]);
                CHECK(vistos.insert((int)m).second);

                // Y se tiene que poder recuperar lo que se metio.
                CHECK(mixtoBase(m)    == bloqueBaseDe(TN[i].entero));
                CHECK(mixtoNivel(m)   == nivel);
                CHECK(mixtoRelleno(m) == bloqueBaseDe(TR[j]));
            }
        }
    }

    // Que la prueba haya recorrido algo de verdad.
    CHECK(parejas > 500);
}

TEST_CASE("Fusion: el agua se desplaza, no se mezcla") {
    // Regla de siempre, que el cambio no puede haber tocado: un solido que cae
    // en agua la desplaza (el llamante la reparte a los lados).
    const Fusion f = fusionarNiveles(conNivel(ARENA, 3), BLOCK_WATER);

    CHECK(f.tipo == ResultadoFusion::SOLO_EL_QUE_CAE);
    CHECK(f.resultado == conNivel(ARENA, 3));
}

TEST_CASE("Fusion: un bloque ENTERO sobre una capa SI se junta") {
    // ⭐ CAMBIO DE REGLA, Y ESTE TEST DECIA LO CONTRARIO.
    //
    // Antes un bloque entero devolvia NO_FUSIONA y se quedaba ENCIMA de la
    // capa: entre los 3 px de tierra y los 16 del cubo quedaban 13 px de aire
    // a la vista, y el bloque se veia FLOTANDO. No habia arreglo posible desde
    // fuera, porque una celda no puede estar a media altura.
    //
    // Ahora el entero cuenta como OCHO OCTAVOS y entra en la misma aritmetica:
    //
    //     tierra 3/8  +  piedra 8/8  =  11/8
    //       -> la celda de abajo queda LLENA
    //       -> los 3/8 que sobran SUBEN como piedra
    const Fusion f = fusionarNiveles(BLOCK_STONE, conNivel(TIERRA, 3));

    CHECK(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(f.resultado != BLOCK_AIR);          // abajo queda algo lleno
    CHECK(f.sobrante  != BLOCK_AIR);          // y algo sube
    // Lo que sube conserva SU material: es piedra, no tierra.
    CHECK(bloqueBaseDe(f.sobrante) == BLOCK_STONE);
    // Y es exactamente lo que no cabia: 11 - 8 = 3 octavos.
    CHECK(nivelDe(f.sobrante) == 3);
}

TEST_CASE("Fusion: sobre una celda LLENA, el que cae sube entero") {
    // ⭐⭐ ESTE TEST CAMBIO DE SENTIDO A PROPOSITO.
    //
    // ANTES decia que 8 + 8 devolvia NO_FUSIONA ("no hay nada que repartir").
    // Pero NO_FUSIONA significa "el que cae se queda encima sin tocar nada", y
    // el llamante lo trataba como el final de la cadena: ahi se cortaba el
    // apilado. Esa era una de las BARRERAS que se pidio quitar.
    //
    // AHORA se responde SUBE_ENTERO: "aqui no cabe, ponlo arriba con su nivel
    // y su material intactos". El resultado visible es el mismo --el bloque
    // acaba en la celda de encima-- pero la cadena NO se corta: arriba vuelve
    // a fusionarse con lo que haya, y la columna sigue creciendo sin tope.
    const Fusion f = fusionarNiveles(BLOCK_STONE, BLOCK_DIRT);
    CHECK(f.tipo == ResultadoFusion::SUBE_ENTERO);

    // Lo de abajo no se toca y lo que cae sube tal cual: ni un octavo se
    // pierde ni se inventa, que es el invariante de siempre.
    CHECK(f.resultado == BLOCK_DIRT);
    CHECK(f.sobrante  == BLOCK_STONE);
}

TEST_CASE("Fusion: una celda MIXTA tampoco corta la cadena") {
    // El caso que mas se nota al construir: una celda que acaba de quedar
    // mixta (dos materiales, llena) rechazaba todo lo que se le pusiera
    // encima. El jugador veia que esa columna "dejaba de funcionar" mientras
    // las de al lado seguian apilando: la barrera, literalmente.
    const BlockType celdaMixta = mixto(TIERRA, 3, ARENA);
    REQUIRE(celdaMixta != BLOCK_AIR);
    REQUIRE(esMixto(celdaMixta));

    // Le cae encima una capa de grava de 4 octavos.
    const Fusion f = fusionarNiveles(conNivel(GRAVA, 4), celdaMixta);
    CHECK(f.tipo == ResultadoFusion::SUBE_ENTERO);

    // La mixta se queda intacta y la grava sube con SU nivel y SU material.
    CHECK(f.resultado == celdaMixta);
    CHECK(bloqueBaseDe(f.sobrante) == GRAVA);
    CHECK(nivelDe(f.sobrante) == 4);
}

TEST_CASE("Fusion: sobre celda llena nunca se pierde material") {
    // Se barren las ocho alturas y varios materiales: pase lo que pase, lo
    // que cae reaparece arriba con el mismo volumen. Es la version de este
    // caso del invariante "ni un octavo se pierde ni se inventa".
    const BlockType LLENOS[] = { BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND,
                                 mixto(TIERRA, 5, ARENA) };
    for (BlockType destino : LLENOS) {
        if (destino == BLOCK_AIR) continue;
        REQUIRE_FALSE(esNivelParcial(destino));

        for (int n = 1; n <= 7; ++n) {
            const BlockType cae = conNivel(GRAVA, n);
            const Fusion f = fusionarNiveles(cae, destino);

            INFO("destino lleno, cae grava de ", n, " octavos");
            REQUIRE(f.tipo == ResultadoFusion::SUBE_ENTERO);
            CHECK(f.resultado == destino);          // abajo intacto
            CHECK(nivelDe(f.sobrante) == n);        // y arriba, lo mismo
            CHECK(bloqueBaseDe(f.sobrante) == GRAVA);
        }

        // Y un bloque entero cayendo sobre otro lleno: igual.
        const Fusion fe = fusionarNiveles(BLOCK_STONE, destino);
        CHECK(fe.tipo == ResultadoFusion::SUBE_ENTERO);
        CHECK(fe.sobrante == BLOCK_STONE);
    }
}

TEST_CASE("Fusion: sin barreras, los 8 niveles valen contra los 8") {
    // ⭐ LO QUE SE PIDIO: "que todos los niveles de 1-8 puedan combinarse sin
    // limite y no creen barreras".
    //
    // Se barre la matriz COMPLETA: cada nivel de cada material contra cada
    // nivel del otro. Ninguna combinacion puede devolver NO_FUSIONA, que es
    // la respuesta que corta la cadena.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA, BLOCK_STONE };

    for (BlockType a : MATS) {
        for (BlockType b : MATS) {
            for (int na = 1; na <= 8; ++na) {
                for (int nb = 1; nb <= 8; ++nb) {
                    const BlockType cae     = (na == 8) ? a : conNivel(a, na);
                    const BlockType destino = (nb == 8) ? b : conNivel(b, nb);

                    const Fusion f = fusionarNiveles(cae, destino);

                    INFO("cae ", (int)a, " nivel ", na,
                         " sobre ", (int)b, " nivel ", nb);
                    CHECK_MESSAGE(f.tipo != ResultadoFusion::NO_FUSIONA,
                                  "esta combinacion crea una barrera");
                }
            }
        }
    }
}

TEST_CASE("Fusion: la materia se conserva en TODA la matriz de niveles") {
    // El invariante, aplicado a las 8x8 combinaciones. Lo que entra (lo que
    // cae mas lo que habia) tiene que salir (lo que queda abajo mas lo que
    // sube). Un fallo aqui es material que aparece o desaparece al construir.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA };

    auto octavos = [](BlockType t) -> int {
        if (t == BLOCK_AIR) return 0;
        // ⭐ Una mixta YA NO esta siempre llena: su relleno lleva altura
        // propia, asi que puede ocupar menos de 8 octavos.
        if (esMixto(t)) return mixtoOctavosTotales(t);
        if (esNivelParcial(t)) return nivelDe(t);
        return 8;                               // bloque entero
    };

    for (BlockType a : MATS) {
        for (BlockType b : MATS) {
            for (int na = 1; na <= 8; ++na) {
                for (int nb = 1; nb <= 8; ++nb) {
                    const BlockType cae     = (na == 8) ? a : conNivel(a, na);
                    const BlockType destino = (nb == 8) ? b : conNivel(b, nb);

                    const Fusion f = fusionarNiveles(cae, destino);
                    if (f.tipo == ResultadoFusion::NO_FUSIONA) continue;
                    if (f.tipo == ResultadoFusion::SOLO_EL_QUE_CAE) continue;

                    const int entra = octavos(cae) + octavos(destino);
                    const int sale  = octavos(f.resultado) + octavos(f.sobrante);

                    INFO("cae ", (int)a, " nivel ", na,
                         " sobre ", (int)b, " nivel ", nb);
                    CHECK(entra == sale);
                }
            }
        }
    }
}

TEST_CASE("Fusion: el entero sobre una capa NI PIERDE NI INVENTA octavos") {
    // El invariante que protege todo el apilado: la materia se conserva.
    // Se barren las siete alturas de capa posibles.
    for (int n = 1; n <= 7; ++n) {
        const Fusion f = fusionarNiveles(BLOCK_STONE, conNivel(TIERRA, n));
        REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

        // Lo que queda abajo (siempre lleno: 8) mas lo que sube.
        const int abajo  = 8;
        const int arriba = (f.sobrante == BLOCK_AIR) ? 0 : nivelDe(f.sobrante);

        INFO("capa de ", n, " octavos + bloque entero");
        CHECK(abajo + arriba == n + 8);
    }
}

// ============================================================================
// ENTRELAZADO: TODAS LAS COMBINACIONES POSIBLES
// ============================================================================
// PEDIDO: "mejora la compatibilidad de poner niveles entrelazados, juntos y de
// todas las maneras posibles juntos multiples niveles de bloques".
// ============================================================================

TEST_CASE("Entrelazado: ningun material se convierte en otro, nunca") {
    // ⭐⭐ LA PROPIEDAD QUE FALTABA.
    //
    // Antes, con total < 8, el material de arriba se convertia en el de abajo:
    // poner arena sobre tierra daba tierra. Ahora cada uno conserva su
    // identidad en TODAS las combinaciones.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA, BLOCK_STONE };

    for (BlockType abajo : MATS) {
        for (BlockType arriba : MATS) {
            if (abajo == arriba) continue;   // mismo material: se suman, es otro caso
            for (int nAbajo = 1; nAbajo <= 7; ++nAbajo) {
                for (int nCae = 1; nCae <= 7; ++nCae) {
                    const Fusion f = fusionarNiveles(conNivel(arriba, nCae),
                                                     conNivel(abajo, nAbajo));
                    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

                    INFO("abajo ", nAbajo, " + cae ", nCae);

                    // El material de ABAJO nunca desaparece.
                    if (esMixto(f.resultado)) {
                        CHECK(mixtoBase(f.resultado) == abajo);
                        CHECK(mixtoRelleno(f.resultado) == arriba);
                    } else {
                        CHECK(bloqueBaseDe(f.resultado) == abajo);
                    }

                    // Y si algo sube, sube con el material de ARRIBA.
                    if (f.sobrante != BLOCK_AIR)
                        CHECK(bloqueBaseDe(f.sobrante) == arriba);
                }
            }
        }
    }
}

TEST_CASE("Entrelazado: el volumen se conserva en las 4x4x7x7 combinaciones") {
    // La contrapartida innegociable del test anterior: conservar la identidad
    // no puede costar ni un octavo.
    const BlockType MATS[] = { TIERRA, ARENA, GRAVA, BLOCK_STONE };

    for (BlockType abajo : MATS) {
        for (BlockType arriba : MATS) {
            if (abajo == arriba) continue;
            for (int nAbajo = 1; nAbajo <= 7; ++nAbajo) {
                for (int nCae = 1; nCae <= 7; ++nCae) {
                    const Fusion f = fusionarNiveles(conNivel(arriba, nCae),
                                                     conNivel(abajo, nAbajo));
                    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

                    // Una mixta puede estar a medio llenar: su volumen es el
                    // de sus dos capas, no 8 fijo.
                    int enCelda = 0;
                    if (esMixto(f.resultado))            enCelda = mixtoOctavosTotales(f.resultado);
                    else if (esNivelParcial(f.resultado)) enCelda = nivelDe(f.resultado);
                    else if (f.resultado != BLOCK_AIR)    enCelda = 8;

                    const int arribaOct = (f.sobrante == BLOCK_AIR)
                                        ? 0
                                        : (esNivelParcial(f.sobrante)
                                           ? nivelDe(f.sobrante) : 8);

                    INFO("abajo ", nAbajo, " + cae ", nCae,
                         " -> celda ", enCelda, " + sobra ", arribaOct);
                    CHECK(enCelda + arribaOct == nAbajo + nCae);
                }
            }
        }
    }
}

// ============================================================================
// UNA MIXTA A MEDIO LLENAR NO ES UN BLOQUE COMPLETO
// ============================================================================
// BUG REPORTADO: "cuando colocas un nivel distinto al de abajo, el juego se
// piensa que es un bloque completo el nivel recien colocado".
//
// LA CAUSA. Las guardas de colocacion preguntaban por el TIPO de bloque
// ("¿es una mixta?") para decidir si la celda estaba llena. Eso valia cuando
// una mixta era por fuerza 8 octavos, pero desde que el relleno tiene altura
// propia una mixta puede ser "3 de tierra + 2 de arena" = 5, con tres libres.
//
// Resultado: colocabas un nivel de otro material, la celda pasaba a mixta, y
// el siguiente clic se iba a la celda de arriba como si ya no cupiera nada.
// ============================================================================

TEST_CASE("Mixta parcial: NO esta llena y admite mas material") {
    const BlockType media = mixto(TIERRA, 3, ARENA, 2);   // 5 octavos
    REQUIRE(media != BLOCK_AIR);
    REQUIRE(esMixto(media));

    // La cuenta de verdad: 5 de 8.
    CHECK(octavosOcupados(media) == 5);
    CHECK_FALSE(celdaLlena(media));
    CHECK(celdaAdmiteMas(media));

    // Y una mixta que SI llega a 8 esta llena, como siempre.
    const BlockType llena = mixto(TIERRA, 3, ARENA, 5);
    REQUIRE(llena != BLOCK_AIR);
    CHECK(octavosOcupados(llena) == 8);
    CHECK(celdaLlena(llena));
    CHECK_FALSE(celdaAdmiteMas(llena));
}

TEST_CASE("Mixta parcial: se le puede seguir anadiendo hasta llenarla") {
    // El caso del jugador: pone tierra, luego arena (queda mixta de 5), y
    // sigue clicando. Cada clic tiene que entrar en la MISMA celda hasta que
    // de verdad se llene.
    BlockType celda = mixto(TIERRA, 3, ARENA, 2);   // 5 octavos
    REQUIRE(celda != BLOCK_AIR);

    int vueltas = 0;
    while (!celdaLlena(celda) && vueltas < 10) {
        const Fusion f = fusionarNiveles(conNivel(ARENA, 1), celda);
        REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
        celda = f.resultado;
        ++vueltas;
    }

    // Tres octavos libres, tres clics.
    CHECK(vueltas == 3);
    CHECK(celdaLlena(celda));
    CHECK(octavosOcupados(celda) == 8);

    // Y el material de abajo no se perdio por el camino.
    CHECK(mixtoBase(celda) == TIERRA);
    CHECK(mixtoNivel(celda) == 3);
}

TEST_CASE("Mixta parcial: anadir conserva el volumen exacto") {
    // Ni un octavo de mas ni de menos al rellenar una mixta.
    for (int nBase = 1; nBase <= 6; ++nBase) {
        for (int nRel = 1; nRel + nBase < 8; ++nRel) {
            const BlockType celda = mixto(TIERRA, nBase, ARENA, nRel);
            if (celda == BLOCK_AIR) continue;

            const int antes = octavosOcupados(celda);
            REQUIRE(antes == nBase + nRel);

            for (int cae = 1; cae <= 7; ++cae) {
                const Fusion f = fusionarNiveles(conNivel(GRAVA, cae), celda);
                if (f.tipo != ResultadoFusion::FUSIONADOS) continue;

                const int despues = octavosOcupados(f.resultado) +
                                    (f.sobrante == BLOCK_AIR
                                        ? 0 : octavosOcupados(f.sobrante));

                INFO("base ", nBase, " relleno ", nRel, " cae ", cae);
                CHECK(despues == antes + cae);
            }
        }
    }
}

TEST_CASE("Entero sobre capa: se funde, no se queda encima") {
    // La otra mitad del pedido: "lo mismo los bloques enteros cuando tienen
    // niveles bajo ellos". Un cubo entero cayendo sobre una capa tiene que
    // entrar en la aritmetica como 8 octavos, no quedarse flotando encima.
    for (int n = 1; n <= 7; ++n) {
        const Fusion f = fusionarNiveles(BLOCK_STONE, conNivel(TIERRA, n));

        INFO("capa de ", n, " octavos + bloque entero");
        REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

        // La celda de abajo queda LLENA -- si no, el sobrante flotaria.
        CHECK(celdaLlena(f.resultado));

        // Y el volumen se conserva: 8 abajo + lo que suba = n + 8.
        const int total = octavosOcupados(f.resultado) +
                          (f.sobrante == BLOCK_AIR
                              ? 0 : octavosOcupados(f.sobrante));
        CHECK(total == n + 8);
    }
}

TEST_CASE("Entero sobre capa: el de abajo conserva su material") {
    // Al fundir, la capa que ya estaba no puede convertirse en otra cosa.
    const Fusion f = fusionarNiveles(BLOCK_STONE, conNivel(TIERRA, 3));
    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    REQUIRE(esMixto(f.resultado));

    CHECK(mixtoBase(f.resultado) == TIERRA);     // lo de abajo, intacto
    CHECK(mixtoNivel(f.resultado) == 3);
    CHECK(mixtoRelleno(f.resultado) == BLOCK_STONE);
}
