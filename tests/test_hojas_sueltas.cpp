#include <doctest/doctest.h>
#include "BlockType.h"
#include <vector>
#include <array>

// ============================================================================
// LAS HOJAS SEPARADAS DEL ARBOL SE ROMPEN
// ============================================================================
// Una hoja no se sostiene sola: vive del tronco. Al talar un arbol, la copa
// que queda flotando se deshace poco a poco y suelta lo que dejaria una hoja
// rota.
//
// Aqui se fija la LOGICA, que es pura sobre BlockType y no necesita OpenGL:
//
//   1. Los predicados esHojaDeArbol / esTroncoDeArbol reconocen las CUATRO
//      especies. El ocote es el que se cuela: se anadio despues y quedo fuera
//      de varias listas del motor.
//
//   2. La busqueda de soporte, reimplementada aqui sobre un mundo de mentira.
//      Es el mismo algoritmo que hojaTieneSoporte() de main.cpp: anchura
//      primero por hojas, hasta HOJA_ALCANCE pasos, buscando un tronco.
// ============================================================================

namespace {

// El alcance real del motor (ver HOJA_ALCANCE en main.cpp).
constexpr int ALCANCE = 4;

// Un mundo de mentira: solo lo que hace falta para probar la regla.
struct MundoFalso {
    std::vector<std::vector<std::vector<BlockType>>> celdas;
    int lado;

    explicit MundoFalso(int n) : lado(n) {
        celdas.assign(n, std::vector<std::vector<BlockType>>(
                             n, std::vector<BlockType>(n, BLOCK_AIR)));
    }

    bool dentro(int x, int y, int z) const {
        return x >= 0 && y >= 0 && z >= 0 &&
               x < lado && y < lado && z < lado;
    }
    BlockType get(int x, int y, int z) const {
        return dentro(x, y, z) ? celdas[x][y][z] : BLOCK_AIR;
    }
    void set(int x, int y, int z, BlockType b) {
        if (dentro(x, y, z)) celdas[x][y][z] = b;
    }
};

// MISMO ALGORITMO que hojaTieneSoporte() en main.cpp: anchura primero,
// avanzando solo por hojas, cortando en cuanto encuentra tronco.
bool tieneSoporte(const MundoFalso& m, int x, int y, int z) {
    std::vector<std::array<int,3>> frente{ {x, y, z} }, siguiente;
    std::vector<std::array<int,3>> vistos{ {x, y, z} };

    auto yaVisto = [&](int a, int b, int c) {
        for (const auto& v : vistos)
            if (v[0] == a && v[1] == b && v[2] == c) return true;
        return false;
    };

    static const int CARAS[6][3] = {
        { 1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };

    for (int paso = 0; paso < ALCANCE; ++paso) {
        siguiente.clear();
        for (const auto& p : frente) {
            for (int i = 0; i < 6; ++i) {
                const int nx = p[0] + CARAS[i][0];
                const int ny = p[1] + CARAS[i][1];
                const int nz = p[2] + CARAS[i][2];
                if (yaVisto(nx, ny, nz)) continue;

                const BlockType vec = m.get(nx, ny, nz);
                if (esTroncoDeArbol(vec)) return true;
                if (!esHojaDeArbol(vec)) continue;

                vistos.push_back({nx, ny, nz});
                siguiente.push_back({nx, ny, nz});
            }
        }
        if (siguiente.empty()) break;
        frente.swap(siguiente);
    }
    return false;
}

} // namespace

// ----------------------------------------------------------------------------
// LOS PREDICADOS RECONOCEN LAS CUATRO ESPECIES
// ----------------------------------------------------------------------------

TEST_CASE("Hojas: las cuatro especies se reconocen como hoja") {
    // El OCOTE es el que importa: se anadio como cuarta especie despues del
    // resto y se quedo fuera de varias listas del motor (por ejemplo de la
    // isCrossSprite de los tests). Si vuelve a quedarse fuera, sus hojas no
    // se pudrirían nunca y una copa de ocote flotaria para siempre.
    CHECK(esHojaDeArbol(BLOCK_LEAVES));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_ENCINO));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_OYAMEL));
    CHECK(esHojaDeArbol(BLOCK_LEAVES_OCOTE));
}

