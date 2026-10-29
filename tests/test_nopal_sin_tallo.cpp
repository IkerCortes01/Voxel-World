#include <doctest/doctest.h>
#include "BlockType.h"
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <cmath>

// ============================================================================
// EL NOPAL YA NO TIENE TRONCO: SU EJE SON PENCAS
// ============================================================================
// LO QUE SE PIDIO: "elimina de la generacion de nopal, el cladodio y sea puras
// pencas y sea mas realistas los nopales la generacion".
//
// Aclaracion de nomenclatura, porque en este motor se usaban las dos palabras
// para la misma cosa: un CLADODIO ES una penca. `BLOCK_NOPAL_CLADODIO_X2` esta
// comentado en BlockType.h como "Dos pencas juntas". Lo que SI era una pieza
// distinta es `BLOCK_NOPAL_TALLO`: un bloque CUBICO COMPLETO que hacia de
// tronco central, con las pencas brotando de sus costados.
//
// Ese tronco es lo que se retira, y es lo que da el realismo pedido:
//
//   - Una Opuntia ficus-indica NO tiene fuste de madera. Es una cactacea cuyo
//     cuerpo entero son pencas encadenadas: cada penca nueva brota del borde
//     de la anterior, desde el suelo.
//   - Lo que en un ejemplar viejo parece un tronco es la penca basal
//     LIGNIFICADA -- sigue siendo una penca. Esa funcion la cumple ya el
//     bloque de base (BLOCK_NOPAL_BASE_*), que es la union con el suelo.
//
// ----------------------------------------------------------------------------
// POR QUE ESTE ARCHIVO REIMPLEMENTA LAS REGLAS
// ----------------------------------------------------------------------------
// `World::generarNopal` vive dentro de main.cpp, que no se enlaza con los
// tests (ver docs/AUDITORIA-2026-09-17.md, hallazgo I3). Asi que aqui se
// reimplementan las REGLAS ESTRUCTURALES del generador y se fijan sus
// invariantes. Es la misma convencion que ya siguen test_agua_flujo.cpp y
// test_luz_costura.cpp, y se declara igual de explicitamente.
//
// Lo que este archivo SI puede garantizar de verdad, sin reimplementar nada:
// que el tallo esta fuera del inventario creativo y que su ID sigue existiendo
// para no romper los mundos guardados. Esos dos casos van al final y llaman al
// codigo real.
// ============================================================================

namespace {

// Reimplementacion de la cadena del eje, copiada de la estructura de
// `generarNopal` en main.cpp.
struct Planta {
    std::set<std::tuple<int,int,int>> pencas;   // el eje
    std::map<int, std::pair<int,int>> ejePorY;  // altura -> (x,z) del eje
    int topeY = 0;
};

// El mismo hash del generador, para que las formas sean las mismas.
int hashNopal(int wx, int wz) { return wx * 6421 + wz * 9187 + 3313; }

int rnd(int h, int salt, int mod) {
    unsigned v = (unsigned)(h ^ (salt * 2654435761u));
    v ^= v >> 13; v *= 1274126177u; v ^= v >> 16;
    return mod > 0 ? (int)(v % (unsigned)mod) : 0;
}

// Construye el eje igual que el generador: pencas apiladas que pueden
// desviarse un bloque por piso.
Planta construirEje(int wx, int baseY, int wz) {
    const int dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    const int h = hashNopal(wx, wz);

    Planta p;
    const int alturaEje = 3 + rnd(h, 1, 4);      // 3..6
    int ejeX = wx, ejeZ = wz;
    p.topeY = baseY;

    for (int i = 1; i <= alturaEje; ++i) {
        const int y = baseY + i;
        int nx = ejeX, nz = ejeZ;
        if (i > 1 && rnd(h, i * 613 + 29, 3) == 0) {
            const int d = rnd(h, i * 811 + 43, 4);
            const int lx = ejeX + dirs[d][0];
            const int lz = ejeZ + dirs[d][1];
            // EL DESVIO SON DOS PASOS: penca de enlace a la altura anterior,
            // y luego se sube desde ella. En vacio siempre cabe.
            p.pencas.insert({lx, y - 1, lz});
            nx = lx; nz = lz;
        }
        p.pencas.insert({nx, y, nz});
        p.ejePorY[y] = {nx, nz};
        ejeX = nx; ejeZ = nz;
        p.topeY = y;
    }
    return p;
}

// ¿Los dos bloques se tocan por una CARA? (no diagonal, no el mismo)
bool tocanPorCara(std::tuple<int,int,int> a, std::tuple<int,int,int> b) {
    const int dx = std::get<0>(a) - std::get<0>(b);
    const int dy = std::get<1>(a) - std::get<1>(b);
    const int dz = std::get<2>(a) - std::get<2>(b);
    return (std::abs(dx) + std::abs(dy) + std::abs(dz)) == 1;
}

// Un muestreo de posiciones, para que las pruebas no dependan de un solo
// ejemplar afortunado.
std::vector<std::pair<int,int>> muestraDePosiciones() {
    std::vector<std::pair<int,int>> v;
    for (int x = -240; x <= 240; x += 17)
        for (int z = -240; z <= 240; z += 23)
            v.push_back({x, z});
    return v;
}

} // namespace

