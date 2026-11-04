#pragma once

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR: FIGURA HUMANA SIMPLE Y ARTICULADA
// ============================================================================
// PEDIDO: "un modelo para el jugador con forma humana simple y con
// articulaciones y movimiento [...] si ve directo abajo, se vea el torso y el
// cuerpo desde arriba hasta los pies y los brazos".
//
// Piezas en forma de caja, como el resto del mundo, encadenadas por sus
// articulaciones: hombro -> codo -> mano y cadera -> rodilla -> pie. Cada
// pieza cuelga del extremo de la anterior, asi que doblar el codo mueve el
// antebrazo Y la mano sin despegarlos.
//
// La cabeza no se dibuja: la camara esta dentro de ella.
//
// Espacio local: origen en los pies, +Y arriba, +Z delante, +X derecha. Metros.
// ============================================================================

namespace ModeloJugador {

struct V { float x, y, z; };

inline V suma(V a, V b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline V escala(V a, float k) { return { a.x * k, a.y * k, a.z * k }; }

// Rotacion 3x3 por filas.
struct Rot {
    float m[9];
    static Rot identidad() { return { { 1, 0, 0, 0, 1, 0, 0, 0, 1 } }; }
    // Giro en X: +a lleva la pieza que cuelga (-Y) hacia delante (+Z).
    static Rot enX(float a) {
        const float c = std::cos(a), s = std::sin(a);
        return { { 1, 0, 0, 0, c, s, 0, -s, c } };
    }
    // Giro en Z: separa el brazo del cuerpo.
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

enum Material { CAMISA, PANTALON, ZAPATO, PIEL };

// Una pieza: caja centrada en `centro`, girada por `rot`, de semilados `medio`.
struct Caja {
    V centro;
    Rot rot;
    V medio;
    Material material;
};

// Las medidas, de una estatura de 1.80 m.
namespace M {
    constexpr float CADERA   = 0.94f;   // altura de la cadera
    constexpr float HOMBRO   = 1.45f;
    constexpr float X_HOMBRO = 0.27f;
    constexpr float X_CADERA = 0.10f;
    constexpr float MUSLO    = 0.46f;
    constexpr float PIERNA   = 0.42f;
    constexpr float BRAZO    = 0.31f;
    constexpr float ANTEBRAZO= 0.27f;
    constexpr float MANO     = 0.15f;
    // El cuerpo se pone un poco por detras del ojo: asi, al mirar abajo, se
    // ve el pecho por delante y los pies, y no el interior del torso.
    constexpr float ATRAS    = 0.11f;
    constexpr float OJO      = 1.62f;   // la misma altura que la camara
}

// La pose: los giros de cada articulacion, en radianes.
struct Pose {
    float hombroI = 0, hombroD = 0, codoI = 0, codoD = 0;
    float separaI = 0.08f, separaD = 0.08f;   // brazos un poco abiertos
    float caderaI = 0, caderaD = 0, rodillaI = 0, rodillaD = 0;
    float inclinacion = 0;   // el torso hacia delante
    float agachado = 0;      // 0 de pie .. 1 agachado
    float bote = 0;          // metros: el cuerpo sube y baja al andar
};

// El estado que la anima.
struct Estado {
    float fasePaso = 0;      // radianes, avanza con la distancia andada
    float rapidez = 0;       // bloques/s en horizontal
    bool  enSuelo = true;
    bool  agachado = false;
    bool  volando = false;
    bool  golpeando = false; // clic izquierdo mantenido: el brazo derecho pica
    float tiempo = 0;        // segundos, para respirar y picar
};

// Avanzar el paso con lo que se ha andado este frame (en bloques).
inline float avanzarPaso(float fase, float distancia) {
    // Una zancada completa (dos pasos) cada ~2.2 bloques.
    return std::fmod(fase + distancia * (6.2831853f / 2.2f), 6.2831853f);
}

inline Pose poseDe(const Estado& e) {
    Pose p;
    float ritmo = e.rapidez / 4.3f;              // 4.3 = andar
    if (ritmo > 1.6f) ritmo = 1.6f;              // corriendo, mas amplitud
    const float s = std::sin(e.fasePaso);
    const float amp = 0.55f * ritmo;

    if (e.volando) {
        // Volando: piernas juntas, algo recogidas; brazos sueltos.
        p.caderaI = p.caderaD = 0.15f;
        p.rodillaI = p.rodillaD = 0.25f;
        p.hombroI = p.hombroD = 0.10f;
    } else if (!e.enSuelo) {
        // En el aire: rodillas recogidas y brazos arriba para equilibrar.
        p.caderaI = 0.45f; p.caderaD = 0.25f;
        p.rodillaI = 0.80f; p.rodillaD = 0.55f;
        p.hombroI = p.hombroD = -0.35f;
        p.separaI = p.separaD = 0.35f;
        p.codoI = p.codoD = 0.40f;
    } else {
        // Andar: cada pierna adelante cuando su brazo va atras.
        p.caderaI = amp * s;
        p.caderaD = -amp * s;
        // La rodilla se dobla al recoger la pierna (cuando va hacia delante).
        p.rodillaI = (s > 0 ? s : 0) * 0.9f * ritmo + 0.05f;
        p.rodillaD = (-s > 0 ? -s : 0) * 0.9f * ritmo + 0.05f;
        p.hombroI = -amp * 0.8f * s;
        p.hombroD = amp * 0.8f * s;
        p.codoI = 0.15f + 0.25f * ritmo;
        p.codoD = 0.15f + 0.25f * ritmo;
        p.bote = std::fabs(std::sin(e.fasePaso)) * 0.035f * (ritmo > 1 ? 1 : ritmo);
        p.inclinacion = 0.06f * ritmo;
    }

    // Respirar: los brazos se mecen un poco aunque este quieto.
    const float resp = std::sin(e.tiempo * 1.6f) * 0.025f;
    p.hombroI += resp;
    p.hombroD += resp;

    // Picar: el brazo derecho baja una y otra vez.
    if (e.golpeando) {
        const float g = 0.5f + 0.5f * std::sin(e.tiempo * 12.0f);
        p.hombroD = 0.9f + 0.8f * g;
        p.codoD = 0.35f;
        p.separaD = 0.10f;
    }

    if (e.agachado) {
        p.agachado = 1.0f;
        p.caderaI += 0.9f; p.caderaD += 0.9f;
        p.rodillaI += 1.5f; p.rodillaD += 1.5f;
        p.inclinacion += 0.35f;
    }

    // Las rodillas y los codos solo doblan hacia un lado.
    const auto bisagra = [](float& a, float max) { a = a < 0 ? 0 : (a > max ? max : a); };
    bisagra(p.rodillaI, 2.3f); bisagra(p.rodillaD, 2.3f);
    bisagra(p.codoI, 2.5f);    bisagra(p.codoD, 2.5f);
    return p;
}

// ----------------------------------------------------------------------------
// LAS PIEZAS, YA COLOCADAS
// ----------------------------------------------------------------------------
inline void construir(const Pose& p, std::vector<Caja>& salida) {
    salida.clear();
    // Agachado: las rodillas dobladas bajan la cadera.
    const float bajaCadera = p.agachado * 0.36f;
    const float yCadera = M::CADERA - bajaCadera + p.bote;
    const Rot torso = Rot::enX(-p.inclinacion);   // + = hacia delante
    const V cadera = { 0, yCadera, -M::ATRAS };

    // El torso, de la cadera al hombro, girado sobre la cadera.
    const float altoTorso = M::HOMBRO - M::CADERA;
    const V centroTorso = suma(cadera, torso.aplicar({ 0, altoTorso * 0.5f, 0 }));
    salida.push_back({ centroTorso, torso, { 0.23f, altoTorso * 0.5f + 0.04f, 0.12f }, CAMISA });

    // Una cadena que cuelga: cada pieza se pone a medio camino y la siguiente
    // empieza donde acaba esta.
    const auto miembro = [&](V raiz, const Rot& base, float giro1, float giro2,
                             float largo1, float largo2, float grueso1, float grueso2,
                             Material m1, Material m2) {
        const Rot r1 = base.por(Rot::enX(giro1));
        salida.push_back({ suma(raiz, r1.aplicar({ 0, -largo1 * 0.5f, 0 })), r1,
                           { grueso1, largo1 * 0.5f + 0.02f, grueso1 }, m1 });
        const V codo = suma(raiz, r1.aplicar({ 0, -largo1, 0 }));
        const Rot r2 = r1.por(Rot::enX(giro2));
        salida.push_back({ suma(codo, r2.aplicar({ 0, -largo2 * 0.5f, 0 })), r2,
                           { grueso2, largo2 * 0.5f + 0.02f, grueso2 }, m2 });
        return std::make_pair(suma(codo, r2.aplicar({ 0, -largo2, 0 })), r2);
    };

    // --- Brazos: cuelgan del hombro, separados un poco del cuerpo ---
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = lado == 0 ? -1.0f : 1.0f;
        const V hombro = suma(cadera, torso.aplicar({ sx * M::X_HOMBRO, altoTorso - 0.03f, 0 }));
        const Rot base = torso.por(Rot::enZ(sx * (lado == 0 ? p.separaI : p.separaD)));
        const float h = lado == 0 ? p.hombroI : p.hombroD;
        // El codo dobla hacia delante: el antebrazo sube.
        const float c = lado == 0 ? p.codoI : p.codoD;
        const auto fin = miembro(hombro, base, h, c, M::BRAZO, M::ANTEBRAZO,
                                 0.065f, 0.058f, CAMISA, PIEL);
        salida.push_back({ suma(fin.first, fin.second.aplicar({ 0, -M::MANO * 0.5f, 0 })),
                           fin.second, { 0.05f, M::MANO * 0.5f, 0.035f }, PIEL });
    }

    // --- Piernas: de la cadera; la rodilla dobla hacia atras ---
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = lado == 0 ? -1.0f : 1.0f;
        const V raiz = { sx * M::X_CADERA, yCadera, -M::ATRAS };
        const float c = lado == 0 ? p.caderaI : p.caderaD;
        const float r = lado == 0 ? p.rodillaI : p.rodillaD;
        const auto fin = miembro(raiz, Rot::identidad(), c, -r, M::MUSLO, M::PIERNA,
                                 0.085f, 0.07f, PANTALON, PANTALON);
        // El pie, plano y hacia delante desde el tobillo.
        // Se deshace el giro de muslo y pierna: el pie queda plano.
        const Rot pie = fin.second.por(Rot::enX(-(c - r)));
        salida.push_back({ suma(fin.first, pie.aplicar({ 0, -0.04f, 0.07f })), pie,
                           { 0.065f, 0.045f, 0.13f }, ZAPATO });
    }
}

// La altura del ojo de la figura en esa pose. Agachado, la camara del motor
// baja hasta 0.80 y la figura se queda en 1.26: se aplasta en vertical lo que
// falte para que el ojo siga encima de los hombros y no dentro del pecho.
inline float escalaVertical(const Pose& p, float alturaOjoCamara) {
    const float ojo = M::OJO - p.agachado * 0.36f;
    const float k = alturaOjoCamara / ojo;
    return k > 1.0f ? 1.0f : (k < 0.4f ? 0.4f : k);
}

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

} // namespace ModeloJugador