TEST_CASE("Hojas: lo que NO es hoja no se confunde") {
    const BlockType NO_HOJA[] = {
        BLOCK_AIR, BLOCK_WOOD, BLOCK_WOOD_OCOTE, BLOCK_STONE, BLOCK_DIRT,
        BLOCK_TALLGRASS, BLOCK_RAMA_PINO, BLOCK_RAMA_OCOTE,
        BLOCK_NOPAL_CLADODIO, BLOCK_IXTLE_HOJA,   // "hoja" de maguey: no cuenta
    };
    for (BlockType t : NO_HOJA) {
        INFO("no deberia ser hoja de arbol: ", (int)t);
        CHECK_FALSE(esHojaDeArbol(t));
    }
}

TEST_CASE("Hojas: el tronco de las cuatro especies sostiene") {
    CHECK(esTroncoDeArbol(BLOCK_WOOD));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_ENCINO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_OYAMEL));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_OCOTE));
    // El corazon del ocote es el mismo tronco visto por dentro.
    CHECK(esTroncoDeArbol(BLOCK_WOOD_OCOTE_DENTRO));
}

TEST_CASE("Hojas: una RAMA no cuenta como tronco") {
    // Una rama suelta, sin tronco, no deberia sostener una copa entera. Si
    // contara, talar el tronco dejaria la copa colgada de sus propias ramas.
    CHECK_FALSE(esTroncoDeArbol(BLOCK_RAMA_PINO));
    CHECK_FALSE(esTroncoDeArbol(BLOCK_RAMA_ENCINO));
    CHECK_FALSE(esTroncoDeArbol(BLOCK_RAMA_OYAMEL));
    CHECK_FALSE(esTroncoDeArbol(BLOCK_RAMA_OCOTE));
}

TEST_CASE("Hojas: una capa parcial de hoja sigue siendo hoja") {
    // Igual que en el resto del motor: la regla vale para el bloque entero y
    // para su loncha.
    for (int n = 1; n <= 7; ++n) {
        INFO("capa de hoja, nivel ", n);
        CHECK(esHojaDeArbol(conNivel(BLOCK_LEAVES, n)));
    }
}

// ----------------------------------------------------------------------------
// LA REGLA DE SOPORTE
// ----------------------------------------------------------------------------

TEST_CASE("Hojas: una hoja pegada al tronco aguanta") {
    MundoFalso m(24);
    m.set(10, 10, 10, BLOCK_WOOD);
    m.set(11, 10, 10, BLOCK_LEAVES);
    CHECK(tieneSoporte(m, 11, 10, 10));
}

TEST_CASE("Hojas: una hoja SOLA en el aire no aguanta") {
    MundoFalso m(24);
    m.set(5, 5, 5, BLOCK_LEAVES);   // sin nada alrededor
    CHECK_FALSE(tieneSoporte(m, 5, 5, 5));
}

TEST_CASE("Hojas: la cadena llega hasta 4 hojas de distancia") {
    // El tronco en el origen y una fila de hojas alejandose. A distancia 4
    // todavia se sostiene; a 5 ya no.
    MundoFalso m(24);
    m.set(10, 10, 10, BLOCK_WOOD);
    for (int d = 1; d <= 6; ++d) m.set(10 + d, 10, 10, BLOCK_LEAVES);

    for (int d = 1; d <= ALCANCE; ++d) {
        INFO("hoja a distancia ", d);
        CHECK(tieneSoporte(m, 10 + d, 10, 10));
    }
    INFO("hoja a distancia ", ALCANCE + 1);
    CHECK_FALSE(tieneSoporte(m, 10 + ALCANCE + 1, 10, 10));
}

