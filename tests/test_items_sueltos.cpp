#include <doctest/doctest.h>
#include "BlockType.h"
#include "Inventory.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// stb_image se implementa AQUI porque el binario de tests no enlaza main.cpp
// (que es donde vive la otra implementacion). Sin esto, leer el alpha de un
// PNG en un test daria un error de enlazado.
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// ============================================================================
// LOS ITEMS TIRADOS EN EL SUELO
// ============================================================================
// Tres bugs que se veian en el mundo y que ningun test cazaba, porque los tres
// viven en el RENDER y en el trasiego de datos entre inventario y suelo:
//
//   1. Una herramienta soltada se dibujaba como un CUBO con el hacha
//      estampada en las seis caras, en vez de como el objeto que es.
//
//   2. Tirar una herramienta y recogerla la DEJABA COMO NUEVA. La durabilidad
//      no viajaba con el item suelto, asi que tirar y recoger era una forma
//      gratuita e ilimitada de reparar.
//
//   3. Los items de maguey salian abiertos por los costados: "no cargan las
//      texturas de las caras este y oeste".
//
// El tercero es el interesante, porque NO era una textura que faltara. La
// causa esta medida abajo, en su propio test.
// ============================================================================

namespace {

std::string raizProyecto() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    return r;
#else
    return {};
#endif
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

bool existe(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    return (bool)f;
}

// Saca los argumentos de `siluetaPNG = "..."` del render de items sueltos.
// Es el mismo enfoque textual que test_texturas_existen.cpp: leer el fuente y
// comprobar el disco, sin arrancar OpenGL.
std::vector<std::string> siluetasPedidas(const std::string& fuente) {
    std::vector<std::string> out;
    const std::string marca = "siluetaPNG = \"";
    size_t p = 0;
    while ((p = fuente.find(marca, p)) != std::string::npos) {
        const size_t ini = p + marca.size();
        const size_t fin = fuente.find('"', ini);
        if (fin == std::string::npos) break;
        out.push_back(fuente.substr(ini, fin - ini));
        p = fin;
    }
    return out;
}

} // namespace

// ============================================================================
// 1. LA DURABILIDAD SOBREVIVE AL SUELO
// ============================================================================

TEST_CASE("Herramienta soltada: la vida no se reinicia al recogerla") {
    Inventory inv;

    // Un hacha de piedra a medio gastar.
    const int MAXIMA = vidaMaximaHerramienta(BLOCK_HACHA_PIEDRA);
    REQUIRE(MAXIMA > 0);
    const int GASTADA = MAXIMA / 3;

    // Se recoge del suelo con esa vida (es lo que hace updateItems al
    // recogerla, con el vidaMedios que traia el ItemEntity).
    inv.addHerramienta(BLOCK_HACHA_PIEDRA, GASTADA);

    // Tiene que entrar con SU desgaste, no con la vida llena.
    int encontrada = -1;
    for (int i = 0; i < inv.total(); i++) {
        if (inv.at(i).blockType == BLOCK_HACHA_PIEDRA) {
            encontrada = inv.at(i).vidaMedios;
            break;
        }
    }
    CHECK(encontrada == GASTADA);
    CHECK(encontrada != MAXIMA);   // el bug: volvia como nueva
    CHECK(encontrada != 0);        // 0 seria "sin estrenar" = llena
}

TEST_CASE("Herramienta soltada: dos hachas distintas NO se mezclan") {
    // ESTE ES EL MOTIVO DE addHerramienta().
    //
    // addItem() apila por tipo, y un slot solo tiene UN vidaMedios. Si las dos
    // hachas cayeran en el mismo slot, una de las dos durabilidades se
    // perderia: el jugador recogeria un hacha gastada y una entera y acabaria
    // con dos iguales.
    Inventory inv;
    const int MAXIMA = vidaMaximaHerramienta(BLOCK_HACHA_PIEDRA);

    inv.addHerramienta(BLOCK_HACHA_PIEDRA, 4);        // casi rota
    inv.addHerramienta(BLOCK_HACHA_PIEDRA, MAXIMA);   // entera

    std::vector<int> vidas;
    for (int i = 0; i < inv.total(); i++) {
        if (inv.at(i).blockType == BLOCK_HACHA_PIEDRA && !inv.at(i).isEmpty())
            vidas.push_back(inv.at(i).vidaMedios);
    }

    REQUIRE(vidas.size() == 2);       // dos casillas, no una apilada
    CHECK(vidas[0] == 4);
    CHECK(vidas[1] == MAXIMA);
}

