#pragma once

#include <cstddef>

// ============================================================================
// QUE IMAGEN USA UNA TUNA
// ============================================================================
// EL BUG QUE ESTO CIERRA: las tunas no se dibujaban NUNCA. No es que tardaran
// en aparecer -- el fruto simplemente no estaba.
//
// LA CADENA, QUE ATRAVIESA TRES DECISIONES CORRECTAS:
//
//   1. La textura de una tuna depende de su POSICION EN EL MUNDO: la variedad
//      sale del bloque y la madurez de un hash de la celda. Asi que no se
//      puede pedir con `getBlockTexture(tipo, cara)` -- le faltan las
//      coordenadas.
//
//      Consecuencia: el barrido exhaustivo de precargarTodasLasCaras(), que
//      recorre el enum entero pidiendo las seis caras de cada bloque, NUNCA
//      llegaba a la tuna.
//
//   2. El unico sitio que pedia la textura de tuna es el mesher, y el mesher
//      corre en WORKERS. Alli `puedeCargar()` es false a proposito -- abrir un
//      PNG y crear una textura de GL desde un hilo de trabajo cuelga el driver
//      -- asi que la carga devolvia 0.
//
//   3. El mesher hacia `if (texT != 0)` y, con 0, no emitia NINGUNA CARA. Esa
//      rama no pasa por `texSegura`, que es quien marca `texturasFaltantes`,
//      asi que el chunk llegaba a LISTO sin fruto y sin nada pendiente. Nadie
//      volvia a pedir el remallado.
//
// Las tres piezas son razonables por separado. Juntas dejaban la tuna en un
// punto ciego permanente: nadie la cargaba y nadie se quejaba.
//
// ----------------------------------------------------------------------------
// POR QUE ESTO VIVE EN UN HEADER APARTE
// ----------------------------------------------------------------------------
// Porque hay DOS sitios que tienen que elegir exactamente los mismos archivos:
//
//   - la PRECARGA, que los sube a la GPU desde el hilo principal
//   - el MESHER, que los pide por posicion al dibujar
//
// Si se separan, la precarga carga imagenes que nadie pide y el mesher pide
// imagenes que nadie cargo -- que es, literalmente, el bug. Teniendo la lista
// en un solo sitio, un test puede comprobar que coinciden.
// ============================================================================

namespace Render {

// Variedad de la planta. La dice el BLOQUE, no la posicion: al romper una tuna
// roja el jugador recoge una tuna roja, asi que lo que se ve y lo que se coge
// tienen que salir del mismo dato.
enum class VariedadTuna : int {
    VERDE    = 0,
    AMARILLA = 1,
    ROJA     = 2
};

// La imagen que corresponde a una variedad y un estado de madurez.
//
// Un fruto SIN MADURAR siempre usa la verde, sea cual sea la variedad de la
// planta: nace como brote tierno y solo toma su color al madurar. Por eso hay
// cuatro casos pero solo TRES archivos.
inline const char* archivoTuna(VariedadTuna variedad, bool madura) {
    if (!madura) return "Tuna verde crecida.png";

    switch (variedad) {
        case VariedadTuna::AMARILLA: return "Tuna amarilla crecida.png";
        case VariedadTuna::ROJA:     return "Tuna roja crecida.png";
        case VariedadTuna::VERDE:
        default:                     return "Tuna verde crecida.png";
    }
}

// ----------------------------------------------------------------------------
// LA LISTA COMPLETA, PARA LA PRECARGA
// ----------------------------------------------------------------------------
// Todo lo que archivoTuna() puede llegar a devolver. La precarga recorre esto
// desde el hilo principal para que el worker lo encuentre siempre en el cache.
//
// Si alguien anade una variedad, tiene que aparecer aqui. Un test comprueba
// que no se puede generar ningun archivo que no este en la lista.
inline const char* const* archivosTunaPrecarga(size_t& cuantos) {
    static const char* const LISTA[] = {
        "Tuna verde crecida.png",      // sin madurar, y variedad verde
        "Tuna amarilla crecida.png",
        "Tuna roja crecida.png",
    };
    cuantos = sizeof(LISTA) / sizeof(LISTA[0]);
    return LISTA;
}

} // namespace Render