TEST_CASE("Hojas: una copa ENTERA sin tronco se queda sin soporte") {
    // ESTE ES EL CASO QUE MOTIVA TODO.
    //
    // Sin la regla de la cadena, un bloque de hojas se sostendria a si mismo:
    // cada hoja tiene hojas vecinas, asi que ninguna "esta suelta". Una copa
    // talada flotaria para siempre.
    //
    // Con la busqueda de TRONCO, ninguna se salva.
    MundoFalso m(24);
    for (int x = 8; x <= 12; ++x)
        for (int y = 8; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z)
                m.set(x, y, z, BLOCK_LEAVES);
    // No hay ni un tronco en todo el mundo.

    int sostenidas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 8; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z)
                if (tieneSoporte(m, x, y, z)) ++sostenidas;

    CHECK(sostenidas == 0);
}

TEST_CASE("Hojas: una copa CON su tronco no se cae sola") {
    // El reverso del anterior, y es igual de importante: un arbol intacto no
    // puede empezar a deshojarse solo. Si esto fallara, los bosques del mundo
    // se desharian sin que nadie los tocara.
    MundoFalso m(24);

    // Tronco vertical por el eje de la copa.
    for (int y = 6; y <= 12; ++y) m.set(10, y, 10, BLOCK_WOOD);

    // Copa de radio 2 alrededor de la punta.
    int hojas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z) {
                if (m.get(x, y, z) != BLOCK_AIR) continue;   // no pisar tronco
                m.set(x, y, z, BLOCK_LEAVES);
                ++hojas;
            }
    REQUIRE(hojas > 0);

    int sueltas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z)
                if (esHojaDeArbol(m.get(x, y, z)) && !tieneSoporte(m, x, y, z))
                    ++sueltas;

    INFO("hojas de la copa: ", hojas, ", sueltas: ", sueltas);
    CHECK(sueltas == 0);
}

TEST_CASE("Hojas: la cadena NO pasa por ramas ni por aire") {
    // El tronco esta a 2 bloques, pero por medio hay una RAMA en vez de una
    // hoja. Como la busqueda solo avanza por hojas, no llega: la rama no
    // conduce soporte.
    MundoFalso m(24);
    m.set(10, 10, 10, BLOCK_WOOD);
    m.set(11, 10, 10, BLOCK_RAMA_PINO);   // el puente que NO vale
    m.set(12, 10, 10, BLOCK_LEAVES);

    CHECK_FALSE(tieneSoporte(m, 12, 10, 10));
}

TEST_CASE("Hojas: especies mezcladas se sostienen entre si") {
    // La regla es por FAMILIA (hoja/tronco), no por especie: una hoja de
    // ocote pegada a un tronco de encino cuenta como sostenida. Es lo
    // razonable -- el motor no modela injertos, y exigir que coincidan haria
    // que una copa mixta se cayera a trozos.
    MundoFalso m(24);
    m.set(10, 10, 10, BLOCK_WOOD_ENCINO);
    m.set(11, 10, 10, BLOCK_LEAVES_OCOTE);
    CHECK(tieneSoporte(m, 11, 10, 10));
}

// ============================================================================
// EL OCOTE CHINO (Pinus leiophylla)
// ============================================================================
// La quinta especie. Es la prueba de fuego de haber centralizado los
// predicados: si esHojaDeArbol y esTroncoDeArbol son la unica fuente de
// verdad, anadir una especie es tocarlos a ellos y el resto del motor se
// entera solo.

TEST_CASE("Ocote chino: sus hojas son hojas de arbol") {
    CHECK(esHojaDeArbol(BLOCK_LEAVES_OCOTE_CHINO));
    // Y la celda que lleva la rama dentro TAMBIEN: por dentro hay ramaje,
    // pero de cara al mundo (soporte, hacha, mesher) es follaje. Si no lo
    // fuera, esas celdas no se pudririan al talar y la copa quedaria a
    // trozos: el esqueleto de ramas flotando y las hojas sueltas caidas.
    CHECK(esHojaDeArbol(BLOCK_LEAVES_OCOTE_CHINO_RAMA));
}

TEST_CASE("Ocote chino: su tronco sostiene la copa") {
    CHECK(esTroncoDeArbol(BLOCK_WOOD_OCOTE_CHINO));
    CHECK(esTroncoDeArbol(BLOCK_WOOD_OCOTE_CHINO_DENTRO));
}

