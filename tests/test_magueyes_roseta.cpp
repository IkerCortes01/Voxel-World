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
#include <vector>

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
    CHECK((int)BLOCK_MAGUEY_CIRIAL_PENCA < (int)BLOCK_TYPE_MAX);
    CHECK((int)FAM_MAGUEY_CIRIAL < (int)Compuesto::FAM_COUNT);
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

// PEDIDO: "haz que cada maguey tenga su item, fisicas de item suelto y
// colocado, y diferentes tamanos de maguey colocado".
TEST_CASE("Magueyes: cada especie da SU penca, con nombre y tamano propios") {
    using namespace Compuesto;
    const Familia FAMS[] = { FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA, FAM_MAGUEY_SALMIANA,
                             FAM_AGAVE_POTATORUM, FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE,
                             FAM_MAGUEY_CUISHE, FAM_MAGUEY_CIRIAL };
    std::vector<BlockType> vistas;
    for (Familia f : FAMS) {
        const BlockType penca = AgaveRigido::formaDe(f).penca;
        INFO("familia " << (int)f);
        CHECK(esPencaDeEspecie(penca));
        CHECK(esPencaDeMaguey(penca));          // se dibuja carnosa, como las demas
        CHECK(seTiraAlColocar(penca));          // al "colocarla" se deja en el suelo, con fisica
        CHECK(penca != BLOCK_IXTLE_HOJA);       // ya no es la del pulquero
        for (BlockType v : vistas) CHECK(v != penca);       // ninguna repetida
        vistas.push_back(penca);
        const std::string nombre = NombreItem::nombreDeItem(penca);
        CHECK(nombre.find("PENCA") != std::string::npos);
        CHECK(tamanoDePenca(penca) > 0.5f);
        CHECK(tamanoDePenca(penca) < 2.5f);
    }
    CHECK(esPencaDeEspecie(BLOCK_AGAVE_ABISAII_PENCA));
    CHECK(seTiraAlColocar(BLOCK_AGAVE_ABISAII_PENCA));
    // Las de siempre siguen como estaban.
    CHECK(seTiraAlColocar(BLOCK_IXTLE_HOJA));
    CHECK(seTiraAlColocar(BLOCK_PENCA_AGAVE_AZUL));
    CHECK(seTiraAlColocar(BLOCK_NOPAL_CLADODIO));
    CHECK_FALSE(esPencaDeEspecie(BLOCK_IXTLE_HOJA));
    CHECK_FALSE(esPencaDeEspecie(BLOCK_MAGUEY_ESPADIN_PUNTA));
    CHECK_FALSE(seTiraAlColocar(BLOCK_STONE));
    // De la mas grande a la mas chica: el salmiana manda y el potatorum cierra.
    CHECK(tamanoDePenca(BLOCK_MAGUEY_SALMIANA_PENCA) > tamanoDePenca(BLOCK_IXTLE_HOJA));
    CHECK(tamanoDePenca(BLOCK_IXTLE_HOJA) > tamanoDePenca(BLOCK_MAGUEY_ESPADIN_PENCA));
    CHECK(tamanoDePenca(BLOCK_MAGUEY_ESPADIN_PENCA) > tamanoDePenca(BLOCK_AGAVE_POTATORUM_PENCA));
    CHECK(tamanoDePenca(BLOCK_MAGUEY_SALMIANA_PENCA) > 2.0f * tamanoDePenca(BLOCK_AGAVE_POTATORUM_PENCA));
    CHECK(tamanoDePenca(BLOCK_STONE) == 1.0f);
}

