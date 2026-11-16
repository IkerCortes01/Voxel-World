// ============================================================================
// EL SISTEMA DE ORIENTACION DE LAS PIEZAS
// ============================================================================
// PEDIDO: "que escaleras, paredes y miles de modificaciones de bloques mas,
// como en Minecraft, se coloquen con la cara viendo frente al jugador", y
// "que al presionar clic derecho con el martillo cambies la orientacion de
// norte, sur, este, oeste, noreste, sureste, suroeste, noroeste".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "BloqueCompuesto.h"
#include "Formas.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

using namespace Formas;

namespace {
int formaLlamada(const char* nombre) {
    for (int i = 0; i < NUM; ++i) if (std::strcmp(forma(i).nombre, nombre) == 0) return i;
    return -1;
}
// La caja mas ALTA de la pieza (el ultimo peldano): por donde cae su centro.
void centroDeLoAlto(BlockType t, float& cx, float& cz) {
    float c[4][6];
    const int n = cajasDePieza(t, c);
    int m = 0;
    for (int i = 1; i < n; ++i) if (c[i][1] > c[m][1]) m = i;
    cx = (c[m][0] + c[m][3]) * 0.5f; cz = (c[m][2] + c[m][5]) * 0.5f;
}
std::string leerOri(const char* rel) {
    for (const char* pre : { "", "../", "../../", "../../../", "../../../../" }) {
        std::ifstream f(std::string(pre) + rel, std::ios::binary);
        if (f.good()) return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }
    return {};
}
}  // namespace

TEST_CASE("Orientacion: desde las cuatro direcciones la escalera sube alejandose de quien la pone") {
    static_assert(NUM_MATERIALES <= 64, "el material cabe en 6 bits");
    const float M[4][2] = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };
    for (int f = 0; f <= 2; ++f) {                 // la de 2, la de 3 y la de 4 peldanos: la MISMA regla
        for (const auto& m : M) {
            const BlockType t = alColocar(pieza(3, f), m[0], m[1], 1, 0.0f);
            INFO("forma " << f << " mirada " << m[0] << "," << m[1]);
            CHECK_FALSE(invertidaDe(t));
            CHECK_FALSE(diagonalDe(t));
            float cx = 0, cz = 0;
            centroDeLoAlto(t, cx, cz);
            // Lo alto queda del lado hacia el que se mira.
            CHECK(((cx - 0.5f) * m[0] + (cz - 0.5f) * m[1]) > 0.2f);
            CHECK(formaDePieza(t) == f);
            CHECK(materialDePieza(t) == 3);
        }
        // Desde el lado contrario, media vuelta.
        CHECK(((giroDePieza(alColocar(pieza(0, f), 0, 1, 1, 0)) + 2) & 3) == giroDePieza(alColocar(pieza(0, f), 0, -1, 1, 0)));
        CHECK(((giroDePieza(alColocar(pieza(0, f), 1, 0, 1, 0)) + 2) & 3) == giroDePieza(alColocar(pieza(0, f), -1, 0, 1, 0)));
    }
    // En diagonal exacta no hay sorteo: siempre lo mismo.
    CHECK(giroDeMirada(1.0f, 1.0f) == 0);
    CHECK(giroDeMirada(-1.0f, -1.0f) == 2);
    CHECK(giroDeMirada(1.0f, -1.0f) == 2);
    CHECK(giroDeMirada(-1.0f, 1.0f) == 0);
}

