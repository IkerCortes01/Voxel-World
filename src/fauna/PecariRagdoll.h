#ifndef PECARI_RAGDOLL_H
#define PECARI_RAGDOLL_H

#include <cmath>
#include <cstdint>
#include "PecariEsqueleto.h"
#include "PecariHeridas.h"

// ============================================================================
// EL PECARI MUERTO SE DESPLOMA: RAGDOLL
// ============================================================================
// PEDIDO: "que al morir se conviertan en ragdolls".
//
// Nueve puntos con fisica Verlet y restricciones de distancia:
//
//      cabeza
//        |
//   hombroI --- hombroD        <- el torso: cuatro puntos unidos en rigido
//      |   \   /   |
//   caderaI --- caderaD
//      |           |
//    cuatro pezunas            <- cada una colgada de su hombro o cadera, con
//                                 un largo que puede encogerse (la pata se dobla)
//
// No se simula hueso por hueso: de esos nueve puntos se SACAN la orientacion
// del cuerpo y los angulos de patas, cuello y cabeza, y con eso se mueve el
// mismo esqueleto y la misma malla que el animal vivo. Sin geometria nueva.
//
// COSTE: nueve puntos, dieciocho restricciones, seis iteraciones, a 60 Hz.
// Cuando se queda quieto deja de simularse.
//
// Unidades: posiciones del mundo en BLOQUES; el esqueleto, en METROS locales
// (+Z delante, +X a la derecha, +Y arriba).
// ============================================================================

namespace Fauna {

// Lo unico que el ragdoll necesita del mundo.
struct IMundoRagdoll {
    virtual bool solido(int x, int y, int z) const = 0;
    virtual ~IMundoRagdoll() = default;
};

class Ragdoll {
public:
    enum Punto { HI, HD, CI, CD, CAB, PDI, PDD, PTI, PTD, N };

    static constexpr float PASO = 1.0f / 60.0f;
    static constexpr float GRAVEDAD = 16.3f;      // 9.8 m/s2 en bloques de 0.6 m
    static constexpr int   ITERACIONES = 6;

    // Lo que hereda del animal vivo para dibujarse igual.
    int      etapa = 3;
    float    escala = 1.0f;
    uint32_t semilla = 0;
    float    erizado = 0.0f;
    int      lodActual = 0;

    std::vector<Herida> heridas;   // las que tenia al morir

    float edad = 0.0f;          // segundos desde que murio
    bool  dormido = false;      // quieto: ya no se simula

    // --------------------------------------------------------------------
    // NACER DEL ANIMAL VIVO
    // --------------------------------------------------------------------
    // `vx, vz` es la velocidad que llevaba y `empujeX, empujeZ` el golpe, en
    // bloques/s. Todo en el mismo marco que el render: orientacion 0 = +Z.
    void iniciar(const EsqueletoReposo& esq, float escalaAnimal,
                 float x, float y, float z, float orientacion,
                 float vx, float vz, float empujeX, float empujeZ) {
        escala = escalaAnimal;
        const auto piv = [&](HuesoAnimal h) { return esq.huesos[(int)h].pivote; };
        reposo[HI] = piv(HuesoAnimal::HOMBRO_DI);
        reposo[HD] = piv(HuesoAnimal::HOMBRO_DD);
        reposo[CI] = piv(HuesoAnimal::CADERA_TI);
        reposo[CD] = piv(HuesoAnimal::CADERA_TD);
        const V3 cab = piv(HuesoAnimal::CABEZA);
        reposo[CAB] = V3(cab.x, cab.y, cab.z + 0.12f);   // hacia el hocico
        cuelloReposo = piv(HuesoAnimal::CUELLO);
        reposo[PDI] = V3(reposo[HI].x, 0.02f, reposo[HI].z);
        reposo[PDD] = V3(reposo[HD].x, 0.02f, reposo[HD].z);
        reposo[PTI] = V3(reposo[CI].x, 0.02f, reposo[CI].z);
        reposo[PTD] = V3(reposo[CD].x, 0.02f, reposo[CD].z);

        const float esc = escala / 0.60f;
        const float cy = std::cos(orientacion), sy = std::sin(orientacion);
        for (int i = 0; i < N; ++i) {
            const V3& l = reposo[i];
            p[i] = V3(x + (l.x * cy + l.z * sy) * esc,
                      y + l.y * esc,
                      z + (-l.x * sy + l.z * cy) * esc);
            // Verlet: la velocidad es la diferencia con la posicion anterior.
            // El golpe levanta un poco el cuerpo y lo tumba hacia un lado.
            const float sube = (i <= CAB) ? 2.2f : 0.6f;
            prev[i] = V3(p[i].x - (vx + empujeX) * PASO,
                         p[i].y - sube * PASO,
                         p[i].z - (vz + empujeZ) * PASO);
        }
        construirRestricciones(esc);
        dormido = false;
        quietoSeg = 0.0f;
        edad = 0.0f;
    }