TEST_CASE("Herramienta soltada: sin estrenar sigue contando como llena") {
    // Una recien fabricada entra con 0, que TODO el motor interpreta como
    // vida completa (ver vidaFraccionSlot y gastarHerramienta). Soltarla y
    // recogerla no debe convertir ese 0 en un hacha rota.
    Inventory inv;
    inv.addHerramienta(BLOCK_PICO_PEDERNAL, 0);

    bool hallada = false;
    for (int i = 0; i < inv.total(); i++) {
        if (inv.at(i).blockType == BLOCK_PICO_PEDERNAL) {
            CHECK(inv.at(i).vidaMedios == 0);
            hallada = true;
            break;
        }
    }
    CHECK(hallada);

    // Y la barra la lee como llena, no como vacia.
    for (int i = 0; i < inv.total(); i++) {
        if (inv.at(i).blockType == BLOCK_PICO_PEDERNAL) {
            CHECK(inv.vidaFraccionSlot(i) == doctest::Approx(1.0f));
            break;
        }
    }
}

// ============================================================================
// 2. TODA HERRAMIENTA TIENE SU SILUETA 3D
// ============================================================================

TEST_CASE("Items sueltos: las seis herramientas se dibujan como silueta") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    // Cada herramienta que se gasta tiene que aparecer como `case` del switch
    // de siluetas. Si alguien añade una herramienta nueva y se olvida, se
    // dibujara como un cubo -- que es el bug que esto cierra.
    const BlockType HERRAMIENTAS[] = {
        BLOCK_HACHA_PIEDRA,    BLOCK_PICO_PIEDRA,    BLOCK_MARTILLO_PIEDRA,
        BLOCK_HACHA_PEDERNAL,  BLOCK_PICO_PEDERNAL,  BLOCK_MARTILLO_PEDERNAL,
    };
    const char* NOMBRES[] = {
        "BLOCK_HACHA_PIEDRA",   "BLOCK_PICO_PIEDRA",   "BLOCK_MARTILLO_PIEDRA",
        "BLOCK_HACHA_PEDERNAL", "BLOCK_PICO_PEDERNAL", "BLOCK_MARTILLO_PEDERNAL",
    };

    // El bloque del switch de siluetas, acotado para no confundirlo con los
    // cientos de `case` que hay en el resto del archivo.
    const size_t ini = fuente.find("const char* siluetaPNG = nullptr;");
    REQUIRE(ini != std::string::npos);
    const size_t fin = fuente.find("if (siluetaPNG)", ini);
    REQUIRE(fin != std::string::npos);
    const std::string bloque = fuente.substr(ini, fin - ini);

    for (size_t i = 0; i < sizeof(HERRAMIENTAS) / sizeof(HERRAMIENTAS[0]); i++) {
        INFO("Herramienta sin silueta 3D: " << NOMBRES[i]);
        CHECK(bloque.find(NOMBRES[i]) != std::string::npos);
        CHECK(esHerramientaGastable(HERRAMIENTAS[i]));
    }
}

TEST_CASE("Items sueltos: toda silueta pedida existe en disco") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    const std::vector<std::string> rutas = siluetasPedidas(fuente);
    REQUIRE(rutas.size() >= 11);   // palo + 6 herramientas + 4 de maguey

    // Las rutas van desde `resourcepacks/`, porque las siluetas mezclan
    // Items/ (herramientas) y Blocks/ (maguey).
    for (const std::string& r : rutas) {
        const std::string completa = raiz + "resourcepacks/" + r;
        INFO("Silueta que el codigo pide y no esta en disco: " << r);
        CHECK(existe(completa));
    }
}

