#pragma once
// ============================================================================
// EL CLIMA: DESPEJADO, LLUVIAS, TORMENTAS, GRANIZO, NIEVE Y TOLVANERAS
// ============================================================================
// PEDIDO: "que las nubes, dependiendo del bioma, clima y aleatorio, se junten
// o haya muchas mas, y empiece a llover en diferentes niveles, y pueda ser
// tormenta, tormenta electrica y diferentes tormentas de la vida real".
//
// LOS TIPOS DE TIEMPO, de lo mas tranquilo a lo mas violento:
//
//   DESPEJADO            pocas nubes sueltas
//   NUBLADO              cielo cubierto, gris, sin lluvia
//   NEBLINA              niebla baja y quieta (mananas de bosque)
//   LLOVIZNA             gotas finas, cielo gris claro
//   LLUVIA               lluvia normal
//   AGUACERO             lluvia fuerte, cielo oscuro
//   TORMENTA             aguacero con rafagas de viento
//   TORMENTA ELECTRICA   lluvia fuerte, rayos y truenos
//   GRANIZADA            granizo, algo de lluvia y rayos
//   TORMENTA TROPICAL    lo peor del tropico: lluvia y viento al maximo
//   NEVADA               nieve que cae despacio (solo donde hace frio)
//   VENTISCA             nieve horizontal, no se ve nada
//   TOLVANERA            tormenta de polvo del desierto: viento y arena
//
// DE QUE DEPENDE:
//
//   EL LUGAR   la temperatura y la humedad del bioma (0..1). En el desierto
//              casi siempre esta despejado y hay tolvaneras; en el bosque
//              humedo llueve mucho y amanece con neblina; donde hace frio la
//              lluvia es nieve; el tropico tiene aguaceros y tormentas
//              tropicales.
//   LA HORA    las tormentas electricas son de tarde, como en la realidad
//              (el suelo se calienta todo el dia y el aire sube por la
//              tarde); la neblina, de manana.
//   EL AZAR    cada tramo de tiempo saca su tiempo de un hash: no hay dos
//              dias iguales.
//
// SIN ESTADO, como las nubes: el tiempo que hace es una FUNCION del reloj del
// mundo y del sitio. No hay que guardarlo, y el mismo instante da siempre el
// mismo cielo. Cada tramo dura entre 6 y 12 minutos de mundo; entre uno y otro
// el tiempo CAMBIA POCO A POCO (minuto y medio): la tormenta se arma, las
// nubes se juntan, empieza a gotear y luego llueve. Y el mundo esta partido
// en regiones de 640 bloques con su propio tiempo, fundidas entre si: andando
// se puede salir de debajo de una tormenta.
//
// Este archivo no sabe nada de OpenGL ni de audio: da numeros, y los tests
// los comprueban. El motor los convierte en nubes, luz, niebla, gotas y
// sonido.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Clima {

enum Tipo {
    DESPEJADO, NUBLADO, NEBLINA, LLOVIZNA, LLUVIA, AGUACERO, TORMENTA,
    TORMENTA_ELECTRICA, GRANIZADA, TORMENTA_TROPICAL, NEVADA, VENTISCA,
    TOLVANERA, NUM_TIPOS
};

inline const char* nombre(int t) {
    static const char* const N[NUM_TIPOS] = {
        "DESPEJADO", "NUBLADO", "NEBLINA", "LLOVIZNA", "LLUVIA", "AGUACERO",
        "TORMENTA", "TORMENTA ELECTRICA", "GRANIZADA", "TORMENTA TROPICAL",
        "NEVADA", "VENTISCA", "TOLVANERA",
    };
    return t >= 0 && t < NUM_TIPOS ? N[t] : "?";
}

// Lo que el tiempo le hace al mundo. Todo de 0 a 1 menos los rayos.
struct Perfil {
    float cobertura = 0.0f;   // cuanto cielo tapan las nubes
    float oscuridad = 0.0f;   // nubes grises y luz apagada
    float agua = 0.0f;        // lluvia
    float granizo = 0.0f;
    float nieve = 0.0f;
    float polvo = 0.0f;       // arena y polvo en el aire
    float viento = 0.0f;
    float rayos = 0.0f;       // rayos por minuto
    float niebla = 0.0f;      // cuanto se acerca la niebla
};

