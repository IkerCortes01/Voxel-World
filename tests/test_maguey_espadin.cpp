// EL MAGUEY ESPADIN (Agave angustifolia), el mas comun.
// PEDIDO: "anade un maguey, el mas comun sera el Maguey Espadin: pencas largas,
// estrechas y terminadas en punta, como una espada; entre 1,2 y 1,5 metros de
// altura (hasta 2 en condiciones optimas); verde azulado o grisaceo, con
// espinas pequenas en los bordes y una espina terminal fuerte".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "BloqueCompuesto.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string raiz() {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    return r;
}
std::string motor() {
    std::ifstream f(raiz() + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

namespace AG = Compuesto::AgaveAzul;
namespace ES = Compuesto::MagueyEspadin;

TEST_CASE("Espadin: mide lo que mide un espadin, y es mas estrecho que el tequilana") {
    // De 1,2 a 1,5 m las hechas; hasta 2 m "en condiciones optimas" no se pasa.
    CHECK(ES::alturaReal(AG::HECHA) >= 1.2f);
    CHECK(ES::alturaReal(AG::HECHA) <= 1.5f);
    CHECK(ES::alturaReal(AG::MADURA) >= 1.2f);
    CHECK(ES::alturaReal(AG::MADURA) <= 2.0f);
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        CHECK(ES::alturaReal(e) < AG::alturaReal(e));            // mas bajo
        CHECK(ES::radioDeEtapa(e) < AG::radioDeEtapa(e));        // y mas estrecho
        // Hoja de espada: la roseta es mas alta que su radio (la del tequilana
        // es mas ancha que alta).
        CHECK(ES::radioDeEtapa(e) < ES::alturaReal(e));
        // Ocupa las celdas que de verdad llena.
        CHECK(ES::celdasDeEtapa(e) == (ES::alturaReal(e) > 1.0f ? 2 : 1));
        if (e > AG::HIJUELO) CHECK(ES::alturaReal(e) > ES::alturaReal((uint16_t)(e - 1)));
    }
    CHECK(ES::radioDeEtapa(AG::MADURA) < 1.0f);                  // cabe holgado en su chunk
}

TEST_CASE("Espadin: es su propia familia, con el estado del tequilana") {
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e)
        for (uint16_t g = 0; g < 8; ++g) {
            const BlockType t = ES::nuevo(e, g);
            CHECK(Compuesto::esCompuesto(t));
            CHECK(ES::esEspadin(t));
            CHECK_FALSE(AG::esAgaveAzul(t));
            CHECK(Compuesto::familiaDe(t) == Compuesto::FAM_MAGUEY_ESPADIN);
            CHECK(AG::etapaDe(t) == e);                          // los mismos campos
            CHECK(AG::giroDe(t) == g);
            CHECK(AG::faseDe(t) == AG::ROSETA);                  // siempre en roseta
            CHECK(AG::segmentoDe(t) == 0);
            const BlockType arriba = ES::conSegmento(t, 1);
            CHECK(ES::esEspadin(arriba));
            CHECK(AG::segmentoDe(arriba) == 1);
            CHECK(AG::etapaDe(arriba) == e);
            CHECK(arriba != t);
        }
    CHECK_FALSE(ES::esEspadin(AG::nuevo(AG::HECHA, 0)));
    CHECK_FALSE(ES::esEspadin(BLOCK_STONE));
    // Sabe dibujarse (si no, no se dibujaria nada) y tiene cuerpo y puntas.
    CHECK(Compuesto::familiaTieneModelo(Compuesto::FAM_MAGUEY_ESPADIN));
    CHECK(Compuesto::componentesDe(ES::nuevo(AG::HECHA, 0)) == 2);
    // Va al final: ninguna familia de antes cambia de numero.
    CHECK((int)Compuesto::FAM_MAGUEY_ESPADIN < (int)Compuesto::FAM_COUNT);
    CHECK((int)Compuesto::FAM_MAGUEY_ESPADIN > (int)Compuesto::FAM_PLANTA_EN_AGUA);
}