TEST_CASE("Orientacion: invertida contra el techo y en la mitad alta de una cara de lado") {
    CHECK_FALSE(invertidaAlColocar(1, 0.9f));      // sobre un bloque: normal, se apunte donde se apunte
    CHECK(invertidaAlColocar(-1, 0.1f));           // contra el techo: invertida
    CHECK(invertidaAlColocar(0, 0.75f));           // de lado, arriba
    CHECK_FALSE(invertidaAlColocar(0, 0.25f));     // de lado, abajo
    CHECK_FALSE(invertidaAlColocar(0, 0.5f));      // justo en la mitad: normal, sin sorteo
    for (int f = 0; f <= 2; ++f) for (int g = 0; g < 4; ++g) {
        float a[4][6], b[4][6];
        const int n = cajasDePieza(pieza(1, f, g), a);
        REQUIRE(cajasDePieza(pieza(1, f, g, true), b) == n);
        for (int i = 0; i < n; ++i) {
            // Las mismas cajas, espejadas en vertical; a lo ancho y a lo largo, iguales.
            CHECK(b[i][1] == doctest::Approx(1.0f - a[i][4]));
            CHECK(b[i][4] == doctest::Approx(1.0f - a[i][1]));
            CHECK(b[i][0] == a[i][0]); CHECK(b[i][3] == a[i][3]);
            CHECK(b[i][2] == a[i][2]); CHECK(b[i][5] == a[i][5]);
        }
    }
    CHECK(invertidaDe(alColocar(pieza(0, 0), 0, 1, 0, 0.8f)));
    CHECK_FALSE(invertidaDe(alColocar(pieza(0, 0), 0, 1, 0, 0.2f)));
    CHECK(invertidaDe(alColocar(pieza(0, 0), 0, 1, -1, 0.0f)));
}

TEST_CASE("Orientacion: las paredes quedan al fondo, giradas con la mirada y sin salirse de la casilla") {
    const int pared = formaLlamada("PARED"), central = formaLlamada("PARED CENTRAL");
    REQUIRE(pared >= 0);
    REQUIRE(central >= 0);
    const float M[4][2] = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };
    for (const auto& m : M) {
        float c[4][6];
        REQUIRE(cajasDePieza(alColocar(pieza(0, pared), m[0], m[1], 1, 0), c) == 1);
        const float cx = (c[0][0] + c[0][3]) * 0.5f - 0.5f, cz = (c[0][2] + c[0][5]) * 0.5f - 0.5f;
        CHECK((cx * m[0] + cz * m[1]) > 0.3f);                 // pegada a la cara del fondo
        // La central sigue en el centro, y solo cambia de eje.
        REQUIRE(cajasDePieza(alColocar(pieza(0, central), m[0], m[1], 1, 0), c) == 1);
        CHECK((c[0][0] + c[0][3]) * 0.5f == doctest::Approx(0.5f));
        CHECK((c[0][2] + c[0][5]) * 0.5f == doctest::Approx(0.5f));
        const bool aLoLargoDeX = (c[0][3] - c[0][0]) > (c[0][5] - c[0][2]);
        CHECK(aLoLargoDeX == (m[0] == 0));
    }
    // Ninguna forma, en ninguna orientacion, se sale de su casilla.
    for (int f = 0; f < NUM; ++f) for (int g = 0; g < 4; ++g) for (int inv = 0; inv < 2; ++inv) {
        float c[4][6];
        const int n = cajasDePieza(pieza(0, f, g, inv != 0), c);
        for (int i = 0; i < n; ++i) for (int k = 0; k < 6; ++k) {
            CHECK(c[i][k] >= 0.0f);
            CHECK(c[i][k] <= 1.0f);
        }
    }
}

