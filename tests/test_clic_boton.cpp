// El mini sonido de los botones (src/audio/ClicBoton.h). Cabecera sin motor.
#include "doctest/doctest.h"

#include "audio/ClicBoton.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

using namespace Audio::ClicBoton;

TEST_CASE("ClicBoton: es corto, pequeno y no mete un pop") {
    const std::vector<int16_t> s = generar(22050);
    REQUIRE(s.size() > 600);
    CHECK(s.size() < 22050 / 10);              // menos de una decima de segundo
    int pico = 0;
    for (int16_t v : s) pico = (std::max)(pico, std::abs((int)v));
    CHECK(pico > 6000);                        // se oye
    CHECK(pico <= (int)PICO);                  // pero es un sonido pequeno
    CHECK(std::abs((int)s.front()) < 200);     // empieza en cero
    CHECK(std::abs((int)s.back()) < 200);      // y acaba en cero
    // La energia esta al principio: es un clic, no un pitido.
    long long antes = 0, despues = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        const long long e = (long long)s[i] * s[i];
        (i < s.size() / 3 ? antes : despues) += e;
    }
    CHECK(antes > despues * 2);
}

TEST_CASE("ClicBoton: siempre el mismo sonido") {
    CHECK(generar(22050) == generar(22050));
}

TEST_CASE("ClicBoton: solo suena con el raton sobre un boton, y no se repite pegado") {
    CHECK(debeSonar(10.00, 9.99, 0.0));        // el dibujo lo marco hace un frame
    CHECK_FALSE(debeSonar(10.00, 9.50, 0.0));  // hace medio segundo: ya no esta encima
    CHECK_FALSE(debeSonar(10.00, -1.0e9, 0.0)); // nunca hubo boton
    CHECK_FALSE(debeSonar(10.00, 9.99, 9.98)); // dos clics pegados suenan una vez
    CHECK(debeSonar(10.00, 9.99, 9.90));
}

TEST_CASE("ClicBoton: el motor lo llama desde el clic y desde cada tipo de boton") {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    REQUIRE(f.good());
    const std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(src.find("action == GLFW_PRESS) sonarClicDeBoton();") != std::string::npos);
    size_t n = 0;
    for (size_t p = src.find("ratonSobreBoton();"); p != std::string::npos;
         p = src.find("ratonSobreBoton();", p + 1)) ++n;
    CHECK(n >= 5);   // Button, botonDePausa, opciones (2) y avatar
}
