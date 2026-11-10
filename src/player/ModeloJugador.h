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
    constexpr float CUELLO_LADO  = 0.70f;   // 40 de la oreja hacia el hombro
    constexpr float TORSO_GIRO   = 0.52f;   // 30 de los hombros sobre la pelvis
    constexpr float TORSO_LADO   = 0.61f;   // 35 de flexion lateral de la columna
    constexpr float INCLINA_MIN = -0.25f, INCLINA_MAX = 1.10f;
    // La escapula: sube (encoger) y va hacia delante o atras sobre las
    // costillas. En metros, no en angulo.
    constexpr float ENCOGER_M = 0.040f, ADELANTAR_M = 0.030f;

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
    constexpr float CADERA_ROT_MIN  = -0.61f, CADERA_ROT_MAX  = 0.79f;   // 35 interna, 45 externa
    constexpr float RODILLA_MAX     = 2.40f;
    constexpr float TOBILLO_MIN = -0.87f, TOBILLO_MAX = 0.35f;   // punta abajo / arriba
    constexpr float TOBILLO_LADO_MIN = -0.26f, TOBILLO_LADO_MAX = 0.52f;   // eversion 15, inversion 30
    constexpr float DEDOS_PIE_MIN = -0.70f, DEDOS_PIE_MAX = 1.10f;

    // ⭐ LOS MUSCULOS QUE CRUZAN DOS ARTICULACIONES
    // Un musculo biarticular no da para estirarse en las dos a la vez:
    //   ISQUIOTIBIALES  con la rodilla recta la cadera solo flexiona ~80
    //                   grados (la prueba de la pierna recta); doblada, 120.
    //   GEMELOS         con la rodilla recta el pie sube ~10 grados; doblada,
    //                   el soleo solo deja ~20.
    //   RECTO FEMORAL   con la cadera hacia atras la rodilla dobla menos (la
    //                   prueba de Ely).
    //   FALANGES        la ultima falange no se dobla sola: va con la del
    //                   medio (comparten el flexor profundo).
    constexpr float CADERA_FLEX_RODILLA_RECTA = 1.40f;
    constexpr float TOBILLO_MAX_RODILLA_RECTA = 0.17f;
    constexpr float RODILLA_MAX_CADERA_ATRAS  = 1.90f;
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
    float encoger = 0.0f;              // 0..1: la escapula sube hacia la oreja
    float adelantar = 0.0f;            // -1..1: la escapula hacia delante (+) o atras
};
struct Pierna {
    float flex = 0.0f, abd = 0.03f;
    float rot = 0.0f;                  // + = rotacion externa (la punta hacia fuera)
    float rodilla = 0.05f;
    float tobillo = 0.0f;
    float tobilloLado = 0.0f;          // + = inversion (la planta mira hacia dentro)
    float dedos = 0.0f;                // + = dedos hacia arriba (extension)
};
// ⭐ LAS PROPORCIONES: el largo de cada hueso respecto al de M (1 = el de
// una persona media de 1.80). Las pone el avatar.
struct Proporciones {
    float torso = 1.0f, brazo = 1.0f, antebrazo = 1.0f, muslo = 1.0f, pierna = 1.0f;
    float hombroY = 1.0f;   // la altura del hombro sobre la pelvis (hombros caidos < 1)
    float mano = 1.0f;      // el tamano de la mano: palma, nudillos y falanges
};
struct Pose {
    Brazo brazo[2];
    Pierna pierna[2];
    Proporciones prop;
    float flexibilidad = 1.0f;         // multiplica los rangos blandos (0.7 rigido .. 1.25 muy flexible)
    float inclinacion = 0.0f;          // el torso hacia delante
    float giroTorso = 0.0f;            // los hombros sobre la pelvis
    float torsoLado = 0.0f;            // + = la columna se dobla hacia la derecha
    float giroPelvis = 0.0f;
    // + = la pelvis se inclina hacia delante sobre las cabezas de los
    // femures: el tronco entero baja y las piernas siguen donde estaban (es
    // como se dobla una persona de pie). Cuenta como flexion de cadera.
    float pelvisInclina = 0.0f;
    float respira = 0.0f;              // -1..1
    float cuelloFlex = 0.0f;           // la cabeza hacia el pecho
    float cuelloGiro = 0.0f;           // + = la cara hacia la derecha
    float cuelloLado = 0.0f;           // + = la oreja derecha hacia el hombro
    float agachado = 0.0f;
    float tumbado = 0.0f;              // 0..1: boca abajo, el torso en el suelo
    float apretar = 0.0f;              // 0..1, cuanto aprieta el puno derecho
    float levantaPecho = 0.0f;         // tumbado: 0 pegado al suelo .. 1 apoyado en los codos
    float pelvisLado = 0.0f;           // la cadera cae hacia un lado al andar (radianes)
    uint8_t enMano = 0;                // Herramienta que se dibuja en la derecha
    bool pedernal = false;             // cabeza de pedernal (si no, de piedra)
};

// Los topes de hueso (el codo y la rodilla al estirarse, el codo al doblarse)
// son fijos; los de partes blandas (capsulas, ligamentos, musculos) se
// abren o cierran con la flexibilidad de cada persona.
inline void limitar(Pose& p) {
    using namespace Lim;
    p.flexibilidad = sujetar(p.flexibilidad, 0.6f, 1.3f);
    const float k = p.flexibilidad;
    auto suave = [](float x) { x = sujetar(x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); };
    auto blando = [k](float v, float lo, float hi) { return sujetar(v, lo * k, hi * k); };
    p.inclinacion = blando(p.inclinacion, INCLINA_MIN, INCLINA_MAX);
    p.giroTorso = blando(p.giroTorso, -TORSO_GIRO, TORSO_GIRO);
    p.torsoLado = blando(p.torsoLado, -TORSO_LADO, TORSO_LADO);
    p.cuelloFlex = blando(p.cuelloFlex, -CUELLO_FLEX, CUELLO_FLEX);
    p.cuelloGiro = blando(p.cuelloGiro, -CUELLO_GIRO, CUELLO_GIRO);
    p.cuelloLado = blando(p.cuelloLado, -CUELLO_LADO, CUELLO_LADO);
    for (Brazo& b : p.brazo) {
        b.flex = blando(b.flex, HOMBRO_FLEX_MIN, HOMBRO_FLEX_MAX);
        b.abd = sujetar(b.abd, HOMBRO_ABD_MIN, HOMBRO_ABD_MAX * k);
        b.rot = blando(b.rot, HOMBRO_ROT_MIN, HOMBRO_ROT_MAX);
        b.codo = sujetar(b.codo, 0.0f, CODO_MAX);
        b.prono = sujetar(b.prono, PRONO_MIN, PRONO_MAX);
        b.munFlex = blando(b.munFlex, MUNECA_FLEX_MIN, MUNECA_FLEX_MAX);
        b.munDesv = sujetar(b.munDesv, MUNECA_DESV_MIN, MUNECA_DESV_MAX);
        b.encoger = sujetar(b.encoger, 0.0f, 1.0f);
        b.adelantar = sujetar(b.adelantar, -1.0f, 1.0f);
        for (int d = 0; d < 5; ++d) {
            const auto& L = d == 4 ? PULGAR : DEDO;
            for (int f = 0; f < 3; ++f) b.dedo[d][f] = sujetar(b.dedo[d][f], L[f][0], L[f][1]);
            // La ultima falange acompana a la del medio: sola se dobla poco.
            if (d < 4) b.dedo[d][2] = std::fmin(b.dedo[d][2], 0.35f + 0.75f * std::fmax(0.0f, b.dedo[d][1]));
            b.abre[d] = sujetar(b.abre[d], DEDO_ABRE_MIN, DEDO_ABRE_MAX);
        }
    }
    // Isquiotibiales: cuanto flexiona la cadera depende de la rodilla. La
    // inclinacion de la pelvis es flexion de las dos caderas a la vez, asi
    // que se frena con la pierna que menos deja.
    auto flexMaxDe = [&](const Pierna& g) {
        const float doblada = suave(sujetar(g.rodilla, 0.0f, RODILLA_MAX) / 1.4f);
        return (CADERA_FLEX_RODILLA_RECTA + (CADERA_FLEX_MAX - CADERA_FLEX_RODILLA_RECTA) * doblada) * k;
    };
    float inclinaMax = 1e9f;
    for (const Pierna& g : p.pierna) inclinaMax = std::fmin(inclinaMax, flexMaxDe(g) - std::fmax(0.0f, g.flex));
    p.pelvisInclina = sujetar(p.pelvisInclina, -0.26f * k, std::fmax(0.0f, inclinaMax));
    for (Pierna& g : p.pierna) {
        g.rodilla = sujetar(g.rodilla, 0.0f, RODILLA_MAX);
        g.flex = sujetar(g.flex, CADERA_FLEX_MIN * k - p.pelvisInclina, flexMaxDe(g) - p.pelvisInclina);
        // Recto femoral: con la cadera atras la rodilla no llega al tope.
        const float atras = suave(-(g.flex + p.pelvisInclina) / 0.35f);
        g.rodilla = std::fmin(g.rodilla, RODILLA_MAX + (RODILLA_MAX_CADERA_ATRAS - RODILLA_MAX) * atras);
        g.abd = blando(g.abd, CADERA_ABD_MIN, CADERA_ABD_MAX);
        g.rot = blando(g.rot, CADERA_ROT_MIN, CADERA_ROT_MAX);
        // Gemelos: con la rodilla recta el pie sube menos.
        const float tobMax = TOBILLO_MAX_RODILLA_RECTA + (TOBILLO_MAX - TOBILLO_MAX_RODILLA_RECTA) * suave(g.rodilla / 0.8f);
        g.tobillo = sujetar(g.tobillo, TOBILLO_MIN, tobMax * k);
        g.tobilloLado = sujetar(g.tobilloLado, TOBILLO_LADO_MIN, TOBILLO_LADO_MAX);
        g.dedos = sujetar(g.dedos, DEDOS_PIE_MIN, DEDOS_PIE_MAX);
    }
}

