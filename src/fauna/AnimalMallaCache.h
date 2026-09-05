#ifndef ANIMAL_MALLA_CACHE_H
#define ANIMAL_MALLA_CACHE_H

#include "AnimalMalla.h"
#include "PecariAnatomia.h"
#include "PecariEsqueleto.h"   // pesos de hueso: se calculan al generar
#include <cstdint>
#include <cmath>
#include <vector>
// deque y no vector para las entradas de la cache: sus elementos no se mueven
// al crecer, y esta clase ENTREGA PUNTEROS a ellos (ver la nota de `entradas`).
#include <deque>

// ============================================================================
// CACHE DE MALLAS COMPARTIDAS
// ============================================================================
// RESPONSABILIDAD UNICA: garantizar que N animales del mismo tipo compartan UNA
// malla, no N copias.
//
// Es el punto 24 y el 27, y es lo que hace que la memoria sea O(tipos) en vez
// de O(individuos):
//
//   SIN cache:  100 pecaries x ~90 KB = 9 MB
//   CON cache:  5 variantes x 4 LOD x ~90 KB = 1.8 MB, sea cual sea el numero
//               de animales vivos.
//
// Cada instancia solo guarda su transformacion y unos pocos parametros: unos
// 60 bytes. Cien pecaries anaden 6 KB, no 9 MB.
//
// ----------------------------------------------------------------------------
// POR QUE LA CLAVE NO ES EL PUNTERO A LOS PARAMETROS
// ----------------------------------------------------------------------------
// Dos animales con los MISMOS parametros deben compartir malla aunque sus
// structs sean objetos distintos. Por eso la clave se construye a partir de lo
// que de verdad cambia la geometria: especie, etapa vital y LOD.
//
// La semilla individual NO entra en la clave: solo afecta al jaspeado del
// color, y ese matiz no justifica duplicar la malla entera. Es exactamente el
// compromiso que pide el punto 26: variacion sin duplicar geometria.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// ESPECIE
// ----------------------------------------------------------------------------
// Enumerada para que la clave de cache sea un entero pequeno y para dejar
// preparada la arquitectura reutilizable del punto 49.
enum class EspecieAnimal : uint8_t {
    PECARI_COLLAR = 0,
    PECARI_LABIADO,
    PECARI_CHACO,
    _COUNT
};

// ----------------------------------------------------------------------------
// CLAVE DE CACHE
// ----------------------------------------------------------------------------
struct ClaveMalla {
    EspecieAnimal especie;
    uint8_t       etapa;    // 0..4
    uint8_t       lod;      // 0..3

    bool operator==(const ClaveMalla& o) const {
        return especie == o.especie && etapa == o.etapa && lod == o.lod;
    }
};

// ============================================================================
// LA CACHE
// ============================================================================
class CacheMallas {
private:
    struct Entrada {
        ClaveMalla  clave;
        MallaAnimal malla;
        // ⭐ LOS PESOS DE HUESO VIVEN AQUI, junto a su malla.
        //
        // Van en la MISMA entrada a proposito: solo tienen sentido para los
        // vertices de esta malla concreta, asi que compartir su vida es lo
        // unico que garantiza que no puedan descuadrarse ni quedar colgando
        // por separado. Y se calculan UNA VEZ al generar, no por frame.
        std::vector<PesoVertice> pesos;
        bool        valida = false;
    };

    // ========================================================================
    // ⚠️ std::deque, NO std::vector. AQUI ESTABA EL CRASH.
    // ========================================================================
    // Esto era un `std::vector<Entrada>`, y `obtener()` devuelve PUNTEROS a
    // sus elementos (`&e.malla`). El comentario de esa funcion prometia que
    // "el puntero es estable mientras la cache no se vacie".
    //
    // ERA FALSO. Un vector REASIGNA al crecer: cuando push_back se queda sin
    // capacidad, mueve todos los elementos a memoria nueva y libera la vieja.
    // En ese instante, TODOS los punteros entregados antes quedan colgando.
    //
    // Y esos punteros no se usan al momento: se guardan en las
    // InstanciaDibujo (ver PecariMundo.h) y se dereferencian mas tarde, al
    // dibujar. Asi que la secuencia mortal era:
    //
    //   1. Se prepara el dibujo de la manada: cada animal pide su malla y
    //      guarda el puntero en su instancia.
    //   2. Uno de ellos tiene una combinacion nueva (otra etapa, otro LOD) ->
    //      push_back -> el vector se reasigna.
    //   3. Los punteros de los animales YA PROCESADOS apuntan a memoria
    //      liberada.
    //   4. El render los dereferencia. SIGSEGV.
    //
    // Eso explica todo lo observado: que fuera INTERMITENTE (solo peta cuando
    // el push_back toca reasignar, no siempre), que ocurriera al soltar una
    // manada nueva (combinaciones nuevas de etapa) y tambien al cargar el
    // mundo (la cache se llena de golpe).
    //
    // deque NO reasigna: crece por bloques y las referencias a los elementos
    // existentes siguen siendo validas para siempre. Es exactamente la
    // garantia que esta clase estaba prometiendo sin darla.
    //
    // El coste es nulo aqui: la busqueda sigue siendo lineal sobre <=128
    // entradas, y con 60 combinaciones reales cabe en uno o dos bloques.
    std::deque<Entrada> entradas;