TEST_CASE("Orientacion: las escaleras se unen en esquina exterior e interior, y se sueltan al quitar la vecina") {
    for (int f = 0; f <= 2; ++f) for (int inv = 0; inv < 2; ++inv) for (int g = 0; g < 4; ++g) {
        INFO("forma " << f << " giro " << g << " inv " << inv);
        const BlockType yo = pieza(2, f, g, inv != 0);
        int fx = 0, fz = 0;
        direccionDeGiro(g, fx, fz);
        BlockType delante = BLOCK_AIR, detras = BLOCK_AIR;
        auto get = [&](int dx, int, int dz) -> BlockType {
            if (dx == fx && dz == fz) return delante;
            if (dx == -fx && dz == -fz) return detras;
            return BLOCK_AIR;
        };
        CHECK(unionDeEscalera(yo, get) == RECTA);
        // Delante, otra girada un cuarto: esquina exterior (de otro material tambien vale).
        delante = pieza(7, f, giroIzquierda(g), inv != 0);
        CHECK(unionDeEscalera(yo, get) == EXTERIOR_IZQ);
        delante = pieza(7, f, (g + 1) & 3, inv != 0);
        CHECK(unionDeEscalera(yo, get) == EXTERIOR_DER);
        // Las que no forman esquina no se unen: misma direccion, la contraria,
        // otra mitad, otro numero de peldanos, en diagonal, o un bloque cualquiera.
        delante = pieza(7, f, g, inv != 0);                     CHECK(unionDeEscalera(yo, get) == RECTA);
        delante = pieza(7, f, (g + 2) & 3, inv != 0);           CHECK(unionDeEscalera(yo, get) == RECTA);
        delante = pieza(7, f, (g + 1) & 3, inv == 0);           CHECK(unionDeEscalera(yo, get) == RECTA);
        delante = pieza(7, (f + 1) % 3, (g + 1) & 3, inv != 0); CHECK(unionDeEscalera(yo, get) == RECTA);
        delante = pieza(7, f, (g + 1) & 3, inv != 0, true);     CHECK(unionDeEscalera(yo, get) == RECTA);
        delante = BLOCK_STONE;                                  CHECK(unionDeEscalera(yo, get) == RECTA);
        // Detras: esquina interior.
        delante = BLOCK_AIR;
        detras = pieza(7, f, giroIzquierda(g), inv != 0);
        CHECK(unionDeEscalera(yo, get) == INTERIOR_IZQ);
        detras = pieza(7, f, (g + 1) & 3, inv != 0);
        CHECK(unionDeEscalera(yo, get) == INTERIOR_DER);

        // La geometria: la exterior ocupa MENOS que la recta y la interior MAS.
        auto volumenDe = [&](Union u) {
            float c[8][6];
            const int n = cajasDeEscaleraUnida(yo, u, c);
            float v = 0;
            for (int i = 0; i < n; ++i) {
                for (int k = 0; k < 6; ++k) { CHECK(c[i][k] >= 0.0f); CHECK(c[i][k] <= 1.0f); }
                v += (c[i][3] - c[i][0]) * (c[i][4] - c[i][1]) * (c[i][5] - c[i][2]);
            }
            return v;
        };
        const float recta = volumenDe(RECTA);
        CHECK(volumenDe(EXTERIOR_IZQ) < recta - 0.05f);
        CHECK(volumenDe(EXTERIOR_DER) == doctest::Approx(volumenDe(EXTERIOR_IZQ)));
        CHECK(volumenDe(INTERIOR_IZQ) > recta + 0.05f);
        CHECK(volumenDe(INTERIOR_DER) == doctest::Approx(volumenDe(INTERIOR_IZQ)));

        // Al quitar la vecina vuelve a ser recta: no se guardo nada en la pieza.
        detras = BLOCK_AIR;
        CHECK(unionDeEscalera(yo, get) == RECTA);
        float a[8][6], b[4][6];
        const int n = cajasDePiezaEnMundo(yo, get, a);
        REQUIRE(n == cajasDePieza(yo, b));
        for (int i = 0; i < n; ++i) for (int k = 0; k < 6; ++k) CHECK(a[i][k] == b[i][k]);
    }
    // La esquina exterior de la escalera que sube al sur con la de delante
    // subiendo al este: lo alto se queda en el rincon sur-este (+Z, +X).
    float c[8][6];
    const int n = cajasDeEscaleraUnida(pieza(0, 0, 0), EXTERIOR_IZQ, c);
    REQUIRE(n == 2);
    CHECK(c[1][0] == 0.5f); CHECK(c[1][3] == 1.0f);
    CHECK(c[1][2] == 0.5f); CHECK(c[1][5] == 1.0f);
    // La interior: el tramo del fondo entero mas el cuarto de delante, del lado este.
    const int k = cajasDeEscaleraUnida(pieza(0, 0, 0), INTERIOR_IZQ, c);
    REQUIRE(k == 3);
    CHECK(c[2][0] == 0.5f); CHECK(c[2][3] == 1.0f);
    CHECK(c[2][2] == 0.0f); CHECK(c[2][5] == 0.5f);
    CHECK(c[2][1] == 0.5f); CHECK(c[2][4] == 1.0f);
    // Las demas formas no se unen nunca.
    CHECK_FALSE(escaleraQueSeUne(pieza(0, formaLlamada("PARED"))));
    CHECK_FALSE(escaleraQueSeUne(BLOCK_STONE));
}

