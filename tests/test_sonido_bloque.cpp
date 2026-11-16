// LOS SONIDOS DE CADA BLOQUE (src/audio/SonidoBloque.h).
// PEDIDO: "mejora los sonidos y agrega 100 sonidos entrelazados de cada bloque
// al ser colocado, mientras es golpeado, y mejora los sonidos de pisadas del
// jugador a cada bloque, que sea unico y diferente cada bloque".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "audio/SonidoBloque.h"

#include <cmath>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <tuple>

using namespace Audio;

namespace {
std::string leerS(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
double energia(const SoundBuffer& b) {
    double e = 0.0;
    for (int16_t v : b.samples) e += (double)v * (double)v;
    return e;
}
} // namespace

TEST_CASE("Sonido: cada bloque tiene su voz, siempre la misma y distinta de las demas") {
    std::set<std::tuple<int, int, int>> vistas;
    for (uint32_t id = 1; id <= (uint32_t)BLOCK_TYPE_MAX; ++id) {
        const VozDeBloque a = vozDeBloque(id), b = vozDeBloque(id);
        // La misma cada vez.
        CHECK(a.tono == b.tono);
        CHECK(a.brillo == b.brillo);
        CHECK(a.cuerpo == b.cuerpo);
        // Dentro de lo que suena natural: ni pitidos ni truenos.
        CHECK(a.tono >= 0.84f);   CHECK(a.tono <= 1.20f);
        CHECK(a.brillo >= 0.78f); CHECK(a.brillo <= 1.22f);
        CHECK(a.cuerpo >= 0.70f); CHECK(a.cuerpo <= 1.30f);
        CHECK(a.peso >= 0.85f);   CHECK(a.peso <= 1.15f);
        CHECK(a.cola >= 0.85f);   CHECK(a.cola <= 1.20f);
        vistas.insert(std::make_tuple((int)(a.tono * 1000), (int)(a.brillo * 1000), (int)(a.cuerpo * 1000)));
    }
    // Unica: ningun otro bloque tiene el mismo tono, brillo y cuerpo.
    CHECK(vistas.size() == (size_t)BLOCK_TYPE_MAX);
    // Y las voces estan repartidas, no amontonadas en un rincon.
    float tMin = 9.0f, tMax = 0.0f;
    for (uint32_t id = 1; id <= (uint32_t)BLOCK_TYPE_MAX; ++id) {
        const float t = vozDeBloque(id).tono;
        if (t < tMin) tMin = t;
        if (t > tMax) tMax = t;
    }
    CHECK(tMax - tMin > 0.30f);
}

TEST_CASE("Sonido: cada material tiene 100 capas para colocar y 100 para golpear") {
    CHECK(SonidosDeBloque::CAPAS == 100);
    SonidosDeBloque s;
    for (int m = 0; m < (int)StepMaterial::COUNT; ++m)
        for (bool golpear : { false, true }) {
            INFO("material " << m << (golpear ? " golpear" : " colocar"));
            const auto& banco = s.banco((StepMaterial)m, golpear);
            REQUIRE(banco.size() == 100u);
            int partes[3] = { 0, 0, 0 };
            for (const auto& c : banco) {
                REQUIRE(c.parte >= 0);
                REQUIRE(c.parte <= 2);
                ++partes[c.parte];
                CHECK(c.s.size() > 100u);               // algo que oir
                CHECK(c.s.size() < 22050u / 4);         // y corto: menos de un cuarto de segundo
                for (float v : c.s) { REQUIRE(std::isfinite(v)); }
            }
            // Hay de las tres: golpe, cuerpo y cola.
            CHECK(partes[0] >= 15);
            CHECK(partes[1] >= 15);
            CHECK(partes[2] >= 15);
            // Pedirlo otra vez no lo vuelve a generar: es el mismo banco.
            CHECK(&s.banco((StepMaterial)m, golpear) == &banco);
        }
    // Las cien de un material no son la misma repetida.
    const auto& piedra = s.banco(StepMaterial::Stone, true);
    std::set<size_t> largos;
    for (const auto& c : piedra) largos.insert(c.s.size());
    CHECK(largos.size() > 40u);
    // Y golpear no es colocar.
    CHECK(SonidosDeBloque::capa(StepMaterial::Wood, true, 5).s != SonidosDeBloque::capa(StepMaterial::Wood, false, 5).s);
}

TEST_CASE("Sonido: componer da un sonido corto, sin saturar, y cada vez distinto") {
    SonidosDeBloque s;
    for (int m = 0; m < (int)StepMaterial::COUNT; ++m)
        for (GestoBloque g : { GestoBloque::COLOCAR, GestoBloque::GOLPEAR, GestoBloque::ROMPER }) {
            INFO("material " << m << " gesto " << (int)g);
            const SoundBuffer* b = s.componer(g, (StepMaterial)m, vozDeBloque(7), 0.7f);
            REQUIRE(b != nullptr);
            CHECK(b->sampleRate == AudioMixer::SAMPLE_RATE);
            CHECK(b->samples.size() > 300u);
            CHECK(b->samples.size() <= (size_t)(AudioMixer::SAMPLE_RATE * 0.43f));
            int pico = 0;
            for (int16_t v : b->samples) { const int a = v < 0 ? -v : v; if (a > pico) pico = a; }
            CHECK(pico > 6000);             // se oye
            CHECK(pico <= 32000);           // y no satura
            CHECK(b->samples.front() == 0); // empieza sin clic
            CHECK(std::abs((int)b->samples.back()) < 2500);
        }
    // Dos golpes seguidos al mismo bloque no son el mismo sonido.
    const SoundBuffer uno = *s.componer(GestoBloque::GOLPEAR, StepMaterial::Stone, vozDeBloque(1), 0.7f);
    const SoundBuffer dos = *s.componer(GestoBloque::GOLPEAR, StepMaterial::Stone, vozDeBloque(1), 0.7f);
    CHECK(uno.samples != dos.samples);
    // Romper dura mas que golpear: lleva el escombro detras.
    double largoGolpe = 0.0, largoRotura = 0.0;
    for (int k = 0; k < 12; ++k) {
        largoGolpe += (double)s.componer(GestoBloque::GOLPEAR, StepMaterial::Stone, vozDeBloque(1), 0.7f)->samples.size();
        largoRotura += (double)s.componer(GestoBloque::ROMPER, StepMaterial::Stone, vozDeBloque(1), 1.0f)->samples.size();
    }
    CHECK(largoRotura > largoGolpe * 1.15);
}

TEST_CASE("Sonido: el mismo golpe suena distinto en dos bloques del mismo material") {
    // Dos voces: una grave y apagada, otra aguda y brillante. Con las MISMAS
    // capas (mismo dado), el sonido cambia.
    VozDeBloque grave, aguda;
    grave.tono = 0.84f; grave.brillo = 0.78f;
    aguda.tono = 1.20f; aguda.brillo = 1.22f;
    SonidosDeBloque a, b;       // dos iguales: sacan las mismas capas en el mismo orden
    const SoundBuffer sg = *a.componer(GestoBloque::GOLPEAR, StepMaterial::Wood, grave, 0.7f);
    const SoundBuffer sa = *b.componer(GestoBloque::GOLPEAR, StepMaterial::Wood, aguda, 0.7f);
    CHECK(sg.samples != sa.samples);
    // La aguda va mas deprisa por las mismas capas: acaba antes.
    CHECK(sa.samples.size() < sg.samples.size());
    // Y un material blando no suena como uno duro.
    SonidosDeBloque c, d;
    const double piedra = energia(*c.componer(GestoBloque::GOLPEAR, StepMaterial::Stone, VozDeBloque(), 0.7f));
    const double arena = energia(*d.componer(GestoBloque::GOLPEAR, StepMaterial::Sand, VozDeBloque(), 0.7f));
    CHECK(piedra != arena);
}

TEST_CASE("Sonido: el motor los usa al colocar, al golpear, al romper y al pisar") {
    const std::string src = leerS("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("sonarBloque(Audio::GestoBloque::COLOCAR, blockToPlace, 0.60f);") != std::string::npos);
    CHECK(src.find("sonarBloque(Audio::GestoBloque::GOLPEAR, blockType, 0.50f);") != std::string::npos);
    CHECK(src.find("sonarBloque(Audio::GestoBloque::ROMPER, blockType, 0.80f);") != std::string::npos);
    // Ya no se pide el .wav que no existia.
    CHECK(src.find("g_soundManager->playPlaceBlock(glfwGetTime());") == std::string::npos);
    // La voz sale de la identidad del bloque: el nivel, la de su bloque; la pieza, la de su material.
    const size_t id = src.find("inline uint32_t idDeVoz(BlockType b) {");
    REQUIRE(id != std::string::npos);
    const std::string cuerpo = src.substr(id, 700);
    CHECK(cuerpo.find("if (Formas::esPieza(b)) b = Formas::materialBloque(b);") != std::string::npos);
    CHECK(cuerpo.find("if (esNivelParcial(b)) b = bloqueBaseDe(b);") != std::string::npos);
    // Las pisadas llevan el tono y el brillo del bloque que se pisa.
    CHECK(src.find("const Audio::VozDeBloque vozSuelo = Audio::vozDeBloque(idDeVoz(ground));") != std::string::npos);
    CHECK(src.find("mat, sprinting, crouching, vozSuelo.tono, vozSuelo.brillo);") != std::string::npos);
    const std::string pasos = leerS("src/audio/FootstepSystem.h");
    CHECK(pasos.find("corte *= brilloBloque;") != std::string::npos);
    CHECK(pasos.find("mixer->play(b, vol, pitch * tonoBloque);") != std::string::npos);
}
