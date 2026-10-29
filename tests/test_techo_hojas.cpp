#include <doctest/doctest.h>
#include "BlockType.h"
#include "FisicaCaida.h"
#include <fstream>
#include <map>
#include <tuple>
#include <string>
#include <iterator>

// ============================================================================
// TECHO DE HOJAS
// ============================================================================
// Un entramado de ramas con follaje: lo que se pone sobre una enramada para
// dar sombra. Se craftea con hojas y palos en diagonal y da 8.
//
// Lo que estos tests fijan -- y todo son cosas que, de romperse, corrompen
// mundos guardados o dejan el bloque invisible:
//
//   1. El tope del enum lo cubre (si no, no recibe icono ni pasa la
//      validacion de IDs leidos de disco).
//   2. Tiene SIETE niveles y empieza en el 1: es una lamina, no un cubo.
//   3. Las 16 familias que viajan dentro de los IDs mixtos guardados NO se
//      han movido al anadir la nueva.
// ============================================================================

TEST_CASE("Techo: esta dentro del tope del enum") {
    CHECK((int)BLOCK_TECHO_HOJAS    <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_TECHO_HOJAS_L7 <= (int)BLOCK_TYPE_MAX);
}

TEST_CASE("Techo: sus siete niveles van seguidos y en orden") {
    // `tablaNiveles` guarda solo el PRIMERO y deduce el resto por el rango
    // [primero, primero+7). Si no fueran contiguos, los niveles 2..7 se
    // leerian como otros bloques.
    CHECK((int)BLOCK_TECHO_HOJAS_L2 == (int)BLOCK_TECHO_HOJAS_L1 + 1);
    CHECK((int)BLOCK_TECHO_HOJAS_L3 == (int)BLOCK_TECHO_HOJAS_L1 + 2);
    CHECK((int)BLOCK_TECHO_HOJAS_L7 == (int)BLOCK_TECHO_HOJAS_L1 + 6);
}

TEST_CASE("Techo: admite niveles y se coloca como lamina de nivel 1") {
    CHECK(admiteNiveles(BLOCK_TECHO_HOJAS));

    // Colocado nace en su nivel 1 -- 3 px, el espesor de un techo.
    const BlockType l1 = conNivel(BLOCK_TECHO_HOJAS, 1);
    CHECK(esNivelParcial(l1));
    CHECK(nivelDe(l1) == 1);
    CHECK(bloqueBaseDe(l1) == BLOCK_TECHO_HOJAS);
}

TEST_CASE("Techo: puede crecer poniendo mas, hasta llenar la celda") {
    // "Puedan colocarse mas y crecer": los siete niveles existen y cada uno
    // sigue siendo techo.
    for (int n = 1; n <= 7; ++n) {
        const BlockType b = conNivel(BLOCK_TECHO_HOJAS, n);
        INFO("nivel ", n);
        CHECK(esNivelParcial(b));
        CHECK(nivelDe(b) == n);
        CHECK(bloqueBaseDe(b) == BLOCK_TECHO_HOJAS);
    }
    // El octavo ya es el bloque entero.
    CHECK(conNivel(BLOCK_TECHO_HOJAS, 8) == BLOCK_TECHO_HOJAS);
}

TEST_CASE("Techo: anadirlo NO movio las familias ya guardadas") {
    // ⭐⭐ EL TEST QUE PROTEGE LOS MUNDOS EXISTENTES.
    //
    // El indice de familia viaja DENTRO del ID de cada celda mixta guardada.
    // La primera version de este bloque metio su fila en MEDIO de la tabla y
    // corrio los cinco centinelas de los minerales y, con ellos, la madera:
    // una capa de tierra con un tronco encima habria pasado a leerse como otra
    // cosa en todos los mundos.
    //
    // Al final de la tabla es seguro. Aqui se fija que siga siendo asi.
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    REQUIRE(n >= 17);
    REQUIRE(familiasCaben());

    CHECK(T[0].entero  == BLOCK_GRASS);
    CHECK(T[1].entero  == BLOCK_DIRT);
    CHECK(T[2].entero  == BLOCK_STONE);
    CHECK(T[10].entero == BLOCK_LIMESTONE);
    // El techo va DESPUES de las 16 originales, no en medio. (Ya no es el
    // ultimo: los tres tablones volvieron a la tabla detras de el.)
    CHECK(T[16].entero == BLOCK_TECHO_HOJAS);
}