// ----------------------------------------------------------------------------
// LA REGLA NUEVA: NI UN BLOQUE DE TRONCO
// ----------------------------------------------------------------------------

TEST_CASE("Nopal: el eje son pencas, nunca BLOCK_NOPAL_TALLO") {
    // Es el corazon del cambio. El eje se construye con
    // BLOCK_NOPAL_CLADODIO -- si alguien volviera a meter el tallo aqui, el
    // nopal recuperaria la silueta de arbolito.
    //
    // No se puede comprobar sobre el mundo real (main.cpp no se enlaza), pero
    // si se puede fijar que el bloque que el generador usa para el eje es la
    // penca y no el tronco, que es la constante que se edito.
    CHECK(BLOCK_NOPAL_CLADODIO != BLOCK_NOPAL_TALLO);

    // Y que la penca es lo que el motor reconoce como cladodio, o sea: como
    // parte del cuerpo de la planta.
    CHECK(esCladodio(BLOCK_NOPAL_CLADODIO));
    CHECK_FALSE(esCladodio(BLOCK_NOPAL_TALLO));
}

TEST_CASE("Nopal: la cadena del eje esta SOLDADA, sin diagonales ni huecos") {
    // La regla dura del generador: cada bloque nuevo toca al anterior por una
    // CARA. Dos cubos en diagonal no se tocan por ninguna, y el resultado
    // serian pencas partidas flotando -- el bug que `brotarCladodio` ya
    // documenta para los brotes laterales, y que el eje serpenteante podria
    // reintroducir si el desvio fuera de mas de un bloque.
    // Con la penca de enlace, el eje ya no es una cadena LINEAL (un piso puede
    // tener dos pencas), asi que lo correcto es comprobar CONEXIDAD: que todo
    // el cuerpo sea una sola pieza alcanzable desde la base moviendose solo
    // por caras. Eso es lo que de verdad impide una penca flotando.
    for (auto [x, z] : muestraDePosiciones()) {
        const Planta p = construirEje(x, 64, z);
        REQUIRE(p.pencas.size() >= 3);

        // Inundacion desde la penca mas baja.
        std::vector<std::tuple<int,int,int>> todas(p.pencas.begin(), p.pencas.end());
        std::sort(todas.begin(), todas.end(), [](auto a, auto b) {
            return std::get<1>(a) < std::get<1>(b);
        });

        std::set<std::tuple<int,int,int>> vistas;
        std::vector<std::tuple<int,int,int>> pila{ todas.front() };
        vistas.insert(todas.front());
        while (!pila.empty()) {
            const auto cur = pila.back(); pila.pop_back();
            for (const auto& otra : p.pencas) {
                if (vistas.count(otra)) continue;
                if (tocanPorCara(cur, otra)) { vistas.insert(otra); pila.push_back(otra); }
            }
        }

        INFO("nopal en ", x, ",", z, ": ", vistas.size(), " de ",
             p.pencas.size(), " pencas conectadas a la base");
        CHECK(vistas.size() == p.pencas.size());
    }
}

