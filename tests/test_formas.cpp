// LA MESA DE REFINERIA Y SUS 50 FORMAS (src/Formas.h).
// PEDIDO: "implementa un nuevo bloque, el de Mesa de Refineria [...] bloques de
// madera y tierra y bloques completos en si te daran estas opciones: Escalera,
// Pared, Piso, Techo, Columna. Y tu creas 50 para cada item y bloque".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "BloqueCompuesto.h"
#include "Formas.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

using namespace Formas;

namespace {
std::string raizF() {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    return r;
}
std::string motorF() {
    std::ifstream f(raizF() + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Formas: son 50, diez de cada clase, con nombre propio") {
    CHECK(NUM == 50);
    CHECK(NUM_CLASES == 5);
    CHECK(std::string(nombreDeClase(ESCALERA)) == "ESCALERA");
    CHECK(std::string(nombreDeClase(PARED)) == "PARED");
    CHECK(std::string(nombreDeClase(PISO)) == "PISO");
    CHECK(std::string(nombreDeClase(TECHO)) == "TECHO");
    CHECK(std::string(nombreDeClase(COLUMNA)) == "COLUMNA");
    std::set<std::string> nombres;
    for (int i = 0; i < NUM; ++i) {
        const Forma& f = forma(i);
        INFO("forma " << i << ": " << f.nombre);
        CHECK(std::strlen(f.nombre) > 2);
        CHECK(std::strlen(f.nombre) <= 20);                 // cabe en su boton
        CHECK(nombres.insert(f.nombre).second);             // ninguna repetida
        // La fuente del juego: mayusculas, numeros y espacio.
        for (const char* q = f.nombre; *q; ++q)
            CHECK(((*q >= 'A' && *q <= 'Z') || (*q >= '0' && *q <= '9') || *q == ' '));
    }
    // La primera de cada clase es la que se pidio por su nombre.
    for (int c = 0; c < NUM_CLASES; ++c)
        CHECK(std::string(forma(c * POR_CLASE).nombre) == nombreDeClase(c));
}

TEST_CASE("Formas: todas las cajas caben en el bloque, tienen volumen y no son el cubo entero") {
    for (int i = 0; i < NUM; ++i) {
        const Forma& f = forma(i);
        INFO("forma " << i << ": " << f.nombre);
        CHECK(f.n >= 1);
        CHECK(f.n <= 4);
        for (int k = 0; k < f.n; ++k) {
            const Caja& c = f.c[k];
            CHECK(c.x0 < c.x1); CHECK(c.y0 < c.y1); CHECK(c.z0 < c.z1);
            CHECK(c.x1 <= 16); CHECK(c.y1 <= 16); CHECK(c.z1 <= 16);
        }
        CHECK(volumen(i) > 0);
        CHECK(volumen(i) < 4096);                           // menos que un bloque: es una forma
        CHECK(rinde(i) >= 1);
        CHECK(rinde(i) <= 8);
        CHECK(rinde(i) * volumen(i) <= 4096);               // no sale mas material del que entra
    }
    // Las cajas de una misma forma no se pisan (o el volumen saldria de mas).
    for (int i = 0; i < NUM; ++i) {
        const Forma& f = forma(i);
        for (int a = 0; a < f.n; ++a)
            for (int b = a + 1; b < f.n; ++b) {
                const Caja& p = f.c[a]; const Caja& q = f.c[b];
                const bool pisan = p.x0 < q.x1 && q.x0 < p.x1 && p.y0 < q.y1 && q.y0 < p.y1 &&
                                   p.z0 < q.z1 && q.z0 < p.z1;
                INFO("forma " << i << ": " << f.nombre);
                CHECK_FALSE(pisan);
            }
    }
}

TEST_CASE("Formas: cada clase es lo que dice ser") {
    // Un PISO toca el suelo; un TECHO, la cara de arriba; una COLUMNA sube; una
    // ESCALERA tiene mas de una altura (o es un peldano bajo).
    for (int k = 0; k < POR_CLASE; ++k) {
        const Forma& piso = forma(PISO * POR_CLASE + k);
        bool tocaSuelo = false;
        for (int i = 0; i < piso.n; ++i) if (piso.c[i].y0 == 0) tocaSuelo = true;
        INFO(piso.nombre);
        CHECK(tocaSuelo);
        const Forma& techo = forma(TECHO * POR_CLASE + k);
        bool arriba = false, suelo = false;
        for (int i = 0; i < techo.n; ++i) {
            if (techo.c[i].y1 == 16) arriba = true;
            if (techo.c[i].y0 == 0) suelo = true;
        }
        INFO(techo.nombre);
        CHECK(arriba);
        CHECK_FALSE(suelo);                                 // por debajo se pasa
        const Forma& col = forma(COLUMNA * POR_CLASE + k);
        int alto = 0;
        for (int i = 0; i < col.n; ++i) alto = (std::max)(alto, (int)col.c[i].y1);
        INFO(col.nombre);
        CHECK(alto >= 8);
        CHECK(col.c[0].y0 == 0);
    }
    // La escalera: un peldano a media altura y otro arriba, hacia atras.
    const Forma& e = forma(0);
    CHECK(e.n == 2);
    CHECK(e.c[0].y1 == 8);
    CHECK(e.c[1].y1 == 16);
    CHECK(e.c[1].z0 == 8);
    // De un bloque: 4 pisos y 4 columnas, pero una sola escalera.
    CHECK(rinde(PISO * POR_CLASE) == 4);
    CHECK(rinde(COLUMNA * POR_CLASE) == 4);
    CHECK(rinde(ESCALERA * POR_CLASE) == 1);
    CHECK(rinde(PARED * POR_CLASE) == 4);
    CHECK(rinde(TECHO * POR_CLASE) == 4);
}

TEST_CASE("Formas: girar da cuartos de vuelta, y cuatro vuelven al principio") {
    const Caja c = { 0, 0, 12, 16, 16, 16 };               // la pared, al fondo (+Z)
    const Caja g1 = girar(c, 1), g2 = girar(c, 2), g3 = girar(c, 3), g4 = girar(c, 4);
    CHECK(g1.x0 == 0);  CHECK(g1.x1 == 4);  CHECK(g1.z0 == 0); CHECK(g1.z1 == 16);    // a -X
    CHECK(g2.z0 == 0);  CHECK(g2.z1 == 4);  CHECK(g2.x0 == 0); CHECK(g2.x1 == 16);    // a -Z
    CHECK(g3.x0 == 12); CHECK(g3.x1 == 16);                                           // a +X
    CHECK(g4.x0 == c.x0); CHECK(g4.z0 == c.z0); CHECK(g4.x1 == c.x1); CHECK(g4.z1 == c.z1);
    CHECK(g1.y0 == c.y0); CHECK(g1.y1 == c.y1);             // la altura no cambia
    // Segun hacia donde mira quien la pone, la pieza queda de frente.
    CHECK(giroDeYaw(0.0f) == 2);      // mira a -Z: la escalera sube hacia -Z
    CHECK(giroDeYaw(180.0f) == 0);    // mira a +Z: sin girar
    CHECK(giroDeYaw(90.0f) == 1);
    CHECK(giroDeYaw(270.0f) == 3);
    CHECK(giroDeYaw(-90.0f) == 3);
    CHECK(giroDeYaw(360.0f + 10.0f) == 2);
}

TEST_CASE("Formas: 50 piezas por cada material, y la pieza recuerda de que esta hecha") {
    CHECK(NUM_MATERIALES >= 20);
    CHECK(NUM_MATERIALES <= 128);                           // caben en 7 bits
    std::set<int> vistos;
    for (int m = 0; m < NUM_MATERIALES; ++m) {
        const BlockType b = bloqueDeMaterial(m);
        CHECK(b != BLOCK_AIR);
        CHECK(vistos.insert((int)b).second);                // ninguno repetido
        CHECK(materialDe(b) == m);
        for (int f = 0; f < NUM; ++f)
            for (int g = 0; g < 4; ++g) {
                const BlockType t = pieza(m, f, g);
                CHECK(esPieza(t));
                CHECK(Compuesto::esCompuesto(t));
                CHECK(materialDePieza(t) == m);
                CHECK(formaDePieza(t) == f);
                CHECK(giroDePieza(t) == g);
                CHECK(materialBloque(t) == b);
            }
    }
    // Madera, tierra y piedra estan; lo que no es un bloque entero, no.
    for (BlockType b : { BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_WOOD, BLOCK_DIRT, BLOCK_SAND, BLOCK_STONE,
                         BLOCK_COBBLESTONE })
        CHECK(materialDe(b) >= 0);
    for (BlockType b : { BLOCK_AIR, BLOCK_STICK, BLOCK_NOPAL_FRUTO, BLOCK_HACHA_PIEDRA, BLOCK_MESA_REFINERIA,
                         BLOCK_TALLGRASS })
        CHECK(materialDe(b) < 0);
    CHECK_FALSE(esPieza(BLOCK_PLANKS));
    CHECK_FALSE(esPieza(Compuesto::AgaveRigido::nuevo(Compuesto::FAM_MAGUEY_ESPADIN, 2, 0)));
    // La pieza en el mundo: sus cajas giradas, en fracciones de bloque.
    float c[4][6];
    const BlockType escalera = pieza(materialDe(BLOCK_PLANKS), 0, 2);
    CHECK(cajasDePieza(escalera, c) == 2);
    CHECK(c[1][2] == doctest::Approx(0.0f));                // girada media vuelta: sube hacia -Z
    CHECK(c[1][5] == doctest::Approx(0.5f));
    CHECK(c[1][4] == doctest::Approx(1.0f));
    CHECK(conGiro(escalera, 0) == pieza(materialDe(BLOCK_PLANKS), 0, 0));
}

TEST_CASE("Mesa de refineria: el bloque existe, con nombre, y la pieza se llama como su forma") {
    CHECK((int)BLOCK_MESA_REFINERIA <= (int)BLOCK_TYPE_MAX);
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_MESA_REFINERIA), "MESA DE REFINERIA") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(pieza(0, 0, 0)), "ESCALERA") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(pieza(3, COLUMNA * POR_CLASE + 6, 1)), "COLUMNA CLASICA") == 0);
    CHECK(Compuesto::familiaTieneModelo(Compuesto::FAM_FORMA));
    CHECK((int)Compuesto::FAM_FORMA < (int)Compuesto::FAM_COUNT);
    for (const char* t : { "mesa de refineria arriba.png", "mesa de refineria de abajo.png",
                           "mesa de refineria de lado.png" }) {
        std::ifstream f(raizF() + "resourcepacks/Textures/Blocks/" + t, std::ios::binary);
        INFO("textura: " << t);
        CHECK(f.good());
    }
}

