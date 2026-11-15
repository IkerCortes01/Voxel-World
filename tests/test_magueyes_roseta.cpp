// EL AGAVE ALBOMARGINATA Y EL MAGUEY SALMIANA, y la tabla de especies que
// comparten la rama de dibujo del tequilana (Compuesto::AgaveRigido).
// PEDIDO: "anade la generacion y forma de dos magueys": el albomarginata
// (roseta de 70 cm a 1 m de alto y cerca de 1,5 m de diametro, 16 a 30 hojas en
// forma de espada, rigidas a arqueadas) y el salmiana (de 1,5 a casi 4 m de
// alto y mas de 3 m de ancho, pencas gruesas y carnosas).
#include "doctest/doctest.h"

#include "BlockType.h"
#include "BloqueCompuesto.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string raizM() {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    return r;
}
std::string motorM() {
    std::ifstream f(raizM() + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

namespace AG = Compuesto::AgaveAzul;
namespace AR = Compuesto::AgaveRigido;
using Compuesto::FAM_AGAVE_ALBOMARGINATA;
using Compuesto::FAM_AGAVE_AZUL;
using Compuesto::FAM_MAGUEY_ESPADIN;
using Compuesto::FAM_MAGUEY_SALMIANA;

TEST_CASE("Albomarginata: roseta baja y abierta, de 70 cm a 1 m de alto y 1,5 m de diametro") {
    const auto F = FAM_AGAVE_ALBOMARGINATA;
    CHECK(AR::alturaReal(F, AG::HECHA) >= 0.70f);
    CHECK(AR::alturaReal(F, AG::MADURA) >= 0.70f);
    CHECK(AR::alturaReal(F, AG::MADURA) <= 1.00f);
    const float diametro = 2.0f * AR::radioDeEtapa(F, AG::MADURA);
    CHECK(diametro >= 1.35f);
    CHECK(diametro <= 1.65f);
    // Mas ancha que alta.
    CHECK(diametro > AR::alturaReal(F, AG::MADURA));
    // De 16 a 30 hojas las que ya son planta.
    for (uint16_t e = AG::MEDIA; e <= AG::MADURA; ++e) {
        CHECK(AR::hojasDeEtapa(F, e) >= 16);
        CHECK(AR::hojasDeEtapa(F, e) <= 30);
    }
    // Hoja mas fina que la del tequilana y algo arqueada ("rigidas a arqueadas").
    CHECK(AR::formaDe(F).grueso < 1.0f);
    CHECK(AR::formaDe(F).caida > 0.0f);
    // Cabe en una celda.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) CHECK(AR::celdasDeEtapa(F, e) == 1);
}

TEST_CASE("Salmiana: el gigante, de 1,5 a casi 4 m de alto y unos 3 m de ancho, con pencas gruesas") {
    const auto F = FAM_MAGUEY_SALMIANA;
    CHECK(AR::alturaReal(F, AG::MEDIA) >= 1.5f);
    CHECK(AR::alturaReal(F, AG::HECHA) >= 2.5f);
    CHECK(AR::alturaReal(F, AG::MADURA) >= 3.0f);
    CHECK(AR::alturaReal(F, AG::MADURA) < 4.0f);
    CHECK(2.0f * AR::radioDeEtapa(F, AG::MADURA) >= 2.8f);
    // Nunca cruza el borde de su chunk.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) CHECK(AR::radioDeEtapa(F, e) <= AR::RADIO_MAX);
    // Mucho mas carnosa que ninguna, y se vence por su peso; menos hojas, enormes.
    for (auto otra : { FAM_AGAVE_AZUL, FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA }) {
        CHECK(AR::formaDe(F).grueso > AR::formaDe(otra).grueso * 1.5f);
        CHECK(AR::alturaReal(F, AG::MADURA) > AR::alturaReal(otra, AG::MADURA));
        CHECK(AR::hojasDeEtapa(F, AG::MADURA) < AR::hojasDeEtapa(otra, AG::MADURA));
    }
    CHECK(AR::formaDe(F).caida > 0.15f);
    // Las celdas que llena: hasta 4 la madura, y nunca menos de lo que mide.
    CHECK(AR::celdasDeEtapa(F, AG::MADURA) == 4);
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        CHECK((float)AR::celdasDeEtapa(F, e) >= AR::alturaReal(F, e) - 0.06f);
        CHECK((float)AR::celdasDeEtapa(F, e) < AR::alturaReal(F, e) + 1.0f);
    }
}

TEST_CASE("Agaves de roseta: cada especie es su familia y el tequilana no cambia") {
    const Compuesto::Familia TODAS[4] = { FAM_AGAVE_AZUL, FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA,
                                          FAM_MAGUEY_SALMIANA };
    for (auto f : TODAS) {
        CHECK(AR::esRigido(f));
        CHECK(Compuesto::familiaTieneModelo(f));
        for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
            const BlockType t = AR::nuevo(f, e, 5);
            CHECK(Compuesto::familiaDe(t) == f);
            CHECK(AG::etapaDe(t) == e);
            CHECK(AG::giroDe(t) == 5);
            CHECK(AG::faseDe(t) == AG::ROSETA);
            CHECK(Compuesto::componentesDe(t) == 2);
            // Toda la columna es de la misma planta, con segmentos seguidos.
            for (int s = 0; s < AR::celdasDe(t); ++s) {
                const BlockType c = AR::conSegmento(t, (uint16_t)s);
                CHECK(Compuesto::familiaDe(c) == f);
                CHECK((int)AG::segmentoDe(c) == s);
                CHECK(AG::etapaDe(c) == e);
            }
        }
    }
    CHECK_FALSE(AR::esRigido(Compuesto::FAM_MAGUEY));
    CHECK_FALSE(AR::esRigido(Compuesto::FAM_NOPAL));
    // El tequilana, exactamente como estaba.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        CHECK(AR::alturaReal(FAM_AGAVE_AZUL, e) == AG::alturaReal(e));
        CHECK(AR::hojasDeEtapa(FAM_AGAVE_AZUL, e) == AG::pencasDeEtapa(e));
        CHECK(AR::celdasDeEtapa(FAM_AGAVE_AZUL, e) == AG::celdasDeEtapa(e));
        CHECK(AR::nuevo(FAM_AGAVE_AZUL, e, 3) == AG::nuevo(e, 3));
    }
    CHECK(AR::formaDe(FAM_AGAVE_AZUL).grueso == 1.0f);
    CHECK(AR::formaDe(FAM_AGAVE_AZUL).caida == 0.0f);
    // Las familias nuevas, al final: ninguna de antes cambia de numero.
    CHECK((int)FAM_AGAVE_ALBOMARGINATA == (int)FAM_MAGUEY_ESPADIN + 1);
    CHECK((int)FAM_MAGUEY_SALMIANA < (int)Compuesto::FAM_COUNT);
}