    // Contadores para poder REPORTAR el coste real en vez de estimarlo.
    mutable size_t aciertos = 0;
    mutable size_t fallos   = 0;

public:
    // Tope de seguridad. Si se superara, algo esta generando claves que no
    // deberian existir.
    static constexpr size_t MAX_ENTRADAS = 128;

    // ------------------------------------------------------------------------
    // OBTENER UNA MALLA
    // ------------------------------------------------------------------------
    // Devuelve la malla compartida para esa combinacion, generandola solo la
    // primera vez.
    //
    // ⭐ EL PUNTERO ES ESTABLE mientras la cache no se vacie, Y AHORA ES
    // VERDAD: las entradas viven en un deque, que no reubica lo que ya tiene
    // al crecer.
    //
    // Esta promesa estaba escrita aqui cuando el contenedor era un vector, y
    // era falsa: cada push_back que agotaba la capacidad movia todas las
    // entradas y dejaba colgando los punteros ya entregados. Como quien los
    // recibe los GUARDA para dibujar despues (InstanciaDibujo::malla), el
    // resultado era un SIGSEGV intermitente al aparecer fauna nueva.
    //
    // Si alguien vuelve a cambiar el contenedor, tiene que conservar esta
    // garantia o cambiar la firma para que no entregue punteros.
    const MallaAnimal* obtener(EspecieAnimal especie, int etapa, int lod) {
        if (etapa < 0) etapa = 0;
        if (etapa > 4) etapa = 4;
        if (lod < 0) lod = 0;
        if (lod > 3) lod = 3;

        const ClaveMalla clave{ especie, (uint8_t)etapa, (uint8_t)lod };

        for (const Entrada& e : entradas) {
            if (e.valida && e.clave == clave) {
                ++aciertos;
                return &e.malla;
            }
        }

        if (entradas.size() >= MAX_ENTRADAS) return nullptr;

        ++fallos;
        entradas.push_back(Entrada{});
        Entrada& nueva = entradas.back();
        nueva.clave = clave;

        ParametrosPecari p = parametrosDe(especie);
        p = Especies::aplicarEdad(p, etapa);

        // La semilla de la MALLA es fija por clave: la variacion individual se
        // aplica al dibujar, no aqui. Si dependiera del individuo, cada animal
        // generaria su propia malla y se perderia todo el ahorro.
        p.semilla = 1000u + (uint32_t)etapa * 17u + (uint32_t)lod * 131u;

        ConstructorPecari::generar(nueva.malla, p, lod);

        // Los pesos de hueso, derivados de la posicion de cada vertice. Se
        // calculan aqui --una vez por (especie, etapa, LOD)-- y no por frame
        // ni por animal: son los mismos para todos los que compartan la malla.
        CalcularPesosPecari(nueva.malla, p, nueva.pesos);

        nueva.valida = true;

        return &nueva.malla;
    }

    // ------------------------------------------------------------------------
    // LOS PESOS DE UNA MALLA
    // ------------------------------------------------------------------------
    // Devuelve nullptr si esa combinacion no esta cacheada todavia: el
    // llamador debe pedir antes la malla con obtener(), que es quien la crea.
    //
    // El puntero es estable por la misma razon que el de la malla: las
    // entradas viven en un deque, que NO reasigna al crecer. Ver la nota de
    // `entradas` sobre el crash que eso provocaba.
    const std::vector<PesoVertice>* pesosDe(EspecieAnimal especie,
                                            int etapa, int lod) const {
        if (etapa < 0) etapa = 0;
        if (etapa > 4) etapa = 4;
        if (lod < 0) lod = 0;
        if (lod > 3) lod = 3;

        const ClaveMalla clave{ especie, (uint8_t)etapa, (uint8_t)lod };
        for (const Entrada& e : entradas) {
            if (e.valida && e.clave == clave) return &e.pesos;
        }
        return nullptr;
    }

    // ------------------------------------------------------------------------
    // PRECALENTAR
    // ------------------------------------------------------------------------
    // Genera por adelantado las combinaciones que se van a usar seguro, para
    // que no haya un tiron la primera vez que aparece un animal.
    void precalentar(EspecieAnimal especie) {
        for (int etapa = 0; etapa <= 4; ++etapa) {
            for (int lod = 0; lod <= 3; ++lod) {
                obtener(especie, etapa, lod);
            }
        }
    }

    // ------------------------------------------------------------------------
    // COSTE REAL
    // ------------------------------------------------------------------------
    // Existe para poder reportar memoria MEDIDA en vez de estimada.
    size_t bytesTotales() const {
        size_t total = 0;
        for (const Entrada& e : entradas) {
            if (e.valida) total += e.malla.bytes();
        }
        return total;
    }

