#ifndef POSTURA_H
#define POSTURA_H

// ============================================================================
// DE PIE, AGACHADO Y TUMBADO: SHIFT Y ESPACIO COMO INTERRUPTORES
// ============================================================================
// PEDIDO: "cuando se presiona una vez Shift, se mantiene, y si se presiona dos
// veces Shift se agacha hasta que el torso toque el suelo y si presionas
// space, te devuelve a agacharte y si lo presionas de nuevo estas parado".
//
//   SHIFT    de pie -> agachado -> tumbado. No hay que mantenerlo.
//   ESPACIO  tumbado -> agachado -> de pie. Solo salta si ya estaba de pie.
//
// La pulsacion de ESPACIO que levanta no salta, aunque se mantenga: el salto
// necesita soltarla y volver a pulsar. Si no, levantarse con espacio y seguir
// pulsando haria saltar al jugador sin querer.
//
// En el aire, volando, nadando o en una escalera SHIFT vuelve a ser la tecla
// mantenida de siempre (bajar), porque ahi "agacharse" significa descender.
// Y la postura se pierde: nadie sale del agua tumbado.
//
// No sabe nada de GLFW ni de la fisica: recibe dos teclas y devuelve la
// intencion. Si un techo impide levantarse, la fisica lo retiene agachado y se
// levanta en cuanto quepa.
// ============================================================================

namespace PlayerSys {

enum class Postura { DE_PIE, AGACHADO, TUMBADO };

struct ControlPostura {
    Postura postura = Postura::DE_PIE;
    bool shiftAntes = false;
    bool espacioAntes = false;
    bool espacioGastado = false;   // esta pulsacion de espacio ya cambio la postura

    struct Salida {
        bool agachar = false;      // InputCommand::crouch
        bool tumbar = false;       // InputCommand::prone
        bool saltar = false;       // InputCommand::jump
    };

    // `libre`: en el suelo o cayendo, sin volar, nadar ni escalar.
    Salida avanzar(bool shift, bool espacio, bool libre) {
        const bool pulsaShift = shift && !shiftAntes;
        const bool pulsaEspacio = espacio && !espacioAntes;
        shiftAntes = shift;
        espacioAntes = espacio;
        if (!espacio) espacioGastado = false;

        Salida s;
        if (!libre) {
            postura = Postura::DE_PIE;
            s.agachar = shift;
            s.saltar = espacio && !espacioGastado;
            return s;
        }

        if (pulsaShift) {
            if (postura == Postura::DE_PIE) postura = Postura::AGACHADO;
            else postura = Postura::TUMBADO;
        }
        if (pulsaEspacio && postura != Postura::DE_PIE) {
            postura = postura == Postura::TUMBADO ? Postura::AGACHADO : Postura::DE_PIE;
            espacioGastado = true;
        }

        s.agachar = postura != Postura::DE_PIE;
        s.tumbar = postura == Postura::TUMBADO;
        s.saltar = postura == Postura::DE_PIE && espacio && !espacioGastado;
        return s;
    }
};

} // namespace PlayerSys

#endif // POSTURA_H
