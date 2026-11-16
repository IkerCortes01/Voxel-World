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
    CHECK(src.find("Formas::alColocar(blockToPlace, direction.x, direction.z, result.normal.y, alturaApuntada)") != std::string::npos);
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

// BUG: "cuando estoy ya en las ultimas dos filas de inventario, todo se pone
// oscuro, el inventario y la zona de crafteo". El icono de una pieza encendia
// la textura y no la apagaba al salir.
TEST_CASE("Formas: el icono de una pieza apaga la textura al terminar") {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    const std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(src.empty());
    const size_t ini = src.rfind("void dibujarIconoDePieza(BlockType");
    REQUIRE(ini != std::string::npos);
    const size_t fin = src.find("void drawItemIcon(BlockType blockType, float cx, float cy, float size) {", ini);
    REQUIRE(fin != std::string::npos);
    const std::string cuerpo = src.substr(ini, fin - ini);
    const size_t enciende = cuerpo.find("glEnable(GL_TEXTURE_2D);");
    const size_t apaga = cuerpo.rfind("glDisable(GL_TEXTURE_2D);");
    REQUIRE(enciende != std::string::npos);
    REQUIRE(apaga != std::string::npos);
    CHECK(apaga > enciende);
}

// PEDIDO: "arregla todos los bugs de los bloques modificados en la mesa de
// refineria y anade que todos los bloques, no items, tengan la capacidad de ser
// modificados, y ademas que dependiendo de donde vea el jugador se colocara
// frente el bloque modificado, y arregla que donde un bloque modificado tenga
// vacio no contara como bloque completo y el jugador puede pasar libre".
TEST_CASE("Formas: todos los bloques enteros se pueden modificar, y los items no") {
    using namespace Formas;
    // Los troncos, tablones y hojas de todos los arboles, los techos, la
    // nieve, el vidrio y el mineral, ademas de los de siempre.
    for (BlockType b : { BLOCK_WOOD_OCOTE_CHINO, BLOCK_WOOD_PATULA, BLOCK_WOOD_PINONERO, BLOCK_WOOD_ALTURAS,
                         BLOCK_WOOD_PSEUDOSTROBUS, BLOCK_PLANKS_ALTURAS,
                         BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL, BLOCK_LEAVES_OCOTE,
                         BLOCK_LEAVES_OCOTE_CHINO, BLOCK_LEAVES_PATULA, BLOCK_LEAVES_PINONERO,
                         BLOCK_LEAVES_ALTURAS, BLOCK_LEAVES_PSEUDOSTROBUS,
                         BLOCK_TECHO_HOJAS, BLOCK_TECHO_SUELO, BLOCK_SNOW, BLOCK_GLASS, BLOCK_IRON_ORE }) {
        INFO("bloque " << (int)b);
        const int m = materialDe(b);
        REQUIRE(m >= 0);
        // La pieza hecha con el vuelve a el: sus texturas, su dureza y su sonido.
        CHECK(materialBloque(pieza(m, 0, 0)) == b);
        CHECK(materialBloque(pieza(m, NUM - 1, 3)) == b);
    }
    // Los items y lo que no es un bloque entero, no.
    for (BlockType b : { BLOCK_STICK, BLOCK_HILO_IXTLE, BLOCK_HACHA_PIEDRA, BLOCK_PALA_PIEDRA, BLOCK_PEDAZO_PIEDRA,
                         BLOCK_PEDAZO_OBSIDIANA, BLOCK_OBSIDIANA_AFILADA, BLOCK_TAZON_PINO, BLOCK_TALLGRASS,
                         BLOCK_FLOR_DALIA, BLOCK_PENCA_AGAVE_AZUL, BLOCK_WATER, BLOCK_LAVA })
        CHECK(materialDe(b) < 0);
    // Los 22 de antes siguen en su numero: las piezas ya puestas no cambian.
    CHECK(materialDe(BLOCK_PLANKS) == 0);
    CHECK(materialDe(BLOCK_WOOD) == 7);
    CHECK(materialDe(BLOCK_STONE) == 17);
    CHECK(materialDe(BLOCK_GRASS) == 21);
    // Y ninguno repetido.
    for (int m = 0; m < NUM_MATERIALES; ++m)
        for (int k = m + 1; k < NUM_MATERIALES; ++k)
            CHECK(bloqueDeMaterial(m) != bloqueDeMaterial(k));
}

