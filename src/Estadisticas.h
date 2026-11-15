#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>

// ============================================================================
// LAS ESTADISTICAS DEL JUGADOR
// ============================================================================
// PEDIDO: "Estadisticas, osea, guarda no solo la cantidad que corriste,
// caminaste, bloques que minaste o colocaste, cuanto dano has hecho a
// animales, cuantos arboles talaste etc, quiero que de esos inventes 90 mas de
// estadisticas que crecen con el tiempo".
//
// Cada mundo lleva las suyas (saves/<mundo>/estadisticas.txt, una por linea:
// "clave valor"). Se guardan por CLAVE, no por posicion: anadir o reordenar la
// lista no estropea las de un mundo viejo, y una clave desconocida se ignora.
//
// Hay cuatro clases, y cada una se escribe distinto en la pantalla:
//   CUENTA    veces que paso algo (saltos, bloques minados...)
//   METROS    distancia recorrida
//   SEGUNDOS  tiempo (se muestra en h/min/s)
//   RECORD    el maximo alcanzado (altura, caida, velocidad): no se suma, se
//             queda con el mayor
//   MINIMO    el menor (la altura mas baja)
//
// Esta cabecera no sabe nada del motor: main.cpp mira lo que pasa y llama a
// sumar/record. Asi la lista y el guardado se pueden probar solos.
namespace Estadisticas {

enum class Clase { CUENTA, METROS, SEGUNDOS, RECORD, MINIMO, DANO, HORAS };

struct Info {
    const char* clave;     // la que se guarda: no cambiarla nunca
    const char* nombre;    // la que se ve (solo A-Z, 0-9, espacio, - y +)
    Clase clase;
};

// ⚠️ El ORDEN se puede cambiar; la CLAVE no.
#define VW_ESTADISTICAS(X) \
    /* --- Las que se pidieron --- */ \
    X(CAMINADO,            "caminado",            "DISTANCIA CAMINADA",              METROS) \
    X(CORRIDO,             "corrido",             "DISTANCIA CORRIDA",               METROS) \
    X(BLOQUES_MINADOS,     "minados",             "BLOQUES MINADOS",                 CUENTA) \
    X(BLOQUES_COLOCADOS,   "colocados",           "BLOQUES COLOCADOS",               CUENTA) \
    X(DANO_ANIMALES,       "dano_animales",       "DANO HECHO A ANIMALES",           DANO) \
    X(ARBOLES_TALADOS,     "arboles",             "ARBOLES TALADOS",                 CUENTA) \
    /* --- Distancias --- */ \
    X(AGACHADO,            "agachado",            "DISTANCIA AGACHADO",              METROS) \
    X(ARRASTRADO,          "arrastrado",          "DISTANCIA ARRASTRANDOSE",         METROS) \
    X(NADADO,              "nadado",              "DISTANCIA NADADA",                METROS) \
    X(VOLADO,              "volado",              "DISTANCIA VOLADA",                METROS) \
    X(CAIDO,               "caido",               "METROS CAIDOS",                   METROS) \
    X(SUBIDO,              "subido",              "METROS SUBIDOS",                  METROS) \
    X(RECORRIDO_TOTAL,     "recorrido",           "DISTANCIA TOTAL",                 METROS) \
    X(RECORRIDO_NOCHE,     "recorrido_noche",     "DISTANCIA DE NOCHE",              METROS) \
    X(RECORRIDO_LLUVIA,    "recorrido_lluvia",    "DISTANCIA BAJO LA LLUVIA",        METROS) \
    X(RECORRIDO_CUEVA,     "recorrido_cueva",     "DISTANCIA BAJO TIERRA",           METROS) \
    X(RECORRIDO_ALTURA,    "recorrido_altura",    "DISTANCIA EN LAS ALTURAS",        METROS) \
    X(RECORRIDO_CARGADO,   "recorrido_cargado",   "DISTANCIA CON HERRAMIENTA",       METROS) \
    X(RECORRIDO_FRIO,      "recorrido_frio",      "DISTANCIA CON FRIO",              METROS) \
    X(RECORRIDO_CALOR,     "recorrido_calor",     "DISTANCIA CON CALOR",             METROS) \
    /* --- Tiempos --- */ \
    X(TIEMPO_JUGADO,       "t_jugado",            "TIEMPO JUGADO",                   SEGUNDOS) \
    X(TIEMPO_DIA,          "t_dia",               "TIEMPO DE DIA",                   SEGUNDOS) \
    X(TIEMPO_NOCHE,        "t_noche",             "TIEMPO DE NOCHE",                 SEGUNDOS) \
    X(TIEMPO_LLUVIA,       "t_lluvia",            "TIEMPO BAJO LA LLUVIA",           SEGUNDOS) \
    X(TIEMPO_AGUA,         "t_agua",              "TIEMPO EN EL AGUA",               SEGUNDOS) \
    X(TIEMPO_BUCEANDO,     "t_buceando",          "TIEMPO BAJO EL AGUA",             SEGUNDOS) \
    X(TIEMPO_AGACHADO,     "t_agachado",          "TIEMPO AGACHADO",                 SEGUNDOS) \
    X(TIEMPO_TUMBADO,      "t_tumbado",           "TIEMPO TUMBADO",                  SEGUNDOS) \
    X(TIEMPO_QUIETO,       "t_quieto",            "TIEMPO QUIETO",                   SEGUNDOS) \
    X(TIEMPO_AIRE,         "t_aire",              "TIEMPO EN EL AIRE",               SEGUNDOS) \
    X(TIEMPO_VOLANDO,      "t_volando",           "TIEMPO VOLANDO",                  SEGUNDOS) \
    X(TIEMPO_CORRIENDO,    "t_corriendo",         "TIEMPO CORRIENDO",                SEGUNDOS) \
    X(TIEMPO_CAMINANDO,    "t_caminando",         "TIEMPO CAMINANDO",                SEGUNDOS) \
    X(TIEMPO_DURMIENDO,    "t_durmiendo",         "TIEMPO DURMIENDO",                SEGUNDOS) \
    X(TIEMPO_PAUSA,        "t_pausa",             "TIEMPO EN PAUSA",                 SEGUNDOS) \
    X(TIEMPO_INVENTARIO,   "t_inventario",        "TIEMPO EN EL INVENTARIO",         SEGUNDOS) \
    X(TIEMPO_CUEVA,        "t_cueva",             "TIEMPO BAJO TIERRA",              SEGUNDOS) \
    X(TIEMPO_ALTURA,       "t_altura",            "TIEMPO EN LAS ALTURAS",           SEGUNDOS) \
    X(TIEMPO_MINANDO,      "t_minando",           "TIEMPO MINANDO",                  SEGUNDOS) \
    X(TIEMPO_HERRAMIENTA,  "t_herramienta",       "TIEMPO CON HERRAMIENTA",          SEGUNDOS) \
    X(TIEMPO_MANOS_VACIAS, "t_manos_vacias",      "TIEMPO CON LAS MANOS VACIAS",     SEGUNDOS) \
    X(TIEMPO_MUSICA,       "t_musica",            "TIEMPO CON MUSICA",               SEGUNDOS) \
    X(TIEMPO_PULSO_ALTO,   "t_pulso_alto",        "TIEMPO CON EL PULSO ALTO",        SEGUNDOS) \
    X(TIEMPO_CERCA_ANIMAL, "t_cerca_animal",      "TIEMPO CERCA DE ANIMALES",        SEGUNDOS) \
    X(TIEMPO_AMANECER,     "t_amanecer",          "AMANECERES VISTOS EN SEGUNDOS",   SEGUNDOS) \
    X(TIEMPO_ATARDECER,    "t_atardecer",         "ATARDECERES VISTOS EN SEGUNDOS",  SEGUNDOS) \
    /* --- La temperatura del cuerpo: un tiempo por cada uno de los 11 niveles --- */ \
    X(TEMP_DEMASIADO_FRIO, "t_temp_m5",           "TIEMPO CON DEMASIADO FRIO",       SEGUNDOS) \
    X(TEMP_MUCHO_FRIO,     "t_temp_m4",           "TIEMPO CON MUCHO FRIO",           SEGUNDOS) \
    X(TEMP_FRIO,           "t_temp_m3",           "TIEMPO CON FRIO",                 SEGUNDOS) \
    X(TEMP_POCO_FRIO,      "t_temp_m2",           "TIEMPO CON POCO FRIO",            SEGUNDOS) \
    X(TEMP_LIGERO_FRIO,    "t_temp_m1",           "TIEMPO CON LIGERO FRIO",          SEGUNDOS) \
    X(TEMP_TEMPLADO,       "t_temp_0",            "TIEMPO TEMPLADO",                 SEGUNDOS) \
    X(TEMP_LIGERO_CALOR,   "t_temp_p1",           "TIEMPO CON LIGERO CALOR",         SEGUNDOS) \
    X(TEMP_POCO_CALOR,     "t_temp_p2",           "TIEMPO CON POCO CALOR",           SEGUNDOS) \
    X(TEMP_CALOR,          "t_temp_p3",           "TIEMPO CON CALOR",                SEGUNDOS) \
    X(TEMP_MUCHO_CALOR,    "t_temp_p4",           "TIEMPO CON MUCHO CALOR",          SEGUNDOS) \
    X(TEMP_DEMASIADO_CALOR,"t_temp_p5",           "TIEMPO CON DEMASIADO CALOR",      SEGUNDOS) \
    /* --- El cuerpo --- */ \
    X(LATIDOS,             "latidos",             "LATIDOS DEL CORAZON",             CUENTA) \
    X(RESPIRACIONES,       "respiraciones",       "RESPIRACIONES",                   CUENTA) \
    X(PASOS,               "pasos",               "PASOS DADOS",                     CUENTA) \
    X(SALTOS,              "saltos",              "SALTOS",                          CUENTA) \
    X(ATERRIZAJES,         "aterrizajes",         "ATERRIZAJES",                     CUENTA) \
    X(CAIDAS_FUERTES,      "caidas_fuertes",      "CAIDAS DE MAS DE 4 METROS",       CUENTA) \
    X(VECES_AGACHADO,      "veces_agachado",      "VECES QUE TE AGACHASTE",          CUENTA) \
    X(VECES_TUMBADO,       "veces_tumbado",       "VECES QUE TE TUMBASTE",           CUENTA) \
    X(ZAMBULLIDAS,         "zambullidas",         "ZAMBULLIDAS",                     CUENTA) \
    X(VECES_DORMIDO,       "veces_dormido",       "NOCHES DORMIDAS",                 CUENTA) \
    X(SIESTAS,             "siestas",             "SIESTAS",                         CUENTA) \
    X(HORAS_DORMIDAS,      "horas_dormidas",      "HORAS DEL MUNDO DORMIDAS",        HORAS) \
    X(DIAS_VIVIDOS,        "dias",                "DIAS VIVIDOS",                    CUENTA) \
    X(GESTOS,              "gestos",              "GESTOS HECHOS",                   CUENTA) \
    /* --- Bloques y herramientas --- */ \
    X(MINADOS_TIERRA,      "minados_tierra",      "TIERRA Y ARENA MINADA",           CUENTA) \
    X(MINADOS_MADERA,      "minados_madera",      "MADERA MINADA",                   CUENTA) \
    X(MINADOS_PIEDRA,      "minados_piedra",      "PIEDRA MINADA",                   CUENTA) \
    X(MINADOS_CONSTRUIDO,  "minados_construido",  "BLOQUES CONSTRUIDOS ROTOS",       CUENTA) \
    X(MINADOS_MANO,        "minados_mano",        "BLOQUES ROTOS A MANO",            CUENTA) \
    X(MINADOS_PICO,        "minados_pico",        "BLOQUES ROTOS CON PICO",          CUENTA) \
    X(MINADOS_HACHA,       "minados_hacha",       "BLOQUES ROTOS CON HACHA",         CUENTA) \
    X(MINADOS_MARTILLO,    "minados_martillo",    "BLOQUES ROTOS CON MARTILLO",      CUENTA) \
    X(MINADOS_AZADA,       "minados_azada",       "BLOQUES ROTOS CON AZADA",         CUENTA) \
    X(TRONCOS,             "troncos",             "TRONCOS CORTADOS",                CUENTA) \
    X(HOJAS,               "hojas",               "HOJAS QUITADAS",                  CUENTA) \
    X(MINADOS_NOCHE,       "minados_noche",       "BLOQUES MINADOS DE NOCHE",        CUENTA) \
    X(CAPAS_COLOCADAS,     "capas_colocadas",     "CAPAS COLOCADAS",                 CUENTA) \
    X(ENTEROS_COLOCADOS,   "enteros_colocados",   "BLOQUES ENTEROS COLOCADOS",       CUENTA) \
    X(GOLPES,              "golpes",              "GOLPES DADOS",                    CUENTA) \
    X(GOLPES_AIRE,         "golpes_aire",         "GOLPES AL AIRE",                  CUENTA) \
    X(GOLPES_ANIMALES,     "golpes_animales",     "GOLPES A ANIMALES",               CUENTA) \
    X(ANIMALES_CAZADOS,    "animales_cazados",    "ANIMALES CAZADOS",                CUENTA) \
    /* --- Objetos --- */ \
    X(ITEMS_RECOGIDOS,     "items_recogidos",     "OBJETOS RECOGIDOS",               CUENTA) \
    X(PEDAZOS_RECOGIDOS,   "pedazos_recogidos",   "PEDAZOS RECOGIDOS DEL SUELO",     CUENTA) \
    X(ITEMS_TIRADOS,       "items_tirados",       "OBJETOS TIRADOS",                 CUENTA) \
    X(ITEMS_FABRICADOS,    "items_fabricados",    "OBJETOS FABRICADOS",              CUENTA) \
    X(HERRAMIENTAS_HECHAS, "herramientas_hechas", "HERRAMIENTAS FABRICADAS",         CUENTA) \
    X(TUNAS,               "tunas",               "TUNAS COSECHADAS",                CUENTA) \
    X(PENCAS,              "pencas",              "PENCAS COSECHADAS",               CUENTA) \
    X(CAMBIOS_HOTBAR,      "cambios_hotbar",      "CAMBIOS DE CASILLA",              CUENTA) \
    /* --- El juego --- */ \
    X(PAUSAS,              "pausas",              "VECES EN PAUSA",                  CUENTA) \
    X(INVENTARIOS,         "inventarios",         "VECES QUE ABRISTE EL INVENTARIO", CUENTA) \
    X(CANCIONES,           "canciones",           "CANCIONES ESCUCHADAS",            CUENTA) \
    X(CAPTURAS,            "capturas",            "CAPTURAS DE PANTALLA",            CUENTA) \
    X(GUARDADOS,           "guardados",           "VECES GUARDADO",                  CUENTA) \
    X(SESIONES,            "sesiones",            "SESIONES DE JUEGO",               CUENTA) \
    /* --- Los records --- */ \
    X(ALTURA_MAX,          "altura_max",          "ALTURA MAXIMA",                   RECORD) \
    X(ALTURA_MIN,          "altura_min",          "PROFUNDIDAD MAXIMA",              MINIMO) \
    X(CAIDA_MAX,           "caida_max",           "CAIDA MAS LARGA",                 RECORD) \
    X(VELOCIDAD_MAX,       "velocidad_max",       "VELOCIDAD MAXIMA EN M POR S",     RECORD) \
    X(LEJOS_CENTRO,        "lejos_centro",        "LO MAS LEJOS DEL CENTRO DEL MUNDO", RECORD) \
    X(RACHA_MINADO,        "racha_minado",        "MAS BLOQUES SEGUIDOS SIN PARAR",  RECORD) \
    X(SESION_LARGA,        "sesion_larga",        "SESION MAS LARGA",                RECORD)

enum Id {
#define VW_E_ID(id, clave, nombre, clase) id,
    VW_ESTADISTICAS(VW_E_ID)
#undef VW_E_ID
    NUM
};

inline const Info& info(int i) {
    static const Info T[NUM] = {
#define VW_E_INFO(id, clave, nombre, clase) { clave, nombre, Clase::clase },
        VW_ESTADISTICAS(VW_E_INFO)
#undef VW_E_INFO
    };
    return T[i];
}

struct Registro {
    double v[NUM] = {};
    bool tiene[NUM] = {};      // un RECORD/MINIMO sin dato todavia no se muestra como 0

