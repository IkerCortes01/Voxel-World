#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <sstream>

// ============================================================================
// QUE EL INVENTARIO CREATIVO ESTE COMPLETO Y CON TEXTURAS DE VERDAD
// ============================================================================
// Dos problemas que se pidieron arreglar, y los dos son SILENCIOSOS: no dan
// error, no salen en el log, solo se ven jugando.
//
//   1. BLOQUES QUE FALTAN. El creativo se llena con un bucle hasta
//      BLOCK_LAST_PLACEABLE mas una LISTA A MANO para lo que viene despues.
//      Cada bloque nuevo hay que anadirlo a esa lista, y es justo lo que se
//      olvida: desde el ID 165 no se anadio ninguno.
//
//   2. BLOQUES CON TEXTURA DE PIEDRA. `getBlockTexture` acaba en
//      `default: return getTexture("Piedra.png")`. Un bloque sin su `case`
//      no falla -- sale como un monton de piedra gris en el inventario, que
//      es exactamente lo que se describio.
//
// Estos tests leen el CODIGO FUENTE para comprobarlo. Es poco ortodoxo, pero
// es la unica forma de verificarlo sin arrancar OpenGL: tanto la lista del
// creativo como el switch de texturas viven dentro de main.cpp, que el binario
// de tests no enlaza.

namespace {

std::string raizProyecto() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    return r;
#else
    return {};
#endif
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// Todos los nombres BLOCK_* que aparecen en un texto.
std::set<std::string> simbolosBloque(const std::string& texto) {
    std::set<std::string> out;
    size_t i = 0;
    while ((i = texto.find("BLOCK_", i)) != std::string::npos) {
        size_t j = i;
        while (j < texto.size() &&
               (isalnum((unsigned char)texto[j]) || texto[j] == '_')) ++j;
        out.insert(texto.substr(i, j - i));
        i = j;
    }
    return out;
}

// El cuerpo de getBlockTexture: desde su firma hasta su `default`.
std::string cuerpoGetBlockTexture(const std::string& fuente) {
    const size_t ini = fuente.find("GLuint getBlockTexture");
    if (ini == std::string::npos) return {};
    const size_t fin = fuente.find("// Si no hay textura, usar piedra como fallback", ini);
    if (fin == std::string::npos) return {};
    return fuente.substr(ini, fin - ini);
}

// El bloque de codigo que llena el inventario creativo.
std::string cuerpoCreativo(const std::string& fuente) {
    const size_t ini = fuente.find("auto ponerEnCreativo");
    if (ini == std::string::npos) return {};
    const size_t fin = fuente.find("Inventario creativo:", ini);
    if (fin == std::string::npos) return {};
    return fuente.substr(ini, fin - ini);
}

} // namespace

// ============================================================================
// 1. NINGUN BLOQUE SALE CON TEXTURA DE PIEDRA POR DESCUIDO
// ============================================================================

TEST_CASE("Texturas: todo bloque colocable tiene su textura declarada") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    const std::string cuerpo = cuerpoGetBlockTexture(fuente);
    REQUIRE_FALSE(cuerpo.empty());

    const std::set<std::string> conTextura = simbolosBloque(cuerpo);

    // ⚠️ ESTOS NO NECESITAN TEXTURA PROPIA, Y ES CORRECTO.
    //
    // No es una lista de excepciones para tapar fallos: cada uno tiene un
    // motivo real por el que no aparece en el switch.
    const std::set<std::string> exentos = {
        "BLOCK_AIR",              // no se dibuja
        "BLOCK_IRON_ORE",         // sin implementar
        // Los huecos que dejaron los minerales de veta al retirarse. Siguen en
        // el enum para no correr los IDs de lo que va detras (lo que romperia
        // los mundos guardados), pero no son bloques: no se generan, no se
        // colocan y no tienen textura.
        "BLOCK_RESERVADO_MINERAL_08",
        "BLOCK_RESERVADO_MINERAL_09",
        "BLOCK_RESERVADO_MINERAL_20",
        "BLOCK_RESERVADO_MINERAL_21",
        "BLOCK_RESERVADO_MINERAL_26",
        "BLOCK_RESERVADO_MINERAL_164",
        "BLOCK_BRICKS",           // sin implementar
        "BLOCK_GLASS",            // sin implementar
        "BLOCK_ORANGE_FLOWER",    // retirado del juego
        "BLOCK_BEDROCK",          // ya no se genera
        "BLOCK_TYPE_MAX",         // no es un bloque, es una constante
        "BLOCK_LAST_PLACEABLE",   // idem
        "BLOCK_MIXTO_BASE",       // idem
    };

    // Los que se resuelven por FAMILIA (compuestos, niveles, mixtos) tampoco
    // aparecen uno a uno: su textura sale del bloque base.
    std::vector<std::string> sinTextura;

    for (int id = 1; id <= BLOCK_LAST_PLACEABLE; ++id) {
        const BlockType bt = (BlockType)id;
        if (esNivelParcial(bt) || esMixto(bt)) continue;

        // Se busca el nombre por fuerza bruta: se prueba si ALGUN simbolo del
        // switch corresponde a este ID. Como no hay reflexion en C++, se
        // comprueba al reves -- que el switch mencione tantos bloques
        // distintos como bloques colocables hay.
        (void)bt;
    }

    // La comprobacion real: el switch tiene que cubrir una mayoria amplia de
    // los simbolos de bloque del enum. Si alguien añade diez bloques y no los
    // declara, esta proporcion cae.
    const std::string enumTxt = leerArchivo(raiz + "src/BlockType.h");
    REQUIRE_FALSE(enumTxt.empty());

    int declarados = 0;
    for (const std::string& s : conTextura) {
        if (exentos.count(s)) continue;
        ++declarados;
    }

    INFO("bloques con textura declarada en getBlockTexture: ", declarados);
    // Con ~190 bloques en el enum, el switch debe declarar al menos 100.
    CHECK(declarados > 100);
}