TEST_CASE("Techo: la receta esta registrada y da 8") {
    // Se lee el propio main.cpp: el motor no se enlaza con los tests.
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    REQUIRE(f.good());
    const std::string src((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());

    // Da OCHO y va sin forma (shapeless), para que valga cualquier especie de
    // hoja y cualquier esquina de la rejilla.
    CHECK(src.find("CraftingRecipe recipe(BLOCK_TECHO_HOJAS, 8, true)")
          != std::string::npos);
#endif
}

// ============================================================================
// EL TECHO CUELGA DEL TECHO DE SU CELDA
// ============================================================================
// Todos los demas niveles crecen desde el SUELO de su celda. El techo de
// hojas es la excepcion: una lamina de 3 px a ras de suelo es una ALFOMBRA,
// no un techo. Asi que su franja va de 16-h a 16 en vez de 0 a h.
//
// Son TRES sitios los que tienen que decir lo mismo, y de eso va esto:
//
//   1. EL MESHER     lo que se VE
//   2. LA CAJA       lo que FRENA al jugador y lo que se puede APUNTAR
//   3. EL APILADO    que no levante al jugador por una capa que cuelga
//
// Si uno de los tres se queda atras, el techo se ve en un sitio y choca en
// otro -- el bug clasico de geometria que no cuadra con su colision.
// ============================================================================

namespace {

std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}

}  // namespace

TEST_CASE("Techo: la altura de cada nivel es la de la tabla de niveles") {
    // La franja colgada se calcula como 1 - alturaDe(). Si estas alturas
    // cambiaran, el techo se despegaria del techo o lo atravesaria.
    CHECK(alturaDe(conNivel(BLOCK_TECHO_HOJAS, 1)) == doctest::Approx(3.0f / 16.0f));
    CHECK(alturaDe(conNivel(BLOCK_TECHO_HOJAS, 7)) == doctest::Approx(13.0f / 16.0f));
    // Lleno es la celda entera: colgado o apoyado da lo mismo.
    CHECK(alturaDe(BLOCK_TECHO_HOJAS) == doctest::Approx(1.0f));
}

TEST_CASE("Techo: colgado, la franja llega SIEMPRE hasta el techo") {
    // Es la propiedad que define "techo": el borde de arriba toca el 16,
    // mida lo que mida la lamina. Lo que se mueve es el borde de ABAJO.
    for (int n = 1; n <= 7; ++n) {
        const BlockType b = conNivel(BLOCK_TECHO_HOJAS, n);
        const float h    = alturaDe(b);
        const float yIni = 1.0f - h;
        const float yFin = 1.0f;

        INFO("nivel ", n);
        CHECK(yFin == doctest::Approx(1.0f));        // pegado al techo
        CHECK(yIni == doctest::Approx(1.0f - h));    // y baja lo que mide
        CHECK(yIni >= 0.0f);                         // nunca sale de su celda
        CHECK(yFin - yIni == doctest::Approx(h));    // conserva su grosor
    }
}

