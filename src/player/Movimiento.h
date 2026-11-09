#pragma once

#include "ModeloJugador.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ============================================================================
// EL MOVIMIENTO PROGRAMABLE DEL JUGADOR
// ============================================================================
// PEDIDO: "mejora el modelo para que tenga articulaciones, movimiento realista
// del humano real programable y ser anatomicamente perfecto y modificable".
//
// Cada articulacion del esqueleto (ModeloJugador::Pose) es un CANAL con nombre,
// unidad y su rango sano. Una ANIMACION es una lista de claves: en el segundo
// t, tales canales valen tanto. Entre clave y clave se interpola con una curva
// Catmull-Rom: la velocidad no se corta en cada clave, como en un movimiento
// de verdad, que acelera y frena sin tirones.
//
// Las animaciones se escriben en texto. Las que trae el juego estan aqui
// abajo; las del jugador van en la carpeta animaciones/ (*.txt), con el mismo
// formato:
//
//     # un comentario
//     nombre   SALUDAR
//     duracion 2.6          segundos
//     bucle    no           si = se repite hasta que se pare
//     curva    natural      natural (Catmull-Rom) | suave | lineal
//     0.35  brazo.d.abd=100  codo.d=95  antebrazo.d.prono=-40
//     0.70  brazo.d.rot=-25
//
// Cada linea que empieza por un numero es una clave: el tiempo y luego
// canal=valor. Los angulos van en GRADOS. El lado es .i (izquierda), .d
// (derecha) o .a (ambos a la vez). Un canal que no sale en una clave sigue su
// curva entre las claves que si lo tienen. Antes de su primera clave y despues
// de la ultima (sin bucle) vuelve a la pose del juego: la animacion entra y
// sale sola, sin saltos.
//
// Todo pasa al final por ModeloJugador::limitar, asi que ninguna animacion
// puede romper una articulacion: un codo no se dobla hacia atras aunque el
// archivo lo pida, y la cadera no sube con la rodilla recta mas de lo que
// dejan los isquiotibiales.
// ============================================================================

