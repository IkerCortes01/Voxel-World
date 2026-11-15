// El menu de pausa de 20 botones (src/ui/MenuPausa.h) y las estadisticas del
// jugador (src/Estadisticas.h). Los dos son cabeceras sin motor: aqui se
// prueba la cabecera de verdad, no una copia.
#include "doctest/doctest.h"

#include "ui/MenuPausa.h"
#include "Estadisticas.h"

#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace {

struct Ventana { int w, h; };
const Ventana VENTANAS[] = { {1280, 720}, {1920, 1080}, {1024, 600}, {800, 600}, {640, 480} };

bool seSolapan(const MenuPausa::Rect& a, const MenuPausa::Rect& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

} // namespace

TEST_CASE("menu de pausa: son 20 botones repartidos en 9 filas") {
    int suma = 0;
    for (int n : MenuPausa::POR_FILA) suma += n;
    CHECK(suma == 20);
    CHECK(MenuPausa::NUM == 20);
    // El pedido: uno arriba, y debajo dos filas de dos.
    CHECK(MenuPausa::POR_FILA[0] == 1);
    CHECK(MenuPausa::POR_FILA[1] == 2);
    CHECK(MenuPausa::POR_FILA[2] == 2);
}

TEST_CASE("menu de pausa: los botones caben en la ventana y no se pisan") {
    for (const Ventana& v : VENTANAS) {
        CAPTURE(v.w);
        CAPTURE(v.h);
        const MenuPausa::Disposicion d = MenuPausa::disponer(v.w, v.h);
        for (int i = 0; i < MenuPausa::NUM; ++i) {
            const MenuPausa::Rect& r = d.boton[i];
            CAPTURE(i);
            CHECK(r.w > 40.0f);
            CHECK(r.h >= 14.0f);
            CHECK(r.x >= 0.0f);
            CHECK(r.x + r.w <= (float)v.w);
            CHECK(r.y >= 0.0f);
            CHECK(r.y + r.h <= (float)v.h);
            for (int j = i + 1; j < MenuPausa::NUM; ++j) {
                CAPTURE(j);
                CHECK_FALSE(seSolapan(r, d.boton[j]));
            }
        }
        CHECK(d.tituloY >= 0.0f);
        CHECK(d.avisoY < (float)v.h);
    }
}

TEST_CASE("menu de pausa: volver a jugar va arriba y centrado; debajo, opciones y estadisticas") {
    const MenuPausa::Disposicion d = MenuPausa::disponer(1280, 720);
    const MenuPausa::Rect& volver = d.boton[MenuPausa::VOLVER];
    CHECK(volver.x + volver.w / 2.0f == doctest::Approx(640.0f));
    for (int i = 1; i < MenuPausa::NUM; ++i) CHECK(d.boton[i].y > volver.y);

    const MenuPausa::Rect& op = d.boton[MenuPausa::OPCIONES];
    const MenuPausa::Rect& es = d.boton[MenuPausa::ESTADISTICAS];
    CHECK(op.y == doctest::Approx(es.y));
    CHECK(op.x < es.x);
    // Logros y reportar bugs, justo debajo de esos dos.
    const MenuPausa::Rect& lo = d.boton[MenuPausa::LOGROS];
    const MenuPausa::Rect& bu = d.boton[MenuPausa::BUGS];
    CHECK(lo.y > op.y);
    CHECK(lo.y == doctest::Approx(bu.y));
    CHECK(lo.x == doctest::Approx(op.x));
    CHECK(bu.x == doctest::Approx(es.x));
}

TEST_CASE("menu de pausa: es pared de ladrillo (las juntas de una fila no caen sobre las de la siguiente)") {
    const MenuPausa::Disposicion d = MenuPausa::disponer(1280, 720);
    // Las juntas (borde derecho de cada boton menos el ultimo) de cada fila.
    std::vector<std::vector<float>> juntas;
    int b = 0;
    for (int f = 0; f < MenuPausa::NUM_FILAS; ++f) {
        std::vector<float> j;
        for (int i = 0; i < MenuPausa::POR_FILA[f]; ++i, ++b)
            if (i + 1 < MenuPausa::POR_FILA[f]) j.push_back(d.boton[b].x + d.boton[b].w);
        juntas.push_back(j);
    }
    // De la fila 3 en adelante se alternan 2 y 3: ninguna junta coincide.
    for (int f = 2; f + 1 < MenuPausa::NUM_FILAS; ++f)
        for (float a : juntas[f])
            for (float c : juntas[f + 1]) {
                CAPTURE(f);
                CHECK(std::fabs(a - c) > 40.0f);
            }
    // Y el de arriba tapa la junta de la fila de opciones/estadisticas.
    const MenuPausa::Rect& volver = d.boton[MenuPausa::VOLVER];
    REQUIRE(juntas[1].size() == 1);
    CHECK(juntas[1][0] > volver.x + 40.0f);
    CHECK(juntas[1][0] < volver.x + volver.w - 40.0f);
}

TEST_CASE("menu de pausa: el clic encuentra el boton que se ve") {
    for (const Ventana& v : VENTANAS) {
        const MenuPausa::Disposicion d = MenuPausa::disponer(v.w, v.h);
        for (int i = 0; i < MenuPausa::NUM; ++i) {
            const MenuPausa::Rect& r = d.boton[i];
            CHECK(MenuPausa::botonEn(d, r.x + r.w / 2.0f, r.y + r.h / 2.0f) == i);
        }
        CHECK(MenuPausa::botonEn(d, -5.0f, -5.0f) == -1);
    }
}