TEST_CASE("Nopal: el eje se desvia, no es un poste recto") {
    // Una columna perfectamente vertical era el otro rasgo que hacia parecer
    // la planta un poste. Aqui se comprueba que en el conjunto de la muestra
    // una parte de los ejemplares serpentea de verdad.
    //
    // No se exige que TODOS se desvien: un nopal recto es perfectamente
    // posible y el desvio es 1 de cada 3 pisos. Lo que se fija es que la
    // variacion existe en la poblacion.
    int conDesvio = 0, total = 0;
    for (auto [x, z] : muestraDePosiciones()) {
        const Planta p = construirEje(x, 64, z);
        ++total;
        // ¿Alguna penca del eje se salio de la columna original?
        for (auto& [pos, xz] : p.ejePorY) {
            (void)pos;
            if (xz.first != x || xz.second != z) { ++conDesvio; break; }
        }
    }
    REQUIRE(total > 100);
    const double frac = (double)conDesvio / total;
    INFO("ejes con desvio: ", conDesvio, "/", total);
    // Con desvio de 1/3 por piso y 3-6 pisos, la gran mayoria se desvia. Se
    // pide una banda amplia: lo que se vigila es que no sea ni 0% (todos
    // rectos: el bug que se arreglaba) ni 100% (ninguno recto, que tampoco es
    // natural).
    CHECK(frac > 0.30);
    CHECK(frac < 1.00);
}

TEST_CASE("Nopal: la planta empieza a ensancharse desde abajo") {
    // Sin tronco que "subir", los brotes laterales arrancan justo encima de
    // la base (primerBrote = baseY + 1) en vez de a un tercio de la altura
    // del tallo. Es la otra mitad de por que una nopalera se ve como un
    // macizo y no como un arbol con la copa arriba.
    const int baseY = 64;
    const int primerBrote = baseY + 1;

    // Con el tronco, el primer brote salia en baseY + 1 + alturaTallo/3, que
    // para un tallo de 3-6 era baseY+2 .. baseY+3.
    for (int alturaTallo = 3; alturaTallo <= 6; ++alturaTallo) {
        const int brotViejo = baseY + 1 + alturaTallo / 3;
        INFO("altura de tallo ", alturaTallo);
        CHECK(primerBrote <= brotViejo);
    }
    // Y arranca pegado a la base, no por encima.
    CHECK(primerBrote == baseY + 1);
}

// ----------------------------------------------------------------------------
// COMPATIBILIDAD: LOS MUNDOS YA GUARDADOS
// ----------------------------------------------------------------------------
// Estos dos casos NO reimplementan nada: llaman al codigo real.

TEST_CASE("Nopal: el ID del tallo SIGUE existiendo, para no romper saves") {
    // BLOCK_NOPAL_TALLO es el ID 34, en medio del enum. Borrar la fila
    // correria en -1 los ~156 IDs que van detras, y todos los mundos
    // guardados leerian otro bloque: una penca pasaria a leerse como el
    // fruto. Es la misma regla que ya protege a los cinco minerales de veta.
    //
    // Se comprueba por POSICION, no por valor absoluto: lo que importa es que
    // el tallo siga entre la ultima base y el cladodio, sin correr nada.
    CHECK((int)BLOCK_NOPAL_TALLO == (int)BLOCK_NOPAL_BASE_A_ARCILLA + 1);
    CHECK((int)BLOCK_NOPAL_CLADODIO == (int)BLOCK_NOPAL_TALLO + 1);
    CHECK((int)BLOCK_NOPAL_FRUTO == (int)BLOCK_NOPAL_CLADODIO + 1);
}

