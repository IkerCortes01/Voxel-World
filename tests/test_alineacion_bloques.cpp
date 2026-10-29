#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// ALINEACION DE LOS BLOQUES CUBICOS COMPLETOS
// ============================================================================
// BUG REPORTADO: los bloques quedan separados o desalineados al ponerlos uno
// encima de otro.
//
// LA CAUSA NO ERA DE COORDENADAS, y conviene que quede escrito porque es el
// sitio donde todo el mundo mira primero:
//
//   - `emitQuad` usa limites de celda ENTEROS: la cara de arriba de la celda
//     `layer` se emite en `layer + 1` y la de abajo de la siguiente en
//     `layer`. Mismo numero, mismo float, cero hueco.
//   - La base del chunk es `position.x * CHUNK_SIZE`: multiplicacion entera,
//     sin redondeo ni acumulacion de error.
//
// El problema era que al COLOCAR, todo bloque macizo se convertia en su NIVEL
// 1 -- 3 px de 16. El jugador pedia un cubo y recibia una lamina; dos laminas
// apiladas dejan 13 px de aire entre ellas.
//
// Estos tests fijan la medida de las piezas, que es lo que se puede comprobar
// sin arrancar OpenGL, y que el motor no vuelva a convertir a nivel 1.
// ============================================================================

namespace {
std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f) return std::string();
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return std::string();
#endif
}
} // namespace

TEST_CASE("Alineacion: un bloque cubico mide EXACTAMENTE una celda") {
    // Si midiera menos, quedaria un hueco; si midiera mas, se solaparia con el
    // vecino. 1.0 exacto es la unica medida que pega sin penetrar.
    const BlockType CUBICOS[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_COBBLESTONE,
        BLOCK_LIMESTONE, BLOCK_CLAY, BLOCK_CLAY_DIRT, BLOCK_CLAY_SAND,
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,
        BLOCK_GRASS, BLOCK_SNOW,
    };
    for (BlockType b : CUBICOS) {
        INFO("bloque ", (int)b);
        CHECK(alturaDe(b) == doctest::Approx(1.0f));
        CHECK_FALSE(esNivelParcial(b));
    }
}

TEST_CASE("Alineacion: una celda MIXTA tambien esta llena") {
    // Dos materiales en la misma celda, pero el conjunto ocupa el voxel
    // entero: se anda por encima como por un bloque normal.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_STONE);
    REQUIRE(m != BLOCK_AIR);
    CHECK(alturaDe(m) == doctest::Approx(1.0f));
}

TEST_CASE("Alineacion: los niveles parciales SI son mas bajos, a proposito") {
    // La contrapartida: una capa parcial NO debe medir 1.0, o dejaria de ser
    // una capa. Lo que se arreglo es que un bloque ENTERO no se convirtiera en
    // una de estas al colocarlo, no las capas en si.
    for (int n = 1; n <= 7; ++n) {
        const BlockType b = conNivel(BLOCK_STONE, n);
        INFO("nivel ", n);
        CHECK(esNivelParcial(b));
        CHECK(alturaDe(b) < 1.0f);
        CHECK(alturaDe(b) > 0.0f);
    }
    // Y el octavo ya es el cubo entero.
    CHECK(alturaDe(conNivel(BLOCK_STONE, 8)) == doctest::Approx(1.0f));
}

TEST_CASE("Alineacion: todo lo que admite niveles se coloca COMO NIVEL") {
    // ⭐⭐ ESTE TEST CAMBIO DE SENTIDO A PROPOSITO.
    //
    // ANTES vigilaba que la conversion a nivel 1 estuviera CONDICIONADA a lo
    // que hubiera debajo: sobre algo lleno se ponia el cubo entero, y solo
    // sobre suelo parcial o aire la lamina. Nacio del bug de la lamina
    // "colgada con 13 px de aire debajo".
    //
    // AHORA la regla es otra, y por peticion explicita: un bloque que admite
    // niveles se coloca SIEMPRE como capa, en cualquier sitio. Para tener un
    // cubo macizo hay que apilar sus ocho octavos.
    //
    // El miedo viejo ya no aplica: una capa crece desde el SUELO de su celda
    // (ver alturaDe), asi que sobre la tapa de un cubo se apoya sin dejar un
    // solo pixel de hueco. El bug original era poner la lamina en la celda
    // EQUIVOCADA, no la lamina en si.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // La guarda vieja NO debe volver: era justo la que ponia cubos enteros.
    CHECK_MESSAGE(src.find("const bool apoyoLleno") == std::string::npos,
                  "volvio la regla vieja: colocaria cubos enteros sobre "
                  "bloques llenos");

    // Y la conversion a nivel 1 ocurre sin mirar lo que hay debajo.
    const size_t c = src.find("blockToPlace = conNivel(bloqueBaseDe(blockToPlace), 1);");
    REQUIRE_MESSAGE(c != std::string::npos,
                    "ya no se convierte a nivel 1 en el camino principal");
}

TEST_CASE("Alineacion: solo piedra labrada, techo de hojas y tablones van enteros") {
    // La piedra labrada es el material de construccion del juego: obligar a
    // ocho clics por bloque haria inviable levantar un muro. El techo de
    // hojas y los tablones se pidieron expresamente como "bloque completo".
    //
    // Si alguien anade una tercera excepcion, que sea una decision y no un
    // descuido: este test la hace visible. Ahora la regla vive en una sola
    // funcion (seColocaEntero) y aqui se recorren TODOS los bloques.
    for (int i = 1; i <= BLOCK_TYPE_MAX; ++i) {
        const BlockType t = (BlockType)i;
        if (!seColocaEntero(t)) continue;
        const BlockType b = bloqueBaseDe(t);
        INFO("bloque que se coloca entero: ", i);
        CHECK((b == BLOCK_COBBLESTONE || b == BLOCK_TECHO_HOJAS ||
               b == BLOCK_TECHO_SUELO || esTablon(b)));
    }
    CHECK(seColocaEntero(BLOCK_COBBLESTONE));
    CHECK(seColocaEntero(BLOCK_TECHO_HOJAS));

    // Y la guarda de placeBlock pregunta a esa funcion, no a una lista suya.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("!seColocaEntero(blockToPlace) &&") != std::string::npos);
}

