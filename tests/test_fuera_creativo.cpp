#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <sstream>
#include <string>

// ============================================================================
// LO QUE SE QUITO DEL CREATIVO, Y EL NIVEL QUE SE VOLVIA BLOQUE
// ============================================================================

TEST_CASE("Creativo: fuera la base de ixtle, las piezas de maguey y los pinos por dentro") {
    CHECK(fueraDelCreativo(BLOCK_IXTLE_TALLO));
    CHECK(fueraDelCreativo(BLOCK_IXTLE_ENORME));
    CHECK(fueraDelCreativo(BLOCK_MAGUEY_HUECO));
    CHECK(fueraDelCreativo(BLOCK_AGAVE_AZUL_QUIOTE));
    CHECK(fueraDelCreativo(BLOCK_AGAVE_ABISAII_PENCA));
    CHECK(fueraDelCreativo(BLOCK_WOOD_OCOTE_DENTRO));
    CHECK(fueraDelCreativo(BLOCK_WOOD_PSEUDOSTROBUS_DENTRO));
    // Lo que sueltan los magueys naturales se queda.
    CHECK_FALSE(fueraDelCreativo(BLOCK_IXTLE_HOJA));
    CHECK_FALSE(fueraDelCreativo(BLOCK_PENCA_AGAVE_AZUL));
    // Y los troncos con corteza, tambien.
    CHECK_FALSE(fueraDelCreativo(BLOCK_WOOD_OCOTE));
    CHECK_FALSE(fueraDelCreativo(BLOCK_WOOD_PATULA));
}

TEST_CASE("Creativo: un tronco por dentro que quede en el mundo suelta el de corteza") {
    CHECK(troncoDeSuDentro(BLOCK_WOOD_PATULA_DENTRO) == BLOCK_WOOD_PATULA);
    CHECK(troncoDeSuDentro(BLOCK_WOOD_OCOTE_CHINO_DENTRO) == BLOCK_WOOD_OCOTE_CHINO);
    CHECK(troncoDeSuDentro(BLOCK_WOOD_ALTURAS_DENTRO) == BLOCK_WOOD_ALTURAS);
    CHECK(troncoDeSuDentro(BLOCK_STONE) == BLOCK_STONE);
}

TEST_CASE("Niveles: un material que se coloca por niveles no tapa la capa, la sube") {
    // enteroQueSustituyeNivel vive en main.cpp: se comprueba que corta antes
    // de devolver un cubo cuando lo de la mano admite niveles.
    std::string raiz;
    for (const char* r : { "", "../", "../../", "../../../", "../../../../" }) {
        std::ifstream f(std::string(r) + "src/main.cpp");
        if (f) { raiz = r; break; }
    }
    std::ifstream f(raiz + "src/main.cpp");
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string s = ss.str();
    const size_t ini = s.find("BlockType enteroQueSustituyeNivel(BlockType enMano, BlockType nivel) {");
    REQUIRE(ini != std::string::npos);
    const size_t fin = s.find("\n}\n", ini) != std::string::npos ? s.find("\n}\n", ini) : s.find("\r\n}\r\n", ini);
    const std::string cuerpo = s.substr(ini, fin - ini);
    const size_t guarda = cuerpo.find("if (admiteNiveles(enMano)) return BLOCK_AIR;");
    const size_t devuelve = cuerpo.find("return b;");
    REQUIRE(guarda != std::string::npos);
    CHECK(guarda < devuelve);
}