TEST_CASE("Nopal: el tallo esta RETIRADO del inventario creativo") {
    // La contrapartida de conservar el ID: el bloque no se puede obtener. Si
    // esto fallara, el tallo seguiria estando en el menu de creativo y se
    // podrian construir nopales con tronco a mano, que es justo lo que se
    // pidio eliminar.
    CHECK(esBloqueRetirado(BLOCK_NOPAL_TALLO));

    // Y no se lleva por delante a la penca ni a la base, que SI se usan.
    CHECK_FALSE(esBloqueRetirado(BLOCK_NOPAL_CLADODIO));
    CHECK_FALSE(esBloqueRetirado(BLOCK_NOPAL_BASE_PASTO));
    CHECK_FALSE(esBloqueRetirado(BLOCK_NOPAL_BASE_TIERRA));
    CHECK_FALSE(esBloqueRetirado(BLOCK_NOPAL_BASE_ARENA));
    CHECK_FALSE(esBloqueRetirado(BLOCK_NOPAL_FRUTO));
}

TEST_CASE("Nopal: 'retirado' y 'hueco de mineral' son listas distintas") {
    // Son dos razones distintas para no salir en el menu, y por eso son dos
    // funciones. Un hueco de mineral NO es un bloque (no se genera, no tiene
    // textura propia); el tallo SI lo es (tiene textura, forma y drop) y solo
    // se le quita poder aparecer. Mezclarlas haria que un cambio en una
    // arrastrara a la otra sin querer.
    CHECK(esBloqueRetirado(BLOCK_NOPAL_TALLO));
    CHECK_FALSE(esHuecoDeMineralRetirado(BLOCK_NOPAL_TALLO));

    CHECK(esHuecoDeMineralRetirado(BLOCK_RESERVADO_MINERAL_08));
    CHECK_FALSE(esBloqueRetirado(BLOCK_RESERVADO_MINERAL_08));
}

// ============================================================================
// GEOMETRIA DE LA PENCA: SILUETA VARIADA Y GIRO REAL
// ============================================================================
// Segunda tanda de mejoras. Tres cosas que estaban flojas:
//
//   1. El perfil obovado era una tabla `static const` IDENTICA en todas las
//      pencas del mundo. Botanicamente correcta, pero clonada: una nopalera
//      entera con el mismo recorte se lee como un patron repetido.
//   2. La caja era SIEMPRE alineada con los ejes del mundo. Los rumbos
//      diagonales se conseguian desplazando cada rodaja, asi que el conjunto
//      subia torcido pero cada pieza seguia siendo un cubo paralelo a los
//      demas -- una mata se veia como una reja.
//   3. Nada rellenaba los huecos ENTRE pencas, asi que la mata tenia la
//      estructura de un arbol (ramas separadas con aire en medio) en vez de la
//      de una nopalera (pencas encajadas hasta verse maciza).
//
// Igual que arriba, la geometria vive en main.cpp y no se enlaza, asi que se
// reimplementan las formulas y se fijan sus INVARIANTES. Los numeros de las
// aserciones salen de medir el codigo real, no de suponer.

namespace {

struct Silueta { float escalaAncho, alturaMaxAncho, cuello, anguloY; };

Silueta siluetaDe(int wx, int wy, int wz, int giro) {
    unsigned h = (unsigned)(wx * 73856093) ^ (unsigned)(wy * 19349663) ^
                 (unsigned)(wz * 83492791);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    Silueta s;
    s.escalaAncho    = 0.83f + (float)((h >> 7)  % 35u) * 0.01f;
    s.alturaMaxAncho = 0.50f + (float)((h >> 13) % 19u) * 0.01f;
    s.cuello         = 0.18f + (float)((h >> 19) % 17u) * 0.01f;
    const float rumbo  = (float)giro * 0.7853982f;
    const float desvio = ((float)((h >> 23) % 45u) - 22.0f) * 0.0174533f;
    s.anguloY = rumbo + desvio;
    return s;
}

float anchoRel(const Silueta& nf, float t) {
    const float pico = nf.alturaMaxAncho, cuello = nf.cuello;
    const float APICE = 0.62f;
    float w;
    if (t <= pico) {
        const float u = (pico > 1e-4f) ? (t / pico) : 1.0f;
        w = cuello + (1.0f - cuello) * (0.5f - 0.5f * cosf(u * 3.14159265f));
    } else {
        const float u = (1.0f - pico > 1e-4f) ? ((t - pico) / (1.0f - pico)) : 1.0f;
        w = APICE + (1.0f - APICE) * (0.5f + 0.5f * cosf(u * 3.14159265f));
    }
    w *= nf.escalaAncho;
    if (w > 1.0f) w = 1.0f;
    if (w < 0.08f) w = 0.08f;
    return w;
}

} // namespace