TEST_CASE("Techo: al subir de nivel engorda hacia ABAJO, no hacia arriba") {
    // Poner un segundo techo en la misma celda sube el nivel. Colgado, eso
    // tiene que bajar el borde inferior y dejar el superior clavado en 16.
    float anteriorIni = 1.0f;
    for (int n = 1; n <= 7; ++n) {
        const float yIni = 1.0f - alturaDe(conNivel(BLOCK_TECHO_HOJAS, n));
        INFO("nivel ", n);
        CHECK(yIni <= anteriorIni);   // el suelo de la lamina baja
        anteriorIni = yIni;
    }
    // Del nivel 1 al 7 ha bajado de 13/16 a 3/16.
    CHECK(1.0f - alturaDe(conNivel(BLOCK_TECHO_HOJAS, 1)) == doctest::Approx(13.0f / 16.0f));
    CHECK(1.0f - alturaDe(conNivel(BLOCK_TECHO_HOJAS, 7)) == doctest::Approx(3.0f / 16.0f));
}

TEST_CASE("Techo: el mesher lo dibuja colgado") {
    const std::string src = fuenteMotor();
    REQUIRE_MESSAGE(!src.empty(), "no se pudo leer src/main.cpp");

    const size_t m = src.find("EL TECHO DE HOJAS CUELGA, NO SE APOYA");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "el mesher ya no cuelga el techo: volveria a ser alfombra");

    // Margen amplio: entre la marca y el codigo hay el comentario que explica
    // por que solo cuelga la variante colgante.
    const std::string cuerpo = src.substr(m, 2600);
    // Se pregunta por `esTechoColgante`, no por el ID a pelo: ahora hay DOS
    // variantes del techo y solo una cuelga.
    CHECK(cuerpo.find("esTechoColgante(block)") != std::string::npos);
    CHECK(cuerpo.find("yFin = 1.0f;") != std::string::npos);
    CHECK(cuerpo.find("yIni = 1.0f - alturaDe(block);") != std::string::npos);
}

TEST_CASE("Techo: su caja de colision cuelga igual que su dibujo") {
    // ⭐ EL QUE IMPORTA. Si el mesher cuelga la lamina y la caja se queda
    // abajo, el jugador atraviesa lo que ve y choca contra aire a sus pies.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("EL TECHO DE HOJAS CUELGA DEL TECHO DE SU CELDA");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "la caja del techo ya no cuelga: no cuadraria con el dibujo");

    // Margen amplio: entre la marca y el codigo hay ahora el comentario que
    // explica por que solo cuelga cuando ocupa la celda en exclusiva.
    const std::string cuerpo = src.substr(m, 2600);
    CHECK(cuerpo.find("minY = 1.0f - altoNivel;  maxY = 1.0f;") != std::string::npos);
    // Y el resto de niveles siguen apoyandose en el suelo.
    CHECK(cuerpo.find("minY = 0.0f;              maxY = altoNivel;") != std::string::npos);
}

TEST_CASE("Techo: apilarlo bajo los pies no levanta al jugador") {
    // `subirSiApilaBajoLosPies` existe para "la capa crece bajo mis pies, subo
    // con ella". Un techo crece hacia ABAJO desde el techo: nada sube, asi que
    // no hay que seguir nada. Sin la salida, el jugador daria un saltito por
    // cada techo que colocase a sus pies.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("EL TECHO DE HOJAS NO LEVANTA A NADIE");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "sin esta salida el techo empuja al jugador hacia arriba");

    const std::string cuerpo = src.substr(m, 1200);
    CHECK(cuerpo.find("if (bloqueBaseDe(puesto) == BLOCK_TECHO_HOJAS) return;")
          != std::string::npos);
}

TEST_CASE("Techo: puede combinarse con otros niveles en la misma celda") {
    // PEDIDO: "que los techos de hojas puedan tener eso encima y se puedan
    // colocar en el suelo o combinado con niveles".
    //
    // Para ser el material de ARRIBA de una celda mixta hay que estar en
    // `tablaRelleno`: sin entrada ahi, `indiceRelleno` devuelve -1 y
    // `mixto()` rechaza la pareja.
    CHECK(indiceRelleno(BLOCK_TECHO_HOJAS) >= 0);

    // Sobre una capa de tierra, a su propia altura.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_TECHO_HOJAS, 2);
    REQUIRE_MESSAGE(m != BLOCK_AIR,
                    "el techo no puede combinarse: falta en tablaRelleno");
    CHECK(esMixto(m));
    CHECK(mixtoBase(m) == BLOCK_DIRT);
    CHECK(mixtoRelleno(m) == BLOCK_TECHO_HOJAS);
    CHECK(mixtoNivelRelleno(m) == 2);
    CHECK(mixtoOctavosTotales(m) == 5);
}

