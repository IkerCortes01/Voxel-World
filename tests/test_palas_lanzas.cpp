// PALAS, LANZAS Y LA DAGA DE OBSIDIANA.
// PEDIDO: "anade la pala de piedra, lanza de piedra, daga de obsidiana, lanza
// de pedernal, pala de pedernal" (con las recetas de las capturas).
#include "doctest/doctest.h"

#include "BlockType.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

namespace {
std::string leerP(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
// La receta de `resultado` en main.cpp: que hay en cada casilla que se nombra.
std::string receta(const std::string& src, const char* resultado) {
    const size_t k = src.find(std::string("CraftingRecipe recipe(") + resultado + ", 1, false);");
    if (k == std::string::npos) return {};
    return src.substr(k, src.find("recipes.push_back(recipe);", k) - k);
}
} // namespace

TEST_CASE("Herramientas nuevas: son cinco, con nombre, imagen y vida propia") {
    static const struct { BlockType t; const char* nombre; const char* png; } H[5] = {
        { BLOCK_PALA_PIEDRA,    "PALA DE PIEDRA",    "Pala de piedra.png" },
        { BLOCK_LANZA_PIEDRA,   "LANZA DE PIEDRA",   "Lanza de piedra.png" },
        { BLOCK_DAGA_OBSIDIANA, "DAGA DE OBSIDIANA", "Daga de Obsidiana.png" },
        { BLOCK_LANZA_PEDERNAL, "LANZA DE PEDERNAL", "Lanza de pedernal afilado.png" },
        { BLOCK_PALA_PEDERNAL,  "PALA DE PEDERNAL",  "Pala de Pedernal.png" },
    };
    const std::string src = leerP("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    for (const auto& h : H) {
        INFO(h.nombre);
        CHECK(std::strcmp(NombreItem::nombreDeItem(h.t), h.nombre) == 0);
        CHECK(esHerramientaGastable(h.t));
        CHECK(vidaMaximaHerramienta(h.t) > 0);
        CHECK_FALSE(esRocaParaPico(h.t));               // no son piedra que picar
        CHECK_FALSE(leerP((std::string("resourcepacks/Textures/Items/") + h.png).c_str()).empty());
        CHECK(src.find(std::string("\"") + h.png + "\"") != std::string::npos);                     // su icono
        CHECK(src.find(std::string("siluetaPNG = \"Textures/Items/") + h.png + "\"; break;") != std::string::npos);  // y su forma en la mano
    }
    CHECK((int)BLOCK_PALA_PEDERNAL < (int)BLOCK_TYPE_MAX);
    // La de pedernal aguanta mas que la de piedra; la daga de obsidiana, la que menos.
    CHECK(vidaMaximaHerramienta(BLOCK_PALA_PEDERNAL) > vidaMaximaHerramienta(BLOCK_PALA_PIEDRA));
    CHECK(vidaMaximaHerramienta(BLOCK_LANZA_PEDERNAL) > vidaMaximaHerramienta(BLOCK_LANZA_PIEDRA));
    CHECK(vidaMaximaHerramienta(BLOCK_DAGA_OBSIDIANA) < vidaMaximaHerramienta(BLOCK_LANZA_PIEDRA));
}

TEST_CASE("Pala: cava lo blando, entero o en niveles, y solo se gasta con eso") {
    CHECK(esPala(BLOCK_PALA_PIEDRA));
    CHECK(esPala(BLOCK_PALA_PEDERNAL));
    CHECK_FALSE(esPala(BLOCK_PICO_PIEDRA));
    CHECK_FALSE(esPala(BLOCK_LANZA_PIEDRA));
    for (BlockType b : { BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_CLAY, BLOCK_SNOW }) {
        INFO("bloque " << (int)b);
        CHECK(esTierraParaPala(b));
        CHECK(desgasteHerramienta(BLOCK_PALA_PIEDRA, b) == 2);
        CHECK(desgasteHerramienta(BLOCK_PALA_PEDERNAL, b) == 2);
        if (admiteNiveles(b)) CHECK(esTierraParaPala(primerNivelDe(b)));      // un nivel de tierra tambien
    }
    for (BlockType b : { BLOCK_STONE, BLOCK_WOOD, BLOCK_PLANKS, BLOCK_COBBLESTONE, BLOCK_LEAVES }) {
        INFO("bloque " << (int)b);
        CHECK_FALSE(esTierraParaPala(b));
        CHECK(desgasteHerramienta(BLOCK_PALA_PIEDRA, b) == 0);
    }
    // El motor la hace rapida en lo suyo.
    const std::string src = leerP("src/main.cpp");
    CHECK(src.find("return esTierraParaPala(type) ? PALA_TIERRA_BREAK_TIME") != std::string::npos);
    CHECK(src.find("constexpr float PALA_TIERRA_BREAK_TIME    = 0.6f;") != std::string::npos);
}

TEST_CASE("Lanzas y daga: son armas, pegan mas que un hacha y la lanza llega mas lejos") {
    CHECK(esLanza(BLOCK_LANZA_PIEDRA));
    CHECK(esLanza(BLOCK_LANZA_PEDERNAL));
    CHECK(esDaga(BLOCK_DAGA_OBSIDIANA));
    for (BlockType a : { BLOCK_LANZA_PIEDRA, BLOCK_LANZA_PEDERNAL, BLOCK_DAGA_OBSIDIANA }) {
        CHECK(esArma(a));
        CHECK(danoDeArma(a) > 6);                       // mas que el hacha (6)
        // No se gastan picando bloques: se gastan al herir.
        CHECK(desgasteHerramienta(a, BLOCK_STONE) == 0);
        CHECK(desgasteHerramienta(a, BLOCK_DIRT) == 0);
    }
    CHECK(danoDeArma(BLOCK_LANZA_PEDERNAL) > danoDeArma(BLOCK_LANZA_PIEDRA));
    CHECK(danoDeArma(BLOCK_LANZA_PIEDRA) > danoDeArma(BLOCK_DAGA_OBSIDIANA));
    CHECK(alcanceExtraDeArma(BLOCK_LANZA_PIEDRA) > 1.0f);
    CHECK(alcanceExtraDeArma(BLOCK_DAGA_OBSIDIANA) == 0.0f);
    CHECK(alcanceExtraDeArma(BLOCK_HACHA_PIEDRA) == 0.0f);
    // Lo que no es un arma no cambia nada.
    CHECK_FALSE(esArma(BLOCK_PALA_PIEDRA));
    CHECK_FALSE(esArma(BLOCK_HACHA_PIEDRA));
    CHECK(danoDeArma(BLOCK_HACHA_PIEDRA) == 0);
    const std::string src = leerP("src/main.cpp");
    CHECK(src.find("const int dano = esArma(enMano)    ? danoDeArma(enMano)") != std::string::npos);
    CHECK(src.find("if (esArma(enMano)) state->inventory.gastarHerramienta(2);") != std::string::npos);
    CHECK(src.find("GameState::ALCANCE_GOLPE_FAUNA + alcanceExtraDeArma(state->inventory.getSelectedBlock()));") != std::string::npos);
    // La daga, ademas, corta plantas como las lascas.
    size_t filos = 0;
    for (size_t k = src.find(" esDaga(herramienta) ||"); k != std::string::npos; k = src.find(" esDaga(herramienta) ||", k + 1)) ++filos;
    CHECK(filos == 3);
}

TEST_CASE("Recetas: las cinco, casilla a casilla como en las capturas") {
    const std::string src = leerP("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // Pala: en columna, a la izquierda (0 / 3 / 6).
    std::string r = receta(src, "BLOCK_PALA_PIEDRA");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[0] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(r.find("recipe.pattern[3] = BLOCK_HILO_IXTLE;") != std::string::npos);
    CHECK(r.find("recipe.pattern[6] = BLOCK_STICK;") != std::string::npos);
    r = receta(src, "BLOCK_PALA_PEDERNAL");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[0] = BLOCK_PEDERNAL_AFILADO;") != std::string::npos);
    CHECK(r.find("recipe.pattern[3] = BLOCK_HILO_IXTLE;") != std::string::npos);
    CHECK(r.find("recipe.pattern[6] = BLOCK_STICK;") != std::string::npos);
    // Lanza: en diagonal (0 \ 4 \ 8).
    r = receta(src, "BLOCK_LANZA_PIEDRA");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[0] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(r.find("recipe.pattern[4] = BLOCK_HILO_IXTLE;") != std::string::npos);
    CHECK(r.find("recipe.pattern[8] = BLOCK_STICK;") != std::string::npos);
    r = receta(src, "BLOCK_LANZA_PEDERNAL");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[0] = BLOCK_PEDERNAL_AFILADO;") != std::string::npos);
    CHECK(r.find("recipe.pattern[4] = BLOCK_HILO_IXTLE;") != std::string::npos);
    CHECK(r.find("recipe.pattern[8] = BLOCK_STICK;") != std::string::npos);
    // Daga: en fila, arriba (0 - 1 - 2).
    r = receta(src, "BLOCK_DAGA_OBSIDIANA");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[0] = BLOCK_OBSIDIANA_AFILADA;") != std::string::npos);
    CHECK(r.find("recipe.pattern[1] = BLOCK_HILO_IXTLE;") != std::string::npos);
    CHECK(r.find("recipe.pattern[2] = BLOCK_STICK;") != std::string::npos);
    // Ninguna pisa a las de siempre: el martillo de piedra va en la columna
    // del CENTRO (1 / 4 / 7) y el hacha en 0, 1 y 4.
    r = receta(src, "BLOCK_MARTILLO_PIEDRA");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[1] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    r = receta(src, "BLOCK_HACHA_PIEDRA");
    REQUIRE_FALSE(r.empty());
    CHECK(r.find("recipe.pattern[1] = BLOCK_HILO_IXTLE;") != std::string::npos);
}

// PEDIDO: "cambia el crafteo de los tazones a esto con todas las maderas":
// tablon, piedra, tablon en la fila de en medio y un tablon abajo en el centro.
TEST_CASE("Tazones: tres tablones en U y la piedra en el hueco, con cualquier madera") {
    const std::string src = leerP("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    const size_t ini = src.find("const struct { BlockType tablon, tazon; } t = {");
    REQUIRE(ini != std::string::npos);
    const std::string r = src.substr(ini, src.find("recipes.push_back(recipe);", ini) - ini);
    // La forma de la captura.
    CHECK(r.find("recipe.pattern[3] = t.tablon;") != std::string::npos);
    CHECK(r.find("recipe.pattern[4] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(r.find("recipe.pattern[5] = t.tablon;") != std::string::npos);
    CHECK(r.find("recipe.pattern[7] = t.tablon;") != std::string::npos);
    // Ya no lleva palos.
    CHECK(r.find("BLOCK_STICK") == std::string::npos);
    // Todas las maderas, y cada una da SU tazon: sale de la tabla.
    CHECK(r.find("juegoDeTazon(k).tablon, juegoDeTazon(k).vacio") != std::string::npos);
    CHECK(tazonDeTablon(BLOCK_PLANKS) == BLOCK_TAZON_PINO);
    CHECK(tazonDeTablon(BLOCK_PLANKS_ENCINO) == BLOCK_TAZON_ENCINO);
    CHECK(tazonDeTablon(BLOCK_PLANKS_OYAMEL) == BLOCK_TAZON_OYAMEL);
    // La receta vieja (palo, piedra, palo, tablon) ya no esta.
    CHECK(src.find("recipe.pattern[4] = TABLON[i];") == std::string::npos);
}

// PEDIDO: "arregla que cada tablones de madera tenga su propio tazon".
TEST_CASE("Tazones: cada madera tiene el suyo, con sus tres estados, su nombre y su imagen") {
    static const BlockType TABLONES[8] = { BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL, BLOCK_PLANKS_OCOTE,
                                           BLOCK_PLANKS_PATULA, BLOCK_PLANKS_PINONERO, BLOCK_PLANKS_ALTURAS,
                                           BLOCK_PLANKS_PSEUDOSTROBUS };
    REQUIRE(NUM_JUEGOS_DE_TAZON == 8);
    std::set<int> vistos;
    for (BlockType tablon : TABLONES) {
        INFO("tablon " << (int)tablon);
        const BlockType vacio = tazonDeTablon(tablon);
        REQUIRE(vacio != BLOCK_AIR);
        const BlockType agua = tazonLleno(vacio), miel = tazonConAguamiel(vacio);
        // Ocho maderas, ocho tazones distintos (y veinticuatro con los llenos).
        CHECK(vistos.insert((int)vacio).second);
        CHECK(vistos.insert((int)agua).second);
        CHECK(vistos.insert((int)miel).second);
        CHECK(esTazonVacio(vacio));
        CHECK(esTazonConAgua(agua));
        CHECK(esTazonConAguamiel(miel));
        CHECK_FALSE(esTazonConAgua(vacio));
        CHECK_FALSE(esTazonVacio(agua));
        // Se llena y se vacia sin cambiar de madera.
        CHECK(tazonVaciado(agua) == vacio);
        CHECK(tazonVaciado(miel) == BLOCK_AIR);            // el barro no gasta aguamiel
        CHECK(tazonVaciadoTodo(miel) == vacio);
        CHECK(tazonVaciadoTodo(agua) == vacio);
        CHECK(tazonConAguamiel(agua) == miel);
        CHECK(tazonLleno(agua) == BLOCK_AIR);
        for (BlockType t : { vacio, agua, miel }) {
            CHECK(esTazon(t));
            CHECK((int)t <= BLOCK_TYPE_MAX);
            const char* n = NombreItem::nombreDeItem(t);
            REQUIRE(n != nullptr);
            CHECK(std::strncmp(n, "TAZON DE ", 9) == 0);
        }
        CHECK(std::strstr(NombreItem::nombreDeItem(agua), "CON AGUA") != nullptr);
        CHECK(std::strstr(NombreItem::nombreDeItem(miel), "CON AGUAMIEL") != nullptr);
    }
    CHECK(vistos.size() == 24);
    // Lo que no es un tablon de arbol no da tazon.
    CHECK(tazonDeTablon(BLOCK_STONE) == BLOCK_AIR);
    CHECK(tazonDeTablon(BLOCK_WOOD) == BLOCK_AIR);
    CHECK_FALSE(esTazon(BLOCK_PLANKS));
    // Los nuevos van al final: los de siempre conservan su numero.
    CHECK((int)BLOCK_TAZON_OCOTE > (int)BLOCK_TAZON_OYAMEL_AGUAMIEL);
    CHECK((int)BLOCK_TAZON_PSEUDOSTROBUS_AGUAMIEL == (int)BLOCK_TYPE_MAX);

    // Cada uno con su imagen en disco, con nombre sin enie, y en el creativo.
    const std::string src = leerP("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    static const char* const IMAGENES[8][2] = {
        { "tazon de madera de pino.png", "Tazon de madera de pino con agua.png" },
        { "tazon de madera de encino.png", "Tazon de madera de encino con agua.png" },
        { "Tazon Madera de Oyame.png", "Tazon de madera de Oyame con agua.png" },
        { "Tazon de madera Pinus montezumae.png", "Tazon de madera Pinus montezumae con agua.png" },
        { "Tazon de Madera de Pino patula.png", "Tazon de Madera de Pino patula con agua.png" },
        { "Tazon Madera de Pino pinonero mexicano.png", "Tazon Madera de Pino pinonero mexicano con agua.png" },
        { "Tazon Madera de Pino de las alturas.png", "Tazon Madera de Pino de las alturas con agua.png" },
        { "Tazon madera de Pino de pseudostrobus.png", "Tazon madera de Pino de pseudostrobus con agua.png" },
    };
    for (const auto& par : IMAGENES) {
        std::string miel = par[1];
        miel.replace(miel.find(" con agua.png"), 13, " con aguamiel.png");
        for (const std::string& img : { std::string(par[0]), std::string(par[1]), miel }) {
            INFO(img);
            bool esta = false;
            for (const char* pre : { "", "../", "../../", "../../../", "../../../../" }) {
                std::ifstream f(std::string(pre) + "resourcepacks/Textures/Items/" + img, std::ios::binary);
                if (f.good()) { esta = true; break; }
            }
            CHECK(esta);
            CHECK(src.find("\"" + img + "\"") != std::string::npos);
            for (unsigned char c : img) CHECK(c < 128);
        }
    }
    for (const char* m : { "OCOTE", "PATULA", "PINONERO", "ALTURAS", "PSEUDOSTROBUS" })
        for (const char* suf : { "", "_AGUA", "_AGUAMIEL" })
            CHECK(src.find(std::string("ponerEnCreativo(BLOCK_TAZON_") + m + suf + ");") != std::string::npos);
    // El barro se amasa con el tazon de agua de cualquier madera.
    CHECK(src.find("const BlockType t = juegoDeTazon(k).agua;") != std::string::npos);
}
