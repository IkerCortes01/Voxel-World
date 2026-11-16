// CELDAS JUNTAS (src/CeldasJuntas.h): varias cosas en la misma casilla.
// PEDIDO: "haz que todos los bloques modificados en la mesa de refineria no se
// repelen de los niveles de bloques y se junten sin predominar uno; quiero que
// cada nivel tenga su propia casilla, o sea, que si pones uno del nivel minimo
// de bloques no cuenta que la casilla esta totalmente ocupada, y que deje
// espacio para demas niveles, sin limite de combinarse y apilarse".
#include "doctest/doctest.h"

#include "CeldasJuntas.h"
#include "Formas.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

using namespace CeldasJuntas;

namespace {
std::string leerJ(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
Caja losa(float y0, float alto) { return { 0.0f, y0, 0.0f, 1.0f, y0 + alto, 1.0f }; }
} // namespace

TEST_CASE("Juntas: dos cajas que solo se tocan no se cruzan; las que se meten una en otra, si") {
    CHECK_FALSE(seCruzan(losa(0.0f, 0.125f), losa(0.125f, 0.125f)));     // apiladas
    CHECK(seCruzan(losa(0.0f, 0.25f), losa(0.125f, 0.25f)));             // una dentro de otra
    const Caja pared = { 0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f };
    const Caja otraPared = { 0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    CHECK_FALSE(seCruzan(pared, otraPared));                             // cada una a su lado
    CHECK(seCruzan(pared, losa(0.0f, 0.125f)));                          // la losa llega hasta la pared
}

TEST_CASE("Juntas: un nivel minimo no llena la casilla: lo siguiente se apila encima") {
    // En la casilla hay un nivel de un octavo. Otro nivel igual entra, y va
    // encima; y otro, y otro, hasta llenar la casilla.
    Caja puestas[16];
    int n = 0;
    puestas[n++] = losa(0.0f, 0.125f);
    const Caja nueva = losa(0.0f, 0.125f);
    for (int k = 1; k < 8; ++k) {
        const float y = buscarHueco(puestas, n, &nueva, 1, 0.125f);
        CHECK(y == doctest::Approx(0.125f * (float)k));
        puestas[n++] = losa(y, 0.125f);
    }
    // Llena hasta arriba: ya no cabe otro, y se dice (numero negativo).
    CHECK(buscarHueco(puestas, n, &nueva, 1, 0.125f) < 0.0f);
}

TEST_CASE("Juntas: una pieza y un nivel comparten casilla sin que mande ninguno") {
    // Un piso de la mesa (una losa fina abajo) y encima un nivel de tierra.
    const Caja piso = losa(0.0f, 0.1875f);              // 3 de 16
    const Caja nivel = losa(0.0f, 0.125f);
    float y = buscarHueco(&piso, 1, &nivel, 1, 0.125f);
    CHECK(y == doctest::Approx(0.1875f));               // el nivel se apoya en el piso
    // Al reves: el nivel estaba, y el piso se apoya en el.
    y = buscarHueco(&nivel, 1, &piso, 1, 0.1875f);
    CHECK(y == doctest::Approx(0.125f));
    // Un techo (losa arriba) deja libre el suelo: el nivel entra abajo, a ras.
    const Caja techo = losa(0.8125f, 0.1875f);
    CHECK(buscarHueco(&techo, 1, &nivel, 1, 0.125f) == doctest::Approx(0.0f));
    // Una pared ocupa su lado de arriba abajo: una losa de lado a lado la
    // atravesaria a cualquier altura, asi que no entra.
    const Caja pared = { 0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f };
    CHECK(buscarHueco(&pared, 1, &nivel, 1, 0.125f) < 0.0f);
    // Pero otra pared, en el lado de enfrente, si.
    const Caja paredEnfrente = { 0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    CHECK(buscarHueco(&pared, 1, &paredEnfrente, 1, 1.0f) == doctest::Approx(0.0f));
    // Lo que no cabe sin salirse por arriba, no entra.
    const Caja casiLlena = losa(0.0f, 0.95f);
    CHECK(buscarHueco(&casiLlena, 1, &nivel, 1, 0.125f) < 0.0f);
}

TEST_CASE("Juntas: el registro guarda lo juntado en orden y lo suelta de arriba abajo") {
    Registro r;
    CHECK_FALSE(r.tiene(1, 2, 3));
    CHECK(r.de(1, 2, 3).empty());
    CHECK(r.juntar(1, 2, 3, BLOCK_DIRT, 0.125f));
    CHECK(r.juntar(1, 2, 3, BLOCK_SAND, 0.25f));
    CHECK(r.juntar(-4, 70, 9, BLOCK_STONE, 0.0f));
    CHECK_FALSE(r.juntar(1, 2, 3, BLOCK_AIR, 0.0f));        // el aire no se junta
    CHECK(r.celdas() == 2);
    REQUIRE(r.de(1, 2, 3).size() == 2);
    CHECK(r.de(1, 2, 3)[0].tipo == BLOCK_DIRT);
    CHECK(r.de(1, 2, 3)[1].y0 == doctest::Approx(0.25f));
    // Romper quita lo ultimo que se puso; lo de debajo se queda.
    Junto j = r.quitarUltimo(1, 2, 3);
    CHECK(j.tipo == BLOCK_SAND);
    CHECK(r.de(1, 2, 3).size() == 1);
    j = r.quitarUltimo(1, 2, 3);
    CHECK(j.tipo == BLOCK_DIRT);
    CHECK_FALSE(r.tiene(1, 2, 3));                          // vacia: se olvida la casilla
    CHECK(r.quitarUltimo(1, 2, 3).tipo == BLOCK_AIR);
    // El primero, para cuando el bloque de la casilla desaparece.
    CHECK(r.quitarPrimero(-4, 70, 9).tipo == BLOCK_STONE);
    CHECK(r.celdas() == 0);
    // "Sin limite": el unico tope es de seguridad, y es alto.
    CHECK(MAX_POR_CELDA >= 16);
    for (size_t i = 0; i < MAX_POR_CELDA; ++i) CHECK(r.juntar(0, 0, 0, BLOCK_DIRT, 0.0f));
    CHECK_FALSE(r.juntar(0, 0, 0, BLOCK_DIRT, 0.0f));
}

TEST_CASE("Juntas: se guarda con la partida y vuelve igual; un archivo roto no la lia") {
    Registro r;
    r.juntar(10, 64, -3, BLOCK_DIRT, 0.1875f);
    r.juntar(10, 64, -3, BLOCK_SAND, 0.3125f);
    r.juntar(0, 5, 0, BLOCK_STONE, 0.0f);
    std::stringstream f(std::ios::in | std::ios::out | std::ios::binary);
    r.guardar(f);
    const std::string bytes = f.str();
    Registro leido;
    leido.juntar(9, 9, 9, BLOCK_DIRT, 0.0f);                // lo de otro mundo se va al cargar
    std::istringstream g(bytes, std::ios::binary);
    leido.cargar(g);
    CHECK(leido.celdas() == 2);
    CHECK_FALSE(leido.tiene(9, 9, 9));
    REQUIRE(leido.de(10, 64, -3).size() == 2);
    CHECK(leido.de(10, 64, -3)[0].tipo == BLOCK_DIRT);
    CHECK(leido.de(10, 64, -3)[0].y0 == doctest::Approx(0.1875f));
    CHECK(leido.de(10, 64, -3)[1].tipo == BLOCK_SAND);
    CHECK(leido.de(0, 5, 0)[0].tipo == BLOCK_STONE);
    // Cortado a la mitad: se queda con lo que leyo entero, sin reventar.
    for (size_t corte : { (size_t)0, (size_t)3, (size_t)10, bytes.size() / 2, bytes.size() - 1 }) {
        Registro medio;
        std::istringstream h(bytes.substr(0, corte), std::ios::binary);
        medio.cargar(h);
        CHECK(medio.celdas() <= 2);
    }
    // Un numero de casillas absurdo no reserva memoria.
    const uint32_t absurdo = 0xFFFFFFFFu;
    Registro malo;
    std::istringstream m(std::string(reinterpret_cast<const char*>(&absurdo), 4), std::ios::binary);
    malo.cargar(m);
    CHECK(malo.celdas() == 0);
}

TEST_CASE("Juntas: el motor junta, dibuja, deja apuntar, choca, rompe y guarda") {
    const std::string src = leerJ("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // Colocar: una pieza o un nivel, en la casilla que ya tiene una pieza o un nivel.
    CHECK(src.find("const float y0 = CeldasJuntas::buscarHueco(puestas, np, nuevas, nn, alto);") != std::string::npos);
    CHECK(src.find("if (!g_celdasJuntas.juntar(c.x, c.y, c.z, nuevo, y0)) continue;") != std::string::npos);
    // El apilado de niveles de siempre no se toca: aqui solo entra lo que se repelia.
    CHECK(src.find("if (!Formas::esPieza(nuevo) && !Formas::esPieza(hay) && juntos.empty()) continue;") != std::string::npos);
    // Apuntar y marcar: las cajas de lo juntado cuentan como las del bloque.
    CHECK(src.find("anadirJuntos(fm, x, y, z);") != std::string::npos);
    CHECK(src.find("    anadirJuntos(f, wx, wy, wz);") != std::string::npos);
    // Chocar.
    CHECK(src.find("g_worldQuery->setBlockBoxProvider(&cajaDeCeldaMundo);") != std::string::npos);
    // Romper: lo ultimo que se junto, y el resto se queda.
    CHECK(src.find("g_celdasJuntas.quitarUltimo(") != std::string::npos);
    // Dibujar.
    CHECK(src.find("for (const auto& [c, juntos] : g_celdasJuntas.todas()) {") != std::string::npos);
    // Guardar y cargar con el mundo, y vaciar al cambiar de mundo.
    CHECK(src.find("worldPath / \"celdas_juntas.dat\"") != std::string::npos);
    CHECK(src.find("if (f.is_open()) g_celdasJuntas.guardar(f);") != std::string::npos);
    CHECK(src.find("if (f.is_open()) g_celdasJuntas.cargar(f);") != std::string::npos);
    const size_t limpia = src.find("    g_celdasJuntas.clear();");
    const size_t carga = src.find("if (f.is_open()) g_celdasJuntas.cargar(f);");
    REQUIRE(limpia != std::string::npos);
    CHECK(limpia < carga);
}
