#pragma once

#include "BlockType.h"
#include <cmath>

// ============================================================================
// DORMIR: ACOSTARSE, PASAR LA NOCHE Y DESPERTAR
// ============================================================================
// Con UNA pulsacion de Z el jugador se tumba (otra lo despierta antes): la camara baja despacio hasta el suelo, la
// pantalla se va a negro, y al despertar ya es de dia.
//
// ----------------------------------------------------------------------------
// DONDE SE PUEDE DORMIR
// ----------------------------------------------------------------------------
// En cualquier parte, tambien a cielo abierto. PEDIDO: "elimina la dependencia
// de tener un techo y paredes al rededor de la mecanica de dormir". (Antes se
// pedia un refugio: techo y 3 paredes.)
//
// Quedan tres condiciones, cada una con su aviso:
//
//   SUELO     Algo firme bajo los pies.
//   AGUA      No estar dentro de agua ni lava.
//   ESPACIO   Que quepa tumbarse: hace falta al menos una celda libre
//             adyacente. Es lo que descarta el agujero de 1x1.
//// ----------------------------------------------------------------------------
// EL CICLO, EN SEGUNDOS REALES
// ----------------------------------------------------------------------------
//     0.0 -> 10.0   dormido. La camara baja y la pantalla se oscurece.
//     >= 10.0       despierta y AMANECE.
//     interrupcion  si se pulsa Z otra vez, despierta donde toque:
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
// ⭐ DORMIR A CUALQUIER HORA: DE NOCHE SE DUERME, DE DIA SE ECHA LA SIESTA
// ----------------------------------------------------------------------------
// PEDIDO: "que se pueda dormir en cualquier hora del dia y noche".
//
// Antes despertar SIEMPRE llevaba a las 7:00 de la manana siguiente: echarse
// a mediodia se saltaba casi un dia entero. Ahora depende de cuando:
//
//   DE NOCHE (19:00 a 5:00)  como siempre: del tiron hasta las 7:00, cortado
//                            hasta las 5:30.
//   DE DIA                   una SIESTA: del tiron, 3 horas; cortada, 1,5.
//
// Devuelve cuantas HORAS avanza el reloj (siempre hacia delante).
constexpr double SIESTA_HORAS = 3.0;
constexpr double SIESTA_CORTA_HORAS = 1.5;

inline bool EsDeNoche(double hora) { return hora >= 19.0 || hora < 5.0; }

inline double HorasDormidas(double hora, bool completo) {
    if (!EsDeNoche(hora)) return completo ? SIESTA_HORAS : SIESTA_CORTA_HORAS;
    double avance = HoraAlDespertar(completo) - hora;
    if (avance <= 0.0) avance += 24.0;   // de madrugada: hasta la manana
    return avance;
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

    // ⭐ SIN TECHO NI PAREDES. PEDIDO: "elimina la dependencia de tener un
    // techo y paredes al rededor de la mecanica de dormir". Se puede dormir a
    // cielo abierto; solo hace falta suelo, no estar en el agua y sitio para
    // tumbarse. (SIN_TECHO y SIN_PAREDES siguen en Motivo con su texto, pero
    // ya no se devuelven.)
    static const int DIRS[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    // --- ESPACIO PARA TUMBARSE ---
    //
    // ⭐ ESTA ES LA CONDICION QUE DESCARTA EL AGUJERO DE 1x1.
    //
    // Tumbarse ocupa dos celdas: la del cuerpo y una contigua para las piernas.
    // Se exige que al menos UNA de las cuatro adyacentes este libre a la altura
    // del cuerpo. En un hueco de 1x1 las cuatro estan tapadas -- es justo lo
    // que lo convierte en 1x1 -- asi que ahi no se puede dormir.
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
