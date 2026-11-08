#pragma once
// ============================================================================
// LAS NUBES: CUMULOS DE BOLAS SUAVES QUE NACEN, SE JUNTAN Y SE DESHACEN
// ============================================================================
// PEDIDO: "un modelo 3D de nubes con una tecnologia que cambia de forma de
// forma aleatoria y elegante y no gaste recursos".
//
// PEDIDO 2: "que las nubes puedan juntarse y hacerse mas grandes o lo
// contrario, separarse tanto a ser pequenas, y que sean miles mas ... que
// solo llueva en nubes muy grandes, como en la vida real; donde no hay, no
// llovera".
//
// LA TECNICA. Un cumulo de verdad es un racimo de burbujas de aire humedo
// que suben, se hinchan y se deshacen. Aqui cada nube es eso mismo: un
// racimo de 1 a 12 BOLAS (esferas) con la base plana y la cima abombada.
// Cada bola se dibuja como un IMPOSTOR: un solo cuadro que mira a la camara,
// y el shader (NubesGL.h) calcula en cada pixel la normal de la esfera que
// tendria detras. Se ve redonda y con volumen desde cualquier lado, se
// ilumina con el sol de verdad, y cuesta CUATRO vertices por bola.
//
// ⭐ LA MASA DE NUBE: LO QUE LAS JUNTA Y LAS SEPARA.
//
// El cielo tiene un campo de "humedad" (`masa`, 0..1): dos octavas de ruido
// de valor, una de ~520 bloques y otra de ~190, que viajan con el viento y
// se transforman despacio con el tiempo (de 7 a 18 minutos). Cada celda del
// cielo (30 x 30 bloques) mira la masa que le toca:
//
//   MASA BAJA   pocas celdas con nube, y pequenas: nubecitas sueltas de una
//               o dos bolas, con mucho cielo entre ellas.
//   MASA ALTA   casi todas las celdas con nube, y GRANDES: cada una mide mas
//               que su celda, asi que se solapan con las vecinas y se leen
//               como una sola masa enorme, con torres en la cima.
//
// Como el campo se mueve y cambia sin parar, una zona de nubecitas se va
// llenando y sus nubes crecen hasta FUNDIRSE; y una masa grande se adelgaza
// y se ROMPE en trozos que se encogen. Nadie mueve nubes de un lado a otro:
// es el campo el que cambia, y las nubes lo siguen.
//
// ⭐ SOLO LLUEVE BAJO LAS GRANDES. `lluviaEn` dice cuanto llueve en un punto
// del suelo: nada si la masa encima es pequena, y todo bajo las masas mas
// grandes. El clima (Clima.h) decide QUE cae y con que fuerza; esto decide
// DONDE. Al salir de debajo de la nube se sale de la lluvia.
//
// MILES DE NUBES. La vista llega a 640 bloques: unas dos mil celdas. Las
// lejanas se dibujan con menos bolas, mas grandes (sin saltos: las que
// sobran se desvanecen poco a poco), asi que el coste sigue en unos miles
// de cuadros.
//
// SIN ESTADO. Todo sale de un hash de la celda del cielo y del reloj del
// mundo: no hay listas que crezcan, ni memoria que liberar, ni nada que
// guardar con el mundo. El mismo instante da siempre el mismo cielo.
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

constexpr float CELDA = 30.0f;          // una nube como mucho por celda
constexpr float RADIO_VISTA = 640.0f;   // mas alla no se dibujan
constexpr float LOD_DESDE = 260.0f;     // desde aqui, menos bolas y mas grandes
constexpr float LOD_HASTA = 360.0f;
constexpr float ALTURA = 190.0f;        // la base de las nubes (bloques)
constexpr float SOBRE_OJO = 35.0f;      // y siempre al menos esto por encima
constexpr float VIENTO_X = 1.10f;       // bloques por segundo de mundo
constexpr float VIENTO_Z = 0.40f;
constexpr int MAX_BOLAS_NUBE = 24;
constexpr int MAX_BOLAS = 8192;

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
    return 300.0f + 400.0f * azar(s);
}

