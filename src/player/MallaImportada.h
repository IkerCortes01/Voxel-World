#pragma once

#include "MallaJugador.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR CON UN MODELO IMPORTADO (GLB -> .vwm)
// ============================================================================
// PEDIDOS: "remplaza el modelo del jugador por este model.glb" y "haz que se
// vea identico a lo que descargaste [...] y que pueda ser modificado en
// avatar con libertad".
//
// tools/convertir_jugador_glb.py reduce el GLB (1 millon de triangulos) a
// ~60.000 con un decimador que respeta las costuras UV, y deja la textura de
// color, el mapa de normales y una mascara de zonas (piel, pelo, camisa,
// short). Se ve con la textura original: ver render/CuerpoGL.h.
//
// Aqui se le pone el ESQUELETO DEL JUEGO (ModeloJugador): cada vertice sigue
// a uno o dos huesos (skinning). Asi hace todo lo que hacia el procedural --
// andar, golpear, los gestos de la tecla G y los limites anatomicos.
//
// LOS PESOS: el hueso de cada vertice es el mas cercano, midiendo la
// distancia en "grosores" de cada hueso. Si su padre o su hijo esta casi
// igual de cerca, el vertice se reparte entre los dos (codos, rodillas,
// hombros y caderas se doblan en curva).
//
// EL AVATAR, CON LIBERTAD:
//   - Cada hueso tiene su ESCALA: grosor (brazos, biceps, antebrazos, pecho,
//     cintura, cadera, muslos, gemelos) y tamano (cabeza, manos, pies). El
//     largo de cada hueso sigue a las proporciones del esqueleto: la malla se
//     estira con el.
//   - La cara y el cuerpo se moldean por ZONAS alrededor de puntos que se
//     buscan solos en la malla (la punta de la nariz, el menton, las orejas,
//     los ojos, el cuello, el pecho, el vientre, los gluteos): ancho y largo
//     de cara, frente, pomulos, mandibula, menton, ojos, cejas, nariz, boca,
//     labios, orejas, cuello, busto, abdomen y gluteos.
//   - Los colores (piel de los genes, pelo con canas, camisa y short) los
//     cambia el shader con la mascara de zonas.
// ============================================================================