// ----------------------------------------------------------------------------
// LO QUE HACE EL JUGADOR, PARA ELEGIR LA ANIMACION
// ----------------------------------------------------------------------------
// El suelo bajo los pies (cambia como se agacha).
enum class Superficie : uint8_t { NATURAL, CONSTRUCCION, LIQUIDO };
// Lo que hace el brazo.
enum class Accion : uint8_t { NINGUNA, PICAR, GOLPE_AIRE, GOLPE_ANIMAL, COLOCAR };
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
    float tRecoger = -1;     // segundos desde la F; <0 nada
    int saltoVariante = 0;   // cual de los 3 saltos
    float tAire = 0;         // segundos desde que despego
    bool sigilo = false;     // CTRL una vez y W: andar despacio
    bool inspeccion = false; // ...y SHIFT: medio agachado
    float quieto = 0;        // segundos de pie sin hacer nada (gestos de reposo)

    // --- Reacciones del cuerpo ---
    float aterrizaje = -1;   // segundos desde que toco el suelo tras caer; <0 nada
    float fuerzaCaida = 0;   // 0..1: lo fuerte del golpe al aterrizar
    float aceleracion = 0;   // bloques/s^2 hacia delante (+ arranca, - frena)
    float giroVel = 0;       // grados/s que gira el cuerpo sobre si mismo
    float faseGiro = 0;      // avanza con lo girado: un paso cada ~30 grados
    uint32_t estilo = 0;     // la manera de moverse de este personaje
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

