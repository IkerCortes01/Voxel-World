#pragma once

#include <cstdio>
#include <fstream>
#include <string>

// ============================================================================
// LAS OPCIONES DEL MENU DE PAUSA
// ============================================================================
// Dieciseis, en cuatro grupos. Aqui vive lo que son, que valores admiten, como
// se escriben en pantalla y como se guardan. Lo que HACE cada una lo aplica el
// motor (ver aplicarOpciones en main.cpp).
//
// Sin OpenGL ni mundo, para que los tests puedan comprobarlas.
// ============================================================================

namespace Opciones {

enum Id {
    // Imagen
    BRILLO, OSCURIDAD, DIFUMINADO, VSYNC, SUAVIDAD_LUZ,
    // Render
    VELOCIDAD_RENDER, FLUIDEZ_ANIMALES, DISTANCIA, RAPIDEZ_DISTANCIA, DETALLE,
    // Rendimiento
    FPS_MINIMO, OPTIMIZACION, CARGA_CHUNKS,
    // Atmosfera
    ATMOSFERA, INTENSIDAD_LUZ, SATURACION,
    NUM
};

enum Grupo { IMAGEN, RENDER, RENDIMIENTO, ATMOSFERA_G, NUM_GRUPOS };

// PORCENTAJE y NUMERO se ajustan tambien arrastrando; LISTA va de etiqueta en
// etiqueta.
enum Tipo { PORCENTAJE, NUMERO, LISTA };

struct Def {
    const char* clave;      // en opciones.txt
    const char* nombre;     // en pantalla (la fuente solo tiene A-Z, 0-9, + - %)
    Grupo grupo;
    Tipo tipo;
    int min, max, paso, porDefecto;
    const char* const* etiquetas;   // solo LISTA: una por valor, de min a max
};

inline const char* const NO_SI[]       = { "NO", "SI" };
inline const char* const DIFUMINADO_E[] = { "SIN NIEBLA", "BAJO", "NORMAL", "ALTO" };
inline const char* const SUAVIDAD_E[]  = { "BAJA", "MEDIA", "ALTA", "ULTRA" };
inline const char* const LIMITE_E[]    = { "60", "90", "120", "144", "240", "SIN LIMITE" };
inline const char* const FLUIDEZ_E[]   = { "20 HZ", "30 HZ", "60 HZ", "MAXIMA" };
inline const char* const VELOCIDAD_E[] = { "BAJA", "NORMAL", "ALTA" };
inline const char* const DETALLE_E[]   = { "BAJO", "MEDIO", "ALTO" };
inline const char* const FPS_MIN_E[]   = { "30", "45", "60", "90", "120" };

inline const Def& def(int id) {
    static const Def D[NUM] = {
        { "brillo",            "BRILLO",               IMAGEN,      PORCENTAJE, 50, 150, 10, 100, nullptr },
        { "oscuridad",         "OSCURIDAD",            IMAGEN,      PORCENTAJE,  0, 100, 10,  50, nullptr },
        { "difuminado",        "DIFUMINADO",           IMAGEN,      LISTA,       0,   3,  1,   2, DIFUMINADO_E },
        { "vsync",             "VSYNC",                IMAGEN,      LISTA,       0,   1,  1,   0, NO_SI },
        { "suavidad_luz",      "SUAVIDAD DE LUZ",      IMAGEN,      LISTA,       0,   3,  1,   2, SUAVIDAD_E },
        { "velocidad_render",  "LIMITE DE FPS",        RENDER,      LISTA,       0,   5,  1,   5, LIMITE_E },
        { "fluidez_animales",  "FLUIDEZ DE ANIMALES",  RENDER,      LISTA,       0,   3,  1,   3, FLUIDEZ_E },
        { "distancia",         "DISTANCIA DE RENDER",  RENDER,      NUMERO,      2, 100,  1,   8, nullptr },
        { "rapidez_distancia", "RAPIDEZ A DISTANCIA",  RENDER,      LISTA,       0,   2,  1,   1, VELOCIDAD_E },
        { "detalle",           "DETALLE",              RENDER,      LISTA,       0,   2,  1,   2, DETALLE_E },
        { "fps_minimo",        "FPS MINIMO",           RENDIMIENTO, LISTA,       0,   4,  1,   2, FPS_MIN_E },
        { "optimizacion",      "OPTIMIZACION POTENTE", RENDIMIENTO, LISTA,       0,   1,  1,   1, NO_SI },
        { "carga_chunks",      "CARGA DE CHUNKS",      RENDIMIENTO, LISTA,       0,   2,  1,   1, VELOCIDAD_E },
        { "atmosfera",         "MEJORA ATMOSFERICA",   ATMOSFERA_G, PORCENTAJE,  0, 100, 10, 100, nullptr },
        { "intensidad_luz",    "INTENSIDAD DE LUZ",    ATMOSFERA_G, PORCENTAJE, 50, 150, 10, 100, nullptr },
        { "saturacion",        "SATURACION",           ATMOSFERA_G, PORCENTAJE,  0, 200, 10, 100, nullptr },
    };
    return D[id];
}

inline const char* nombreGrupo(int g) {
    static const char* const G[NUM_GRUPOS] = { "IMAGEN", "RENDER", "RENDIMIENTO", "ATMOSFERA" };
    return G[g];
}

inline int acotar(int id, int v) {
    const Def& d = def(id);
    return v < d.min ? d.min : (v > d.max ? d.max : v);
}

struct Valores {
    int v[NUM];
    Valores() { for (int i = 0; i < NUM; ++i) v[i] = def(i).porDefecto; }
    int operator[](int id) const { return v[id]; }
    void poner(int id, int valor) { v[id] = acotar(id, valor); }