    // --------------------------------------------------------------------
    // SIMULAR
    // --------------------------------------------------------------------
    void avanzar(float dt, const IMundoRagdoll& mundo) {
        edad += dt;
        if (dormido) return;
        acumulado += dt;
        if (acumulado > 0.1f) acumulado = 0.1f;       // tras un tiron, no recuperar
        while (acumulado >= PASO) {
            acumulado -= PASO;
            paso(mundo);
        }
    }

    const V3& punto(int i) const { return p[i]; }
    const V3& puntoReposo(int i) const { return reposo[i]; }

    // --------------------------------------------------------------------
    // DE LOS PUNTOS AL ESQUELETO
    // --------------------------------------------------------------------
    // rot (3x3 por filas) y (tx,ty,tz): mundo = t + rot * (local * esc).
    // La pose son los giros de los huesos que mueve el ragdoll.
    void resolver(float rot[9], float& tx, float& ty, float& tz,
                  PoseEsqueleto& pose) const {
        const float esc = escala / 0.60f;
        float actual[9], reposoM[9];
        marco(p, actual);
        marco(reposo, reposoM);
        // rot = actual * reposo^T
        for (int f = 0; f < 3; ++f)
            for (int c = 0; c < 3; ++c)
                rot[f * 3 + c] = actual[f * 3 + 0] * reposoM[c * 3 + 0] +
                                 actual[f * 3 + 1] * reposoM[c * 3 + 1] +
                                 actual[f * 3 + 2] * reposoM[c * 3 + 2];

        const V3 cAct = centroTorso(p);
        const V3 cRep = centroTorso(reposo);
        const V3 r = aplicar(rot, V3(cRep.x * esc, cRep.y * esc, cRep.z * esc));
        tx = cAct.x - r.x; ty = cAct.y - r.y; tz = cAct.z - r.z;

        // Un punto del mundo, devuelto al espacio local del animal (metros).
        const auto local = [&](const V3& w) {
            const V3 d(w.x - tx, w.y - ty, w.z - tz);
            const V3 l(rot[0] * d.x + rot[3] * d.y + rot[6] * d.z,
                       rot[1] * d.x + rot[4] * d.y + rot[7] * d.z,
                       rot[2] * d.x + rot[5] * d.y + rot[8] * d.z);
            return V3(l.x / esc, l.y / esc, l.z / esc);
        };
        // Angulo de bisagra en X de una direccion, con el mismo convenio que
        // ResolverPose: girar +a lleva (0,-1,0) hacia -Z.
        const auto bisagra = [](const V3& d) { return std::atan2(-d.z, -d.y); };
        const auto cabeceo = [](const V3& d) { return std::atan2(-d.y, d.z); };
        const auto acotar = [](float a, float lim) {
            while (a > 3.14159265f) a -= 6.2831853f;
            while (a < -3.14159265f) a += 6.2831853f;
            return a < -lim ? -lim : (a > lim ? lim : a);
        };

        pose.limpiar();
        struct Pata { int pie, raiz; HuesoAnimal sup, med; };
        static const Pata PATAS[4] = {
            { PDI, HI, HuesoAnimal::HOMBRO_DI, HuesoAnimal::CODO_DI },
            { PDD, HD, HuesoAnimal::HOMBRO_DD, HuesoAnimal::CODO_DD },
            { PTI, CI, HuesoAnimal::CADERA_TI, HuesoAnimal::RODILLA_TI },
            { PTD, CD, HuesoAnimal::CADERA_TD, HuesoAnimal::RODILLA_TD },
        };
        for (const Pata& pt : PATAS) {
            const V3 raiz = reposo[pt.raiz];
            const V3 pie = local(p[pt.pie]);
            const V3 d1(pie.x - raiz.x, pie.y - raiz.y, pie.z - raiz.z);
            const V3 d0(reposo[pt.pie].x - raiz.x, reposo[pt.pie].y - raiz.y,
                        reposo[pt.pie].z - raiz.z);
            pose.giroX[(int)pt.sup] = acotar(bisagra(d1) - bisagra(d0), 1.6f);
            // Si la pata se acorto, se dobla por la articulacion media.
            const float encoge = 1.0f - d1.longitud() / (d0.longitud() + 1e-5f);
            pose.giroX[(int)pt.med] = acotar(encoge * 2.2f, 1.2f);
        }

        const V3 cab = local(p[CAB]);
        const V3 d1(cab.x - cuelloReposo.x, cab.y - cuelloReposo.y, cab.z - cuelloReposo.z);
        const V3 d0(reposo[CAB].x - cuelloReposo.x, reposo[CAB].y - cuelloReposo.y,
                    reposo[CAB].z - cuelloReposo.z);
        pose.giroX[(int)HuesoAnimal::CUELLO] = acotar(cabeceo(d1) - cabeceo(d0), 1.2f);
        pose.giroY[(int)HuesoAnimal::CABEZA] =
            acotar(std::atan2(d1.x, d1.z) - std::atan2(d0.x, d0.z), 1.0f);
        // La cola cae inerte.
        pose.giroX[(int)HuesoAnimal::COLA] = 0.6f;
    }

