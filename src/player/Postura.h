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
// CTRL Y SIGILO: "si presiona el jugador una vez ctrl y luego w, caminara
// despacio, y si presiona ctrl y W y luego shift, caminara como semi
// agachado, como si estuviera inspeccionando silencioso".
//
//   CTRL tocado (pulsar y soltar sin W)  activa o quita el SIGILO.
//   CTRL mantenido con W                 sigue siendo correr, como siempre.
//   SHIFT en sigilo                      medio agachado (INSPECCION), y otra
//                                        vez SHIFT lo quita. No agacha.
//   ESPACIO en inspeccion                vuelve al sigilo normal, sin saltar.
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

    bool sigilo = false;
    bool inspeccion = false;
    bool ctrlAntes = false;
    bool ctrlTocando = false;      // CTRL se pulso sin W: puede ser un toque
    bool wConCtrl = false;         // y mientras estaba pulsado se uso W
    double tCtrl = 0.0;
    static constexpr double TOQUE_SEG = 0.45;

    struct Salida {
        bool agachar = false;      // InputCommand::crouch
        bool tumbar = false;       // InputCommand::prone
        bool saltar = false;       // InputCommand::jump
        bool sigilo = false;       // InputCommand::sigilo
        bool inspeccion = false;   // InputCommand::inspeccion
        bool correr = false;       // CTRL mantenido, fuera del sigilo
    };

    // `libre`: en el suelo o cayendo, sin volar, nadar ni escalar.
    // `ctrl`, `w` y `t` (segundos) solo hacen falta para el sigilo.
    Salida avanzar(bool shift, bool espacio, bool libre,
                   bool ctrl = false, bool w = false, double t = 0.0) {
        const bool pulsaShift = shift && !shiftAntes;
        const bool pulsaEspacio = espacio && !espacioAntes;
        shiftAntes = shift;
        espacioAntes = espacio;
        if (!espacio) espacioGastado = false;

        // El toque de CTRL.
        if (ctrl && !ctrlAntes) { ctrlTocando = !w; wConCtrl = false; tCtrl = t; }
        if (ctrl && w) { wConCtrl = true; ctrlTocando = false; }   // eso es correr
        if (!ctrl && ctrlAntes) {
            if (ctrlTocando && !wConCtrl && t - tCtrl <= TOQUE_SEG && libre &&
                postura == Postura::DE_PIE) {
                sigilo = !sigilo;
                if (!sigilo) inspeccion = false;
            }
            ctrlTocando = false;
        }
        ctrlAntes = ctrl;

        Salida s;
        if (!libre) {
            postura = Postura::DE_PIE;
            sigilo = inspeccion = false;
            s.agachar = shift;
            s.saltar = espacio && !espacioGastado;
            s.correr = ctrl;
            return s;
        }

        // En sigilo SHIFT no agacha: pone o quita la inspeccion. Y ESPACIO,
        // si se esta inspeccionando, solo se endereza.
        if (sigilo) {
            if (pulsaShift) inspeccion = !inspeccion;
            if (pulsaEspacio && inspeccion) { inspeccion = false; espacioGastado = true; }
            s.sigilo = true;
            s.inspeccion = inspeccion;
            s.saltar = espacio && !espacioGastado && !inspeccion;
            return s;
        }
        s.correr = ctrl && !ctrlTocando;

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