TEST_CASE("Ocote chino: la copa se sostiene de su tronco") {
    // El caso completo: tronco + copa con celdas de hoja y de hoja+rama
    // mezcladas, que es como la deja el generador.
    MundoFalso m(24);
    for (int y = 6; y <= 12; ++y) m.set(10, y, 10, BLOCK_WOOD_OCOTE_CHINO);

    // Copa alrededor de la punta, alternando los dos tipos de celda.
    int puestas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z) {
                if (m.get(x, y, z) != BLOCK_AIR) continue;
                m.set(x, y, z, ((x + y + z) % 3 == 0)
                                   ? BLOCK_LEAVES_OCOTE_CHINO_RAMA
                                   : BLOCK_LEAVES_OCOTE_CHINO);
                ++puestas;
            }
    REQUIRE(puestas > 0);

    int sueltas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z)
                if (esHojaDeArbol(m.get(x, y, z)) && !tieneSoporte(m, x, y, z))
                    ++sueltas;

    INFO("celdas de copa: ", puestas, ", sueltas: ", sueltas);
    CHECK(sueltas == 0);
}

TEST_CASE("Ocote chino: talado el tronco, la copa entera se queda sin soporte") {
    // Mismo montaje sin tronco: ni una sola celda debe salvarse, ni siquiera
    // las que llevan rama dentro. La rama NO sostiene -- si contara, la copa
    // se quedaria colgada de su propio ramaje para siempre.
    MundoFalso m(24);
    int puestas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z) {
                m.set(x, y, z, ((x + y + z) % 3 == 0)
                                   ? BLOCK_LEAVES_OCOTE_CHINO_RAMA
                                   : BLOCK_LEAVES_OCOTE_CHINO);
                ++puestas;
            }

    int sostenidas = 0;
    for (int x = 8; x <= 12; ++x)
        for (int y = 10; y <= 12; ++y)
            for (int z = 8; z <= 12; ++z)
                if (tieneSoporte(m, x, y, z)) ++sostenidas;

    INFO("celdas: ", puestas);
    CHECK(sostenidas == 0);
}

TEST_CASE("Ocote chino: la celda de hoja+rama son DOS piezas") {
    // Es una celda compartida: por dentro conviven la rama y el follaje, cada
    // uno con su caja. Es lo que permite ver el ramaje entre las hojas sin
    // gastar el doble de bloques.
    CHECK(esCompartido(BLOCK_LEAVES_OCOTE_CHINO_RAMA));
    CHECK(piezasDe(BLOCK_LEAVES_OCOTE_CHINO_RAMA) == 2);

    // La PRINCIPAL es la rama: es lo que sujeta el follaje, igual que el
    // cajete del maguey sujeta su jugo.
    CHECK(piezaN(BLOCK_LEAVES_OCOTE_CHINO_RAMA, 0) == BLOCK_RAMA_OCOTE);
    // Y la acompanante, las hojas.
    CHECK(piezaN(BLOCK_LEAVES_OCOTE_CHINO_RAMA, 1) == BLOCK_LEAVES_OCOTE_CHINO);
}

TEST_CASE("Ocote chino: el enum no piso a nadie") {
    // Los bloques nuevos van al FINAL: insertar en medio correria los IDs y
    // los mundos guardados leerian otro bloque en su lugar.
    CHECK((int)BLOCK_WOOD_OCOTE_CHINO > (int)BLOCK_PENCA_AGAVE_AZUL);
    CHECK((int)BLOCK_LEAVES_OCOTE_CHINO_RAMA <= BLOCK_TYPE_MAX);

    // Y el orden interno de la especie, que es el que asume piezaN.
    CHECK((int)BLOCK_WOOD_OCOTE_CHINO_DENTRO == (int)BLOCK_WOOD_OCOTE_CHINO + 1);
    CHECK((int)BLOCK_LEAVES_OCOTE_CHINO      == (int)BLOCK_WOOD_OCOTE_CHINO + 2);
}
