#pragma once

#include "BlockType.h"
#include <cstdint>

// ============================================================================
// BLOQUES COMPUESTOS: VARIAS COSAS Y UN ESTADO DENTRO DE UN SOLO VOXEL
// ============================================================================
// Un voxel del motor guarda UN BlockType y nada mas: no hay metadatos, ni
// tile entities, ni una tabla lateral. Eso hacia imposible un maguey que
// "tiene 7 de 15 de aguamiel, esta en la etapa 4 y le quedan tres puntas".
//
// Este archivo resuelve eso SIN tocar el formato de guardado, apoyandose en un
// hecho del motor: BlockType se serializa como 4 BYTES (32 bits) y solo se
// usan los primeros ~168 valores. Sobran miles de millones.
//
// Asi que el estado se mete DENTRO del ID:
//
//     ID = COMPUESTO_BASE + familia * ESTADOS_POR_FAMILIA + estado
//
// Es la misma tecnica que el motor YA usa para las celdas mixtas
// (BLOCK_MIXTO_BASE + base*7*FAMILIAS + relleno), aqui generalizada.
//
// ----------------------------------------------------------------------------
// LO QUE SE GANA
// ----------------------------------------------------------------------------
//   - PERSISTENCIA GRATIS. El estado viaja en el ID, asi que el save, la
//     paleta del chunk y la carga funcionan sin cambiar una linea. No hay
//     formato nuevo que versionar ni migracion que escribir.
//   - CERO MEMORIA EXTRA. No hay mapa lateral que llenar ni purgar al
//     descargar un chunk (una fuga que el motor ya sufre con waterLevels).
//   - CERO ENTIDADES. Las puntas de un maguey no son objetos: son un campo
//     de bits que el mesher lee al construir la malla.
//
// ----------------------------------------------------------------------------
// LO QUE CUESTA
// ----------------------------------------------------------------------------
//   - Los campos son ACOTADOS: lo que quepa en sus bits. Para "cuanta
//     aguamiel" o "que etapa de crecimiento" sobra; para un float con
//     decimales, no. Cuando haga falta un campo continuo, se cuantiza.
//
// ----------------------------------------------------------------------------
// GENERICO, NO DEL MAGUEY
// ----------------------------------------------------------------------------
// El maguey es la primera familia, no el motivo del diseño. La misma
// estructura vale para arbol+frutos, piedra+musgo, tronco+hongos o
// planta+flores: cada una declara sus componentes y el reparto de bits de su
// estado, y hereda gratis el raycast por componente, el guardado y el render.
// ============================================================================

namespace Compuesto {

// ----------------------------------------------------------------------------
// EL ESPACIO DE IDs
// ----------------------------------------------------------------------------
// Va MUY por encima del enum y del rango de las celdas mixtas, para que no
// pueda pisar un bloque real ni ahora ni cuando el enum crezca.
//
// Las mixtas ocupan [1000, 1000 + 22*154) = [1000, 4388). Se arranca en
// 100.000 con holgura de sobra.
constexpr int COMPUESTO_BASE = 100000;

// Cuantos estados distintos puede tener una familia. 16 bits = 65.536
// combinaciones, que es lo que dan los campos de abajo con margen para crecer.
constexpr int BITS_ESTADO        = 16;
constexpr int ESTADOS_POR_FAMILIA = 1 << BITS_ESTADO;

// Familias registradas. Añadir una es añadir una linea AL FINAL: insertar en
// medio correria los IDs y los mundos guardados leerian otra planta.
enum Familia : int {
    FAM_MAGUEY = 0,     // cuerpo + puntas + aguamiel

    // --- Preparadas, aun sin implementar ---
    // Se declaran ya para reservarles su hueco de IDs: hacerlo despues
    // obligaria a insertar en medio, que es justo lo que corrompe saves.
    FAM_ARBOL_FRUTO,    // tronco/rama + frutos
    FAM_PIEDRA_MUSGO,   // roca + musgo
    FAM_PLANTA_FLOR,    // mata + flores

    // La BIZNAGA: cactus de barril del desierto mexicano. Crece por etapas,
    // como el maguey, pero es una bola y no una roseta.
    FAM_BIZNAGA,