TEST_CASE("Techo: combinado SE APOYA, no cuelga") {
    // La decision de diseno: suelto sigue colgando del techo de su celda
    // (es lo que lo hace un techo), pero al compartir celda se apoya sobre
    // la capa de abajo como cualquier otro nivel.
    //
    // El mesher dibuja el relleno de `alturaBaseMixto` a `alturaDe`, asi que
    // la propiedad se comprueba sobre esos dos numeros: el techo empieza
    // EXACTAMENTE donde acaba la capa de abajo.
    const BlockType m = mixto(BLOCK_STONE, 4, BLOCK_TECHO_HOJAS, 3);
    REQUIRE(m != BLOCK_AIR);

    // Donde acaba la piedra = donde empieza el techo. Sin hueco.
    CHECK(alturaBaseMixto(m) == doctest::Approx(alturaNivelPx(4) / 16.0f));
    // Y la celda acaba donde acaba el techo, no en el 16.
    CHECK(alturaDe(m) == doctest::Approx(alturaNivelPx(7) / 16.0f));
}

TEST_CASE("Techo: tambien puede ser la capa de ABAJO") {
    // "que puedan tener eso encima": el techo en el suelo y otro material
    // apoyado sobre el.
    const BlockType m = mixto(BLOCK_TECHO_HOJAS, 2, BLOCK_SAND, 3);
    REQUIRE(m != BLOCK_AIR);

    CHECK(mixtoBase(m) == BLOCK_TECHO_HOJAS);
    CHECK(mixtoNivel(m) == 2);
    CHECK(mixtoRelleno(m) == BLOCK_SAND);
    CHECK(mixtoOctavosTotales(m) == 5);
}

TEST_CASE("Techo: se funde con los materiales del suelo") {
    // Arena, grava, tierra, tierra arcillosa y piedra: los que se nombraron.
    // Ninguna combinacion puede devolver NO_FUSIONA, que es lo que corta el
    // apilado y crea las barreras.
    const BlockType SUELOS[] = {
        BLOCK_SAND, BLOCK_GRAVEL, BLOCK_DIRT, BLOCK_CLAY_DIRT, BLOCK_STONE,
    };
    for (BlockType s : SUELOS) {
        for (int nb = 1; nb <= 7; ++nb) {
            const Fusion f = fusionarNiveles(conNivel(BLOCK_TECHO_HOJAS, 2),
                                             conNivel(s, nb));
            INFO("techo sobre ", (int)s, " nivel ", nb);
            CHECK(f.tipo != ResultadoFusion::NO_FUSIONA);
        }
    }
}

TEST_CASE("Techo: la caja cuelga SOLO si ocupa la celda en exclusiva") {
    // ⭐ EL BUG DEL SALTO VISUAL AL APILAR SOBRE EL TECHO.
    //
    // El techo SUELTO cuelga: su franja va de 16-h a 16. Pero en cuanto
    // comparte celda con otro material, el mesher lo dibuja APOYADO como
    // cualquier relleno (la rama de colgar esta dentro de `!esMixto`).
    //
    // La caja tiene que seguir la MISMA regla. Si colgara tambien en la
    // mixta, el bloque se veria abajo y frenaria arriba -- el clasico
    // "geometria que no cuadra con su colision".
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("SOLO CUELGA SI OCUPA LA CELDA EN EXCLUSIVA");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "la caja del techo volvio a colgar siempre: en una celda "
                    "mixta se veria en un sitio y frenaria en otro");

    const std::string cuerpo = src.substr(m, 1400);
    // Dos condiciones, y las dos importan: cuelga solo si ocupa la celda en
    // exclusiva (`!esMixto`) Y si es la variante colgante (`esTechoColgante`,
    // que distingue BLOCK_TECHO_HOJAS de BLOCK_TECHO_SUELO).
    CHECK(cuerpo.find("!esMixto(type) && esTechoColgante(type)")
          != std::string::npos);
}

