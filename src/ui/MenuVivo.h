// ============================================================================
// ⭐ EL MENU VIVO: UNA MINI IA QUE JUEGA SOLA EN EL FONDO DEL MENU PRINCIPAL
// ============================================================================
// PEDIDO: "quiero que crees un sistema de que una mini ia juega el juego de
// fondo y hace cosas".
//
// El primer plano del paisaje del menu principal es un mundito de bloques visto
// de lado: un ANILLO de N columnas (sale por un lado y entra por el otro), cada
// una con su altura, y arboles encima. En el vive un personaje que nadie
// maneja. Decide solo que hacer, va hasta alli andando y lo hace:
//
//   TALAR       va al arbol maduro mas cercano y le da cuatro hachazos: el
//               arbol cae y se lleva 3 de madera.
//   PLANTAR     pone un retono, que crece en tres etapas hasta ser un arbol.
//               Planta mas cuanto menos bosque queda: nunca lo acaba.
//   CONSTRUIR   con 5 de madera busca un sitio llano y levanta una casita,
//               pieza a pieza (8 piezas, 1 de madera cada una). Si se queda sin
//               madera a medias, se va a talar y vuelve.
//   DESMONTAR   cuando ya hay 3 casas, desarma la mas vieja (recupera la mitad
//               de la madera) para tener donde hacer otra.
//   ALLANAR     pica el bloque que sobresale entre dos vecinos (y se queda la
//               piedra) o rellena el hoyo.
//   DESCANSAR   se queda un rato quieto, mirando a un lado y a otro.
//
// Es una maquina de estados con pesos (lo que mas falta hace pesa mas) y algo
// de azar: no hay dos menus iguales, y no se queda parado ni en bucle.
//
// El suelo siempre es andable: entre dos columnas vecinas hay como mucho UN
// escalon (medio bloque), y ni allanar ni construir lo rompen. Las casas van
// DETRAS del personaje: no le estorban al pasar.
//
// Cabecera sola, sin motor ni OpenGL: main.cpp la dibuja (dibujarMenuVivo) y
// tests/test_menu_vivo.cpp la hace jugar veinte minutos seguidos.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace MenuVivo {

constexpr int N = 120;                 // columnas del anillo
constexpr int ALTO_MAX = 4;            // escalones de alto, como mucho
constexpr int ARBOL_MADURO = 3;        // etapas: 1 retono, 2 joven, 3 maduro
constexpr int GOLPES_ARBOL = 4;
constexpr int MADERA_POR_ARBOL = 3;
constexpr int PIEZAS_CASA = 8;
constexpr int MAX_CASAS = 3;
constexpr int MADERA_PARA_CASA = 5;    // con menos no empieza una
constexpr int MADERA_TOPE = 12;        // con tanta ya no tala
constexpr float VELOCIDAD = 2.4f;      // columnas por segundo
constexpr float CRECER_SEG = 14.0f;    // lo que tarda un arbol en pasar de etapa

enum class Tarea : uint8_t { PENSAR, IR, TALAR, PLANTAR, CONSTRUIR, DESMONTAR, ALLANAR, DESCANSAR, NUM };

inline const char* rotulo(Tarea t) {
    switch (t) {
        case Tarea::IR:        return "CAMINANDO";
        case Tarea::TALAR:     return "TALANDO";
        case Tarea::PLANTAR:   return "PLANTANDO";
        case Tarea::CONSTRUIR: return "CONSTRUYENDO";
        case Tarea::DESMONTAR: return "DESMONTANDO";
        case Tarea::ALLANAR:   return "ALLANANDO";
        case Tarea::DESCANSAR: return "DESCANSANDO";
        default:               return "PENSANDO";
    }
}

inline int mod(int i) { return ((i % N) + N) % N; }
inline float envolver(float x) {
    x = std::fmod(x, (float)N);
    return x < 0.0f ? x + (float)N : x;
}
// De `a` a `b` por el camino corto del anillo, con signo.
inline float delta(float a, float b) {
    float d = std::fmod(b - a, (float)N);
    if (d > N * 0.5f) d -= (float)N;
    if (d < -N * 0.5f) d += (float)N;
    return d;
}

