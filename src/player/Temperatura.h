#pragma once

#include <cmath>

// ============================================================================
// LA TEMPERATURA DEL JUGADOR
// ============================================================================
// PEDIDO: "un nuevo sistema de temperatura con una linea gruesa horizontal con
// 3 bolas [...] si estas en biomas calidos o lugares cerrados da calor; si lo
// contrario, es de noche, estas mojado, nieva [...] te dara frio. Niveles: el
// de enmedio Templado, luego ligero calor, poco calor, calor, mucho calor,
// demasiado calor, y ligero frio, poco frio, mucho frio, demasiado frio [...]
// cada uno con una diferencia de perspectiva en la camara, vision
// distorsionada, temblor y animaciones en el cuerpo".
//
// LA ESCALA: de -5 (demasiado frio) a +5 (demasiado calor), 0 templado. Son
// 11 niveles: al lado del frio se anade "frio" (-3) para que quede simetrico
// al "calor" (+3).
//
// DOS TEMPERATURAS:
//   AMBIENTE   la del sitio ahora: el bioma, la hora (de noche baja; al sol
//              del mediodia sube), la altura (cada 30 bloques sobre 90, un
//              punto menos), estar bajo techo (abriga: lleva el ambiente
//              hacia lo templado y quita el sol), la lluvia, la nieve, el
//              viento, estar mojado o en el agua, y el fuego y la lava cerca.
//   CUERPO     la del jugador: sigue al ambiente con inercia (~40 s; mojado,
//              el doble de rapido hacia el frio). Es la que se ve en la barra y
//              la que manda en los efectos.
//
// LOS EFECTOS (efectos()): el color de la pantalla (azul hacia el marino en
// el frio, naranja hacia el rojizo en el calor), el temblor de la camara y del
// cuerpo con el frio, la vista que ondula con el calor, la vineta en los
// extremos y un poco de cambio de campo de vision.
// ============================================================================