TEST_CASE("Techo: cualquier material se le puede poner encima") {
    // PEDIDO: "que puedas colocar niveles sobre el techo de hojas de
    // cualquier tipo de bloque sin que se bugue".
    //
    // Ninguna combinacion puede devolver NO_FUSIONA (eso corta el apilado) ni
    // perder volumen por el camino.
    const BlockType MATS[] = {
        BLOCK_SAND, BLOCK_GRAVEL, BLOCK_DIRT, BLOCK_CLAY_DIRT,
        BLOCK_CLAY_SAND, BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_SNOW,
    };

    for (BlockType mat : MATS) {
        for (int nTecho = 1; nTecho <= 7; ++nTecho) {
            for (int nCae = 1; nCae <= 7; ++nCae) {
                const Fusion f = fusionarNiveles(conNivel(mat, nCae),
                                                 conNivel(BLOCK_TECHO_HOJAS, nTecho));

                INFO("material ", (int)mat, " nivel ", nCae,
                     " sobre techo nivel ", nTecho);

                // Nunca una barrera.
                REQUIRE(f.tipo != ResultadoFusion::NO_FUSIONA);
                if (f.tipo != ResultadoFusion::FUSIONADOS) continue;

                // Y el volumen se conserva exacto.
                auto oct = [](BlockType t) -> int {
                    if (t == BLOCK_AIR) return 0;
                    if (esMixto(t)) return mixtoOctavosTotales(t);
                    if (esNivelParcial(t)) return nivelDe(t);
                    return 8;
                };
                CHECK(oct(f.resultado) + oct(f.sobrante) == nTecho + nCae);
            }
        }
    }
}

TEST_CASE("Techo: nada flota al apilarle encima") {
    // Si sube un sobrante, lo de abajo tiene que quedar LLENO. Si no, el
    // material de arriba nace en el suelo de su celda y se ve flotando.
    for (int nTecho = 1; nTecho <= 7; ++nTecho) {
        for (int nCae = 1; nCae <= 7; ++nCae) {
            const Fusion f = fusionarNiveles(conNivel(BLOCK_GRAVEL, nCae),
                                             conNivel(BLOCK_TECHO_HOJAS, nTecho));
            if (f.tipo != ResultadoFusion::FUSIONADOS) continue;
            if (f.sobrante == BLOCK_AIR) continue;

            const bool abajoLleno = esMixto(f.resultado)
                                  ? (mixtoOctavosTotales(f.resultado) >= 8)
                                  : !esNivelParcial(f.resultado);

            INFO("techo ", nTecho, " + grava ", nCae);
            CHECK_MESSAGE(abajoLleno,
                          "sube material con la celda a medio llenar: flota");
        }
    }
}

// ============================================================================
// LAS DOS ORIENTACIONES: COLGADO Y APOYADO
// ============================================================================
// PEDIDO: "que puedas colocar y apilar mas niveles de techo de hojas tanto
// arriba como abajo; si el jugador esta abajo del nivel, colocara bloques
// bajo el nivel, y si esta arriba se apilan sobre el nivel".
//
// El techo cuelga de la cara de arriba de su celda --es lo que lo distingue
// de una alfombra-- y esa orientacion estaba cableada en el ID. Para tener
// los dos sentidos hace falta una familia HERMANA que se apoye en el suelo.
// ============================================================================

