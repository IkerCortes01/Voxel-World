#pragma once

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR: ESQUELETO HUMANO CON SUS LIMITES
// ============================================================================
// PEDIDOS:
//   "un modelo para el jugador con forma humana simple y con articulaciones y
//    movimiento [...] si ve directo abajo, se vea el torso y el cuerpo".
//   "limitaciones de movimiento de cabeza, cuello, tendones, musculos [...]
//    manos con 5 dedos y muneca [...] piernas, rodillas, dedos".
//
// Aqui vive solo el ESQUELETO: donde esta cada articulacion y como gira. La
// piel, los musculos, los tendones y las venas se ponen encima en
// MallaJugador.h. Separados para poder probar el movimiento sin geometria.
//
// Cada hueso cuelga del extremo del anterior (cinematica directa), asi que
// nada se despega: hombro -> codo -> muneca -> palma -> 3 falanges por dedo;
// cadera -> rodilla -> tobillo -> pie -> 2 falanges por dedo del pie.
//
// Todos los giros pasan por `limitar`, con los rangos de una persona sana:
// el codo no se dobla hacia atras, la rodilla tampoco, la cabeza no gira mas
// de 70 grados sobre el cuello y los dedos no se doblan hacia el dorso.
//
// La cabeza no se dibuja: la camara esta dentro de ella.
//
// Espacio local: origen en los pies, +Y arriba, +Z delante, +X derecha. Las
// medidas son de una persona de 1.80 en la escala del jugador (1.8 de alto,
// ojo a 1.62).
// ============================================================================