namespace Movimiento {

using ModeloJugador::Pose;
constexpr float GRAD = 3.14159265f / 180.0f;

// ----------------------------------------------------------------------------
// LOS CANALES
// ----------------------------------------------------------------------------
enum Unidad : uint8_t { GRADOS, FRACCION };

struct Canal {
    std::string nombre;     // "brazo.d.flex"
    std::string etiqueta;   // "HOMBRO DER FLEXION"
    Unidad unidad;
    float minimo, maximo;   // en la unidad del canal (grados o fraccion)
    float (*leer)(const Pose&, int lado);
    void (*poner)(Pose&, int lado, float v);   // v ya en radianes o fraccion
    int lado;               // -1 global, 0 izquierda, 1 derecha
};

namespace detalle {
// Los dedos se doblan juntos con la proporcion de una mano real que se
// cierra: la falange del medio la que mas, la ultima algo menos.
constexpr float CIERRE_DEDO[3] = { 1.45f, 1.75f, 1.00f };
constexpr float CIERRE_PULGAR[3] = { 0.60f, 0.70f, 0.90f };
inline void cerrarDedo(Pose& p, int l, int d, float v) {
    const float* k = d == 4 ? CIERRE_PULGAR : CIERRE_DEDO;
    for (int f = 0; f < 3; ++f) p.brazo[l].dedo[d][f] = k[f] * v;
}
inline float cierreDedo(const Pose& p, int l, int d) {
    return p.brazo[l].dedo[d][1] / (d == 4 ? CIERRE_PULGAR[1] : CIERRE_DEDO[1]);
}
} // namespace detalle

// Todos los canales, en orden de la cabeza a los pies.
inline const std::vector<Canal>& canales() {
    static std::vector<Canal> C;
    if (!C.empty()) return C;
    using namespace ModeloJugador::Lim;
    auto global = [](const char* n, const char* e, Unidad u, float lo, float hi,
                     float (*leer)(const Pose&, int), void (*poner)(Pose&, int, float)) {
        C.push_back({ n, e, u, lo, hi, leer, poner, -1 });
    };
    const float G = 1.0f / GRAD;
    global("cuello.flex", "CUELLO FLEXION", GRADOS, -CUELLO_FLEX * G, CUELLO_FLEX * G,
           [](const Pose& p, int) { return p.cuelloFlex; }, [](Pose& p, int, float v) { p.cuelloFlex = v; });
    global("cuello.giro", "CUELLO GIRO", GRADOS, -CUELLO_GIRO * G, CUELLO_GIRO * G,
           [](const Pose& p, int) { return p.cuelloGiro; }, [](Pose& p, int, float v) { p.cuelloGiro = v; });
    global("cuello.lado", "CUELLO LATERAL", GRADOS, -CUELLO_LADO * G, CUELLO_LADO * G,
           [](const Pose& p, int) { return p.cuelloLado; }, [](Pose& p, int, float v) { p.cuelloLado = v; });
    global("torso.inclina", "TORSO ADELANTE", GRADOS, INCLINA_MIN * G, INCLINA_MAX * G,
           [](const Pose& p, int) { return p.inclinacion; }, [](Pose& p, int, float v) { p.inclinacion = v; });
    global("torso.giro", "TORSO GIRO", GRADOS, -TORSO_GIRO * G, TORSO_GIRO * G,
           [](const Pose& p, int) { return p.giroTorso; }, [](Pose& p, int, float v) { p.giroTorso = v; });
    global("torso.lado", "TORSO LATERAL", GRADOS, -TORSO_LADO * G, TORSO_LADO * G,
           [](const Pose& p, int) { return p.torsoLado; }, [](Pose& p, int, float v) { p.torsoLado = v; });
    global("pelvis.inclina", "PELVIS ADELANTE", GRADOS, -15.0f, 120.0f,
           [](const Pose& p, int) { return p.pelvisInclina; }, [](Pose& p, int, float v) { p.pelvisInclina = v; });
    global("pelvis.giro", "PELVIS GIRO", GRADOS, -30.0f, 30.0f,
           [](const Pose& p, int) { return p.giroPelvis; }, [](Pose& p, int, float v) { p.giroPelvis = v; });
    global("pelvis.lado", "PELVIS CAIDA", GRADOS, -10.0f, 10.0f,
           [](const Pose& p, int) { return p.pelvisLado; }, [](Pose& p, int, float v) { p.pelvisLado = v; });

    static const char* LADO[2] = { "i", "d" };
    static const char* LADO_E[2] = { "IZQ", "DER" };
    for (int l = 0; l < 2; ++l) {
        auto lateral = [&](const char* n, const char* e, Unidad u, float lo, float hi,
                           float (*leer)(const Pose&, int), void (*poner)(Pose&, int, float)) {
            // "brazo.flex" -> "brazo.d.flex"
            std::string nombre = n;
            const size_t punto = nombre.find('.');
            nombre = punto == std::string::npos ? nombre + "." + LADO[l]
                                                : nombre.substr(0, punto) + "." + LADO[l] + nombre.substr(punto);
            std::string etiqueta = e;
            const size_t esp = etiqueta.find(' ');
            etiqueta = esp == std::string::npos ? etiqueta + " " + LADO_E[l]
                                                : etiqueta.substr(0, esp) + " " + LADO_E[l] + etiqueta.substr(esp);
            C.push_back({ nombre, etiqueta, u, lo, hi, leer, poner, l });
        };
        lateral("hombro.encoger", "HOMBRO ENCOGER", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return p.brazo[l].encoger; }, [](Pose& p, int l, float v) { p.brazo[l].encoger = v; });
        lateral("hombro.adelantar", "HOMBRO ADELANTAR", FRACCION, -1.0f, 1.0f,
                [](const Pose& p, int l) { return p.brazo[l].adelantar; }, [](Pose& p, int l, float v) { p.brazo[l].adelantar = v; });
        lateral("brazo.flex", "HOMBRO FLEXION", GRADOS, HOMBRO_FLEX_MIN * G, HOMBRO_FLEX_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].flex; }, [](Pose& p, int l, float v) { p.brazo[l].flex = v; });
        lateral("brazo.abd", "HOMBRO ABDUCCION", GRADOS, HOMBRO_ABD_MIN * G, HOMBRO_ABD_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].abd; }, [](Pose& p, int l, float v) { p.brazo[l].abd = v; });
        lateral("brazo.rot", "HOMBRO ROTACION", GRADOS, HOMBRO_ROT_MIN * G, HOMBRO_ROT_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].rot; }, [](Pose& p, int l, float v) { p.brazo[l].rot = v; });
        lateral("codo", "CODO", GRADOS, 0.0f, CODO_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].codo; }, [](Pose& p, int l, float v) { p.brazo[l].codo = v; });
        lateral("antebrazo.prono", "ANTEBRAZO PRONACION", GRADOS, PRONO_MIN * G, PRONO_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].prono; }, [](Pose& p, int l, float v) { p.brazo[l].prono = v; });
        lateral("muneca.flex", "MUNECA FLEXION", GRADOS, MUNECA_FLEX_MIN * G, MUNECA_FLEX_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].munFlex; }, [](Pose& p, int l, float v) { p.brazo[l].munFlex = v; });
        lateral("muneca.desv", "MUNECA DESVIACION", GRADOS, MUNECA_DESV_MIN * G, MUNECA_DESV_MAX * G,
                [](const Pose& p, int l) { return p.brazo[l].munDesv; }, [](Pose& p, int l, float v) { p.brazo[l].munDesv = v; });
        lateral("dedos.indice", "INDICE CIERRE", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return detalle::cierreDedo(p, l, 0); }, [](Pose& p, int l, float v) { detalle::cerrarDedo(p, l, 0, v); });
        lateral("dedos.medio", "MEDIO CIERRE", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return detalle::cierreDedo(p, l, 1); }, [](Pose& p, int l, float v) { detalle::cerrarDedo(p, l, 1, v); });
        lateral("dedos.anular", "ANULAR CIERRE", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return detalle::cierreDedo(p, l, 2); }, [](Pose& p, int l, float v) { detalle::cerrarDedo(p, l, 2, v); });
        lateral("dedos.menique", "MENIQUE CIERRE", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return detalle::cierreDedo(p, l, 3); }, [](Pose& p, int l, float v) { detalle::cerrarDedo(p, l, 3, v); });
        lateral("dedos.pulgar", "PULGAR CIERRE", FRACCION, 0.0f, 1.0f,
                [](const Pose& p, int l) { return detalle::cierreDedo(p, l, 4); }, [](Pose& p, int l, float v) { detalle::cerrarDedo(p, l, 4, v); });
        lateral("dedos.abrir", "DEDOS SEPARAR", FRACCION, -0.4f, 1.0f,
                [](const Pose& p, int l) { return p.brazo[l].abre[0] / DEDO_ABRE_MAX; },
                [](Pose& p, int l, float v) { for (float& a : p.brazo[l].abre) a = v * DEDO_ABRE_MAX; });
        lateral("pierna.flex", "CADERA FLEXION", GRADOS, CADERA_FLEX_MIN * G, CADERA_FLEX_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].flex; }, [](Pose& p, int l, float v) { p.pierna[l].flex = v; });
        lateral("pierna.abd", "CADERA ABDUCCION", GRADOS, CADERA_ABD_MIN * G, CADERA_ABD_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].abd; }, [](Pose& p, int l, float v) { p.pierna[l].abd = v; });
        lateral("pierna.rot", "CADERA ROTACION", GRADOS, CADERA_ROT_MIN * G, CADERA_ROT_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].rot; }, [](Pose& p, int l, float v) { p.pierna[l].rot = v; });
        lateral("rodilla", "RODILLA", GRADOS, 0.0f, RODILLA_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].rodilla; }, [](Pose& p, int l, float v) { p.pierna[l].rodilla = v; });
        lateral("tobillo.flex", "TOBILLO FLEXION", GRADOS, TOBILLO_MIN * G, TOBILLO_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].tobillo; }, [](Pose& p, int l, float v) { p.pierna[l].tobillo = v; });
        lateral("tobillo.lado", "TOBILLO INVERSION", GRADOS, TOBILLO_LADO_MIN * G, TOBILLO_LADO_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].tobilloLado; }, [](Pose& p, int l, float v) { p.pierna[l].tobilloLado = v; });
        lateral("dedospie", "DEDOS PIE", GRADOS, DEDOS_PIE_MIN * G, DEDOS_PIE_MAX * G,
                [](const Pose& p, int l) { return p.pierna[l].dedos; }, [](Pose& p, int l, float v) { p.pierna[l].dedos = v; });
    }
    return C;
}