TEST_CASE("Agaves de roseta: nombres, texturas en disco y etiquetas al final del enum") {
    CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(FAM_AGAVE_ALBOMARGINATA, AG::HECHA, 0)),
                      "AGAVE ALBOMARGINATA") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(FAM_MAGUEY_SALMIANA, AG::HECHA, 0)),
                      "MAGUEY SALMIANA") == 0);
    CHECK((int)BLOCK_AGAVE_ALBOMARGINATA_PENCA == (int)BLOCK_MAGUEY_ESPADIN_PUNTA + 1);
    CHECK((int)BLOCK_MAGUEY_SALMIANA_PENCA <= (int)BLOCK_TYPE_MAX);
    for (BlockType t : { BLOCK_AGAVE_ALBOMARGINATA_PENCA, BLOCK_MAGUEY_SALMIANA_PENCA }) {
        CHECK(fueraDelCreativo(t));
        CHECK_FALSE(esRocaParaPico(t));
    }
    CHECK(AR::formaDe(FAM_AGAVE_ALBOMARGINATA).penca == BLOCK_AGAVE_ALBOMARGINATA_PENCA);
    CHECK(AR::formaDe(FAM_MAGUEY_SALMIANA).penca == BLOCK_MAGUEY_SALMIANA_PENCA);
    for (const char* t : { "Agave albomarginata penca.png", "Maguey de salmiana.png" }) {
        std::ifstream f(raizM() + "resourcepacks/Textures/Blocks/" + t, std::ios::binary);
        INFO("textura: " << t);
        CHECK(f.good());
    }
    const std::string src = motorM();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("getTexture(\"Agave albomarginata penca.png\")") != std::string::npos);
    CHECK(src.find("getTexture(\"Maguey de salmiana.png\")") != std::string::npos);
}

