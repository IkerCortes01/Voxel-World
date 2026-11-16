// EL ARRANQUE: QUE EL MENU ABRA AL INSTANTE.
// PEDIDO: "arregla que tarda mucho el juego en abrir el menu principal, que sea
// al instante".
//
// Habia dos cosas. La primera era un cuelgue: rellenar los huecos de una
// textura recorria la imagen entera por cada pixel vacio, y con una de
// 1286x1223 el juego no llegaba a abrir. La segunda, que todas las texturas y
// el terreno inicial se cargaban antes de ensenar el menu.
#include "doctest/doctest.h"

#include "render/RellenoTextura.h"

#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace RT = RellenoTextura;

namespace {
std::string leerA(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
void poner(std::vector<unsigned char>& im, int w, int x, int y, int r, int g, int b, int a) {
    unsigned char* p = im.data() + ((size_t)y * w + x) * 4;
    p[0] = (unsigned char)r; p[1] = (unsigned char)g; p[2] = (unsigned char)b; p[3] = (unsigned char)a;
}
} // namespace

TEST_CASE("Relleno: cada hueco toma el color del pixel opaco mas cercano") {
    const int W = 9, H = 5;
    std::vector<unsigned char> im((size_t)W * H * 4, 0);
    poner(im, W, 0, 2, 200, 10, 10, 255);       // rojo a la izquierda
    poner(im, W, 8, 2, 10, 10, 200, 255);       // azul a la derecha
    const std::vector<unsigned char> out = RT::rellenar(im.data(), W, H);
    REQUIRE(out.size() == im.size());
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const unsigned char* p = out.data() + ((size_t)y * W + x) * 4;
            INFO("pixel " << x << "," << y);
            CHECK(p[3] == 255);                 // ya no queda transparencia
            if (x <= 3) CHECK(p[0] == 200);     // mas cerca del rojo
            if (x >= 5) CHECK(p[2] == 200);     // mas cerca del azul
        }
    // Los opacos no se tocan.
    CHECK(out[((size_t)2 * W + 0) * 4 + 0] == 200);
    CHECK(out[((size_t)2 * W + 8) * 4 + 2] == 200);
}

TEST_CASE("Relleno: lo medio transparente tambien se rellena, y una imagen vacia se deja igual") {
    const int W = 4, H = 4;
    std::vector<unsigned char> im((size_t)W * H * 4, 0);
    poner(im, W, 1, 1, 50, 60, 70, 255);
    poner(im, W, 2, 2, 250, 250, 250, 120);     // borde difuminado: no cuenta como dibujo
    const std::vector<unsigned char> out = RT::rellenar(im.data(), W, H);
    const unsigned char* p = out.data() + ((size_t)2 * W + 2) * 4;
    CHECK(p[0] == 50);
    CHECK(p[3] == 255);
    const std::vector<unsigned char> vacia((size_t)W * H * 4, 0);
    CHECK(RT::rellenar(vacia.data(), W, H) == vacia);
    // Sin tamano no revienta.
    CHECK(RT::rellenar(vacia.data(), 0, 0).empty());
}

TEST_CASE("Relleno: una textura de 1286x1223 se rellena en un instante (antes colgaba el juego)") {
    // Lo que media el archivo que dejo el juego sin abrir: un tercio con
    // dibujo y el resto vacio. Con la busqueda de antes eran 2,5 billones de
    // pasos; ahora, uno por pixel.
    const int W = 1286, H = 1223;
    std::vector<unsigned char> im((size_t)W * H * 4, 0);
    for (int y = 400; y < 800; ++y)
        for (int x = 300; x < 900; ++x) poner(im, W, x, y, (x * 7) & 255, (y * 3) & 255, 90, 255);
    const auto t0 = std::chrono::steady_clock::now();
    const std::vector<unsigned char> out = RT::rellenar(im.data(), W, H);
    const double seg = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    CHECK(seg < 2.0);                           // de sobra: son milesimas
    CHECK(out[3] == 255);                       // la esquina, rellena
    CHECK(out[((size_t)(H - 1) * W + (W - 1)) * 4 + 3] == 255);
    // La esquina de arriba a la izquierda toma el color de la esquina del dibujo.
    CHECK(out[0] == ((300 * 7) & 255));
    CHECK(out[1] == ((400 * 3) & 255));
}