inline int buscarCanal(const std::string& nombre) {
    const auto& C = canales();
    for (size_t i = 0; i < C.size(); ++i)
        if (C[i].nombre == nombre) return (int)i;
    return -1;
}

// Valor del canal en su unidad (grados o fraccion) y vuelta.
inline float aUnidad(const Canal& c, float interno) { return c.unidad == GRADOS ? interno / GRAD : interno; }
inline float aInterno(const Canal& c, float v) { return c.unidad == GRADOS ? v * GRAD : v; }
inline float leer(const Pose& p, int canal) {
    const Canal& c = canales()[canal];
    return aUnidad(c, c.leer(p, c.lado));
}
inline void poner(Pose& p, int canal, float v) {
    const Canal& c = canales()[canal];
    c.poner(p, c.lado, aInterno(c, v));
}

// ----------------------------------------------------------------------------
// LAS ANIMACIONES
// ----------------------------------------------------------------------------
enum Curva : uint8_t { NATURAL, SUAVE, LINEAL };

struct Valor { int canal; float v; };   // v en la unidad del canal
struct Clave {
    float t;
    std::vector<Valor> valores;
};
struct Animacion {
    std::string nombre;
    std::string origen;     // "" = del juego; si no, el archivo
    float duracion = 1.0f;
    bool bucle = false;
    Curva curva = NATURAL;
    std::vector<Clave> claves;   // ordenadas por t
};

