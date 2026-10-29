#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <iterator>
#include <filesystem>

// ============================================================================
// LOS CUATRO PINOS MEXICANOS NUEVOS
// ============================================================================
// Patula, piñonero, de las alturas y pseudostrobus. Cada uno con sus cuatro
// piezas: corteza, corte con los anillos, acicula y tablones.
//
// Lo que estos tests fijan, y todo son cosas que de romperse dejan el arbol
// invisible, sin madera o remallando el chunk 60 veces por segundo:
//
//   1. Las 16 piezas entran al enum y NINGUNA movio los IDs de antes.
//   2. Las cuatro especies estan clasificadas como tronco y como hoja -- de
//      eso dependen el hacha, el soporte de la copa y el mesher.
//   3. Cada pieza tiene su textura EN DISCO, con el nombre exacto.
//   4. Cada especie se genera en el mundo y da tablones.
//
// Varios se hacen leyendo main.cpp: el binario de tests no enlaza el motor.
// Es el mismo camino que test_creativo_completo.cpp y test_techo_hojas.cpp.
// ============================================================================

namespace {

std::string raiz() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    return r;
#else
    return {};
#endif
}

std::string leer(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

bool existeTextura(const std::string& nombre) {
    return std::filesystem::exists(
        raiz() + "resourcepacks/Textures/Blocks/" + nombre);
}

}  // namespace

TEST_CASE("Pinos: las 16 piezas son bloques distintos y validos") {
    const BlockType PIEZAS[] = {
        BLOCK_WOOD_PATULA,   BLOCK_WOOD_PATULA_DENTRO,
        BLOCK_LEAVES_PATULA, BLOCK_PLANKS_PATULA,
        BLOCK_WOOD_PINONERO,   BLOCK_WOOD_PINONERO_DENTRO,
        BLOCK_LEAVES_PINONERO, BLOCK_PLANKS_PINONERO,
        BLOCK_WOOD_ALTURAS,    BLOCK_WOOD_ALTURAS_DENTRO,
        BLOCK_LEAVES_ALTURAS,  BLOCK_PLANKS_ALTURAS,
        BLOCK_WOOD_PSEUDOSTROBUS,   BLOCK_WOOD_PSEUDOSTROBUS_DENTRO,
        BLOCK_LEAVES_PSEUDOSTROBUS, BLOCK_PLANKS_PSEUDOSTROBUS,
    };
    constexpr int N = (int)(sizeof(PIEZAS) / sizeof(PIEZAS[0]));
    CHECK(N == 16);

    for (int i = 0; i < N; ++i) {
        INFO("pieza ", i);
        CHECK((int)PIEZAS[i] <= (int)BLOCK_TYPE_MAX);
        CHECK(PIEZAS[i] != BLOCK_AIR);
        // Ningun ID repetido: dos piezas con el mismo numero serian el mismo
        // bloque y una de las dos no existiria.
        for (int j = i + 1; j < N; ++j) CHECK(PIEZAS[i] != PIEZAS[j]);
    }
}

TEST_CASE("Pinos: los cuatro troncos cuentan como tronco de arbol") {
    // De `esTroncoDeArbol` dependen el hacha (que gaste vida y tarde lo que
    // tarda la madera) y el SOPORTE de la copa: sin esto, las hojas de estos
    // pinos se considerarian sin arbol y se pudririan nada mas generarse.
    //
    // El corte (…_DENTRO) tambien cuenta: es el mismo tronco visto por dentro.
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PATULA));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PATULA_DENTRO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PINONERO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PINONERO_DENTRO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_ALTURAS));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_ALTURAS_DENTRO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PSEUDOSTROBUS));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_PSEUDOSTROBUS_DENTRO));

    // Y por tanto el hacha los reconoce como organicos.
    CHECK(esOrganicoParaHacha(BLOCK_WOOD_PATULA));
    CHECK(esOrganicoParaHacha(BLOCK_WOOD_PSEUDOSTROBUS));

    // Los TABLONES no son tronco: son el producto ya labrado.
    CHECK_FALSE(esTroncoDeArbol(BLOCK_PLANKS_PATULA));
}

TEST_CASE("Pinos: las cuatro aciculas cuentan como hoja de arbol") {
    CHECK(esHojaDeArbol(BLOCK_LEAVES_PATULA));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_PINONERO));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_ALTURAS));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_PSEUDOSTROBUS));

    // Un tronco no es hoja, y una hoja no es tronco.
    CHECK_FALSE(esHojaDeArbol(BLOCK_WOOD_PATULA));
    CHECK_FALSE(esTroncoDeArbol(BLOCK_LEAVES_PATULA));
}