    FAM_COUNT
};

constexpr int COMPUESTO_FIN =
    COMPUESTO_BASE + FAM_COUNT * ESTADOS_POR_FAMILIA;

// ¿Este ID es un bloque compuesto?
inline bool esCompuesto(BlockType t) {
    return (int)t >= COMPUESTO_BASE && (int)t < COMPUESTO_FIN;
}

// Componer y descomponer.
inline BlockType hacer(Familia f, uint16_t estado) {
    return (BlockType)(COMPUESTO_BASE + (int)f * ESTADOS_POR_FAMILIA + estado);
}
inline Familia familiaDe(BlockType t) {
    if (!esCompuesto(t)) return FAM_COUNT;
    return (Familia)(((int)t - COMPUESTO_BASE) / ESTADOS_POR_FAMILIA);
}
inline uint16_t estadoDe(BlockType t) {
    if (!esCompuesto(t)) return 0;
    return (uint16_t)(((int)t - COMPUESTO_BASE) % ESTADOS_POR_FAMILIA);
}

// ----------------------------------------------------------------------------
// EMPAQUETADO DE CAMPOS
// ----------------------------------------------------------------------------
// Un campo es (desplazamiento, ancho). Leer y escribir pasa SIEMPRE por estas
// dos funciones para que ningun sitio del motor se invente su propia
// aritmetica de bits -- que es como se acaba con dos versiones que no
// coinciden.
struct Campo {
    uint8_t desplaz;   // desde que bit
    uint8_t ancho;     // cuantos bits

    constexpr uint16_t mascara() const {
        return (uint16_t)(((1u << ancho) - 1u) << desplaz);
    }
    constexpr uint16_t maximo() const {
        return (uint16_t)((1u << ancho) - 1u);
    }
};

inline uint16_t leer(uint16_t estado, Campo c) {
    return (uint16_t)((estado & c.mascara()) >> c.desplaz);
}

inline uint16_t escribir(uint16_t estado, Campo c, uint16_t valor) {
    if (valor > c.maximo()) valor = c.maximo();   // se acota, no se desborda
    return (uint16_t)((estado & ~c.mascara()) |
                      ((uint16_t)(valor << c.desplaz) & c.mascara()));
}

// ============================================================================
// FAMILIA: MAGUEY
// ============================================================================
// Reparto de los 16 bits del estado. El total no puede pasar de 16; si algun
// dia hace falta mas, se sube BITS_ESTADO (y con el ESTADOS_POR_FAMILIA), lo
// que corre los IDs de las familias siguientes -- asi que hacerlo exige
// migrar, igual que insertar una familia en medio.
namespace Maguey {

    // Etapa de crecimiento. Cinco etapas reales + margen.
    //   0 brote | 1 joven | 2 adulto | 3 maduro | 4 productor
    constexpr Campo ETAPA     = { 0, 3 };   // 0..7

    // Giro de la planta: cuatro orientaciones. Es lo que evita que un campo
    // de magueyes se vea como copias calcadas del mismo modelo.
    constexpr Campo GIRO      = { 3, 2 };   // 0..3

    // Puntas que le quedan. NO es un bloque por combinacion: es un contador,
    // y el mesher decide donde va cada una a partir del hash de la posicion.
    constexpr Campo PUNTAS    = { 5, 3 };   // 0..7

    // Aguamiel acumulada, en dieciseisavos de la capacidad. Sube GRADUALMENTE
    // con el tiempo; no aparece de golpe.
    constexpr Campo AGUAMIEL  = { 8, 4 };   // 0..15

    // ¿Esta capado? Un maguey sin capar no produce aunque sea maduro: hay que
    // abrirlo primero, que es como se hace de verdad.
    constexpr Campo CAPADO    = { 12, 1 };  // 0..1

    // Quedan libres los bits 13..15 para lo que venga.

    // --- Etapas con nombre, para no repartir numeros magicos por el motor ---
    enum Etapa : uint16_t {
        BROTE = 0, JOVEN = 1, ADULTO = 2, MADURO = 3, PRODUCTOR = 4
    };

    // Capacidad maxima de aguamiel segun la etapa. Un maguey joven no da
    // nada; solo el productor llena del todo.
    inline uint16_t capacidad(uint16_t etapa) {
        switch (etapa) {
            case PRODUCTOR: return 15;   // el tope del campo
            case MADURO:    return 8;
            default:        return 0;    // aun no produce
        }
    }

    // ¿Puede producir aguamiel ahora mismo?
    // Hacen falta las dos cosas: estar en edad Y estar capado.
    inline bool produce(uint16_t estado) {
        return leer(estado, CAPADO) != 0 &&
               capacidad(leer(estado, ETAPA)) > 0;
    }

    // ¿Hay bastante para llenar un tazon? Se pide la mitad de la capacidad de
    // un productor: menos que eso es un fondo que no merece la pena.
    constexpr uint16_t AGUAMIEL_PARA_TAZON = 8;

    inline bool hayParaTazon(uint16_t estado) {
        return leer(estado, AGUAMIEL) >= AGUAMIEL_PARA_TAZON;
    }

