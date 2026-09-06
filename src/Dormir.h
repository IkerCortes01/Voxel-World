#pragma once

#include "BlockType.h"
#include <cmath>

// ============================================================================
// DORMIR: ACOSTARSE, PASAR LA NOCHE Y DESPERTAR
// ============================================================================
// Con Z el jugador se tumba: la camara baja despacio hasta el suelo, la
// pantalla se va a negro, y al despertar ya es de dia.
//
// ----------------------------------------------------------------------------
// POR QUE HACE FALTA UN REFUGIO, Y POR QUE NO VALE UN AGUJERO DE 1x1
// ----------------------------------------------------------------------------
// Dormir a la intemperie tiene que estar prohibido, porque si no la noche deja
// de existir como problema: bastaria pulsar Z en cualquier parte para saltarsela.
// El refugio es el COSTE de saltarse la noche -- hay que haberlo construido o
// encontrado antes.
//
// Pero un hueco de un bloque tampoco cuenta. Cavar 1x1 en una pared es trivial
// (dos golpes) y dejaria el requisito en nada: seria "pulsa Z mirando a una
// pared". Se pide sitio para TUMBARSE, que es lo que se esta haciendo.
//
// De ahi las tres condiciones, y las tres se comprueban por separado para que
// el aviso pueda decir cual falta:
//
//   TECHO     Algo solido sobre la cabeza, no el cielo. Es lo que distingue
//             un refugio de un claro del bosque.
//   PAREDES   Al menos 3 de los 4 lados cerrados. Tres y no cuatro para que
//             valga una habitacion con puerta -- un cuarto sellado por los
//             cuatro costados no se puede ni construir sin quedarse dentro.
//   ESPACIO   Que quepa tumbarse: hace falta al menos una celda libre
//             adyacente. Es justo lo que descarta el agujero de 1x1.
//
// ----------------------------------------------------------------------------
// EL CICLO, EN SEGUNDOS REALES
// ----------------------------------------------------------------------------
//     0.0 -> 10.0   dormido. La camara baja y la pantalla se oscurece.
//     >= 10.0       despierta y AMANECE.
//     interrupcion  si se suelta antes, despierta donde toque:
//                   antes de DESPERTAR_TEMPRANO no cambia la hora,
//                   despues, amanece igual pero mas temprano.
//
// La hora a la que se despierta NO es la misma en los dos casos, y eso es
// deliberado: dormir del tiron lleva a plena manana; cortar el sueno a media
// noche deja al jugador en el gris del amanecer.
// ============================================================================

namespace Dormir {

// ----------------------------------------------------------------------------
// TIEMPOS DEL CICLO, en segundos reales
// ----------------------------------------------------------------------------

// Lo que dura dormir entero.
constexpr float DURACION = 10.0f;

// A partir de aqui, soltar la tecla ya cuenta como haber dormido: se despierta
// de madrugada en vez de no pasar nada. Antes de este punto el jugador solo se
// ha tumbado un momento.
constexpr float DESPERTAR_TEMPRANO = 5.0f;

// Cuanto tarda la camara en bajar del todo. Menos que la duracion entera para
// que el jugador vea el movimiento acabado y luego la pantalla apagandose, en
// vez de las dos cosas a la vez todo el rato.
constexpr float BAJADA_CAMARA = 2.2f;

// Cuando empieza a oscurecerse la pantalla, y cuando esta del todo negra.
constexpr float FUNDIDO_INICIO = 1.2f;
constexpr float FUNDIDO_FIN    = 7.0f;

// ----------------------------------------------------------------------------
// LAS HORAS A LAS QUE SE DESPIERTA
// ----------------------------------------------------------------------------
// En horas del reloj del mundo (0..24).

// Sueno completo: plena manana, con el sol ya alto.
constexpr double HORA_AMANECER = 7.0;

// Sueno interrumpido: el gris de antes de que salga el sol. Es la recompensa
// reducida de haberse levantado antes.
constexpr double HORA_TEMPRANO = 5.5f;

// ----------------------------------------------------------------------------
// EN QUE PUNTO DEL SUENO ESTA
// ----------------------------------------------------------------------------

// Cuanto ha bajado la camara: 0 de pie, 1 tumbado del todo.
// Suavizado en los dos extremos (smoothstep) para que no arranque ni pare de
// golpe -- un cuerpo que se acuesta acelera y frena, no baja a velocidad
// constante.
inline float AlturaCamara(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= BAJADA_CAMARA) return 1.0f;
    const float u = t / BAJADA_CAMARA;
    return u * u * (3.0f - 2.0f * u);
}

// Cuanto de negra esta la pantalla: 0 se ve todo, 1 negro total.
inline float Oscuridad(float t) {
    if (t <= FUNDIDO_INICIO) return 0.0f;
    if (t >= FUNDIDO_FIN)    return 1.0f;
    const float u = (t - FUNDIDO_INICIO) / (FUNDIDO_FIN - FUNDIDO_INICIO);
    return u * u * (3.0f - 2.0f * u);
}

// ¿Ya termino de dormir?
inline bool Terminado(float t) { return t >= DURACION; }

// Al soltar la tecla en el instante `t`, ¿ha dormido lo bastante como para que
// amanezca?
inline bool CuentaComoDormido(float t) { return t >= DESPERTAR_TEMPRANO; }

// A que hora despierta, segun como haya acabado el sueno.
inline double HoraAlDespertar(bool completo) {
    return completo ? HORA_AMANECER : HORA_TEMPRANO;
}