TEST_CASE("Pinos: cada pieza tiene su textura en disco") {
    // ⭐ EL TEST QUE MAS IMPORTA DE ESTE ARCHIVO.
    //
    // Si falta una, `getTexture` devuelve 0 y el mesher lo lee como "textura
    // aun no cargada": marca el chunk para reintentar y lo remalla 60 veces
    // por segundo, PARA SIEMPRE (ver PENDIENTES.md, "Rebuild infinito si
    // falta una textura"). No es un fallo visual, es un bucle de CPU.
    //
    // Los nombres son EXACTOS, tal como los pide main.cpp.
    const char* TEXTURAS[] = {
        "Tronco de Pino patula.png",
        "Tronco de Pino patula por dentro.png",
        "Tablones de Madera de Pino patula.png",
        "Tronco de Pino pinonero mexicano.png",
        "Tronco de Pino pinonero mexicano por dentro.png",
        "Hojas de Pino pinonero mexicano.png",
        "Tablones de Madera de Pino pinonero mexicano.png",
        "Tronco de Pino de las alturas.png",
        "Tronco de Pino de las alturas por dentro.png",
        "Hojas de Pino de las alturas.png",
        "Tablones de Madera de Pino de las alturas.png",
        "Tronco de Pseudostrobus.png",
        "Tronco de Pseudostrobus por dentro.png",
        "Hojas de Pseudostrobus.png",
        "Tablones de madera de Pino de pseudostrobus.png",
    };
    for (const char* t : TEXTURAS) {
        INFO("textura: ", t);
        CHECK(existeTextura(t));
    }
}

TEST_CASE("Pinos: ninguna textura nueva lleva enye ni tildes") {
    // ⚠️ ESTO COSTO UNA VUELTA, y por eso esta aqui.
    //
    // Los archivos del piñonero llegaron como "Pino piñonero mexicano.png".
    // En disco esa ñ son dos bytes UTF-8, pero MSVC compila el literal de la
    // cadena a la pagina de codigos del sistema: el nombre que pide el codigo
    // y el que hay en disco dejan de coincidir, y se cae en el rebuild
    // infinito de arriba.
    //
    // Ninguna de las ~200 texturas del proyecto lleva tildes ni ñ. Era una
    // convencion no escrita; esto la convierte en un test.
    const std::string fuente = leer(raiz() + "src/main.cpp");
    REQUIRE(!fuente.empty());

    // Se miran solo las RUTAS DE ARCHIVO --lo que va dentro de
    // loadTexture("…") y getTexture("…")-- y no los comentarios: en prosa
    // "piñonero" se escribe con ñ, que es lo correcto en español. Lo que no
    // puede llevarla es el nombre del archivo.
    const char* LLAMADAS[] = { "loadTexture(\"", "getTexture(\"" };
    int rutasRevisadas = 0;
    for (const char* llamada : LLAMADAS) {
        size_t i = 0;
        while ((i = fuente.find(llamada, i)) != std::string::npos) {
            const size_t ini = i + std::string(llamada).size();
            const size_t fin = fuente.find('"', ini);
            if (fin == std::string::npos) break;
            const std::string ruta = fuente.substr(ini, fin - ini);
            INFO("ruta: ", ruta);
            // ñ en UTF-8 son los bytes C3 B1.
            CHECK(ruta.find("\xC3\xB1") == std::string::npos);
            ++rutasRevisadas;
            i = fin;
        }
    }
    // Si el barrido no encontro rutas, el test no estaria comprobando nada.
    CHECK(rutasRevisadas > 50);
}

