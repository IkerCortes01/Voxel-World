#pragma once

#include <cstdint>

// ============================================================================
// ¿ESTA PENCA LA PUSO EL JUGADOR, O LA GENERO UN NOPAL?
// ============================================================================
// Se pidio que la penca de nopal quede ACOSTADA solo cuando la coloca el
// jugador, y que las nacidas de un nopal conserven su forma de planta.
//
// ----------------------------------------------------------------------------
// LAS DOS SON EL MISMO BLOQUE
// ----------------------------------------------------------------------------
// Generada y colocada son BLOCK_NOPAL_FRUTO, asi que el ID no distingue. Pero
// hay una diferencia real en el mundo: la generacion SOLO coloca pencas
// PEGADAS a un cladodio -- comprueba `tocaCladodio` antes de escribir cada una
// -- mientras que la que pone el jugador cae donde el apunte.
//
//     PEGADA A LA PLANTA  =  la genero un nopal   -> conserva su forma
//     SUELTA              =  la puso el jugador   -> se acuesta
//
// No es una marca guardada en el bloque: es una propiedad del SITIO donde
// esta, asi que sobrevive a guardar y cargar el mundo sin ocupar un solo byte,
// y sigue siendo correcta si el jugador rompe la mata alrededor.
//
// ----------------------------------------------------------------------------
// POR QUE HAY QUE MIRAR TAMBIEN LAS DIAGONALES
// ----------------------------------------------------------------------------
// Este fue un bug real y merece quedar escrito.
//
// La primera version miraba solo las SEIS CARAS RECTAS. Pero la condicion que
// usaba la orientacion de la penca (`sueltaEnPlanta`) cuenta ademas las
// diagonales a media fuerza. O sea: dos reglas distintas para la misma
// pregunta.
//
// El desacuerdo se veia en un caso nada raro -- una penca en diagonal a un
// cladodio, que es JUSTO como sale un brote del borde superior de otra:
//
//     la orientacion decia "pegada"  -> se quedaba DE PIE
//     el dibujo decia    "suelta"    -> le pintaba la losa plana
//
// De pie por dentro y tumbada por fuera. Un brote en diagonal ES parte de la
// planta, asi que la respuesta correcta para las dos es "pegada".
//
// Ahora se miran las 18 vecindades: 6 caras, 8 diagonales verticales y 4
// esquinas del plano. Y sobre todo, se miran DESDE UN SOLO SITIO.
//
// ----------------------------------------------------------------------------
// CUATRO SITIOS PREGUNTAN LO MISMO
// ----------------------------------------------------------------------------
//   - la ORIENTACION de la forma (de pie o tumbada)
//   - la CAJA DE COLISION
//   - el DIBUJO de la losa en el mesher
//   - y si al COLOCARLA se lanza la animacion de caida
//
// Si discreparan, la penca se veria plana y se colisionaria como un cubo, o se
// caeria y se volveria a levantar sola al terminar la animacion. Las dos cosas
// han pasado ya en este codigo.
// ============================================================================

namespace Render {

// Las 18 vecindades que decide si una penca esta pegada a la planta.
//
// Se exponen como dato --y no escritas dentro de un bucle-- para que un test
// pueda comprobar que son exactamente las que se esperan, sin depender de
// BlockType ni del mundo.
struct VecindadPenca {
    int dx, dy, dz;
};

inline const VecindadPenca* vecindadesPenca(int& cuantas) {
    static const VecindadPenca V[] = {
        // Las seis caras rectas.
        { 1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1},

        // Diagonales verticales: un brote que sale del borde de otra penca,
        // que es la forma normal de crecer de un nopal.
        { 1, 1,0}, {-1, 1,0}, {0, 1, 1}, {0, 1,-1},
        { 1,-1,0}, {-1,-1,0}, {0,-1, 1}, {0,-1,-1},

        // Las cuatro esquinas del plano horizontal.
        { 1,0, 1}, {-1,0,-1}, { 1,0,-1}, {-1,0, 1},
    };
    cuantas = (int)(sizeof(V) / sizeof(V[0]));
    return V;
}

// ¿Esta suelta? `encadena(dx,dy,dz)` responde si el vecino en ese
// desplazamiento forma parte de la misma planta.
//
// Se pasa como funcion para que el test pueda montar una vecindad a mano sin
// necesitar un mundo de voxeles.
template <typename TEncadena>
inline bool pencaSuelta(TEncadena encadena) {
    int n = 0;
    const VecindadPenca* v = vecindadesPenca(n);
    for (int i = 0; i < n; ++i) {
        if (encadena(v[i].dx, v[i].dy, v[i].dz)) return false;
    }
    return true;
}

} // namespace Render