TEST_CASE("Espadin: sus piezas de textura van al final del enum y no se pican ni salen en creativo") {
    CHECK((int)BLOCK_MAGUEY_ESPADIN_PENCA == (int)BLOCK_PENCA_NOPAL_PUESTA + 1);
    CHECK((int)BLOCK_MAGUEY_ESPADIN_PUNTA == (int)BLOCK_MAGUEY_ESPADIN_PENCA + 1);
    CHECK((int)BLOCK_MAGUEY_ESPADIN_PUNTA <= (int)BLOCK_TYPE_MAX);
    CHECK(fueraDelCreativo(BLOCK_MAGUEY_ESPADIN_PENCA));
    CHECK(fueraDelCreativo(BLOCK_MAGUEY_ESPADIN_PUNTA));
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_ESPADIN_PENCA));
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_ESPADIN_PUNTA));
    CHECK(std::strcmp(NombreItem::nombreDeItem(ES::nuevo(AG::HECHA, 0)), "MAGUEY ESPADIN") == 0);
}

TEST_CASE("Espadin: las texturas estan en disco, con nombre sin acentos") {
    // (La espina no tiene archivo propio: sale de la misma foto de la hoja.)
    for (const char* t : { "Maguey Espadin.png" }) {
        std::ifstream f(raiz() + "resourcepacks/Textures/Blocks/" + t, std::ios::binary);
        INFO("textura: " << t);
        CHECK(f.good());
    }
    const std::string src = motor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("getTexture(\"Maguey Espadin.png\")") != std::string::npos);
    CHECK(src.find("Maguey Espadin punta.png") == std::string::npos);
}

TEST_CASE("Espadin: el motor lo dibuja, lo siembra por todas partes, lo tala entero y da pencas") {
    const std::string src = motor();
    REQUIRE_FALSE(src.empty());
    // Se dibuja con la rama del tequilana, con sus medidas y sus texturas.
    // (Despues entraron el albomarginata y el salmiana y la rama dejo de
    // preguntar por el espadin: cada especie trae su forma de una tabla.)
    CHECK(src.find("Compuesto::AgaveRigido::esRigido(Compuesto::familiaDe(block))) {") != std::string::npos);
    CHECK(src.find("Compuesto::AgaveRigido::alturaReal(ESPECIE, etapa)") != std::string::npos);
    CHECK(Compuesto::AgaveRigido::formaDe(Compuesto::FAM_MAGUEY_ESPADIN).penca == BLOCK_MAGUEY_ESPADIN_PENCA);
    CHECK(Compuesto::AgaveRigido::alturaReal(Compuesto::FAM_MAGUEY_ESPADIN, AG::HECHA) ==
          doctest::Approx(ES::alturaReal(AG::HECHA)));
    // La siembra: sin pedir bioma, y mas focos que ninguna otra especie.
    const size_t a = src.find("// ---- MAGUEY ESPADIN ----");
    REQUIRE(a != std::string::npos);
    const size_t b = src.find("// ---- AGAVE ALBOMARGINATA Y MAGUEY SALMIANA ----", a);
    REQUIRE(b != std::string::npos);
    const std::string siembra = src.substr(a, b - a);
    CHECK(siembra.find("col.biome") == std::string::npos);
    CHECK(siembra.find("if ((hf & 1u) == 0u) {") != std::string::npos);      // 1 de cada 2 focos
    CHECK(siembra.find("ES::conSegmento(planta, (uint16_t)dy)") != std::string::npos);   // la planta entera
    CHECK(siembra.find("!= BLOCK_AIR) break;") != std::string::npos);        // no pisa lo que haya
    // Al talarlo cae entero y da pencas de maguey.
    CHECK(src.find("} else if (Compuesto::AgaveRigido::esRigido(fam)) {") != std::string::npos);
    CHECK(src.find("if (Compuesto::AgaveRigido::esRigido(Compuesto::familiaDe(b)))") != std::string::npos);
}