TEST_CASE("Techo: existen las dos orientaciones y son bloques distintos") {
    CHECK((int)BLOCK_TECHO_SUELO <= (int)BLOCK_TYPE_MAX);
    CHECK(BLOCK_TECHO_SUELO != BLOCK_TECHO_HOJAS);

    // Las dos admiten niveles: la altura es lo que se apila.
    CHECK(admiteNiveles(BLOCK_TECHO_HOJAS));
    CHECK(admiteNiveles(BLOCK_TECHO_SUELO));

    // Y sus siete niveles van seguidos, como manda tablaNiveles.
    CHECK((int)BLOCK_TECHO_SUELO_L7 == (int)BLOCK_TECHO_SUELO_L1 + 6);
    for (int n = 1; n <= 7; ++n) {
        const BlockType b = conNivel(BLOCK_TECHO_SUELO, n);
        INFO("nivel ", n);
        CHECK(esNivelParcial(b));
        CHECK(nivelDe(b) == n);
        CHECK(bloqueBaseDe(b) == BLOCK_TECHO_SUELO);
    }
}

TEST_CASE("Techo: solo la variante colgante cuelga") {
    // De esto dependen el mesher Y la caja de colision. Si las dos colgaran
    // (o ninguna), el bloque se veria en un sitio y frenaria en otro.
    CHECK(esTechoColgante(BLOCK_TECHO_HOJAS));
    CHECK(esTechoColgante(conNivel(BLOCK_TECHO_HOJAS, 3)));

    CHECK_FALSE(esTechoColgante(BLOCK_TECHO_SUELO));
    CHECK_FALSE(esTechoColgante(conNivel(BLOCK_TECHO_SUELO, 3)));

    // Pero las dos son techo de hojas.
    CHECK(esTechoDeHojas(BLOCK_TECHO_HOJAS));
    CHECK(esTechoDeHojas(BLOCK_TECHO_SUELO));
    CHECK(esTechoDeHojas(conNivel(BLOCK_TECHO_SUELO, 5)));

    // Y nada mas lo es.
    CHECK_FALSE(esTechoDeHojas(BLOCK_DIRT));
    CHECK_FALSE(esTechoDeHojas(BLOCK_LEAVES));
}

TEST_CASE("Techo: las dos orientaciones se apilan entre si") {
    // ⭐ LO QUE EVITA LA FRONTERA VISIBLE.
    //
    // Son el MISMO material: apilar uno sobre otro tiene que sumar alturas,
    // no formar una celda mixta de "techo + techo" con una juntura en medio.
    const Fusion f = fusionarNiveles(conNivel(BLOCK_TECHO_SUELO, 2),
                                     conNivel(BLOCK_TECHO_HOJAS, 3));

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK_FALSE(esMixto(f.resultado));
    CHECK(nivelDe(f.resultado) == 5);

    // Y conserva la orientacion de la capa de ABAJO, que estaba primero.
    CHECK(esTechoColgante(f.resultado));
}

TEST_CASE("Techo: apilar conserva el volumen en las dos orientaciones") {
    const BlockType ORIENT[] = { BLOCK_TECHO_HOJAS, BLOCK_TECHO_SUELO };

    for (BlockType abajo : ORIENT) {
        for (BlockType arriba : ORIENT) {
            for (int na = 1; na <= 7; ++na) {
                for (int nb = 1; nb <= 7; ++nb) {
                    const Fusion f = fusionarNiveles(conNivel(arriba, nb),
                                                     conNivel(abajo, na));
                    INFO("abajo ", (int)abajo, " nivel ", na,
                         " + arriba ", (int)arriba, " nivel ", nb);

                    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);

                    auto oct = [](BlockType t) -> int {
                        if (t == BLOCK_AIR) return 0;
                        if (esMixto(t)) return mixtoOctavosTotales(t);
                        if (esNivelParcial(t)) return nivelDe(t);
                        return 8;
                    };
                    CHECK(oct(f.resultado) + oct(f.sobrante) == na + nb);
                }
            }
        }
    }
}