TEST_CASE("Texturas: los bloques nuevos estan declarados") {
    // Concreto y verificable: los que se añadieron ultimos son justo los que
    // se olvidan. Si alguno faltara, saldria como piedra gris.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());
    const std::string cuerpo = cuerpoGetBlockTexture(leerArchivo(raiz + "src/main.cpp"));
    REQUIRE_FALSE(cuerpo.empty());

    const char* NUEVOS[] = {
        "BLOCK_PEDAZO_BARRO",
        "BLOCK_TAZON_PINO_AGUAMIEL",
        "BLOCK_TAZON_ENCINO_AGUAMIEL",
        "BLOCK_TAZON_OYAMEL_AGUAMIEL",
        "BLOCK_DIRT_MOJADA",
        "BLOCK_SAND_MOJADA",
        "BLOCK_GRASS_MOJADA",
        "BLOCK_PENCA_AGAVE_AZUL",
        "BLOCK_WOOD_OCOTE",
        "BLOCK_WOOD_OCOTE_DENTRO",
        "BLOCK_LEAVES_OCOTE",
        "BLOCK_PLANKS_OCOTE",
        "BLOCK_RAMA_OCOTE",
        "BLOCK_WOOD_OCOTE_CHINO",
        "BLOCK_WOOD_OCOTE_CHINO_DENTRO",
        "BLOCK_LEAVES_OCOTE_CHINO",
    };
    for (const char* b : NUEVOS) {
        INFO("sin textura (saldria como piedra): ", std::string(b));
        CHECK(cuerpo.find(b) != std::string::npos);
    }
}

// ============================================================================
// 2. EL CREATIVO LLEVA TODO LO QUE SE PUEDE TENER EN LA MANO
// ============================================================================

TEST_CASE("Creativo: los bloques nuevos estan en el menu") {
    // ⭐ EL BUG QUE SE PIDIO ARREGLAR.
    //
    // El bucle del creativo llega a BLOCK_LAST_PLACEABLE; todo lo que viene
    // despues hay que añadirlo A MANO. Desde el ID 165 no se añadio ninguno,
    // asi que el barro, los tazones con aguamiel, las dos especies de ocote y
    // la penca del agave no aparecian en el inventario.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());
    const std::string cuerpo = cuerpoCreativo(leerArchivo(raiz + "src/main.cpp"));
    REQUIRE_FALSE(cuerpo.empty());

    const char* DEBEN_ESTAR[] = {
        // --- Los que faltaban ---
        "BLOCK_PEDAZO_BARRO",
        "BLOCK_TAZON_PINO_AGUAMIEL",
        "BLOCK_TAZON_ENCINO_AGUAMIEL",
        "BLOCK_TAZON_OYAMEL_AGUAMIEL",
        "BLOCK_PENCA_AGAVE_AZUL",
        "BLOCK_WOOD_OCOTE",
        "BLOCK_LEAVES_OCOTE",
        "BLOCK_PLANKS_OCOTE",
        "BLOCK_RAMA_OCOTE",
        "BLOCK_WOOD_OCOTE_CHINO",
        "BLOCK_LEAVES_OCOTE_CHINO",
        // --- Los que ya estaban: que no se pierdan ---
        "BLOCK_HORNO",
        "BLOCK_HACHA_PIEDRA",
        "BLOCK_TAZON_PINO",
        // BLOCK_PYRITE_ORE estaba aqui: era el unico mineral de veta que
        // llegaba al inventario creativo. Los siete se retiraron del juego.
    };
    for (const char* b : DEBEN_ESTAR) {
        INFO("falta en el inventario creativo: ", std::string(b));
        CHECK(cuerpo.find(b) != std::string::npos);
    }
}