namespace TemperaturaJugador {

constexpr int NIVELES = 11;   // -5..+5

inline const char* nombreNivel(int n) {
    static const char* const N[NIVELES] = {
        "DEMASIADO FRIO", "MUCHO FRIO", "FRIO", "POCO FRIO", "LIGERO FRIO", "TEMPLADO",
        "LIGERO CALOR", "POCO CALOR", "CALOR", "MUCHO CALOR", "DEMASIADO CALOR",
    };
    const int i = n + 5;
    return N[i < 0 ? 0 : (i >= NIVELES ? NIVELES - 1 : i)];
}

struct Color { float r, g, b; };
// El color de cada nivel: del azul frio al azul marino, y del naranja al
// naranja rojizo; templado, blanco calido.
inline Color colorNivel(float t) {
    const Color templado{ 0.92f, 0.92f, 0.86f };
    const Color frio{ 0.45f, 0.75f, 1.00f }, marino{ 0.05f, 0.12f, 0.45f };
    const Color calor{ 1.00f, 0.72f, 0.30f }, rojizo{ 0.95f, 0.25f, 0.08f };
    auto mezcla = [](Color a, Color b, float k) { return Color{ a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k }; };
    if (t < 0.0f) {
        const float k = std::fmin(1.0f, -t / 5.0f);
        return k < 0.2f ? mezcla(templado, frio, k / 0.2f) : mezcla(frio, marino, (k - 0.2f) / 0.8f);
    }
    const float k = std::fmin(1.0f, t / 5.0f);
    return k < 0.2f ? mezcla(templado, calor, k / 0.2f) : mezcla(calor, rojizo, (k - 0.2f) / 0.8f);
}

struct Entorno {
    float bioma = 0.5f;            // 0 helado .. 1 muy caluroso (el clima del bioma)
    float hora = 12.0f;            // 0..24
    float altura = 64.0f;          // y de los pies
    float cielo = 1.0f;            // 0 bajo techo .. 1 a cielo abierto
    float lluvia = 0.0f, nieve = 0.0f, viento = 0.0f;   // 0..1, lo que cae aqui
    bool enAgua = false;
    float fuego = 0.0f;            // 0..1: fuego, lava u horno encendido cerca
    float nubes = 0.0f;            // 0..1: lo tapado que esta el sol
};

// La temperatura del sitio en la escala -5..+5.
inline float ambiente(const Entorno& e) {
    // El bioma: 0.5 templado; los extremos llegan a +-4.
    float t = (e.bioma - 0.5f) * 8.0f;
    // El dia: el sol del mediodia calienta, la madrugada enfria.
    const float dia = std::cos((e.hora - 14.0f) / 24.0f * 6.2831853f);   // 1 a las 14 h, -1 a las 2 h
    t += 1.6f * dia * (0.4f + 0.6f * e.cielo) * (dia > 0.0f ? (1.0f - 0.6f * e.nubes) : 1.0f);
    // La altura.
    if (e.altura > 90.0f) t -= (e.altura - 90.0f) / 30.0f;
    // Lo que cae y el viento (solo a cielo abierto).
    t -= e.cielo * (1.2f * e.lluvia + 2.2f * e.nieve + 1.0f * e.viento);
    // Bajo techo abriga: lleva hacia lo templado (pero no anula un desierto).
    t += (1.0f - e.cielo) * (t < 0.0f ? -t * 0.6f : -t * 0.25f + 0.4f);
    if (e.enAgua) t -= 2.0f;
    t += 4.5f * e.fuego;
    return t < -6.0f ? -6.0f : (t > 6.0f ? 6.0f : t);
}

struct Estado {
    float cuerpo = 0.0f;           // la temperatura del jugador (-5..+5)
    float mojado = 0.0f;           // 0..1: se seca en ~1 minuto
    float tiempo = 0.0f;
    void avanzar(float dt, const Entorno& e, float rapidez) {
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.25f) dt = 0.25f;
        tiempo += dt;
        // Mojado: en el agua o bajo la lluvia sin techo; se seca despacio
        // (mas rapido con calor o junto al fuego).
        const float moja = e.enAgua ? 1.0f : e.lluvia * e.cielo;
        if (moja > mojado) mojado += (moja - mojado) * (1.0f - std::exp(-dt / 3.0f));
        else mojado = std::fmax(0.0f, mojado - dt * (1.0f / 60.0f) * (1.0f + std::fmax(0.0f, cuerpo) * 0.4f + 3.0f * e.fuego));
        float meta = ambiente(e) - 2.0f * mojado * (e.enAgua ? 0.0f : 1.0f);
        // Moverse calienta un poco.
        meta += std::fmin(1.0f, rapidez / 6.0f) * 0.6f;
        const float tau = (meta < cuerpo ? 40.0f / (1.0f + mojado) : 40.0f);
        cuerpo += (meta - cuerpo) * (1.0f - std::exp(-dt / tau));
        if (cuerpo < -5.0f) cuerpo = -5.0f;
        if (cuerpo > 5.0f) cuerpo = 5.0f;
    }
    int nivel() const { return (int)std::lround(cuerpo); }
};

// ----------------------------------------------------------------------------
// LOS EFECTOS
// ----------------------------------------------------------------------------
struct Efectos {
    float tinte[4] = { 0, 0, 0, 0 };   // color y opacidad del velo de pantalla
    float vineta = 0.0f;               // 0..1: lo que se oscurecen los bordes
    float temblor = 0.0f;              // 0..1: la camara y el cuerpo tiritan
    float ondulacion = 0.0f;           // 0..1: la vista ondula (calor)
    float campo = 0.0f;                // grados de mas (+) o de menos (-) en el FOV
};

inline Efectos efectos(float t) {
    Efectos f;
    const float a = std::fabs(t);
    if (a < 0.6f) return f;
    const Color c = colorNivel(t);
    const float k = std::fmin(1.0f, (a - 0.5f) / 4.5f);
    f.tinte[0] = c.r; f.tinte[1] = c.g; f.tinte[2] = c.b;
    f.tinte[3] = 0.04f + 0.20f * k * k;
    f.vineta = a >= 3.5f ? std::fmin(1.0f, (a - 3.5f) / 1.5f) : 0.0f;
    if (t < 0.0f) {
        f.temblor = a >= 1.5f ? std::fmin(1.0f, (a - 1.5f) / 3.5f) : 0.0f;
        f.campo = -3.0f * k;           // con frio la vista se encoge
    } else {
        f.ondulacion = a >= 2.5f ? std::fmin(1.0f, (a - 2.5f) / 2.5f) : 0.0f;
        f.campo = 2.5f * k;            // con calor se abre y se marea
    }
    return f;
}

} // namespace TemperaturaJugador