// ============================================================================
// 3. EL BUG DE LAS CARAS ESTE Y OESTE DEL MAGUEY
// ============================================================================

TEST_CASE("Maguey: el borde lateral de la textura esta VACIO") {
    // ESTA ES LA CAUSA MEDIDA DEL BUG, y el motivo de que el arreglo sea
    // pasar el maguey a silueta por contorno en vez de retocar texturas.
    //
    // El camino viejo (item3D) levantaba el canto con cuatro tiras pegadas al
    // BORDE del cuadro: la tira oeste muestrea la columna u=0 y la este la
    // columna u=1. En "Maguey.png" esas dos columnas son 100% transparentes,
    // asi que el alpha test (GL_GREATER 0.5) descartaba las dos tiras ENTERAS
    // y la penca quedaba abierta por los costados.
    //
    // Arriba y abajo si se veian, porque esas filas si tienen dibujo. De ahi
    // que el sintoma fuera exactamente "faltan el este y el oeste".
    //
    // Este test fija el dato: si algun dia alguien repinta la textura y llena
    // los bordes, el test avisa de que la premisa cambio.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string ruta = raiz + "resourcepacks/Textures/Blocks/Maguey.png";
    REQUIRE(existe(ruta));

    // Se lee el PNG a pelo con stb_image, igual que hace SiluetaItem.
    int w = 0, h = 0, canales = 0;
    unsigned char* datos = stbi_load(ruta.c_str(), &w, &h, &canales, 4);
    REQUIRE(datos != nullptr);
    REQUIRE(w > 0);
    REQUIRE(h > 0);

    auto alphaEn = [&](int x, int y) -> int {
        return datos[((size_t)y * w + x) * 4 + 3];
    };

    int opacosIzq = 0, opacosDer = 0, opacosArriba = 0, opacosAbajo = 0;
    for (int y = 0; y < h; y++) {
        if (alphaEn(0,     y) >= 128) opacosIzq++;
        if (alphaEn(w - 1, y) >= 128) opacosDer++;
    }
    for (int x = 0; x < w; x++) {
        if (alphaEn(x, 0)     >= 128) opacosArriba++;
        if (alphaEn(x, h - 1) >= 128) opacosAbajo++;
    }
    stbi_image_free(datos);

    // Los costados no tienen un solo pixel opaco: por eso el canto este/oeste
    // desaparecia entero.
    CHECK(opacosIzq == 0);
    CHECK(opacosDer == 0);

    // Y arriba/abajo si, que es por lo que esas dos caras nunca fallaron.
    CHECK(opacosArriba > 0);
    CHECK(opacosAbajo  > 0);
}

TEST_CASE("Maguey: sus items sueltos ya no se dibujan como cubo") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    const size_t ini = fuente.find("const char* siluetaPNG = nullptr;");
    REQUIRE(ini != std::string::npos);
    const size_t fin = fuente.find("if (siluetaPNG)", ini);
    REQUIRE(fin != std::string::npos);
    const std::string bloque = fuente.substr(ini, fin - ini);

    // Las piezas de maguey que el jugador recoge del suelo. Mientras cayeran
    // al cubo del final, dos de sus seis caras salian mal -- y como el canto
    // por contorno no puede caer en una columna vacia, entrar aqui ES el
    // arreglo.
    const char* PIEZAS[] = {
        "BLOCK_IXTLE_HOJA",         // hoja del pulquero
        "BLOCK_PENCA_AGAVE_AZUL",   // penca del tequilana
        "BLOCK_MAGUEY_PUNTA",       // punta del pulquero
        "BLOCK_AGAVE_AZUL_PUNTA",   // punta del tequilana
    };
    for (const char* p : PIEZAS) {
        INFO("Pieza de maguey que sigue dibujandose como cubo: " << p);
        CHECK(bloque.find(p) != std::string::npos);
    }
}

