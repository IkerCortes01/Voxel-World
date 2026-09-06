#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

// ============================================================================
// QUE MAQUINA ES ESTA, Y CUANTO PUEDE PEDIRSELE
// ============================================================================
// RESPONSABILIDAD UNICA: mirar el hardware y traducirlo a ajustes concretos
// (distancia de render, hilos, presupuesto de mallado).
//
// NO dibuja, NO conoce World, NO toca OpenGL: recibe las cadenas que devuelve
// glGetString ya leidas. Por eso se puede probar sin arrancar el juego, que es
// justo lo que hace tests/test_perfil.cpp con una tabla de GPUs reales.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA QUE RESUELVE
// ----------------------------------------------------------------------------
// RENDER_DISTANCE era una constante: 5. El mismo numero para una integrada de
// 2012 y para una RTX de 2024.
//
// Y el numero estaba elegido para la maquina de desarrollo, con este
// razonamiento escrito en el codigo: "bajado de 6 a 5 para sostener 26 FPS".
// O sea que la maquina buena tambien se quedaba en 5, viendo cerca y
// desperdiciando tarjeta, mientras que en una peor que la de referencia 5
// seguia siendo demasiado.
//
// Un juego que se lleva en un USB a otro ordenador no puede tener ese numero
// fijo. Aqui se decide al arrancar.
//
// ----------------------------------------------------------------------------
// POR QUE SE MIRA LA GPU Y NO SOLO SE MIDE
// ----------------------------------------------------------------------------
// Hay dos formas de saber cuanto aguanta una maquina:
//
//   MEDIR      Arrancar, ver los FPS y ajustar. Es lo correcto a medio plazo,
//              y es lo que hace el regulador adaptativo -- pero necesita
//              varios segundos de partida para converger. Durante esos
//              segundos el jugador ya esta viendo tirones.
//
//   RECONOCER  Preguntarle a la GPU su nombre y partir de un ajuste sensato
//              desde el PRIMER frame.
//
// Se hacen las dos: esto da el punto de partida y el regulador afina desde
// ahi. El coste de reconocer es una comparacion de cadenas al arrancar.
//
// ----------------------------------------------------------------------------
// LA REGLA DE ORO: ANTE LA DUDA, LO CONSERVADOR
// ----------------------------------------------------------------------------
// Una GPU desconocida NO se trata como potente. Un juego que va corto de vista
// pero fluido se siente bien; uno que va a tirones se siente roto, y el
// jugador no sabe que puede subir un ajuste. Ademas el regulador SUBE la
// calidad cuando sobra margen, asi que equivocarse por abajo se corrige solo
// en unos segundos -- equivocarse por arriba, no.
// ============================================================================

namespace Render {

// ----------------------------------------------------------------------------
// LOS NIVELES
// ----------------------------------------------------------------------------
// Cuatro escalones. No mas: cada uno tiene que significar algo distinto de
// verdad, y con ocho niveles la diferencia entre vecinos no se ve.
enum class NivelMaquina : int {
    MINIMO = 0,   // integradas viejas (HD 3000/4000, GMA), netbooks
    BAJO,         // integradas modernas o dedicadas de gama de entrada
    MEDIO,        // dedicada normal, o integrada actual buena (Iris Xe, Vega)
    ALTO          // dedicada de gama media-alta en adelante
};

inline const char* nombreNivel(NivelMaquina n) {
    switch (n) {
        case NivelMaquina::MINIMO: return "minimo";
        case NivelMaquina::BAJO:   return "bajo";
        case NivelMaquina::MEDIO:  return "medio";
        default:                   return "alto";
    }
}

// ----------------------------------------------------------------------------
// LO QUE SE DECIDE
// ----------------------------------------------------------------------------
struct Perfil {
    NivelMaquina nivel = NivelMaquina::BAJO;

    // Radio de chunks que se cargan y se dibujan. Es el ajuste que MAS pesa:
    // el area crece con el cuadrado, asi que pasar de 5 a 4 quita un 36% de
    // los chunks a la vista.
    int distanciaRender = 4;

    // Hilos que generan y mallan chunks. Nunca todos los nucleos: el hilo
    // principal necesita el suyo para dibujar, y el sistema operativo tambien
    // existe.
    int hilosTrabajo = 2;

    // Milisegundos por frame que puede gastarse en construir mallas. Es el
    // freno que evita el tiron al explorar: mas presupuesto carga el mundo mas
    // deprisa, pero un solo chunk pesado puede desbordarlo entero.
    float presupuestoMalladoMs = 2.5f;

    // FPS a los que apunta el regulador. Por debajo baja calidad, por encima
    // la sube.
    int fpsObjetivo = 120;