TEST_CASE("Relleno: lo enorme se reduce a 256 de lado antes de cargarlo") {
    CHECK(RT::divisorPara(16, 16) == 1);
    CHECK(RT::divisorPara(256, 256) == 1);
    CHECK(RT::divisorPara(512, 512) == 2);
    CHECK(RT::divisorPara(1286, 1223) == 6);    // 1286 / 6 = 214
    CHECK(RT::divisorPara(1402, 1122) == 6);
    for (int lado : { 257, 300, 1024, 1254, 2048, 4096 }) {
        const int d = RT::divisorPara(lado, lado);
        CHECK(lado / d <= RT::LADO_MAXIMO);
        if (d > 1) CHECK(lado / (d - 1) > RT::LADO_MAXIMO);        // ni un paso de mas
    }
    // Reducir a la mitad: cada pixel es la media de su bloque de 2x2, y lo
    // transparente no mancha el color.
    const int W = 4, H = 2;
    std::vector<unsigned char> im((size_t)W * H * 4, 0);
    poner(im, W, 0, 0, 100, 0, 0, 255); poner(im, W, 1, 0, 200, 0, 0, 255);
    poner(im, W, 0, 1, 100, 0, 0, 255); poner(im, W, 1, 1, 200, 0, 0, 255);
    poner(im, W, 2, 0, 0, 80, 0, 255);          // un opaco y tres vacios
    int w2 = 0, h2 = 0;
    const std::vector<unsigned char> ch = RT::reducir(im.data(), W, H, 2, w2, h2);
    REQUIRE(w2 == 2);
    REQUIRE(h2 == 1);
    CHECK(ch[0] == 150);
    CHECK(ch[3] == 255);
    CHECK(ch[4 + 1] == 80);                     // el verde entero, no un cuarto
    CHECK(ch[4 + 3] == 63);                     // y el alfa, el de uno de cuatro
    // Sin reducir, la devuelve tal cual.
    CHECK(RT::reducir(im.data(), W, H, 1, w2, h2) == im);
    CHECK(w2 == W);
}

TEST_CASE("Arranque: el menu se ensena antes de cargar texturas y terreno") {
    const std::string src = leerA("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // El relleno ya no es la busqueda de fuerza bruta.
    CHECK(src.find("RellenoTextura::rellenar(chica.data(), w, h);") != std::string::npos);
    CHECK(src.find("for (int sy = 0; sy < h; ++sy) {") == std::string::npos);
    // main() ya no carga las texturas ni genera el terreno antes del bucle.
    const size_t m = src.find("\nint main() {");
    REQUIRE(m != std::string::npos);
    const size_t bucle = src.find("while (!glfwWindowShouldClose(window))", m);
    REQUIRE(bucle != std::string::npos);
    const std::string antes = src.substr(m, bucle - m);
    CHECK(antes.find("g_cargaDeArranque.preparar();") != std::string::npos);
    CHECK(antes.find("g_textureManager->precargarTodasLasCaras();") == std::string::npos);
    CHECK(antes.find("\n    prewarmItemTextures();") == std::string::npos);
    CHECK(antes.find("world.generateInitialChunks(2, window);") == std::string::npos);
    CHECK(antes.find("world.buildAllPendingMeshes(window);") == std::string::npos);
    // Se reparte por cuadros en el menu, y se termina de golpe al salir de el:
    // a un mundo nunca se entra con texturas a medias.
    CHECK(src.find("if (g_gameState->screenState != SCREEN_MAIN_MENU) g_cargaDeArranque.todo();") != std::string::npos);
    CHECK(src.find("else if (++cuadrosVistos > 2) g_cargaDeArranque.paso(0.006);") != std::string::npos);
    // Y lo ultimo que hace es arrancar los hilos de trabajo, como antes.
    const size_t c = src.find("struct CargaDeArranque {");
    REQUIRE(c != std::string::npos);
    const std::string carga = src.substr(c, 5200);
    CHECK(carga.find("g_gameState->world.finishInitialGeneration();") != std::string::npos);
    CHECK(carga.find("prewarmItemTextures();") != std::string::npos);
    CHECK(carga.find("g_textureManager->precargarTodasLasCaras();") != std::string::npos);
}