TEST_CASE("Orientacion: el martillo recorre norte, sur, este, oeste, noreste, sureste, suroeste y noroeste") {
    static const char* const ORDEN[8] = { "NORTE", "SUR", "ESTE", "OESTE", "NORESTE", "SURESTE", "SUROESTE", "NOROESTE" };
    REQUIRE(NUM_ORIENTACIONES == 8);
    for (int o = 0; o < 8; ++o) CHECK(std::strcmp(nombreDeOrientacion(o), ORDEN[o]) == 0);
    for (int inv = 0; inv < 2; ++inv) {
        BlockType t = conOrientacionN(pieza(5, 1, 0, inv != 0), 0);
        std::set<int> vistas;
        for (int paso = 0; paso < 8; ++paso) {
            CHECK(orientacionDe(t) == paso);
            CHECK(conOrientacionN(t, paso) == t);              // ida y vuelta
            CHECK(materialDePieza(t) == 5);
            CHECK(formaDePieza(t) == 1);
            CHECK(invertidaDe(t) == (inv != 0));               // el martillo no la voltea
            CHECK(diagonalDe(t) == (paso >= 4));
            vistas.insert((int)t);
            t = siguienteOrientacion(t);
        }
        CHECK(vistas.size() == 8);                             // ocho estados distintos
        CHECK(orientacionDe(t) == 0);                          // y de la ultima, al norte otra vez
    }
    // El norte es -Z, el sur +Z, el este +X y el oeste -X.
    const int DX[4] = { 0, 0, 1, -1 }, DZ[4] = { -1, 1, 0, 0 };
    for (int o = 0; o < 4; ++o) {
        int dx = 0, dz = 0;
        direccionDeGiro(giroDePieza(conOrientacionN(pieza(0, 0), o)), dx, dz);
        CHECK(dx == DX[o]);
        CHECK(dz == DZ[o]);
    }
    // Una pared en diagonal: su centro, ya girado, cae hacia donde dice el nombre.
    const int EX[4] = { 1, 1, -1, -1 }, EZ[4] = { -1, 1, 1, -1 };       // NE, SE, SO, NO
    for (int o = 4; o < 8; ++o) {
        float c[4][6];
        REQUIRE(cajasDePieza(conOrientacionN(pieza(0, formaLlamada("PARED")), o), c) == 1);
        float x = (c[0][0] + c[0][3]) * 0.5f, z = (c[0][2] + c[0][5]) * 0.5f;
        puntoEnDiagonal(x, z);
        INFO(nombreDeOrientacion(o));
        CHECK((x - 0.5f) * EX[o - 4] > 0.15f);
        CHECK((z - 0.5f) * EZ[o - 4] > 0.15f);
    }
}

TEST_CASE("Orientacion: se choca con la forma girada; en diagonal, con tiras que siguen la pieza") {
    auto aire = [](int, int, int) { return BLOCK_AIR; };
    for (int f = 0; f < NUM; ++f) for (int g = 0; g < 4; ++g) for (int inv = 0; inv < 2; ++inv) {
        const BlockType t = pieza(0, f, g, inv != 0);
        float a[32][6], b[4][6];
        const int n = cajasDeChoque(t, aire, a, 32);
        REQUIRE(n == cajasDePieza(t, b));                      // sin diagonal: las mismas que se dibujan
        for (int i = 0; i < n; ++i) for (int k = 0; k < 6; ++k) CHECK(a[i][k] == b[i][k]);
    }
    // Una pared en diagonal: tiras dentro de la casilla, a toda su altura, que
    // siguen la pared y dejan libres los rincones de los lados.
    const BlockType pared = conOrientacionN(pieza(0, formaLlamada("PARED CENTRAL")), 4);
    float c[32][6];
    const int n = cajasDeChoque(pared, aire, c, 32);
    CHECK(n >= 4);
    CHECK(n <= TIRAS_DIAGONAL);
    float area = 0;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < 6; ++k) { CHECK(c[i][k] >= 0.0f); CHECK(c[i][k] <= 1.0f); }
        CHECK(c[i][1] == 0.0f); CHECK(c[i][4] == 1.0f);
        area += (c[i][3] - c[i][0]) * (c[i][5] - c[i][2]);
        // El centro de cada tira, deshecho el giro, cae dentro de la pared.
        const float x = (c[i][0] + c[i][3]) * 0.5f - 0.5f, z = (c[i][2] + c[i][5]) * 0.5f - 0.5f;
        const float fondo = 0.70710678f * (z - x);             // su z en la pieza sin girar
        CHECK(std::abs(fondo) < 0.2f);
    }
    CHECK(area < 0.6f);                                        // no es el cuadrado entero
    CHECK(area > 0.15f);
    // Y nunca desborda lo que se le deja escribir.
    float poco[3][6];
    CHECK(cajasDeChoque(pared, aire, poco, 3) <= 3);
}