TEST_CASE("Mesa de refineria: el motor la fabrica, la abre, dibuja sus piezas y choca con ellas") {
    const std::string src = motorF();
    REQUIRE_FALSE(src.empty());
    // La receta: hacha, pedernal, pico / tablones, martillo, tablones / tres piedras.
    const size_t a = src.find("CraftingRecipe recipe(BLOCK_MESA_REFINERIA, 1, false);");
    REQUIRE(a != std::string::npos);
    const std::string receta = src.substr(a, 700);
    CHECK(receta.find("recipe.pattern[1] = BLOCK_PEDAZO_PEDERNAL;") != std::string::npos);
    CHECK(receta.find("recipe.pattern[4] = m;") != std::string::npos);
    CHECK(receta.find("recipe.pattern[8] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    // Clic derecho: se abre aunque la mano este vacia (antes de comprobar que hay algo que colocar).
    const size_t abre = src.find("abrirRefineria(state);");
    const size_t mano = src.find("if (!state->inventory.hasSelectedBlock()) return;");
    REQUIRE(abre != std::string::npos);
    REQUIRE(mano != std::string::npos);
    CHECK(abre < mano);
    // Las tres caras.
    CHECK(src.find("getTexture(\"mesa de refineria arriba.png\")") != std::string::npos);
    CHECK(src.find("getTexture(\"mesa de refineria de abajo.png\")") != std::string::npos);
    CHECK(src.find("getTexture(\"mesa de refineria de lado.png\")") != std::string::npos);
    // La pantalla: el panel solo con algo en la primera casilla; la herramienta se gasta, no se consume.
    CHECK(src.find("if (!r.entrada.isEmpty()) {") != std::string::npos);
    CHECK(src.find("\"REFINAR Y CREAR\"") != std::string::npos);
    CHECK(src.find("h.vidaMedios -= 2;") != std::string::npos);
    CHECK(src.find("if (g_refineria.abierta) {\r\n                dibujarRefineria(window, width, height);") +
          src.find("if (g_refineria.abierta) {\n                dibujarRefineria(window, width, height);") !=
          2 * std::string::npos);
    // Al cerrar no se pierde nada.
    CHECK(src.find("for (InventorySlot* s : { &g_refineria.entrada, &g_refineria.herramienta }) {") !=
          std::string::npos);
    // Las piezas: mismas cajas para dibujar y para chocar; se colocan de frente y devuelven la pieza.
    size_t veces = 0;
    for (size_t p = src.find("Formas::cajasDePieza("); p != std::string::npos;
         p = src.find("Formas::cajasDePieza(", p + 1)) ++veces;
    CHECK(veces >= 2);
    CHECK(src.find("Formas::conGiro(blockToPlace, Formas::giroDeYaw(state->player.yaw))") != std::string::npos);
    CHECK(src.find("state->spawnItem(pos, Formas::conGiro(blockType, 0));") != std::string::npos);
}

// PEDIDO: "haz que los iconos de los bloques que se modifican sean igual a la
// forma fisica al ser colocados, ademas que tengan las mismas propiedades de
// los tablones de madera enteros pero solo cambia su forma, hitbox y espacio, y
// que las paredes sean seleccionables sin problemas y tengan la misma duracion
// y dureza de la madera, y los demas igual".
TEST_CASE("Piezas: mismas propiedades que su material, se seleccionan por su forma y el icono es la forma") {
    const std::string src = motorF();
    REQUIRE_FALSE(src.empty());
    // Dureza y herramienta: las dos funciones de tiempo de rotura pasan por el material.
    const size_t a = src.find("float getBlockBreakTime(BlockType type) {");
    REQUIRE(a != std::string::npos);
    CHECK(src.substr(a, 900).find("if (Formas::esPieza(type)) type = Formas::materialBloque(type);") !=
          std::string::npos);
    const size_t b = src.find("float getBlockBreakTimeForMode(BlockType type, int gameMode,");
    REQUIRE(b != std::string::npos);
    CHECK(src.substr(b, 400).find("if (Formas::esPieza(type)) type = Formas::materialBloque(type);") !=
          std::string::npos);
    // Desgaste de la herramienta, particulas y pisadas, tambien.
    CHECK(src.find("desgasteHerramienta(herramienta, Formas::esPieza(blockType)") != std::string::npos);
    CHECK(src.find("pos, Formas::esPieza(blockType) ? Formas::materialBloque(blockType) : BLOCK_IXTLE_HOJA);") !=
          std::string::npos);
    CHECK(src.find("if (Formas::esPieza(b)) b = Formas::materialBloque(b);") != std::string::npos);
    // La seleccion: por las cajas de la forma, tenga una o varias.
    CHECK(src.find("if ((esCompartido(block) || Compuesto::esCompuesto(block)) && !Formas::esPieza(block)) {") !=
          std::string::npos);
    CHECK(src.find("if (fm.n > 1 || (fm.n == 1 && Formas::esPieza(block))) {") != std::string::npos);
    // El icono dibuja las cajas de la forma, no el cubo del material.
    CHECK(src.find("if (Formas::esPieza(blockType)) { dibujarIconoDePieza(blockType, cx, cy, size); return; }") !=
          std::string::npos);
    const size_t i = src.find("void dibujarIconoDePieza(BlockType pieza, float cx, float cy, float size) {");
    REQUIRE(i != std::string::npos);
    const std::string icono = src.substr(i, 2600);
    CHECK(icono.find("Formas::forma(Formas::formaDePieza(pieza))") != std::string::npos);
    CHECK(icono.find("Formas::materialBloque(pieza)") != std::string::npos);
}

TEST_CASE("Piezas: toda forma se puede tocar con un rayo, y la pared solo donde hay pared") {
    // Un rayo vertical por cada punto de una rejilla: toca la pieza si pasa por
    // alguna de sus cajas. Toda forma tiene donde tocarla, y ninguna es el
    // cubo entero (hay rayos que pasan de largo).
    for (int f = 0; f < NUM; ++f) {
        const Forma& fo = forma(f);
        int tocan = 0, pasan = 0;
        for (int x = 0; x < 16; ++x)
            for (int z = 0; z < 16; ++z)
                for (int y = 0; y < 16; ++y) {
                    bool dentro = false;
                    for (int k = 0; k < fo.n; ++k)
                        if (x >= fo.c[k].x0 && x < fo.c[k].x1 && y >= fo.c[k].y0 && y < fo.c[k].y1 &&
                            z >= fo.c[k].z0 && z < fo.c[k].z1) dentro = true;
                    dentro ? ++tocan : ++pasan;
                }
        INFO(fo.nombre);
        CHECK(tocan > 0);
        CHECK(pasan > 0);
        CHECK(tocan == volumen(f));
    }
    // La pared (forma 10) ocupa solo el fondo de su celda: delante queda libre.
    const Forma& pared = forma(PARED * POR_CLASE);
    CHECK(pared.c[0].z0 == 12);
    CHECK(pared.c[0].y1 == 16);
    CHECK(pared.c[0].x1 - pared.c[0].x0 == 16);
}