namespace ModeloJugador {

// Un hueso: su marco, el segmento que ocupa y su grosor.
struct Hueso {
    Junta marco;
    V a, b;
    float radio;
    int padre;
};

enum : int { H_PELVIS, H_PECHO, H_CABEZA, H_POR_LADO_INICIO };
// Por cada lado, en este orden (ver huesosDe).
// Los _M son huesos de ayuda en cada articulacion: giran la mitad que su
// vecino (ver huesosDe). Con ellos rodillas, codos y caderas muy dobladas no
// se colapsan (el defecto tipico del skinning lineal).
enum : int { HL_CLAVICULA, HL_BRAZO, HL_ANTE1, HL_ANTE2, HL_MANO, HL_DEDO0 = 5,
             HL_MUSLO = HL_DEDO0 + 15, HL_PIERNA, HL_PIE, HL_DEDOS_PIE,
             HL_HOMBRO_M, HL_CODO_M, HL_MUNECA_M, HL_CADERA_M, HL_RODILLA_M, HL_TOBILLO_M, HL_NUM };
constexpr int NUM_HUESOS = H_POR_LADO_INICIO + 2 * HL_NUM;
inline int huesoDe(int lado, int k) { return H_POR_LADO_INICIO + lado * HL_NUM + k; }

namespace detalle {
// El giro a medio camino entre dos marcos (cuaterniones: la media normalizada
// es exactamente el slerp al 50 %).
struct Cuat { float w, x, y, z; };
inline Cuat aCuat(const Rot& r) {
    const float* m = r.m;
    Cuat q;
    const float tr = m[0] + m[4] + m[8];
    if (tr > 0.0f) {
        const float s = std::sqrt(tr + 1.0f) * 2.0f;
        q = { 0.25f * s, (m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s };
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const float s = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2.0f;
        q = { (m[7] - m[5]) / s, 0.25f * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s };
    } else if (m[4] > m[8]) {
        const float s = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2.0f;
        q = { (m[2] - m[6]) / s, (m[1] + m[3]) / s, 0.25f * s, (m[5] + m[7]) / s };
    } else {
        const float s = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2.0f;
        q = { (m[3] - m[1]) / s, (m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25f * s };
    }
    return q;
}
inline Rot aRot(Cuat q) {
    const float w = q.w, x = q.x, y = q.y, z = q.z;
    return { { 1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
               2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
               2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y) } };
}
inline Rot medioGiro(const Rot& a, const Rot& b) {
    const Cuat p = aCuat(a);
    Cuat q = aCuat(b);
    if (p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z < 0.0f) q = { -q.w, -q.x, -q.y, -q.z };
    Cuat m = { p.w + q.w, p.x + q.x, p.y + q.y, p.z + q.z };
    const float l = std::sqrt(m.w * m.w + m.x * m.x + m.y * m.y + m.z * m.z);
    if (l < 1e-6f) return a;
    m = { m.w / l, m.x / l, m.y / l, m.z / l };
    return aRot(m);
}
} // namespace detalle

// Los huesos de un esqueleto ya colocado. Mismo orden siempre: el de la pose
// de referencia y el de la actual se emparejan por indice.
inline void huesosDe(const Esqueleto& s, Hueso* h) {
    h[H_PELVIS] = { s.pelvis, suma(s.pelvis.p, s.pelvis.r.aplicar({ 0, -0.14f, 0 })),
                    suma(s.pelvis.p, s.pecho.r.aplicar({ 0, 0.16f, 0 })), 0.14f, -1 };
    h[H_PECHO] = { s.pecho, suma(s.pelvis.p, s.pecho.r.aplicar({ 0, 0.16f, 0 })),
                   suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.04f, 0 })), 0.13f, H_PELVIS };
    h[H_CABEZA] = { s.cuello, s.cuello.p, suma(s.cuello.p, s.cuello.r.aplicar({ 0, 0.22f, 0.02f })), 0.095f, H_PECHO };
    for (int l = 0; l < 2; ++l) {
        auto H = [&](int k) -> Hueso& { return h[huesoDe(l, k)]; };
        const Mano& m = s.mano[l];
        const Pie& pie = s.pie[l];
        H(HL_CLAVICULA) = { s.pecho, suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.05f, 0 })), s.hombro[l].p, 0.045f, H_PECHO };
        H(HL_BRAZO) = { s.hombro[l], s.hombro[l].p, s.codo[l].p, 0.090f, huesoDe(l, HL_CLAVICULA) };
        const V medio = mezclar(s.codo[l].p, m.muneca.p, 0.5f);
        H(HL_ANTE1) = { s.codo[l], s.codo[l].p, medio, 0.050f, huesoDe(l, HL_BRAZO) };
        H(HL_ANTE2) = { { medio, s.codo[l].r.por(Rot::enY(s.prono[l] * 0.5f)) }, medio, m.muneca.p, 0.040f,
                        huesoDe(l, HL_ANTE1) };
        H(HL_MANO) = { m.muneca, m.muneca.p, m.dedo[1][0].p, 0.034f, huesoDe(l, HL_ANTE2) };
        for (int d = 0; d < 5; ++d)
            for (int f = 0; f < 3; ++f)
                H(HL_DEDO0 + d * 3 + f) = { m.dedo[d][f], m.dedo[d][f].p, m.dedo[d][f + 1].p, 0.0095f,
                                            f == 0 ? huesoDe(l, HL_MANO) : huesoDe(l, HL_DEDO0 + d * 3 + f - 1) };
        H(HL_MUSLO) = { s.cadera[l], s.cadera[l].p, s.rodilla[l].p, 0.085f, H_PELVIS };
        H(HL_PIERNA) = { s.rodilla[l], s.rodilla[l].p, pie.tobillo.p, 0.060f, huesoDe(l, HL_MUSLO) };
        H(HL_PIE) = { pie.tobillo, pie.talon, pie.bola, 0.040f, huesoDe(l, HL_PIERNA) };
        H(HL_DEDOS_PIE) = { { pie.bola, pie.dedo[0][0].r }, pie.bola, pie.dedo[2][2].p, 0.020f, huesoDe(l, HL_PIE) };
        // Los de ayuda: en la articulacion, con el giro a medias.
        auto ayuda = [&](int k, int padre, int hijo, V p, float radio) {
            H(k) = { { p, detalle::medioGiro(h[padre].marco.r, h[hijo].marco.r) }, p, p, radio, padre };
            h[hijo].padre = huesoDe(l, k);
        };
        ayuda(HL_HOMBRO_M, huesoDe(l, HL_CLAVICULA), huesoDe(l, HL_BRAZO), s.hombro[l].p, 0.055f);
        ayuda(HL_CODO_M, huesoDe(l, HL_BRAZO), huesoDe(l, HL_ANTE1), s.codo[l].p, 0.055f);
        ayuda(HL_MUNECA_M, huesoDe(l, HL_ANTE2), huesoDe(l, HL_MANO), m.muneca.p, 0.035f);
        ayuda(HL_CADERA_M, H_PELVIS, huesoDe(l, HL_MUSLO), s.cadera[l].p, 0.10f);
        ayuda(HL_RODILLA_M, huesoDe(l, HL_MUSLO), huesoDe(l, HL_PIERNA), s.rodilla[l].p, 0.07f);
        ayuda(HL_TOBILLO_M, huesoDe(l, HL_PIERNA), huesoDe(l, HL_PIE), pie.tobillo.p, 0.05f);
    }
}