    // Efectos que se pueden apagar en las maquinas justas. Cada uno cuesta
    // medido, no supuesto (ver las notas de cada asignacion).
    bool nieblaPorPixel = false;   // GL_NICEST en vez de GL_FASTEST
    bool follajeAnimado = true;    // el refresco de las aciculas al andar

    // Para el log y la pantalla de ajustes.
    std::string descripcionGPU;
};

// ----------------------------------------------------------------------------
// RECONOCER LA GPU POR SU NOMBRE
// ----------------------------------------------------------------------------
// Busca subcadenas en lo que devuelve glGetString(GL_RENDERER). No pretende
// ser una base de datos completa: acierta las familias frecuentes y, para todo
// lo demas, cae en un valor por defecto prudente.
namespace detalle {

inline std::string aMinusculas(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    }
    return r;
}

inline bool contiene(const std::string& heno, const char* aguja) {
    return heno.find(aguja) != std::string::npos;
}

} // namespace detalle

// ----------------------------------------------------------------------------
// LA CLASIFICACION
// ----------------------------------------------------------------------------
// `renderer` y `vendor` son lo que devuelve glGetString. `nucleos` es
// hardware_concurrency(), y `memoriaMB` la RAM del sistema (0 = no se sabe).
inline NivelMaquina clasificar(const std::string& rendererCrudo,
                               const std::string& vendorCrudo,
                               unsigned nucleos,
                               uint64_t memoriaMB) {
    using namespace detalle;
    const std::string r = aMinusculas(rendererCrudo);
    const std::string v = aMinusculas(vendorCrudo);

    // ------------------------------------------------------------------
    // 1. LO QUE SEGURO ES LENTO
    // ------------------------------------------------------------------
    // Software rendering: no hay GPU de verdad detras. Es el caso de una
    // maquina virtual sin aceleracion, y ahi hay que pedir lo minimo.
    if (contiene(r, "llvmpipe") || contiene(r, "softpipe") ||
        contiene(r, "swiftshader") || contiene(r, "gdi generic") ||
        contiene(r, "software")) {
        return NivelMaquina::MINIMO;
    }

    // Integradas Intel viejas. La HD 4000 (2012) es la maquina de referencia
    // de este motor: con 756.000 caras a la vista gasta 10,4 ms solo en
    // dibujar, asi que el techo esta en ~96 FPS aunque la CPU no hiciera nada.
    // Todo lo anterior o igual entra aqui.
    if (contiene(r, "gma") || contiene(r, "hd graphics 2000") ||
        contiene(r, "hd graphics 3000") || contiene(r, "hd graphics 4000") ||
        contiene(r, "hd graphics 2500") || contiene(r, "hd graphics 4400") ||
        contiene(r, "hd graphics 4600")) {
        return NivelMaquina::MINIMO;
    }

    // ------------------------------------------------------------------
    // 2. LO QUE SEGURO ES RAPIDO
    // ------------------------------------------------------------------
    // Dedicadas modernas. Se reconoce la FAMILIA, no el modelo exacto: una
    // lista de modelos se queda vieja en un año.
    // ⚠️ OJO CON LAS MARCAS INTERCALADAS. El driver de Intel se anuncia como
    // "Intel(R) Arc(TM) A770 Graphics": buscar "arc a" NO casa, porque entre
    // "arc" y "a770" hay un "(tm)". Lo caso un test con el nombre real del
    // driver, y por eso se buscan los dos trozos por separado.
    const bool esArc = contiene(r, "arc") &&
                       (contiene(r, " a") || contiene(r, ")a"));

    if (contiene(r, "rtx") || contiene(r, "radeon rx") ||
        esArc || contiene(r, "quadro rtx")) {
        return NivelMaquina::ALTO;
    }

    // GeForce GTX de la 900 en adelante. La 700 y anteriores van a MEDIO.
    if (contiene(r, "gtx 9") || contiene(r, "gtx 10") ||
        contiene(r, "gtx 16")) {
        return NivelMaquina::ALTO;
    }

    // Apple Silicon: integrada, pero rapida.
    if (contiene(r, "apple m1") || contiene(r, "apple m2") ||
        contiene(r, "apple m3") || contiene(r, "apple m4")) {
        return NivelMaquina::ALTO;
    }

    // ------------------------------------------------------------------
    // 3. LA ZONA MEDIA
    // ------------------------------------------------------------------
    // Integradas actuales decentes: van bien pero no son una dedicada.
    if (contiene(r, "iris") || contiene(r, "uhd graphics") ||
        contiene(r, "vega") || contiene(r, "radeon graphics")) {
        return NivelMaquina::BAJO;
    }

    // Dedicadas viejas o de entrada.
    if (contiene(r, "geforce") || contiene(r, "radeon") ||
        contiene(r, "nvidia") || contiene(v, "nvidia")) {
        return NivelMaquina::MEDIO;
    }

    // ------------------------------------------------------------------
    // 4. NO SE RECONOCE: DECIDIR POR LA CPU Y LA RAM
    // ------------------------------------------------------------------
    // Es la rama que de verdad importa para "llevarlo en un USB a otro PC":
    // la mayoria de GPUs del mundo no estan en la lista de arriba.
    //
    // Se usa lo unico que siempre se sabe -- nucleos y memoria -- y se tira a
    // la baja. Un equipo con pocos nucleos o poca RAM no va a mover un mundo
    // grande por buena que sea su tarjeta.
    if (nucleos >= 8 && memoriaMB >= 16000) return NivelMaquina::MEDIO;
    if (nucleos >= 4 && memoriaMB >= 8000)  return NivelMaquina::BAJO;

    // Cuando no se sabe nada, lo minimo. El regulador subira si sobra margen.
    return NivelMaquina::MINIMO;
}