TEST_CASE("Agaves de roseta: el motor los dibuja con una sola rama y los siembra donde toca") {
    const std::string src = motorM();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("Compuesto::AgaveRigido::esRigido(Compuesto::familiaDe(block))) {") != std::string::npos);
    CHECK(src.find("texSegura(FORMA.penca, 0)") != std::string::npos);
    CHECK(src.find("A::engrosado(etapa) * FORMA.grueso") != std::string::npos);
    CHECK(src.find("A::caidaDeEtapa(etapa) + FORMA.caida") != std::string::npos);
    CHECK(src.find("} else if (Compuesto::AgaveRigido::esRigido(fam)) {") != std::string::npos);   // dan pencas
    CHECK(src.find("if (Compuesto::AgaveRigido::esRigido(Compuesto::familiaDe(b)))") != std::string::npos);  // caen enteros
    const size_t a = src.find("// ---- AGAVE ALBOMARGINATA Y MAGUEY SALMIANA ----");
    REQUIRE(a != std::string::npos);
    const size_t b = src.find("// ---- AGAVE TEQUILANA AZUL ----", a);
    REQUIRE(b != std::string::npos);
    const std::string siembra = src.substr(a, b - a);
    CHECK(siembra.find("Compuesto::FAM_AGAVE_ALBOMARGINATA, 20, 3u") != std::string::npos);
    CHECK(siembra.find("Compuesto::FAM_MAGUEY_SALMIANA,     28, 3u") != std::string::npos);
    CHECK(siembra.find("BIOME_MOUNTAINS") != std::string::npos);     // el albomarginata, en las faldas
    CHECK(siembra.find("BIOME_PLAINS") != std::string::npos);        // el salmiana, en el altiplano
    CHECK(siembra.find("if (!cabe) continue;") != std::string::npos);   // entero o nada
    CHECK(siembra.find("if (!libre) continue;") != std::string::npos);  // con sitio alrededor
}

// ⭐ CINCO MAS. PEDIDO: "anade estos nuevos tipos de maguey: Agave potatorum,
// Agave karwinskii, Madrecuixe, Cuishe, Cirial".
using Compuesto::FAM_AGAVE_KARWINSKII;
using Compuesto::FAM_AGAVE_POTATORUM;
using Compuesto::FAM_MAGUEY_CIRIAL;
using Compuesto::FAM_MAGUEY_CUISHE;
using Compuesto::FAM_MAGUEY_MADRECUIXE;

TEST_CASE("Potatorum: roseta pequena y densa, de 30 a 80 cm de alto y 60 a 120 de diametro") {
    const auto F = FAM_AGAVE_POTATORUM;
    for (uint16_t e = AG::MEDIA; e <= AG::MADURA; ++e) {
        CHECK(AR::alturaReal(F, e) >= 0.30f);
        CHECK(AR::alturaReal(F, e) <= 0.80f);
        CHECK(2.0f * AR::radioDeEtapa(F, e) >= 0.60f);
        CHECK(2.0f * AR::radioDeEtapa(F, e) <= 1.20f);
    }
    CHECK(AR::formaDe(F).tronco == 0.0f);                  // nace a ras de suelo
    CHECK(AR::formaDe(F).grueso > 1.3f);                   // hojas gruesas
    CHECK(AR::hojasDeEtapa(F, AG::MADURA) < AG::pencasDeEtapa(AG::MADURA));   // pocas y anchas
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) CHECK(AR::celdasDeEtapa(F, e) == 1);
}