// ----------------------------------------------------------------------------
// POR QUE NO SE PUEDE DORMIR AQUI
// ----------------------------------------------------------------------------
// Se devuelve el motivo concreto, no un simple false: el jugador tiene que
// poder saber que le falta al refugio en vez de adivinarlo.
enum class Motivo {
    PUEDE = 0,        // adelante
    SIN_TECHO,        // esta a cielo abierto
    SIN_PAREDES,      // muy poco cerrado
    SIN_ESPACIO,      // cabe de pie, pero no para tumbarse (el 1x1)
    EN_EL_AIRE,       // no hay suelo bajo los pies
    EN_AGUA           // dentro de un liquido
};

inline const char* TextoDeMotivo(Motivo m) {
    switch (m) {
        case Motivo::PUEDE:       return "";
        case Motivo::SIN_TECHO:   return "NO PUEDES DORMIR A CIELO ABIERTO";
        case Motivo::SIN_PAREDES: return "NECESITAS UN LUGAR CON PAREDES";
        case Motivo::SIN_ESPACIO: return "NO HAY SITIO PARA TUMBARSE";
        case Motivo::EN_EL_AIRE:  return "NECESITAS PISAR SUELO FIRME";
        case Motivo::EN_AGUA:     return "NO PUEDES DORMIR EN EL AGUA";
    }
    return "";
}

// ----------------------------------------------------------------------------
// LOS PARAMETROS DEL REFUGIO
// ----------------------------------------------------------------------------

// Hasta que altura se busca techo sobre la cabeza. Con 6 vale una casa de
// techo alto o una cueva; mas arriba ya es una montana que pasa por encima, no
// un refugio.
constexpr int ALTURA_TECHO = 6;

// Cuantos lados de los cuatro tienen que estar cerrados. Tres, para que valga
// una habitacion con puerta o con una entrada abierta.
constexpr int PAREDES_MINIMAS = 3;

// A que distancia se busca la pared. 3 bloques: una habitacion normal cuenta,
// pero un claro rodeado de arboles lejanos no.
constexpr int ALCANCE_PARED = 3;

// ----------------------------------------------------------------------------
// ¿SE PUEDE DORMIR AQUI?
// ----------------------------------------------------------------------------
// Se le pasan dos funciones que consultan el mundo, para que este archivo no
// dependa de World ni de nada del motor -- misma inversion de dependencias que
// IWorldQuery en el character controller, y por eso se puede testear entero
// sin arrancar OpenGL.
//
//   solido(x,y,z)  true si ese bloque BLOQUEA el paso (es un cubo macizo)
//   liquido(x,y,z) true si es agua o lava
//
// (px,py,pz) son los pies del jugador, ya en enteros.
template <typename FnSolido, typename FnLiquido>
inline Motivo PuedeDormir(int px, int py, int pz,
                          FnSolido solido, FnLiquido liquido) {
    // --- Lo basico: suelo bajo los pies y nada de agua ---
    if (liquido(px, py, pz) || liquido(px, py + 1, pz)) return Motivo::EN_AGUA;
    if (!solido(px, py - 1, pz)) return Motivo::EN_EL_AIRE;

    // --- TECHO ---
    // Se mira hacia arriba desde la cabeza. Si se llega al tope sin encontrar
    // nada solido, esta a cielo abierto.
    bool hayTecho = false;
    for (int h = 2; h <= ALTURA_TECHO; ++h) {
        if (solido(px, py + h, pz)) { hayTecho = true; break; }
    }
    if (!hayTecho) return Motivo::SIN_TECHO;

    // --- PAREDES ---
    // Los cuatro rumbos, a la altura del cuerpo. Un lado cuenta como cerrado
    // si hay algo solido dentro del alcance.
    static const int DIRS[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    int cerrados = 0;
    for (const auto& d : DIRS) {
        bool cerrado = false;
        for (int r = 1; r <= ALCANCE_PARED && !cerrado; ++r) {
            // Se mira a la altura del pecho (py+1): es donde una pared de
            // verdad esta, y evita que un escalon de un bloque cuente como
            // muro.
            if (solido(px + d[0] * r, py + 1, pz + d[1] * r)) cerrado = true;
        }
        if (cerrado) ++cerrados;
    }
    if (cerrados < PAREDES_MINIMAS) return Motivo::SIN_PAREDES;

    // --- ESPACIO PARA TUMBARSE ---
    //
    // ⭐ ESTA ES LA CONDICION QUE DESCARTA EL AGUJERO DE 1x1.
    //
    // Tumbarse ocupa dos celdas: la del cuerpo y una contigua para las piernas.
    // Se exige que al menos UNA de las cuatro adyacentes este libre a la altura
    // del cuerpo. En un hueco de 1x1 las cuatro estan tapadas -- es justo lo
    // que lo convierte en 1x1 -- asi que ahi no se puede dormir por mucho techo
    // y paredes que tenga.
    bool hayHueco = false;
    for (const auto& d : DIRS) {
        const int ax = px + d[0], az = pz + d[1];
        // Libre a la altura de los pies Y del pecho: si solo cabe uno de los
        // dos, no es sitio para tenderse.
        if (!solido(ax, py, az) && !solido(ax, py + 1, az)) {
            hayHueco = true;
            break;
        }
    }
    if (!hayHueco) return Motivo::SIN_ESPACIO;

    return Motivo::PUEDE;
}

} // namespace Dormir