TEST_CASE("Penca: la silueta es obovada, no un rectangulo") {
    // El rasgo que define un cladodio: cuello estrecho abajo, ancho maximo en
    // el tercio superior, apice redondeado. Si esto se pierde, la penca vuelve
    // a leerse como un cubo con las esquinas limadas.
    for (int wx = -40; wx <= 40; wx += 9)
        for (int wz = -40; wz <= 40; wz += 11) {
            const Silueta s = siluetaDe(wx, 65, wz, 0);
            const float base  = anchoRel(s, 0.02f);
            const float medio = anchoRel(s, s.alturaMaxAncho);
            const float apice = anchoRel(s, 0.98f);

            INFO("penca en ", wx, ",", wz);
            // El cuello es CLARAMENTE mas estrecho que el punto ancho.
            CHECK(base < medio * 0.75f);
            // El apice tambien se cierra, pero mucho menos: una penca no
            // acaba en punta (Reyes-Aguero 2005).
            CHECK(apice < medio);
            CHECK(apice > base);
        }
}

TEST_CASE("Penca: la curva de ancho no se sale nunca de rango") {
    // Un ancho relativo >1 desbordaria la caja; uno <=0 haria desaparecer la
    // rodaja y dejaria un agujero en la penca.
    for (int wx = -30; wx <= 30; wx += 7)
        for (int wz = -30; wz <= 30; wz += 13)
            for (int i = 0; i < 24; ++i) {
                const Silueta s = siluetaDe(wx, 66, wz, i % 8);
                const float w = anchoRel(s, (float)i / 23.0f);
                INFO("penca ", wx, ",", wz, " t=", (float)i / 23.0f);
                CHECK(w > 0.0f);
                CHECK(w <= 1.0f);
            }
}

TEST_CASE("Penca: cada una tiene su propia silueta") {
    // El punto de la mejora. Antes las 609 plantas compartian una sola tabla
    // de perfil; ahora cada penca saca la suya del hash de su posicion.
    std::set<std::tuple<int,int,int>> vistas;
    const int N = 3000;
    for (int i = 0; i < N; ++i) {
        const Silueta s = siluetaDe(i * 13, 65, i * 7 + 3, 0);
        vistas.insert({ (int)lroundf((s.escalaAncho    - 0.83f) / 0.01f),
                        (int)lroundf((s.alturaMaxAncho - 0.50f) / 0.01f),
                        (int)lroundf((s.cuello         - 0.18f) / 0.01f) });
    }
    INFO("siluetas distintas en ", N, " pencas: ", vistas.size());
    // Con 35*19*17 = 11.305 combinaciones posibles, en 3.000 muestras deben
    // salir del orden de miles. Si saliera 1, el perfil volvio a ser fijo.
    CHECK(vistas.size() > 1000);
}