TEST_CASE("Los de tronco: la roseta va subida en su tallo, y cada uno tiene el suyo") {
    const Compuesto::Familia T[4] = { FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE, FAM_MAGUEY_CUISHE,
                                      FAM_MAGUEY_CIRIAL };
    for (auto f : T) {
        CHECK(AR::esRigido(f));
        CHECK(Compuesto::familiaTieneModelo(f));
        CHECK(AR::formaDe(f).tronco >= 1.0f);              // un tallo de verdad
        CHECK(AR::formaDe(f).radioTronco > 0.08f);
        CHECK(AR::formaDe(f).radioTronco < 0.30f);
        CHECK(AR::formaDe(f).caida == 0.0f);               // hojas rectas
        // El tronco crece con la planta y la roseta es pequena.
        CHECK(AR::troncoDeEtapa(f, AG::MADURA) == doctest::Approx(AR::formaDe(f).tronco));
        CHECK(AR::troncoDeEtapa(f, AG::HIJUELO) < 0.5f * AR::formaDe(f).tronco);
        CHECK(AR::alturaReal(f, AG::MADURA) < 1.0f);
        CHECK(AR::alturaTotal(f, AG::MADURA) > 1.7f);
        CHECK(AR::alturaTotal(f, AG::MADURA) < 3.0f);
        // Ocupa las celdas de todo lo que mide, tronco incluido.
        CHECK(AR::celdasDeEtapa(f, AG::MADURA) >= 2);
        CHECK((float)AR::celdasDeEtapa(f, AG::MADURA) >= AR::alturaTotal(f, AG::MADURA) - 0.06f);
        // Mas tronco que roseta: es lo que los distingue.
        CHECK(AR::formaDe(f).tronco > AR::alturaReal(f, AG::MADURA));
    }
    // El madrecuixe, el de tallo mas grueso y hojas mas gruesas y numerosas.
    for (auto f : { FAM_AGAVE_KARWINSKII, FAM_MAGUEY_CUISHE, FAM_MAGUEY_CIRIAL }) {
        CHECK(AR::formaDe(FAM_MAGUEY_MADRECUIXE).radioTronco > AR::formaDe(f).radioTronco);
        CHECK(AR::formaDe(FAM_MAGUEY_MADRECUIXE).grueso > AR::formaDe(f).grueso);
        CHECK(AR::hojasDeEtapa(FAM_MAGUEY_MADRECUIXE, AG::MADURA) > AR::hojasDeEtapa(f, AG::MADURA));
    }
    // El cirial, el mas alto y el mas fino; el cuishe, alto y delgado.
    for (auto f : { FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE, FAM_MAGUEY_CUISHE }) {
        CHECK(AR::alturaTotal(FAM_MAGUEY_CIRIAL, AG::MADURA) > AR::alturaTotal(f, AG::MADURA));
        CHECK(AR::formaDe(FAM_MAGUEY_CIRIAL).radioTronco < AR::formaDe(f).radioTronco);
    }
    CHECK(AR::formaDe(FAM_MAGUEY_CUISHE).tronco > AR::formaDe(FAM_AGAVE_KARWINSKII).tronco);
    // Los de antes siguen sin tronco.
    for (auto f : { FAM_AGAVE_AZUL, FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA, FAM_MAGUEY_SALMIANA })
        CHECK(AR::alturaTotal(f, AG::MADURA) == AR::alturaReal(f, AG::MADURA));
}