namespace detalle {
inline float distSegmento(V p, V a, V b) {
    const V ab = resta(b, a);
    const float l2 = punto(ab, ab);
    const float t = l2 > 1e-12f ? sujetar(punto(resta(p, a), ab) / l2, 0.0f, 1.0f) : 0.0f;
    return largo(resta(p, suma(a, escala(ab, t))));
}
// La transpuesta de una rotacion es su inversa.
inline Rot traspuesta(const Rot& r) {
    return { { r.m[0], r.m[3], r.m[6], r.m[1], r.m[4], r.m[7], r.m[2], r.m[5], r.m[8] } };
}
inline float suaveRadio(float d, float r) {
    const float u = sujetar(1.0f - d / r, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}
} // namespace detalle

// ----------------------------------------------------------------------------
// LO QUE EL AVATAR LE HACE AL MODELO
// ----------------------------------------------------------------------------
// Factores (1 = como el modelo) y empujes (0 = nada). Ver aplicarAvatarMalla
// en CabezaJugador.h, que los saca de los deslizadores.
struct AjustesMalla {
    // La escala de cada parte (grosor; tamano en cabeza, manos y pies).
    float cabeza = 1, cuello = 1, hombros = 1, brazo = 1, biceps = 1, antebrazo = 1, mano = 1;
    float pecho = 1, cintura = 1, cadera = 1, muslo = 1, gemelo = 1, pie = 1;
    // La cara y el cuerpo por zonas.
    float caraAncho = 1, caraLargo = 1, frente = 1, pomulos = 1, mandibula = 1, menton = 1;
    float ojos = 1, ojosSeparacion = 1, ojosAltura = 1, cejasAltura = 1;
    float narizLargo = 1, narizAncho = 1, narizPuente = 1, bocaAncho = 1, labios = 1;
    float orejas = 1, orejasDespegadas = 0, cuelloLargo = 1;
    float busto = 0, abdomen = 0, gluteos = 1;
    // Los de la segunda tanda (ver Avatar.h): 1 o 0 = nada.
    float craneo = 1, papada = 0, sienes = 1, ojosProfundidad = 1, ojosInclinacion = 0, cejasArco = 0, cejasGrosor = 1;
    float narizPunta = 0, narizAletas = 1, narizAltura = 1, bocaAltura = 1, sonrisa = 0, bocaSaliente = 0;
    float orejasAltura = 1, orejasPunta = 0, lobulos = 1, peloVolumen = 1, nuez = 0.3f, trapecio = 1;
    float triceps = 1, dedosLargo = 1, espalda = 1, pectoral = 0.2f, cuadriceps = 1, pieDedos = 1, cinturaAltura = 0;
    // El color que pide el avatar en cada zona y cuanto se usa (0 = el del
    // modelo tal cual; 1 = ese color con el sombreado de la textura).
    float piel[3] = { 1, 1, 1 }, pelo[3] = { 1, 1, 1 }, camisa[3] = { 1, 1, 1 }, shortC[3] = { 1, 1, 1 };
    float mezcla[4] = { 0, 0, 0, 0 };
    bool operator==(const AjustesMalla& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
};

struct MallaImportada {
    bool cargada = false;
    int formato = 1;                     // 1: color por vertice, 2: con uv y tangentes
    float params[32] = {};
    std::vector<VerticeJ> modelo;        // como viene del archivo (pose de referencia)
    std::vector<VerticeJ> base;          // con los ajustes de la cara y el cuerpo
    std::vector<float> uv, tangente;     // 2 y 4 por vertice (formato 2)
    std::vector<uint32_t> indices, indicesSinCabeza;
    std::vector<uint8_t> h0, h1;         // los dos huesos de cada vertice
    std::vector<float> w0;               // el peso del primero (el segundo, 1 - w0)
    Hueso ref[NUM_HUESOS];               // la pose de referencia del modelo, tal cual
    Hueso reposo[NUM_HUESOS];            // la misma con las proporciones del avatar
    Proporciones propReposo;
    AjustesMalla ajustes;
    int versionForma = 0;                // sube cada vez que cambia `base`

    // Los puntos de la cara y del cuerpo, en la pose de referencia.
    struct Puntos {
        V cabeza, nariz, menton, boca, ojo[2], oreja[2], cuelloBase, pecho[2], vientre, gluteo[2];
        float cabezaRadio = 0.11f;
    } pt;

    Proporciones proporciones() const {
        Proporciones p;
        p.torso = params[2]; p.brazo = params[3]; p.antebrazo = params[4];
        p.muslo = params[5]; p.pierna = params[6]; p.hombroY = params[7];
        p.mano = params[21] > 0.3f ? params[21] : 1.0f;
        return p;
    }
    // La pose en que esta hecho el modelo: en cruz, los brazos algo caidos.
    Pose poseReferencia() const {
        Pose p;
        p.prop = proporciones();
        for (Brazo& b : p.brazo) {
            b = Brazo{};
            b.abd = params[0]; b.flex = 0.0f; b.rot = 0.0f; b.codo = 0.0f;
            b.prono = 0.0f; b.munFlex = 0.0f; b.munDesv = 0.0f;
        }
        for (Pierna& g : p.pierna) { g = Pierna{}; g.abd = params[1]; g.rodilla = 0.0f; }
        return p;
    }
    // El color medio de cada zona en la textura (para recolorear).
    const float* colorZona(int k) const { return &params[9 + k * 3]; }