TEST_CASE("Magueyes: dos pencas dejadas de la misma especie no miden lo mismo") {
    const BlockType p = BLOCK_MAGUEY_ESPADIN_PENCA;
    const float base = tamanoDePenca(p);
    float menor = 99.0f, mayor = 0.0f;
    for (int g = 0; g < 360; ++g) {
        const float t = tamanoDeItemDejado(p, (float)g);
        CHECK(t >= base * 0.85f - 1e-4f);
        CHECK(t <= base * 1.15f + 1e-4f);
        if (t < menor) menor = t;
        if (t > mayor) mayor = t;
        // El mismo giro, el mismo tamano: no cambia al guardar y volver.
        CHECK(tamanoDeItemDejado(p, (float)g) == t);
    }
    CHECK(mayor - menor > base * 0.25f);
    // La que mide 1 de base tambien varia; lo que no es penca, no.
    CHECK(tamanoDeItemDejado(BLOCK_AGAVE_ALBOMARGINATA_PENCA, 37.0f) != 1.0f);
    CHECK(tamanoDeItemDejado(BLOCK_NOPAL_CLADODIO, 37.0f) == 1.0f);
    CHECK(tamanoDeItemDejado(BLOCK_STONE, 123.0f) == 1.0f);
}

TEST_CASE("Magueyes: el motor suelta la penca de cada especie y la dibuja de su tamano") {
    const std::string src = [] {
        std::ifstream f(raizM() + "src/main.cpp", std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("const BlockType suPenca = Compuesto::AgaveRigido::formaDe(fam).penca;") != std::string::npos);
    CHECK(src.find("state->spawnItem(pos, suPenca);") != std::string::npos);
    CHECK(src.find("state->spawnItem(pos, BLOCK_AGAVE_ABISAII_PENCA);") != std::string::npos);
    CHECK(src.find("const float e = tamanoDeItemDejado(item.blockType, item.giro);") != std::string::npos);
    for (const char* png : { "Maguey Espadin.png", "Agave albomarginata penca.png", "Maguey de salmiana.png",
                             "Agave potatorum.png", "Agave karwinskii.png", "Agave Madrecuixe.png",
                             "Agave Cuishe.png", "Agave Cirial.png", "Agave abisaii penca.png" }) {
        INFO(png);
        CHECK(src.find(std::string("siluetaPNG = \"Textures/Blocks/") + png + "\"; break;") != std::string::npos);
    }
}

// PEDIDO: "anade este maguey: Maguey Sisal" (hojas gruesas de 1,5 a 2 m, verde
// glauco, borde casi liso y punta marron; erguido y ordenado, tan alto como
// largas sus hojas, en abanico circular).
TEST_CASE("Sisal: es un maguey mas, erguido, con su hoja, su nombre y su siembra") {
    using namespace Compuesto;
    namespace AR = AgaveRigido;
    namespace AG = AgaveAzul;
    CHECK(AR::esRigido(FAM_MAGUEY_SISAL));
    CHECK((int)FAM_MAGUEY_SISAL < (int)FAM_COUNT);
    const AR::Forma f = AR::formaDe(FAM_MAGUEY_SISAL);
    CHECK(f.penca == BLOCK_MAGUEY_SISAL_PENCA);
    CHECK(f.caida == 0.0f);                 // hojas rectas: crece erguido
    CHECK(f.grueso > 1.0f);                 // gruesas y carnosas
    CHECK(f.tronco == 0.0f);                // roseta a ras de suelo
    // Maduro: entre 1,5 y 2 m de alto.
    const float alto = AR::alturaReal(FAM_MAGUEY_SISAL, AG::MADURA);
    CHECK(alto >= 1.5f);
    CHECK(alto <= 2.0f);
    CHECK(AR::celdasDe(AR::nuevo(FAM_MAGUEY_SISAL, AG::MADURA, 0)) == 2);
    CHECK(puedeDibujarse(AR::nuevo(FAM_MAGUEY_SISAL, AG::MADURA, 0)));
    CHECK(segmentoDeMaguey(AR::conSegmento(AR::nuevo(FAM_MAGUEY_SISAL, AG::MADURA, 0), 1)) == 1);
    CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(FAM_MAGUEY_SISAL, AG::HECHA, 0)), "MAGUEY SISAL") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_MAGUEY_SISAL_PENCA), "PENCA DE MAGUEY SISAL") == 0);
    // Su penca es un item como las demas, y de las grandes.
    CHECK(esPencaDeEspecie(BLOCK_MAGUEY_SISAL_PENCA));
    CHECK(seTiraAlColocar(BLOCK_MAGUEY_SISAL_PENCA));
    CHECK(tamanoDePenca(BLOCK_MAGUEY_SISAL_PENCA) > tamanoDePenca(BLOCK_MAGUEY_ESPADIN_PENCA));
    // La textura esta en disco y el motor la pide por su nombre.
    std::ifstream t(raizM() + "resourcepacks/Textures/Blocks/maguey sisal.png", std::ios::binary);
    CHECK(t.good());
    std::ifstream fsrc(raizM() + "src/main.cpp", std::ios::binary);
    const std::string src((std::istreambuf_iterator<char>(fsrc)), std::istreambuf_iterator<char>());
    CHECK(src.find("case BLOCK_MAGUEY_SISAL_PENCA:      return getTexture(\"maguey sisal.png\");") != std::string::npos);
    CHECK(src.find("{ Compuesto::FAM_MAGUEY_SISAL,        24, 3u, 0x5be0cd19u") != std::string::npos);
}