    // Centro del torso, en bloques: para la luz, el culling y la distancia.
    V3 centro() const { return centroTorso(p); }

    // --------------------------------------------------------------------
    // EL JUGADOR EMPUJA EL CUERPO
    // --------------------------------------------------------------------
    // PEDIDO: "que puedas empujarlo con la hitbox del jugador de forma
    // realista".
    //
    // El jugador es un cilindro vertical (pies en `py`); cada punto del
    // ragdoll, una esfera de `radioPunto`. El punto que queda dentro se saca
    // en horizontal, moviendo `p` y no `prev`: en Verlet eso ES darle
    // velocidad. Asi el cuerpo sigue el empujon, arrastra por las
    // restricciones lo que tiene unido y lo frena el roce con el suelo.
    // Empujar la cabeza lo gira; empujar el torso lo arrastra entero.
    //
    // `cesion` reparte el choque por masas (CesionAnteJugador): el cuerpo se
    // lleva esa fraccion del solape y el resto se devuelve en (outX, outZ)
    // para el jugador. Un cadaver de 18 kg estorba; no es humo.
    int empujarCon(float px, float py, float pz,
                   float radioJugador, float alturaJugador,
                   float radioPunto, float cesion,
                   float& outX, float& outZ) {
        int tocados = 0;
        const float rSuma = radioJugador + radioPunto;
        for (int i = 0; i < N; ++i) {
            // Pisandolo por encima o pasando por debajo no hay choque.
            if (p[i].y + radioPunto < py) continue;
            if (p[i].y - radioPunto > py + alturaJugador) continue;
            const float dx = p[i].x - px, dz = p[i].z - pz;
            const float d2 = dx * dx + dz * dz;
            if (d2 >= rSuma * rSuma || d2 < 1e-8f) continue;
            const float d = std::sqrt(d2);
            const float pen = rSuma - d;
            const float ux = dx / d, uz = dz / d;
            p[i].x += ux * pen * cesion;
            p[i].z += uz * pen * cesion;
            outX -= ux * pen * (1.0f - cesion);
            outZ -= uz * pen * (1.0f - cesion);
            ++tocados;
        }
        // Un cuerpo dormido no se simula: el empujon lo despierta.
        if (tocados > 0) { dormido = false; quietoSeg = 0.0f; }
        return tocados;
    }