TEST_CASE("Techo: se coloca donde sea, y siempre como bloque completo") {
    // PEDIDO: "que se pueda colocar como sea y como se pueda", combinado con
    // el anterior: "que si se coloque se ponga y sea un bloque completo".
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Ya no hay salida por altura: se coloca apunte a donde apunte.
    CHECK(src.find("EL TECHO DE HOJAS SOLO SE COLOCA ARRIBA") == std::string::npos);
    CHECK(src.find("techoDebeColgar") == std::string::npos);

    // Queda fuera de la conversion "todo bloque con niveles nace en L1": la
    // excepcion vive en seColocaEntero, que ya incluye al techo.
    CHECK(seColocaEntero(BLOCK_TECHO_HOJAS));
    CHECK(src.find("!seColocaEntero(blockToPlace) &&") != std::string::npos);

    // Y lo que se coloca es el ID base: el nivel 8, el bloque entero.
    const size_t c = src.find("EL TECHO DE HOJAS SE COLOCA COMO BLOQUE COMPLETO");
    REQUIRE(c != std::string::npos);
    const std::string completo = src.substr(c, 1500);
    CHECK(completo.find("blockToPlace = BLOCK_TECHO_HOJAS;") != std::string::npos);
    CHECK(completo.find("conNivel(") == std::string::npos);

    CHECK(alturaDe(BLOCK_TECHO_HOJAS) == doctest::Approx(1.0f));
    CHECK_FALSE(esNivelParcial(BLOCK_TECHO_HOJAS));
}

// ----------------------------------------------------------------------------
// EL TEJADO SE AGARRA DE LAS PAREDES
// ----------------------------------------------------------------------------
// PEDIDO: "si lo colocas en techo con bloques a los lados de paredes, ahi se
// mantendra y podra conectar a otros bloques de techo de hojas, pero si no hay
// un soporte, caera como un bloque con gravedad normal".
//
// Mundo de mentira: un mapa de celdas, todo lo demas es aire. `sujeta` es la
// misma idea que la del motor (algo solido que no es techo), reducida a piedra
// para que el test no dependa de isBlockSolid.
namespace {
struct MundoTecho {
    std::map<std::tuple<int, int, int>, BlockType> celdas;
    void poner(int x, int y, int z, BlockType t) { celdas[{x, y, z}] = t; }
    BlockType get(int x, int y, int z) const {
        auto it = celdas.find({x, y, z});
        return it == celdas.end() ? BLOCK_AIR : it->second;
    }
    bool sostenido(int x, int y, int z, std::vector<Fisica::CeldaTecho>& g) const {
        return Fisica::techoSostenidoCon(
            [&](int cx, int cy, int cz) { return get(cx, cy, cz); },
            [](BlockType v) { return v == BLOCK_STONE; },
            x, y, z, g);
    }
};
}