TEST_CASE("Cinco magueyes: nombre, textura en disco, etiqueta y siembra") {
    struct E { Compuesto::Familia f; BlockType penca; const char* nombre; const char* archivo; };
    const E ES[5] = {
        { FAM_AGAVE_POTATORUM, BLOCK_AGAVE_POTATORUM_PENCA, "AGAVE POTATORUM", "Agave potatorum.png" },
        { FAM_AGAVE_KARWINSKII, BLOCK_AGAVE_KARWINSKII_PENCA, "AGAVE KARWINSKII", "Agave karwinskii.png" },
        { FAM_MAGUEY_MADRECUIXE, BLOCK_MAGUEY_MADRECUIXE_PENCA, "MADRECUIXE", "Agave Madrecuixe.png" },
        { FAM_MAGUEY_CUISHE, BLOCK_MAGUEY_CUISHE_PENCA, "CUISHE", "Agave Cuishe.png" },
        { FAM_MAGUEY_CIRIAL, BLOCK_MAGUEY_CIRIAL_PENCA, "CIRIAL", "Agave Cirial.png" },
    };
    const std::string src = motorM();
    REQUIRE_FALSE(src.empty());
    for (const E& e : ES) {
        INFO(e.nombre);
        CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(e.f, AG::HECHA, 0)), e.nombre) == 0);
        CHECK(AR::formaDe(e.f).penca == e.penca);
        CHECK((int)e.penca <= (int)BLOCK_TYPE_MAX);
        CHECK(fueraDelCreativo(e.penca));
        CHECK_FALSE(esRocaParaPico(e.penca));
        std::ifstream f(raizM() + "resourcepacks/Textures/Blocks/" + e.archivo, std::ios::binary);
        CHECK(f.good());
        CHECK(src.find(std::string("getTexture(\"") + e.archivo + "\")") != std::string::npos);
    }
    CHECK((int)BLOCK_MAGUEY_CIRIAL_PENCA == (int)BLOCK_TYPE_MAX);
    CHECK((int)FAM_MAGUEY_CIRIAL == (int)Compuesto::FAM_COUNT - 1);
    // Cada una tiene su fila de siembra, y el tronco se dibuja.
    for (const char* fila : { "Compuesto::FAM_AGAVE_POTATORUM,     18", "Compuesto::FAM_AGAVE_KARWINSKII,    26",
                              "Compuesto::FAM_MAGUEY_MADRECUIXE,   30", "Compuesto::FAM_MAGUEY_CUISHE,       30",
                              "Compuesto::FAM_MAGUEY_CIRIAL,       34" })
        CHECK(src.find(fila) != std::string::npos);
    CHECK(src.find("const float TRONCO = Compuesto::AgaveRigido::troncoDeEtapa(ESPECIE, etapa);") != std::string::npos);
    CHECK(src.find("- POSADA + ELEVADO + TRONCO;") != std::string::npos);
    CHECK(src.find("if (TRONCO > 0.02f) {") != std::string::npos);
}

// PEDIDO: "que los magueyes tengan una sola caja de casilla". El selector
// junta las celdas de una mata por su segmento: tiene que leerse igual en
// todas las especies.
TEST_CASE("Magueyes: el segmento de la celda se lee en todas las especies") {
    using namespace Compuesto;
    for (Familia f : { FAM_AGAVE_AZUL, FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA, FAM_MAGUEY_SALMIANA,
                       FAM_AGAVE_POTATORUM, FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE,
                       FAM_MAGUEY_CUISHE, FAM_MAGUEY_CIRIAL }) {
        const BlockType base = AgaveRigido::nuevo(f, AgaveAzul::MADURA, 0);
        CHECK(segmentoDeMaguey(base) == 0);
        for (uint16_t s = 1; s < 4; ++s)
            CHECK(segmentoDeMaguey(AgaveRigido::conSegmento(base, s)) == (int)s);
    }
    const BlockType abisaii = AgaveAbisaii::nuevo(AgaveAbisaii::HECHA, 0);
    CHECK(segmentoDeMaguey(abisaii) == 0);
    CHECK(segmentoDeMaguey(AgaveAbisaii::conSegmento(abisaii, 1)) == 1);
    // Lo que no es un maguey no tiene segmento.
    CHECK(segmentoDeMaguey(BLOCK_STONE) == -1);
    CHECK(segmentoDeMaguey(BLOCK_AIR) == -1);
}