// ----------------------------------------------------------------------------
// DE NIVEL A AJUSTES
// ----------------------------------------------------------------------------
inline Perfil perfilPara(NivelMaquina nivel, unsigned nucleos) {
    Perfil p;
    p.nivel = nivel;

    // Los hilos de trabajo salen de los nucleos, no del nivel: una CPU buena
    // con GPU mala sigue pudiendo generar terreno deprisa. Se reserva SIEMPRE
    // uno para el hilo principal (que dibuja) y otro para el sistema.
    const int libres = (nucleos > 2) ? (int)nucleos - 2 : 1;
    p.hilosTrabajo = libres < 1 ? 1 : (libres > 6 ? 6 : libres);

    switch (nivel) {
        case NivelMaquina::MINIMO:
            // 3 chunks de radio: se ve mas cerca, pero la niebla lo disimula y
            // el juego va fluido. Medido en la HD 4000: bajar de 5 a 4 ya daba
            // 118 FPS estables, y de 4 a 3 quita otro 44% de caras.
            p.distanciaRender      = 3;
            p.presupuestoMalladoMs = 1.5f;
            p.fpsObjetivo          = 150;
            p.nieblaPorPixel       = false;
            // El refresco del follaje remalla chunks enteros al andar: medido
            // entre 4 y 16 ms por pasada. En la maquina mas justa no compensa.
            p.follajeAnimado       = false;
            break;

        case NivelMaquina::BAJO:
            p.distanciaRender      = 4;
            p.presupuestoMalladoMs = 2.0f;
            p.fpsObjetivo          = 150;
            p.nieblaPorPixel       = false;
            p.follajeAnimado       = true;
            break;

        case NivelMaquina::MEDIO:
            p.distanciaRender      = 6;
            p.presupuestoMalladoMs = 3.0f;
            p.fpsObjetivo          = 144;
            p.nieblaPorPixel       = false;
            p.follajeAnimado       = true;
            break;

        case NivelMaquina::ALTO:
        default:
            // Aqui si se abre la mano: es el caso de "y entre mejor sea la
            // computadora, ira mejor". Mas vista Y mas FPS a la vez.
            p.distanciaRender      = 8;
            p.presupuestoMalladoMs = 5.0f;
            p.fpsObjetivo          = 200;
            p.nieblaPorPixel       = true;
            p.follajeAnimado       = true;
            break;
    }

    return p;
}

// ----------------------------------------------------------------------------
// LA ENTRADA UNICA
// ----------------------------------------------------------------------------
// Lo que llama el motor al arrancar, con las cadenas de glGetString ya leidas.
inline Perfil detectar(const char* renderer,
                       const char* vendor,
                       unsigned nucleos,
                       uint64_t memoriaMB) {
    const std::string r = renderer ? renderer : "";
    const std::string v = vendor   ? vendor   : "";

    if (nucleos == 0) nucleos = std::thread::hardware_concurrency();
    if (nucleos == 0) nucleos = 2;      // si ni eso se sabe, suponer poco

    const NivelMaquina nivel = clasificar(r, v, nucleos, memoriaMB);
    Perfil p = perfilPara(nivel, nucleos);
    p.descripcionGPU = r.empty() ? "(desconocida)" : r;
    return p;
}

// ----------------------------------------------------------------------------
// TOPES DE SEGURIDAD
// ----------------------------------------------------------------------------
// El regulador adaptativo mueve la distancia en caliente. Estos son los
// limites dentro de los que puede moverse, para que ni se quede ciego ni
// intente cargar medio mundo.
constexpr int DISTANCIA_MINIMA = 2;
constexpr int DISTANCIA_MAXIMA = 12;

inline int acotarDistancia(int d) {
    if (d < DISTANCIA_MINIMA) return DISTANCIA_MINIMA;
    if (d > DISTANCIA_MAXIMA) return DISTANCIA_MAXIMA;
    return d;
}

} // namespace Render