// PEDIDO: "haz que la hitbox de casilla de los magueyes este exactamente donde
// esta el modelo 3D".
TEST_CASE("Magueyes: la caja que se marca mide lo que mide el modelo") {
    using namespace Compuesto;
    namespace AR = AgaveRigido;
    namespace AG = AgaveAzul;
    for (Familia f : { FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA, FAM_MAGUEY_SALMIANA, FAM_AGAVE_POTATORUM,
                       FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE, FAM_MAGUEY_CUISHE, FAM_MAGUEY_CIRIAL,
                       FAM_MAGUEY_SISAL, FAM_AGAVE_FOURCROYDES, FAM_AGAVE_AMERICANA })
        for (uint16_t e : { (uint16_t)AG::HIJUELO, (uint16_t)AG::JOVEN, (uint16_t)AG::MEDIA, (uint16_t)AG::HECHA,
                            (uint16_t)AG::MADURA }) {
            INFO("familia " << (int)f << " etapa " << e);
            const BlockType t = AR::nuevo(f, e, 0);
            const CajaDeModelo m = cajaDelModeloDeMaguey(t);
            REQUIRE(m.vale);
            // Las mismas cuentas que el dibujo.
            CHECK(m.radio == AR::radioDeEtapa(f, e));
            CHECK(m.alto == AR::alturaTotal(f, e));
            // Y da igual a que celda de la planta se apunte: la caja es la misma.
            for (int s = 1; s < AR::celdasDe(t); ++s) {
                const CajaDeModelo arriba = cajaDelModeloDeMaguey(AR::conSegmento(t, (uint16_t)s));
                CHECK(arriba.radio == m.radio);
                CHECK(arriba.alto == m.alto);
            }
        }
    // El tequilana usa su propio radio, el que usa su dibujo.
    const CajaDeModelo azul = cajaDelModeloDeMaguey(AR::nuevo(FAM_AGAVE_AZUL, AG::MADURA, 0));
    CHECK(azul.radio == AG::radioDeEtapa(AG::MADURA));
    // Un salmiana hecho se sale de su celda por los lados; un potatorum chico cabe de sobra.
    CHECK(cajaDelModeloDeMaguey(AR::nuevo(FAM_MAGUEY_SALMIANA, AG::MADURA, 0)).radio > 1.0f);
    CHECK(cajaDelModeloDeMaguey(AR::nuevo(FAM_AGAVE_POTATORUM, AG::HIJUELO, 0)).radio < 0.5f);
    // El pulquero y el abisaii, igual.
    const BlockType pulquero = Maguey::nuevo(Maguey::ADULTO, 0);
    const CajaDeModelo mp = cajaDelModeloDeMaguey(pulquero);
    REQUIRE(mp.vale);
    CHECK(mp.radio == Maguey::radioDeCeldas(Maguey::celdasDeEtapa(Maguey::ADULTO)));
    CHECK(mp.alto == Maguey::alturaReal(Maguey::celdasDeEtapa(Maguey::ADULTO)));
    const CajaDeModelo ab = cajaDelModeloDeMaguey(AgaveAbisaii::nuevo(AgaveAbisaii::HECHA, 0));
    REQUIRE(ab.vale);
    CHECK(ab.alto == AgaveAbisaii::alturaReal(AgaveAbisaii::HECHA));
    // Lo que no es un maguey no tiene caja de modelo.
    CHECK_FALSE(cajaDelModeloDeMaguey(BLOCK_STONE).vale);
    CHECK_FALSE(cajaDelModeloDeMaguey(Biznaga::nuevo(0, 0, 0)).vale);
    CHECK_FALSE(cajaDelModeloDeMaguey(Flor::crear(0, 0, 0)).vale);
}

