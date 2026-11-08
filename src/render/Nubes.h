#pragma once
// ============================================================================
// LAS NUBES: CUMULOS DE BOLAS SUAVES QUE NACEN, RESPIRAN Y SE DESHACEN
// ============================================================================
// PEDIDO: "un modelo 3D de nubes con una tecnologia que cambia de forma de
// forma aleatoria y elegante y no gaste recursos".
//
// LA TECNICA. Un cumulo de verdad es un racimo de burbujas de aire humedo
// que suben, se hinchan y se deshacen. Aqui cada nube es eso mismo: un
// racimo de 6 a 16 BOLAS (esferas) con la base plana y la cima abombada.
// Cada bola se dibuja como un IMPOSTOR: un solo cuadro que mira a la camara,
// y el shader (NubesGL.h) calcula en cada pixel la normal de la esfera que
// tendria detras. Se ve redonda y con volumen desde cualquier lado, se
// ilumina con el sol de verdad, y cuesta CUATRO vertices por bola.
//
// EL CAMBIO DE FORMA, sin guardar nada:
//
//   RESPIRAR  cada bola crece y encoge a su ritmo (de 35 a 80 s por ciclo)
//             y se mece un poco: la nube nunca esta quieta, pero nunca da
//             tirones.
//   VIVIR     cada nube nace, dura unos minutos y se deshace. Nace desde el
//             centro hacia fuera y se deshace desde los bordes. Cuando muere,
//             en su sitio nace otra con una forma nueva, al azar.
//   EL TIEMPO cuantas nubes hay cambia despacio: ratos de cielo despejado y
//             ratos de cielo cubierto.
//   EL VIENTO las arrastra a todas juntas.
//
// SIN ESTADO. Todo sale de un hash de la celda del cielo y del reloj del
// mundo: no hay listas que crezcan, ni memoria que liberar, ni nada que
// guardar con el mundo. El mismo instante da siempre el mismo cielo. Por
// frame son unas 150-250 bolas, menos de mil vertices: nada, al lado del
// terreno.
//
// Este archivo no sabe nada de OpenGL: genera las bolas y los tests las
// comprueban. El dibujo esta en NubesGL.h.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Nubes {

// Una bola de nube, relativa al OJO del jugador (en bloques).
struct Bola {
    float x, y, z;
    float radio;
    float alfa;     // 0..1: lo que se ve (nace, muere, se aleja)
    float sombra;   // 0..1: la base de la nube es mas oscura que la cima
    float lejos;    // 0..1: cuanto se funde con el horizonte
    float semilla;  // 0..1: el dibujo de su borde deshilachado (ver NubesGL.h)
};

constexpr float CELDA = 72.0f;          // una nube como mucho por celda
constexpr float RADIO_VISTA = 240.0f;   // mas alla no se dibujan
constexpr float ALTURA = 190.0f;        // la base de las nubes (bloques)
constexpr float SOBRE_OJO = 35.0f;      // y siempre al menos esto por encima
constexpr float VIENTO_X = 1.10f;       // bloques por segundo de mundo
constexpr float VIENTO_Z = 0.40f;
constexpr int MAX_BOLAS_NUBE = 24;
constexpr int MAX_BOLAS = 1024;