    size_t mallasVivas() const {
        size_t n = 0;
        for (const Entrada& e : entradas) if (e.valida) ++n;
        return n;
    }

    size_t trianglesTotales() const {
        size_t n = 0;
        for (const Entrada& e : entradas) {
            if (e.valida) n += e.malla.triangulos();
        }
        return n;
    }

    size_t numAciertos() const { return aciertos; }
    size_t numFallos()   const { return fallos; }

    void vaciar() {
        entradas.clear();
        aciertos = fallos = 0;
    }

    static ParametrosPecari parametrosDe(EspecieAnimal e) {
        switch (e) {
            case EspecieAnimal::PECARI_LABIADO: return Especies::pecariLabiado();
            case EspecieAnimal::PECARI_CHACO:   return Especies::pecariDelChaco();
            default:                            return Especies::pecariDeCollar();
        }
    }
};

// ============================================================================
// SELECCION DE LOD
// ============================================================================
// El punto 17 y el 42: el detalle depende de la distancia.
namespace LOD {

    // Umbrales en BLOQUES del motor (0.60 m cada uno).
    //
    // Se eligieron contra la distancia de carga de chunks del motor
    // (RENDER_DISTANCE=5 x CHUNK_SIZE=16 = 80 bloques) y contra la niebla, que
    // se vuelve opaca a 78.4 bloques. No tiene sentido gastar detalle en algo
    // que la niebla ya esta ocultando.
    constexpr float DIST_LOD0 = 12.0f;   // 7.2 m: el animal llena la pantalla
    constexpr float DIST_LOD1 = 30.0f;   // 18 m
    constexpr float DIST_LOD2 = 55.0f;   // 33 m
    // mas alla: LOD3

    // HISTERESIS. Sin ella, un animal justo en el umbral oscila entre dos
    // niveles cada frame y cuesta MAS que si estuviera siempre en el alto.
    // Es una de las reglas de oro del LOD que exige 01_ARQUITECTURA.
    constexpr float HISTERESIS = 3.0f;

    inline int seleccionar(float distanciaBloques, int lodActual) {
        // Al SUBIR de nivel (acercarse) se usa el umbral tal cual; al BAJAR
        // (alejarse) se exige pasar el umbral mas la histeresis.
        auto umbral = [&](float base, bool subiendo) {
            return subiendo ? base : (base + HISTERESIS);
        };

        int nuevo;
        if (distanciaBloques < umbral(DIST_LOD0, lodActual > 0)) nuevo = 0;
        else if (distanciaBloques < umbral(DIST_LOD1, lodActual > 1)) nuevo = 1;
        else if (distanciaBloques < umbral(DIST_LOD2, lodActual > 2)) nuevo = 2;
        else nuevo = 3;

        return nuevo;
    }

    // ------------------------------------------------------------------------
    // TRANSICION SIN POPPING
    // ------------------------------------------------------------------------
    // El punto 18 pide dithering o crossfade.
    //
    // El pipeline fijo no tiene shaders para hacer un crossfade real, pero SI
    // tiene alpha test. Se devuelve un factor 0..1 de cuanto ha progresado la
    // transicion, y quien dibuja puede usarlo para un dithering por alpha.
    //
    // Devuelve 1.0 cuando no hay transicion en curso.
    inline float factorTransicion(float distanciaBloques, int lod) {
        float base = 0.0f;
        switch (lod) {
            case 0: base = DIST_LOD0; break;
            case 1: base = DIST_LOD1; break;
            case 2: base = DIST_LOD2; break;
            default: return 1.0f;
        }
        const float d = distanciaBloques - (base - HISTERESIS);
        if (d <= 0.0f) return 1.0f;
        if (d >= HISTERESIS) return 0.0f;
        return 1.0f - (d / HISTERESIS);
    }
}

// ============================================================================
// CULLING
// ============================================================================
// El punto 29. Lo minimo util sin depender de la matriz de proyeccion del
// motor: descarte por distancia y por semiespacio de camara.
namespace Culling {

    // Descarte por distancia. El mas barato y el que mas quita.
    inline bool visiblePorDistancia(float dx, float dz, float maxBloques) {
        return (dx*dx + dz*dz) <= (maxBloques * maxBloques);
    }

    // Descarte por angulo: si el animal esta claramente detras de la camara,
    // no se dibuja.
    //
    // Se usa un margen generoso (dot > -0.35 en vez de > 0) porque el frustum
    // real es mas ancho que el eje de vision, y recortar de mas produce
    // animales que desaparecen por el borde de la pantalla.
    inline bool visiblePorAngulo(float dx, float dz,
                                 float dirCamX, float dirCamZ) {
        const float len = std::sqrt(dx*dx + dz*dz);
        if (len < 1e-4f) return true;   // encima de la camara: visible
        const float dot = (dx * dirCamX + dz * dirCamZ) / len;
        return dot > -0.35f;
    }
}

} // namespace Fauna

#endif // ANIMAL_MALLA_CACHE_H
