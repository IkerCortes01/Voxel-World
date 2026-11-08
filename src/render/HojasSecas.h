#pragma once
// ============================================================================
// LAS HOJAS SECAS: CAEN DE LOS ARBOLES, REVOLOTEAN Y SE AMONTONAN
// ============================================================================
// PEDIDO: "que los arboles poco a poco suelten hojas secas y tiradas y se
// acumulen como particulas con ragdolls".
//
// Cada hoja es un cuerpo pequeno con su propia fisica, como un ragdoll de
// una sola pieza:
//
//   SOLTARSE   de vez en cuando se mira una celda al azar cerca del jugador;
//              si es hoja de arbol con aire debajo, de ahi se suelta una hoja
//              seca. Asi un bosque suelta muchas, un arbol solo unas pocas, y
//              con viento mas (las rafagas las arrancan).
//   CAER       no cae como una piedra: el aire la frena hasta poco mas de un
//              bloque por segundo, el viento la arrastra, y va en VAIVEN, de
//              un lado a otro, girando y cabeceando como una hoja de verdad.
//              Atraviesa las copas (se cuela entre las hojas, frenada).
//   POSARSE    al tocar suelo --un bloque, una losa de nivel, el agua-- se
//              queda tumbada, casi plana, con su giro. Las que caen encima de
//              otras quedan un pelo mas arriba: se ve el monton.
//   VOLVER     tirada en el suelo no esta muerta: una rafaga fuerte la levanta
//              y la hace rodar, y al pasar el jugador corriendo las patea. Si
//              se rompe el bloque de debajo, vuelve a caer.
//
// Se amontonan hasta MAX; cuando no caben mas, la mas vieja de las tiradas
// deja sitio. Las que quedan muy lejos del jugador se van. No se guardan con
// el mundo: es decorado vivo.
//
// Sin OpenGL ni World: el motor le pasa una funcion que dice que hay en una
// celda, y la dibuja con lo que esto calcula (ver `vertices`).
// ============================================================================

#include <cmath>
#include <cstdint>

namespace HojasSecas {

constexpr int MAX = 1600;
constexpr float RADIO = 22.0f;        // donde se sueltan, alrededor del jugador
constexpr float LEJOS = 40.0f;        // mas alla, se van
constexpr float GRAVEDAD = 6.0f;      // bloques/s2: con el roce, ~1.5 bloques/s

// Lo que hay en una celda, para una hoja:
//   0      aire (o hierba: se cuela)
//   (0,1]  algo que la sostiene, hasta esa altura de la celda (1 = bloque
//          entero, 0.375 = losa de nivel 3, ~0.9 el agua: flota)
//   < 0    copa de arbol: se atraviesa, frenada
enum Estado : uint8_t { LIBRE = 0, CAYENDO, TIRADA };

struct Hoja {
    float x, y, z;
    float vx, vy, vz;
    float giro, vGiro;      // rumbo (radianes) y como gira
    float vaiven, ritmo;    // fase y frecuencia del balanceo
    float cabeceo;          // inclinacion de la punta: 0 plana
    float alabeo;           // inclinacion de lado
    float edad, vida;       // segundos
    float tam;              // largo, en bloques
    float r, g, b;
    float luz = 1.0f;       // la pone el motor
    Estado estado = LIBRE;
};

class Sistema {
public:
    Hoja h[MAX];
    int vivas = 0;          // cuantas no estan LIBRE (para la estadistica)

    float aleatorio() {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        return (float)(semilla & 0xFFFFFFu) * (1.0f / 16777216.0f);
    }

    // Suelta una hoja seca desde la cara de abajo de la celda (x, y, z).
    void soltar(int x, int y, int z) {
        Hoja* o = hueco();
        if (!o) return;
        o->x = (float)x + 0.1f + 0.8f * aleatorio();
        o->z = (float)z + 0.1f + 0.8f * aleatorio();
        o->y = (float)y - 0.02f;
        o->vx = o->vz = 0.0f;
        o->vy = -0.2f;
        o->giro = 6.2831853f * aleatorio();
        o->vGiro = (aleatorio() - 0.5f) * 5.0f;
        o->vaiven = 6.2831853f * aleatorio();
        o->ritmo = 2.2f + 2.0f * aleatorio();
        o->cabeceo = 0.0f;
        o->alabeo = 0.0f;
        o->edad = 0.0f;
        o->vida = 600.0f + 600.0f * aleatorio();
        o->tam = 0.20f + 0.14f * aleatorio();
        // Pardos, ocres y amarillos de hoja seca, cada una con su tono.
        static const float TONOS[6][3] = {
            {0.55f, 0.36f, 0.16f}, {0.68f, 0.48f, 0.20f}, {0.45f, 0.28f, 0.12f},
            {0.74f, 0.60f, 0.25f}, {0.52f, 0.40f, 0.18f}, {0.62f, 0.30f, 0.14f} };
        const int k = (int)(aleatorio() * 6.0f) % 6;
        const float v = 0.85f + 0.3f * aleatorio();
        o->r = TONOS[k][0] * v; o->g = TONOS[k][1] * v; o->b = TONOS[k][2] * v;
        o->estado = CAYENDO;
    }