struct Columna {
    int alto = 0;          // escalones
    int arbol = 0;         // 0 nada, 1..ARBOL_MADURO
    int golpes = 0;        // hachazos que lleva
    float crece = 0.0f;
};

struct Casa {
    int centro = 0;        // ocupa centro-1 .. centro+1
    int piezas = 0;
};

struct Sim {
    Columna col[N];
    std::vector<Casa> casas;           // de la mas vieja a la mas nueva
    float x = 0.5f;                    // donde esta, en columnas
    int dir = 1;                       // hacia donde mira
    Tarea tarea = Tarea::PENSAR, alLlegar = Tarea::PENSAR;
    int destino = 0;                   // la columna (o el centro de la casa) de la tarea
    float meta = 0.5f;                 // donde se para
    float reloj = 0.5f;
    float golpe = 0.0f;                // 0..1 dentro del gesto en curso (para dibujarlo)
    float fase = 0.0f;                 // el paso, al andar
    int madera = 0, piedra = 0;
    int casaActiva = -1;
    int hechas[(int)Tarea::NUM] = {};  // cuantas veces termino cada cosa
    int decisiones = 0;
    uint32_t rng = 1u;
    bool lista = false;

    float azar() {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float)(rng & 0xffffffu) / 16777216.0f;
    }

    void iniciar(uint32_t semilla) {
        *this = Sim();
        rng = semilla ? semilla : 1u;
        for (int k = 0; k < 8; ++k) azar();
        const float f1 = azar() * 6.2831853f, f2 = azar() * 6.2831853f;
        for (int i = 0; i < N; ++i) {
            const float th = 6.2831853f * (float)i / (float)N;
            const int h = (int)std::lround(2.0f + 1.4f * std::sin(3.0f * th + f1) + 0.7f * std::sin(7.0f * th + f2));
            col[i].alto = (std::max)(0, (std::min)(ALTO_MAX, h));
        }
        // Un escalon como mucho entre vecinas: se rebaja la que sobresale.
        for (int vuelta = 0; vuelta < 4 * N; ++vuelta) {
            bool cambio = false;
            for (int i = 0; i < N; ++i) {
                const int m = (std::min)(col[mod(i - 1)].alto, col[mod(i + 1)].alto) + 1;
                if (col[i].alto > m) { col[i].alto = m; cambio = true; }
            }
            if (!cambio) break;
        }
        // Bultos y hoyos sueltos en lo llano: algo que allanar.
        for (int k = 0; k < 18; ++k) {
            const int i = (int)(azar() * N) % N;
            const int a = col[i].alto;
            if (col[mod(i - 1)].alto != a || col[mod(i + 1)].alto != a) continue;
            if (azar() < 0.5f) { if (a < ALTO_MAX) col[i].alto = a + 1; }
            else if (a > 0) col[i].alto = a - 1;
        }
        for (int i = 0; i < N; ++i) {
            if (col[mod(i - 1)].arbol || azar() > 0.22f) continue;
            const float r = azar();
            col[i].arbol = r < 0.70f ? ARBOL_MADURO : (r < 0.85f ? 2 : 1);
            col[i].crece = azar() * CRECER_SEG;
        }
        x = 0.5f + (float)(int)(azar() * N);
        lista = true;
    }

    // --- lo que hay -----------------------------------------------------
    bool enCasa(int i, int margen = 1) const {
        for (const Casa& c : casas)
            if (std::fabs(delta((float)c.centro, (float)mod(i))) <= (float)margen) return true;
        return false;
    }
    bool libre(int i) const { return col[mod(i)].arbol == 0 && !enCasa(i); }
    bool plantable(int i) const {
        return libre(i) && col[mod(i - 1)].arbol == 0 && col[mod(i + 1)].arbol == 0;
    }
    bool sitio(int c) const {
        for (int k = -1; k <= 1; ++k)
            if (col[mod(c + k)].alto != col[mod(c)].alto || col[mod(c + k)].arbol) return false;
        return !enCasa(c, 5);
    }
    // +1 hay que rellenar, -1 hay que picar, 0 nada.
    int allanable(int i) const {
        if (!libre(i) || enCasa(i, 2)) return 0;
        const int a = col[mod(i)].alto, l = col[mod(i - 1)].alto, r = col[mod(i + 1)].alto;
        if (a > l && a > r) return -1;
        if (a < l && a < r) return 1;
        return 0;
    }
    int arboles() const {
        int n = 0;
        for (const Columna& c : col) if (c.arbol) ++n;
        return n;
    }
    template <class F>
    int cercano(F vale, int hasta) const {
        const int aqui = (int)std::floor(x);
        for (int d = 2; d <= hasta; ++d)
            for (int s = 1; s >= -1; s -= 2)
                if (vale(mod(aqui + s * d))) return mod(aqui + s * d);
        return -1;
    }

    // --- decidir --------------------------------------------------------
    void pensar(float espera) { tarea = Tarea::PENSAR; reloj = espera; golpe = 0.0f; }
    void ir(int c, Tarea que, bool alLado) {
        destino = mod(c);
        alLlegar = que;
        const float centro = (float)destino + 0.5f;
        meta = alLado ? envolver(centro + (delta(x, centro) >= 0.0f ? -1.0f : 1.0f)) : centro;
        tarea = Tarea::IR;
    }
    void descansar() { tarea = Tarea::DESCANSAR; reloj = 2.0f + 2.5f * azar(); golpe = 0.0f; }

    void elegir() {
        ++decisiones;
        // Una casa a medias y madera en el bolsillo: se termina.
        if (madera > 0)
            for (int k = 0; k < (int)casas.size(); ++k)
                if (casas[k].piezas < PIEZAS_CASA) { casaActiva = k; ir(casas[k].centro, Tarea::CONSTRUIR, false); return; }
        bool aMedias = false;
        for (const Casa& c : casas) aMedias = aMedias || c.piezas < PIEZAS_CASA;
        if (madera >= MADERA_PARA_CASA && !aMedias) {
            if ((int)casas.size() < MAX_CASAS) {
                const int c = cercano([this](int i) { return sitio(i); }, N / 2 - 1);
                if (c >= 0) {
                    casas.push_back(Casa{ c, 0 });
                    casaActiva = (int)casas.size() - 1;
                    ir(c, Tarea::CONSTRUIR, false);
                    return;
                }
            } else if (azar() < 0.35f) {
                ir(casas[0].centro, Tarea::DESMONTAR, false);
                return;
            }
        }
        const int arbol = madera < MADERA_TOPE
            ? cercano([this](int i) { return col[i].arbol == ARBOL_MADURO; }, N / 2 - 1) : -1;
        const int hueco = cercano([this](int i) { return plantable(i); }, 25);
        const int bulto = cercano([this](int i) { return allanable(i) != 0; }, 30);
        const int bosque = arboles();
        float pTalar = arbol >= 0 ? (aMedias ? 9.0f : 5.0f) : 0.0f;
        if (bosque < 8) pTalar *= 0.2f;                       // que no lo acabe
        const float pPlantar = hueco >= 0 ? (bosque < 14 ? 6.0f : 1.5f) : 0.0f;
        const float pAllanar = bulto >= 0 ? 2.0f : 0.0f;
        const float pDescansar = 1.0f;
        float r = azar() * (pTalar + pPlantar + pAllanar + pDescansar);
        if ((r -= pTalar) < 0.0f) { ir(arbol, Tarea::TALAR, true); return; }
        if ((r -= pPlantar) < 0.0f) { ir(hueco, Tarea::PLANTAR, true); return; }
        if ((r -= pAllanar) < 0.0f) { ir(bulto, Tarea::ALLANAR, true); return; }
        descansar();
    }

    // --- el tiempo ------------------------------------------------------
    void avanzar(float dt) {
        if (!lista) iniciar(1u);
        if (!(dt > 0.0f)) return;
        if (dt > 0.1f) dt = 0.1f;
        for (Columna& c : col) {
            if (c.arbol == 0 || c.arbol >= ARBOL_MADURO) continue;
            c.crece += dt;
            if (c.crece >= CRECER_SEG) { c.crece = 0.0f; ++c.arbol; }
        }
        // Cada `periodo` segundos, un golpe; `golpe` es por donde va el gesto.
        auto cadaGolpe = [&](float periodo) {
            reloj += dt;
            golpe = reloj / periodo;
            if (reloj < periodo) return false;
            reloj -= periodo;
            golpe = 0.0f;
            return true;
        };
        switch (tarea) {
            case Tarea::PENSAR:
                reloj -= dt;
                if (reloj <= 0.0f) elegir();
                break;
            case Tarea::IR: {
                const float d = delta(x, meta);
                if (std::fabs(d) <= VELOCIDAD * dt) {
                    x = meta;
                    const float mira = delta(x, (float)destino + 0.5f);
                    if (std::fabs(mira) > 0.01f) dir = mira > 0.0f ? 1 : -1;
                    tarea = alLlegar;
                    reloj = 0.0f;
                    golpe = 0.0f;
                } else {
                    dir = d > 0.0f ? 1 : -1;
                    x = envolver(x + dir * VELOCIDAD * dt);
                    fase += dt * 9.0f;
                }
                break;
            }
            case Tarea::TALAR: {
                Columna& c = col[destino];
                if (c.arbol != ARBOL_MADURO) { pensar(0.3f); break; }
                if (!cadaGolpe(0.55f)) break;
                if (++c.golpes >= GOLPES_ARBOL) {
                    c.arbol = 0; c.golpes = 0; c.crece = 0.0f;
                    madera += MADERA_POR_ARBOL;
                    ++hechas[(int)Tarea::TALAR];
                    pensar(0.5f);
                }
                break;
            }
            case Tarea::PLANTAR:
                if (!cadaGolpe(1.2f)) break;
                if (plantable(destino)) {
                    col[destino].arbol = 1; col[destino].crece = 0.0f; col[destino].golpes = 0;
                    ++hechas[(int)Tarea::PLANTAR];
                }
                pensar(0.4f);
                break;
            case Tarea::CONSTRUIR: {
                if (casaActiva < 0 || casaActiva >= (int)casas.size()) { pensar(0.3f); break; }
                Casa& c = casas[casaActiva];
                if (madera <= 0 || c.piezas >= PIEZAS_CASA) { pensar(0.4f); break; }
                if (!cadaGolpe(0.7f)) break;
                ++c.piezas;
                --madera;
                ++hechas[(int)Tarea::CONSTRUIR];
                break;
            }
            case Tarea::DESMONTAR: {
                if (casas.empty() || casas[0].centro != destino) { pensar(0.3f); break; }
                if (!cadaGolpe(0.4f)) break;
                Casa& c = casas[0];
                if (c.piezas > 0 && (--c.piezas % 2) == 0) ++madera;      // se recupera la mitad
                if (c.piezas <= 0) {
                    casas.erase(casas.begin());
                    casaActiva = -1;
                    ++hechas[(int)Tarea::DESMONTAR];
                    pensar(0.5f);
                }
                break;
            }
            case Tarea::ALLANAR: {
                if (!cadaGolpe(1.0f)) break;
                const int que = allanable(destino);
                if (que < 0) { --col[destino].alto; ++piedra; }
                else if (que > 0) { ++col[destino].alto; if (piedra > 0) --piedra; }
                if (que != 0) ++hechas[(int)Tarea::ALLANAR];
                pensar(0.4f);
                break;
            }
            case Tarea::DESCANSAR: {
                const float antes = reloj;
                reloj -= dt;
                if (antes > 1.2f && reloj <= 1.2f) dir = -dir;           // mira al otro lado
                if (reloj <= 0.0f) { ++hechas[(int)Tarea::DESCANSAR]; pensar(0.2f); }
                break;
            }
            default: pensar(0.2f); break;
        }
    }
};

}  // namespace MenuVivo