namespace detalle {
inline std::string mayusculas(std::string s) {
    for (char& c : s) if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    return s;
}
inline std::string minusculas(std::string s) {
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return s;
}
inline std::vector<std::string> palabras(const std::string& l) {
    std::vector<std::string> w;
    std::string cur;
    for (char c : l) {
        if (c == ' ' || c == '\t' || c == ',' || c == '\r') { if (!cur.empty()) w.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) w.push_back(cur);
    return w;
}
} // namespace detalle

// Lee una animacion del texto. Lo que no entiende lo anota en `errores` (una
// linea por problema) y sigue: un canal mal escrito no tira la animacion.
inline bool leerAnimacion(const std::string& texto, Animacion& a, std::string* errores = nullptr) {
    a = Animacion{};
    auto error = [&](int linea, const std::string& m) {
        if (errores) *errores += "LINEA " + std::to_string(linea) + ": " + m + "\n";
    };
    size_t i = 0;
    int nLinea = 0;
    while (i < texto.size()) {
        size_t f = texto.find('\n', i);
        if (f == std::string::npos) f = texto.size();
        std::string l = texto.substr(i, f - i);
        i = f + 1;
        ++nLinea;
        const size_t hash = l.find('#');
        if (hash != std::string::npos) l = l.substr(0, hash);
        const std::vector<std::string> w = detalle::palabras(l);
        if (w.empty()) continue;
        const std::string k = detalle::minusculas(w[0]);
        if (k == "nombre") {
            std::string n;
            for (size_t j = 1; j < w.size(); ++j) n += (j > 1 ? " " : "") + w[j];
            a.nombre = detalle::mayusculas(n);
        } else if (k == "duracion" && w.size() > 1) {
            a.duracion = (float)std::atof(w[1].c_str());
        } else if (k == "bucle" && w.size() > 1) {
            a.bucle = detalle::minusculas(w[1]) == "si";
        } else if (k == "curva" && w.size() > 1) {
            const std::string c = detalle::minusculas(w[1]);
            a.curva = c == "lineal" ? LINEAL : (c == "suave" ? SUAVE : NATURAL);
        } else if ((k[0] >= '0' && k[0] <= '9') || k[0] == '.') {
            Clave c;
            c.t = (float)std::atof(k.c_str());
            for (size_t j = 1; j < w.size(); ++j) {
                const size_t igual = w[j].find('=');
                if (igual == std::string::npos) { error(nLinea, "FALTA = EN " + w[j]); continue; }
                std::string nombre = detalle::minusculas(w[j].substr(0, igual));
                const float v = (float)std::atof(w[j].c_str() + igual + 1);
                // ".a." = los dos lados a la vez.
                const size_t ambos = nombre.find(".a");
                const bool esAmbos = ambos != std::string::npos &&
                                     (ambos + 2 == nombre.size() || nombre[ambos + 2] == '.');
                std::vector<std::string> nombres;
                if (esAmbos) {
                    nombres.push_back(nombre.substr(0, ambos) + ".i" + nombre.substr(ambos + 2));
                    nombres.push_back(nombre.substr(0, ambos) + ".d" + nombre.substr(ambos + 2));
                } else {
                    nombres.push_back(nombre);
                }
                for (const std::string& n : nombres) {
                    const int canal = buscarCanal(n);
                    if (canal < 0) { error(nLinea, "CANAL DESCONOCIDO " + n); continue; }
                    c.valores.push_back({ canal, v });
                }
            }
            a.claves.push_back(c);
        } else {
            error(nLinea, "NO ENTIENDO " + w[0]);
        }
    }
    if (a.duracion < 0.05f) a.duracion = 0.05f;
    // Ordenadas por tiempo (insercion: son pocas).
    for (size_t x = 1; x < a.claves.size(); ++x)
        for (size_t y = x; y > 0 && a.claves[y].t < a.claves[y - 1].t; --y) std::swap(a.claves[y], a.claves[y - 1]);
    return !a.nombre.empty() && !a.claves.empty();
}

// El texto de una animacion (para guardarla tal cual se ve).
inline std::string textoDe(const Animacion& a) {
    std::string s;
    char l[96];
    s += "nombre   " + a.nombre + "\n";
    std::snprintf(l, sizeof(l), "duracion %.2f\n", a.duracion); s += l;
    s += std::string("bucle    ") + (a.bucle ? "si" : "no") + "\n";
    s += std::string("curva    ") + (a.curva == LINEAL ? "lineal" : (a.curva == SUAVE ? "suave" : "natural")) + "\n";
    for (const Clave& c : a.claves) {
        std::snprintf(l, sizeof(l), "%.2f", c.t); s += l;
        for (const Valor& v : c.valores) {
            std::snprintf(l, sizeof(l), "  %s=%g", canales()[v.canal].nombre.c_str(), std::round(v.v * 10.0f) / 10.0f);
            s += l;
        }
        s += "\n";
    }
    return s;
}

namespace detalle {
// Catmull-Rom entre b y c, con a antes y d despues.
inline float catmull(float a, float b, float c, float d, float u) {
    const float u2 = u * u, u3 = u2 * u;
    return 0.5f * ((2.0f * b) + (-a + c) * u + (2.0f * a - 5.0f * b + 4.0f * c - d) * u2 + (-a + 3.0f * b - 3.0f * c + d) * u3);
}
} // namespace detalle

// Pone la animacion en el segundo `t` sobre la pose `p` (la del juego), con
// peso 0..1. Los canales que la animacion no toca quedan como estan.
inline void aplicar(const Animacion& a, float t, Pose& p, float peso = 1.0f) {
    if (a.claves.empty() || peso <= 0.0f) return;
    if (a.bucle) { t = std::fmod(t, a.duracion); if (t < 0.0f) t += a.duracion; }
    const size_t nC = canales().size();
    std::vector<char> usado(nC, 0);
    for (const Clave& c : a.claves) for (const Valor& v : c.valores) usado[v.canal] = 1;
    struct Punto { float t, v; };
    std::vector<Punto> pts;
    const Pose base = p;
    for (size_t canal = 0; canal < nC; ++canal) {
        if (!usado[canal]) continue;
        const float vBase = leer(base, (int)canal);
        pts.clear();
        for (const Clave& c : a.claves)
            for (const Valor& v : c.valores)
                if (v.canal == (int)canal) pts.push_back({ c.t, v.v });
        // Sin bucle: entra desde la pose del juego y vuelve a ella al final.
        if (!a.bucle) {
            if (pts.front().t > 0.0f) pts.insert(pts.begin(), { 0.0f, vBase });
            if (pts.back().t < a.duracion) pts.push_back({ a.duracion, vBase });
        } else {
            // En bucle, el final enlaza con el principio.
            const Punto primero = pts.front(), ultimo = pts.back();
            pts.insert(pts.begin(), { ultimo.t - a.duracion, ultimo.v });
            pts.push_back({ primero.t + a.duracion, primero.v });
        }
        float v;
        if (t <= pts.front().t) v = pts.front().v;
        else if (t >= pts.back().t) v = pts.back().v;
        else {
            size_t k = 1;
            while (k < pts.size() && pts[k].t < t) ++k;
            const Punto& b = pts[k - 1];
            const Punto& c = pts[k];
            float u = (c.t - b.t) > 1e-5f ? (t - b.t) / (c.t - b.t) : 1.0f;
            if (a.curva == LINEAL) v = b.v + (c.v - b.v) * u;
            else if (a.curva == SUAVE) { u = u * u * (3.0f - 2.0f * u); v = b.v + (c.v - b.v) * u; }
            else {
                // Los vecinos de fuera se reflejan en los bordes: asi la
                // curva arranca y llega quieta a la pose del juego.
                const float va = k >= 2 ? pts[k - 2].v : b.v;
                const float vd = k + 1 < pts.size() ? pts[k + 1].v : c.v;
                v = detalle::catmull(va, b.v, c.v, vd, u);
            }
        }
        poner(p, (int)canal, vBase + (v - vBase) * peso);
    }
    ModeloJugador::limitar(p);
}

// ----------------------------------------------------------------------------
// LAS QUE TRAE EL JUEGO
// ----------------------------------------------------------------------------
// Hechas con los tiempos de una persona: un saludo de 3 vaivenes a ~2 por
// segundo, un estiramiento que se sostiene, una sentadilla que baja en ~1 s
// con la cadera atras y el tronco inclinado para no perder el equilibrio...
inline const char* TEXTO_INCLUIDAS = R"(
nombre SALUDAR
duracion 2.6
# El brazo arriba con el hombro en rotacion externa: el antebrazo queda de
# pie y el vaiven lo hace el codo, en el plano del cuerpo.
0.40  brazo.d.abd=92 brazo.d.flex=12 brazo.d.rot=62 codo.d=92 antebrazo.d.prono=-20 muneca.d.flex=-10 dedos.d.abrir=0.6 cuello.lado=-6 hombro.d.encoger=0.15
0.65  codo.d=65
0.90  codo.d=110
1.15  codo.d=65
1.40  codo.d=110
1.65  codo.d=65
1.90  codo.d=92 brazo.d.abd=92 brazo.d.flex=12 brazo.d.rot=62 antebrazo.d.prono=-20
2.60

nombre ESTIRARSE
duracion 3.6
0.90  brazo.a.flex=170 brazo.a.abd=15 codo.a=5 hombro.a.encoger=0.8 torso.inclina=-12 cuello.flex=-25 dedos.a.abrir=1 tobillo.a.flex=-25
2.30  brazo.a.flex=172 brazo.a.abd=18 codo.a=0 hombro.a.encoger=0.9 torso.inclina=-14 cuello.flex=-30 tobillo.a.flex=-30 torso.lado=4
2.70  torso.lado=-4
3.60

nombre ENCOGER HOMBROS
duracion 1.8
0.40  hombro.a.encoger=1 codo.a=80 antebrazo.a.prono=-75 brazo.a.abd=20 dedos.a.abrir=0.7 cuello.lado=10 cuello.flex=-5
1.10  hombro.a.encoger=1 codo.a=82 antebrazo.a.prono=-78
1.80

nombre MIRAR ALREDEDOR
duracion 5.0
curva natural
1.0   cuello.giro=-65 torso.giro=-18 pelvis.giro=-4
1.8   cuello.giro=-65 torso.giro=-18
2.9   cuello.giro=65 torso.giro=18 pelvis.giro=4
3.7   cuello.giro=65 torso.giro=18
5.0

nombre SENTADILLA
duracion 3.0
# Hasta muslos paralelos al suelo: la pelvis se inclina ~30 grados, el
# tronco la acompana y los brazos van delante como contrapeso.
1.10  pierna.a.flex=78 pelvis.inclina=30 rodilla.a=100 tobillo.a.flex=26 torso.inclina=8 brazo.a.flex=85 codo.a=10 pierna.a.abd=10 pierna.a.rot=15 cuello.flex=-15
1.60  pierna.a.flex=80 pelvis.inclina=31 rodilla.a=102 tobillo.a.flex=27 brazo.a.flex=88
3.00

nombre TOCARSE LOS PIES
duracion 4.2
# Las piernas quedan de pie y se dobla la pelvis sobre los femures; con las
# rodillas rectas los isquiotibiales la frenan hacia los 80 grados y el
# resto lo pone la columna. Con flexibilidad baja no se llega.
1.40  pelvis.inclina=85 torso.inclina=50 rodilla.a=0 brazo.a.flex=135 codo.a=5 cuello.flex=10 muneca.a.flex=-10
2.80  pelvis.inclina=88 torso.inclina=55 brazo.a.flex=140
4.20

nombre SENALAR
duracion 2.4
0.45  brazo.d.flex=88 brazo.d.abd=8 codo.d=4 antebrazo.d.prono=60 dedos.d.indice=0 dedos.d.medio=0.95 dedos.d.anular=1 dedos.d.menique=1 dedos.d.pulgar=0.7 torso.giro=8 cuello.giro=6
1.80  brazo.d.flex=90
2.40

nombre ESTIRAR CUELLO
duracion 6.0
1.2   cuello.lado=35 hombro.i.encoger=0
2.4   cuello.lado=0 cuello.flex=40
3.6   cuello.lado=-35
4.8   cuello.lado=0 cuello.flex=-30
6.0

nombre EQUILIBRIO
duracion 4.0
0.9   pierna.i.flex=90 rodilla.i=95 tobillo.i.flex=-20 brazo.a.abd=80 codo.a=10 pelvis.lado=-3 torso.lado=-3 tobillo.d.lado=5
2.0   brazo.i.abd=72 brazo.d.abd=88 tobillo.d.lado=-4
3.0   brazo.i.abd=86 brazo.d.abd=74 tobillo.d.lado=6
4.0

nombre RESPIRAR HONDO
duracion 5.0
curva suave
2.0   hombro.a.encoger=0.45 hombro.a.adelantar=-0.6 torso.inclina=-6 cuello.flex=-8 antebrazo.a.prono=-25
2.6   hombro.a.encoger=0.5 hombro.a.adelantar=-0.7
5.0
)";

// Parte un texto con varias animaciones (cada una empieza en "nombre").
inline std::vector<Animacion> leerVarias(const std::string& texto, const std::string& origen) {
    std::vector<Animacion> v;
    std::string actual;
    auto cerrar = [&]() {
        Animacion a;
        if (!actual.empty() && leerAnimacion(actual, a)) { a.origen = origen; v.push_back(a); }
        actual.clear();
    };
    size_t i = 0;
    while (i < texto.size()) {
        size_t f = texto.find('\n', i);
        if (f == std::string::npos) f = texto.size();
        const std::string l = texto.substr(i, f - i);
        i = f + 1;
        const std::vector<std::string> w = detalle::palabras(l);
        if (!w.empty() && detalle::minusculas(w[0]) == "nombre") cerrar();
        actual += l + "\n";
    }
    cerrar();
    return v;
}

inline const std::vector<Animacion>& incluidas() {
    static const std::vector<Animacion> v = leerVarias(TEXTO_INCLUIDAS, "");
    return v;
}

} // namespace Movimiento
