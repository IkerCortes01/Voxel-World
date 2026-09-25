#include <doctest/doctest.h>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <cstdio>

// ============================================================================
// TODA TEXTURA QUE EL CODIGO PIDE TIENE QUE EXISTIR
// ============================================================================
// BUG QUE ESTO PROTEGE: el maguey se veia "corrupto y sin textura".
//
// La causa era de las que no dan ningun aviso: las texturas se habian
// RENOMBRADO en disco (Maguei -> Maguey) pero el codigo seguia pidiendo los
// nombres viejos. getTexture() devolvia 0, y el mesher trata el 0 como
// "textura aun no cargada": marcaba el chunk para reintentar, para siempre.
// Resultado en pantalla: geometria sin textura, o directamente parches de
// piedra por el fallback.
//
// Cuatro nombres estaban rotos y el juego arrancaba sin quejarse.
//
// Este test lee el propio main.cpp, saca TODOS los nombres de textura que
// pide, y comprueba que cada archivo esta en disco. No hace falta arrancar
// OpenGL: es comparar cadenas contra el sistema de archivos.
//
// No es lo mismo que verificarlo a mano: esto corre en cada build, asi que un
// renombrado futuro se caza en el acto en vez de aparecer como "el bloque X
// se ve raro" semanas despues.

