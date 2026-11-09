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
enum : int { HL_CLAVICULA, HL_BRAZO, HL_ANTE1, HL_ANTE2, HL_MANO, HL_DEDO0 = 5,
             HL_MUSLO = HL_DEDO0 + 15, HL_PIERNA, HL_PIE, HL_DEDOS_PIE, HL_NUM };
constexpr int NUM_HUESOS = H_POR_LADO_INICIO + 2 * HL_NUM;
inline int huesoDe(int lado, int k) { return H_POR_LADO_INICIO + lado * HL_NUM + k; }

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
        H(HL_CLAVICULA) = { s.pecho, suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.05f, 0 })), s.hombro[l].p, 0.055f, H_PECHO };
        H(HL_BRAZO) = { s.hombro[l], s.hombro[l].p, s.codo[l].p, 0.075f, huesoDe(l, HL_CLAVICULA) };
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
    Hueso ref[NUM_HUESOS];
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
        const Esqueleto s = resolver(poseReferencia(), true);
        huesosDe(s, ref);
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
        // En primera persona la camara esta dentro de la cabeza: sin ella.
        indicesSinCabeza.clear();
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            bool cabeza = false;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = indices[t + k];
                if ((h0[v] == H_CABEZA && w0[v] > 0.5f) || (h1[v] == H_CABEZA && w0[v] < 0.5f)) cabeza = true;
            }
            if (!cabeza) indicesSinCabeza.insert(indicesSinCabeza.end(), { indices[t], indices[t + 1], indices[t + 2] });
        }
        buscarPuntos(s);
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
    void aplicarAjustes(const AjustesMalla& a) {
        if (a == ajustes && !base.empty()) return;
        ajustes = a;
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
        empujar({ p.cabeza.x, p.nariz.y + 0.07f, p.nariz.z - 0.03f }, 0.06f, { 0, 0.012f * (a.frente - 1) * 4, 0.02f * (a.frente - 1) * 4 });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.nariz.x + sx * 0.050f, p.nariz.y, p.nariz.z - 0.040f }, 0.035f, { sx * 0.012f * (a.pomulos - 1) * 3, 0, 0.006f * (a.pomulos - 1) * 3 });
        }
        zona({ p.menton.x, p.menton.y + 0.025f, p.menton.z - 0.05f }, 0.075f, a.mandibula, 1.0f, 1.0f);
        empujar(p.menton, 0.035f, { 0, -0.010f * (a.menton - 1) * 3, 0.018f * (a.menton - 1) * 3 });
        // Los ojos y las cejas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.ojo[l], 0.020f, a.ojos, a.ojos, a.ojos);
            empujar(p.ojo[l], 0.024f, { sx * 0.030f * (a.ojosSeparacion - 1), 0.040f * (a.ojosAltura - 1), 0 });
            empujar({ p.ojo[l].x, p.ojo[l].y + 0.018f, p.ojo[l].z + 0.004f }, 0.022f, { 0, 0.05f * (a.cejasAltura - 1), 0 });
        }
        // La nariz y la boca.
        // El largo: la punta sale (~2 cm de nariz: un 25 % son 5 mm); el ancho,
        // alrededor de la base.
        empujar(p.nariz, 0.030f, { 0, -0.004f * (a.narizLargo - 1), 0.020f * (a.narizLargo - 1) });
        zona({ p.nariz.x, p.nariz.y, p.nariz.z - 0.018f }, 0.035f, a.narizAncho, 1.0f, 1.0f);
        empujar({ p.nariz.x, p.nariz.y + 0.025f, p.nariz.z - 0.012f }, 0.022f, { 0, 0, 0.012f * (a.narizPuente - 1) * 3 });
        zona(p.boca, 0.030f, a.bocaAncho, a.labios, 1.0f);
        // Las orejas: tamano y lo despegadas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.oreja[l], 0.035f, a.orejas, a.orejas, a.orejas);
            empujar(p.oreja[l], 0.035f, { sx * 0.015f * a.orejasDespegadas, 0, 0 });
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
        // El busto, el vientre y los gluteos: abultan hacia fuera.
        for (int l = 0; l < 2; ++l) {
            empujar(p.pecho[l], 0.085f, { 0, -0.010f * a.busto, 0.055f * a.busto });
            zona(p.gluteo[l], 0.10f, 1.0f, 1.0f, a.gluteos);
        }
        empujar(p.vientre, 0.14f, { 0, 0, 0.07f * a.abdomen });
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
        S[H_PECHO] = { a.pecho, largoRel(H_PECHO), a.pecho };
        S[H_CABEZA] = { a.cabeza, a.cabeza, a.cabeza };
        for (int l = 0; l < 2; ++l) {
            auto H = [&](int k) { return huesoDe(l, k); };
            S[H(HL_CLAVICULA)] = { a.hombros, a.hombros, a.hombros };
            const float brazo = a.brazo * (1.0f + (a.biceps - 1.0f) * 0.6f);
            S[H(HL_BRAZO)] = { brazo, largoRel(H(HL_BRAZO)), brazo };
            S[H(HL_ANTE1)] = { a.antebrazo, largoRel(H(HL_ANTE1)), a.antebrazo };
            S[H(HL_ANTE2)] = { a.antebrazo, largoRel(H(HL_ANTE2)), a.antebrazo };
            S[H(HL_MANO)] = { a.mano, a.mano, a.mano };
            for (int d = 0; d < 15; ++d) S[H(HL_DEDO0 + d)] = { a.mano, a.mano, a.mano };
            S[H(HL_MUSLO)] = { a.muslo, largoRel(H(HL_MUSLO)), a.muslo };
            S[H(HL_PIERNA)] = { a.gemelo, largoRel(H(HL_PIERNA)), a.gemelo };
            S[H(HL_PIE)] = { a.pie, a.pie, a.pie };
            S[H(HL_DEDOS_PIE)] = { a.pie, a.pie, a.pie };
        }
    }

    // Las matrices de cada hueso para el shader: 3 filas de 4 por hueso
    // (rotacion*escala y traslacion), de la pose de referencia a la actual.
    void matrices(const Esqueleto& s, float* filas) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        V S[NUM_HUESOS];
        escalas(h, S);
        for (int b = 0; b < NUM_HUESOS; ++b) {
            const Rot escalada = h[b].marco.r.por(Rot{ { S[b].x, 0, 0, 0, S[b].y, 0, 0, 0, S[b].z } });
            const Rot R = escalada.por(detalle::traspuesta(ref[b].marco.r));
            const V T = resta(h[b].marco.p, R.aplicar(ref[b].marco.p));
            float* f = filas + b * 12;
            for (int k = 0; k < 3; ++k) {
                f[k * 4 + 0] = R.m[k * 3 + 0];
                f[k * 4 + 1] = R.m[k * 3 + 1];
                f[k * 4 + 2] = R.m[k * 3 + 2];
                f[k * 4 + 3] = k == 0 ? T.x : (k == 1 ? T.y : T.z);
            }
        }
    }

    // La malla en la pose del esqueleto `s`, en la CPU (sin shader, y para
    // las pruebas); sin la cabeza en primera persona.
    void deformar(const Esqueleto& s, Malla& m, bool conCabeza = true) const {
        const std::vector<uint32_t>& idx = indicesDe(conCabeza);
        if (m.i.size() != idx.size()) m.i = idx;
        float F[NUM_HUESOS * 12];
        matrices(s, F);
        auto aplicar = [&](int b, V p, float w) {
            const float* f = F + b * 12;
            return V{ (f[0] * p.x + f[1] * p.y + f[2] * p.z + f[3] * w), (f[4] * p.x + f[5] * p.y + f[6] * p.z + f[7] * w),
                      (f[8] * p.x + f[9] * p.y + f[10] * p.z + f[11] * w) };
        };
        m.v.resize(base.size());
        for (size_t i = 0; i < base.size(); ++i) {
            const VerticeJ& v = base[i];
            const int a = h0[i], b = h1[i];
            V p = aplicar(a, v.p, 1.0f);
            V nn = aplicar(a, v.n, 0.0f);
            if (a != b) {
                const float wa = w0[i], wb = 1.0f - wa;
                p = suma(escala(p, wa), escala(aplicar(b, v.p, 1.0f), wb));
                nn = suma(escala(nn, wa), escala(aplicar(b, v.n, 0.0f), wb));
            }
            m.v[i] = { p, normalizar(nn), v.r, v.g, v.b };
        }
    }
    const std::vector<uint32_t>& indicesDe(bool conCabeza) const {
        if (!conCabeza && indicesSinCabeza.empty()) return indices;
        return conCabeza ? indices : indicesSinCabeza;
    }
};

} // namespace ModeloJugador