TEST_CASE("Alineacion: colocar de lado no secuestra la capa de abajo") {
    // ⭐ EL BUG DE LOS LADOS.
    //
    // La rama anti-flotacion no miraba POR DONDE habia entrado el rayo, asi
    // que al apuntar al COSTADO de un cubo --con una capa bajo el destino--
    // en vez de poner el nivel donde apuntabas, subia esa capa de abajo.
    // Desde fuera se ve como "no me deja poner niveles al lado de un bloque".
    //
    // La normal del raycast dice por que cara se entro: solo con componente
    // Y positiva (una tapa) tiene sentido el caso de "flotar encima".
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("SOLO APUNTANDO HACIA ARRIBA");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "volvio el bug de los lados: la rama anti-flotacion "
                    "se dispara apuntando de costado");

    const std::string cuerpo = src.substr(m, 2200);
    CHECK(cuerpo.find("const bool apuntaHaciaArriba = (result.normal.y > 0);")
          != std::string::npos);
    CHECK(cuerpo.find("if (apuntaHaciaArriba && destinoLibre") != std::string::npos);
}

TEST_CASE("Alineacion: material distinto se funde, no se estira al techo") {
    // `mixto()` rellena de la capa al TECHO: poner una lamina de arena sobre
    // una capa de tierra ocupaba los siete octavos libres. El jugador ponia
    // 3 px y le salia un bloque casi entero.
    //
    // `fusionarNiveles` conserva la altura de cada material y manda el
    // sobrante a la celda de arriba. Las dos ramas de material distinto
    // --de lado y encima-- tienen que usar la misma aritmetica.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("POR `fusionarNiveles`, NO POR `mixto`");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "la rama de encima volvio a usar mixto(): un nivel de "
                    "otro material se estiraria hasta el techo");

    const std::string cuerpo = src.substr(m, 2400);
    CHECK(cuerpo.find("fusionarNiveles(aFundir, tocado)") != std::string::npos);
    CHECK(cuerpo.find("ResultadoFusion::FUSIONADOS") != std::string::npos);
    // Y el sobrante sube, en vez de perderse o estirarse.
    CHECK(cuerpo.find("fusAqui.sobrante") != std::string::npos);
}

TEST_CASE("Alineacion: el mesher usa limites de celda enteros") {
    // La garantia de que no hay hueco ni solape: la cara de arriba de una
    // celda y la de abajo de la siguiente salen del MISMO numero.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Cara de arriba en `layer + 1`, cara de abajo en `layer`. Sin epsilon.
    CHECK(src.find("const float yy = (float)(layer + 1);") != std::string::npos);
    CHECK(src.find("const float yy = (float)layer;") != std::string::npos);

    // Y la base del chunk es multiplicacion entera, sin redondeo.
    CHECK(src.find("(float)(chunk->position.x * CHUNK_SIZE)") != std::string::npos);
}

TEST_CASE("Alineacion: una capa nunca nace con hueco debajo") {
    // ⭐ EL BUG DEL HUECO AL COLOCAR DE LADO.
    //
    // `placePos` sale del raycast: es la celda VACIA por la que entro el
    // rayo. Apuntando al costado de un bloque que esta a media altura, esa
    // celda puede tener OTRA vacia justo debajo. La capa nacia en el suelo de
    // SU celda --16 px por encima del suelo real-- y entre las dos quedaba
    // una franja de aire a la vista.
    //
    // La regla: una capa parcial no puede empezar en el aire. Antes de
    // colocar se baja el destino mientras haya hueco debajo.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("UNA CAPA NUNCA NACE CON HUECO DEBAJO");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "volvio el hueco: una capa colocada de lado puede "
                    "quedar flotando sobre aire");

    const std::string cuerpo = src.substr(m, 3200);

    // Solo baja CAPAS: un bloque entero al costado es construccion legitima.
    CHECK(cuerpo.find("if (esNivelParcial(blockToPlace) && placePos.y > 0)")
          != std::string::npos);

    // Baja mientras lo de debajo sea aire, con tope para no recorrer medio
    // mundo en un clic.
    CHECK(cuerpo.find("intentos < 8") != std::string::npos);
    CHECK(cuerpo.find("placePos.y = destinoY;") != std::string::npos);
}

TEST_CASE("Alineacion: al bajar sobre una capa, se funden") {
    // Si la capa baja y se posa sobre OTRA capa, tiene que sumarse a ella.
    // Dejarla encima en su propia celda serian dos laminas de 3 px apoyadas
    // una en otra -- medio hueco otra vez.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("Y SI AL BAJAR SE POSA SOBRE UNA CAPA, SE FUNDEN");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "la capa que baja ya no se funde con la de abajo");

    const std::string cuerpo = src.substr(m, 2200);
    CHECK(cuerpo.find("fusionarNiveles(blockToPlace, soporte)") != std::string::npos);
    CHECK(cuerpo.find("ResultadoFusion::FUSIONADOS") != std::string::npos);
    // Y el sobrante va a la celda de encima, que se apoya en la ya llena.
    CHECK(cuerpo.find("fb.sobrante") != std::string::npos);
}