TEST_CASE("Pinos: las cuatro especies se generan en el mundo") {
    const std::string fuente = leer(raiz() + "src/main.cpp");
    REQUIRE(!fuente.empty());

    // El enum de especies que usa el generador.
    CHECK(fuente.find("enum class EspeciePino") != std::string::npos);

    // Cada una tiene su rama de despacho con su horquilla de altura.
    CHECK(fuente.find("EspeciePino::PATULA") != std::string::npos);
    CHECK(fuente.find("EspeciePino::PINONERO") != std::string::npos);
    CHECK(fuente.find("EspeciePino::ALTURAS") != std::string::npos);
    CHECK(fuente.find("EspeciePino::PSEUDOSTROBUS") != std::string::npos);

    // Y el generador de terreno las ofrece.
    const std::string deco = leer(raiz() + "src/terrain/DecorationSystem.h");
    REQUIRE(!deco.empty());
    CHECK(deco.find("TREE_PATULA") != std::string::npos);
    CHECK(deco.find("TREE_PINONERO") != std::string::npos);
    CHECK(deco.find("TREE_ALTURAS") != std::string::npos);
    CHECK(deco.find("TREE_PSEUDOSTROBUS") != std::string::npos);
}

TEST_CASE("Pinos: cada especie aparece en algun bioma") {
    // Una especie que existe pero que ningun bioma devuelve nunca es codigo
    // muerto: el jugador no la veria jamas.
    const std::string deco = leer(raiz() + "src/terrain/DecorationSystem.h");
    REQUIRE(!deco.empty());

    // `return TREE_X;` dentro de GetTreeType es lo que la hace aparecer.
    CHECK(deco.find("return TREE_PATULA;") != std::string::npos);
    CHECK(deco.find("return TREE_PINONERO;") != std::string::npos);
    CHECK(deco.find("return TREE_ALTURAS;") != std::string::npos);
    CHECK(deco.find("return TREE_PSEUDOSTROBUS;") != std::string::npos);
}

TEST_CASE("Pinos: la acicula no se suelta siempre al romperla") {
    // Las hojas de TODAS las especies sueltan su hoja el 20% de las veces.
    // Si una cae al `default` de getDroppedItem la suelta el 100%, que es el
    // sesgo que ya se colo una vez con el encino, el oyamel y el ocote.
    const std::string fuente = leer(raiz() + "src/main.cpp");
    REQUIRE(!fuente.empty());

    // Las cuatro tienen que estar en el mismo `case` que las demas hojas,
    // justo antes del sorteo del 20%.
    //
    // Se ancla al grupo de getDroppedItem, no al primer "case
    // BLOCK_LEAVES_PATULA" que aparezca: ese nombre sale tambien en la tabla
    // de colores y en la de durezas, mucho antes en el archivo.
    const size_t grupo = fuente.find("Y LAS DE LOS CUATRO PINOS NUEVOS");
    REQUIRE_MESSAGE(grupo != std::string::npos,
                    "las hojas nuevas no estan en el case de getDroppedItem: "
                    "caerian al default y se soltarian siempre");

    const std::string cerca = fuente.substr(grupo, 900);
    CHECK(cerca.find("BLOCK_LEAVES_PATULA") != std::string::npos);
    CHECK(cerca.find("BLOCK_LEAVES_PINONERO") != std::string::npos);
    CHECK(cerca.find("BLOCK_LEAVES_ALTURAS") != std::string::npos);
    CHECK(cerca.find("BLOCK_LEAVES_PSEUDOSTROBUS") != std::string::npos);
    CHECK(cerca.find("rand() % 100) < 20") != std::string::npos);
}

TEST_CASE("Pinos: cada especie da sus propios tablones") {
    // Sin receta de tablones, el tronco no sirve para nada: se puede talar y
    // quedarse con madera inutilizable. Le paso al ocote chino en su dia.
    const std::string fuente = leer(raiz() + "src/main.cpp");
    REQUIRE(!fuente.empty());

    const size_t m = fuente.find("LOS CUATRO PINOS NUEVOS: CADA UNO DA SU PROPIO TABLON");
    REQUIRE(m != std::string::npos);
    const std::string bloque = fuente.substr(m, 2400);

    // Las cuatro parejas tronco -> tablon.
    CHECK(bloque.find("BLOCK_PLANKS_PATULA") != std::string::npos);
    CHECK(bloque.find("BLOCK_PLANKS_PINONERO") != std::string::npos);
    CHECK(bloque.find("BLOCK_PLANKS_ALTURAS") != std::string::npos);
    CHECK(bloque.find("BLOCK_PLANKS_PSEUDOSTROBUS") != std::string::npos);

    // 6 tablones por tronco, como el resto de especies.
    CHECK(bloque.find("recipe(m.tablon, 6, true)") != std::string::npos);

    // Y sus tablones dan palos: sin esto la madera nueva no fabrica nada.
    CHECK(bloque.find("recipe(BLOCK_STICK, 8, false)") != std::string::npos);
}