inline Perfil perfilDe(int t) {
    //                 cob   osc   agua granizo nieve polvo viento rayos niebla
    static const float P[NUM_TIPOS][9] = {
        /* DESPEJADO   */ {0.22f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.15f, 0.0f, 0.00f},
        /* NUBLADO     */ {0.80f, 0.30f, 0.00f, 0.00f, 0.00f, 0.00f, 0.25f, 0.0f, 0.12f},
        /* NEBLINA     */ {0.60f, 0.22f, 0.00f, 0.00f, 0.00f, 0.00f, 0.05f, 0.0f, 0.80f},
        /* LLOVIZNA    */ {0.88f, 0.40f, 0.22f, 0.00f, 0.00f, 0.00f, 0.20f, 0.0f, 0.30f},
        /* LLUVIA      */ {0.95f, 0.55f, 0.50f, 0.00f, 0.00f, 0.00f, 0.35f, 0.0f, 0.40f},
        /* AGUACERO    */ {1.00f, 0.70f, 0.85f, 0.00f, 0.00f, 0.00f, 0.50f, 0.0f, 0.55f},
        /* TORMENTA    */ {1.00f, 0.80f, 0.90f, 0.00f, 0.00f, 0.00f, 0.85f, 0.5f, 0.55f},
        /* T.ELECTRICA */ {1.00f, 0.85f, 0.80f, 0.00f, 0.00f, 0.00f, 0.60f, 8.0f, 0.50f},
        /* GRANIZADA   */ {1.00f, 0.80f, 0.35f, 0.80f, 0.00f, 0.00f, 0.50f, 3.0f, 0.40f},
        /* T.TROPICAL  */ {1.00f, 0.88f, 1.00f, 0.00f, 0.00f, 0.00f, 1.00f, 4.0f, 0.65f},
        /* NEVADA      */ {0.95f, 0.40f, 0.00f, 0.00f, 0.55f, 0.00f, 0.20f, 0.0f, 0.45f},
        /* VENTISCA    */ {1.00f, 0.60f, 0.00f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 0.85f},
        /* TOLVANERA   */ {0.30f, 0.15f, 0.00f, 0.00f, 0.00f, 1.00f, 0.95f, 0.0f, 0.80f},
    };
    const float* v = P[t >= 0 && t < NUM_TIPOS ? t : 0];
    Perfil p;
    p.cobertura = v[0]; p.oscuridad = v[1]; p.agua = v[2]; p.granizo = v[3];
    p.nieve = v[4]; p.polvo = v[5]; p.viento = v[6]; p.rayos = v[7]; p.niebla = v[8];
    return p;
}

inline Perfil mezclar(const Perfil& a, const Perfil& b, float t) {
    auto m = [t](float x, float y) { return x + (y - x) * t; };
    Perfil p;
    p.cobertura = m(a.cobertura, b.cobertura);
    p.oscuridad = m(a.oscuridad, b.oscuridad);
    p.agua = m(a.agua, b.agua);
    p.granizo = m(a.granizo, b.granizo);
    p.nieve = m(a.nieve, b.nieve);
    p.polvo = m(a.polvo, b.polvo);
    p.viento = m(a.viento, b.viento);
    p.rayos = m(a.rayos, b.rayos);
    p.niebla = m(a.niebla, b.niebla);
    return p;
}

// El clima del sitio: temperatura (0 helado .. 1 muy caluroso) y humedad
// (0 desierto .. 1 selva). Lo da el bioma.
struct Lugar {
    float temperatura = 0.6f;
    float humedad = 0.5f;
};