// ----------------------------------------------------------------------------
// LA MASA DE NUBE
// ----------------------------------------------------------------------------
// Ruido de valor en 3D: (x, z) en el marco del aire y el tiempo como tercera
// dimension, asi el campo se TRANSFORMA en vez de solo desplazarse.
inline float valorEn(int64_t x, int64_t z, int64_t w) {
    const uint32_t h = mezclar((uint32_t)x * 0x8DA6B343u ^ mezclar((uint32_t)z * 0xD8163841u ^
                               mezclar((uint32_t)w * 0xCB1AB31Fu + 0x1B873593u)));
    return (float)(h >> 8) * (1.0f / 16777216.0f);
}
inline float ruido3(double x, double z, double w) {
    const double fx = std::floor(x), fz = std::floor(z), fw = std::floor(w);
    const int64_t ix = (int64_t)fx, iz = (int64_t)fz, iw = (int64_t)fw;
    float u = (float)(x - fx), v = (float)(z - fz), s = (float)(w - fw);
    u = u * u * (3.0f - 2.0f * u);
    v = v * v * (3.0f - 2.0f * v);
    s = s * s * (3.0f - 2.0f * s);
    float capa[2];
    for (int k = 0; k < 2; ++k) {
        const float a = valorEn(ix, iz, iw + k), b = valorEn(ix + 1, iz, iw + k);
        const float c = valorEn(ix, iz + 1, iw + k), d = valorEn(ix + 1, iz + 1, iw + k);
        capa[k] = (a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v;
    }
    return capa[0] + (capa[1] - capa[0]) * s;
}

// La humedad cruda del cielo en (ax, az) del marco del aire: 0..1, media 0.5.
inline float masaBruta(double ax, double az, double t) {
    const float a = ruido3(ax / 520.0, az / 520.0, t / 1080.0);
    const float b = ruido3(ax / 190.0 + 17.3, az / 190.0 - 5.1, t / 430.0 + 3.7);
    return 0.62f * a + 0.38f * b;
}

// La masa de nube, 0..1, segun cuanto cielo tapa el clima (`cob`) y lo
// tormentoso que es (`crece`). Con poca cobertura casi todo queda bajo y
// solo los picos del campo hacen nubes grandes; con mucha, casi todo el
// cielo es masa alta y quedan claros sueltos.
inline float masa(double ax, double az, double t, float cob, float crece = 0.0f) {
    const float u = 1.05f - cob * 0.85f - 0.20f * crece;
    return suave(u - 0.28f, u + 0.28f, masaBruta(ax, az, t));
}

// ⭐ CUANTO LLUEVE EN EL SUELO (wx, wz), 0..1: lo que tiene encima. Solo las
// masas grandes descargan; bajo nubecitas o cielo abierto, nada.
inline float lluviaEn(double wx, double wz, double t, float cob, float crece = 0.0f) {
    const float m = masa(wx - VIENTO_X * t, wz - VIENTO_Z * t, t, cob, crece);
    return suave(0.66f, 0.90f, m);
}

// Las bolas del cielo en el instante `t` (segundos de mundo), vistas desde el
// ojo en (ojoX, ojoY, ojoZ). Escribe como mucho `max` y devuelve cuantas.
//
// `cob` (0..1) es cuanto cielo tapan segun el CLIMA (ver Clima.h); con -1 se
// usa la cobertura propia de arriba. `crece` (0..1) es lo tormentoso: las
// nubes se hacen mas grandes y bajan, y acaban juntandose en un manto.
inline int generar(double t, double ojoX, double ojoY, double ojoZ,
                   Bola* out, int max, float cob = -1.0f, float crece = 0.0f) {
    int n = 0;
    // El viento mueve el cielo entero: se trabaja en el marco del aire.
    const double aireX = ojoX - VIENTO_X * t;
    const double aireZ = ojoZ - VIENTO_Z * t;
    const float base = (std::max)(SOBRE_OJO,
        (float)((std::max)((double)ALTURA, ojoY + SOBRE_OJO) - ojoY) - 30.0f * crece);
    const float tf = (float)std::fmod(t, 100000.0);   // para las ondas: sin perder precision
    const float cobAhora = cob >= 0.0f ? cob : cobertura(t);

    const int i0 = (int)std::floor((aireX - RADIO_VISTA) / CELDA) - 1;
    const int i1 = (int)std::floor((aireX + RADIO_VISTA) / CELDA) + 1;
    const int j0 = (int)std::floor((aireZ - RADIO_VISTA) / CELDA) - 1;
    const int j1 = (int)std::floor((aireZ + RADIO_VISTA) / CELDA) + 1;

    for (int i = i0; i <= i1; ++i) {
        for (int j = j0; j <= j1; ++j) {
            // Lejos de la vista: ni se calcula.
            const float gx = (float)((i + 0.5) * CELDA - aireX);
            const float gz = (float)((j + 0.5) * CELDA - aireZ);
            if (gx * gx + gz * gz > (RADIO_VISTA + 60.0f) * (RADIO_VISTA + 60.0f)) continue;

            const uint32_t sc = semillaCelda(i, j);
            // La GENERACION: cada celda tiene su propio reloj, desfasado, para
            // que las nubes no nazcan todas a la vez.
            const double vida = vidaDe(sc);
            uint32_t sf = sc ^ 0x51ED270Bu;
            const double desfase = azar(sf) * vida;
            const double g = std::floor((t + desfase) / vida);
            const float f = (float)((t + desfase) / vida - g);   // 0..1 de su vida
            uint32_t s = mezclar(sc ^ (uint32_t)(int64_t)g * 0x27D4EB2Fu);

            // ¿Hay nube en esta celda? La masa de su sitio decide: con poca,
            // solo algunas celdas; con mucha, casi todas. Cada celda tiene su
            // umbral y aparece (o se va) despacio al cruzarlo.
            const float m = masa((i + 0.5) * CELDA, (j + 0.5) * CELDA, t, cobAhora, crece);
            const float umbral = azar(s);
            const float presente = suave(umbral - 0.07f, umbral + 0.07f, 0.10f + 0.95f * m);
            if (presente <= 0.003f) continue;

            // Donde, en el marco del aire, y relativo al ojo.
            const float cx = (float)((i + 0.15 + 0.7 * azar(s)) * CELDA - aireX);
            const float cz = (float)((j + 0.15 + 0.7 * azar(s)) * CELDA - aireZ);
            const float dist = std::sqrt(cx * cx + cz * cz);
            if (dist > RADIO_VISTA + 40.0f) continue;

            // ⭐ EL TAMANO SIGUE A LA MASA. Con masa alta mide mas que su
            // celda y se funde con las vecinas; con masa baja, una nubecita.
            const float tam = (0.22f + 1.55f * m * m * (0.75f + 0.5f * azar(s))) * (1.0f + 0.5f * crece);
            // Lo que crece hacia arriba: las grandes echan torres.
            const float torre = suave(0.55f, 0.95f, m) * (0.5f + 0.5f * crece);
            const float largo = (16.0f + 12.0f * azar(s)) * tam;
            const float ancho = (8.0f + 7.0f * azar(s)) * tam;
            const float ang = 6.2831853f * azar(s);
            const float ca = std::cos(ang), sa = std::sin(ang);
            const float rMedio = (6.5f + 3.0f * azar(s)) * (0.60f + 0.5f * tam);
            // Muy encimadas: separadas un 60 % de su radio, se funden en una masa.
            const int nBase = std::clamp((int)(largo / (rMedio * 0.6f)) + 1, 1, 12);
            const int nCima = std::clamp(nBase / 3 + (int)(torre * 4.0f), 0, MAX_BOLAS_NUBE - nBase);
            // LAS LEJANAS: con la mitad de bolas, mas gordas. Las que sobran
            // se desvanecen con la distancia en vez de desaparecer de golpe.
            const float lod = suave(LOD_DESDE, LOD_HASTA, dist);
            // Se estira y se encoge entera, muy despacio.
            const float estira = 1.0f + 0.12f * std::sin(tf * 0.06f + 6.28f * azar(s));
            const float lejos = suave(RADIO_VISTA * 0.40f, RADIO_VISTA, dist);
            const float verse = 1.0f - suave(RADIO_VISTA * 0.72f, RADIO_VISTA, dist);
            // La base de las grandes, mas oscura: es la panza que descarga.
            const float panza = 1.0f - 0.30f * suave(0.6f, 1.0f, m) - 0.15f * crece;

            float rBase[MAX_BOLAS_NUBE];
            float uBase[MAX_BOLAS_NUBE];
            for (int k = 0; k < nBase + nCima && k < MAX_BOLAS_NUBE; ++k) {
                const bool cima = k >= nBase;
                float u, v, r;
                if (!cima) {
                    u = nBase == 1 ? 0.0f
                        : -1.0f + 2.0f * ((float)k + 0.5f) / (float)nBase + 0.25f * (azar(s) - 0.5f);
                    v = 0.7f * (azar(s) - 0.5f);
                    r = rMedio * (0.85f + 0.35f * azar(s)) * (1.0f - 0.40f * u * u);
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
                const float alto = azar(s);
                const float semilla = azar(s);

                // NACER Y MORIR: nace del centro hacia fuera, se deshace
                // desde los bordes.
                const float borde = std::fabs(u);
                const float nace = suave(0.02f + 0.10f * borde, 0.20f + 0.10f * borde, f);
                const float muere = 1.0f - suave(0.88f - 0.12f * borde, 1.0f - 0.08f * borde, f);
                // De lejos quedan una de cada dos de la base (las pares) y la
                // primera de la cima; las demas se apagan.
                const bool sobraLejos = cima ? (k > nBase) : (k % 2 == 1);
                const float lodAlfa = sobraLejos ? 1.0f - lod : 1.0f;
                const float vive = nace * muere * lodAlfa;
                if (vive <= 0.003f || verse <= 0.003f) continue;
                if (n >= max) return n;

                const float lx = u * largo * 0.5f * estira + meceX;
                const float lz = v * ancho;
                const float ly = cima ? r * 0.9f + rBase[std::clamp(nBase / 2, 0, nBase - 1)] * 0.45f
                                          + torre * r * 1.3f * alto
                                      : r * (0.30f + 0.15f * alto);
                Bola& o = out[n++];
                o.x = cx + lx * ca - lz * sa;
                o.z = cz + lx * sa + lz * ca;
                o.y = base + ly + meceY;
                o.radio = r * respira * (0.55f + 0.45f * nace * muere) * (0.6f + 0.4f * presente)
                        * (sobraLejos ? 1.0f : 1.0f + 0.30f * lod);
                o.alfa = vive * verse * presente;
                o.sombra = cima ? 1.0f : 0.80f * panza;
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