TEST_CASE("Orientacion: se guarda en el propio bloque y los mundos de antes se leen igual") {
    // Todo el estado va en los 16 bits de la pieza: se guarda y se carga con el bloque.
    for (int m : { 0, 1, 22, NUM_MATERIALES - 1, 63 }) for (int f : { 0, 2, NUM - 1 })
        for (int g = 0; g < 4; ++g) for (int inv = 0; inv < 2; ++inv) for (int d = 0; d < 2; ++d) {
            const BlockType t = pieza(m, f, g, inv != 0, d != 0);
            CHECK(esPieza(t));
            CHECK(materialDePieza(t) == m);
            CHECK(formaDePieza(t) == f);
            CHECK(giroDePieza(t) == g);
            CHECK(invertidaDe(t) == (inv != 0));
            CHECK(diagonalDe(t) == (d != 0));
            // Lo que viaja en el inventario no lleva orientacion.
            const BlockType item = conGiro(t, 0);
            CHECK(giroDePieza(item) == 0);
            CHECK_FALSE(invertidaDe(item));
            CHECK_FALSE(diagonalDe(item));
        }
    // Una pieza guardada con el formato de antes (material en 7 bits, giro en
    // 13..14, nada en el 6 ni en el 15) es la MISMA pieza ahora.
    for (int m = 0; m < NUM_MATERIALES; ++m) for (int f = 0; f < NUM; ++f) for (int g = 0; g < 4; ++g) {
        const uint16_t viejo = (uint16_t)(m | (f << 7) | (g << 13));
        const BlockType t = Compuesto::hacer(Compuesto::FAM_FORMA, viejo);
        CHECK(t == pieza(m, f, g));
        CHECK_FALSE(invertidaDe(t));
        CHECK_FALSE(diagonalDe(t));
    }
}

TEST_CASE("Orientacion: colocar, la vista previa y el martillo usan las mismas funciones") {
    const std::string src = leerOri("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // Colocar (en casilla libre y juntando) y la vista previa: Formas::alColocar.
    CHECK(src.find("blockToPlace = Formas::alColocar(blockToPlace, direction.x, direction.z, result.normal.y, alturaApuntada);") != std::string::npos);
    CHECK(src.find("nuevo = Formas::alColocar(selectedBlock, direction.x, direction.z, result.normal.y, alturaApuntada);") != std::string::npos);
    CHECK(src.find("const BlockType comoQueda = Formas::alColocar(enManoV, rayDirection.x, rayDirection.z,") != std::string::npos);
    CHECK(src.find("Formas::conGiro(blockToPlace, Formas::giroDeMirada(") == std::string::npos);
    // La altura apuntada sale igual en los dos sitios.
    CHECK(src.find("alturaApuntada = origin.y + direction.y * result.distance - (float)result.blockPos.y;") != std::string::npos);
    CHECK(src.find("float alturaV = rayOrigin.y + rayDirection.y * rayResult.distance - (float)rayResult.blockPos.y;") != std::string::npos);
    // El martillo, con clic derecho, sobre una pieza.
    const size_t m = src.find("EL MARTILLO GIRA LA PIEZA");
    REQUIRE(m != std::string::npos);
    const std::string cuerpo = src.substr(m, 1500);
    CHECK(cuerpo.find("if (esMartillo(state->inventory.getSelectedBlock())) {") != std::string::npos);
    CHECK(cuerpo.find("const BlockType girada = Formas::siguienteOrientacion(tocada);") != std::string::npos);
    // Dibujar, apuntar y chocar parten de las mismas cajas, con las vecinas.
    CHECK(src.find("const int nCajas = Formas::cajasDePiezaEnMundo(block,") != std::string::npos);
    CHECK(src.find("if (enDiagonal) Formas::puntoEnDiagonal(px, pz);") != std::string::npos);
    CHECK(src.find("const int n = Formas::cajasDeChoque(type, get, c, MAX_PIEZAS_FORMA);") != std::string::npos);
    CHECK(src.find("const int k = Formas::cajasDeChoque(t, [&](int dx, int dy, int dz) {") != std::string::npos);
}