    bool cargar(const std::string& ruta) {
        cargada = false;
        FILE* f = std::fopen(ruta.c_str(), "rb");
        if (!f) return false;
        char magia[4];
        uint32_t nv = 0, nt = 0;
        bool ok = std::fread(magia, 1, 4, f) == 4 && std::fread(&nv, 4, 1, f) == 1 && std::fread(&nt, 4, 1, f) == 1;
        formato = ok && std::memcmp(magia, "VWM2", 4) == 0 ? 2 : (ok && std::memcmp(magia, "VWM1", 4) == 0 ? 1 : 0);
        ok = ok && formato != 0 && nv > 0 && nv < 2000000 && nt > 0 && nt < 4000000;
        const int nParams = formato == 2 ? 32 : 16;
        const int porVertice = formato == 2 ? 12 : 9;
        std::memset(params, 0, sizeof(params));
        std::vector<float> v;
        if (ok) {
            ok = std::fread(params, 4, nParams, f) == (size_t)nParams;
            v.resize((size_t)nv * porVertice);
            indices.resize((size_t)nt * 3);
            ok = ok && std::fread(v.data(), 4, v.size(), f) == v.size() &&
                 std::fread(indices.data(), 4, indices.size(), f) == indices.size();
        }
        std::fclose(f);
        if (!ok) return false;
        for (uint32_t i : indices) if (i >= nv) return false;
        modelo.resize(nv);
        uv.assign(formato == 2 ? (size_t)nv * 2 : 0, 0.0f);
        tangente.assign(formato == 2 ? (size_t)nv * 4 : 0, 0.0f);
        for (uint32_t i = 0; i < nv; ++i) {
            const float* q = &v[(size_t)i * porVertice];
            if (formato == 2) {
                modelo[i] = { { q[0], q[1], q[2] }, { q[3], q[4], q[5] }, 0.8f, 0.7f, 0.6f };
                uv[i * 2] = q[6]; uv[i * 2 + 1] = q[7];
                for (int k = 0; k < 4; ++k) tangente[i * 4 + k] = q[8 + k];
            } else {
                modelo[i] = { { q[0], q[1], q[2] }, { q[3], q[4], q[5] }, q[6], q[7], q[8] };
            }
        }
        atar();
        base = modelo;
        ajustes = AjustesMalla{};
        ++versionForma;
        cargada = true;
        return true;
    }

    // Los pesos de cada vertice contra el esqueleto en la pose de referencia,
    // y los puntos de la cara y el cuerpo.
    void atar() {
        // ⭐ LA MANO DEL ESQUELETO A LA MEDIDA DE LA DEL MODELO: en la cruz,
        // la punta del dedo medio tiene que llegar a donde acaba la mano del
        // modelo. Si no, los nudillos del esqueleto caen en mitad de los
        // dedos de la malla y al cerrar el puno los dedos se rompen.
        {
            const Esqueleto s0 = resolver(poseReferencia(), true);
            float punta = 0.0f;
            for (const VerticeJ& v : modelo) punta = std::fmax(punta, v.p.x);
            const float muneca = s0.mano[1].muneca.p.x, dedo = s0.mano[1].dedo[1][3].p.x;
            if (dedo - muneca > 0.05f && punta > muneca + 0.05f) {
                const float base = params[21] > 0.3f ? params[21] : 1.0f;
                params[21] = sujetar(base * (punta - muneca) / (dedo - muneca), 0.6f, 1.4f);
            }
        }
        const Esqueleto s = resolver(poseReferencia(), true);
        huesosDe(s, ref);
        for (int b = 0; b < NUM_HUESOS; ++b) reposo[b] = ref[b];
        propReposo = proporciones();
        const size_t n = modelo.size();
        // El esqueleto del juego va M::ATRAS por detras del ojo; la malla
        // viene centrada sobre los pies. Se mueve para que el eje del tronco
        // caiga en la columna del esqueleto.
        {
            double zx = 0.0, zz = 0.0;
            int cuantos = 0;
            for (const VerticeJ& v : modelo)
                if (v.p.y > 1.00f && v.p.y < 1.30f && std::fabs(v.p.x) < 0.15f) { zx += v.p.x; zz += v.p.z; ++cuantos; }
            if (cuantos > 0) {
                const float dx = s.pelvis.p.x - (float)(zx / cuantos), dz = s.pelvis.p.z - (float)(zz / cuantos);
                for (VerticeJ& v : modelo) { v.p.x += dx; v.p.z += dz; }
            }
        }
        h0.assign(n, 0); h1.assign(n, 0); w0.assign(n, 1.0f);
        for (size_t i = 0; i < n; ++i) {
            const V p = modelo[i].p;
            float d[NUM_HUESOS];
            int mejor = 0;
            for (int b = 0; b < NUM_HUESOS; ++b) {
                d[b] = detalle::distSegmento(p, ref[b].a, ref[b].b) / ref[b].radio;
                if (d[b] < d[mejor]) mejor = b;
            }
            int segundo = -1;
            for (int b = 0; b < NUM_HUESOS; ++b) {
                if (b == mejor || (ref[b].padre != mejor && ref[mejor].padre != b)) continue;
                if (segundo < 0 || d[b] < d[segundo]) segundo = b;
            }
            h0[i] = (uint8_t)mejor;
            h1[i] = (uint8_t)(segundo < 0 ? mejor : segundo);
            if (segundo >= 0) {
                const float a = std::fmax(d[mejor], 0.05f), b = std::fmax(d[segundo], 0.05f);
                const float fa = 1.0f / (a * a * a * a), fb = 1.0f / (b * b * b * b);
                w0[i] = fa / (fa + fb);
                if (w0[i] > 0.97f) { w0[i] = 1.0f; h1[i] = h0[i]; }
            }
        }
        buscarPuntos(s);
        // En primera persona la camara va en la cabeza (ver ojos()): la cabeza
        // y el cuello no se dibujan. BUG: dejandolos, al mirar abajo la vista
        // atravesaba el cuello y el cuello de la camisa por dentro. Al mirar
        // abajo el cuerpo se echa atras (ver el cuerpo en main.cpp) y la
        // camara queda delante del pecho: se ve el pecho por fuera, como una
        // persona, y lo que se quito queda detras de la camara.
        indicesSinCabeza.clear();
        const V entreOjos = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            bool fuera = false;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = indices[t + k];
                // La cabeza del menton para arriba; el cuello y los hombros se ven.
                const bool cabeza = ((h0[v] == H_CABEZA && w0[v] > 0.3f) || (h1[v] == H_CABEZA && w0[v] < 0.7f)) &&
                                    modelo[v].p.y > pt.menton.y + 0.005f;
                if (cabeza || largo(resta(modelo[v].p, entreOjos)) < 0.08f) fuera = true;
            }
            if (!fuera) indicesSinCabeza.insert(indicesSinCabeza.end(), { indices[t], indices[t + 1], indices[t + 2] });
        }
    }