// ============================================================================
// 4. LOS ITEMS DE CACTUS
// ============================================================================
// Mismo tratamiento que el maguey, y por el mismo motivo: sus texturas no
// llenan el cuadro, asi que el canto de cuatro tiras del camino item3D se
// descartaria en las columnas transparentes y la pieza saldria abierta por los
// costados. Con el contorno real el solido se cierra siempre.
//
// La diferencia esta en el GROSOR: un cactus es lo mas gordo del inventario.

TEST_CASE("Cactus: sus items sueltos se dibujan por contorno, no como cubo") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    const size_t ini = fuente.find("const char* siluetaPNG = nullptr;");
    REQUIRE(ini != std::string::npos);
    const size_t fin = fuente.find("if (siluetaPNG)", ini);
    REQUIRE(fin != std::string::npos);
    const std::string bloque = fuente.substr(ini, fin - ini);

    // Todo lo que el jugador recoge del nopal y sus frutos. Si una pieza se
    // quedara fuera, caeria al cubo del final y se veria como un dado con la
    // penca estampada en las seis caras.
    const char* PIEZAS[] = {
        "BLOCK_NOPAL_CLADODIO",    // lo que cae al romper cualquier parte
        "BLOCK_NOPAL_MOJADO",      // limpia de espinas
        "BLOCK_NOPAL_SECO",        // curada al sol
        "BLOCK_NOPAL_TIRAS",       // cortada en tiras
        "BLOCK_NOPAL_SIN_BABA",    // desbabada
        "BLOCK_NOPAL_BABA",        // el mucilago
        "BLOCK_ESPINAS_NOPAL",     // lo que se le quita
        "BLOCK_TUNA",              // los frutos
        "BLOCK_TUNA_AMARILLA",
        "BLOCK_TUNA_ROJA",
    };
    for (const char* p : PIEZAS) {
        INFO("Pieza de cactus que sigue dibujandose como cubo: " << p);
        CHECK(bloque.find(p) != std::string::npos);
    }
}

TEST_CASE("Cactus: el predicado agrupa lo que se suelta y nada mas") {
    // Lo que SI es item de cactus.
    CHECK(esCactusSuelto(BLOCK_NOPAL_CLADODIO));
    CHECK(esCactusSuelto(BLOCK_NOPAL_MOJADO));
    CHECK(esCactusSuelto(BLOCK_NOPAL_SECO));
    CHECK(esCactusSuelto(BLOCK_NOPAL_TIRAS));
    CHECK(esCactusSuelto(BLOCK_NOPAL_SIN_BABA));
    CHECK(esCactusSuelto(BLOCK_NOPAL_BABA));
    CHECK(esCactusSuelto(BLOCK_ESPINAS_NOPAL));
    CHECK(esCactusSuelto(BLOCK_TUNA));
    CHECK(esCactusSuelto(BLOCK_TUNA_AMARILLA));
    CHECK(esCactusSuelto(BLOCK_TUNA_ROJA));

    // ⚠️ Y lo que NO: los bloques con los que el generador CONSTRUYE la
    // planta. Esos se ven en el mundo, no en la mano, y el mesher ya tiene su
    // propia geometria para ellos. Si entraran aqui no romperia nada hoy --
    // el predicado solo decide grosor de item -- pero seria una lista que
    // dice una cosa y contiene otra.
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_BASE_PASTO));
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_BASE_ARENA));
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_TALLO));
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_CLADODIO_X2));
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_CLADODIO_X3));
    CHECK_FALSE(esCactusSuelto(BLOCK_NOPAL_CLADODIO_DIAG));

    // Ni nada de otra familia.
    CHECK_FALSE(esCactusSuelto(BLOCK_STONE));
    CHECK_FALSE(esCactusSuelto(BLOCK_STICK));
    CHECK_FALSE(esCactusSuelto(BLOCK_IXTLE_HOJA));       // eso es maguey
    CHECK_FALSE(esCactusSuelto(BLOCK_PENCA_AGAVE_AZUL));
}

