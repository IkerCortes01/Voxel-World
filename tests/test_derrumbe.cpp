#include <doctest/doctest.h>
#include "FisicaCaida.h"
// El agua con nivel vive aqui: hace falta para comprobar que un arbol al caer
// NO la borra (ver el test "el agua NUNCA se borra").
#include "BloqueCompuesto.h"

// ============================================================================
// isCrossSprite PARA LOS TESTS
// ============================================================================
// esSueloFirme() la necesita para excluir la vegetacion, pero la del motor
// vive en main.cpp y arrastra el juego entero (texturas, mesher, OpenGL). Los
// tests no enlazan main.cpp, asi que aportan la suya.
//
// Reconoce las plantas por TIPO, que es lo unico que la regla de soporte
// necesita saber: una planta no sostiene una estructura, tenga la geometria
// que tenga. Va en el namespace Fisica porque es ahi donde el header la
// declara.
namespace Fisica {
bool isCrossSprite(BlockType type) {
    if (type == BLOCK_TALLGRASS) return true;
    if (esGuijarro(type) || esRaiz(type)) return true;
    if (esIxtle(type)) return true;
    if (esCompartido(type)) return true;
    if (esCladodio(type) || type == BLOCK_NOPAL_FRUTO || esTuna(type))
        return true;

    switch (type) {
        case BLOCK_RAMA_PINO:
        case BLOCK_RAMA_ENCINO:
        case BLOCK_RAMA_OYAMEL:
        // ⭐ El OCOTE es la cuarta especie y se quedo fuera de esta lista al
        // anadirse. Sin el, su rama no contaba como sprite en los tests.
        case BLOCK_RAMA_OCOTE:
        case BLOCK_NOPAL_MOJADO:
        case BLOCK_NOPAL_TIRAS:
        case BLOCK_NOPAL_SIN_BABA:
        case BLOCK_NOPAL_BABA:
        case BLOCK_NOPAL_SECO:
        case BLOCK_MAGUEY_PUNTA:
        case BLOCK_MAGUEY_HUECO:
        case BLOCK_AGUAMIEL:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// esRamaParaFisica PARA LOS TESTS
// ============================================================================
// Mismo caso que la de arriba: la del motor (isRama) vive en main.cpp.
// aplastablePorArbol() la usa para NO machacar el ramaje del propio arbol.
bool esRamaParaFisica(BlockType type) {
    return type == BLOCK_RAMA_PINO   || type == BLOCK_RAMA_ENCINO ||
           type == BLOCK_RAMA_OYAMEL || type == BLOCK_RAMA_OCOTE;
}
} // namespace Fisica

using Fisica::puedeCaer;
using Fisica::esSueloFirme;
using Fisica::esTerrenoNatural;

// ============================================================================
// TESTS DEL DERRUMBE: QUE CAE Y QUE NO
// ============================================================================
// Estos tests nacen de un bug REAL que destruia terreno.
//
// SINTOMA: al romper un bloque cualquiera aparecian parches de piedra tirados
// a ras de suelo sobre el pasto, con cuadros verdes incrustados y huecos
// donde antes habia suelo.
//
// CAUSA: el sistema de estructuras (el que hace que un arbol talado se venga
// abajo) trataba el TERRENO como una estructura desprendible. Al recorrer la
// piedra hacia abajo, el vecino inferior era mas piedra -- que tambien
// "podia caer" -- asi que en vez de contar como APOYO se sumaba a la pieza.
// El recorrido se comia la columna entera y, si el trozo no llegaba al tope
// de 512 bloques, todo ese terreno se desprendia y caia.
//
// LA REGLA QUE LO ARREGLA, y que estos tests protegen:
//
//     lo que SUJETA una estructura es cimiento, y un cimiento NO CAE
//
// Es decir: esSueloFirme(t) implica !puedeCaer(t). Si alguien rompe esa
// equivalencia -- por ejemplo haciendo que la arena caiga como en otros
// juegos -- reaparece el bug, y estos tests lo cazan antes de que llegue al
// jugador.

// ----------------------------------------------------------------------------
// LA INVARIANTE CENTRAL
// ----------------------------------------------------------------------------

TEST_CASE("Derrumbe: el TERRENO NATURAL nunca puede caer") {
    // La invariante que arregla el bug. El terreno es el cimiento del mundo:
    // no puede desprenderse, porque no hay nada debajo sobre lo que caer.
    //
    // Se barre el enum entero para que un bloque nuevo no pueda colarse en la
    // lista de terreno y volverse desprendible sin que nadie lo note.
    for (int id = 0; id <= BLOCK_TYPE_MAX; ++id) {
        const BlockType t = (BlockType)id;
        if (esTerrenoNatural(t)) {
            INFO("el terreno id=", id, " se puede caer");
            CHECK_FALSE(puedeCaer(t));
        }
    }
}

TEST_CASE("Derrumbe: el terreno corriente se queda donde esta") {
    // Los sospechosos del bug, uno por uno. Son los bloques con los que esta
    // hecho el suelo del mundo.
    const BlockType TERRENO[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_GRAVEL,
        BLOCK_CLAY, BLOCK_CLAY_DIRT, BLOCK_CLAY_SAND, BLOCK_LIMESTONE,
        BLOCK_COBBLESTONE, BLOCK_SNOW
    };
    for (BlockType t : TERRENO) {
        INFO("bloque ", (int)t);
        CHECK(esSueloFirme(t));
        CHECK_FALSE(puedeCaer(t));
    }
}

TEST_CASE("Derrumbe: los minerales tampoco caen") {
    // Van incrustados en la roca: si cayeran, minar una veta abriria un
    // socavon en la montaña.
    const BlockType VETAS[] = {
        BLOCK_COAL_ORE, BLOCK_SILVER_ORE, BLOCK_GOLD_ORE,
        BLOCK_DIAMOND_ORE, BLOCK_SCRAP_METAL, BLOCK_PYRITE_ORE
    };
    for (BlockType t : VETAS) {
        INFO("mineral ", (int)t);
        CHECK_FALSE(puedeCaer(t));
    }
}

TEST_CASE("Derrumbe: las capas parciales de terreno tampoco caen") {
    // Una loncha de tierra es tierra. Si la capa cayera y el bloque entero
    // no, el mismo material se comportaria de dos formas distintas segun su
    // altura -- y el bug volveria por esa puerta.
    const BlockType BASES[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_GRAVEL
    };
    for (BlockType base : BASES)
        for (int nivel = 1; nivel <= 7; ++nivel) {
            const BlockType capa = conNivel(base, nivel);
            INFO("capa de ", (int)base, " nivel ", nivel);
            CHECK_FALSE(puedeCaer(capa));
        }
}

TEST_CASE("Derrumbe: las celdas mixtas de terreno tampoco caen") {
    // Una celda mixta esta LLENA: es suelo a todos los efectos.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_SAND);
    REQUIRE(m != BLOCK_AIR);
    CHECK(esSueloFirme(m));
    CHECK_FALSE(puedeCaer(m));
}

// ----------------------------------------------------------------------------
// LO QUE SI TIENE QUE SEGUIR CAYENDO
// ----------------------------------------------------------------------------
// El arreglo no puede llevarse por delante la funcion que el sistema tenia:
// que un arbol talado se venga abajo.

TEST_CASE("Derrumbe: los arboles siguen cayendo") {
    // Si esto fallara, el arreglo habria roto la caida de arboles: talar
    // dejaria la copa flotando en el aire.
    const BlockType ARBOL[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL,
        BLOCK_RAMA_PINO, BLOCK_RAMA_ENCINO, BLOCK_RAMA_OYAMEL
    };
    for (BlockType t : ARBOL) {
        INFO("pieza de arbol ", (int)t);
        CHECK(puedeCaer(t));
        CHECK_FALSE(esSueloFirme(t));   // y no sujeta a nadie
    }
}

TEST_CASE("Derrumbe: la madera trabajada tambien cae") {
    // Un puente o una torre de tablones son construccion, no cimiento: si le
    // quitas el apoyo, se viene abajo.
    const BlockType TABLAS[] = {
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL
    };
    for (BlockType t : TABLAS) {
        INFO("tablon ", (int)t);
        CHECK(puedeCaer(t));
    }
}

// ----------------------------------------------------------------------------
// LOS CASOS QUE YA ESTABAN RESUELTOS Y NO DEBEN ROMPERSE
// ----------------------------------------------------------------------------

TEST_CASE("Derrumbe: el aire y los liquidos no entran en el sistema") {
    CHECK_FALSE(puedeCaer(BLOCK_AIR));
    CHECK_FALSE(puedeCaer(BLOCK_WATER));
    CHECK_FALSE(puedeCaer(BLOCK_LAVA));
    // Y no sujetan nada: un arbol sobre agua se cae.
    CHECK_FALSE(esSueloFirme(BLOCK_AIR));
    CHECK_FALSE(esSueloFirme(BLOCK_WATER));
    CHECK_FALSE(esSueloFirme(BLOCK_LAVA));
}

TEST_CASE("Derrumbe: la bedrock es el fondo del mundo") {
    CHECK_FALSE(puedeCaer(BLOCK_BEDROCK));
}

TEST_CASE("Derrumbe: los guijarros se quedan donde estan") {
    // No son bloques que llenen su celda: son un montoncito apoyado. Ademas
    // van sembrados por toda la superficie, asi que meterlos en el recorrido
    // costaria tiempo sin que cayera ninguno.
    const BlockType CANTOS[] = {
        BLOCK_PEDAZO_PIEDRA, BLOCK_PEDAZO_GRAVA, BLOCK_PEDAZO_PEDERNAL,
        BLOCK_PEDAZO_CALIZA, BLOCK_PEDAZO_TIERRA, BLOCK_PEDAZO_COBRE
    };
    for (BlockType t : CANTOS) {
        INFO("guijarro ", (int)t);
        CHECK_FALSE(puedeCaer(t));
    }
}

TEST_CASE("Derrumbe: las plantas no sostienen estructuras") {
    // Si una planta sujetara, un arbol cuyas hojas rozaran otro arbol no
    // caeria nunca.
    CHECK_FALSE(esSueloFirme(BLOCK_TALLGRASS));
    CHECK_FALSE(esSueloFirme(BLOCK_LEAVES));
    CHECK_FALSE(esSueloFirme(BLOCK_WOOD));
}

// ----------------------------------------------------------------------------
// BARRIDO COMPLETO: NINGUN BLOQUE EN TIERRA DE NADIE
// ----------------------------------------------------------------------------

TEST_CASE("Derrumbe: el terreno natural nunca cae, y nada mas esta exento") {
    // La red final, sobre la invariante DE VERDAD:
    //
    //     esTerrenoNatural(t)  =>  !puedeCaer(t)
    //
    // ⚠️ Al escribir este test lo puse al reves -- "lo que sujeta no cae" --
    // y salto en 81 bloques. Tenia razon el codigo, no el test: TABLONES,
    // bases de nopal y tazones aguantan peso Y PUEDEN CAERSE, porque son
    // construccion y planta, no cimiento. Un puente de madera sostiene lo que
    // le eches encima y aun asi se viene abajo si le quitas los pilares.
    //
    // La invariante correcta es mas estrecha: solo el TERRENO NATURAL esta
    // exento de caerse. Todo lo demas -- incluso lo que sujeta -- puede
    // derrumbarse si se queda sin apoyo.
    int terreno = 0, caen = 0, rotas = 0;

    for (int id = 1; id <= BLOCK_TYPE_MAX; ++id) {
        const BlockType t = (BlockType)id;
        const bool esCimiento = esTerrenoNatural(t);
        const bool cae        = puedeCaer(t);

        if (esCimiento) ++terreno;
        if (cae)        ++caen;

        if (esCimiento && cae) {
            ++rotas;
            INFO("el terreno id=", id, " puede caerse: vuelve el bug");
            CHECK(false);
        }
    }

    CHECK(rotas == 0);

    // Y que ninguna lista quede vacia. Si `caen` fuera 0 el arreglo se habria
    // pasado de frenada y no se derrumbaria nada; si `terreno` fuera 0, el
    // suelo volveria a ser desprendible.
    CHECK(terreno > 0);
    CHECK(caen > 0);
}

TEST_CASE("Derrumbe: lo que sujeta SIN ser terreno si puede caerse") {
    // El caso que corrigio el test anterior, fijado a proposito para que no
    // se "arregle" por error en el futuro: un tablon sostiene y se cae.
    const BlockType CONSTRUCCION[] = {
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL
    };
    for (BlockType t : CONSTRUCCION) {
        INFO("construccion ", (int)t);
        CHECK(esSueloFirme(t));          // aguanta peso...
        CHECK_FALSE(esTerrenoNatural(t)); // ...pero no es cimiento...
        CHECK(puedeCaer(t));              // ...asi que se derrumba.
    }
}

// ============================================================================
// EL ARBOL QUE CAE APLASTA LO QUE PILLA DEBAJO
// ============================================================================
// Antes, al aterrizar, cada bloque del arbol comprobaba `!= BLOCK_AIR` y se
// DESCARTABA si la celda estaba ocupada. Talar un pino sobre un maguey no
// aplastaba el maguey: hacia desaparecer medio arbol sin dejar rastro.
//
// Ahora un tronco revienta lo blando y ocupa su sitio, pero se apoya sobre lo
// firme en vez de perforarlo.

using Fisica::aplastablePorArbol;

TEST_CASE("Aplastar: las plantas ceden bajo un arbol que cae") {
    // Lo que un tronco se lleva por delante en el campo.
    const BlockType BLANDO[] = {
        BLOCK_TALLGRASS,
        BLOCK_NOPAL_CLADODIO, BLOCK_NOPAL_FRUTO,   // el nopal
        BLOCK_TUNA,                                 // y su fruto
        BLOCK_IXTLE_HOJA,                           // el maguey pulquero
        BLOCK_MAGUEY_PUNTA, BLOCK_AGUAMIEL,
    };
    for (BlockType t : BLANDO) {
        INFO("planta ", (int)t);
        CHECK(aplastablePorArbol(t));
    }
}

TEST_CASE("Aplastar: el terreno firme NO se perfora") {
    // Un arbol se apoya en el suelo, no lo atraviesa. Si esto fallara, un
    // pino cayendo abriria un agujero en la roca.
    const BlockType FIRME[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL,
        BLOCK_LIMESTONE, BLOCK_COBBLESTONE, BLOCK_BEDROCK,
        BLOCK_PLANKS,                     // la construccion tampoco
    };
    for (BlockType t : FIRME) {
        INFO("firme ", (int)t);
        CHECK_FALSE(aplastablePorArbol(t));
    }
}

TEST_CASE("Aplastar: un arbol no se come a otro arbol") {
    // Las piezas del propio arbol se respetan: dos arboles que caen juntos
    // quedan los dos tumbados. Ademas los bloques se colocan uno a uno, asi
    // que sin esto un tronco podria borrar al que acaba de posarse.
    const BlockType ARBOL[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_WOOD_OCOTE, BLOCK_WOOD_OCOTE_DENTRO,
        BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL,
        BLOCK_LEAVES_OCOTE,
        BLOCK_RAMA_PINO, BLOCK_RAMA_ENCINO, BLOCK_RAMA_OYAMEL,
        BLOCK_RAMA_OCOTE,
    };
    for (BlockType t : ARBOL) {
        INFO("pieza de arbol ", (int)t);
        CHECK_FALSE(aplastablePorArbol(t));
    }
}

TEST_CASE("Aplastar: el agua NUNCA se borra") {
    // EL CASO PELIGROSO. El agua con nivel es un bloque COMPUESTO, igual que
    // el maguey, asi que sin un filtro propio caeria en "es planta, se
    // aplasta" y un arbol al caer en un charco lo EVAPORARIA.
    //
    // El sistema de fluidos se sostiene sobre lo contrario: el agua no se
    // crea ni se destruye, solo se reparte. Un tronco no puede secar un lago.
    CHECK_FALSE(aplastablePorArbol(BLOCK_WATER));
    CHECK_FALSE(aplastablePorArbol(BLOCK_LAVA));
    for (int n = 1; n <= 8; ++n) {
        INFO("agua con nivel ", n);
        CHECK_FALSE(aplastablePorArbol(Compuesto::Agua::nuevo((uint16_t)n)));
    }
}

TEST_CASE("Aplastar: una capa suelta cede, el bloque entero no") {
    // Una loncha fina de tierra en el suelo no detiene un arbol; el bloque
    // asentado si. El corte esta en la mitad.
    for (int n = 1; n <= 4; ++n) {
        INFO("capa fina de ", n, " octavos");
        CHECK(aplastablePorArbol(conNivel(BLOCK_DIRT, n)));
    }
    for (int n = 5; n <= 7; ++n) {
        INFO("capa gruesa de ", n, " octavos");
        CHECK_FALSE(aplastablePorArbol(conNivel(BLOCK_DIRT, n)));
    }
    CHECK_FALSE(aplastablePorArbol(BLOCK_DIRT));   // el bloque entero aguanta
}

TEST_CASE("Aplastar: el aire no es algo que aplastar") {
    // El aterrizaje comprueba el aire por su cuenta; aqui solo se fija que
    // esta funcion no lo reclame como "aplastable".
    CHECK_FALSE(aplastablePorArbol(BLOCK_AIR));
}

// ============================================================================
// LAS CUATRO ESPECIES CAEN, INCLUIDO EL OCOTE
// ============================================================================
TEST_CASE("Derrumbe: el OCOTE tambien se viene abajo") {
    // El ocote es la cuarta especie y se anadio despues, asi que no estaba en
    // el test de arboles de mas arriba. Si alguna de sus piezas dejara de
    // caer, talar un ocote dejaria la copa flotando.
    const BlockType OCOTE[] = {
        BLOCK_WOOD_OCOTE, BLOCK_WOOD_OCOTE_DENTRO,
        BLOCK_LEAVES_OCOTE, BLOCK_RAMA_OCOTE,
    };
    for (BlockType t : OCOTE) {
        INFO("pieza de ocote ", (int)t);
        CHECK(puedeCaer(t));
        CHECK_FALSE(esSueloFirme(t));   // y no sujeta a nadie
    }
}