    // Prueba `intentos` celdas al azar alrededor de (px, py, pz). `esHoja`
    // dice si una celda es hoja de arbol; `celda`, lo de arriba.
    template <class FnHoja, class FnCelda>
    void buscarArboles(float px, float py, float pz, int intentos, FnHoja esHoja, FnCelda celda) {
        for (int k = 0; k < intentos; ++k) {
            const float a = 6.2831853f * aleatorio();
            const float d = RADIO * std::sqrt(aleatorio());
            const int x = (int)std::floor(px + d * std::cos(a));
            const int z = (int)std::floor(pz + d * std::sin(a));
            const int y = (int)std::floor(py) - 4 + (int)(aleatorio() * 22.0f);
            // Solo las de abajo de la copa: con aire debajo, por donde caer.
            if (esHoja(x, y, z) && celda(x, y - 1, z) == 0.0f) soltar(x, y, z);
        }
    }

    // Un paso de fisica para todas. `viento` (vx, vz) en bloques/s, `racha`
    // 0..1 lo fuerte del momento; el jugador, sus pies y su velocidad.
    template <class FnCelda>
    void avanzar(float dt, float vientoX, float vientoZ, float racha,
                 float jx, float jy, float jz, float jvx, float jvz, FnCelda celda) {
        if (dt <= 0.0f) return;
        if (dt > 0.1f) dt = 0.1f;
        ++cuadro;
        vivas = 0;
        const float jRapidez = std::sqrt(jvx * jvx + jvz * jvz);
        for (int i = 0; i < MAX; ++i) {
            Hoja& o = h[i];
            if (o.estado == LIBRE) continue;
            o.edad += dt;
            const float dx = o.x - jx, dz = o.z - jz;
            if (o.edad > o.vida || dx * dx + dz * dz > LEJOS * LEJOS || o.y < -10.0f) {
                o.estado = LIBRE;
                continue;
            }
            ++vivas;
            if (o.estado == TIRADA) tirada(o, i, dt, vientoX, vientoZ, racha, jx, jy, jz, jvx, jvz, jRapidez, celda);
            else cayendo(o, dt, vientoX, vientoZ, celda);
        }
    }

    // Lo que se ve: cuanto le queda (se desvanece al final) de 0 a 1.
    static float opacidad(const Hoja& o) {
        const float quedan = o.vida - o.edad;
        return quedan > 20.0f ? 1.0f : (quedan > 0.0f ? quedan / 20.0f : 0.0f);
    }

    // Las cuatro esquinas de la hoja (punta, lado, punta, lado), en el mundo.
    // Un rombo alargado: de lejos se lee como hoja, y son cuatro vertices.
    static void vertices(const Hoja& o, float v[12]) {
        const float cp = std::cos(o.cabeceo), sp = std::sin(o.cabeceo);
        const float cq = std::cos(o.alabeo), sq = std::sin(o.alabeo);
        const float cg = std::cos(o.giro), sg = std::sin(o.giro);
        const float l = o.tam * 0.5f * opacidadTam(o), w = o.tam * 0.28f * opacidadTam(o);
        const float ax = cg * cp, ay = sp, az = sg * cp;      // a lo largo
        const float bx = -sg * cq, by = sq, bz = cg * cq;     // a lo ancho
        const float px[4] = { l, 0.0f, -l * 0.8f, 0.0f };
        const float pw[4] = { 0.0f, w, 0.0f, -w };
        for (int k = 0; k < 4; ++k) {
            v[k * 3 + 0] = o.x + ax * px[k] + bx * pw[k];
            v[k * 3 + 1] = o.y + ay * px[k] + by * pw[k];
            v[k * 3 + 2] = o.z + az * px[k] + bz * pw[k];
        }
    }

private:
    uint32_t semilla = 0x2545F491u;
    uint32_t cuadro = 0;

    // Al final de su vida, ademas de apagarse, se encoge: seca y deshecha.
    static float opacidadTam(const Hoja& o) { return 0.4f + 0.6f * opacidad(o); }

    // Un sitio libre, o la tirada mas vieja si ya no cabe ninguna.
    Hoja* hueco() {
        for (int k = 0; k < MAX; ++k) {
            cursor = (cursor + 1) % MAX;
            if (h[cursor].estado == LIBRE) return &h[cursor];
        }
        Hoja* vieja = nullptr;
        for (int k = 0; k < MAX; ++k)
            if (h[k].estado == TIRADA && (!vieja || h[k].edad > vieja->edad)) vieja = &h[k];
        return vieja;
    }
    int cursor = 0;