    // ⭐ LOS PUNTOS DE REFERENCIA, buscados en la malla: la punta de la nariz
    // es lo mas adelantado de la cara; el menton, lo mas bajo de delante de
    // la cabeza; las orejas, lo mas ancho de la cabeza a cada lado; los ojos,
    // sobre la nariz y a los lados (con las medidas de una cara media).
    void buscarPuntos(const Esqueleto& s) {
        Puntos p;
        const float yCuello = s.cuello.p.y;
        double cx = 0, cy = 0, cz = 0;
        int n = 0;
        float yMax = 0.0f;
        for (const VerticeJ& v : modelo)
            if (v.p.y > yCuello + 0.06f) { cx += v.p.x; cy += v.p.y; cz += v.p.z; ++n; yMax = std::fmax(yMax, v.p.y); }
        p.cabeza = n ? V{ (float)(cx / n), (float)(cy / n), (float)(cz / n) } : s.cuello.p;
        p.cabezaRadio = std::fmax(0.08f, (yMax - yCuello) * 0.5f);
        p.nariz = p.cabeza;
        for (const VerticeJ& v : modelo)
            if (v.p.y > yCuello + 0.06f && std::fabs(v.p.x - p.cabeza.x) < 0.025f && v.p.y < p.cabeza.y + 0.04f &&
                v.p.z > p.nariz.z) p.nariz = v.p;
        p.menton = p.nariz;
        for (const VerticeJ& v : modelo)
            if (std::fabs(v.p.x - p.cabeza.x) < 0.02f && v.p.z > p.cabeza.z + 0.04f && v.p.y > yCuello &&
                v.p.y < p.menton.y && v.p.y < p.nariz.y) p.menton = v.p;
        p.boca = mezclar(p.nariz, p.menton, 0.42f);
        p.boca.z = p.nariz.z - 0.012f;
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            p.ojo[l] = { p.nariz.x + sx * 0.032f, p.nariz.y + 0.034f, p.nariz.z - 0.028f };
            V oreja = p.cabeza;
            for (const VerticeJ& v : modelo)
                if (std::fabs(v.p.y - p.cabeza.y) < 0.035f && v.p.y > yCuello + 0.06f && (v.p.x - p.cabeza.x) * sx > (oreja.x - p.cabeza.x) * sx)
                    oreja = v.p;
            p.oreja[l] = oreja;
            p.pecho[l] = { s.pelvis.p.x + sx * 0.09f, s.pelvis.p.y + 0.36f, s.pelvis.p.z + 0.11f };
            p.gluteo[l] = { s.pelvis.p.x + sx * 0.08f, s.pelvis.p.y - 0.06f, s.pelvis.p.z - 0.10f };
        }
        p.cuelloBase = s.cuello.p;
        p.vientre = { s.pelvis.p.x, s.pelvis.p.y + 0.12f, s.pelvis.p.z + 0.10f };
        pt = p;
    }

    // ⭐ LA CARA Y EL CUERPO POR ZONAS. Cada ajuste mueve los vertices de
    // alrededor de su punto con un peso que se apaga suave hasta su radio.
    void aplicarAjustes(const AjustesMalla& a) { aplicarAjustes(a, proporciones()); }
    void aplicarAjustes(const AjustesMalla& a, const Proporciones& prop) {
        const bool mismaProp = std::memcmp(&prop, &propReposo, sizeof(prop)) == 0;
        if (a == ajustes && mismaProp && !base.empty() && versionForma > 1) return;
        ajustes = a;
        propReposo = prop;
        base = modelo;
        // Escalar alrededor de `c` en cada eje, dentro del radio `r`.
        auto zona = [&](V c, float r, float kx, float ky, float kz) {
            if (kx == 1.0f && ky == 1.0f && kz == 1.0f) return;
            for (VerticeJ& v : base) {
                const float w = detalle::suaveRadio(largo(resta(v.p, c)), r);
                if (w <= 0.0f) continue;
                v.p.x = c.x + (v.p.x - c.x) * (1.0f + (kx - 1.0f) * w);
                v.p.y = c.y + (v.p.y - c.y) * (1.0f + (ky - 1.0f) * w);
                v.p.z = c.z + (v.p.z - c.z) * (1.0f + (kz - 1.0f) * w);
            }
        };
        auto empujar = [&](V c, float r, V d) {
            if (d.x == 0.0f && d.y == 0.0f && d.z == 0.0f) return;
            for (VerticeJ& v : base) {
                const float w = detalle::suaveRadio(largo(resta(v.p, c)), r);
                if (w > 0.0f) v.p = suma(v.p, escala(d, w));
            }
        };
        const Puntos& p = pt;
        const float R = p.cabezaRadio;
        // La cabeza entera: ancho y largo.
        zona(p.cabeza, R * 1.6f, a.caraAncho, a.caraLargo, 1.0f);
        // La frente, los pomulos, la mandibula y el menton.
        empujar({ p.cabeza.x, p.nariz.y + 0.07f, p.nariz.z - 0.03f }, 0.06f, { 0, 0.012f * (a.frente - 1) * 6, 0.02f * (a.frente - 1) * 6 });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.nariz.x + sx * 0.050f, p.nariz.y, p.nariz.z - 0.040f }, 0.035f, { sx * 0.012f * (a.pomulos - 1) * 5, 0, 0.006f * (a.pomulos - 1) * 5 });
        }
        zona({ p.menton.x, p.menton.y + 0.025f, p.menton.z - 0.05f }, 0.075f, a.mandibula, 1.0f, 1.0f);
        empujar(p.menton, 0.035f, { 0, -0.010f * (a.menton - 1) * 5, 0.018f * (a.menton - 1) * 5 });
        // Los ojos y las cejas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.ojo[l], 0.020f, a.ojos, a.ojos, a.ojos);
            empujar(p.ojo[l], 0.024f, { sx * 0.045f * (a.ojosSeparacion - 1), 0.060f * (a.ojosAltura - 1), 0.012f * (1 - a.ojosProfundidad) });
            // La comisura de fuera sube o baja: los ojos se inclinan.
            empujar({ p.ojo[l].x + sx * 0.016f, p.ojo[l].y, p.ojo[l].z - 0.004f }, 0.014f, { 0, 0.005f * a.ojosInclinacion, 0 });
            const V ceja = { p.ojo[l].x, p.ojo[l].y + 0.018f, p.ojo[l].z + 0.004f };
            empujar(ceja, 0.022f, { 0, 0.08f * (a.cejasAltura - 1), 0.004f * (a.cejasGrosor - 1) });
            empujar({ ceja.x + sx * 0.008f, ceja.y + 0.003f, ceja.z }, 0.012f, { 0, 0.005f * a.cejasArco, 0 });
        }
        // La nariz y la boca.
        // El largo: la punta sale (~2 cm de nariz: un 25 % son 5 mm); el ancho,
        // alrededor de la base.
        empujar(p.nariz, 0.030f, { 0, -0.006f * (a.narizLargo - 1), 0.035f * (a.narizLargo - 1) });
        empujar(p.nariz, 0.016f, { 0, 0.006f * a.narizPunta, 0 });
        zona({ p.nariz.x, p.nariz.y - 0.010f, p.nariz.z - 0.012f }, 0.020f, a.narizAletas, 1.0f, 1.0f);
        empujar({ p.nariz.x, p.nariz.y + 0.01f, p.nariz.z - 0.015f }, 0.035f, { 0, 0.06f * (a.narizAltura - 1), 0 });
        zona({ p.nariz.x, p.nariz.y, p.nariz.z - 0.018f }, 0.035f, a.narizAncho, 1.0f, 1.0f);
        empujar({ p.nariz.x, p.nariz.y + 0.025f, p.nariz.z - 0.012f }, 0.022f, { 0, 0, 0.012f * (a.narizPuente - 1) * 5 });
        zona(p.boca, 0.030f, a.bocaAncho, a.labios, 1.0f);
        empujar(p.boca, 0.032f, { 0, 0.07f * (a.bocaAltura - 1), 0.008f * a.bocaSaliente });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.boca.x + sx * 0.022f, p.boca.y, p.boca.z - 0.006f }, 0.013f, { sx * 0.002f * a.sonrisa, 0.004f * a.sonrisa, -0.001f * a.sonrisa });
        }
        // El craneo, las sienes y la papada.
        zona({ p.cabeza.x, p.cabeza.y, p.cabeza.z - 0.05f }, R * 1.2f, 1.0f, 1.0f, a.craneo);
        zona({ p.cabeza.x, p.ojo[0].y + 0.02f, p.ojo[0].z - 0.05f }, 0.06f, a.sienes, 1.0f, 1.0f);
        empujar({ p.menton.x, p.menton.y - 0.02f, p.menton.z - 0.04f }, 0.045f, { 0, -0.012f * a.papada, 0.010f * a.papada });
        // Las orejas: tamano y lo despegadas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.oreja[l], 0.035f, a.orejas, a.orejas, a.orejas);
            empujar(p.oreja[l], 0.035f, { sx * 0.025f * a.orejasDespegadas, 0.05f * (a.orejasAltura - 1), 0 });
            empujar({ p.oreja[l].x, p.oreja[l].y + 0.025f, p.oreja[l].z - 0.005f }, 0.018f, { sx * 0.004f * a.orejasPunta, 0.012f * a.orejasPunta, -0.004f * a.orejasPunta });
            zona({ p.oreja[l].x, p.oreja[l].y - 0.025f, p.oreja[l].z }, 0.014f, a.lobulos, a.lobulos, a.lobulos);
        }
        // El cuello: lo de encima sube (la cabeza con el) y la franja se
        // estira; y su grosor.
        if (a.cuelloLargo != 1.0f) {
            const float dy = 0.07f * (a.cuelloLargo - 1.0f), y0 = p.cuelloBase.y - 0.03f;
            for (VerticeJ& v : base) {
                const float u = sujetar((v.p.y - y0) / 0.08f, 0.0f, 1.0f);
                if (std::fabs(v.p.x - p.cuelloBase.x) < 0.35f) v.p.y += dy * u;
            }
        }
        // El pelo: lo de arriba de la cabeza, mas o menos voluminoso.
        if (a.peloVolumen != 1.0f)
            for (VerticeJ& v : base) {
                const float u = sujetar((v.p.y - (p.ojo[0].y + 0.035f)) / 0.05f, 0.0f, 1.0f);
                if (u <= 0.0f || largo(resta(v.p, p.cabeza)) > R * 2.0f) continue;
                const float k = 1.0f + (a.peloVolumen - 1.0f) * u;
                v.p = suma(p.cabeza, escala(resta(v.p, p.cabeza), k));
            }
        // La nuez, los trapecios y los pectorales.
        empujar({ p.cuelloBase.x, p.cuelloBase.y + 0.03f, p.cuelloBase.z + 0.05f }, 0.025f, { 0, 0, 0.014f * (a.nuez - 0.3f) });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.cuelloBase.x + sx * 0.09f, p.cuelloBase.y - 0.03f, p.cuelloBase.z - 0.02f }, 0.06f, { 0, 0.03f * (a.trapecio - 1), 0 });
            empujar({ p.pecho[l].x, p.pecho[l].y + 0.04f, p.pecho[l].z }, 0.08f, { 0, 0, 0.025f * (a.pectoral - 0.2f) });
        }
        // El busto, el vientre y los gluteos: abultan hacia fuera.
        for (int l = 0; l < 2; ++l) {
            empujar(p.pecho[l], 0.085f, { 0, -0.010f * a.busto, 0.055f * a.busto });
            zona({ p.gluteo[l].x, p.gluteo[l].y + 0.05f * a.cinturaAltura, p.gluteo[l].z }, 0.10f, 1.0f, 1.0f, a.gluteos);
        }
        empujar(p.vientre, 0.14f, { 0, 0, 0.07f * a.abdomen });

        // ⭐ EL REPOSO CON LAS PROPORCIONES Y GROSORES DEL AVATAR. El esqueleto
        // de referencia con los largos pedidos, y cada vertice llevado alli
        // con la escala de sus huesos (grosor y largo). Se hace una vez por
        // cambio; cada frame solo se GIRA (ver cuaternionesDuales), que es lo
        // que permite el skinning con cuaterniones duales sin estirar nada.
        Pose pr = poseReferencia();
        pr.prop = prop;
        huesosDe(resolver(pr, true), reposo);
        V S[NUM_HUESOS];
        escalas(reposo, S);
        Rot M[NUM_HUESOS];
        V T[NUM_HUESOS];
        for (int b = 0; b < NUM_HUESOS; ++b) {
            const Rot escalada = reposo[b].marco.r.por(Rot{ { S[b].x, 0, 0, 0, S[b].y, 0, 0, 0, S[b].z } });
            M[b] = escalada.por(detalle::traspuesta(ref[b].marco.r));
            T[b] = resta(reposo[b].marco.p, M[b].aplicar(ref[b].marco.p));
        }
        for (size_t i = 0; i < base.size(); ++i) {
            const int x = h0[i], y = h1[i];
            const V p0 = base[i].p;
            V q = suma(M[x].aplicar(p0), T[x]);
            if (x != y) q = suma(escala(q, w0[i]), escala(suma(M[y].aplicar(p0), T[y]), 1.0f - w0[i]));
            base[i].p = q;
        }
        ++versionForma;
    }

    // La escala de cada hueso en su propio marco: x y z el grosor, y el
    // largo. El largo sale solo: lo que mide el hueso ahora entre lo que media
    // en la pose de referencia (las proporciones del avatar lo cambian).
    void escalas(const Hueso* h, V* S) const {
        const AjustesMalla& a = ajustes;
        for (int b = 0; b < NUM_HUESOS; ++b) S[b] = { 1, 1, 1 };
        auto largoRel = [&](int b) {
            const float l0 = largo(resta(ref[b].b, ref[b].a));
            return l0 > 1e-4f ? largo(resta(h[b].b, h[b].a)) / l0 : 1.0f;
        };
        S[H_PELVIS] = { a.cadera, 1.0f, 0.5f * (a.cadera + a.cintura) };
        S[H_PECHO] = { a.pecho * a.espalda, largoRel(H_PECHO), a.pecho };
        S[H_CABEZA] = { a.cabeza, a.cabeza, a.cabeza };
        for (int l = 0; l < 2; ++l) {
            auto H = [&](int k) { return huesoDe(l, k); };
            S[H(HL_CLAVICULA)] = { a.hombros, a.hombros, a.hombros };
            const float brazo = a.brazo * (1.0f + (a.biceps - 1.0f) * 0.6f) * (1.0f + (a.triceps - 1.0f) * 0.4f);
            S[H(HL_BRAZO)] = { brazo, largoRel(H(HL_BRAZO)), brazo };
            S[H(HL_ANTE1)] = { a.antebrazo, largoRel(H(HL_ANTE1)), a.antebrazo };
            S[H(HL_ANTE2)] = { a.antebrazo, largoRel(H(HL_ANTE2)), a.antebrazo };
            S[H(HL_MANO)] = { a.mano, a.mano, a.mano };
            for (int d = 0; d < 15; ++d) S[H(HL_DEDO0 + d)] = { a.mano, a.mano * a.dedosLargo, a.mano };
            S[H(HL_MUSLO)] = { a.muslo, largoRel(H(HL_MUSLO)), a.muslo * (1.0f + (a.cuadriceps - 1.0f) * 0.7f) };
            S[H(HL_PIERNA)] = { a.gemelo, largoRel(H(HL_PIERNA)), a.gemelo };
            S[H(HL_PIE)] = { a.pie, a.pie, a.pie };
            S[H(HL_DEDOS_PIE)] = { a.pie, a.pie, a.pie * a.pieDedos };
            // Los de ayuda: el grosor medio de sus dos vecinos, sin estirar.
            auto media = [&](int k, int x, int y) { S[H(k)] = { 0.5f * (S[x].x + S[y].x), 1.0f, 0.5f * (S[x].z + S[y].z) }; };
            media(HL_HOMBRO_M, H(HL_CLAVICULA), H(HL_BRAZO));
            media(HL_CODO_M, H(HL_BRAZO), H(HL_ANTE1));
            media(HL_MUNECA_M, H(HL_ANTE2), H(HL_MANO));
            media(HL_CADERA_M, H_PELVIS, H(HL_MUSLO));
            media(HL_RODILLA_M, H(HL_MUSLO), H(HL_PIERNA));
            media(HL_TOBILLO_M, H(HL_PIERNA), H(HL_PIE));        }
    }

    // ⭐ CADA HUESO COMO CUATERNION DUAL (giro + traslacion), del reposo a la
    // pose actual: 8 floats por hueso (giro x,y,z,w y la parte dual x,y,z,w).
    //
    // Con matrices (skinning lineal) un vertice entre dos huesos muy girados
    // promedia dos posiciones y se mete hacia dentro: codos y rodillas se
    // adelgazan, la muneca girada se retuerce como un caramelo y los dedos se
    // aplastan. Mezclando cuaterniones duales el vertice GIRA alrededor de la
    // articulacion y conserva el volumen.
    void cuaternionesDuales(const Esqueleto& s, float* dq) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        for (int b = 0; b < NUM_HUESOS; ++b) {
            const Rot R = h[b].marco.r.por(detalle::traspuesta(reposo[b].marco.r));
            const V t = resta(h[b].marco.p, R.aplicar(reposo[b].marco.p));
            const detalle::Cuat q = detalle::aCuat(R);
            float* o = dq + b * 8;
            o[0] = q.x; o[1] = q.y; o[2] = q.z; o[3] = q.w;
            // La parte dual: (0, t) * q / 2.
            o[4] = 0.5f * (t.x * q.w + t.y * q.z - t.z * q.y);
            o[5] = 0.5f * (-t.x * q.z + t.y * q.w + t.z * q.x);
            o[6] = 0.5f * (t.x * q.y - t.y * q.x + t.z * q.w);
            o[7] = -0.5f * (t.x * q.x + t.y * q.y + t.z * q.z);
        }
    }

    // ⭐ DONDE ESTAN SUS OJOS en la pose del esqueleto `s`: el punto entre los
    // dos, un poco por dentro, siguiendo a la cabeza (gira, se inclina y
    // escala con ella). Ahi va la camara en primera persona.
    V ojos(const Esqueleto& s) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        const Junta& r0 = ref[H_CABEZA].marco;
        const V entre = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        V local = detalle::traspuesta(r0.r).aplicar(resta(entre, r0.p));
        // Algo por encima y por delante de los ojos: asi la camara no roza los
        // parpados ni las cejas y al mirar abajo ve el cuello y los hombros
        // por fuera, sin meterse en ellos.
        local.y += 0.045f;
        local.z += 0.03f;
        local = escala(local, ajustes.cabeza);
        return suma(h[H_CABEZA].marco.p, h[H_CABEZA].marco.r.aplicar(local));
    }
    // La malla en la pose del esqueleto `s`, en la CPU (sin shader, y para
    // las pruebas); sin la cabeza en primera persona.
    void deformar(const Esqueleto& s, Malla& m, bool conCabeza = true) const {
        const std::vector<uint32_t>& idx = indicesDe(conCabeza);
        if (m.i.size() != idx.size()) m.i = idx;
        float DQ[NUM_HUESOS * 8];
        cuaternionesDuales(s, DQ);
        m.v.resize(base.size());
        for (size_t i = 0; i < base.size(); ++i) {
            const VerticeJ& v = base[i];
            const float* A = DQ + h0[i] * 8;
            const float* B = DQ + h1[i] * 8;
            const float wa = w0[i];
            const float signo = (A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3]) < 0.0f ? -1.0f : 1.0f;
            const float wb = (1.0f - wa) * signo;
            float r[4], d[4];
            for (int k = 0; k < 4; ++k) { r[k] = A[k] * wa + B[k] * wb; d[k] = A[4 + k] * wa + B[4 + k] * wb; }
            const float l = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);
            for (int k = 0; k < 4; ++k) { r[k] /= l; d[k] /= l; }
            const V qv = { r[0], r[1], r[2] };
            auto girar = [&](V p) {
                const V c = suma(cruz(qv, p), escala(p, r[3]));
                return suma(p, escala(cruz(qv, c), 2.0f));
            };
            const V dv = { d[0], d[1], d[2] };
            const V t = escala(suma(resta(escala(dv, r[3]), escala(qv, d[3])), cruz(qv, dv)), 2.0f);
            m.v[i] = { suma(girar(v.p), t), normalizar(girar(v.n)), v.r, v.g, v.b };
        }
    }
    const std::vector<uint32_t>& indicesDe(bool conCabeza) const {
        if (!conCabeza && indicesSinCabeza.empty()) return indices;
        return conCabeza ? indices : indicesSinCabeza;
    }
};

} // namespace ModeloJugador