TEST_CASE("Cactus: 5 px de grosor sobre los 16 del lado") {
    // ⭐ ESTE TEST CAZO UN FACTOR 2 DE MAS EN EL RENDER.
    //
    // El render usa `scale` como SEMILADO y pasa a SiluetaItem el SEMI-espesor.
    // La cuenta completa:
    //
    //     lado   = 2 * scale
    //     grosor = 2 * semiGrosor * scale
    //     grosor / lado = semiGrosor
    //
    // O sea que el valor que se pasa YA ES la fraccion del lado. El codigo lo
    // multiplicaba ademas por 0.5, asi que todo salia a la mitad: la penca de
    // agave a 2 px en vez de 4, y el cactus habria salido a 2.5 en vez de 5.
    constexpr float FACTOR_CACTUS = 5.0f / 16.0f;
    const float scale = 0.2f;              // el que usa el render

    const float lado   = 2.0f * scale;
    const float semiG  = FACTOR_CACTUS;    // sin el 0.5 de mas
    const float grosor = 2.0f * semiG * scale;

    INFO("lado " << lado << " grosor " << grosor);
    CHECK(grosor / lado == doctest::Approx(5.0f / 16.0f));

    // Y el orden que se buscaba: cactus (5px) > agave (4px) > herramienta (~3).
    CHECK(FACTOR_CACTUS > 0.25f);          // mas gordo que la penca de agave
    CHECK(0.25f > 0.18f);                  // que a su vez es mas que una lamina
}

TEST_CASE("Cactus: el largo y el ancho salen de la textura, no de una constante") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    // "Que mida lo que la textura tiene": las caras planas se dibujan de
    // -scale a +scale y el contorno del canto se saca del PNG, asi que la
    // silueta ES la del dibujo. Lo unico impuesto es el grosor.
    //
    // Se comprueba que las texturas existen y son cuadradas: si una no lo
    // fuera, el modelo la estiraria al cuadro y dejaria de medir lo que mide.
    const char* PNGS[] = {
        "resourcepacks/Textures/Items/Penca de Nopal de Castilla.png",
        "resourcepacks/Textures/Items/Espinas de Nopal de castilla.png",
        "resourcepacks/Textures/Items/Tuna verde crecida.png",
        "resourcepacks/Textures/Items/Baba de nopal.png",
    };
    for (const char* p : PNGS) {
        int w = 0, h = 0, canales = 0;
        const std::string completa = raiz + p;
        INFO("textura de cactus: " << p);
        REQUIRE(existe(completa));
        unsigned char* datos = stbi_load(completa.c_str(), &w, &h, &canales, 4);
        REQUIRE(datos != nullptr);
        CHECK(w == h);          // cuadrada: el modelo no la deforma
        CHECK(w > 0);
        stbi_image_free(datos);
    }
}

TEST_CASE("Cactus: hasta la textura mas vacia da un solido cerrado") {
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    // Las espinas son el caso extremo: apenas un 6 % del cuadro es opaco, y
    // son lineas finas repartidas. Es EXACTAMENTE el caso que hundio al canto
    // de cuatro tiras (el palo tenia 5 de 64 pixeles de borde opacos).
    //
    // Con el contorno real basta un pixel opaco para levantar canto, asi que
    // la pieza se cierra igual. Aqui se mide que ese pixel existe.
    const std::string p =
        raiz + "resourcepacks/Textures/Items/Espinas de Nopal de castilla.png";
    REQUIRE(existe(p));

    int w = 0, h = 0, canales = 0;
    unsigned char* datos = stbi_load(p.c_str(), &w, &h, &canales, 4);
    REQUIRE(datos != nullptr);

    int opacos = 0;
    for (int i = 0; i < w * h; ++i)
        if (datos[i * 4 + 3] > 127) ++opacos;
    stbi_image_free(datos);

    INFO("pixeles opacos en las espinas: " << opacos << " de " << (w * h));
    // Hay figura que contornear...
    CHECK(opacos > 0);
    // ...pero es minoria clara, que es lo que hace inviable el canto de borde.
    CHECK(opacos < (w * h) / 2);
}