inline float sat(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float suave(float a, float b, float x) {
    const float t = sat((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}

// Cuan probable es cada tiempo en este lugar y a esta hora (0..24). No suman
// 1: son pesos.
inline void pesos(const Lugar& l, float hora, float w[NUM_TIPOS]) {
    const float calor = sat(l.temperatura), frio = 1.0f - calor;
    const float humedo = sat(l.humedad), seco = 1.0f - humedo;
    // Donde hiela, lo que cae es nieve: la lluvia se apaga y la nieve sube.
    const float hiela = suave(0.55f, 0.80f, frio);
    const float liquido = 1.0f - hiela;
    // Tarde de tormentas (13 a 20 h) y manana de neblina (5 a 9 h).
    const float tarde = suave(12.0f, 14.5f, hora) * (1.0f - suave(19.0f, 21.5f, hora));
    const float manana = suave(4.5f, 6.0f, hora) * (1.0f - suave(8.5f, 10.0f, hora));
    const float templado = sat(1.0f - std::fabs(calor - 0.45f) * 2.5f);

    w[DESPEJADO] = 1.0f + 3.0f * seco;
    w[NUBLADO] = 1.1f;
    w[NEBLINA] = 0.6f * humedo * (1.0f - 0.5f * calor) * (0.3f + 2.5f * manana);
    w[LLOVIZNA] = 1.1f * humedo * liquido;
    w[LLUVIA] = 1.2f * humedo * liquido;
    w[AGUACERO] = 0.8f * humedo * (0.4f + calor) * liquido;
    w[TORMENTA] = 0.45f * humedo * liquido;
    w[TORMENTA_ELECTRICA] = (0.25f + 0.9f * calor) * (0.25f + humedo) * (0.35f + 2.0f * tarde) * liquido;
    w[GRANIZADA] = 0.30f * (0.4f + humedo) * templado * (0.5f + tarde) * liquido;
    w[TORMENTA_TROPICAL] = 0.9f * calor * calor * humedo * humedo * liquido;
    w[NEVADA] = 2.2f * hiela * (0.3f + humedo);
    w[VENTISCA] = 0.6f * hiela * hiela;
    w[TOLVANERA] = 1.4f * seco * seco * calor * (1.0f - hiela);
}

inline uint32_t hash(uint32_t h) {
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}
inline float azar(uint32_t h) { return (float)(hash(h) >> 8) * (1.0f / 16777216.0f); }

constexpr double TRAMO = 540.0;          // la duracion media de un tramo
constexpr double CAMBIO = 90.0;          // lo que tarda en cambiar el tiempo
constexpr double REGION = 640.0;         // bloques
constexpr double SEGUNDOS_DIA = 1440.0;  // como SEGUNDOS_POR_DIA del motor

// Donde empieza el tramo `k` de una region: los tramos no miden todos igual
// (de 6 a 12 minutos), y cada region va desfasada para que no cambie el
// tiempo en todo el mundo a la vez.
inline double inicioTramo(int64_t k, uint32_t region) {
    // Acumular duraciones seria caro: se reparte un desfase acotado por
    // tramo alrededor de la rejilla regular. Sigue siendo creciente.
    const double desfaseRegion = azar(region ^ 0x9e3779b9u) * TRAMO;
    const double jitter = (azar((uint32_t)k * 0x85ebca6bu ^ region) - 0.5) * TRAMO * 0.6;
    return (double)k * TRAMO + jitter + desfaseRegion;
}

// El tiempo que toca en el tramo `k` de la region, sorteado con los pesos.
inline int tipoDeTramo(int64_t k, uint32_t region, const Lugar& l) {
    const double t0 = inicioTramo(k, region);
    // La hora de la mitad del tramo: es la que manda.
    double dia = std::fmod(t0 + TRAMO * 0.5, SEGUNDOS_DIA);
    if (dia < 0.0) dia += SEGUNDOS_DIA;
    float w[NUM_TIPOS];
    pesos(l, (float)(dia / SEGUNDOS_DIA * 24.0), w);
    float total = 0.0f;
    for (float x : w) total += x;
    float r = azar((uint32_t)k * 0xc2b2ae35u ^ hash(region + 0x27d4eb2fu)) * total;
    for (int i = 0; i < NUM_TIPOS; ++i) {
        if (r < w[i]) return i;
        r -= w[i];
    }
    return DESPEJADO;
}

// El tiempo de UNA region en el instante t: el de su tramo, fundido con el
// anterior durante el CAMBIO.
inline Perfil perfilRegion(double t, uint32_t region, const Lugar& l, int* tipo = nullptr) {
    int64_t k = (int64_t)std::floor(t / TRAMO);
    // El tramo que contiene t (el desfase puede moverlo uno arriba o abajo).
    for (int i = 0; i < 3 && inicioTramo(k + 1, region) <= t; ++i) ++k;
    for (int i = 0; i < 3 && inicioTramo(k, region) > t; ++i) --k;
    const int actual = tipoDeTramo(k, region, l);
    const int antes = tipoDeTramo(k - 1, region, l);
    const float f = suave(0.0f, (float)CAMBIO, (float)(t - inicioTramo(k, region)));
    if (tipo) *tipo = f >= 0.5f ? actual : antes;
    return mezclar(perfilDe(antes), perfilDe(actual), f);
}

inline uint32_t region(int64_t i, int64_t j) {
    return hash((uint32_t)i * 0x9E3779B1u ^ hash((uint32_t)j * 0x85EBCA77u + 0x165667B1u));
}

// EL TIEMPO EN UN SITIO: el de las cuatro regiones de alrededor, fundido.
// `tipo` devuelve el que mas pesa (para el HUD).
inline Perfil perfilEn(double t, double x, double z, const Lugar& l, int* tipo = nullptr) {
    const double fx = x / REGION - 0.5, fz = z / REGION - 0.5;
    const int64_t i = (int64_t)std::floor(fx), j = (int64_t)std::floor(fz);
    // Una transicion ancha entre regiones: solo el 40 % central de cada una
    // tiene "su" tiempo puro.
    const float ux = suave(0.3f, 0.7f, (float)(fx - (double)i));
    const float uz = suave(0.3f, 0.7f, (float)(fz - (double)j));
    int t00, t10, t01, t11;
    const Perfil p00 = perfilRegion(t, region(i, j), l, &t00);
    const Perfil p10 = perfilRegion(t, region(i + 1, j), l, &t10);
    const Perfil p01 = perfilRegion(t, region(i, j + 1), l, &t01);
    const Perfil p11 = perfilRegion(t, region(i + 1, j + 1), l, &t11);
    if (tipo) {
        const float w[4] = { (1 - ux) * (1 - uz), ux * (1 - uz), (1 - ux) * uz, ux * uz };
        const int ts[4] = { t00, t10, t01, t11 };
        int mejor = 0;
        for (int k = 1; k < 4; ++k) if (w[k] > w[mejor]) mejor = k;
        *tipo = ts[mejor];
    }
    return mezclar(mezclar(p00, p10, ux), mezclar(p01, p11, ux), uz);
}

// ----------------------------------------------------------------------------
// LOS RAYOS
// ----------------------------------------------------------------------------
// Tambien sin estado: cada segundo de mundo tiene, segun un hash, su rayo o
// no, con una probabilidad que sale de los rayos por minuto del tiempo que
// hace. La luz del rayo llega al instante; el trueno, a la velocidad del
// sonido (343 m/s, y un bloque son 0,6 m): de un rayo a 3 km el trueno tarda
// 9 segundos, como en la vida real.
struct Rayo {
    double t;          // cuando cae
    float distancia;   // metros
    float angulo;      // rumbo desde el jugador, en radianes
    uint32_t semilla;  // la forma del zigzag
};

constexpr double SONIDO_MS = 343.0;
constexpr float DISTANCIA_MIN = 250.0f, DISTANCIA_MAX = 7000.0f;

inline double retrasoTrueno(const Rayo& r) { return r.distancia / SONIDO_MS; }

// Los rayos que caen en (t0, t1], con `porMinuto` sacado del tiempo de cada
// segundo por quien llama (ver rayosEntre).
template <class FnRitmo>
inline int rayosEntre(double t0, double t1, FnRitmo porMinuto, Rayo* out, int max) {
    int n = 0;
    const int64_t s0 = (int64_t)std::floor(t0), s1 = (int64_t)std::floor(t1);
    for (int64_t s = s0; s <= s1 && n < max; ++s) {
        const uint32_t h = hash((uint32_t)s * 0x2545F491u + 0x68E31DA4u);
        const float p = porMinuto((double)s) / 60.0f;
        if (p <= 0.0f || azar(h) >= p) continue;
        Rayo r;
        r.t = (double)s + azar(h ^ 0x1u);
        if (r.t <= t0 || r.t > t1) continue;
        const float u = azar(h ^ 0x2u);
        r.distancia = DISTANCIA_MIN + (DISTANCIA_MAX - DISTANCIA_MIN) * u * u;
        r.angulo = 6.2831853f * azar(h ^ 0x3u);
        r.semilla = hash(h ^ 0x4u);
        out[n++] = r;
    }
    return n;
}

// Cuanto ilumina un rayo `dt` segundos despues de caer: dos o tres
// parpadeos en un cuarto de segundo y una cola corta. Los cercanos alumbran
// mas.
inline float destello(const Rayo& r, double dt) {
    if (dt < 0.0 || dt > 0.6) return 0.0f;
    const float x = (float)dt;
    float f = std::exp(-x * 9.0f);
    if (r.semilla & 1u) f = (std::max)(f, 0.8f * std::exp(-std::fabs(x - 0.12f) * 40.0f));
    if (r.semilla & 2u) f = (std::max)(f, 0.6f * std::exp(-std::fabs(x - 0.24f) * 40.0f));
    const float cerca = 1.0f / (1.0f + r.distancia / 1500.0f);
    return f * (0.35f + 0.65f * cerca);
}

// Lo fuerte que suena el trueno (0..1).
inline float fuerzaTrueno(const Rayo& r) {
    return 1.0f / (1.0f + r.distancia / 1200.0f);
}

} // namespace Clima
