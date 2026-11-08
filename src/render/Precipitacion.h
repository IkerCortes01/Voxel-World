#pragma once
// ============================================================================
// LO QUE CAE DEL CIELO: LLUVIA, GRANIZO, NIEVE Y POLVO
// ============================================================================
// Las gotas no son particulas con vida propia: no se crean ni se destruyen.
// Hay un juego FIJO de gotas en una caja de 32 x 32 bloques alrededor del
// jugador, cada una con su sitio y su fase sacados de un hash, y su altura
// es el reloj: cae, llega abajo y vuelve a empezar arriba. La caja esta
// anclada a la rejilla del mundo (no al jugador): al andar, las gotas no se
// mueven con la camara, se pasa entre ellas.
//
//   LLUVIA   rayas finas, rapidas, inclinadas por el viento. Cuantas, segun
//            la intensidad: de una llovizna de 150 a un aguacero de 1400.
//   GRANIZO  bolitas blancas, mas rapidas todavia.
//   NIEVE    copos lentos que se mecen.
//   POLVO    rayas cortas y pardas casi horizontales: van con el viento.
//
// BAJO TECHO NO LLUEVE: una gota solo se dibuja por encima del bloque mas
// alto de su columna. Esa altura la da el motor (un callback), y se guarda en
// una cache de la caja que se renueva poco a poco, 64 columnas por frame.
//
// Sin OpenGL: escribe vertices en un arreglo y el motor los pinta.
// ============================================================================

#include <cmath>
#include <cstdint>

namespace Precipitacion {

constexpr int LADO = 32;            // la caja, en bloques
constexpr float ALTO = 26.0f;       // la columna de gotas, en bloques
constexpr float DEBAJO = 9.0f;      // cuanto baja por debajo del ojo
constexpr int MAX_GOTAS = 3200;

// Un trazo: de (x0,y0,z0) a (x1,y1,z1), con su color y opacidad. Para lo que
// se dibuja como punto (granizo, nieve) los dos extremos coinciden.
struct Trazo {
    float x0, y0, z0, x1, y1, z1;
    float r, g, b, a;
    int clase;          // 0 lluvia, 1 granizo, 2 nieve, 3 polvo
};

inline uint32_t hash(uint32_t h) {
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}
inline float azar(uint32_t h) { return (float)(hash(h) >> 8) * (1.0f / 16777216.0f); }

inline int modulo(int a, int m) { const int r = a % m; return r < 0 ? r + m : r; }

// Lo mas alto de cada columna de la caja: debajo de eso, nada cae. Se
// rellena poco a poco (`renovar`), asi el coste por frame es fijo y pequeno.
struct Techos {
    int alto[LADO][LADO];
    bool lista = false;
    int cursor = 0;