    void reiniciar() { *this = Registro{}; }
    void sumar(Id id, double x) { v[id] += x; tiene[id] = true; }
    void contar(Id id) { sumar(id, 1.0); }
    void record(Id id, double x) {
        if (!tiene[id] || x > v[id]) v[id] = x;
        tiene[id] = true;
    }
    void minimo(Id id, double x) {
        if (!tiene[id] || x < v[id]) v[id] = x;
        tiene[id] = true;
    }

    std::string texto() const {
        std::string s;
        char linea[160];
        for (int i = 0; i < NUM; ++i) {
            if (!tiene[i]) continue;
            std::snprintf(linea, sizeof(linea), "%s %.6g\n", info(i).clave, v[i]);
            s += linea;
        }
        return s;
    }
    void leer(const std::string& s) {
        reiniciar();
        std::unordered_map<std::string, int> porClave;
        for (int i = 0; i < NUM; ++i) porClave[info(i).clave] = i;
        size_t p = 0;
        while (p < s.size()) {
            size_t fin = s.find('\n', p);
            if (fin == std::string::npos) fin = s.size();
            const std::string linea = s.substr(p, fin - p);
            p = fin + 1;
            const size_t esp = linea.find(' ');
            if (esp == std::string::npos) continue;
            auto it = porClave.find(linea.substr(0, esp));
            if (it == porClave.end()) continue;             // clave de otra version
            const double x = std::atof(linea.c_str() + esp + 1);
            if (!std::isfinite(x)) continue;
            v[it->second] = x;
            tiene[it->second] = true;
        }
    }
};

// Un tiempo, legible. Los segundos van como "SEG" y no como "S": en la fuente
// del juego la S y el 5 se dibujan casi iguales, y "41S" se leia "415".
inline std::string tiempo(double segundos) {
    char b[64];
    const long t = (long)segundos;
    if (t >= 3600) std::snprintf(b, sizeof(b), "%ld H %02ld MIN", t / 3600, (t / 60) % 60);
    else if (t >= 60) std::snprintf(b, sizeof(b), "%ld MIN %02ld SEG", t / 60, t % 60);
    else std::snprintf(b, sizeof(b), "%ld SEG", t);
    return b;
}

// Como se ve cada valor en la pantalla de estadisticas.
inline std::string formato(int i, double x, bool tiene = true) {
    if (!tiene) return "-";
    char b[64];
    switch (info(i).clase) {
        case Clase::METROS:
        case Clase::RECORD:
        case Clase::MINIMO:
            if (std::string(info(i).clave) == "racha_minado") { std::snprintf(b, sizeof(b), "%.0f", x); break; }
            if (std::string(info(i).clave) == "sesion_larga") return tiempo(x);
            if (std::string(info(i).clave) == "velocidad_max") { std::snprintf(b, sizeof(b), "%.1f", x); break; }
            if (std::fabs(x) >= 1000.0) std::snprintf(b, sizeof(b), "%.2f KM", x / 1000.0);
            else std::snprintf(b, sizeof(b), "%.0f M", x);
            break;
        case Clase::SEGUNDOS: return tiempo(x);
        case Clase::DANO:  std::snprintf(b, sizeof(b), "%.1f", x); break;
        case Clase::HORAS: std::snprintf(b, sizeof(b), "%.1f H", x); break;
        case Clase::CUENTA:
        default:           std::snprintf(b, sizeof(b), "%.0f", x); break;
    }
    return b;
}

} // namespace Estadisticas