TEST_CASE("menu de pausa: la letra se encoge para que el rotulo quepa") {
    const MenuPausa::Rect r{ 0, 0, 200, 40 };
    CHECK(MenuPausa::letraQueCabe(r, 5, 18.0f) == doctest::Approx(18.0f));
    const float letra = MenuPausa::letraQueCabe(r, 30, 18.0f);
    CHECK(letra < 18.0f);
    CHECK(30 * letra * 0.7f <= r.w);
}

TEST_CASE("pantallas de la pausa: la lista reparte todos los elementos en paginas") {
    for (const Ventana& v : VENTANAS) {
        CAPTURE(v.w);
        CAPTURE(v.h);
        const MenuPausa::Listado l = MenuPausa::listar(v.w, v.h, Estadisticas::NUM);
        CHECK(l.porPagina == l.filas * l.columnas);
        CHECK(l.paginas * l.porPagina >= Estadisticas::NUM);
        CHECK((l.paginas - 1) * l.porPagina < Estadisticas::NUM);
        // La ultima fila no baja hasta los botones.
        CHECK(l.y0 + l.filas * l.pasoFila <= l.volver.y);
        CHECK(l.volver.y + l.volver.h <= (float)v.h);
        CHECK_FALSE(seSolapan(l.anterior, l.volver));
        CHECK_FALSE(seSolapan(l.volver, l.siguiente));
    }
    CHECK(MenuPausa::listar(1280, 720, 0).paginas == 1);
    CHECK(MenuPausa::paginaValida(-3, 4) == 0);
    CHECK(MenuPausa::paginaValida(9, 4) == 3);
    CHECK(MenuPausa::paginaValida(2, 4) == 2);
}

TEST_CASE("estadisticas: las seis pedidas mas noventa o mas inventadas, con claves unicas") {
    CHECK(Estadisticas::NUM >= 96);
    std::set<std::string> claves, nombres;
    for (int i = 0; i < Estadisticas::NUM; ++i) {
        const Estadisticas::Info& in = Estadisticas::info(i);
        CAPTURE(in.clave);
        CHECK(claves.insert(in.clave).second);
        CHECK(nombres.insert(in.nombre).second);
        CHECK(std::string(in.clave).find(' ') == std::string::npos);   // "clave valor" por linea
        // La fuente del juego solo tiene A-Z, 0-9, espacio y unos signos.
        for (char c : std::string(in.nombre)) {
            const bool vale = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '+';
            CHECK(vale);
        }
    }
}

TEST_CASE("estadisticas: se guardan y se leen por clave") {
    Estadisticas::Registro a;
    a.sumar(Estadisticas::CAMINADO, 123.5);
    a.contar(Estadisticas::BLOQUES_MINADOS);
    a.contar(Estadisticas::BLOQUES_MINADOS);
    a.record(Estadisticas::ALTURA_MAX, 90.0);
    a.record(Estadisticas::ALTURA_MAX, 70.0);      // no baja
    a.minimo(Estadisticas::ALTURA_MIN, 12.0);
    a.minimo(Estadisticas::ALTURA_MIN, 30.0);      // no sube

    Estadisticas::Registro b;
    b.leer(a.texto() + "clave_de_otra_version 7\nlinea rota\n\n");
    CHECK(b.v[Estadisticas::CAMINADO] == doctest::Approx(123.5));
    CHECK(b.v[Estadisticas::BLOQUES_MINADOS] == doctest::Approx(2.0));
    CHECK(b.v[Estadisticas::ALTURA_MAX] == doctest::Approx(90.0));
    CHECK(b.v[Estadisticas::ALTURA_MIN] == doctest::Approx(12.0));
    CHECK(b.tiene[Estadisticas::ALTURA_MAX]);
    CHECK_FALSE(b.tiene[Estadisticas::SALTOS]);     // lo que no paso no aparece
}

TEST_CASE("estadisticas: cada clase se escribe a su manera") {
    CHECK(Estadisticas::formato(Estadisticas::SALTOS, 0.0, false) == "-");
    CHECK(Estadisticas::formato(Estadisticas::SALTOS, 12.0) == "12");
    CHECK(Estadisticas::formato(Estadisticas::CAMINADO, 250.0) == "250 M");
    CHECK(Estadisticas::formato(Estadisticas::CAMINADO, 2500.0) == "2.50 KM");
    CHECK(Estadisticas::formato(Estadisticas::TIEMPO_JUGADO, 45.0) == "45 SEG");
    CHECK(Estadisticas::formato(Estadisticas::TIEMPO_JUGADO, 125.0) == "2 MIN 05 SEG");
    CHECK(Estadisticas::formato(Estadisticas::TIEMPO_JUGADO, 3725.0) == "1 H 02 MIN");
    CHECK(Estadisticas::formato(Estadisticas::SESION_LARGA, 95.0) == "1 MIN 35 SEG");
    CHECK(Estadisticas::formato(Estadisticas::RACHA_MINADO, 7.0) == "7");
}