    template <class FnCelda>
    void cayendo(Hoja& o, float dt, float vientoX, float vientoZ, FnCelda celda) {
        const int cx = (int)std::floor(o.x), cy = (int)std::floor(o.y), cz = (int)std::floor(o.z);
        const float aqui = celda(cx, cy, cz);
        // En la copa: el roce es mucho mayor, se cuela despacio.
        const float roce = aqui < 0.0f ? 7.0f : 2.4f;
        // EL VAIVEN: la hoja planea de un lado a otro, perpendicular a su
        // largo, y cabecea al cambiar de sentido.
        o.vaiven += o.ritmo * dt;
        const float s = std::sin(o.vaiven);
        const float lado = 1.6f * s;
        const float ex = -std::sin(o.giro), ez = std::cos(o.giro);
        o.vx += ((vientoX - o.vx) * roce + ex * lado) * dt;
        o.vz += ((vientoZ - o.vz) * roce + ez * lado) * dt;
        // Cae mas cuando va plana por el centro del vaiven, menos en los
        // extremos (ahi planea): eso da el "columpio".
        o.vy += (-GRAVEDAD - o.vy * (roce + 1.6f) + 1.2f * std::fabs(s)) * dt;
        o.giro += o.vGiro * dt;
        o.cabeceo = 0.45f * std::cos(o.vaiven);
        o.alabeo = 0.55f * s;

        const float nx = o.x + o.vx * dt, ny = o.y + o.vy * dt, nz = o.z + o.vz * dt;
        // De lado contra un bloque: no lo atraviesa, resbala.
        const int bx = (int)std::floor(nx), bz = (int)std::floor(nz), by = (int)std::floor(o.y);
        const float lat = celda(bx, by, bz);
        if (lat > 0.0f && o.y < (float)by + lat) { o.vx *= -0.2f; o.vz *= -0.2f; }
        else { o.x = nx; o.z = nz; }
        o.y = ny;
        // ¿Ha tocado suelo?
        const int fy = (int)std::floor(o.y);
        const float suelo = celda((int)std::floor(o.x), fy, (int)std::floor(o.z));
        if (suelo > 0.0f && o.y < (float)fy + suelo) posar(o, (float)fy + suelo);
    }

    void posar(Hoja& o, float superficie) {
        // Un pelo por encima, distinto en cada una: las que caen sobre otras
        // se apilan y no parpadean entre si.
        o.y = superficie + 0.004f + 0.018f * aleatorio();
        o.vx = o.vy = o.vz = 0.0f;
        o.cabeceo = (aleatorio() - 0.5f) * 0.20f;
        o.alabeo = (aleatorio() - 0.5f) * 0.30f;
        o.estado = TIRADA;
    }

    template <class FnCelda>
    void tirada(Hoja& o, int i, float dt, float vientoX, float vientoZ, float racha,
                float jx, float jy, float jz, float jvx, float jvz, float jRapidez, FnCelda celda) {
        // EL JUGADOR LAS PATEA: pasando corriendo por encima, salen volando.
        const float dx = o.x - jx, dz = o.z - jz;
        if (jRapidez > 1.5f && dx * dx + dz * dz < 0.45f * 0.45f && std::fabs(o.y - jy) < 0.6f) {
            levantar(o, jvx * 0.5f + dx * 2.0f, 1.2f + 1.2f * aleatorio(), jvz * 0.5f + dz * 2.0f);
            return;
        }
        // LA RAFAGA: con viento fuerte, de vez en cuando una se levanta y
        // rueda. Cada hoja tiene su umbral: no se van todas a la vez.
        const float viento = std::sqrt(vientoX * vientoX + vientoZ * vientoZ);
        const float empuje = racha * viento;
        const float umbral = 4.0f + 6.0f * (float)((i * 2654435761u) >> 24) / 255.0f;
        if (empuje > umbral && aleatorio() < dt * 0.6f) {
            levantar(o, vientoX * 0.4f, 0.8f + 1.4f * aleatorio(), vientoZ * 0.4f);
            return;
        }
        // Sin apoyo (se rompio el bloque de debajo): vuelve a caer. Se mira
        // por turnos, unas pocas por frame.
        if ((i + (int)cuadro) % 24 == 0) {
            const int fy = (int)std::floor(o.y - 0.03f);
            const float suelo = celda((int)std::floor(o.x), fy, (int)std::floor(o.z));
            if (suelo <= 0.0f || o.y - 0.03f > (float)fy + suelo + 0.05f) levantar(o, 0.0f, 0.0f, 0.0f);
        }
    }

    void levantar(Hoja& o, float vx, float vy, float vz) {
        o.vx = vx; o.vy = vy; o.vz = vz;
        o.y += 0.05f;
        o.vGiro = (aleatorio() - 0.5f) * 9.0f;
        o.estado = CAYENDO;
    }
};

} // namespace HojasSecas