TEST_CASE("Magueyes: el trozo de cada celda no se sale de ella y sigue al modelo") {
    using namespace Compuesto;
    namespace AR = AgaveRigido;
    namespace AG = AgaveAzul;
    for (Familia f : { FAM_AGAVE_AZUL, FAM_MAGUEY_ESPADIN, FAM_AGAVE_ALBOMARGINATA, FAM_MAGUEY_SALMIANA,
                       FAM_AGAVE_POTATORUM, FAM_AGAVE_KARWINSKII, FAM_MAGUEY_MADRECUIXE, FAM_MAGUEY_CUISHE,
                       FAM_MAGUEY_CIRIAL, FAM_MAGUEY_SISAL, FAM_AGAVE_FOURCROYDES, FAM_AGAVE_AMERICANA })
        for (uint16_t e : { (uint16_t)AG::HIJUELO, (uint16_t)AG::MEDIA, (uint16_t)AG::MADURA }) {
            const BlockType t = AR::nuevo(f, e, 0);
            for (int s = 0; s < AR::celdasDe(t); ++s) {
                INFO("familia " << (int)f << " etapa " << e << " celda " << s);
                const CajaDeModelo c = cajaDeCeldaDeMaguey(AR::conSegmento(t, (uint16_t)s));
                REQUIRE(c.vale);
                CHECK(c.radio <= 0.5f);         // una caja no sale de su celda
                CHECK(c.radio >= 0.10f);
                CHECK(c.alto <= 1.0f);
                CHECK(c.alto > 0.0f);
            }
        }
    // Una mata chica ya no es un cubo entero: su caja es mas estrecha y mas baja.
    const CajaDeModelo chico = cajaDeCeldaDeMaguey(AR::nuevo(FAM_AGAVE_POTATORUM, AG::HIJUELO, 0));
    CHECK(chico.radio < 0.5f);
    CHECK(chico.alto < 1.0f);
    // El tramo que es solo tronco tiene el grosor del tronco; arriba, la roseta.
    const BlockType cirial = AR::nuevo(FAM_MAGUEY_CIRIAL, AG::MADURA, 0);
    const CajaDeModelo pie = cajaDeCeldaDeMaguey(cirial);
    const CajaDeModelo copa = cajaDeCeldaDeMaguey(AR::conSegmento(cirial, (uint16_t)(AR::celdasDe(cirial) - 1)));
    CHECK(pie.radio == AR::formaDe(FAM_MAGUEY_CIRIAL).radioTronco);
    CHECK(pie.alto == 1.0f);
    CHECK(copa.radio > pie.radio);
}