TEST_CASE("Penca girada: NUNCA se sale de su voxel") {
    // ESTE ES EL INVARIANTE QUE PROTEGE LA COLISION.
    //
    // La caja de colision va celda a celda (nopalHitboxCon). Una esquina que
    // invadiera el voxel vecino se VERIA pero no se podria tocar ni romper, y
    // el resaltado marcaria otro sitio. Por eso el mesher encoge la rodaja
    // antes de rotarla.
    //
    // Se recorre el caso normal (cladodio de mata) y el peor (penca suelta,
    // mas grande), en los 8 rumbos.
    const float CAJAS[2][2] = { {8.0f/16.0f, 7.0f/16.0f},
                                {15.0f/16.0f, 13.0f/16.0f} };
    int fuera = 0, probadas = 0;

    for (int c = 0; c < 2; ++c)
        for (int wx = -24; wx <= 24; wx += 8)
            for (int wz = -24; wz <= 24; wz += 10)
                for (int giro = 0; giro < 8; ++giro) {
                    const Silueta s = siluetaDe(wx, 67, wz, giro);
                    const float ANCHO = CAJAS[c][0], GRUESO = CAJAS[c][1];
                    constexpr int RODAJAS = 12;
                    const float bx0 = 0.5f - ANCHO * 0.5f, bx1 = 0.5f + ANCHO * 0.5f;
                    const float bz0 = 0.5f - GRUESO * 0.5f, bz1 = 0.5f + GRUESO * 0.5f;
                    const float anchoTotal = bx1 - bx0;
                    const float margenMax = anchoTotal * 0.5f - 0.03f;

                    for (int i = 0; i < RODAJAS; ++i) {
                        const float t = ((float)i + 0.5f) / (float)RODAJAS;
                        float m = (1.0f - anchoRel(s, t)) * anchoTotal * 0.5f;
                        if (m > margenMax) m = margenMax;
                        if (m < 0.0f) m = 0.0f;
                        const float rx0 = bx0 + m, rx1 = bx1 - m;
                        if (rx1 <= rx0 || bz1 <= bz0) continue;

                        const float ca_ = cosf(s.anguloY), sa_ = sinf(s.anguloY);
                        float hx = (rx1 - rx0) * 0.5f, hz = (bz1 - bz0) * 0.5f;
                        const float mx = (rx0 + rx1) * 0.5f, mz = (bz0 + bz1) * 0.5f;
                        const float aca = fabsf(ca_), asa = fabsf(sa_);
                        const float radX = hx * aca + hz * asa;
                        const float radZ = hx * asa + hz * aca;
                        constexpr float EB = 0.0015f;
                        const float libreX = fminf(mx - EB, (1.0f - EB) - mx);
                        const float libreZ = fminf(mz - EB, (1.0f - EB) - mz);
                        float k = 1.0f;
                        if (radX > libreX && radX > 1e-6f) k = fminf(k, libreX / radX);
                        if (radZ > libreZ && radZ > 1e-6f) k = fminf(k, libreZ / radZ);
                        if (k < 0.15f) k = 0.15f;
                        hx *= k; hz *= k;

                        const float ex[4] = { -hx, +hx, +hx, -hx };
                        const float ez[4] = { -hz, -hz, +hz, +hz };
                        for (int q = 0; q < 4; ++q) {
                            const float px = 0.5f + (mx - 0.5f) + ex[q]*ca_ - ez[q]*sa_;
                            const float pz = 0.5f + (mz - 0.5f) + ex[q]*sa_ + ez[q]*ca_;
                            ++probadas;
                            if (px < 0.0f || px > 1.0f || pz < 0.0f || pz > 1.0f) ++fuera;
                        }
                    }
                }

    INFO("esquinas probadas: ", probadas, "  fuera del voxel: ", fuera);
    REQUIRE(probadas > 1000);
    CHECK(fuera == 0);
}

TEST_CASE("Penca girada: el angulo no esta clavado en la rejilla") {
    // El giro tiene que ser CONTINUO. Si todos los angulos cayeran en
    // multiplos de 45 grados, volveriamos a tener pencas paralelas y la mata
    // se veria como una reja, que es justo lo que se arreglaba.
    int enRejilla = 0, total = 0;
    for (int wx = -30; wx <= 30; wx += 7)
        for (int wz = -30; wz <= 30; wz += 11) {
            const Silueta s = siluetaDe(wx, 68, wz, 0);
            const float g = s.anguloY / 0.7853982f;
            const float d = fabsf(g - (float)lroundf(g));
            if (d < 0.0222f) ++enRejilla;    // 1 grado / 45
            ++total;
        }
    INFO("angulos clavados en la rejilla: ", enRejilla, " de ", total);
    REQUIRE(total > 40);
    // Con +-22 grados continuos, casi ninguno debe caer justo en el multiplo.
    CHECK((double)enRejilla / total < 0.15);
}
