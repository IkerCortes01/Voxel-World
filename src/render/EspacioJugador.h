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

} // namespace Render