// BUG: "cuando rompo una variante del maguey, aparece el maguey normal de la
// nada y la hitbox de la otra especie se pone arriba".
TEST_CASE("Magueyes: golpear la punta de otra especie no la convierte en pulquero") {
    using namespace Compuesto;
    // Por que pasaba: conPuntas es del pulquero y escribe el bloque como tal.
    const BlockType espadin = AgaveRigido::nuevo(FAM_MAGUEY_ESPADIN, AgaveAzul::MADURA, 0);
    CHECK(familiaDe(Maguey::conPuntas(espadin, 0)) == FAM_MAGUEY);       // la trampa sigue ahi...
    // ...asi que el motor ya no entra en esa rama con otra familia.
    std::ifstream fsrc(raizM() + "src/main.cpp", std::ios::binary);
    const std::string src((std::istreambuf_iterator<char>(fsrc)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(src.empty());
    const size_t rama = src.find("if (Compuesto::familiaDe(blockType) == Compuesto::FAM_MAGUEY &&\n"
                                 "                Compuesto::componenteN(blockType, comp) ==\n"
                                 "                Compuesto::COMP_ADORNO) {");
    const size_t ramaCr = src.find("if (Compuesto::familiaDe(blockType) == Compuesto::FAM_MAGUEY &&\r\n"
                                   "                Compuesto::componenteN(blockType, comp) ==\r\n"
                                   "                Compuesto::COMP_ADORNO) {");
    CHECK((rama != std::string::npos || ramaCr != std::string::npos));
    // Y es el unico sitio donde se arrancan puntas.
    size_t veces = 0;
    for (size_t k = src.find("M::conPuntas(blockType,"); k != std::string::npos;
         k = src.find("M::conPuntas(blockType,", k + 1)) ++veces;
    CHECK(veces == 1);
    // La caja de cada celda y la que se marca salen del modelo.
    CHECK(src.find("Compuesto::cajaDeCeldaDeMaguey(type);") != std::string::npos);
    CHECK(src.find("Compuesto::cajaDelModeloDeMaguey(plantaSeca(get(0, -abajo, 0)));") != std::string::npos);
}


// PEDIDO: "anade este maguey Agave fourcroydes ... y Agave americana".
TEST_CASE("Fourcroydes y americana: dos magueyes mas, uno de tronco y otro a ras de suelo") {
    using namespace Compuesto;
    namespace AR = AgaveRigido;
    namespace AG = AgaveAzul;
    // Al final de sus listas: no mueven a nadie.
    CHECK((int)FAM_AGAVE_AMERICANA == (int)FAM_COUNT - 1);
    CHECK((int)FAM_AGAVE_FOURCROYDES == (int)FAM_COUNT - 2);
    CHECK((int)FAM_AGAVE_FOURCROYDES > (int)FAM_MAGUEY_SISAL);
    CHECK((int)BLOCK_AGAVE_AMERICANA_PENCA <= (int)BLOCK_TYPE_MAX);
    for (Familia f : { FAM_AGAVE_FOURCROYDES, FAM_AGAVE_AMERICANA }) {
        CHECK(AR::esRigido(f));
        CHECK(puedeDibujarse(AR::nuevo(f, AG::MADURA, 0)));
        const AR::Forma fm = AR::formaDe(f);
        CHECK(esPencaDeEspecie(fm.penca));
        CHECK(seTiraAlColocar(fm.penca));
        CHECK(fm.penca == fm.punta);
        // Crece: el hijuelo es mas bajo que la madura.
        CHECK(AR::alturaTotal(f, AG::HIJUELO) < AR::alturaTotal(f, AG::MADURA));
        CHECK(AR::radioDeEtapa(f, AG::MADURA) <= AR::RADIO_MAX);
    }
    // Fourcroydes: tronco de 1 a 2 m y 20 cm de grueso; roseta de 2,5 m; hojas numerosas.
    const AR::Forma fo = AR::formaDe(FAM_AGAVE_FOURCROYDES);
    CHECK(fo.penca == BLOCK_AGAVE_FOURCROYDES_PENCA);
    CHECK(fo.tronco >= 1.0f);
    CHECK(fo.tronco <= 2.0f);
    CHECK(fo.radioTronco == doctest::Approx(0.10f));
    CHECK(AR::radioDeEtapa(FAM_AGAVE_FOURCROYDES, AG::MADURA) * 2.0f == doctest::Approx(2.5f).epsilon(0.05));
    CHECK(fo.hojas > 1.0f);
    CHECK(fo.caida == 0.0f);                                   // rigidas
    CHECK(AR::celdasDe(AR::nuevo(FAM_AGAVE_FOURCROYDES, AG::MADURA, 0)) == 3);
    // Americana: sin tronco; de 0,9 a 1,8 m de alto y de 1,8 a 3 m de ancho; hojas carnosas.
    const AR::Forma am = AR::formaDe(FAM_AGAVE_AMERICANA);
    CHECK(am.penca == BLOCK_AGAVE_AMERICANA_PENCA);
    CHECK(am.tronco == 0.0f);
    const float altoA = AR::alturaReal(FAM_AGAVE_AMERICANA, AG::MADURA);
    CHECK(altoA >= 0.9f);
    CHECK(altoA <= 1.8f);
    const float anchoA = AR::radioDeEtapa(FAM_AGAVE_AMERICANA, AG::MADURA) * 2.0f;
    CHECK(anchoA >= 1.8f);
    CHECK(anchoA <= 3.0f);
    CHECK(am.grueso > fo.grueso);
    CHECK(AR::celdasDe(AR::nuevo(FAM_AGAVE_AMERICANA, AG::MADURA, 0)) == 2);
    // Nombres, tamano de la penca e imagenes.
    CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(FAM_AGAVE_FOURCROYDES, AG::HECHA, 0)), "AGAVE FOURCROYDES") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(AR::nuevo(FAM_AGAVE_AMERICANA, AG::HECHA, 0)), "AGAVE AMERICANA") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_AGAVE_FOURCROYDES_PENCA), "PENCA DE AGAVE FOURCROYDES") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_AGAVE_AMERICANA_PENCA), "PENCA DE AGAVE AMERICANA") == 0);
    CHECK(tamanoDePenca(BLOCK_AGAVE_AMERICANA_PENCA) > tamanoDePenca(BLOCK_MAGUEY_ESPADIN_PENCA));
    CHECK(tamanoDePenca(BLOCK_AGAVE_FOURCROYDES_PENCA) > tamanoDePenca(BLOCK_MAGUEY_ESPADIN_PENCA));
    for (const char* img : { "Agave fourcroydes.png", "Agave Americana.png" }) {
        std::ifstream t(raizM() + "resourcepacks/Textures/Blocks/" + img, std::ios::binary);
        CHECK(t.good());
    }
    std::ifstream fsrc(raizM() + "src/main.cpp", std::ios::binary);
    const std::string src((std::istreambuf_iterator<char>(fsrc)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("case BLOCK_AGAVE_FOURCROYDES_PENCA: return getTexture(\"Agave fourcroydes.png\");") != std::string::npos);
    CHECK(src.find("case BLOCK_AGAVE_AMERICANA_PENCA:   return getTexture(\"Agave Americana.png\");") != std::string::npos);
    CHECK(src.find("{ Compuesto::FAM_AGAVE_FOURCROYDES,   32, 4u, 0xcbbb9d5du") != std::string::npos);
    CHECK(src.find("{ Compuesto::FAM_AGAVE_AMERICANA,     30, 3u, 0x629a292au") != std::string::npos);
    CHECK(src.find("ponerEnCreativo(BLOCK_AGAVE_FOURCROYDES_PENCA);") != std::string::npos);
    CHECK(src.find("ponerEnCreativo(BLOCK_AGAVE_AMERICANA_PENCA);") != std::string::npos);
}