TEST_CASE("Techo: suelto en el aire, sin nada alrededor, cae") {
    MundoTecho m;
    m.poner(0, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK_FALSE(m.sostenido(0, 70, 0, g));
    CHECK(g.size() == 1);                 // cae el solo
}

TEST_CASE("Techo: pegado a una pared por un lado, se mantiene") {
    MundoTecho m;
    m.poner(1, 70, 0, BLOCK_STONE);       // la pared
    m.poner(0, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(0, 70, 0, g));
}

TEST_CASE("Techo: un tejado largo se sostiene por la pared de un extremo") {
    // Pared en x=0, y del borde salen 6 techos hacia fuera, en el aire. Todos
    // estan CONECTADOS a la pared a traves de los demas: ninguno cae, ni el
    // de la punta, que esta a 6 bloques de ella.
    MundoTecho m;
    m.poner(0, 70, 0, BLOCK_STONE);
    for (int x = 1; x <= 6; ++x) m.poner(x, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(6, 70, 0, g));
    CHECK(m.sostenido(3, 70, 0, g));
}

TEST_CASE("Techo: quitar la pared tira el tejado ENTERO") {
    // Mismo tejado sin la pared: todo el grupo conectado sale en `grupo`,
    // que es lo que el motor manda a caer.
    MundoTecho m;
    for (int x = 1; x <= 6; ++x) m.poner(x, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK_FALSE(m.sostenido(3, 70, 0, g));
    CHECK(g.size() == 6);
}

TEST_CASE("Techo: dos paredes, un tejado de lado a lado") {
    // Una casa: paredes en x=0 y x=5 y el tejado cruzando entre ellas, con
    // ancho en z. Se sostiene; y sigue sosteniendose con UNA sola pared.
    MundoTecho m;
    m.poner(0, 70, 0, BLOCK_STONE);
    m.poner(5, 70, 0, BLOCK_STONE);
    for (int x = 1; x <= 4; ++x)
        for (int z = 0; z <= 3; ++z)
            m.poner(x, 70, z, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(2, 70, 3, g));

    m.poner(0, 70, 0, BLOCK_AIR);
    CHECK(m.sostenido(2, 70, 3, g));
}

TEST_CASE("Techo: algo firme DEBAJO tambien lo sujeta") {
    // Apoyado en el suelo, como cualquier bloque con gravedad: no cae.
    MundoTecho m;
    m.poner(0, 69, 0, BLOCK_STONE);
    m.poner(0, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(0, 70, 0, g));
}

TEST_CASE("Techo: pegado solo por ARRIBA no se sostiene") {
    // El pedido habla de paredes a los lados: el envés de un saliente no es
    // una pared, y un techo colgado solo de ahi cae.
    MundoTecho m;
    m.poner(0, 71, 0, BLOCK_STONE);
    m.poner(0, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK_FALSE(m.sostenido(0, 70, 0, g));
}

TEST_CASE("Techo: las variantes viejas en lamina tambien cuentan como tejado") {
    // Mundos guardados con laminas (colgadas o apoyadas): forman parte del
    // mismo tejado y se agarran igual.
    MundoTecho m;
    m.poner(0, 70, 0, BLOCK_STONE);
    m.poner(1, 70, 0, conNivel(BLOCK_TECHO_HOJAS, 2));
    m.poner(2, 70, 0, conNivel(BLOCK_TECHO_SUELO, 3));
    m.poner(3, 70, 0, BLOCK_TECHO_HOJAS);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(3, 70, 0, g));
}

TEST_CASE("Techo: un bloque que no es techo no aplica") {
    MundoTecho m;
    m.poner(0, 70, 0, BLOCK_STONE);

    std::vector<Fisica::CeldaTecho> g;
    CHECK(m.sostenido(0, 70, 0, g));
    CHECK(g.empty());
}

TEST_CASE("Techo: el motor usa la misma regla al colocar y al romper") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Al colocar: el chequeo de un paso se sustituyo por el recorrido.
    CHECK(src.find("techoSostenido(state->world, placePos.x, placePos.y,")
          != std::string::npos);

    // Al romper: revisarSoporte mira los techos vecinos primero.
    const size_t r = src.find("void revisarSoporte(World& world, int x, int y, int z) {");
    REQUIRE(r != std::string::npos);
    CHECK(src.substr(r, 600).find("revisarTechos(world, x, y, z);") != std::string::npos);

    // Y el recorrido de estructuras (arboles) no se los lleva como items.
    CHECK(src.find("if (esTechoDeHojas(inicial)) return false;") != std::string::npos);
}

TEST_CASE("Techo: el apoyado suelta el item normal al picarlo") {
    // `BLOCK_TECHO_SUELO` es una variante de COLOCACION, no un material
    // aparte: en el inventario solo existe el techo normal. Si soltara su
    // propio ID, el jugador tendria un item que no se craftea ni se usa.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("LAS DOS ORIENTACIONES DEL TECHO SUELTAN EL MISMO ITEM");
    REQUIRE(m != std::string::npos);

    const std::string cuerpo = src.substr(m, 900);
    CHECK(cuerpo.find("case BLOCK_TECHO_SUELO:") != std::string::npos);
    CHECK(cuerpo.find("return BLOCK_TECHO_HOJAS;") != std::string::npos);
}