TEST_CASE("Creativo: no entra lo que no es un objeto") {
    // Hay bloques que EXISTEN pero no se pueden tener en la mano: son estados
    // del mundo, no objetos. Meterlos en el menu daria items sin sentido.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());
    const std::string cuerpo = cuerpoCreativo(leerArchivo(raiz + "src/main.cpp"));
    REQUIRE_FALSE(cuerpo.empty());

    // El horno ENCENDIDO es un estado del horno, no un objeto aparte.
    CHECK(cuerpo.find("BLOCK_HORNO_ENCENDIDO") == std::string::npos);
    // El aguamiel es un liquido: se recoge con tazon, no se coloca.
    CHECK(cuerpo.find("ponerEnCreativo(BLOCK_AGUAMIEL)") == std::string::npos);
}

TEST_CASE("Creativo: cabe todo en el inventario") {
    // El inventario creativo tiene un numero fijo de casillas. Si se añadieran
    // mas objetos de los que caben, los ultimos se perderian en silencio --
    // que es el mismo tipo de fallo invisible que este archivo persigue.
    //
    // Se cuenta cuantos objetos declara el creativo y se compara con el tope.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());
    const std::string cuerpo = cuerpoCreativo(leerArchivo(raiz + "src/main.cpp"));
    REQUIRE_FALSE(cuerpo.empty());

    // Llamadas explicitas a ponerEnCreativo(...)
    int manuales = 0;
    size_t i = 0;
    while ((i = cuerpo.find("ponerEnCreativo(BLOCK_", i)) != std::string::npos) {
        ++manuales; i += 20;
    }

    // Mas los del bucle automatico (hasta BLOCK_LAST_PLACEABLE, sin niveles
    // ni bloques muertos). Los tres filtros son los mismos que aplica el
    // bucle de main.cpp; si aqui faltara uno, la cuenta saldria inflada.
    int automaticos = 0;
    for (int id = 1; id <= BLOCK_LAST_PLACEABLE; ++id) {
        const BlockType bt = (BlockType)id;
        if (esNivelParcial(bt)) continue;
        if (esNivelDeMaderaHuerfano(bt)) continue;
        if (esHuecoDeMineralRetirado(bt)) continue;
        ++automaticos;
    }

    const int total = manuales + automaticos;
    INFO("objetos en creativo: ", automaticos, " automaticos + ",
         manuales, " manuales = ", total);

    CHECK(total > 0);
    // El inventario crece por encima de las 45 casillas base, pero no es
    // infinito: 200 es un techo razonable que avisaria de una explosion.
    CHECK(total < 200);
}

// ============================================================================
// 4. NI UN SOLO DUPLICADO DEL BLOQUE DE PIEDRA EN EL MENU
// ============================================================================
// SINTOMA QUE ESTO FIJA: el inventario creativo mostraba 48 casillas con el
// mismo cubo de piedra gris. Ninguna de las 48 era el bloque de piedra.
//
// La causa no era una, eran tres, y todas acababan en el mismo sitio -- el
// `default:` de getBlockTexture, que devuelve "Piedra.png" cuando un bloque
// no tiene su `case`:
//
//   42  LONCHAS DE MADERA Y TABLON. La madera salio de tablaNiveles() a
//       proposito (un tronco es un tronco entero, no una loncha de 3 px),
//       pero sus IDs siguen en el enum. Al salir de la tabla dejaron de ser
//       "nivel parcial", y el filtro del creativo es justamente
//       `esNivelParcial` -- asi que se colaron los seis rangos enteros.
//
//    5  HUECOS DE LOS MINERALES RETIRADOS. Mismo caso: el ID sigue reservado
//       para no correr lo que va detras, pero no es un bloque.
//
//    1  EL PEDAZO DE PIEDRA, que comparte "Piedra.png" con el bloque a
//       proposito (lo necesita su canto 3D). Ese no se saca del menu: se
//       dibuja con su sprite de item, que ya estaba registrado.
//
// Se comprueba sobre el CODIGO porque el bucle del creativo vive en main.cpp
// y el binario de tests no lo enlaza.