    template <class FnTecho>
    void renovar(int ojoX, int ojoZ, FnTecho techo, int columnas = 64) {
        if (!lista) {
            for (int i = 0; i < LADO; ++i)
                for (int j = 0; j < LADO; ++j) alto[i][j] = -100000;
            lista = true;
            columnas = LADO * LADO;   // la primera vez, entera
        }
        for (int k = 0; k < columnas; ++k) {
            const int idx = cursor;
            cursor = (cursor + 1) % (LADO * LADO);
            // La columna del mundo que cae en esta casilla de la caja.
            const int ci = idx / LADO, cj = idx % LADO;
            const int wx = ojoX - LADO / 2 + modulo(ci - (ojoX - LADO / 2), LADO);
            const int wz = ojoZ - LADO / 2 + modulo(cj - (ojoZ - LADO / 2), LADO);
            alto[ci][cj] = techo(wx, wz);
        }
    }
    // La altura (bloque mas alto + 1) de la columna del mundo (x, z).
    int en(int x, int z) const {
        return lista ? alto[modulo(x, LADO)][modulo(z, LADO)] : -100000;
    }
};

struct Entrada {
    double t;                    // segundos de mundo
    double ojoX, ojoY, ojoZ;
    float agua, granizo, nieve, polvo;   // 0..1, del clima
    float vientoX, vientoZ;      // hacia donde sopla, en bloques/s
    float luz;                   // 0..1: de noche las gotas apenas se ven
};

// Escribe los trazos (relativos al OJO) y devuelve cuantos. `techos` decide
// que gotas quedan tapadas.
inline int generar(const Entrada& e, const Techos& techos, Trazo* out, int max) {
    int n = 0;
    const float tf = (float)std::fmod(e.t, 10000.0);
    const float brillo = 0.25f + 0.75f * e.luz;

    struct Clase { float cuanto; int maximo; float velocidad; float largo; float r, g, b, a; uint32_t sal; };
    const Clase clases[4] = {
        // lluvia: rayas de 0,9 bloques que bajan a 16 bloques/s
        { e.agua,    1400, 16.0f, 0.9f, 0.70f, 0.76f, 0.86f, 0.55f, 0x1234567u },
        // granizo: bolitas, 22 bloques/s
        { e.granizo,  700, 22.0f, 0.0f, 0.92f, 0.95f, 1.00f, 0.90f, 0x2468aceu },
        // nieve: copos a 1,6 bloques/s
        { e.nieve,   1600,  1.6f, 0.0f, 0.97f, 0.98f, 1.00f, 0.85f, 0x369cf03u },
        // polvo: rayas pardas que viajan con el viento
        { e.polvo,    900,  0.6f, 0.0f, 0.70f, 0.56f, 0.38f, 0.50f, 0x48d159eu },
    };
    for (int c = 0; c < 4; ++c) {
        const Clase& k = clases[c];
        const int cuantas = (int)(k.cuanto * (float)k.maximo);
        for (int i = 0; i < cuantas && n < max; ++i) {
            const uint32_t s = (uint32_t)i * 0x9E3779B1u ^ k.sal;
            const float ox = azar(s) * LADO, oz = azar(s ^ 0x55u) * LADO;
            const float fase = azar(s ^ 0xAAu);
            // Cuanto ha caido: el reloj, y vuelve a empezar arriba.
            float caida = std::fmod(fase * ALTO + tf * k.velocidad * (0.85f + 0.3f * azar(s ^ 0x3u)), ALTO);
            // La deriva del viento: en lo que tarda en caer, se desplaza.
            const float tCaida = caida / (k.velocidad > 0.01f ? k.velocidad : 0.01f);
            float dx = e.vientoX * tCaida * (c == 3 ? 1.0f : 0.5f);
            float dz = e.vientoZ * tCaida * (c == 3 ? 1.0f : 0.5f);
            if (c == 2) {   // la nieve se mece
                dx += 0.6f * std::sin(tf * 1.3f + fase * 31.0f);
                dz += 0.6f * std::cos(tf * 1.1f + fase * 17.0f);
            }
            if (c == 3) {   // el polvo va casi horizontal y a rafagas
                const float a = std::fmod(fase * 40.0f + tf * 9.0f, 40.0f) - 20.0f;
                // Recorre la caja en la direccion del viento.
                const float vl = std::sqrt(e.vientoX * e.vientoX + e.vientoZ * e.vientoZ) + 0.01f;
                dx = a * e.vientoX / vl;
                dz = a * e.vientoZ / vl;
                caida = ALTO * (0.55f + 0.4f * fase) + 2.0f * std::sin(tf * 0.7f + fase * 9.0f);
            }
            // Anclada a la rejilla del mundo: la caja se repite cada LADO.
            const double baseX = std::floor((e.ojoX - LADO * 0.5 - ox) / LADO + 1.0) * LADO + ox;
            const double baseZ = std::floor((e.ojoZ - LADO * 0.5 - oz) / LADO + 1.0) * LADO + oz;
            const double wx = baseX + dx, wz = baseZ + dz;
            const double wy = e.ojoY + ALTO - DEBAJO - caida;
            if (wy < techos.en((int)std::floor(wx), (int)std::floor(wz))) continue;

            Trazo& o = out[n++];
            o.clase = c;
            o.x1 = (float)(wx - e.ojoX);
            o.y1 = (float)(wy - e.ojoY);
            o.z1 = (float)(wz - e.ojoZ);
            if (k.largo > 0.0f) {
                // La raya apunta hacia donde viene: arriba y contra el viento.
                const float vx = e.vientoX * 0.5f / k.velocidad, vz = e.vientoZ * 0.5f / k.velocidad;
                const float l = k.largo / std::sqrt(1.0f + vx * vx + vz * vz);
                o.x0 = o.x1 - vx * l; o.y0 = o.y1 + l; o.z0 = o.z1 - vz * l;
            } else if (c == 3) {
                const float vl = std::sqrt(e.vientoX * e.vientoX + e.vientoZ * e.vientoZ) + 0.01f;
                o.x0 = o.x1 - e.vientoX / vl * 0.7f; o.y0 = o.y1; o.z0 = o.z1 - e.vientoZ / vl * 0.7f;
            } else {
                o.x0 = o.x1; o.y0 = o.y1; o.z0 = o.z1;
            }
            // Se apagan arriba y abajo de la columna, para que no se vea el corte.
            const float borde = std::fmin(caida / 3.0f, (ALTO - caida) / 3.0f);
            const float a = k.a * (borde < 1.0f ? (borde > 0.0f ? borde : 0.0f) : 1.0f);
            o.r = k.r * brillo; o.g = k.g * brillo; o.b = k.b * brillo; o.a = a;
            if (c == 3) { o.r = k.r * (0.4f + 0.6f * e.luz); o.g = k.g * (0.4f + 0.6f * e.luz); o.b = k.b * (0.4f + 0.6f * e.luz); }
        }
    }
    return n;
}

} // namespace Precipitacion
