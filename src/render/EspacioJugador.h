#pragma once

// ============================================================================
// NADA SE MATERIALIZA DENTRO DEL JUGADOR
// ============================================================================
// EL BUG: el jugador se quedaba SOFOCADO dentro de un bloque al construir.
//
// `placeBlock` ya comprobaba que un bloque colocado a mano no cayera dentro del
// jugador, y esa parte funcionaba. El agujero estaba en la GRAVEDAD: un bloque
// que CAE no pasa por esa comprobacion. Aterriza y hace `setBlock` sin
// preguntarle a nadie, asi que si el jugador ocupaba esa celda, se quedaba
// dentro de la piedra.
//
// EL CASO QUE LO DISPARABA ERA JUSTO EL PILAR:
//
//   1. Saltas y colocas un bloque bajo tus pies.
//   2. Ese bloque no tiene nada debajo --estas construyendo hacia arriba-- asi
//      que la regla de "lo que se pone en el aire se cae" lo desprende.
//   3. Caes con el.
//   4. Al tocar suelo, el bloque se recoloca... en la celda que ocupas.
//
// Se arregla por los dos lados:
//
//   - El bloque que SOSTIENE al jugador ya no se considera "en el aire": lo
//     esta sosteniendo a EL, que es soporte de sobra. Es lo que mantiene viva
//     la habilidad de hacer pilares saltando, que se pidio expresamente no
//     romper.
//
//   - Y ningun bloque que cae puede aterrizar en una celda ocupada por el
//     jugador: esa celda cuenta como ocupada, igual que si hubiera roca, asi
//     que el bloque le pasa por ENCIMA o se descarta.
//
// ----------------------------------------------------------------------------
// POR QUE ESTO VIVE EN UN HEADER
// ----------------------------------------------------------------------------
// La misma prueba hace falta en CUATRO sitios distintos: el aterrizaje de
// piezas, el de bloques sueltos, el sobrante de una fusion de niveles y la
// decision de si un bloque recien puesto debe caer.
//
// Escribirla cuatro veces es exactamente el patron que ya ha producido varios
// bugs en este archivo (dos listas de "suelo firme", dos cajas para la misma
// penca, dos listas de texturas de tuna): en cuanto una se toca, las otras
// dejan de coincidir -- y aqui la consecuencia es que el jugador se queda
// enterrado.
// ============================================================================