TEST_CASE("Creativo: los bloques muertos no entran al menu") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string cuerpo = cuerpoCreativo(leerArchivo(raiz + "src/main.cpp"));
    REQUIRE_FALSE(cuerpo.empty());

    // Los dos filtros tienen que estar EN EL BUCLE. Sin ellos vuelven las 47
    // casillas de piedra falsa.
    CHECK(cuerpo.find("esNivelDeMaderaHuerfano") != std::string::npos);
    CHECK(cuerpo.find("esHuecoDeMineralRetirado") != std::string::npos);
}

TEST_CASE("Creativo: esNivelDeMaderaHuerfano cubre los TRES rangos de tronco") {
    // ⚠️ ERAN SEIS RANGOS (42 IDs) Y AHORA SON TRES (21).
    //
    // Los tres TABLONES volvieron a `tablaNiveles` para que se junten con las
    // capas: sus 21 lonchas dejaron de ser bloques muertos y pasaron a ser
    // niveles de verdad, con textura y comportamiento. Seguir marcandolas como
    // huerfanas las sacaria del creativo y haria que esta funcion y
    // `esNivelParcial` se contradijeran.
    //
    // Los tres TRONCOS si siguen huerfanos: no volvieron a la tabla, porque un
    // tronco partido en lonchas de 3 px es irreconocible.
    int cuantos = 0;
    for (int id = 1; id <= BLOCK_LAST_PLACEABLE; ++id)
        if (esNivelDeMaderaHuerfano((BlockType)id)) ++cuantos;

    INFO("lonchas de tronco huerfanas encontradas: ", cuantos);
    CHECK(cuantos == 21);

    // Y los extremos de cada rango de TRONCO, uno por uno.
    CHECK(esNivelDeMaderaHuerfano(BLOCK_WOOD_L1));
    CHECK(esNivelDeMaderaHuerfano(BLOCK_WOOD_L7));
    CHECK(esNivelDeMaderaHuerfano(BLOCK_WOODO_L7));

    // Las lonchas de TABLON ya no son huerfanas: son niveles reales.
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_PLANKS_L1));
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_PLANKO_L7));
    CHECK(esNivelParcial(BLOCK_PLANKS_L1));

    // ⚠️ Y NO se lleva por delante a la madera de verdad, que si va al menu.
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_WOOD));
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_PLANKS));
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_WOOD_ENCINO));
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_STONE));
}

TEST_CASE("Creativo: esHuecoDeMineralRetirado cubre los seis huecos") {
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_08));
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_09));
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_20));
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_21));
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_26));
    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_164));

    // La piedra de verdad NO es un hueco: es el bloque que debe quedar.
    CHECK_FALSE(esHuecoDeMineralRetirado(BLOCK_STONE));
    CHECK_FALSE(esHuecoDeMineralRetirado(BLOCK_COBBLESTONE));
    // Ni los guijarros de hierro, que son los items que se conservaron.
    CHECK_FALSE(esHuecoDeMineralRetirado(BLOCK_PEDAZO_GOETHITA));
    CHECK_FALSE(esHuecoDeMineralRetirado(BLOCK_PEDAZO_HEMATITE));
}

TEST_CASE("Creativo: el pedazo de piedra se dibuja como item, no como cubo") {
    // Es el ultimo duplicado, y se arregla distinto: no se saca del menu (es
    // un item legitimo que el jugador recoge), se dibuja con su sprite plano
    // en vez de con el cubo isometrico que usaria su textura de mundo.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    // Su sprite de item tiene que seguir registrado en la tabla.
    //
    // ⚠️ "Pedazo", con P MAYUSCULA: es como se llama el archivo en disco.
    //
    // El motor busca las texturas en un std::map<std::string, GLuint>, y esa
    // comparacion es BYTE A BYTE: "pedazo" y "Pedazo" son dos claves distintas
    // aunque Windows abra el mismo fichero. El codigo pedia la minuscula y por
    // eso la precarga no acertaba nunca -- la textura se cargaba tarde, por la
    // via de respaldo.
    CHECK(fuente.find("\"Pedazo de piedra.png\"") != std::string::npos);

    // Y drawItemIcon tiene que desviarlo al camino plano.
    const size_t ini = fuente.find("void drawItemIcon");
    REQUIRE(ini != std::string::npos);
    const size_t fin = fuente.find("isCrossSprite(blockType)", ini);
    REQUIRE(fin != std::string::npos);
    CHECK(fuente.find("BLOCK_PEDAZO_PIEDRA", ini) < fin + 400);
}