namespace ModeloJugador {

constexpr float PI = 3.14159265f;

struct V { float x, y, z; };

inline V suma(V a, V b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline V resta(V a, V b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline V escala(V a, float k) { return { a.x * k, a.y * k, a.z * k }; }
inline float punto(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V cruz(V a, V b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline float largo(V a) { return std::sqrt(punto(a, a)); }
inline V normalizar(V a) {
    const float l = largo(a);
    return l > 1e-9f ? escala(a, 1.0f / l) : V{ 0, 1, 0 };
}
inline V mezclar(V a, V b, float t) { return suma(a, escala(resta(b, a), t)); }
inline float sujetar(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Rotacion 3x3 por filas.
struct Rot {
    float m[9];
    static Rot identidad() { return { { 1, 0, 0, 0, 1, 0, 0, 0, 1 } }; }
    // Giro en X: +a lleva la pieza que cuelga (-Y) hacia delante (+Z), y la
    // que apunta delante (+Z) hacia arriba.
    static Rot enX(float a) {
        const float c = std::cos(a), s = std::sin(a);
        return { { 1, 0, 0, 0, c, s, 0, -s, c } };
    }
    // Giro en Y: +a lleva +Z hacia +X.
    static Rot enY(float a) {
        const float c = std::cos(a), s = std::sin(a);
        return { { c, 0, s, 0, 1, 0, -s, 0, c } };
    }
    // Giro en Z: +a lleva la pieza que cuelga hacia +X.
    static Rot enZ(float a) {
        const float c = std::cos(a), s = std::sin(a);
        return { { c, -s, 0, s, c, 0, 0, 0, 1 } };
    }
    V aplicar(V v) const {
        return { m[0] * v.x + m[1] * v.y + m[2] * v.z,
                 m[3] * v.x + m[4] * v.y + m[5] * v.z,
                 m[6] * v.x + m[7] * v.y + m[8] * v.z };
    }
    Rot por(const Rot& b) const {
        Rot r;
        for (int f = 0; f < 3; ++f)
            for (int c = 0; c < 3; ++c)
                r.m[f * 3 + c] = m[f * 3] * b.m[c] + m[f * 3 + 1] * b.m[3 + c] +
                                 m[f * 3 + 2] * b.m[6 + c];
        return r;
    }
};

// ----------------------------------------------------------------------------
// MEDIDAS
// ----------------------------------------------------------------------------
namespace M {
    constexpr float CADERA    = 0.94f;   // altura de la articulacion de la cadera
    constexpr float X_CADERA  = 0.095f;
    constexpr float ESPALDA   = 0.55f;   // de la pelvis a la base del cuello
    constexpr float HOMBRO    = 0.48f;   // de la pelvis al hombro, por la espalda
    constexpr float X_HOMBRO  = 0.20f;
    constexpr float MUSLO     = 0.45f;
    constexpr float PIERNA    = 0.43f;
    constexpr float BRAZO     = 0.30f;
    constexpr float ANTEBRAZO = 0.26f;
    constexpr float PALMA     = 0.095f;
    constexpr float OJO       = 1.62f;   // la misma altura que la camara
    constexpr float OJO_SOBRE_CUELLO = 0.12f;
    // El cuerpo va por detras del ojo: el ojo esta en la cara, delante del
    // cuello. Asi, al mirar abajo, se ve el pecho por fuera y no por dentro.
    constexpr float ATRAS     = 0.11f;

    // Las falanges: indice, medio, anular, menique y pulgar (el pulgar
    // empieza por el metacarpiano, que se mueve: es lo que lo hace oponible).
    constexpr float DEDO[5][3] = {
        { 0.042f, 0.025f, 0.019f },
        { 0.046f, 0.029f, 0.020f },
        { 0.043f, 0.027f, 0.020f },
        { 0.034f, 0.021f, 0.018f },
        { 0.046f, 0.032f, 0.026f },
    };
    // Los dedos del pie, del gordo al pequeno: dos tramos cada uno. Algo mas
    // largos que los reales para que se lean desde los ojos, a 1.6 de altura.
    constexpr float DEDO_PIE[5] = { 0.040f, 0.032f, 0.029f, 0.025f, 0.021f };
}

// ----------------------------------------------------------------------------
// LIMITES DE CADA ARTICULACION (radianes), los de una persona sana
// ----------------------------------------------------------------------------
namespace Lim {
    constexpr float CUELLO_GIRO  = 1.22f;   // 70 grados a cada lado
    constexpr float CUELLO_FLEX  = 0.87f;   // 50 hacia el pecho; el resto lo ponen los ojos
    constexpr float TORSO_GIRO   = 0.52f;   // 30 de los hombros sobre la pelvis
    constexpr float INCLINA_MIN = -0.25f, INCLINA_MAX = 1.10f;

    constexpr float HOMBRO_FLEX_MIN = -0.90f, HOMBRO_FLEX_MAX = 3.00f;
    constexpr float HOMBRO_ABD_MIN  = -0.20f, HOMBRO_ABD_MAX  = 2.60f;
    constexpr float HOMBRO_ROT_MIN  = -1.20f, HOMBRO_ROT_MAX  = 1.20f;
    constexpr float CODO_MAX        = 2.60f;    // 150 grados, y nunca hacia atras
    constexpr float PRONO_MIN = -1.40f, PRONO_MAX = 1.50f;
    constexpr float MUNECA_FLEX_MIN = -1.20f, MUNECA_FLEX_MAX = 1.40f;
    constexpr float MUNECA_DESV_MIN = -0.35f, MUNECA_DESV_MAX = 0.55f;
    // Dedos: [falange][min,max]. Un poco de hiperextension en la base, nada en
    // la del medio.
    constexpr float DEDO[3][2] = { { -0.35f, 1.57f }, { 0.0f, 1.92f }, { -0.10f, 1.40f } };
    constexpr float PULGAR[3][2] = { { -0.20f, 0.90f }, { 0.0f, 1.00f }, { -0.20f, 1.40f } };
    constexpr float DEDO_ABRE_MIN = -0.15f, DEDO_ABRE_MAX = 0.35f;

    constexpr float CADERA_FLEX_MIN = -0.35f, CADERA_FLEX_MAX = 2.10f;
    constexpr float CADERA_ABD_MIN  = -0.50f, CADERA_ABD_MAX  = 0.80f;
    constexpr float RODILLA_MAX     = 2.40f;
    constexpr float TOBILLO_MIN = -0.87f, TOBILLO_MAX = 0.35f;   // punta abajo / arriba
    constexpr float DEDOS_PIE_MIN = -0.70f, DEDOS_PIE_MAX = 1.10f;
}

// ----------------------------------------------------------------------------
// LA POSE: los giros de cada articulacion
// ----------------------------------------------------------------------------
// Indice 0 = izquierda, 1 = derecha.
struct Brazo {
    float flex = 0.0f, abd = 0.10f, rot = 0.0f;
    float codo = 0.15f;
    float prono = 0.25f;               // 0 = pulgar delante; + = palma hacia atras
    float munFlex = 0.10f, munDesv = 0.0f;
    float dedo[5][3] = {};             // indice, medio, anular, menique, pulgar
    float abre[5] = {};
};
struct Pierna {
    float flex = 0.0f, abd = 0.03f;
    float rodilla = 0.05f;
    float tobillo = 0.0f;
    float dedos = 0.0f;                // + = dedos hacia arriba (extension)
};
struct Pose {
    Brazo brazo[2];
    Pierna pierna[2];
    float inclinacion = 0.0f;          // el torso hacia delante
    float giroTorso = 0.0f;            // los hombros sobre la pelvis
    float giroPelvis = 0.0f;
    float respira = 0.0f;              // -1..1
    float cuelloFlex = 0.0f;           // la cabeza hacia el pecho
    float agachado = 0.0f;
    float tumbado = 0.0f;              // 0..1: boca abajo, el torso en el suelo
    float apretar = 0.0f;              // 0..1, cuanto aprieta el puno derecho
    uint8_t enMano = 0;                // Herramienta que se dibuja en la derecha
    bool pedernal = false;             // cabeza de pedernal (si no, de piedra)
};

inline void limitar(Pose& p) {
    using namespace Lim;
    p.inclinacion = sujetar(p.inclinacion, INCLINA_MIN, INCLINA_MAX);
    p.giroTorso = sujetar(p.giroTorso, -TORSO_GIRO, TORSO_GIRO);
    p.cuelloFlex = sujetar(p.cuelloFlex, -CUELLO_FLEX, CUELLO_FLEX);
    for (Brazo& b : p.brazo) {
        b.flex = sujetar(b.flex, HOMBRO_FLEX_MIN, HOMBRO_FLEX_MAX);
        b.abd = sujetar(b.abd, HOMBRO_ABD_MIN, HOMBRO_ABD_MAX);
        b.rot = sujetar(b.rot, HOMBRO_ROT_MIN, HOMBRO_ROT_MAX);
        b.codo = sujetar(b.codo, 0.0f, CODO_MAX);
        b.prono = sujetar(b.prono, PRONO_MIN, PRONO_MAX);
        b.munFlex = sujetar(b.munFlex, MUNECA_FLEX_MIN, MUNECA_FLEX_MAX);
        b.munDesv = sujetar(b.munDesv, MUNECA_DESV_MIN, MUNECA_DESV_MAX);
        for (int d = 0; d < 5; ++d) {
            const auto& L = d == 4 ? PULGAR : DEDO;
            for (int f = 0; f < 3; ++f) b.dedo[d][f] = sujetar(b.dedo[d][f], L[f][0], L[f][1]);
            b.abre[d] = sujetar(b.abre[d], DEDO_ABRE_MIN, DEDO_ABRE_MAX);
        }
    }
    for (Pierna& g : p.pierna) {
        g.flex = sujetar(g.flex, CADERA_FLEX_MIN, CADERA_FLEX_MAX);
        g.abd = sujetar(g.abd, CADERA_ABD_MIN, CADERA_ABD_MAX);
        g.rodilla = sujetar(g.rodilla, 0.0f, RODILLA_MAX);
        g.tobillo = sujetar(g.tobillo, TOBILLO_MIN, TOBILLO_MAX);
        g.dedos = sujetar(g.dedos, DEDOS_PIE_MIN, DEDOS_PIE_MAX);
    }
}

// ----------------------------------------------------------------------------
// LO QUE HACE EL JUGADOR, PARA ELEGIR LA ANIMACION
// ----------------------------------------------------------------------------
// El suelo bajo los pies (cambia como se agacha).
enum class Superficie : uint8_t { NATURAL, CONSTRUCCION, LIQUIDO };
// Lo que hace el brazo.
enum class Accion : uint8_t { NINGUNA, PICAR, GOLPE_AIRE, GOLPE_ANIMAL };
// Lo que lleva en la mano. MANO = nada (o algo que no es herramienta).
enum class Herramienta : uint8_t { MANO, PICO, HACHA, MARTILLO, AZADA, NUM };
// Contra que golpea: cada herramienta se usa distinto en cada uno.
enum class ClaseBloque : uint8_t { TIERRA, MADERA, PIEDRA, CONSTRUIDO, NUM };

// ----------------------------------------------------------------------------
// EL ESTADO QUE LA ANIMA
// ----------------------------------------------------------------------------
struct Estado {
    float fasePaso = 0;      // radianes, avanza con la distancia andada
    float rapidez = 0;       // bloques/s en horizontal
    bool  enSuelo = true;
    bool  agachado = false;
    bool  tumbado = false;   // boca abajo (SHIFT dos veces, ver Postura.h)
    bool  volando = false;
    bool  golpeando = false; // clic izquierdo mantenido: el brazo derecho pica
    float tiempo = 0;        // segundos
    float giroCuello = 0;    // radianes: cabeza menos pelvis (ver GiroCuerpo)
    float mirada = 0;        // pitch de la camara en grados (- = abajo)
    float faseRespira = 0;   // radianes (ver Fisiologia)

    // --- Las animaciones de cada cosa ---
    Superficie superficie = Superficie::NATURAL;
    Accion accion = Accion::NINGUNA;
    Herramienta herramienta = Herramienta::MANO;
    ClaseBloque bloque = ClaseBloque::TIERRA;
    int variante = 0;        // cual de los 3 golpes a un animal
    int ladoGolpe = 1;       // el brazo del gancho de abajo
    float tAccion = 0;       // segundos desde el clic (golpes de un disparo)
    float escalon = -1;      // 0..1 subiendo o bajando un nivel; <0 nada
    bool escalonSube = true;
    int piernaEscalon = 1;
    bool pedernal = false;   // la herramienta es de pedernal
};

// Avanzar el paso con lo que se ha andado este frame (en bloques).
inline float avanzarPaso(float fase, float distancia) {
    // Una zancada completa (dos pasos) cada ~2.2 bloques.
    return std::fmod(fase + distancia * (2.0f * PI / 2.2f), 2.0f * PI);
}

// Una campana sobre el ciclo de la marcha (0..1, con la vuelta incluida).
// Polinomica: es mas barata que exp y vale igual para dar forma.
inline float campana(float t, float centro, float ancho) {
    float d = t - centro;
    d -= std::floor(d + 0.5f);
    const float x = d / (ancho * 2.2f);
    if (x <= -1.0f || x >= 1.0f) return 0.0f;
    const float u = 1.0f - x * x;
    return u * u;
}
inline float suave(float x) { x = sujetar(x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); }

// Los dedos de una mano relajada: no quedan rectos, se curvan en cascada, mas
// el menique que el indice.
inline void manoRelajada(Brazo& b, float cierre) {
    for (int d = 0; d < 4; ++d) {
        b.dedo[d][0] = 0.30f + 0.08f * d + cierre * 1.15f;
        b.dedo[d][1] = 0.45f + 0.10f * d + cierre * 1.30f;
        b.dedo[d][2] = 0.25f + 0.05f * d + cierre * 0.65f;
        b.abre[d] = 0.12f * (1.0f - cierre);    // un hueco entre dedo y dedo
    }
    b.dedo[4][0] = 0.30f + cierre * 0.45f;
    b.dedo[4][1] = 0.25f + cierre * 0.45f;
    b.dedo[4][2] = 0.20f + cierre * 0.45f;
}


// ============================================================================
// LOS GOLPES: 5 HERRAMIENTAS x 4 CLASES DE BLOQUE = 20 MANERAS DE PICAR
// ============================================================================
// PEDIDO: "con cada herramienta tenga 4 animaciones dependiendo del bloque
// para romper y golpear".
//
// Cada golpe es un ciclo: CARGAR (del final del golpe anterior a la pose
// cargada, despacio), GOLPEAR (de la cargada a la de impacto, rapido y
// acelerando) y RECOGER. Los valores son los giros del brazo derecho y del
// torso en cada pose; el izquierdo acompana si el golpe es a dos manos.
//
// Por que se parecen y por que no: una persona pica piedra desde arriba y con
// todo el cuerpo, la tierra con golpes cortos hacia abajo, la madera de lado
// (el hachazo entra en diagonal) y lo construido con toques controlados para
// no romper lo de al lado.
struct Golpe {
    float periodo;         // segundos por golpe
    float carga;           // fraccion del ciclo cargando
    float flex[2];         // hombro: cargado, impacto
    float abd[2];
    float rot[2];
    float codo[2];
    float muneca[2];       // + hacia la palma
    float torso[2];        // giro de los hombros
    float inclina;         // el cuerpo acompana hacia delante en el impacto
    bool dosManos;
};

inline const Golpe& golpeDe(Herramienta h, ClaseBloque c) {
    //                     periodo carga   flex           abd           rot            codo          muneca         torso          incl  2manos
    static const Golpe T[(int)Herramienta::NUM][(int)ClaseBloque::NUM] = {
        { // MANO
            { 0.42f, 0.55f, { 2.10f, 0.70f }, { 0.12f, 0.10f }, { 0.15f, 0.10f }, { 1.60f, 0.35f }, { -0.5f,  0.6f }, { -0.10f, 0.10f }, 0.08f, false }, // tierra: de arriba abajo
            { 0.40f, 0.55f, { 1.30f, 0.90f }, { 0.90f, 0.20f }, { -0.6f,  0.30f }, { 1.70f, 1.10f }, {  0.0f,  0.3f }, { -0.30f, 0.20f }, 0.04f, false }, // madera: puno de martillo de lado
            { 0.48f, 0.60f, { 0.80f, 1.45f }, { 0.15f, 0.10f }, { 0.00f, 0.00f }, { 2.00f, 0.15f }, {  0.0f,  0.1f }, { -0.25f, 0.25f }, 0.10f, false }, // piedra: directo
            { 0.30f, 0.50f, { 1.25f, 1.05f }, { 0.20f, 0.18f }, { 0.10f, 0.10f }, { 1.30f, 0.95f }, { -0.3f,  0.4f }, { -0.05f, 0.05f }, 0.02f, false }, // construido: toques
        },
        { // PICO
            { 0.62f, 0.62f, { 2.20f, 0.80f }, { 0.15f, 0.10f }, { 0.10f, 0.10f }, { 1.20f, 0.40f }, { -0.6f,  0.5f }, { -0.12f, 0.12f }, 0.18f, true  }, // tierra
            { 0.60f, 0.60f, { 1.70f, 1.00f }, { 1.00f, 0.25f }, { -0.7f,  0.20f }, { 1.10f, 0.35f }, { -0.3f,  0.4f }, { -0.45f, 0.30f }, 0.10f, true  }, // madera: de lado
            { 0.70f, 0.64f, { 2.70f, 0.60f }, { 0.10f, 0.08f }, { 0.10f, 0.10f }, { 1.60f, 0.25f }, { -0.7f,  0.6f }, { -0.20f, 0.20f }, 0.28f, true  }, // piedra: desde arriba, con todo
            { 0.45f, 0.55f, { 1.70f, 1.10f }, { 0.15f, 0.12f }, { 0.10f, 0.10f }, { 1.20f, 0.70f }, { -0.4f,  0.3f }, { -0.08f, 0.08f }, 0.05f, false }, // construido: controlado
        },
        { // HACHA
            { 0.60f, 0.60f, { 2.10f, 0.70f }, { 0.20f, 0.12f }, { 0.20f, 0.10f }, { 1.30f, 0.30f }, { -0.6f,  0.5f }, { -0.15f, 0.15f }, 0.15f, true  }, // tierra
            { 0.64f, 0.60f, { 1.50f, 1.20f }, { 1.30f, 0.15f }, { -0.9f,  0.40f }, { 1.30f, 0.20f }, { -0.4f,  0.3f }, { -0.55f, 0.40f }, 0.12f, true  }, // madera: hachazo en diagonal
            { 0.70f, 0.64f, { 2.50f, 0.80f }, { 0.15f, 0.10f }, { 0.10f, 0.10f }, { 1.40f, 0.35f }, { -0.6f,  0.4f }, { -0.15f, 0.15f }, 0.20f, true  }, // piedra
            { 0.46f, 0.55f, { 1.60f, 1.05f }, { 0.60f, 0.30f }, { -0.3f,  0.10f }, { 1.20f, 0.60f }, { -0.3f,  0.3f }, { -0.20f, 0.10f }, 0.05f, false }, // construido
        },
        { // MARTILLO
            { 0.55f, 0.60f, { 1.90f, 0.75f }, { 0.12f, 0.10f }, { 0.10f, 0.10f }, { 1.50f, 0.50f }, { -0.5f,  0.5f }, { -0.10f, 0.10f }, 0.14f, false }, // tierra: apisonar
            { 0.50f, 0.58f, { 1.80f, 1.00f }, { 0.40f, 0.20f }, { -0.2f,  0.10f }, { 1.60f, 0.70f }, { -0.6f,  0.6f }, { -0.15f, 0.10f }, 0.06f, false }, // madera: clavar
            { 0.62f, 0.62f, { 2.50f, 0.70f }, { 0.12f, 0.10f }, { 0.10f, 0.10f }, { 1.50f, 0.30f }, { -0.7f,  0.6f }, { -0.20f, 0.18f }, 0.22f, true  }, // piedra: mazazo
            { 0.32f, 0.50f, { 1.40f, 1.05f }, { 0.20f, 0.15f }, { 0.10f, 0.10f }, { 1.70f, 1.20f }, { -0.7f,  0.5f }, { -0.04f, 0.04f }, 0.02f, false }, // construido: golpecitos
        },
        { // AZADA
            { 0.66f, 0.62f, { 2.40f, 0.50f }, { 0.15f, 0.10f }, { 0.10f, 0.10f }, { 1.30f, 0.30f }, { -0.5f,  0.7f }, { -0.10f, 0.10f }, 0.30f, true  }, // tierra: cavar
            { 0.58f, 0.60f, { 1.60f, 1.00f }, { 0.90f, 0.20f }, { -0.6f,  0.20f }, { 1.20f, 0.40f }, { -0.3f,  0.4f }, { -0.35f, 0.25f }, 0.10f, true  }, // madera
            { 0.66f, 0.62f, { 2.30f, 0.80f }, { 0.12f, 0.10f }, { 0.10f, 0.10f }, { 1.40f, 0.40f }, { -0.6f,  0.4f }, { -0.15f, 0.15f }, 0.18f, true  }, // piedra
            { 0.44f, 0.55f, { 1.50f, 1.00f }, { 0.20f, 0.15f }, { 0.10f, 0.10f }, { 1.20f, 0.70f }, { -0.4f,  0.4f }, { -0.06f, 0.06f }, 0.04f, false }, // construido
        },
    };
    const int hi = (int)h < (int)Herramienta::NUM ? (int)h : 0;
    const int ci = (int)c < (int)ClaseBloque::NUM ? (int)c : 0;
    return T[hi][ci];
}

// Donde esta el ciclo de un golpe (0..1 = de cargado a impacto) en el tiempo
// `u` (0..1 del periodo). Cargar es suave; golpear acelera (como un brazo de
// verdad, que llega al impacto a toda velocidad); despues se queda un momento
// y vuelve a cargar.
inline float faseGolpe(float u, float carga) {
    if (u < carga) return 1.0f - suave(u / carga);                 // impacto -> cargado
    const float g = (1.0f - carga) * 0.45f;
    if (u < carga + g) { const float k = (u - carga) / g; return k * k; }   // cargado -> impacto
    return 1.0f;                                                    // impacto, se sostiene
}

// La pose del brazo derecho (y del izquierdo si va a dos manos) en un golpe.
inline void aplicarGolpe(Pose& p, const Golpe& g, float tiempo) {
    float u = tiempo / g.periodo;
    u -= std::floor(u);
    const float k = faseGolpe(u, g.carga);       // 0 cargado .. 1 impacto
    auto mezcla = [k](const float v[2]) { return v[0] + (v[1] - v[0]) * k; };
    Brazo& b = p.brazo[1];
    b.flex = mezcla(g.flex);
    b.abd = mezcla(g.abd);
    b.rot = mezcla(g.rot);
    b.codo = mezcla(g.codo);
    b.munFlex = mezcla(g.muneca);
    b.prono = 0.55f;
    manoRelajada(b, 1.0f);
    b.dedo[4][0] = 0.75f; b.dedo[4][1] = 0.65f; b.dedo[4][2] = 0.55f;
    p.giroTorso += mezcla(g.torso);
    p.inclinacion += g.inclina * k;
    p.apretar = 1.0f;
    if (g.dosManos) {
        // La izquierda agarra el mango mas abajo: sigue al brazo derecho,
        // un poco por detras y mas cerrada.
        Brazo& i = p.brazo[0];
        i.flex = b.flex * 0.85f;
        i.abd = 0.25f;
        i.rot = -0.3f;
        i.codo = b.codo * 0.9f + 0.25f;
        i.munFlex = b.munFlex;
        i.prono = 0.55f;
        manoRelajada(i, 1.0f);
    }
}

// Los golpes de un solo disparo (al aire y a un animal): `t` son los
// segundos desde el clic. Devuelve 0..1 de la pose cargada a la de impacto,
// o -1 si ya termino.
inline float faseDisparo(float t, float cargar, float golpear, float volver) {
    if (t < 0.0f) return -1.0f;
    if (t < cargar) return -suave(t / cargar);                       // 0 -> -1: recoge el brazo
    t -= cargar;
    if (t < golpear) { const float k = t / golpear; return -1.0f + 2.0f * k * k; }  // -1 -> 1
    t -= golpear;
    if (t < volver) return 1.0f - suave(t / volver);                 // 1 -> 0
    return -2.0f;
}

// PEDIDO: "una animacion de golpear al pegarle al aire y animales, pero con
// los animales que sean 3 animaciones donde una es un golpe directo a la
// derecha, otro a la izquierda y uno de abajo a la derecha o izquierda".
//
//   AIRE       un directo de derecha, rapido y sin cargar el cuerpo.
//   ANIMAL 0   directo de derecha con todo el cuerpo: gira el torso y se
//              inclina hacia el golpe.
//   ANIMAL 1   gancho de izquierda: el codo sale de lado y el torso gira al
//              reves.
//   ANIMAL 2   gancho de abajo arriba, con el brazo que toque (`lado`).
inline void aplicarDisparo(Pose& p, Accion a, int variante, int lado, float t) {
    const bool animal = a == Accion::GOLPE_ANIMAL;
    const float f = faseDisparo(t, animal ? 0.10f : 0.07f, animal ? 0.12f : 0.10f, 0.20f);
    if (f < -1.5f) return;
    // c: cuanto esta recogido (0..1); g: cuanto esta en el impacto (0..1).
    const float c = f < 0.0f ? -f : 0.0f;
    const float g = f > 0.0f ? f : 0.0f;
    auto puno = [](Brazo& b) {
        manoRelajada(b, 1.0f);
        b.dedo[4][0] = 0.75f; b.dedo[4][1] = 0.70f; b.dedo[4][2] = 0.60f;
        b.prono = 1.1f;                          // nudillos arriba
    };
    if (!animal || variante == 0) {
        Brazo& b = p.brazo[1];
        puno(b);
        const float fuerza = animal ? 1.0f : 0.7f;
        b.flex = 0.55f + 0.25f * c + 1.0f * g;
        b.abd = 0.15f + 0.10f * c;
        b.codo = 0.5f + 1.5f * c - 0.45f * g;
        b.munFlex = 0.0f;
        p.giroTorso += (-0.20f * c + 0.30f * g) * fuerza;
        p.inclinacion += 0.10f * g * fuerza;
        p.apretar = 1.0f;
        // La otra mano se queda en guardia.
        Brazo& o = p.brazo[0];
        puno(o);
        o.flex = 0.9f; o.codo = 1.9f; o.abd = 0.15f;
    } else if (variante == 1) {
        Brazo& b = p.brazo[0];
        puno(b);
        b.flex = 0.6f + 0.6f * g;
        b.abd = 0.4f + 0.9f * g + 0.2f * c;
        b.rot = -0.2f - 0.7f * g;
        b.codo = 1.7f - 0.2f * g;
        p.giroTorso += 0.25f * c - 0.40f * g;
        p.inclinacion += 0.06f * g;
        Brazo& o = p.brazo[1];
        puno(o);
        o.flex = 0.9f; o.codo = 1.9f; o.abd = 0.15f;
    } else {
        Brazo& b = p.brazo[lado];
        puno(b);
        b.flex = -0.25f * c + 1.95f * g + 0.3f * (1.0f - c - g);
        b.abd = 0.25f;
        b.codo = 1.6f - 0.4f * g;
        b.prono = 0.0f;                          // palma hacia el cuerpo
        const float s = lado == 1 ? 1.0f : -1.0f;
        p.giroTorso += s * (-0.25f * c + 0.20f * g);
        p.inclinacion += 0.25f * c - 0.05f * g;
        Brazo& o = p.brazo[1 - lado];
        puno(o);
        o.flex = 0.9f; o.codo = 1.9f; o.abd = 0.15f;
    }
}

// PEDIDO: "cuando estas agachado crea 3 [animaciones] que se haran
// dependiendo del terreno".
//
//   NATURAL       la mano izquierda baja al suelo y se apoya con la palma,
//                 los dedos abiertos; la derecha descansa en la rodilla.
//   CONSTRUCCION  erguido y recogido: las dos manos sobre las rodillas, como
//                 quien se agacha en un suelo hecho.
//   LIQUIDO       los brazos se abren para no perder el equilibrio y las
//                 manos rozan el agua, con las munecas meciendose.
inline void aplicarAgachado(Pose& p, Superficie s, float tiempo) {
    switch (s) {
        case Superficie::NATURAL: {
            Brazo& i = p.brazo[0];
            i.flex = 1.10f; i.abd = 0.25f; i.codo = 0.25f;
            i.munFlex = -0.95f;                   // la palma plana en el suelo
            i.prono = 1.3f;
            manoRelajada(i, 0.0f);
            for (int d = 0; d < 4; ++d) { i.dedo[d][0] = 0.0f; i.dedo[d][1] = 0.1f; i.abre[d] = 0.3f; }
            Brazo& d = p.brazo[1];
            d.flex = 0.75f; d.abd = 0.15f; d.codo = 0.90f; d.prono = 1.0f;
            p.giroTorso += 0.06f * std::sin(tiempo * 0.5f);
            break;
        }
        case Superficie::CONSTRUCCION:
            for (Brazo& b : p.brazo) {
                b.flex = 0.70f; b.abd = 0.12f; b.codo = 1.00f; b.prono = 1.1f; b.munFlex = 0.2f;
                manoRelajada(b, 0.2f);
            }
            p.inclinacion -= 0.15f;
            break;
        case Superficie::LIQUIDO: {
            const float m = std::sin(tiempo * 1.6f);
            for (int l = 0; l < 2; ++l) {
                Brazo& b = p.brazo[l];
                b.flex = 0.55f; b.abd = 0.95f; b.codo = 0.30f;
                b.prono = 1.2f + 0.25f * (l == 0 ? m : -m);
                b.munFlex = 0.25f * (l == 0 ? m : -m);
                manoRelajada(b, 0.0f);
                for (int d = 0; d < 4; ++d) b.abre[d] = 0.2f;
            }
            break;
        }
    }
}

// PEDIDO: "los niveles al bajar o subirlos tendran animacion propia de como
// elevacion de pies mientras caminas". Al subir, la pierna que va delante se
// levanta (cadera y rodilla) y apoya; al bajar, se estira hacia abajo con la
// punta por delante. `k` 0..1 es el avance del escalon.
inline void aplicarEscalon(Pose& p, float k, bool sube, int pierna) {
    if (k < 0.0f || k > 1.0f) return;
    const float a = std::sin(PI * k);
    Pierna& g = p.pierna[pierna];
    if (sube) {
        g.flex += 0.55f * a;
        g.rodilla += 1.00f * a;
        g.tobillo += 0.20f * a;
        p.pierna[1 - pierna].tobillo -= 0.25f * a;   // la otra empuja con la punta
    } else {
        g.flex += 0.25f * a;
        g.rodilla += 0.15f * a;
        g.tobillo -= 0.45f * a;                       // la punta busca el suelo
        p.pierna[1 - pierna].rodilla += 0.45f * a;   // la de atras se dobla
    }
    p.inclinacion += (sube ? 0.06f : -0.03f) * a;
}

inline Pose poseDe(const Estado& e) {
    Pose p;
    p.enMano = (uint8_t)e.herramienta;
    p.pedernal = e.pedernal;
    float ritmo = e.rapidez / 4.3f;              // 4.3 = andar
    if (ritmo > 1.4f) ritmo = 1.4f;
    const float corre = suave((e.rapidez - 4.6f) / 1.0f);
    const float respira = std::sin(e.faseRespira);
    p.respira = respira;

    // El ciclo de cada pierna: 0 = el talon toca el suelo. La derecha va
    // medio ciclo detras de la izquierda.
    const float t0 = e.fasePaso / (2.0f * PI);
    const float tPierna[2] = { t0 - std::floor(t0), t0 + 0.5f - std::floor(t0 + 0.5f) };

    // TUMBADO: boca abajo, con los antebrazos delante en el suelo y las
    // piernas estiradas con el empeine apoyado. Al moverse, se arrastra: el
    // codo de un lado avanza con la rodilla del otro, que se recoge hacia
    // fuera, como gatea un soldado. El giro que tumba el cuerpo lo pone
    // `resolver`; aqui las articulaciones van respecto al torso.
    if (e.tumbado && !e.volando) {
        p.tumbado = 1.0f;
        const float amp = sujetar(e.rapidez / 0.75f, 0.0f, 1.0f);
        const float c = std::sin(e.fasePaso);
        for (int l = 0; l < 2; ++l) {
            const float lado = l == 0 ? c : -c;
            Brazo& b = p.brazo[l];
            b.flex = 2.30f + 0.35f * amp * lado;      // el brazo hacia la cabeza
            b.codo = 0.60f + 0.35f * amp * lado;
            b.abd = 0.35f;
            b.prono = 1.20f;                          // palmas al suelo
            b.munFlex = -0.25f;
            manoRelajada(b, 0.0f);
            for (int d = 0; d < 4; ++d) b.abre[d] = 0.15f;
            Pierna& g = p.pierna[l];
            const float sube = std::fmax(0.0f, -lado) * amp;
            g.flex = 0.02f + 0.45f * sube;
            g.abd = 0.08f + 0.35f * sube;
            g.rodilla = 0.08f + 0.90f * sube;
            g.tobillo = -0.80f;                       // el empeine en el suelo
        }
        p.respira = respira;
        p.giroTorso = e.giroCuello * 0.35f;
        limitar(p);
        return p;
    }

    if (e.volando) {
        // Volando: piernas sueltas, un poco recogidas, que se mecen.
        for (int l = 0; l < 2; ++l) {
            const float m = std::sin(e.tiempo * 1.3f + l * 1.7f) * 0.06f;
            p.pierna[l].flex = 0.18f + m;
            p.pierna[l].rodilla = 0.35f - m;
            p.pierna[l].tobillo = -0.45f;            // la punta cae
            p.brazo[l].flex = 0.12f + m;
            p.brazo[l].codo = 0.35f;
            manoRelajada(p.brazo[l], 0.1f);
        }
    } else if (!e.enSuelo) {
        // En el aire: rodillas recogidas, brazos arriba para equilibrar y los
        // dedos abiertos, como al saltar de verdad.
        p.pierna[0].flex = 0.55f; p.pierna[1].flex = 0.30f;
        p.pierna[0].rodilla = 0.95f; p.pierna[1].rodilla = 0.65f;
        p.pierna[0].tobillo = p.pierna[1].tobillo = -0.30f;
        for (Brazo& b : p.brazo) {
            b.flex = -0.30f; b.abd = 0.45f; b.codo = 0.45f; b.munFlex = -0.2f;
            manoRelajada(b, 0.0f);
            for (int d = 0; d < 4; ++d) { b.dedo[d][0] = 0.05f; b.dedo[d][1] = 0.12f; b.abre[d] = 0.25f; }
        }
    } else {
        // Andar con las curvas de la marcha humana:
        //   CADERA   flexionada al apoyar el talon, extendida al despegar.
        //   RODILLA  dos picos: uno pequeno al cargar el peso y uno grande al
        //            pasar la pierna por delante.
        //   TOBILLO  baja la punta al apoyar, sube a media zancada y empuja
        //            con fuerza al despegar.
        //   DEDOS    se doblan hacia arriba justo al despegar.
        for (int l = 0; l < 2; ++l) {
            const float t = tPierna[l];
            const float c = std::cos(2.0f * PI * t);
            Pierna& g = p.pierna[l];
            g.flex = ritmo * (0.14f + 0.31f * c) + corre * 0.22f * c + corre * 0.12f;
            g.rodilla = 0.06f + ritmo * (0.28f * campana(t, 0.13f, 0.07f) +
                                         1.00f * campana(t, 0.72f, 0.11f)) +
                        corre * 0.70f * campana(t, 0.75f, 0.12f);
            g.tobillo = ritmo * (-0.10f * campana(t, 0.06f, 0.04f) + 0.17f * campana(t, 0.42f, 0.10f) -
                                 0.33f * campana(t, 0.62f, 0.05f));
            g.dedos = ritmo * 0.55f * campana(t, 0.57f, 0.06f);
        }
        // Cada brazo va con la pierna contraria.
        for (int l = 0; l < 2; ++l) {
            const float c = std::cos(2.0f * PI * tPierna[1 - l]);
            Brazo& b = p.brazo[l];
            b.flex = ritmo * (0.05f + 0.28f * c) * (1.0f + 0.8f * corre);
            b.codo = 0.18f + 0.14f * ritmo + corre * 1.10f + ritmo * 0.12f * c;
            b.abd = 0.10f + 0.04f * corre;
            manoRelajada(b, corre * 0.6f);
        }
        // La pelvis gira con la pierna que avanza; los hombros, al reves.
        p.giroPelvis = 0.08f * ritmo * std::cos(2.0f * PI * tPierna[0]);
        p.giroTorso = -0.6f * p.giroPelvis;
        p.inclinacion = 0.04f * ritmo + 0.12f * corre;

        // Quieto: el peso pasa despacio de un pie a otro.
        if (ritmo < 0.2f) {
            const float peso = std::sin(e.tiempo * 0.37f) * (1.0f - ritmo * 5.0f);
            p.pierna[0].rodilla += 0.06f * (peso > 0 ? peso : 0);
            p.pierna[1].rodilla += 0.06f * (peso < 0 ? -peso : 0);
            p.giroPelvis += 0.03f * peso;
        }
    }

    // Respirar: los hombros suben un poco y los brazos se mecen.
    for (Brazo& b : p.brazo) b.flex += respira * 0.02f;

    // Subir o bajar un nivel: la pierna de delante se levanta o se estira.
    if (e.enSuelo && !e.volando) aplicarEscalon(p, e.escalon, e.escalonSube, e.piernaEscalon);

    if (e.agachado) {
        // Agachado: la pelvis baja sola al ajustar los pies al suelo. Los
        // brazos, segun lo que se pisa.
        p.agachado = 1.0f;
        for (Pierna& g : p.pierna) { g.flex += 1.20f; g.rodilla += 2.00f; g.tobillo += 0.30f; }
        p.inclinacion += 0.45f;
        for (Brazo& b : p.brazo) { b.flex += 0.35f; b.codo += 0.25f; }
        aplicarAgachado(p, e.superficie, e.tiempo);
    }

    // Lo que hace el brazo va lo ultimo: manda sobre andar y agacharse.
    // `golpeando` sin accion es el picar de siempre, con la mano.
    const Accion accion = (e.accion == Accion::NINGUNA && e.golpeando) ? Accion::PICAR : e.accion;
    if (accion == Accion::PICAR)
        aplicarGolpe(p, golpeDe(e.herramienta, e.bloque), e.tiempo);
    else if (accion == Accion::GOLPE_AIRE || accion == Accion::GOLPE_ANIMAL)
        aplicarDisparo(p, accion, e.variante, e.ladoGolpe, e.tAccion);

    // La cabeza: los hombros acompanan un tercio del giro del cuello, que asi
    // nunca pasa de su limite. Al mirar abajo, el cuello se dobla hasta su
    // tope y el resto lo ponen los ojos.
    p.giroTorso += e.giroCuello * 0.35f;
    p.cuelloFlex = sujetar(-e.mirada * PI / 180.0f, 0.0f, Lim::CUELLO_FLEX);

    limitar(p);
    return p;
}

// ----------------------------------------------------------------------------
// EL GIRO DEL CUERPO DETRAS DE LA CABEZA
// ----------------------------------------------------------------------------
// La camara gira libre, pero el cuello no da la vuelta: si la cabeza pasa de
// su limite, arrastra el cuerpo. Al andar el cuerpo se pone de frente, y
// quieto se va enderezando despacio, como hace una persona.
struct GiroCuerpo {
    float yaw = 0.0f;         // grados, como player.yaw
    bool iniciado = false;
    float quieto = 0.0f;

    static float envolver(float g) {
        g = std::fmod(g + 180.0f, 360.0f);
        if (g < 0) g += 360.0f;
        return g - 180.0f;
    }
    // Lo que gira la cabeza sobre el cuerpo se reparte entre el cuello (70) y
    // los hombros (un tercio): 95 en total, ni uno mas.
    static constexpr float LIMITE = 95.0f;

    // Devuelve el giro de la cabeza sobre la pelvis, en radianes.
    float avanzar(float yawCabeza, bool moviendo, float dt) {
        if (dt < 0) dt = 0;
        if (dt > 0.1f) dt = 0.1f;
        if (!iniciado) { yaw = yawCabeza; iniciado = true; }
        float d = envolver(yawCabeza - yaw);
        quieto = moviendo ? 0.0f : quieto + dt;
        float rapidez = 0.0f;
        if (moviendo) rapidez = 8.0f;                              // de frente al andar
        else if (quieto > 0.8f && std::fabs(d) > 15.0f) rapidez = 1.5f;
        const float k = 1.0f - std::exp(-rapidez * dt);
        yaw = envolver(yaw + d * k);
        d = envolver(yawCabeza - yaw);
        // El tope del cuello: arrastra al cuerpo.
        if (d > LIMITE) { yaw = envolver(yawCabeza - LIMITE); d = LIMITE; }
        if (d < -LIMITE) { yaw = envolver(yawCabeza + LIMITE); d = -LIMITE; }
        return d * PI / 180.0f;
    }
};

// ----------------------------------------------------------------------------
// EL ESQUELETO, YA COLOCADO
// ----------------------------------------------------------------------------
struct Junta { V p; Rot r; };

struct Mano {
    // El marco de la mano, ya con la pronacion y la muneca: los dedos hacia
    // -Y, la palma hacia +Z, el dorso hacia -Z.
    Junta muneca;
    Junta dedo[5][4];      // [dedo][0..2] base de cada falange, [3] la punta
    float lado = 1.0f;     // hacia donde queda el pulgar, en X local de la mano
};
struct Pie {
    Junta tobillo;         // el marco del pie: +Z hacia la punta
    V talon, bola;         // apoyos de la planta
    Junta dedo[5][3];      // [0] base, [1] articulacion, [2] punta
};
struct Esqueleto {
    Junta pelvis, pecho, cuello;
    Junta hombro[2];       // r = marco del brazo
    Junta codo[2];         // r = marco del antebrazo, sin la pronacion
    float prono[2] = {};   // el giro que el antebrazo reparte a lo largo
    Mano mano[2];
    Junta cadera[2], rodilla[2];
    Pie pie[2];
    float ojo = M::OJO;    // altura del ojo de la figura en esta pose
};

// El grosor de cada dedo del pie (del gordo al pequeno).
inline float radioDedoPie(int d) { return d == 0 ? 0.0160f : 0.0112f - d * 0.0007f; }

namespace detalle {
inline void resolver(const Pose& p, float subir, Esqueleto& s) {
    const Rot rPelvis = Rot::enY(p.giroPelvis);
    s.pelvis = { { 0, M::CADERA + 0.02f + subir, -M::ATRAS }, rPelvis };
    const Rot rPecho = rPelvis.por(Rot::enY(p.giroTorso)).por(Rot::enX(-p.inclinacion));
    s.pecho = { s.pelvis.p, rPecho };
    s.cuello = { suma(s.pelvis.p, rPecho.aplicar({ 0, M::ESPALDA, 0 })),
                 rPecho.por(Rot::enX(p.cuelloFlex)) };
    s.ojo = s.cuello.p.y + M::OJO_SOBRE_CUELLO;

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Brazo& b = p.brazo[l];
        // Al tomar aire los hombros suben un poco.
        const V hombro = suma(s.pelvis.p, rPecho.aplicar({ sx * M::X_HOMBRO,
                                                           M::HOMBRO + p.respira * 0.005f, 0 }));
        const Rot rBrazo = rPecho.por(Rot::enZ(sx * b.abd)).por(Rot::enX(b.flex))
                                 .por(Rot::enY(sx * b.rot));
        s.hombro[l] = { hombro, rBrazo };
        const V codo = suma(hombro, rBrazo.aplicar({ 0, -M::BRAZO, 0 }));
        const Rot rAnte = rBrazo.por(Rot::enX(b.codo));
        s.codo[l] = { codo, rAnte };
        // La mano se construye en la posicion anatomica: palma delante (+Z) y
        // pulgar hacia fuera. En ella el antebrazo no esta torcido. Con el
        // brazo colgando y relajado el pulgar mira adelante: un cuarto de
        // vuelta, mas lo que este pronada. Ese giro se reparte a lo largo del
        // antebrazo (radio sobre cubito), no de golpe en la muneca.
        s.prono[l] = -sx * (PI * 0.5f + b.prono);
        const V muneca = suma(codo, rAnte.aplicar({ 0, -M::ANTEBRAZO, 0 }));
        const float pulgar = sx;                  // el pulgar, hacia fuera
        const Rot rMano = rAnte.por(Rot::enY(s.prono[l])).por(Rot::enX(b.munFlex))
                               .por(Rot::enZ(pulgar * b.munDesv));
        Mano& m = s.mano[l];
        m.muneca = { muneca, rMano };
        m.lado = pulgar;

        // Los nudillos, en arco: el del medio es el mas largo.
        static const float NX[4] = { 0.030f, 0.010f, -0.011f, -0.030f };
        static const float NY[4] = { -0.093f, -0.095f, -0.090f, -0.083f };
        static const float ABRE[4] = { 1.0f, 0.0f, -1.0f, -1.5f };
        for (int d = 0; d < 5; ++d) {
            V base;
            Rot r;
            if (d < 4) {
                base = suma(muneca, rMano.aplicar({ pulgar * NX[d], NY[d], 0 }));
                r = rMano.por(Rot::enZ(pulgar * ABRE[d] * b.abre[d]));
            } else {
                // El pulgar sale del lado de la palma, abierto y girado sobre
                // si mismo: asi al doblarse cruza hacia el centro de la mano.
                base = suma(muneca, rMano.aplicar({ pulgar * 0.026f, -0.022f, 0.009f }));
                r = rMano.por(Rot::enZ(pulgar * 0.65f)).por(Rot::enY(-pulgar * 0.8f));
            }
            for (int f = 0; f < 3; ++f) {
                r = r.por(Rot::enX(b.dedo[d][f]));    // se dobla hacia la palma (+Z)
                m.dedo[d][f] = { base, r };
                base = suma(base, r.aplicar({ 0, -M::DEDO[d][f], 0 }));
            }
            m.dedo[d][3] = { base, r };
        }
    }

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Pierna& g = p.pierna[l];
        const V cadera = suma(s.pelvis.p, rPelvis.aplicar({ sx * M::X_CADERA, -0.02f, 0 }));
        const Rot rMuslo = rPelvis.por(Rot::enZ(sx * g.abd)).por(Rot::enX(g.flex));
        s.cadera[l] = { cadera, rMuslo };
        const V rodilla = suma(cadera, rMuslo.aplicar({ 0, -M::MUSLO, 0 }));
        const Rot rPierna = rMuslo.por(Rot::enX(-g.rodilla));   // la rodilla dobla hacia atras
        s.rodilla[l] = { rodilla, rPierna };
        const V tobillo = suma(rodilla, rPierna.aplicar({ 0, -M::PIERNA, 0 }));
        const Rot rPie = rPierna.por(Rot::enX(g.tobillo));
        Pie& pie = s.pie[l];
        pie.tobillo = { tobillo, rPie };
        pie.talon = suma(tobillo, rPie.aplicar({ 0, -0.068f, -0.058f }));
        pie.bola = suma(tobillo, rPie.aplicar({ 0, -0.068f, 0.140f }));
        // Los dedos: del gordo (por dentro) al pequeno, en la linea oblicua
        // de las cabezas de los metatarsianos.
        const float dentro = -sx;
        const Rot rDedos = rPie.por(Rot::enX(g.dedos));
        for (int d = 0; d < 5; ++d) {
            const float rad = radioDedoPie(d);
            // Separados lo justo para que se vea el hueco entre ellos.
            V base = suma(tobillo, rPie.aplicar({ dentro * (0.030f - d * 0.0178f),
                                                  -0.070f + rad, 0.150f - d * 0.011f }));
            const float L = M::DEDO_PIE[d];
            pie.dedo[d][0] = { base, rDedos };
            base = suma(base, rDedos.aplicar({ 0, 0, L * 0.55f }));
            const Rot rPunta = rDedos.por(Rot::enX(-0.18f));     // la punta se curva un poco
            pie.dedo[d][1] = { base, rPunta };
            pie.dedo[d][2] = { suma(base, rPunta.aplicar({ 0, 0, L * 0.45f })), rPunta };
        }
    }
}

// Lo mas bajo de la planta: talon, bola y punta de los dedos.
inline float plantaMasBaja(const Esqueleto& s) {
    float y = 1e9f;
    for (const Pie& p : s.pie) {
        y = std::fmin(y, p.talon.y);
        y = std::fmin(y, p.bola.y);
        for (int d = 0; d < 5; ++d) y = std::fmin(y, p.dedo[d][2].p.y - 0.009f);
    }
    return y;
}
} // namespace detalle

namespace detalle {
// Mueve el esqueleto entero: girar alrededor de `piv` y luego subir `dy`.
inline void mover(Esqueleto& s, const Rot& R, V piv, float dy) {
    auto P = [&](V& p) { p = suma(piv, R.aplicar(resta(p, piv))); p.y += dy; };
    auto J = [&](Junta& j) { P(j.p); j.r = R.por(j.r); };
    J(s.pelvis); J(s.pecho); J(s.cuello);
    for (int l = 0; l < 2; ++l) {
        J(s.hombro[l]); J(s.codo[l]); J(s.cadera[l]); J(s.rodilla[l]);
        J(s.mano[l].muneca);
        for (auto& dedo : s.mano[l].dedo) for (Junta& j : dedo) J(j);
        Pie& pie = s.pie[l];
        J(pie.tobillo); P(pie.talon); P(pie.bola);
        for (auto& dedo : pie.dedo) for (Junta& j : dedo) J(j);
    }
    s.ojo = s.cuello.p.y + M::OJO_SOBRE_CUELLO;
}
// Lo que toca el suelo estando tumbado: pecho, pelvis, codos, rodillas,
// manos y pies, cada uno con su grosor.
inline float apoyoTumbado(const Esqueleto& s) {
    float y = suma(s.pelvis.p, s.pecho.r.aplicar({ 0, 0.36f, 0.115f })).y;
    y = std::fmin(y, suma(s.pelvis.p, s.pelvis.r.aplicar({ 0, -0.02f, 0.105f })).y);
    for (int l = 0; l < 2; ++l) {
        y = std::fmin(y, s.codo[l].p.y - 0.037f);
        y = std::fmin(y, s.rodilla[l].p.y - 0.050f);
        y = std::fmin(y, s.mano[l].muneca.p.y - 0.020f);
        for (int d = 0; d < 5; ++d) y = std::fmin(y, s.mano[l].dedo[d][3].p.y - 0.009f);
        y = std::fmin(y, s.pie[l].tobillo.p.y - 0.035f);
        for (int d = 0; d < 5; ++d) y = std::fmin(y, s.pie[l].dedo[d][2].p.y - 0.009f);
    }
    return y;
}
} // namespace detalle

// Cuanto se tumba el cuerpo: casi horizontal (85 grados).
constexpr float GIRO_TUMBADO = 1.48f;

// `apoyar`: con los pies en el suelo, la figura sube o baja hasta que lo mas
// bajo de la planta toca y=0. Es lo que hace que agacharse baje el cuerpo y
// que al andar se bote sobre el pie que apoya, sin inventar el bote.
// Tumbado, el cuerpo entero gira sobre la pelvis hasta quedar boca abajo y
// lo que se apoya es el pecho, los codos y las rodillas.
inline Esqueleto resolver(const Pose& p, bool apoyar = true) {
    Esqueleto s;
    detalle::resolver(p, 0.0f, s);
    if (p.tumbado > 0.0f) {
        // Con giro en X negativo la cabeza va hacia +Z y el pecho mira al suelo.
        detalle::mover(s, Rot::enX(-GIRO_TUMBADO * p.tumbado), s.pelvis.p, 0.0f);
        if (apoyar) detalle::mover(s, Rot::identidad(), s.pelvis.p, -detalle::apoyoTumbado(s));
        return s;
    }
    if (apoyar) {
        const float baja = detalle::plantaMasBaja(s);
        detalle::resolver(p, -baja, s);
    }
    return s;
}

// Agachado, la camara del motor baja hasta 0.80 y la figura no tanto: se
// aplasta en vertical lo que falte para que el ojo siga encima de los hombros
// y no dentro del pecho.
inline float escalaVertical(const Esqueleto& s, float alturaOjoCamara) {
    const float k = alturaOjoCamara / s.ojo;
    return k > 1.0f ? 1.0f : (k < 0.4f ? 0.4f : k);
}

// ============================================================================
// EL PULSO Y LA RESPIRACION
// ============================================================================
// Las venas se hinchan con el esfuerzo y el calor, y la arteria de la muneca
// late. Reposo: 68 pulsaciones y 14 respiraciones por minuto; corriendo o
// picando sube hasta 140 y 32, y baja despacio al parar.
struct Fisiologia {
    float esfuerzo = 0.0f;     // 0..1, sigue a la actividad en unos segundos
    float calor = 0.0f;        // 0..1, sube en ~20 s de esfuerzo y baja en ~40
    float fasePulso = 0.0f;    // 0..1
    float faseRespira = 0.0f;  // radianes

    float bpm() const { return 68.0f + 72.0f * esfuerzo; }
    float respiraciones() const { return 14.0f + 18.0f * esfuerzo; }

    void avanzar(float dt, float rapidez, bool golpeando) {
        if (dt < 0) dt = 0;
        if (dt > 0.1f) dt = 0.1f;
        float objetivo = sujetar((rapidez - 1.0f) / 5.0f, 0.0f, 1.0f);
        if (golpeando && objetivo < 0.7f) objetivo = 0.7f;
        // Sube rapido y baja despacio, como el corazon.
        const float k = objetivo > esfuerzo ? 0.35f : 0.08f;
        esfuerzo += (objetivo - esfuerzo) * (1.0f - std::exp(-k * dt));
        const float kc = esfuerzo > calor ? 0.05f : 0.025f;
        calor += (esfuerzo - calor) * (1.0f - std::exp(-kc * dt));
        fasePulso += dt * bpm() / 60.0f;
        fasePulso -= std::floor(fasePulso);
        faseRespira = std::fmod(faseRespira + dt * respiraciones() / 60.0f * 2.0f * PI, 2.0f * PI);
    }
    // La onda del pulso: un golpe seco y un rebote pequeno (la onda dicrota).
    float latido() const {
        return campana(fasePulso, 0.10f, 0.05f) + 0.4f * campana(fasePulso, 0.32f, 0.07f);
    }
};

// ============================================================================
// EL PARPADEO
// ============================================================================
// Una persona parpadea unas 15 veces por minuto, y cada parpadeo dura de 0,1 a
// 0,4 s. Aqui: cada 2,8 a 6 s al azar, 0,18 s en total (cierra 60 ms, se queda
// 30 ms, abre 90 ms): se nota sin estorbar.
struct Parpadeo {
    float proximo = 3.0f;    // segundos hasta el siguiente
    float fase = -1.0f;      // <0: abierto; si no, tiempo dentro del parpadeo
    uint32_t semilla = 0x9E3779B9u;

    static constexpr float CIERRA = 0.06f, QUIETO = 0.03f, ABRE = 0.09f;
    static constexpr float DURA = CIERRA + QUIETO + ABRE;

    float azar() {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        return (semilla & 0xFFFFu) / 65535.0f;
    }

    // Devuelve cuanto esta cerrado el ojo: 0 abierto, 1 cerrado del todo.
    float avanzar(float dt) {
        if (dt < 0) dt = 0;
        if (dt > 0.1f) dt = 0.1f;                 // un tiron no lo deja cerrado
        if (fase < 0.0f) {
            proximo -= dt;
            if (proximo > 0.0f) return 0.0f;
            fase = 0.0f;
        } else {
            fase += dt;
        }
        if (fase >= DURA) {
            fase = -1.0f;
            proximo = 2.8f + 3.2f * azar();
            return 0.0f;
        }
        if (fase < CIERRA) return fase / CIERRA;
        if (fase < CIERRA + QUIETO) return 1.0f;
        return 1.0f - (fase - CIERRA - QUIETO) / ABRE;
    }
};

// Los dos parpados se mueven A LA VEZ: el de arriba recorre mas (baja hasta el
// 56 % de la pantalla) y el de abajo menos (sube hasta ahi), como en un ojo de
// verdad, y se encuentran un poco por debajo del centro. El borde es curvo:
// en el centro el parpado llega antes que en los lados.
struct Parpados { float arriba, abajo; };   // fraccion de pantalla cubierta en el centro
constexpr float ENCUENTRO = 0.56f;
inline float curvaParpado(float x01) {
    const float u = x01 * 2.0f - 1.0f;
    return 0.12f * u * u;
}
inline Parpados parpados(float cierre) {
    cierre = sujetar(cierre, 0.0f, 1.0f);
    // Lo que se retrasan los lados por la curva se compensa para que, cerrado
    // del todo, no quede rendija en las esquinas.
    const float c = suave(cierre);
    const float extra = 1.0f + curvaParpado(0.0f) / (1.0f - ENCUENTRO);
    return { ENCUENTRO * c * extra, (1.0f - ENCUENTRO) * c * extra };
}

} // namespace ModeloJugador