namespace {

// Lee un archivo entero. Devuelve vacio si no se puede abrir.
std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// ¿Existe este archivo?
bool existe(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    return (bool)f;
}

// Saca los argumentos de todas las llamadas a `getTexture("...")`.
std::vector<std::string> texturasPedidas(const std::string& fuente) {
    std::vector<std::string> out;
    const std::string marca = "getTexture(\"";
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

// La raiz del proyecto. Los tests corren desde build/, asi que se prueban
// varias rutas relativas hasta dar con el arbol.
std::string raizProyecto() {
    // CMake inyecta la ruta absoluta del arbol (ver tests/CMakeLists.txt).
    // Antes esto se adivinaba probando rutas relativas, y cuando fallaba los
    // tests se "omitian" y pasaban SIN COMPROBAR NADA -- falsa seguridad,
    // que es peor que un fallo.
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    return r;
#else
    return {};
#endif
}

} // namespace

TEST_CASE("Texturas: todas las que pide el codigo existen en disco") {
    const std::string raiz = raizProyecto();

    // Si esto falla, el test no puede comprobar nada: mejor que salte a que
    // pase en silencio dando falsa seguridad.
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    const std::vector<std::string> pedidas = texturasPedidas(fuente);

    // Si esto fuera 0, el test estaria pasando por no comprobar nada.
    REQUIRE(pedidas.size() > 20);

    const std::string dir = raiz + "resourcepacks/Textures/Blocks/";
    std::set<std::string> rotas;

    for (const std::string& t : pedidas) {
        // Las rutas con ../ salen de Blocks/ y se resuelven aparte.
        if (t.find("..") != std::string::npos) {
            const std::string rel = raiz + "resourcepacks/Textures/" +
                                    t.substr(t.find("../") + 3);
            if (!existe(rel)) rotas.insert(t);
            continue;
        }
        if (!existe(dir + t)) rotas.insert(t);
    }

    for (const std::string& r : rotas) {
        INFO("textura que el codigo pide pero NO existe: ", r);
        CHECK(false);
    }
    CHECK(rotas.empty());
}

TEST_CASE("Texturas: las del maguey estan y con el nombre correcto") {
    // El caso concreto del bug, fijado aparte para que se vea en el nombre
    // del test si vuelve a romperse.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string dir = raiz + "resourcepacks/Textures/Blocks/";
    const char* DEL_MAGUEY[] = {
        "Maguey.png",                  // el cuerpo de la planta
        "Puntas de Maguey.png",        // las espinas
        "Maguei por dentro.png",       // el cajete abierto
        "Tallo de Maguey en Pasto.png",
        "Tallo de Maguey en arena.png",
        "Aguamiel en el maguei.gif",   // el jugo
    };

    for (const char* t : DEL_MAGUEY) {
        INFO("textura del maguey: ", t);
        CHECK(existe(dir + t));
    }
}

TEST_CASE("Texturas: las del pasto y la tierra estan") {
    // ⭐ SE ROMPIO UNA VEZ, Y ASI ES COMO.
    //
    // Al sustituir las texturas de pasto, los .png originales se borraron y
    // en su sitio quedaron .jfif con otro nombre. El codigo sigue pidiendo los
    // nombres de siempre --son literales en getBlockTexture-- asi que el
    // mundo se habria quedado sin textura de pasto hasta que alguien mirara.
    //
    // No da error al compilar ni al arrancar: `getTexture` devuelve 0 y el
    // mesher lo interpreta como "aun no esta, reintenta", asi que el bloque
    // sale sin textura y el juego lo reintenta para siempre.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string dir = raiz + "resourcepacks/Textures/Blocks/";
    const char* DEL_SUELO[] = {
        // Pasto: cara de arriba, costado y sus variantes floridas.
        "Bloque de pasto up.png",
        "Bloque de pasto up 1v.png",
        "Bloque de pasto.png",
        "Bloque de pasto 1v.png",

        // La hierba corta. Es un SPRITE EN CRUZ, asi que ademas necesita
        // canal alfa -- ver el test de abajo.
        "Pasto corto.png",

        // Tierra y sus variantes.
        //
        // ⚠️ "Bloque de Tierra.png", y no "Tierra.png": el codigo pedia el
        // segundo en CINCO sitios y ese archivo nunca existio. La tierra
        // llevaba todo este tiempo sin textura, sin dar un solo error.
        "Bloque de Tierra.png",
        "Tierra arcillosa.png",
        "Tierra negra fertil.png",
    };

    for (const char* t : DEL_SUELO) {
        INFO("textura del suelo: ", t);
        CHECK(existe(dir + t));
    }
}

TEST_CASE("Texturas: la hierba corta tiene canal alfa") {
    // ⭐⭐ LO QUE UN .jpg NO PUEDE DAR.
    //
    // `Pasto corto` es un sprite en cruz: dos quads cruzados con la silueta de
    // la mata recortada. Si su imagen no tiene canal alfa, el recorte
    // desaparece y la hierba se dibuja como un CUADRADO VERDE opaco.
    //
    // Es justo lo que habria pasado al sustituirla por un JPEG, que no tiene
    // alfa por definicion. Por eso se convirtio a PNG con el fondo blanco
    // pasado a transparente.
    //
    // Se comprueba leyendo la cabecera del PNG: el byte 25 es el tipo de
    // color, y 6 = RGBA (4 = gris+alfa tambien valdria).
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string ruta =
        raiz + "resourcepacks/Textures/Blocks/Pasto corto.png";
    REQUIRE(existe(ruta));

    std::ifstream f(ruta, std::ios::binary);
    REQUIRE(f.is_open());

    unsigned char cab[26] = {0};
    f.read(reinterpret_cast<char*>(cab), sizeof(cab));

    // Firma PNG: 89 50 4E 47
    REQUIRE(cab[0] == 0x89);
    REQUIRE(cab[1] == 'P');
    REQUIRE(cab[2] == 'N');
    REQUIRE(cab[3] == 'G');

    const int tipoColor = (int)cab[25];
    INFO("tipo de color PNG = ", tipoColor, " (4 = gris+alfa, 6 = RGBA)");
    const bool tieneAlfa = (tipoColor == 4 || tipoColor == 6);
    CHECK(tieneAlfa);
}

TEST_CASE("Texturas: el codigo ya no pide los nombres viejos") {
    // Las texturas se renombraron de 'Maguei' a 'Maguey' y el codigo se quedo
    // con los viejos. Esto comprueba que no vuelvan a colarse.
    const std::string raiz = raizProyecto();
    REQUIRE_FALSE(raiz.empty());

    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE_FALSE(fuente.empty());

    // Estos nombres YA NO existen en disco.
    const char* MUERTOS[] = {
        "\"Maguei.png\"",
        "\"Puntas de Maguei.png\"",
        "\"Tallo de Maguei en Pasto.png\"",
        "\"Tallo de Maguei en arena.png\"",
    };

    for (const char* m : MUERTOS) {
        INFO("el codigo sigue pidiendo un nombre que no existe: ", m);
        CHECK(fuente.find(m) == std::string::npos);
    }
}