    // --------------------------------------------------------------------
    // ¿LO TOCA UN RAYO?
    // --------------------------------------------------------------------
    // Distancia a lo largo del rayo hasta el cuerpo, o -1 si no lo toca en
    // `alcance`. `d` va normalizada. Cada punto es una esfera de `radioPunto`,
    // y se anade el centro del torso: es donde mas cuerpo hay y ningun punto
    // cae justo ahi.
    float tocaRayo(float ox, float oy, float oz,
                   float dx, float dy, float dz,
                   float alcance, float radioPunto) const {
        float mejor = -1.0f;
        const auto esfera = [&](const V3& c) {
            const float cx = c.x - ox, cy = c.y - oy, cz = c.z - oz;
            const float t = cx * dx + cy * dy + cz * dz;
            if (t < 0.0f) return;
            const float dist2 = cx * cx + cy * cy + cz * cz - t * t;
            const float r2 = radioPunto * radioPunto;
            if (dist2 > r2) return;
            float tEntra = t - std::sqrt(r2 - dist2);
            if (tEntra < 0.0f) tEntra = 0.0f;           // el ojo ya esta dentro
            if (tEntra > alcance) return;
            if (mejor < 0.0f || tEntra < mejor) mejor = tEntra;
        };
        for (int i = 0; i < N; ++i) esfera(p[i]);
        esfera(centroTorso(p));
        return mejor;
    }

private:
    V3 p[N], prev[N], reposo[N];
    V3 cuelloReposo;
    float acumulado = 0.0f, quietoSeg = 0.0f;

    struct Restriccion { uint8_t a, b; float min, max; };
    Restriccion r[24];
    int nR = 0;

    void unir(int a, int b, float esc, float fMin, float fMax) {
        const V3& A = reposo[a];
        const V3& B = reposo[b];
        const float l = V3(A.x - B.x, A.y - B.y, A.z - B.z).longitud() * esc;
        r[nR++] = { (uint8_t)a, (uint8_t)b, l * fMin, l * fMax };
    }

    void construirRestricciones(float esc) {
        nR = 0;
        // El torso, rigido: las seis parejas.
        const int T[4] = { HI, HD, CI, CD };
        for (int i = 0; i < 4; ++i)
            for (int j = i + 1; j < 4; ++j) unir(T[i], T[j], esc, 1.0f, 1.0f);
        // La cabeza, colgada de los dos hombros y separada de las caderas.
        unir(CAB, HI, esc, 1.0f, 1.0f);
        unir(CAB, HD, esc, 1.0f, 1.0f);
        unir(CAB, CI, esc, 0.85f, 1.05f);
        unir(CAB, CD, esc, 0.85f, 1.05f);
        // Las patas: se doblan (hasta la mitad) pero no se estiran, y no
        // atraviesan el cuerpo (la distancia a la esquina opuesta del torso).
        const int PIE[4][3] = { { PDI, HI, CD }, { PDD, HD, CI },
                                { PTI, CI, HD }, { PTD, CD, HI } };
        for (const auto& q : PIE) {
            unir(q[0], q[1], esc, 0.5f, 1.0f);
            unir(q[0], q[2], esc, 0.75f, 1.25f);
        }
    }

    void paso(const IMundoRagdoll& mundo) {
        constexpr float AMORTIGUA = 0.995f;
        float mov = 0.0f;
        for (int i = 0; i < N; ++i) {
            const V3 v((p[i].x - prev[i].x) * AMORTIGUA,
                       (p[i].y - prev[i].y) * AMORTIGUA,
                       (p[i].z - prev[i].z) * AMORTIGUA);
            prev[i] = p[i];
            p[i] = V3(p[i].x + v.x, p[i].y + v.y - GRAVEDAD * PASO * PASO, p[i].z + v.z);
        }
        for (int it = 0; it < ITERACIONES; ++it) {
            for (int k = 0; k < nR; ++k) {
                V3& A = p[r[k].a];
                V3& B = p[r[k].b];
                const V3 d(B.x - A.x, B.y - A.y, B.z - A.z);
                const float l = d.longitud();
                if (l < 1e-6f) continue;
                float objetivo = l;
                if (l < r[k].min) objetivo = r[k].min;
                else if (l > r[k].max) objetivo = r[k].max;
                else continue;
                const float f = 0.5f * (l - objetivo) / l;
                A = V3(A.x + d.x * f, A.y + d.y * f, A.z + d.z * f);
                B = V3(B.x - d.x * f, B.y - d.y * f, B.z - d.z * f);
            }
            for (int i = 0; i < N; ++i) chocar(i, mundo);
        }
        for (int i = 0; i < N; ++i) {
            const float dx = p[i].x - prev[i].x, dy = p[i].y - prev[i].y,
                        dz = p[i].z - prev[i].z;
            const float m = dx * dx + dy * dy + dz * dz;
            if (m > mov) mov = m;
        }
        // Medio segundo sin moverse mas de 2 mm por paso: se duerme.
        quietoSeg = (mov < 0.0035f * 0.0035f) ? quietoSeg + PASO : 0.0f;
        if (quietoSeg > 0.5f) dormido = true;
    }

