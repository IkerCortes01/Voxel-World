// LAS FLORES: cempasuchil, nochebuena, cosmos y dalia (Compuesto::Flor).
// PEDIDO: "anade estas flores [...] y hazlas en 3D y que tengan la capacidad de
// crecer de forma realista y en donde sea".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "BloqueCompuesto.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

namespace FL = Compuesto::Flor;

namespace {
std::string raizF() {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    return r;
}
std::string leerF(const char* relativo) {
    std::ifstream f(raizF() + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Flores: son cuatro, cada una con su item, su nombre y su imagen") {
    CHECK((int)FL::N_ESPECIES == 4);
    static const struct { BlockType item; uint16_t especie; const char* nombre; const char* png; } F[4] = {
        { BLOCK_FLOR_CEMPASUCHIL, FL::CEMPASUCHIL, "CEMPASUCHIL", "Cempasuchil.png" },
        { BLOCK_FLOR_NOCHEBUENA,  FL::NOCHEBUENA,  "NOCHEBUENA",  "Nochebuena.png" },
        { BLOCK_FLOR_COSMOS,      FL::COSMOS,      "COSMOS",      "Cosmos.png" },
        { BLOCK_FLOR_DALIA,       FL::DALIA,       "DALIA",       "Dalia.png" },
    };
    const std::string src = leerF("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    for (const auto& f : F) {
        INFO(f.nombre);
        CHECK(esItemDeFlor(f.item));
        CHECK(especieDeItemDeFlor(f.item) == (int)f.especie);
        CHECK(itemDeFlor((int)f.especie) == f.item);
        CHECK(std::strcmp(NombreItem::nombreDeItem(f.item), f.nombre) == 0);
        // Plantada, a cualquier edad, se llama igual.
        for (uint16_t e = 0; e < FL::N_ETAPAS; ++e)
            CHECK(std::strcmp(NombreItem::nombreDeItem(FL::crear(f.especie, e, 1)), f.nombre) == 0);
        // La imagen esta en disco y el motor la pide.
        std::ifstream img(raizF() + "resourcepacks/Textures/Items/" + f.png, std::ios::binary);
        CHECK(img.good());
        CHECK(src.find(std::string("getTexture(\"../Items/") + f.png + "\")") != std::string::npos);
        CHECK(src.find(std::string("siluetaPNG = \"Textures/Items/") + f.png + "\"; break;") != std::string::npos);
    }
    CHECK_FALSE(esItemDeFlor(BLOCK_MAGUEY_SISAL_PENCA));
    CHECK_FALSE(esItemDeFlor(BLOCK_TALLGRASS));
    CHECK((int)BLOCK_FLOR_DALIA < (int)BLOCK_TYPE_MAX);
}

TEST_CASE("Flores: el estado guarda especie, etapa y giro, y no se pisa con otra familia") {
    std::set<int> ids;
    for (uint16_t s = 0; s < FL::N_ESPECIES; ++s)
        for (uint16_t e = 0; e < FL::N_ETAPAS; ++e)
            for (uint16_t g = 0; g < 4; ++g) {
                const BlockType b = FL::crear(s, e, g);
                CHECK(FL::esFlor(b));
                CHECK(Compuesto::familiaDe(b) == Compuesto::FAM_PLANTA_FLOR);
                CHECK(FL::especieDe(b) == s);
                CHECK(FL::etapaDe(b) == e);
                CHECK(FL::giroDe(b) == g);
                CHECK(Compuesto::puedeDibujarse(b));
                ids.insert((int)b);
            }
    CHECK(ids.size() == 4u * 5u * 4u);          // todas distintas
    CHECK_FALSE(FL::esFlor(BLOCK_AIR));
    CHECK_FALSE(FL::esFlor(BLOCK_TALLGRASS));
    CHECK_FALSE(FL::esFlor(Compuesto::Biznaga::nuevo(0, 0, 0)));
    // Un estado raro (etapa fuera de rango) se lee como una flor valida.
    const BlockType raro = Compuesto::hacer(Compuesto::FAM_PLANTA_FLOR, 0xFFFF);
    CHECK(FL::etapaDe(raro) < FL::N_ETAPAS);
    CHECK(FL::especieDe(raro) < FL::N_ESPECIES);
}

TEST_CASE("Flores: crecen de brote a abierta, una etapa cada vez, y ahi se quedan") {
    for (uint16_t s = 0; s < FL::N_ESPECIES; ++s) {
        BlockType b = FL::crear(s, FL::BROTE, 2);
        float antes = FL::altoDe(b);
        CHECK_FALSE(FL::abierta(b));
        for (int paso = 1; paso < FL::N_ETAPAS; ++paso) {
            const BlockType c = FL::crecida(b);
            CHECK(FL::etapaDe(c) == FL::etapaDe(b) + 1);
            CHECK(FL::especieDe(c) == s);           // sigue siendo la misma planta
            CHECK(FL::giroDe(c) == 2);              // y mirando al mismo lado
            CHECK(FL::altoDe(c) > antes);           // cada etapa, mas alta
            antes = FL::altoDe(c);
            b = c;
        }
        CHECK(FL::etapaDe(b) == FL::ABIERTA);
        CHECK(FL::abierta(b));
        CHECK(FL::crecida(b) == b);                 // ya no crece mas
        // La flor se ve desde JOVEN; antes solo hay mata.
        CHECK_FALSE(FL::abierta(FL::crear(s, FL::TALLO, 0)));
        CHECK(FL::abierta(FL::crear(s, FL::JOVEN, 0)));
    }
    // El brote es un punado de hojas, no una planta entera.
    CHECK(FL::escalaDeEtapa(FL::BROTE) <= 0.25f);
    CHECK(FL::escalaDeEtapa(FL::ABIERTA) == 1.0f);
}

TEST_CASE("Flores: las medidas de cada una son las de la planta de verdad") {
    // Cempasuchil 60 cm a 1 m; dalia y cosmos de alrededor de un metro; la
    // nochebuena es un arbusto y es la mas alta.
    CHECK(FL::medidasDe(FL::CEMPASUCHIL).alto >= 0.6f);
    CHECK(FL::medidasDe(FL::CEMPASUCHIL).alto <= 1.0f);
    CHECK(FL::medidasDe(FL::NOCHEBUENA).alto > FL::medidasDe(FL::COSMOS).alto);
    CHECK(FL::medidasDe(FL::COSMOS).alto > FL::medidasDe(FL::CEMPASUCHIL).alto);
    for (uint16_t s = 0; s < FL::N_ESPECIES; ++s) {
        const FL::Medidas m = FL::medidasDe(s);
        INFO("especie " << s);
        // La flor cabe en su imagen, y queda por encima del tramo de tallo.
        CHECK(m.uCab - m.rCab >= 0.0f);
        CHECK(m.uCab + m.rCab <= 1.0f);
        CHECK(m.vCab - m.rCab >= -0.08f);
        CHECK(m.vCab < m.vTallo);
        CHECK(m.vTallo > 0.3f);
        CHECK(m.vTallo < 0.8f);
        CHECK(m.uPie > 0.4f);
        CHECK(m.uPie < 0.6f);
        CHECK(m.alto <= 1.4f);
    }
}

TEST_CASE("Flores: el ritmo de crecer es de minutos, no de segundos ni de horas") {
    const float porEtapa = FL::segundosPorEtapa();
    CHECK(porEtapa >= 120.0f);
    CHECK(porEtapa <= 600.0f);
    const float entera = porEtapa * (float)(FL::N_ETAPAS - 1);
    CHECK(entera >= 8.0f * 60.0f);
    CHECK(entera <= 40.0f * 60.0f);
    CHECK(FL::MAX_VECINAS >= 3);                // se hacen manchones...
    CHECK(FL::MAX_VECINAS <= 8);                // ...pero no una alfombra
    CHECK(FL::ALCANCE_SEMILLA >= 2);
}

TEST_CASE("Flores: el motor las dibuja en 3D, las deja crecer, plantar, cortar y nacer en el mundo") {
    const std::string src = leerF("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // No frenan, no tapan caras y dejan pasar el sol.
    CHECK(src.find("if (Compuesto::Flor::esFlor(type)) return false;") != std::string::npos);
    CHECK(src.find("if (Compuesto::Flor::esFlor(type)) return true;") != std::string::npos);
    CHECK(src.find("if (Compuesto::Flor::esFlor(encima)) return false;") != std::string::npos);
    // "En donde sea": sobre cualquier bloque macizo y entero.
    const size_t suelo = src.find("bool sueloDeFlor(BlockType b) {");
    REQUIRE(suelo != std::string::npos);
    CHECK(src.substr(suelo, 400).find("return isBlockSolid(b) && !isCrossSprite(b);") != std::string::npos);
    // En 3D: tres laminas cruzadas y la corola tumbada, a doble cara.
    const size_t dib = src.find("LAS FLORES, EN 3D");
    REQUIRE(dib != std::string::npos);
    const std::string cuerpo = src.substr(dib, 5200);
    CHECK(cuerpo.find("for (int k = 0; k < 3; ++k) {") != std::string::npos);
    CHECK(cuerpo.find("(float)k * 1.0472f") != std::string::npos);           // 60 grados
    CHECK(cuerpo.find("for (int i = 3; i >= 0; --i)") != std::string::npos); // la otra cara
    CHECK(cuerpo.find("if (flor) {") != std::string::npos);                  // la corola
    // Crecen solas, se siembran y se pierden sin suelo.
    CHECK(src.find("mundoF.setBlock(x, y, z, FL::crecida(b));") != std::string::npos);
    CHECK(src.find("FL::crear(FL::especieDe(b), FL::BROTE,") != std::string::npos);
    CHECK(src.find("if (cerca < FL::MAX_VECINAS)") != std::string::npos);
    CHECK(src.find("mundoF.setBlock(x, y, z, BLOCK_AIR);            // sin suelo no vive") != std::string::npos);
    // Plantar: de la flor nace un brote.
    CHECK(src.find("Compuesto::Flor::BROTE, (uint16_t)(rand() & 3));") != std::string::npos);
    // Cortar: da su flor si ya abrio.
    CHECK(src.find("state->spawnItem(pos, itemDeFlor((int)FL::especieDe(blockType)));") != std::string::npos);
    // Nacen con el mundo, en cualquier bioma.
    const size_t gen = src.find("---- FLORES: cempasuchil, nochebuena, cosmos y dalia ----");
    REQUIRE(gen != std::string::npos);
    const std::string siembra = src.substr(gen, 2600);
    CHECK(siembra.find("sueloDeFlor(ground)") != std::string::npos);
    CHECK(siembra.find("col.biome") == std::string::npos);       // sin filtro de bioma
    CHECK(siembra.find("FL::crear((uint16_t)e, etapa,") != std::string::npos);
}

// LA PINA DE PINO. PEDIDO: "haz que esto este en el suelo o caiga de las hojas
// de los arboles, solo de la especie de la vida real que da esto".
TEST_CASE("Pinas: solo las dan los pinos; ni el encino ni el oyamel") {
    // Genero Pinus: el pino, los dos ocotes, patula, pinonero, alturas y pseudostrobus.
    for (BlockType h : { BLOCK_LEAVES, BLOCK_LEAVES_OCOTE, BLOCK_LEAVES_OCOTE_RAMA, BLOCK_LEAVES_OCOTE_CHINO,
                         BLOCK_LEAVES_OCOTE_CHINO_RAMA, BLOCK_LEAVES_PATULA, BLOCK_LEAVES_PINONERO,
                         BLOCK_LEAVES_ALTURAS, BLOCK_LEAVES_PSEUDOSTROBUS }) {
        INFO("hoja " << (int)h);
        CHECK(daPinas(h));
        CHECK(esHojaDeArbol(h));
    }
    // El encino es un roble (bellotas) y el oyamel un abeto (sus conos se
    // deshacen en la rama): no dan pinas.
    CHECK_FALSE(daPinas(BLOCK_LEAVES_ENCINO));
    CHECK_FALSE(daPinas(BLOCK_LEAVES_OYAMEL));
    // Ni lo que no es copa.
    for (BlockType b : { BLOCK_WOOD, BLOCK_PLANKS, BLOCK_TECHO_HOJAS, BLOCK_TALLGRASS, BLOCK_GRASS, BLOCK_AIR,
                         BLOCK_PINA_PINO })
        CHECK_FALSE(daPinas(b));
    // De todas las hojas de arbol del juego, las unicas que NO dan pina son esas dos.
    int sinPina = 0;
    for (int id = 1; id <= (int)BLOCK_TYPE_MAX; ++id) {
        const BlockType b = (BlockType)id;
        if (esNivelParcial(b) || !esHojaDeArbol(b)) continue;
        if (!daPinas(b)) ++sinPina;
    }
    CHECK(sinPina == 2);
}

TEST_CASE("Pinas: es un item con nombre e imagen, que se deja en el suelo") {
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_PINA_PINO), "PINA DE PINO") == 0);
    CHECK(seTiraAlColocar(BLOCK_PINA_PINO));
    CHECK((int)BLOCK_PINA_PINO <= (int)BLOCK_TYPE_MAX);
    // La imagen que usa el juego tiene el nombre sin enie (el motor no abre
    // archivos con enie: por eso existe "pinonero").
    std::ifstream img(raizF() + "resourcepacks/Textures/Items/Pina de Pino.png", std::ios::binary);
    CHECK(img.good());
    const std::string src = leerF("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("{ BLOCK_PINA_PINO,      \"Pina de Pino.png\"") != std::string::npos);
    CHECK(src.find("siluetaPNG = \"Textures/Items/Pina de Pino.png\"; break;") != std::string::npos);
}