// Un numero de -1 a 1 que depende solo de (semilla, canal). Es lo que hace que
// cada golpe, cada punetazo y cada gesto salgan un poco distintos sin dejar de
// ser el mismo movimiento: como una persona, que nunca repite un gesto igual.
inline float ruido(uint32_t semilla, uint32_t canal) {
    uint32_t h = semilla * 2654435761u ^ (canal + 0x9E3779B9u) * 2246822519u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 32767.5f - 1.0f;
}

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
    // PEDIDO: "haz mas realistas las animaciones del pico, hacha y martillo
    // para que sean como en la vida real [...] que la animacion del pico sea
    // de el brazo derecho moviendo y golpeando el bloque con el pico, solo
    // con el brazo".
    //
    //   MANO      romper con las manos: punetazos de verdad, con el puno.
    //   PICO      a UNA mano, el brazo derecho: sube el pico por encima del
    //             hombro con el codo doblado y la muneca echada atras, y baja
    //             estirando codo y muneca a la vez (el latigazo que da la
    //             fuerza). El torso apenas acompana.
    //   HACHA     en la madera, el hachazo en diagonal a dos manos con giro de
    //             cadera; en lo demas, a una mano y mas corto.
    //   MARTILLO  como se martillea: el hombro casi quieto, el trabajo lo hacen
    //             el codo y la muneca; en piedra, el mazazo de arriba.
    //   AZADA     cavar: de arriba a abajo, tirando hacia el cuerpo.
    //
    //                     periodo carga   flex           abd           rot            codo          muneca         torso          incl  2manos
    static const Golpe T[(int)Herramienta::NUM][(int)ClaseBloque::NUM] = {
        { // MANO
            { 0.42f, 0.55f, { 2.00f, 0.95f }, { 0.12f, 0.10f }, { 0.15f, 0.10f }, { 1.90f, 0.55f }, { -0.3f,  0.4f }, { -0.10f, 0.12f }, 0.08f, false }, // tierra: puno de arriba abajo
            { 0.40f, 0.55f, { 1.20f, 1.05f }, { 0.85f, 0.20f }, { -0.6f,  0.30f }, { 1.80f, 1.00f }, {  0.0f,  0.2f }, { -0.30f, 0.22f }, 0.04f, false }, // madera: puno de martillo de lado
            { 0.46f, 0.58f, { 0.75f, 1.45f }, { 0.18f, 0.10f }, { 0.00f, 0.00f }, { 2.10f, 0.12f }, {  0.0f,  0.0f }, { -0.28f, 0.28f }, 0.10f, false }, // piedra: directo
            { 0.30f, 0.50f, { 1.30f, 1.20f }, { 0.20f, 0.18f }, { 0.10f, 0.10f }, { 1.50f, 1.05f }, { -0.2f,  0.3f }, { -0.06f, 0.06f }, 0.02f, false }, // construido: golpecitos
        },
        { // PICO: solo el brazo derecho
            { 0.60f, 0.62f, { 2.45f, 0.80f }, { 0.25f, 0.12f }, { 0.30f, 0.05f }, { 1.90f, 0.35f }, { -0.85f, 0.50f }, { -0.08f, 0.06f }, 0.08f, false }, // tierra
            { 0.58f, 0.60f, { 1.90f, 0.95f }, { 1.00f, 0.30f }, { -0.55f, 0.20f }, { 1.85f, 0.40f }, { -0.70f, 0.45f }, { -0.15f, 0.10f }, 0.05f, false }, // madera: en diagonal
            { 0.66f, 0.64f, { 2.80f, 0.65f }, { 0.20f, 0.10f }, { 0.30f, 0.05f }, { 2.05f, 0.25f }, { -0.95f, 0.60f }, { -0.10f, 0.08f }, 0.12f, false }, // piedra: desde lo alto
            { 0.46f, 0.56f, { 1.95f, 1.10f }, { 0.20f, 0.14f }, { 0.20f, 0.08f }, { 1.70f, 0.75f }, { -0.55f, 0.30f }, { -0.04f, 0.04f }, 0.03f, false }, // construido: controlado
        },
        { // HACHA
            { 0.58f, 0.60f, { 2.30f, 0.80f }, { 0.25f, 0.12f }, { 0.30f, 0.08f }, { 1.80f, 0.35f }, { -0.80f, 0.45f }, { -0.10f, 0.08f }, 0.08f, false }, // tierra
            { 0.66f, 0.60f, { 1.55f, 1.15f }, { 1.30f, 0.15f }, { -0.95f, 0.40f }, { 1.40f, 0.20f }, { -0.50f, 0.30f }, { -0.60f, 0.45f }, 0.14f, true  }, // madera: hachazo en diagonal
            { 0.66f, 0.62f, { 2.55f, 0.75f }, { 0.20f, 0.10f }, { 0.30f, 0.08f }, { 1.95f, 0.30f }, { -0.85f, 0.50f }, { -0.10f, 0.08f }, 0.10f, false }, // piedra
            { 0.46f, 0.55f, { 1.70f, 1.05f }, { 0.60f, 0.30f }, { -0.30f, 0.10f }, { 1.60f, 0.65f }, { -0.50f, 0.30f }, { -0.12f, 0.06f }, 0.03f, false }, // construido
        },
        { // MARTILLO: codo y muneca
            { 0.48f, 0.58f, { 1.55f, 0.95f }, { 0.15f, 0.10f }, { 0.15f, 0.10f }, { 2.20f, 0.70f }, { -0.85f, 0.45f }, { -0.05f, 0.05f }, 0.08f, false }, // tierra: apisonar
            { 0.42f, 0.56f, { 1.35f, 1.10f }, { 0.25f, 0.18f }, { 0.10f, 0.05f }, { 2.25f, 0.85f }, { -0.90f, 0.45f }, { -0.03f, 0.03f }, 0.03f, false }, // madera: clavar
            { 0.62f, 0.62f, { 2.50f, 0.75f }, { 0.15f, 0.10f }, { 0.20f, 0.08f }, { 1.90f, 0.35f }, { -0.90f, 0.55f }, { -0.12f, 0.10f }, 0.14f, false }, // piedra: mazazo
            { 0.30f, 0.50f, { 1.25f, 1.12f }, { 0.20f, 0.16f }, { 0.10f, 0.08f }, { 2.10f, 1.25f }, { -0.75f, 0.35f }, { -0.02f, 0.02f }, 0.01f, false }, // construido: golpecitos
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
// `apunta` (radianes, + arriba) es hacia donde mira: el golpe va ahi. Un
// bloque a los pies se golpea con el brazo mas bajo y el cuerpo inclinado; uno
// por encima de la cabeza, con el brazo alto.
inline void aplicarGolpe(Pose& p, const Golpe& g, float tiempo, float apunta = 0.0f,
                         uint32_t semilla = 0) {
    float u = tiempo / g.periodo;
    const uint32_t numero = (uint32_t)std::floor(u);   // que golpe de la serie es
    u -= std::floor(u);
    // Cada golpe de la serie, ligeramente distinto: unos cargan mas, otros
    // golpean un poco mas abajo, la muneca no remata siempre igual.
    const uint32_t sg = semilla * 7919u + numero;
    const float cargaF = sujetar(g.carga + 0.04f * ruido(sg, 1), 0.3f, 0.8f);
    // ⭐ LA CADENA DEL GOLPE, del cuerpo a la herramienta. Un golpe de verdad
    // no mueve todo a la vez: arrancan la cadera y el tronco, luego el hombro,
    // luego el codo y la muneca al final -- el latigazo que da la fuerza. Cada
    // eslabon va un poco por detras del anterior.
    auto fase = [&](float desfase) { float uu = u + desfase; uu -= std::floor(uu); return faseGolpe(uu, cargaF); };
    const float kTronco = fase(0.06f), kHombro = fase(0.025f), k = fase(0.0f), kMuneca = fase(-0.035f);
    // El rebote al dar: la herramienta se frena de golpe y vibra un instante.
    const float tImpacto = (u - (cargaF + (1.0f - cargaF) * 0.45f)) * g.periodo;
    const float rebote = tImpacto > 0.0f ? std::sin(tImpacto * 42.0f) * std::exp(-tImpacto * 16.0f) : 0.0f;
    const float carga = 1.0f + 0.10f * ruido(sg, 2);      // mas o menos arriba
    const float impacto = 0.07f * ruido(sg, 3);           // un poco mas alto o bajo
    auto mezclaEn = [](float kk, const float v[2]) { return v[0] + (v[1] - v[0]) * kk; };
    auto mezcla = [&](const float v[2]) { return mezclaEn(k, v); };
    auto mezclaV = [carga, impacto](float kk, const float v[2]) {
        return v[0] * carga + (v[1] + impacto - v[0] * carga) * kk;
    };
    Brazo& b = p.brazo[1];
    b.flex = mezclaV(kHombro, g.flex) - 0.06f * rebote;
    b.abd = mezclaEn(kHombro, g.abd) + 0.05f * ruido(sg, 4);
    b.rot = mezclaEn(kHombro, g.rot);
    b.codo = mezclaV(k, g.codo);
    b.munFlex = mezclaEn(kMuneca, g.muneca) + 0.10f * ruido(sg, 5) * kMuneca - 0.15f * rebote;
    // El cuerpo entra en el golpe: las rodillas ceden un poco al dar, la
    // pelvis gira con el tronco y la cabeza acompana hacia el blanco.
    for (Pierna& pg : p.pierna) pg.rodilla += 0.04f + 0.10f * kTronco;
    p.giroPelvis += 0.35f * mezclaEn(kTronco, g.torso);
    p.pelvisInclina += 0.05f * kTronco;
    p.cuelloFlex += 0.08f * k;
    b.prono = 0.55f;
    manoRelajada(b, 1.0f);
    b.dedo[4][0] = 0.75f; b.dedo[4][1] = 0.65f; b.dedo[4][2] = 0.55f;
    p.giroTorso += mezclaEn(kTronco, g.torso) * (1.0f + 0.15f * ruido(sg, 6));
    p.inclinacion += g.inclina * kTronco;
    p.apretar = 1.0f;
    // El brazo libre acompana el golpe para equilibrar: baja cuando el otro
    // sube y se adelanta un poco en el impacto.
    if (!g.dosManos) {
        Brazo& o = p.brazo[0];
        o.flex += 0.25f * k - 0.10f * (1.0f - k);
        o.abd += 0.08f;
        o.codo += 0.25f;
    }
    // Hacia la mirada: el impacto entero; la carga, la mitad.
    b.flex += apunta * (0.45f + 0.45f * k);
    if (apunta < 0.0f) p.inclinacion += -apunta * 0.25f * k;
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
//
// PEDIDO (despues): "2 animaciones de golpear al aire, que extiende su brazo
// derecho y luego el izquierdo y viceversa y que se adapte a la mirada, o
// sea, si ves arriba, golpeas arriba".
//
//   AIRE 0 / 1  directo de derecha / de izquierda, alternando. El hombro
//               apunta a donde se mira: arriba, al frente o abajo (con el
//               cuerpo inclinado).
inline void aplicarDisparo(Pose& p, Accion a, int variante, int lado, float t, float apunta = 0.0f) {
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
    if (!animal) {
        // `variante`: bit 0 el lado (0 derecha, 1 izquierda); bits 1-2 el tipo:
        //   0 DIRECTO   el brazo sale recto, el hombro empuja.
        //   1 CRUZADO   igual pero con todo el cuerpo: giro de cadera y paso.
        //   2 GANCHO    el codo sale de lado a 90 grados y el torso barre.
        //   3 UPPERCUT  de abajo arriba, doblando las rodillas antes.
        const int l = (variante & 1) == 0 ? 1 : 0;
        const int tipo = (variante >> 1) & 3;
        const float s = l == 1 ? 1.0f : -1.0f;
        Brazo& b = p.brazo[l];
        puno(b);
        switch (tipo) {
            case 0: case 1: {
                const float cuerpo = tipo == 1 ? 1.8f : 1.0f;
                b.flex = 0.55f + 0.25f * c + (1.0f + apunta) * g;
                b.abd = 0.15f + 0.10f * c;
                b.codo = 0.5f + 1.5f * c - 0.45f * g;
                b.munFlex = 0.0f;
                p.giroTorso += s * (-0.18f * c + 0.28f * g) * cuerpo;
                p.inclinacion += 0.08f * g * cuerpo + (apunta < 0.0f ? -apunta * 0.35f * g : 0.0f);
                if (tipo == 1) p.pierna[1 - l].flex += 0.20f * g;     // el paso
                break;
            }
            case 2:
                b.flex = 0.6f + (0.6f + apunta * 0.8f) * g;
                b.abd = 0.4f + 0.9f * g + 0.2f * c;
                b.rot = -0.2f - 0.7f * g;
                b.codo = 1.7f - 0.2f * g;
                p.giroTorso += s * (0.25f * c - 0.40f * g);
                p.inclinacion += 0.06f * g;
                break;
            default:
                b.flex = -0.25f * c + (1.95f + apunta * 0.5f) * g + 0.3f * (1.0f - c - g);
                b.abd = 0.25f;
                b.codo = 1.6f - 0.4f * g;
                b.prono = 0.0f;
                p.giroTorso += s * (-0.25f * c + 0.20f * g);
                p.inclinacion += 0.25f * c - 0.05f * g;
                for (Pierna& pi : p.pierna) { pi.rodilla += 0.35f * c; pi.flex += 0.15f * c; }
                break;
        }
        p.apretar = l == 1 ? 1.0f : p.apretar;
        Brazo& o = p.brazo[1 - l];               // la otra, en guardia
        puno(o);
        o.flex = 0.9f; o.codo = 1.9f; o.abd = 0.15f;
        return;
    }
    if (variante == 0) {
        Brazo& b = p.brazo[1];
        puno(b);
        const float fuerza = animal ? 1.0f : 0.7f;
        b.flex = 0.55f + 0.25f * c + (1.0f + apunta) * g;
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
    // Las manos de una persona agachada no quedan rigidas: la muneca cae, los
    // dedos se curvan solos y el antebrazo descansa sobre el muslo.
    switch (s) {
        case Superficie::NATURAL: {
            // La izquierda cuelga suelta por fuera de la rodilla, baja, con la
            // muneca caida y los dedos relajados rozando el suelo. (Antes se
            // apoyaba en el suelo con la palma y quedaba demasiado arriba.)
            Brazo& i = p.brazo[0];
            i.flex = 0.62f; i.abd = 0.22f; i.codo = 0.30f;
            i.munFlex = 0.35f;
            i.prono = 0.9f;
            manoRelajada(i, 0.2f);
            // La derecha: el antebrazo sobre el muslo y la mano colgando.
            Brazo& d = p.brazo[1];
            d.flex = 0.95f; d.abd = 0.12f; d.codo = 0.75f; d.prono = 1.25f; d.munFlex = 0.65f;
            manoRelajada(d, 0.25f);
            p.giroTorso += 0.05f * std::sin(tiempo * 0.5f);
            break;
        }
        case Superficie::CONSTRUCCION: {
            // El antebrazo derecho sobre el muslo y la mano suelta delante de
            // la rodilla; la izquierda, baja y colgando entre las rodillas.
            Brazo& d = p.brazo[1];
            d.flex = 0.95f; d.abd = 0.14f; d.codo = 0.70f; d.prono = 1.25f; d.munFlex = 0.70f;
            manoRelajada(d, 0.30f);
            Brazo& i = p.brazo[0];
            i.flex = 0.70f; i.abd = 0.05f; i.codo = 0.40f; i.prono = 1.1f; i.munFlex = 0.45f;
            manoRelajada(i, 0.25f);
            i.munFlex += 0.08f * std::sin(tiempo * 0.7f);
            p.inclinacion -= 0.10f;
            break;
        }
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

// ============================================================================
// COLOCAR UN BLOQUE O UN ITEM
// ============================================================================
// PEDIDO: "una animacion para todos los bloques e items de colocarlos [...]
// con el brazo derecho". El brazo va hacia donde se mira, la mano se abre y
// deja la pieza con un giro de muneca, y vuelve. COLOCAR_SEG en total.
constexpr float COLOCAR_SEG = 0.36f;

inline void aplicarColocar(Pose& p, float t, float apunta) {
    if (t < 0.0f || t > COLOCAR_SEG) return;
    const float u = t / COLOCAR_SEG;
    const float k = std::sin(PI * u);
    Brazo& b = p.brazo[1];
    auto hacia = [k](float& v, float meta) { v += (meta - v) * k; };
    hacia(b.flex, 1.20f + apunta * 0.85f);
    hacia(b.codo, 0.30f);
    hacia(b.abd, 0.12f);
    hacia(b.prono, 1.35f);                     // la palma hacia abajo
    b.munFlex = b.munFlex + ((-0.35f + 0.6f * u) - b.munFlex) * k;   // la deja con la muneca
    manoRelajada(b, 0.15f * (1.0f - k));
    // Al soltar, la mano se abre y los dedos casi se estiran.
    for (int d = 0; d < 5; ++d)
        for (int f = 0; f < 3; ++f) b.dedo[d][f] *= 1.0f - 0.7f * k;
    for (int d = 0; d < 4; ++d) b.abre[d] = 0.12f * k;
    p.giroTorso += 0.08f * k;
    if (apunta < 0.0f) p.inclinacion += -apunta * 0.25f * k;
}

// ============================================================================
// DE PIE SIN HACER NADA: LOS GESTOS DE REPOSO
// ============================================================================
// PEDIDO: "si estas parado, movera y acomodara como en la vida real sus
// brazos, piernas, se estirara". Nadie se queda como una estatua: tras unos
// segundos quieto el cuerpo va encadenando gestos que entran y salen suaves.
// Cada vez que sale un gesto lo hace con su lado, su amplitud y su ritmo, asi
// que dos "estirarse" seguidos no son iguales.
//
//    0  CARGA EL PESO en una pierna: la otra rodilla se afloja.
//    1  SE ESTIRA: los brazos arriba, los dedos abiertos, de puntillas.
//    2  MANOS EN LA CINTURA.
//    3  SE MIRA LAS MANOS y se las frota.
//    4  SACUDE LOS BRAZOS, sueltos.
//    5  CRUZA LOS BRAZOS.
//    6  SE RASCA LA CABEZA.
//    7  SE MIRA LAS UNAS de una mano.
//    8  SE TRUENA LOS DEDOS: manos entrelazadas y empuja hacia delante.
//    9  GOLPECITOS CON EL PIE, impaciente.
//   10  SE BALANCEA de los talones a las puntas.
//   11  ESTIRA LA ESPALDA: manos en los rinones y el pecho hacia atras.
//   12  SE TOCA LA PUNTA DE LOS PIES, sin doblar las rodillas.
constexpr float REPOSO_INICIO = 3.0f, REPOSO_TRAMO = 7.0f;
constexpr int NUM_GESTOS = 13;

inline uint32_t tramoDeReposo(float quieto) {
    return (uint32_t)((quieto - REPOSO_INICIO) / REPOSO_TRAMO);
}
// PEDIDO: "que las animaciones al estar parado aparezcan aleatoriamente, no
// continuas". Algo mas de la mitad de los tramos no hay gesto: el cuerpo se
// queda respirando y balanceandose, y de vez en cuando hace algo.
constexpr float REPOSO_PROBABILIDAD = 0.42f;

inline int gestoDeReposo(float quieto) {
    if (quieto < REPOSO_INICIO) return -1;
    uint32_t h = tramoDeReposo(quieto) * 2654435761u + 0x9E37u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    if ((h >> 8 & 0xFFFFu) / 65535.0f > REPOSO_PROBABILIDAD) return -1;   // este tramo, nada
    return (int)(h % (uint32_t)NUM_GESTOS);
}

inline void aplicarReposo(Pose& p, float quieto, float tiempo) {
    const int gesto = gestoDeReposo(quieto);
    if (gesto < 0) return;
    const uint32_t tramo = tramoDeReposo(quieto);
    float u = (quieto - REPOSO_INICIO) / REPOSO_TRAMO;
    u -= std::floor(u);
    const float k = suave(u / 0.18f) * suave((1.0f - u) / 0.18f);   // entra y sale
    // Las variaciones de esta vez: lado, amplitud y ritmo.
    const int l = ruido(tramo, 11) > 0.0f ? 1 : 0;      // el brazo o la pierna
    const float amp = 1.0f + 0.20f * ruido(tramo, 12);
    const float ritmo = 1.0f + 0.25f * ruido(tramo, 13);
    auto hacia = [k](float& v, float meta) { v += (meta - v) * k; };
    switch (gesto) {
        case 0:
            hacia(p.pierna[l].rodilla, 0.35f * amp);
            hacia(p.pierna[l].flex, 0.12f * amp);
            hacia(p.pierna[l].tobillo, 0.12f);
            p.giroPelvis += (l == 0 ? 0.06f : -0.06f) * k;
            p.pelvisLado += (l == 0 ? -0.06f : 0.06f) * k * amp;
            p.brazo[l].codo += 0.15f * k;
            break;
        case 1: {
            const float alto = std::sin(PI * u);
            for (Brazo& b : p.brazo) {
                hacia(b.flex, 0.4f + 2.5f * alto * amp);
                hacia(b.codo, 0.15f);
                hacia(b.abd, 0.30f);
                hacia(b.munFlex, -0.5f * alto);
                for (int d = 0; d < 4; ++d) { hacia(b.dedo[d][0], -0.2f * alto); hacia(b.dedo[d][1], 0.0f); hacia(b.abre[d], 0.3f); }
            }
            p.inclinacion -= 0.15f * alto * k;
            for (Pierna& g : p.pierna) g.tobillo -= 0.25f * alto * k;
            break;
        }
        case 2:
            for (Brazo& b : p.brazo) {
                hacia(b.flex, 0.15f); hacia(b.abd, 0.75f * amp); hacia(b.rot, -0.55f);
                hacia(b.codo, 1.90f); hacia(b.prono, 1.45f); hacia(b.munFlex, 0.30f);
            }
            p.pierna[l].rodilla += 0.15f * k;
            break;
        case 3: {
            const float frota = std::sin(tiempo * 7.0f * ritmo) * 0.18f;
            for (int i = 0; i < 2; ++i) {
                Brazo& b = p.brazo[i];
                hacia(b.flex, 0.85f + (i == 0 ? frota : -frota));
                hacia(b.abd, -0.15f);
                hacia(b.codo, 1.75f);
                hacia(b.prono, 0.05f);
                manoRelajada(b, 0.25f);
            }
            p.cuelloFlex += 0.3f * k;
            break;
        }
        case 4: {
            const float sacude = std::sin(tiempo * 9.0f * ritmo);
            for (int i = 0; i < 2; ++i) {
                Brazo& b = p.brazo[i];
                hacia(b.flex, 0.25f * amp * (i == 0 ? sacude : -sacude));
                hacia(b.abd, 0.20f + 0.10f * sacude);
                hacia(b.codo, 0.25f);
                hacia(b.munFlex, 0.6f * sacude);
                manoRelajada(b, 0.1f);
            }
            break;
        }
        case 5:
            for (int i = 0; i < 2; ++i) {
                Brazo& b = p.brazo[i];
                hacia(b.flex, 0.75f); hacia(b.abd, -0.15f); hacia(b.rot, 0.70f);
                hacia(b.codo, 2.20f); hacia(b.prono, 0.6f);
                manoRelajada(b, 0.35f);
            }
            p.pierna[l].rodilla += 0.12f * k;
            break;
        case 6: {
            Brazo& b = p.brazo[l];
            const float rasca = std::sin(tiempo * 11.0f * ritmo);
            hacia(b.flex, 2.55f); hacia(b.abd, 0.65f); hacia(b.codo, 2.35f);
            hacia(b.prono, 0.9f); hacia(b.munFlex, 0.4f);
            for (int d = 0; d < 4; ++d) hacia(b.dedo[d][1], 0.9f + 0.3f * rasca);
            p.cuelloFlex += 0.15f * k;
            p.giroTorso += (l == 1 ? -0.05f : 0.05f) * k;
            break;
        }
        case 7: {
            Brazo& b = p.brazo[l];
            hacia(b.flex, 1.15f); hacia(b.abd, -0.05f); hacia(b.codo, 2.05f);
            hacia(b.prono, 0.35f); hacia(b.munFlex, 0.85f);
            for (int d = 0; d < 4; ++d) { hacia(b.dedo[d][0], 1.2f); hacia(b.dedo[d][1], 1.4f); }
            p.cuelloFlex += 0.35f * k;
            break;
        }
        case 8: {
            const float empuja = suave((u - 0.35f) / 0.2f) * (1.0f - suave((u - 0.65f) / 0.2f));
            for (Brazo& b : p.brazo) {
                hacia(b.flex, 1.25f); hacia(b.abd, -0.25f);
                hacia(b.codo, 1.6f - 1.3f * empuja); hacia(b.prono, 1.5f);
                hacia(b.munFlex, -0.6f * empuja);
                for (int d = 0; d < 4; ++d) { hacia(b.dedo[d][0], 1.0f); hacia(b.abre[d], 0.3f); }
            }
            break;
        }
        case 9: {
            const float toque = std::fmax(0.0f, std::sin(tiempo * 9.0f * ritmo));
            hacia(p.pierna[l].tobillo, 0.30f * toque);
            hacia(p.pierna[l].rodilla, 0.12f);
            p.brazo[l].codo += 0.10f * k;
            break;
        }
        case 10: {
            const float balanceo = std::sin(tiempo * 2.2f * ritmo);
            for (Pierna& g : p.pierna) g.tobillo -= 0.18f * amp * std::fmax(0.0f, balanceo) * k;
            p.inclinacion += 0.04f * balanceo * k;
            break;
        }
        case 11:
            for (Brazo& b : p.brazo) {
                hacia(b.flex, -0.55f); hacia(b.abd, 0.30f); hacia(b.rot, -0.9f);
                hacia(b.codo, 1.55f); hacia(b.prono, 1.4f); hacia(b.munFlex, -0.4f);
            }
            p.inclinacion -= 0.22f * amp * k;
            break;
        case 12: {
            const float baja = std::sin(PI * u);
            p.inclinacion += 1.05f * baja * k * amp;
            for (Pierna& g : p.pierna) g.flex += 0.10f * baja * k;
            for (Brazo& b : p.brazo) { hacia(b.flex, 0.30f + 0.90f * baja); hacia(b.codo, 0.05f); manoRelajada(b, 0.1f); }
            break;
        }
    }
}

// ============================================================================
// LO QUE EL CUERPO HACE SIN QUE SE LO PIDAN
// ============================================================================
// Lo que distingue una animacion hecha a mano de una "de programa": el cuerpo
// reacciona a lo que le pasa.
//
//   ATERRIZAR   las rodillas absorben la caida (mas cuanto mas fuerte), el
//               torso se va hacia delante y los brazos bajan; luego se
//               levanta despacio.
//   ACELERAR    al arrancar se inclina hacia delante; al frenar, hacia atras.
//   GIRAR       girando sobre si mismo da pasitos para recolocar los pies.
inline void aplicarReacciones(Pose& p, const Estado& e) {
    if (e.aterrizaje >= 0.0f && e.aterrizaje < 0.55f) {
        const float t = e.aterrizaje;
        const float env = t < 0.08f ? suave(t / 0.08f) : 1.0f - suave((t - 0.08f) / 0.47f);
        const float f = sujetar(e.fuerzaCaida, 0.0f, 1.0f) * env;
        for (Pierna& g : p.pierna) { g.rodilla += 1.0f * f; g.flex += 0.45f * f; g.tobillo += 0.30f * f; }
        p.inclinacion += 0.30f * f;
        for (Brazo& b : p.brazo) { b.flex += 0.25f * f; b.abd += 0.20f * f; b.codo += 0.3f * f; }
    }
    p.inclinacion += sujetar(e.aceleracion * 0.025f, -0.12f, 0.18f);
    if (std::fabs(e.giroVel) > 20.0f && e.rapidez < 0.4f) {
        const float paso = std::sin(e.faseGiro);
        const int l = paso > 0.0f ? 0 : 1;
        const float a = std::fabs(paso) * sujetar((std::fabs(e.giroVel) - 20.0f) / 60.0f, 0.0f, 1.0f);
        p.pierna[l].flex += 0.30f * a;
        p.pierna[l].rodilla += 0.55f * a;
        p.pierna[l].abd += (e.giroVel > 0 ? 0.08f : -0.08f) * a;
    }
}

// La respiracion de una persona: se toma aire mas deprisa de lo que se suelta
// (algo menos de la mitad del ciclo inspirando) y al final de soltarlo hay una
// pausa. Devuelve -1 (vacio) .. 1 (lleno).
inline float ondaRespira(float fase) {
    float u = fase / (2.0f * PI);
    u -= std::floor(u);
    if (u < 0.40f) return -1.0f + 2.0f * suave(u / 0.40f);            // inspira
    if (u < 0.90f) return 1.0f - 2.0f * suave((u - 0.40f) / 0.50f);   // espira
    return -1.0f;                                                      // pausa
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


// ============================================================================
// RECOGER (la tecla F sobre un pedacito)
// ============================================================================
// PEDIDO: "una animacion de recoger [...] con 3 variantes, una cuando estas
// agachado, otra tirado" (y la de pie). Dura RECOGER_SEG: baja, la mano se
// abre, se cierra sobre el pedacito a la mitad y vuelve.
//
//   DE PIE     se dobla por la cintura con las rodillas un poco flexionadas
//              y el brazo derecho baja hasta el suelo.
//   AGACHADO   ya esta cerca: solo estira el brazo hacia delante y abajo.
//   TUMBADO    alarga el brazo por el suelo y lo trae de vuelta.
constexpr float RECOGER_SEG = 0.6f;

inline void aplicarRecoger(Pose& p, float t, bool agachado, bool tumbado) {
    if (t < 0.0f || t > RECOGER_SEG) return;
    const float u = t / RECOGER_SEG;
    const float k = std::sin(PI * u);                    // 0 -> 1 -> 0
    const float cierre = suave((u - 0.40f) / 0.12f);     // agarra a la mitad
    Brazo& b = p.brazo[1];
    auto hacia = [k](float& v, float meta) { v += (meta - v) * k; };
    if (tumbado) {
        hacia(b.flex, 2.95f);
        hacia(b.codo, 0.05f);
        hacia(b.abd, 0.20f);
        hacia(b.prono, 1.35f);
    } else if (agachado) {
        hacia(b.flex, 1.55f);
        hacia(b.codo, 0.10f);
        hacia(b.abd, 0.15f);
        hacia(b.prono, 1.20f);
        p.inclinacion += 0.25f * k;
    } else {
        p.inclinacion += 1.00f * k;
        for (Pierna& g : p.pierna) { g.rodilla += 0.55f * k; g.flex += 0.25f * k; g.tobillo += 0.20f * k; }
        hacia(b.flex, 1.10f);
        hacia(b.codo, 0.15f);
        hacia(b.abd, 0.12f);
        hacia(b.prono, 1.20f);
    }
    b.munFlex = b.munFlex + (0.25f - b.munFlex) * k;
    manoRelajada(b, cierre * 0.85f);
    if (cierre < 0.5f) for (int d = 0; d < 4; ++d) b.abre[d] = 0.25f * k;   // la mano abierta al bajar
}

// ============================================================================
// TRES SALTOS, AL AZAR
// ============================================================================
// PEDIDO: "otras animaciones de saltar variadas, solo 3 que se haran random".
// `t` son los segundos desde que despego: el cuerpo pasa del impulso a la
// pose del aire en los primeros 0.25 s.
//
//   0  RECOGIDO   rodillas arriba, brazos arriba para equilibrar.
//   1  ZANCADA    una pierna delante estirada y la otra atras doblada, los
//                 brazos al reves que las piernas: un salto de carrera.
//   2  ENCOGIDO   las dos rodillas al pecho y los brazos tirando hacia
//                 delante, como para salvar un obstaculo.
inline void aplicarSalto(Pose& p, int variante, float t) {
    const float k = suave(t / 0.25f);
    auto hacia = [k](float& v, float meta) { v += (meta - v) * k; };
    switch (variante) {
        case 1:
            hacia(p.pierna[0].flex, 0.95f);  hacia(p.pierna[0].rodilla, 0.25f);
            hacia(p.pierna[1].flex, -0.30f); hacia(p.pierna[1].rodilla, 1.30f);
            hacia(p.pierna[0].tobillo, 0.10f); hacia(p.pierna[1].tobillo, -0.50f);
            hacia(p.brazo[1].flex, 1.10f);   hacia(p.brazo[1].codo, 0.70f);
            hacia(p.brazo[0].flex, -0.55f);  hacia(p.brazo[0].codo, 0.50f);
            for (Brazo& b : p.brazo) { hacia(b.abd, 0.20f); manoRelajada(b, 0.3f); }
            p.inclinacion += 0.12f * k;
            break;
        case 2:
            for (Pierna& g : p.pierna) {
                hacia(g.flex, 1.35f); hacia(g.rodilla, 1.95f); hacia(g.tobillo, -0.40f);
            }
            for (Brazo& b : p.brazo) {
                hacia(b.flex, 0.95f); hacia(b.codo, 0.35f); hacia(b.abd, 0.25f);
                manoRelajada(b, 0.5f);
            }
            p.inclinacion += 0.18f * k;
            break;
        default: break;   // la 0 es la pose del aire de siempre
    }
}

// ============================================================================
// SIGILO E INSPECCION (CTRL una vez, luego W; y SHIFT encima)
// ============================================================================
// PEDIDO: "si presiona el jugador una vez ctrl y luego w, caminara despacio,
// y si presiona ctrl y W y luego shift, caminara como semi agachado, como si
// estuviera inspeccionando silencioso".
//
//   SIGILO      pasos cortos y suaves: rodillas algo dobladas, la punta del
//               pie baja primero, los brazos un poco adelantados y quietos.
//   INSPECCION  medio agachado y encorvado, la mano izquierda delante a media
//               altura como tanteando, y los hombros girando despacio a un
//               lado y a otro: mirando alrededor sin hacer ruido.
inline void aplicarSigilo(Pose& p, bool inspeccion, float tiempo) {
    for (Pierna& g : p.pierna) { g.rodilla += 0.15f; g.flex += 0.05f; g.tobillo -= 0.06f; }
    for (Brazo& b : p.brazo) {
        b.flex = b.flex * 0.4f + 0.25f;
        b.codo += 0.35f;
        manoRelajada(b, 0.15f);
    }
    p.inclinacion += 0.08f;
    if (!inspeccion) return;
    for (Pierna& g : p.pierna) { g.flex += 0.55f; g.rodilla += 0.85f; g.tobillo += 0.22f; }
    p.inclinacion += 0.35f;
    Brazo& i = p.brazo[0];
    i.flex = 0.75f; i.codo = 1.15f; i.abd = 0.18f; i.prono = 1.0f; i.munFlex = -0.15f;
    manoRelajada(i, 0.0f);
    for (int d = 0; d < 4; ++d) i.abre[d] = 0.2f;
    Brazo& d = p.brazo[1];
    d.flex = 0.45f; d.codo = 0.95f; d.abd = 0.15f;
    p.giroTorso += 0.14f * std::sin(tiempo * 0.6f);
}

inline Pose poseDe(const Estado& e) {
    Pose p;
    p.enMano = (uint8_t)e.herramienta;
    p.pedernal = e.pedernal;
    float ritmo = e.rapidez / 4.3f;              // 4.3 = andar
    if (ritmo > 1.4f) ritmo = 1.4f;
    const float corre = suave((e.rapidez - 4.6f) / 1.0f);
    const float respira = ondaRespira(e.faseRespira);
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
        // PEDIDO: "que las animaciones al estar tirado se adapten a la
        // posicion". Mirando al frente o arriba se apoya en los codos y
        // levanta el pecho (los antebrazos planos, los codos bajo los
        // hombros); mirando abajo se deja caer con los brazos delante. Al
        // arrastrarse, el pecho baja.
        const float levanta = sujetar((e.mirada + 25.0f) / 45.0f, 0.0f, 1.0f) * (1.0f - 0.6f * amp);
        p.levantaPecho = levanta;
        for (int l = 0; l < 2; ++l) {
            const float lado = l == 0 ? c : -c;
            Brazo& b = p.brazo[l];
            b.flex = 2.30f - 0.55f * levanta + 0.35f * amp * lado;
            b.codo = 0.60f + 0.85f * levanta + 0.35f * amp * lado;
            b.abd = 0.35f - 0.10f * levanta;
            b.prono = 1.35f;                          // palmas al suelo
            // La palma plana: la muneca compensa lo que se inclina el antebrazo.
            b.munFlex = -0.35f + 0.25f * levanta;
            manoRelajada(b, 0.12f);
            for (int d = 0; d < 4; ++d) { b.abre[d] = 0.18f; b.dedo[d][0] = 0.12f + 0.04f * d; }
            Pierna& g = p.pierna[l];
            const float sube = std::fmax(0.0f, -lado) * amp;
            g.flex = 0.02f + 0.45f * sube;
            g.abd = 0.08f + 0.35f * sube;
            g.rodilla = 0.08f + 0.90f * sube;
            g.tobillo = -0.80f;                       // el empeine en el suelo
        }
        p.respira = respira;
        p.giroTorso = e.giroCuello * 0.35f;
        aplicarRecoger(p, e.tRecoger, false, true);
        aplicarColocar(p, e.accion == Accion::COLOCAR ? e.tAccion : -1.0f, e.mirada * PI / 180.0f);
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
        aplicarSalto(p, e.saltoVariante, e.tAire);
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
            // La muneca y los dedos van detras del brazo (accion secundaria).
            const float atraso = std::sin(2.0f * PI * tPierna[1 - l] - 0.9f);
            b.munFlex += 0.18f * ritmo * atraso;
            for (int d = 0; d < 4; ++d) b.dedo[d][1] += 0.10f * ritmo * atraso;
        }
        // El estilo de este personaje: unos mueven mas los brazos, otros dan
        // pasos mas largos o abren un poco las puntas.
        {
            const float brazos = 1.0f + 0.15f * ruido(e.estilo, 21);
            const float pasos = 1.0f + 0.10f * ruido(e.estilo, 22);
            for (Brazo& b : p.brazo) b.flex *= brazos;
            for (Pierna& g : p.pierna) { g.flex *= pasos; g.abd += 0.03f * ruido(e.estilo, 23); }
        }
        // La cadera cae del lado de la pierna que va en el aire.
        p.pelvisLado = 0.05f * ritmo * std::sin(2.0f * PI * tPierna[0]);
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

    // Andar con sigilo, o medio agachado inspeccionando.
    if (e.sigilo && e.enSuelo && !e.volando && !e.agachado)
        aplicarSigilo(p, e.inspeccion, e.tiempo);

    for (int l = 0; l < 2; ++l)
        for (int d = 0; d < 4; ++d)
            p.brazo[l].dedo[d][1] += 0.04f * std::sin(e.tiempo * (0.7f + 0.13f * d) + l * 2.1f + d);

    // Respirar: los hombros suben un poco y los brazos se mecen.
    for (Brazo& b : p.brazo) b.flex += respira * 0.02f;

    // Lo que hace el cuerpo solo: aterrizar, acelerar, girar.
    if (e.enSuelo && !e.volando) aplicarReacciones(p, e);

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

    // Quieto mucho rato: los gestos de reposo; y siempre, el balanceo minimo
    // de quien esta de pie (nadie esta del todo quieto).
    if (e.enSuelo && !e.volando && !e.agachado && !e.sigilo && e.rapidez < 0.3f &&
        e.accion == Accion::NINGUNA && !e.golpeando && e.tRecoger < 0.0f) {
        p.inclinacion += 0.012f * std::sin(e.tiempo * 0.43f);
        p.giroPelvis += 0.010f * std::sin(e.tiempo * 0.31f + 1.0f);
        aplicarReposo(p, e.quieto, e.tiempo);
    }

    // Lo que hace el brazo va lo ultimo: manda sobre andar y agacharse.
    // `golpeando` sin accion es el picar de siempre, con la mano.
    const Accion accion = (e.accion == Accion::NINGUNA && e.golpeando) ? Accion::PICAR : e.accion;
    const Pose antes = p;
    if (accion == Accion::PICAR)
        aplicarGolpe(p, golpeDe(e.herramienta, e.bloque), e.tiempo, e.mirada * PI / 180.0f,
                     (uint32_t)e.herramienta * 31u + (uint32_t)e.bloque + e.estilo);
    else if (accion == Accion::GOLPE_AIRE || accion == Accion::GOLPE_ANIMAL)
        aplicarDisparo(p, accion, e.variante, e.ladoGolpe, e.tAccion, e.mirada * PI / 180.0f);
    else if (accion == Accion::COLOCAR)
        aplicarColocar(p, e.tAccion, e.mirada * PI / 180.0f);

    // ⭐ GOLPEAR NO MUEVE TODO EL CUERPO
    //
    // PEDIDO: "cuando golpeas o usas una herramienta, no se mueva todo el
    // cuerpo, solo el brazo derecho y un poco del pecho". Lo que las acciones
    // cambiaron fuera del brazo que golpea se deshace: las piernas, la
    // cadera, la inclinacion y el otro brazo vuelven a lo que estaban
    // haciendo (andar, estar quieto...); del giro del pecho queda un tercio.
    // El brazo que golpea es el derecho, salvo en los punetazos de izquierda.
    if (accion != Accion::NINGUNA) {
        const bool izquierda = (accion == Accion::GOLPE_AIRE && (e.variante & 1) == 1) ||
                               (accion == Accion::GOLPE_ANIMAL && (e.variante == 1 ||
                                                                   (e.variante == 2 && e.ladoGolpe == 0)));
        const int brazo = izquierda ? 0 : 1;
        p.pierna[0] = antes.pierna[0];
        p.pierna[1] = antes.pierna[1];
        p.giroPelvis = antes.giroPelvis;
        p.pelvisLado = antes.pelvisLado;
        p.inclinacion = antes.inclinacion;
        p.giroTorso = antes.giroTorso + (p.giroTorso - antes.giroTorso) * 0.33f;
        p.brazo[1 - brazo] = antes.brazo[1 - brazo];
    }

    // Recoger manda sobre todo lo demas: dura medio segundo.
    aplicarRecoger(p, e.tRecoger, e.agachado, false);

    // La cabeza: los hombros acompanan un tercio del giro del cuello, que asi
    // nunca pasa de su limite. Al mirar abajo, el cuello se dobla hasta su
    // tope y el resto lo ponen los ojos.
    p.giroTorso += e.giroCuello * 0.35f;
    p.cuelloFlex = sujetar(-e.mirada * PI / 180.0f, 0.0f, Lim::CUELLO_FLEX);

    limitar(p);
    return p;
}

// ----------------------------------------------------------------------------
// LAS TRANSICIONES: CADA ARTICULACION CON SU MUELLE
// ----------------------------------------------------------------------------
// `poseDe` dice donde DEBERIA estar cada articulacion; esto hace que llegue
// como llega un cuerpo: con inercia. Cada giro sigue a su objetivo con un
// muelle amortiguado, y no todos igual:
//
//   PIERNAS   rigidos y sin rebote: tienen que seguir el paso sin resbalar.
//   BRAZOS    algo de rebote: al parar un golpe el brazo se pasa un poco y
//             vuelve (el "follow-through" de los animadores).
//   MUNECAS   mas blandos y mas lentos que el brazo: van detras de el
//             (la "accion superpuesta").
//   DEDOS     rapidos pero no instantaneos.
//   TORSO     lento y pesado.
//   POSTURA   agacharse o tumbarse se hace en medio segundo, no de golpe.
//
// Con una accion rapida en marcha (golpe, punetazo) los brazos se endurecen
// para no perder la velocidad del gesto. Muelles con subpasos: estables
// aunque el frame tarde.
struct SuavizadoPose {
    Pose actual;
    float vel[96] = {};
    bool iniciado = false;

    // Recorre a la vez los campos de tres poses: f(valor, objetivo, vel, w, z).
    template <class F>
    static void recorrer(Pose& a, const Pose& o, float* v, bool rapido, F f) {
        int i = 0;
        auto campo = [&](float& x, float meta, float w, float z) { f(x, meta, v[i++], w, z); };
        const float wb = rapido ? 45.0f : 17.0f, zb = rapido ? 0.62f : 0.68f;
        for (int l = 0; l < 2; ++l) {
            Brazo& b = a.brazo[l];
            const Brazo& m = o.brazo[l];
            campo(b.flex, m.flex, wb, zb);
            campo(b.abd, m.abd, wb, zb);
            campo(b.rot, m.rot, wb, zb);
            campo(b.codo, m.codo, wb * 1.1f, zb);
            campo(b.prono, m.prono, wb * 0.8f, 0.6f);
            campo(b.munFlex, m.munFlex, wb * 0.75f, 0.55f);
            campo(b.munDesv, m.munDesv, wb * 0.75f, 0.6f);
            campo(b.encoger, m.encoger, wb * 0.6f, 0.8f);
            campo(b.adelantar, m.adelantar, wb * 0.6f, 0.8f);
            for (int d = 0; d < 5; ++d) {
                for (int k = 0; k < 3; ++k) campo(b.dedo[d][k], m.dedo[d][k], 24.0f + 2.0f * k, 0.8f);
                campo(b.abre[d], m.abre[d], 20.0f, 0.8f);
            }
        }
        for (int l = 0; l < 2; ++l) {
            Pierna& g = a.pierna[l];
            const Pierna& m = o.pierna[l];
            campo(g.flex, m.flex, 40.0f, 1.0f);
            campo(g.abd, m.abd, 30.0f, 1.0f);
            campo(g.rodilla, m.rodilla, 40.0f, 1.0f);
            campo(g.tobillo, m.tobillo, 40.0f, 1.0f);
            campo(g.dedos, m.dedos, 40.0f, 1.0f);
            campo(g.rot, m.rot, 30.0f, 1.0f);
            campo(g.tobilloLado, m.tobilloLado, 30.0f, 1.0f);
        }
        campo(a.inclinacion, o.inclinacion, 11.0f, 0.8f);
        campo(a.giroTorso, o.giroTorso, 12.0f, 0.75f);
        campo(a.torsoLado, o.torsoLado, 11.0f, 0.8f);
        campo(a.pelvisInclina, o.pelvisInclina, 11.0f, 0.85f);
        campo(a.cuelloGiro, o.cuelloGiro, 14.0f, 0.9f);
        campo(a.cuelloLado, o.cuelloLado, 14.0f, 0.9f);
        campo(a.giroPelvis, o.giroPelvis, 14.0f, 0.9f);
        campo(a.pelvisLado, o.pelvisLado, 16.0f, 0.9f);
        campo(a.cuelloFlex, o.cuelloFlex, 14.0f, 0.9f);
        campo(a.agachado, o.agachado, 9.0f, 1.0f);
        campo(a.tumbado, o.tumbado, 7.0f, 1.0f);
        campo(a.levantaPecho, o.levantaPecho, 6.0f, 1.0f);
    }

    const Pose& avanzar(const Pose& objetivo, float dt, bool rapido) {
        if (dt < 0) dt = 0;
        if (dt > 0.1f) dt = 0.1f;
        if (!iniciado) { actual = objetivo; iniciado = true; return actual; }
        recorrer(actual, objetivo, vel, rapido, [dt](float& x, float meta, float& v, float w, float z) {
            const int n = (int)std::ceil(w * dt / 0.25f);
            const float h = dt / (float)(n < 1 ? 1 : n);
            for (int i = 0; i < n; ++i) {
                v += (w * w * (meta - x) - 2.0f * z * w * v) * h;
                x += v * h;
            }
        });
        // Lo que no se suaviza: la respiracion (ya es suave), el puno y lo
        // que lleva en la mano.
        actual.respira = objetivo.respira;
        actual.apretar = objetivo.apretar;
        actual.enMano = objetivo.enMano;
        actual.pedernal = objetivo.pedernal;
        actual.prop = objetivo.prop;
        actual.flexibilidad = objetivo.flexibilidad;
        limitar(actual);
        return actual;
    }
};

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
    // El largo de cada hueso con las proporciones de la pose (la malla los
    // usa para que la piel cubra justo el hueso).
    struct Largos {
        float espalda = M::ESPALDA, brazo = M::BRAZO, antebrazo = M::ANTEBRAZO,
              muslo = M::MUSLO, pierna = M::PIERNA;
    } largo;
};

// El grosor de cada dedo del pie (del gordo al pequeno).
inline float radioDedoPie(int d) { return d == 0 ? 0.0160f : 0.0112f - d * 0.0007f; }

// La parte de la flexion lateral y el giro de la columna que ya se hizo a la
// altura `t` (0 pelvis .. 1 cuello): las vertebras lumbares casi no giran y
// las toracicas si; la malla del torso usa lo mismo.
inline float columnaGiro(float t) { const float u = sujetar(t * 1.4f, 0.0f, 1.0f); return u * u * (3.0f - 2.0f * u); }
inline float columnaLado(float t) { const float u = sujetar(t * 1.25f, 0.0f, 1.0f); return u * u * (3.0f - 2.0f * u); }

namespace detalle {
inline void resolver(const Pose& p, float subir, Esqueleto& s) {
    Esqueleto::Largos& L = s.largo;
    L.espalda = M::ESPALDA * p.prop.torso;
    L.brazo = M::BRAZO * p.prop.brazo;
    L.antebrazo = M::ANTEBRAZO * p.prop.antebrazo;
    L.muslo = M::MUSLO * p.prop.muslo;
    L.pierna = M::PIERNA * p.prop.pierna;
    const float alturaCadera = M::CADERA + (L.muslo - M::MUSLO) + (L.pierna - M::PIERNA);
    const Rot rPelvis = Rot::enY(p.giroPelvis).por(Rot::enZ(p.pelvisLado)).por(Rot::enX(-p.pelvisInclina));
    s.pelvis = { { 0, alturaCadera + 0.02f + subir, -M::ATRAS }, rPelvis };
    // El pecho compensa casi toda la caida de la cadera: los hombros quedan
    // casi rectos aunque la pelvis se incline.
    const Rot rPecho = rPelvis.por(Rot::enZ(-0.8f * p.pelvisLado - p.torsoLado)).por(Rot::enY(p.giroTorso))
                              .por(Rot::enX(-p.inclinacion));
    s.pecho = { s.pelvis.p, rPecho };
    // El cuello: gira, se dobla de lado y hacia el pecho (en ese orden: la
    // atlas-axis gira y el resto de las cervicales se doblan).
    s.cuello = { suma(s.pelvis.p, rPecho.aplicar({ 0, L.espalda, 0 })),
                 rPecho.por(Rot::enY(p.cuelloGiro)).por(Rot::enZ(-p.cuelloLado)).por(Rot::enX(p.cuelloFlex)) };
    s.ojo = s.cuello.p.y + M::OJO_SOBRE_CUELLO;
    const float kHombro = L.espalda / M::ESPALDA;

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Brazo& b = p.brazo[l];
        // ⭐ LA ESCAPULA. Al subir el brazo por encima de ~70 grados la
        // escapula gira y sube con el (el ritmo escapulohumeral: de cada 3
        // grados, 1 lo pone ella); al llevarlo delante se desliza hacia
        // delante. Mas lo que se encoja o adelante a proposito, y al tomar
        // aire los hombros suben un poco.
        const float elevado = std::fmax(b.abd, b.flex);
        const float ritmo = sujetar((elevado - 1.2f) / 1.6f, 0.0f, 1.0f);
        const float sube = Lim::ENCOGER_M * b.encoger + 0.022f * ritmo + p.respira * 0.005f;
        const float delante = Lim::ADELANTAR_M * b.adelantar + 0.012f * sujetar(b.flex / 1.5f, -0.5f, 1.0f);
        const V hombro = suma(s.pelvis.p, rPecho.aplicar({ sx * M::X_HOMBRO,
                                                           M::HOMBRO * kHombro * p.prop.hombroY + sube, delante }));
        const Rot rBrazo = rPecho.por(Rot::enZ(sx * b.abd)).por(Rot::enX(b.flex))
                                 .por(Rot::enY(sx * b.rot));
        s.hombro[l] = { hombro, rBrazo };
        const V codo = suma(hombro, rBrazo.aplicar({ 0, -L.brazo, 0 }));
        const Rot rAnte = rBrazo.por(Rot::enX(b.codo));
        s.codo[l] = { codo, rAnte };
        // La mano se construye en la posicion anatomica: palma delante (+Z) y
        // pulgar hacia fuera. En ella el antebrazo no esta torcido. Con el
        // brazo colgando y relajado el pulgar mira adelante: un cuarto de
        // vuelta, mas lo que este pronada. Ese giro se reparte a lo largo del
        // antebrazo (radio sobre cubito), no de golpe en la muneca.
        s.prono[l] = -sx * (PI * 0.5f + b.prono);
        const V muneca = suma(codo, rAnte.aplicar({ 0, -L.antebrazo, 0 }));
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
                base = suma(muneca, rMano.aplicar({ pulgar * NX[d] * p.prop.mano, NY[d] * p.prop.mano, 0 }));
                r = rMano.por(Rot::enZ(pulgar * ABRE[d] * b.abre[d]));
            } else {
                // El pulgar sale del lado de la palma, abierto y girado sobre
                // si mismo: asi al doblarse cruza hacia el centro de la mano.
                base = suma(muneca, rMano.aplicar({ pulgar * 0.026f * p.prop.mano, -0.022f * p.prop.mano, 0.009f * p.prop.mano }));
                r = rMano.por(Rot::enZ(pulgar * 0.65f)).por(Rot::enY(-pulgar * 0.8f));
            }
            for (int f = 0; f < 3; ++f) {
                r = r.por(Rot::enX(b.dedo[d][f]));    // se dobla hacia la palma (+Z)
                m.dedo[d][f] = { base, r };
                base = suma(base, r.aplicar({ 0, -M::DEDO[d][f] * p.prop.mano, 0 }));
            }
            m.dedo[d][3] = { base, r };
        }
    }

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Pierna& g = p.pierna[l];
        const V cadera = suma(s.pelvis.p, rPelvis.aplicar({ sx * M::X_CADERA, -0.02f, 0 }));
        // La rotacion de la cadera gira el muslo sobre su eje: con ella la
        // rodilla y la punta del pie miran hacia fuera o hacia dentro.
        const Rot rMuslo = rPelvis.por(Rot::enX(p.pelvisInclina)).por(Rot::enZ(sx * g.abd)).por(Rot::enX(g.flex))
                                  .por(Rot::enY(sx * g.rot));
        s.cadera[l] = { cadera, rMuslo };
        const V rodilla = suma(cadera, rMuslo.aplicar({ 0, -L.muslo, 0 }));
        const Rot rPierna = rMuslo.por(Rot::enX(-g.rodilla));   // la rodilla dobla hacia atras
        s.rodilla[l] = { rodilla, rPierna };
        const V tobillo = suma(rodilla, rPierna.aplicar({ 0, -L.pierna, 0 }));
        // El tobillo sube y baja la punta; la subastragalina voltea la planta
        // hacia dentro (inversion) o hacia fuera (eversion).
        const Rot rPie = rPierna.por(Rot::enX(g.tobillo)).por(Rot::enZ(-sx * g.tobilloLado));
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
        detalle::mover(s, Rot::enX(-(GIRO_TUMBADO - 0.30f * p.levantaPecho) * p.tumbado), s.pelvis.p, 0.0f);
        if (apoyar) detalle::mover(s, Rot::identidad(), s.pelvis.p, -detalle::apoyoTumbado(s));
        return s;
    }
    if (apoyar) {
        const float baja = detalle::plantaMasBaja(s);
        detalle::resolver(p, -baja, s);
    }
    return s;
}

// ⭐ LA POSTURA DENTRO DE SU HUECO
//
// BUG: "el modelo del jugador se ve muy estirado y mal las rodillas y manos
// al agacharme o tirarme". La camara del motor agachado esta a 0.86 (hitbox de
// 0.95) y tumbado a 0.42 (hitbox de 0.55), pero la pose dejaba el ojo a 1.24 y
// a 0.61: el cuerpo se APLASTABA en vertical para que cupiera, y un modelo
// realista aplastado se ve estirado y con las rodillas rotas.
//
// Ahora la pose se HUNDE hasta que el ojo cae justo en la camara: agachado,
// hacia unas cuclillas profundas sobre las puntas, con la pelvis inclinada y
// el tronco adelante (la cabeza levantada para seguir mirando al frente);
// tumbado, bajando el pecho al suelo. Se busca por biseccion cuanto mezclar.
inline void hundirHasta(Pose& p, float ojoObjetivo, bool tumbado) {
    auto con = [&](float t) {
        Pose q = p;
        auto ir = [t](float& v, float meta) { v += (meta - v) * t; };
        if (tumbado) {
            ir(q.levantaPecho, 0.0f);
            ir(q.cuelloFlex, -0.25f);
        } else {
            for (Pierna& g : q.pierna) {
                ir(g.flex, 1.95f); ir(g.rodilla, 2.38f); ir(g.tobillo, -0.30f);
                ir(g.abd, 0.22f); ir(g.rot, 0.30f); ir(g.dedos, 0.6f);
            }
            ir(q.pelvisInclina, 0.42f);
            ir(q.inclinacion, 0.82f);
            ir(q.cuelloFlex, -0.55f);
        }
        limitar(q);
        return q;
    };
    if (resolver(con(0.0f), true).ojo <= ojoObjetivo + 0.02f) return;
    float lo = 0.0f, hi = 1.0f;
    for (int i = 0; i < 10; ++i) {
        const float m = 0.5f * (lo + hi);
        if (resolver(con(m), true).ojo > ojoObjetivo) lo = m; else hi = m;
    }
    p = con(hi);
}

// ⭐ EL CENTRO DEL CUERPO EN PLANTA: el medio de lo que ocupa (pelvis y
// gluteos detras, rodillas y pies delante, hombros a los lados), con el
// grosor de cada parte. Es el punto que va en el centro de la hitbox, asi el
// cuerpo queda dentro de ella de pie, agachado o andando.
inline V centroEnPlanta(const Esqueleto& s) {
    float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f;
    auto meter = [&](V p, float r) {
        x0 = std::fmin(x0, p.x - r); x1 = std::fmax(x1, p.x + r);
        z0 = std::fmin(z0, p.z - r); z1 = std::fmax(z1, p.z + r);
    };
    meter(s.pelvis.p, 0.15f);
    meter(s.cuello.p, 0.10f);
    for (int l = 0; l < 2; ++l) {
        meter(s.hombro[l].p, 0.07f);
        meter(s.cadera[l].p, 0.09f);
        meter(s.rodilla[l].p, 0.07f);
        meter(s.pie[l].talon, 0.04f);
        meter(s.pie[l].dedo[0][2].p, 0.02f);
    }
    return { 0.5f * (x0 + x1), s.cuello.p.y, 0.5f * (z0 + z1) };
}

// ----------------------------------------------------------------------------
// LO QUE SE LLEVA EN LA MANO
// ----------------------------------------------------------------------------
// El punto y el marco donde se dibuja el item de la mano derecha: dentro del
// puno, con el plano del item conteniendo el antebrazo y el pulgar (asi una
// herramienta queda con el mango cruzando la mano y la cabeza por encima del
// pulgar) y su grosor en la direccion de la palma.
//
// El marco que devuelve: +X hacia el pulgar, +Y hacia el codo, +Z la palma.
inline Junta agarre(const Esqueleto& s) {
    const Mano& m = s.mano[1];
    const Junta& j = m.muneca;
    const V centro = suma(j.p, j.r.aplicar({ 0, -0.068f, 0.020f }));
    // Con lado = -1 el pulgar va hacia -X: se voltea X para que +X siempre sea
    // el pulgar y el item no salga en espejo.
    const Rot r = m.lado > 0 ? j.r : j.r.por(Rot::enY(PI));
    return { centro, r };
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
