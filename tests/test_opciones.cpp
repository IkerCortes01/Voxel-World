#include <doctest/doctest.h>
#include "ui/Opciones.h"
#include "ui/NombreItem.h"
#include <cstdio>
#include <string>

using namespace Opciones;

TEST_CASE("Opciones: son 16, en grupos de 5, 5, 3 y 3") {
    int n[NUM_GRUPOS] = {};
    for (int i = 0; i < NUM; ++i) ++n[def(i).grupo];
    CHECK(NUM == 16);
    CHECK(n[IMAGEN] == 5);
    CHECK(n[RENDER] == 5);
    CHECK(n[RENDIMIENTO] == 3);
    CHECK(n[ATMOSFERA_G] == 3);
}

TEST_CASE("Opciones: los valores por defecto dejan el juego como estaba") {
    Valores o;
    const LuzShader l = luzShader(o);
    CHECK(l.brillo == doctest::Approx(1.0f));
    CHECK(l.intensidad == doctest::Approx(1.0f));
    CHECK(l.saturacion == doctest::Approx(1.0f));
    CHECK(l.luzMinima == doctest::Approx(0.0f));
    CHECK(l.gamma == doctest::Approx(1.0f));
    CHECK(inicioNiebla(o) == doctest::Approx(0.0f));
    CHECK(pasosDeLuz(o) == 32);
    CHECK(limiteFps(o) == 0);
    CHECK(hzAnimales(o) == 0);
    CHECK(fpsMinimo(o) == 60);
    CHECK(radioAnimalesChunks(o) == doctest::Approx(8.0f));
    CHECK(o[OPTIMIZACION] == 1);
    CHECK(factorVelocidad(o[CARGA_CHUNKS]) == doctest::Approx(1.0f));
}

TEST_CASE("Opciones: el limite de FPS no baja de 60") {
    Valores o;
    for (int v = def(VELOCIDAD_RENDER).min; v <= def(VELOCIDAD_RENDER).max; ++v) {
        o.poner(VELOCIDAD_RENDER, v);
        const int l = limiteFps(o);
        CHECK((l == 0 || l >= 60));
    }
}

TEST_CASE("Opciones: las flechas y la barra no se salen del rango") {
    Valores o;
    for (int i = 0; i < NUM; ++i) {
        for (int k = 0; k < 40; ++k) o.siguiente(i);
        CHECK(o[i] >= def(i).min);
        CHECK(o[i] <= def(i).max);
        for (int k = 0; k < 40; ++k) o.anterior(i);
        CHECK(o[i] >= def(i).min);
        o.ponerFraccion(i, 2.0f);
        CHECK(o[i] <= def(i).max);
        o.ponerFraccion(i, -1.0f);
        CHECK(o[i] == def(i).min);
    }
}

TEST_CASE("Opciones: la oscuridad aclara por debajo de 50 y hunde por encima") {
    Valores o;
    o.poner(OSCURIDAD, 0);
    CHECK(luzShader(o).luzMinima > 0.0f);
    CHECK(luzShader(o).gamma == doctest::Approx(1.0f));
    o.poner(OSCURIDAD, 100);
    CHECK(luzShader(o).luzMinima == doctest::Approx(0.0f));
    CHECK(luzShader(o).gamma > 1.0f);
}

TEST_CASE("Opciones: todo lo que sale en pantalla se puede dibujar") {
    Valores o;
    for (int i = 0; i < NUM; ++i) {
        for (const char* p = def(i).nombre; *p; ++p)
            CHECK((NombreItem::caracterDibujable(*p) || *p == '%'));
        for (int v = def(i).min; v <= def(i).max; v += def(i).paso) {
            o.poner(i, v);
            for (char ch : o.texto(i))
                CHECK((NombreItem::caracterDibujable(ch) || ch == '%'));
        }
    }
    for (int g = 0; g < NUM_GRUPOS; ++g)
        for (const char* p = nombreGrupo(g); *p; ++p)
            CHECK(NombreItem::caracterDibujable(*p));
}

TEST_CASE("Opciones: se guardan y se cargan, menos la distancia") {
    Valores o;
    o.poner(BRILLO, 130);
    o.poner(VSYNC, 1);
    o.poner(SATURACION, 40);
    o.poner(DISTANCIA, 50);
    const std::string ruta = "opciones_prueba.txt";
    REQUIRE(guardar(o, ruta));
    Valores l;
    REQUIRE(cargar(l, ruta));
    std::remove(ruta.c_str());
    CHECK(l[BRILLO] == 130);
    CHECK(l[VSYNC] == 1);
    CHECK(l[SATURACION] == 40);
    CHECK(l[DISTANCIA] == def(DISTANCIA).porDefecto);
}