    // Las flechas: un paso, y las listas dan la vuelta.
    void siguiente(int id) {
        const Def& d = def(id);
        if (d.tipo == LISTA) v[id] = v[id] >= d.max ? d.min : v[id] + 1;
        else poner(id, v[id] + d.paso);
    }
    void anterior(int id) {
        const Def& d = def(id);
        if (d.tipo == LISTA) v[id] = v[id] <= d.min ? d.max : v[id] - 1;
        else poner(id, v[id] - d.paso);
    }
    // Arrastrar: t de 0 a 1 sobre la barra, redondeado al paso.
    void ponerFraccion(int id, float t) {
        const Def& d = def(id);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        const int pasos = (d.max - d.min) / d.paso;
        poner(id, d.min + (int)(t * pasos + 0.5f) * d.paso);
    }
    float fraccion(int id) const {
        const Def& d = def(id);
        return d.max == d.min ? 0.0f : (float)(v[id] - d.min) / (float)(d.max - d.min);
    }

    std::string texto(int id) const {
        const Def& d = def(id);
        if (d.tipo == LISTA) return d.etiquetas[v[id] - d.min];
        if (d.tipo == PORCENTAJE) return std::to_string(v[id]) + "%";
        return std::to_string(v[id]);
    }
};

// --- Lo que significa cada valor, para el motor ---

inline int limiteFps(const Valores& o) {           // 0 = sin limite
    static const int L[] = { 60, 90, 120, 144, 240, 0 };
    return L[o[VELOCIDAD_RENDER]];
}
inline int hzAnimales(const Valores& o) {          // 0 = cada frame
    static const int H[] = { 20, 30, 60, 0 };
    return H[o[FLUIDEZ_ANIMALES]];
}
inline int fpsMinimo(const Valores& o) {
    static const int F[] = { 30, 45, 60, 90, 120 };
    return F[o[FPS_MINIMO]];
}
inline int pasosDeLuz(const Valores& o) {          // escalones del dia a la noche
    static const int P[] = { 8, 16, 32, 64 };
    return P[o[SUAVIDAD_LUZ]];
}
inline float factorVelocidad(int nivel) {          // BAJA, NORMAL, ALTA
    static const float F[] = { 0.5f, 1.0f, 2.0f };
    return F[nivel];
}
inline float radioAnimalesChunks(const Valores& o) {
    static const float R[] = { 3.0f, 5.0f, 8.0f };
    return R[o[DETALLE]];
}

// La luz del shader del terreno.
struct LuzShader {
    float brillo, luzMinima, gamma, intensidad, saturacion;
};
inline LuzShader luzShader(const Valores& o) {
    LuzShader l;
    l.brillo = o[BRILLO] / 100.0f;
    l.intensidad = o[INTENSIDAD_LUZ] / 100.0f;
    l.saturacion = o[SATURACION] / 100.0f;
    // OSCURIDAD 50 deja la luz como esta. Por debajo levanta las sombras (un
    // suelo de luz de hasta 0,3); por encima las hunde (gamma hasta 1,8).
    const int osc = o[OSCURIDAD];
    l.luzMinima = osc < 50 ? (50 - osc) / 50.0f * 0.30f : 0.0f;
    l.gamma = osc > 50 ? 1.0f + (osc - 50) / 50.0f * 0.8f : 1.0f;
    return l;
}

// Donde empieza la niebla, en fraccion del radio. -1 = sin niebla, 0 = el de la
// barra de distancia, sin tocar.
inline float inicioNiebla(const Valores& o) {
    switch (o[DIFUMINADO]) {
        case 0:  return -1.0f;
        case 1:  return 0.85f;
        case 3:  return 0.45f;
        default: return 0.0f;
    }
}

// --- Guardar y cargar: clave=valor, una por linea ---
// La distancia no va aqui: se guarda con cada mundo, como siempre.

inline bool guardar(const Valores& o, const std::string& ruta) {
    std::ofstream f(ruta, std::ios::trunc);
    if (!f) return false;
    for (int i = 0; i < NUM; ++i)
        if (i != DISTANCIA) f << def(i).clave << "=" << o[i] << "\n";
    return (bool)f;
}

// Lo que falte o este mal se queda con su valor por defecto.
inline bool cargar(Valores& o, const std::string& ruta) {
    std::ifstream f(ruta);
    if (!f) return false;
    std::string linea;
    while (std::getline(f, linea)) {
        const size_t igual = linea.find('=');
        if (igual == std::string::npos) continue;
        const std::string clave = linea.substr(0, igual);
        int valor = 0;
        if (std::sscanf(linea.c_str() + igual + 1, "%d", &valor) != 1) continue;
        for (int i = 0; i < NUM; ++i)
            if (i != DISTANCIA && clave == def(i).clave) o.poner(i, valor);
    }
    return true;
}

} // namespace Opciones