TEST_CASE("Formas: la pieza se coloca de frente a donde mira el jugador") {
    using namespace Formas;
    // Una PARED tiene su tablero al fondo (+Z). Mirando a cada lado, el
    // tablero tiene que quedar al fondo de ESE lado.
    const int mat = materialDe(BLOCK_PLANKS);
    const int PARED = 1 * POR_CLASE;
    struct Caso { float dx, dz; int eje; bool alto; };       // eje 0 = X, 2 = Z; alto = pegada al 16
    const Caso CASOS[4] = {
        { 0.0f, -1.0f, 2, false },      // mira a -Z: el tablero en z 0..4
        { 0.0f,  1.0f, 2, true  },      // mira a +Z: en z 12..16
        { -1.0f, 0.0f, 0, false },      // mira a -X: en x 0..4
        { 1.0f,  0.0f, 0, true  },      // mira a +X: en x 12..16
    };
    for (const Caso& c : CASOS) {
        INFO("mirando a " << c.dx << ", " << c.dz);
        float cajas[4][6];
        const int n = cajasDePieza(pieza(mat, PARED, giroDeMirada(c.dx, c.dz)), cajas);
        REQUIRE(n == 1);
        const float lo = cajas[0][c.eje], hi = cajas[0][c.eje + 3];
        if (c.alto) { CHECK(hi == doctest::Approx(1.0f)); CHECK(lo > 0.5f); }
        else        { CHECK(lo == doctest::Approx(0.0f)); CHECK(hi < 0.5f); }
    }
    // En diagonal manda el lado hacia el que mas se mira.
    CHECK(giroDeMirada(0.9f, 0.3f) == giroDeMirada(1.0f, 0.0f));
    CHECK(giroDeMirada(-0.2f, -0.8f) == giroDeMirada(0.0f, -1.0f));
    // Coincide con el giro por angulo de antes (0 grados = mirando a -Z).
    CHECK(giroDeMirada(0.0f, -1.0f) == giroDeYaw(0.0f));
    CHECK(giroDeMirada(-1.0f, 0.0f) == giroDeYaw(90.0f));
    CHECK(giroDeMirada(0.0f, 1.0f) == giroDeYaw(180.0f));
    CHECK(giroDeMirada(1.0f, 0.0f) == giroDeYaw(270.0f));
    // La escalera sube alejandose: mirando a +Z, el peldano alto esta en +Z.
    float esc[4][6];
    REQUIRE(cajasDePieza(pieza(mat, 0, giroDeMirada(0.0f, 1.0f)), esc) == 2);
    CHECK(esc[1][1] > 0.4f);                // el segundo peldano es el alto...
    CHECK(esc[1][2] > 0.4f);                // ...y esta en la mitad de +Z
}

TEST_CASE("Formas: lo que mide de alto cada pieza") {
    using namespace Formas;
    const int mat = materialDe(BLOCK_STONE);
    CHECK(altoDePieza(pieza(mat, 2 * POR_CLASE + 0, 0)) == doctest::Approx(4.0f / 16.0f));     // piso
    CHECK(altoDePieza(pieza(mat, 2 * POR_CLASE + 1, 0)) == doctest::Approx(2.0f / 16.0f));     // piso fino
    CHECK(altoDePieza(pieza(mat, 1 * POR_CLASE + 0, 0)) == doctest::Approx(1.0f));             // pared
    CHECK(altoDePieza(pieza(mat, 3 * POR_CLASE + 0, 0)) == doctest::Approx(1.0f));             // techo: arriba del todo
    for (int f = 0; f < NUM; ++f) {
        const float h = altoDePieza(pieza(mat, f, 0));
        CHECK(h > 0.0f);
        CHECK(h <= 1.0f);
    }
}

TEST_CASE("Formas: se choca caja a caja; por donde la pieza esta vacia, se pasa") {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    auto leer = [&](const char* rel) {
        std::ifstream f(r + rel, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    };
    // El choque recorre TODAS las cajas del bloque, no la que las envuelve.
    const std::string col = leer("src/player/CollisionSystem.h");
    REQUIRE_FALSE(col.empty());
    CHECK(col.find("world->getBlockBoxes(x, y, z, cajas, IWorldQuery::MAX_CAJAS_POR_BLOQUE);") != std::string::npos);
    CHECK(col.find("for (int c = 0; c < n; ++c) {") != std::string::npos);
    CHECK(col.find("if (!world->getBlockBox(x, y, z, bx0, by0, bz0,") == std::string::npos);
    // El motor da las cajas de la pieza, una a una.
    const std::string src = leer("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("g_worldQuery->setBlockBoxesProvider(&cajasDeCeldaMundo);") != std::string::npos);
    const size_t fn = src.find("int cajasDeCeldaMundo(BlockType type, int x, int y, int z, float cajas[][6], int maximo) {");
    REQUIRE(fn != std::string::npos);
    const std::string cuerpo = src.substr(fn, 1400);
    CHECK(cuerpo.find("const int k = Formas::cajasDeChoque(t, [&](int dx, int dy, int dz) {") != std::string::npos);
    CHECK(cuerpo.find("g_celdasJuntas.de(x, y, z)") != std::string::npos);       // y lo juntado a la casilla
    // Las piezas que tienen hueco: se comprueba que el hueco existe de verdad
    // (que sus cajas no llenan el bloque), que es por donde se pasa.
    using namespace Formas;
    for (int f = 0; f < NUM; ++f) {
        INFO(forma(f).nombre);
        CHECK(volumen(f) < 4096);           // ninguna es el cubo entero
    }
    // La puerta: entre sus dos jambas queda libre de 4 a 12, del suelo a 12.
    const Forma& puerta = forma(1 * POR_CLASE + 8);
    REQUIRE(std::string(puerta.nombre) == "PARED CON PUERTA");
    for (int i = 0; i < puerta.n; ++i) {
        const Caja& c = puerta.c[i];
        const bool tapaElVano = c.x0 < 12 && c.x1 > 4 && c.y0 < 12 && c.y1 > 0;
        CHECK_FALSE(tapaElVano);
    }
}