    // Cuantas puntas le tocan a cada etapa al generarse.
    //
    // ⭐ SOLO LOS GRANDES TIENEN PUNTA.
    //
    // El brote y el joven llevaban 2 y 3 espinas, y no debe ser asi: la
    // punta gruesa es lo que distingue a un maguey HECHO. Un maguey pequeno
    // con espinas se veia igual que uno adulto en miniatura, y ademas
    // rompia la regla de reconocerlos de un vistazo por la punta.
    //
    // Ahora la punta aparece a partir del ADULTO y va a mas con la edad.
    inline uint16_t puntasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case BROTE:  return 0;    // sin punta: es un brote
            case JOVEN:  return 0;    // sin punta todavia
            case ADULTO: return 5;
            default:     return 6;    // maduro y productor
        }
    }

    // Escala del modelo por etapa: lo que mide respecto a un maguey mediano.
    // Es lo que hace que un productor se reconozca de lejos.
    inline float escalaDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case BROTE:     return 0.45f;
            case JOVEN:     return 0.80f;
            case ADULTO:    return 1.30f;
            case MADURO:    return 1.80f;
            default:        return 2.20f;   // productor: el mas grande
        }
    }

    // --- Construir un maguey ---
    inline BlockType crear(uint16_t etapa, uint16_t giro,
                           uint16_t puntas, uint16_t aguamiel,
                           bool capado) {
        uint16_t e = 0;
        e = escribir(e, ETAPA,    etapa);
        e = escribir(e, GIRO,     giro);
        e = escribir(e, PUNTAS,   puntas);
        e = escribir(e, AGUAMIEL, aguamiel);
        e = escribir(e, CAPADO,   capado ? 1 : 0);
        return hacer(FAM_MAGUEY, e);
    }

    // Un maguey recien generado: sin capar y sin jugo, con las puntas que le
    // tocan por edad.
    inline BlockType nuevo(uint16_t etapa, uint16_t giro) {
        return crear(etapa, giro, puntasDeEtapa(etapa), 0, false);
    }

    // --- Leer campos de un bloque ya hecho ---
    inline uint16_t etapaDe(BlockType t)    { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)     { return leer(estadoDe(t), GIRO); }
    inline uint16_t puntasDe(BlockType t)   { return leer(estadoDe(t), PUNTAS); }
    inline uint16_t aguamielDe(BlockType t) { return leer(estadoDe(t), AGUAMIEL); }
    inline bool     capadoDe(BlockType t)   { return leer(estadoDe(t), CAPADO) != 0; }

    // --- Devolver un bloque nuevo con un campo cambiado ---
    // Se devuelve otro ID en vez de mutar: los bloques son valores, y asi el
    // llamador decide cuando escribirlo en el mundo.
    inline BlockType conCampo(BlockType t, Campo c, uint16_t v) {
        return hacer(FAM_MAGUEY, escribir(estadoDe(t), c, v));
    }

    inline BlockType conAguamiel(BlockType t, uint16_t v) {
        return conCampo(t, AGUAMIEL, v);
    }
    inline BlockType capar(BlockType t) {
        // Capar tambien le quita una punta: es la que se corta para abrirlo.
        const uint16_t p = puntasDe(t);
        BlockType r = conCampo(t, CAPADO, 1);
        if (p > 0) r = conCampo(r, PUNTAS, (uint16_t)(p - 1));
        return r;
    }
    inline BlockType vaciado(BlockType t) {
        return conAguamiel(t, 0);
    }
    inline BlockType conEtapa(BlockType t, uint16_t etapa) {
        BlockType r = conCampo(t, ETAPA, etapa);
        // Al crecer le salen las puntas que le tocan, si no le quitaron mas.
        const uint16_t p = puntasDeEtapa(etapa);
        if (puntasDe(r) < p && !capadoDe(r)) r = conCampo(r, PUNTAS, p);
        return r;
    }
    inline BlockType conPuntas(BlockType t, uint16_t p) {
        return conCampo(t, PUNTAS, p);
    }

} // namespace Maguey

// ============================================================================
// FAMILIA: BIZNAGA
// ============================================================================
// El cactus de barril del desierto mexicano (Ferocactus, Echinocactus). Una
// bola achatada y acanalada, cubierta de espinas, que crece MUY despacio: los
// ejemplares grandes tienen decadas.
//
// Se modela con el mismo sistema que el maguey, pero es mucho mas simple: no
// produce nada, no se capa. Solo crece.
namespace Biznaga {

    // Etapa de crecimiento. Tres etapas reales, como se pidio.
    //   0 pequena | 1 mediana | 2 adulta
    constexpr Campo ETAPA = { 0, 2 };   // 0..3 (sobra una)