TEST_CASE("Pinas: caen de la copa de los pinos y salen al romper sus hojas, sin enterrar el pinar") {
    const std::string src = leerF("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    const size_t ini = src.find("LAS PINAS CAEN DE LOS PINOS");
    REQUIRE(ini != std::string::npos);
    const std::string cuerpo = src.substr(ini, 3600);
    // Solo bajo hoja de pino, y con aire debajo.
    CHECK(cuerpo.find("if (!daPinas(plantaSeca(mundoP.getBlock(x, y, z)))) continue;") != std::string::npos);
    CHECK(cuerpo.find("if (mundoP.getBlock(x, y - 1, z) != BLOCK_AIR) break;") != std::string::npos);
    // Nace como cosa dejada: cae con su fisica y se queda en el suelo.
    CHECK(cuerpo.find("state->dejarItem(") != std::string::npos);
    CHECK(cuerpo.find("BLOCK_PINA_PINO);") != std::string::npos);
    // Con tope.
    CHECK(cuerpo.find("if (cerca < PinasDePino::MAXIMAS) {") != std::string::npos);
    CHECK(PinasDePino::MAXIMAS >= 6);
    CHECK(PinasDePino::MAXIMAS <= 40);
    // Al romper la hoja, a veces.
    CHECK(src.find("if (daPinas(blockType) && (rand() % 100) < PinasDePino::AL_ROMPER_DE_100)") != std::string::npos);
    CHECK(PinasDePino::AL_ROMPER_DE_100 > 0);
    CHECK(PinasDePino::AL_ROMPER_DE_100 < 50);
}