    // Un punto dentro de un bloque solido sale por arriba si alli hay hueco;
    // si no, vuelve a donde estaba. Al tocar suelo pierde casi toda la
    // velocidad horizontal: un cuerpo no patina.
    void chocar(int i, const IMundoRagdoll& mundo) {
        const int bx = (int)std::floor(p[i].x);
        const int by = (int)std::floor(p[i].y);
        const int bz = (int)std::floor(p[i].z);
        if (!mundo.solido(bx, by, bz)) return;
        if (!mundo.solido(bx, by + 1, bz) && p[i].y - by > 0.3f) {
            p[i].y = (float)(by + 1);
        } else if (!mundo.solido(bx, by + 1, bz)) {
            p[i].y = (float)(by + 1);
        } else {
            p[i] = prev[i];
        }
        constexpr float ROCE = 0.35f;
        prev[i].x = p[i].x - (p[i].x - prev[i].x) * ROCE;
        prev[i].z = p[i].z - (p[i].z - prev[i].z) * ROCE;
        if (prev[i].y < p[i].y) prev[i].y = p[i].y;   // no rebota
    }

    static V3 aplicar(const float m[9], const V3& v) {
        return V3(m[0] * v.x + m[1] * v.y + m[2] * v.z,
                  m[3] * v.x + m[4] * v.y + m[5] * v.z,
                  m[6] * v.x + m[7] * v.y + m[8] * v.z);
    }

    static V3 centroTorso(const V3* q) {
        return V3((q[HI].x + q[HD].x + q[CI].x + q[CD].x) * 0.25f,
                  (q[HI].y + q[HD].y + q[CI].y + q[CD].y) * 0.25f,
                  (q[HI].z + q[HD].z + q[CI].z + q[CD].z) * 0.25f);
    }

    // Base ortonormal del torso, por filas: columnas = (lado, arriba, delante).
    static void marco(const V3* q, float m[9]) {
        V3 del((q[HI].x + q[HD].x - q[CI].x - q[CD].x) * 0.5f,
               (q[HI].y + q[HD].y - q[CI].y - q[CD].y) * 0.5f,
               (q[HI].z + q[HD].z - q[CI].z - q[CD].z) * 0.5f);
        V3 lado((q[HD].x + q[CD].x - q[HI].x - q[CI].x) * 0.5f,
                (q[HD].y + q[CD].y - q[HI].y - q[CI].y) * 0.5f,
                (q[HD].z + q[CD].z - q[HI].z - q[CI].z) * 0.5f);
        del = del.normalizado();
        V3 arriba(del.y * lado.z - del.z * lado.y,
                  del.z * lado.x - del.x * lado.z,
                  del.x * lado.y - del.y * lado.x);
        arriba = arriba.normalizado();
        lado = V3(arriba.y * del.z - arriba.z * del.y,
                  arriba.z * del.x - arriba.x * del.z,
                  arriba.x * del.y - arriba.y * del.x);
        m[0] = lado.x; m[1] = arriba.x; m[2] = del.x;
        m[3] = lado.y; m[4] = arriba.y; m[5] = del.y;
        m[6] = lado.z; m[7] = arriba.z; m[8] = del.z;
    }
};

} // namespace Fauna

#endif // PECARI_RAGDOLL_H