    // Giro: cuatro orientaciones para que un grupo no parezca calcado.
    constexpr Campo GIRO  = { 2, 2 };   // 0..3

    // Cuantas costillas tiene. Una biznaga real tiene entre 13 y 21, y el
    // numero es constante durante toda su vida: se fija al brotar.
    constexpr Campo COSTILLAS = { 4, 3 };   // 0..7 -> se mapea a 13..20

    // Quedan libres los bits 7..15.

    enum Etapa : uint16_t {
        PEQUENA = 0, MEDIANA = 1, ADULTA = 2
    };

    // Cuanto mide cada etapa, en fraccion de bloque. Una biznaga adulta de
    // verdad pasa del metro de diametro, asi que llena casi el voxel.
    inline float escalaDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case PEQUENA: return 0.35f;
            case MEDIANA: return 0.62f;
            default:      return 0.92f;   // adulta
        }
    }

    // Costillas de verdad, a partir del campo de 3 bits.
    inline int costillasDe(uint16_t v) { return 13 + (int)(v % 8u); }

    // --- Construir ---
    inline BlockType crear(uint16_t etapa, uint16_t giro, uint16_t costillas) {
        uint16_t e = 0;
        e = escribir(e, ETAPA,     etapa);
        e = escribir(e, GIRO,      giro);
        e = escribir(e, COSTILLAS, costillas);
        return hacer(FAM_BIZNAGA, e);
    }

    inline BlockType nuevo(uint16_t etapa, uint16_t giro, uint16_t costillas) {
        return crear(etapa, giro, costillas);
    }

    // --- Leer ---
    inline uint16_t etapaDe(BlockType t) { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)  { return leer(estadoDe(t), GIRO); }
    inline uint16_t costillasCampoDe(BlockType t) {
        return leer(estadoDe(t), COSTILLAS);
    }
    inline int costillasReales(BlockType t) {
        return costillasDe(costillasCampoDe(t));
    }

    inline bool esBiznaga(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_BIZNAGA;
    }

    // --- Crecer ---
    // Devuelve la biznaga una etapa mas vieja, o la misma si ya es adulta.
    inline BlockType crecida(BlockType t) {
        const uint16_t e = etapaDe(t);
        if (e >= ADULTA) return t;
        uint16_t est = estadoDe(t);
        est = escribir(est, ETAPA, (uint16_t)(e + 1));
        return hacer(FAM_BIZNAGA, est);
    }

    // ¿Ya no puede crecer mas?
    inline bool estaHecha(BlockType t) { return etapaDe(t) >= ADULTA; }

} // namespace Biznaga

// ============================================================================
// COMPONENTES: LAS PARTES DE UN BLOQUE COMPUESTO
// ============================================================================
// Cada familia declara de que partes se compone. El raycast, la rotura y el
// render preguntan por INDICE y no saben de que planta se trata: eso es lo que
// hace el sistema reutilizable.
//
// Un componente NO es un bloque ni una entidad: es una etiqueta con una caja.
enum Componente : int {
    COMP_CUERPO = 0,   // el modelo principal (la roseta, el tronco, la roca)
    COMP_ADORNO = 1,   // lo que sobresale (puntas, frutos, musgo, flores)
    COMP_FLUIDO = 2,   // lo que se acumula dentro (aguamiel, resina, agua)
    COMP_MAX    = 3
};

// Cuantos componentes tiene ESTE bloque ahora mismo. Depende del estado: un
// maguey sin puntas no tiene componente de adorno, y uno sin jugo no tiene
// fluido, asi que el raycast no prueba cajas que no existen.
inline int componentesDe(BlockType t) {
    if (!esCompuesto(t)) return 1;

    switch (familiaDe(t)) {
        case FAM_MAGUEY: {
            int n = 1;                                   // el cuerpo, siempre
            if (Maguey::puntasDe(t) > 0)   n = 2;        // + puntas
            if (Maguey::aguamielDe(t) > 0) n = 3;        // + jugo
            return n;
        }
        default:
            return 1;
    }
}

// ¿Que componente es el indice i?
inline Componente componenteN(BlockType t, int i) {
    if (i < 0 || i >= componentesDe(t)) return COMP_CUERPO;
    return (Componente)i;
}

// ¿Se puede romper este componente a golpes?
// El fluido nunca: es liquido, se recoge con un recipiente.
inline bool componenteRompible(BlockType t, int i) {
    if (!esCompuesto(t)) return true;
    return componenteN(t, i) != COMP_FLUIDO;
}

} // namespace Compuesto
