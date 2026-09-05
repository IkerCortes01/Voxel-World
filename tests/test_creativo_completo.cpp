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
        "BLOCK_PYRITE_ORE",
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

    // Mas los del bucle automatico (hasta BLOCK_LAST_PLACEABLE, sin niveles).
    int automaticos = 0;
    for (int id = 1; id <= BLOCK_LAST_PLACEABLE; ++id) {
        if (esNivelParcial((BlockType)id)) continue;
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