inline uint32_t mezclar(uint32_t h) {
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}
inline uint32_t semillaCelda(int i, int j) {
    return mezclar((uint32_t)i * 0x9E3779B1u ^ mezclar((uint32_t)j * 0x85EBCA77u + 0x632BE5ABu));
}
// Un numero en [0, 1) que avanza la semilla.
inline float azar(uint32_t& s) {
    s = mezclar(s + 0x6D2B79F5u);
    return (float)(s >> 8) * (1.0f / 16777216.0f);
}
inline float suave(float a, float b, float x) {
    float t = (x - a) / (b - a);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

// Cuanto del cielo esta cubierto, de 0.2 (despejado) a 0.8 (casi cubierto).
// Dos ondas lentas y desacompasadas: el tiempo no se repite cada dia.
inline float cobertura(double t) {
    const double a = std::sin(t / 2300.0) * 0.6 + std::sin(t / 870.0 + 1.7) * 0.4;
    return 0.5f + 0.3f * (float)a;
}

// Cuanto dura una nube, en segundos de mundo (de nacer a deshacerse).
inline float vidaDe(uint32_t semilla) {
    uint32_t s = semilla ^ 0xA511E9B3u;
    return 260.0f + 260.0f * azar(s);
}

// Las bolas del cielo en el instante `t` (segundos de mundo), vistas desde el
// ojo en (ojoX, ojoY, ojoZ). Escribe como mucho `max` y devuelve cuantas.
inline int generar(double t, double ojoX, double ojoY, double ojoZ,
                   Bola* out, int max) {
    int n = 0;
    // El viento mueve el cielo entero: se trabaja en el marco del aire.
    const double aireX = ojoX - VIENTO_X * t;
    const double aireZ = ojoZ - VIENTO_Z * t;
    const float base = (float)((std::max)((double)ALTURA, ojoY + SOBRE_OJO) - ojoY);
    const float tf = (float)std::fmod(t, 100000.0);   // para las ondas: sin perder precision

    const int i0 = (int)std::floor((aireX - RADIO_VISTA) / CELDA) - 1;
    const int i1 = (int)std::floor((aireX + RADIO_VISTA) / CELDA) + 1;
    const int j0 = (int)std::floor((aireZ - RADIO_VISTA) / CELDA) - 1;
    const int j1 = (int)std::floor((aireZ + RADIO_VISTA) / CELDA) + 1;

    for (int i = i0; i <= i1; ++i) {
        for (int j = j0; j <= j1; ++j) {
            const uint32_t sc = semillaCelda(i, j);
            // La GENERACION: cada celda tiene su propio reloj, desfasado, para
            // que las nubes no nazcan todas a la vez.
            const double vida = vidaDe(sc);
            uint32_t sf = sc ^ 0x51ED270Bu;
            const double desfase = azar(sf) * vida;
            const double g = std::floor((t + desfase) / vida);
            const float f = (float)((t + desfase) / vida - g);   // 0..1 de su vida
            uint32_t s = mezclar(sc ^ (uint32_t)(int64_t)g * 0x27D4EB2Fu);

            // ¿Hay nube en esta generacion? Se decide con la cobertura de
            // cuando NACIO: durante su vida no aparece ni desaparece de golpe.
            if (azar(s) >= cobertura((g * vida - desfase))) continue;

            // Donde, en el marco del aire, y relativo al ojo.
            const float cx = (float)((i + 0.2 + 0.6 * azar(s)) * CELDA - aireX);
            const float cz = (float)((j + 0.2 + 0.6 * azar(s)) * CELDA - aireZ);
            const float dist = std::sqrt(cx * cx + cz * cz);
            if (dist > RADIO_VISTA + 40.0f) continue;

            // LA FORMA de esta nube, nueva en cada generacion.
            const float tam = 0.6f + 0.65f * azar(s);
            const float largo = (18.0f + 16.0f * azar(s)) * tam;
            const float ancho = (9.0f + 8.0f * azar(s)) * tam;
            const float ang = 6.2831853f * azar(s);
            const float ca = std::cos(ang), sa = std::sin(ang);
            // Muchas bolas y muy solapadas: asi no se lee "bolas", se lee nube.
            const int nBase = (std::min)(16, 7 + (int)(8.0f * tam * azar(s) + 3.0f * tam));
            const int nCima = (std::max)(2, nBase / 2);
            // Se estira y se encoge entera, muy despacio.
            const float estira = 1.0f + 0.12f * std::sin(tf * 0.06f + 6.28f * azar(s));
            const float lejos = suave(RADIO_VISTA * 0.45f, RADIO_VISTA, dist);
            const float verse = 1.0f - suave(RADIO_VISTA * 0.70f, RADIO_VISTA, dist);

            float rBase[MAX_BOLAS_NUBE];
            float uBase[MAX_BOLAS_NUBE];
            for (int k = 0; k < nBase + nCima && k < MAX_BOLAS_NUBE; ++k) {
                const bool cima = k >= nBase;
                float u, v, r;
                if (!cima) {
                    u = -1.0f + 2.0f * ((float)k + 0.5f) / (float)nBase + 0.25f * (azar(s) - 0.5f);
                    v = azar(s) - 0.5f;
                    r = (6.0f + 3.0f * azar(s)) * tam * (1.0f - 0.45f * u * u);
                    rBase[k] = r;
                    uBase[k] = u;
                } else {
                    // Las de la cima se apoyan sobre una de las del centro.
                    const int sobre = nBase / 2 + (int)((azar(s) - 0.5f) * (float)nBase * 0.5f);
                    const int b = std::clamp(sobre, 0, nBase - 1);
                    u = uBase[b] + 0.2f * (azar(s) - 0.5f);
                    v = 0.4f * (azar(s) - 0.5f);
                    r = rBase[b] * (0.65f + 0.2f * azar(s));
                }
                // RESPIRAR: su ritmo y su fase.
                const float w = 6.2831853f / (35.0f + 45.0f * azar(s));
                const float fase = 6.2831853f * azar(s);
                const float respira = 0.86f + 0.14f * std::sin(tf * w + fase);
                const float meceX = 1.4f * std::sin(tf * w * 0.7f + fase * 1.3f);
                const float meceY = 0.6f * std::sin(tf * w * 0.5f + fase * 0.7f);

                // NACER Y MORIR: nace del centro hacia fuera, se deshace
                // desde los bordes.
                const float borde = std::fabs(u);
                const float nace = suave(0.02f + 0.10f * borde, 0.20f + 0.10f * borde, f);
                const float muere = 1.0f - suave(0.88f - 0.12f * borde, 1.0f - 0.08f * borde, f);
                const float vive = nace * muere;
                if (vive <= 0.003f || verse <= 0.003f) continue;
                if (n >= max) return n;

                const float lx = u * largo * 0.5f * estira + meceX;
                const float lz = v * ancho;
                const float ly = cima ? r * 0.9f + rBase[std::clamp(nBase / 2, 0, nBase - 1)] * 0.45f
                                      : r * (0.30f + 0.15f * azar(s));
                const float semilla = azar(s);
                Bola& o = out[n++];
                o.x = cx + lx * ca - lz * sa;
                o.z = cz + lx * sa + lz * ca;
                o.y = base + ly + meceY;
                o.radio = r * respira * (0.55f + 0.45f * vive);
                o.alfa = vive * verse;
                o.sombra = cima ? 1.0f : 0.80f;
                o.lejos = lejos;
                o.semilla = semilla;
            }
        }
    }
    return n;
}

// De atras hacia delante: las lejanas primero, para que la mezcla de las
// transparencias salga bien.
inline void ordenar(Bola* b, int n) {
    std::sort(b, b + n, [](const Bola& a, const Bola& c) {
        return a.x * a.x + a.y * a.y + a.z * a.z > c.x * c.x + c.y * c.y + c.z * c.z;
    });
}

} // namespace Nubes