namespace Render {

// La caja del jugador, en coordenadas de mundo.
struct CajaJugador {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

// `pos` son los PIES del jugador (su posicion), no su centro.
inline CajaJugador cajaDelJugador(float px, float py, float pz,
                                  float ancho, float alto) {
    const float medio = ancho * 0.5f;
    return CajaJugador{ px - medio, px + medio,
                        py,         py + alto,
                        pz - medio, pz + medio };
}

// ¿La celda de voxel (cx,cy,cz) se solapa con el jugador?
//
// Un voxel ocupa [c, c+1) en cada eje. Se usa solapamiento ESTRICTO: tocarse
// por una cara no cuenta, porque el jugador esta constantemente apoyado en la
// cara de arriba del bloque que pisa -- si eso contara, no se podria colocar
// nada bajo los pies y el pilar dejaria de funcionar.
inline bool celdaPisaAlJugador(int cx, int cy, int cz, const CajaJugador& j) {
    return ((float)cx + 1.0f > j.minX) && ((float)cx < j.maxX) &&
           ((float)cy + 1.0f > j.minY) && ((float)cy < j.maxY) &&
           ((float)cz + 1.0f > j.minZ) && ((float)cz < j.maxZ);
}

// ----------------------------------------------------------------------------
// ¿ESTE BLOQUE ESTA SOSTENIENDO AL JUGADOR?
// ----------------------------------------------------------------------------
// Es la excepcion que mantiene vivo el PILAR SALTANDO.
//
// Un bloque colocado justo bajo los pies del jugador no esta "en el aire":
// aunque no tenga suelo debajo, esta sosteniendo a una persona. Es lo que hace
// un andamio, y es exactamente el gesto de construir hacia arriba saltando.
//
// El margen vertical es de un bloque por debajo de los pies, que es donde cae
// el bloque del pilar. Y se mira tambien el solape horizontal: un bloque
// puesto a un lado, aunque este a la misma altura, no sostiene a nadie.
inline bool sostieneAlJugador(int cx, int cy, int cz, const CajaJugador& j) {
    // ⚠️ LA CELDA TIENE QUE ESTAR POR DEBAJO DEL CUERPO, NO SOLAPARLO.
    //
    // La primera version usaba `cy <= minY + 0.05`, con la idea de tolerar
    // que en pleno salto los pies no caigan justo en el borde de la celda.
    //
    // Pero eso hacia que la celda de los PROPIOS PIES contara como soporte: un
    // jugador en y=11.0 tiene los pies en la celda 11, y 11 <= 11.05 es
    // cierto. Un test de coherencia lo cazo -- esa celda salia a la vez como
    // "sostiene" y como "esta dentro del jugador", que es una contradiccion:
    // una regla diria que el bloque no se cae y la otra que no se puede
    // colocar ahi.
    //
    // LA REGLA, en dos partes:
    //
    //   1. La celda tiene que estar POR DEBAJO del cuerpo. Si solapa al
    //      jugador no es soporte: es sofocacion. Esto es lo que resuelve la
    //      contradiccion -- las dos reglas ya no pueden dar true a la vez.
    //
    //   2. Y tiene que estar CERCA de los pies: como mucho un bloque por
    //      debajo. Es el tramo en el que el jugador, tras soltarlo, cae encima.
    //
    // El caso de en medio del salto (pies en 11.5) entra por la parte 2: la
    // celda 11 no solapa el cuerpo por arriba --el jugador empieza en 11.5 y
    // la celda acaba en 12, asi que SI solapa-- ojo, por eso hace falta la
    // comprobacion explicita de solape y no basta comparar alturas.
    //
    // Resultado: si la celda solapa el cuerpo NUNCA sostiene (gana la
    // sofocacion), y si no lo solapa pero esta justo debajo, sostiene.
    if (celdaPisaAlJugador(cx, cy, cz, j)) return false;

    const bool cercaDeLosPies =
        ((float)cy + 1.0f <= j.minY + 0.001f) &&   // techo en los pies o debajo
        ((float)cy + 1.0f >  j.minY - 1.05f);      // pero no mas de un bloque

    const bool debajoEnPlanta =
        ((float)cx + 1.0f > j.minX) && ((float)cx < j.maxX) &&
        ((float)cz + 1.0f > j.minZ) && ((float)cz < j.maxZ);

    return cercaDeLosPies && debajoEnPlanta;
}

// ============================================================================
// ⭐ APILAR BAJO LOS PIES TE SUBE ENCIMA, NO TE ENTIERRA
// ============================================================================
// LO QUE SE PIDIO: colocando niveles en continuo (clic derecho mantenido)
// justo debajo del jugador, sin saltar, que el jugador se eleve lo necesario
// para no quedarse atascado dentro de lo que acaba de poner.
//
// ----------------------------------------------------------------------------
// POR QUE NO BASTA CON PROHIBIR LA COLOCACION
// ----------------------------------------------------------------------------
// `placeBlock` ya tiene una comprobacion de solape con el jugador, pero su
// respuesta es NO COLOCAR NADA. Para el camino normal esta bien: si apuntas a
// tus propios pies, el clic no hace nada y ya esta.
//
// Para el APILADO DE NIVELES esa respuesta es la equivocada. Un nivel de 1/8
// crece dentro de la celda que ya ocupas: el jugador esta de pie SOBRE esa
// celda y su caja empieza justo donde el nivel va a llegar. Si se prohibe,
// apilar bajo los pies deja de funcionar a partir del primer octavo -- y es
// justo el gesto de levantar el suelo sobre el que estas parado.
//
// Lo correcto es lo que hace un escalon: la superficie sube y el jugador sube
// con ella. Es exactamente el step-up que el controlador ya aplica al andar
// contra un bloque, solo que aqui el suelo se mueve y el jugador no.
//
// ----------------------------------------------------------------------------
// LA CUENTA
// ----------------------------------------------------------------------------
// `alturaDe()` da la altura solida de un bloque en fraccion de voxel (0..1),
// y es la MISMA funcion que usan el mesher y la colision -- no una copia. La
// cara de arriba de la celda `cy` queda entonces en `cy + altura`.
//
// Solo se sube si:
//
//   1. La celda esta en la columna del jugador (solape en planta). Un nivel
//      puesto a un lado no levanta a nadie.
//   2. La nueva superficie queda POR ENCIMA de los pies. Si el nivel sigue
//      por debajo, no hay nada que corregir y devolver un empujon de cero
//      evita el temblor de subir y bajar cada clic.
//   3. La subida es pequena -- como mucho un bloque. Es el mismo tope que
//      `stepHeight` (1.05) y es lo que impide que esto se convierta en un
//      teletransporte vertical si algun dia una celda crece de golpe.
//
// Devuelve CUANTO hay que subir (0 si no hay que hacer nada), no la posicion
// final: el llamante suma ese incremento a la Y del jugador y se lo pasa a la
// camara como escalon, para que el ojo lo reabsorba suavemente en vez de dar
// un tiron.
//
// `alturaSolida` se recibe ya calculada en vez de llamar a `alturaDe()` aqui,
// porque este header no conoce BlockType a proposito: es lo que le permite
// compilarse en los tests sin arrastrar medio motor.
inline float subidaPorApilado(int cx, int cy, int cz,
                              float alturaSolida,
                              const CajaJugador& j,
                              float maxSubida = 1.05f) {
    // Fuera de la columna del jugador: no lo levanta.
    const bool enPlanta =
        ((float)cx + 1.0f > j.minX) && ((float)cx < j.maxX) &&
        ((float)cz + 1.0f > j.minZ) && ((float)cz < j.maxZ);
    if (!enPlanta) return 0.0f;

    const float superficie = (float)cy + alturaSolida;
    const float subida = superficie - j.minY;

    // Ya esta por encima de la nueva superficie: nada que hacer.
    if (subida <= 0.0f) return 0.0f;

    // Mas de un bloque de golpe no es un escalon: es otra cosa, y subir al
    // jugador ahi seria moverlo a un sitio que no ha pedido.
    //
    // El epsilon no es cosmetico: `maxSubida` vale 1.05 y las alturas salen de
    // dividir enteros entre 16, asi que el caso que cae JUSTO en el limite se
    // decide por el ultimo bit del float. Sin margen, un escalon legal se
    // rechazaba por 1e-7 y el jugador se quedaba dentro del bloque.
    if (subida > maxSubida + 1e-4f) return 0.0f;

    return subida;
}

} // namespace Render
