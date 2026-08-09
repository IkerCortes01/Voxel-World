#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "PlayerTypes.h"

// ============================================================================
// INPUT MANAGER
// ============================================================================
// RESPONSABILIDAD UNICA: traducir el estado crudo del teclado/raton a un
// InputCommand neutro. No conoce fisica, ni camara, ni el mundo.
//
// ----------------------------------------------------------------------------
// POR QUE UNA CAPA INTERMEDIA
// ----------------------------------------------------------------------------
// La fisica nunca lee `keys['w']`. Recibe un InputCommand con intenciones
// abstractas (moveForward = 1.0). Ventajas concretas:
//
//   - REBINDEO trivial: cambiar teclas no toca la fisica.
//   - TESTS deterministas: se alimenta un InputCommand sintetico sin GLFW,
//     que es exactamente lo que hacen las pruebas de consistencia de FPS.
//   - REPETICIONES/RED: un comando es serializable; el estado de teclado no.
//   - Soporte de gamepad: basta rellenar los ejes con valores analogicos.
//
// ----------------------------------------------------------------------------
// ACUMULACION DEL RATON
// ----------------------------------------------------------------------------
// Los eventos de raton llegan por callback, potencialmente VARIAS veces por
// frame. Si cada callback rotara la camara directamente, el resultado
// dependeria del numero de eventos recibidos. Aqui se ACUMULAN los deltas y
// se consumen una vez por frame, de forma que el giro total es el mismo
// independientemente de como se repartan los eventos.
// ============================================================================

namespace PlayerSys {

class InputManager {
private:
    const MovementConfig* cfg;

    // Deltas de raton acumulados desde el ultimo consumo.
    float pendingMouseDX = 0.0f;
    float pendingMouseDY = 0.0f;

    // Estado de teclas (indexado por caracter ASCII en minuscula).
    bool keyForward = false;
    bool keyBack    = false;
    bool keyLeft    = false;
    bool keyRight   = false;
    bool keyJump    = false;
    bool keySprint  = false;
    bool keyCrouch  = false;

    bool inputEnabled = true;

    // ------------------------------------------------------------------------
    // DOBLE TOQUE DE W -> CORRER
    // ------------------------------------------------------------------------
    // Pulsar W dos veces rapido activa el sprint, que se mantiene mientras W
    // siga pulsada y se cancela al soltarla.
    //
    // Se implementa aqui, en el InputManager, porque es una cuestion de
    // INTERPRETACION DEL INPUT: la fisica no debe saber si el sprint viene de
    // una tecla dedicada, de un doble toque o de un gamepad. Solo recibe
    // `cmd.sprint`.
    //
    // La deteccion trabaja sobre FLANCOS (la transicion suelta->pulsada), no
    // sobre el estado continuo: si midieramos "W esta pulsada" dos veces,
    // mantenerla pulsada un instante bastaria para disparar el sprint.
    bool  prevForward = false;      // estado de W en el frame anterior
    double lastTapTime = -1000.0;   // instante del ultimo toque de W
    bool  doubleTapSprint = false;  // sprint activo por doble toque
    double currentTime = 0.0;       // reloj inyectado desde el motor

    // Ventana maxima entre los dos toques. 350 ms es el valor habitual en
    // sandbox voxel: lo bastante amplio para que salga sin precision de
    // milisegundos, y lo bastante corto para no dispararse al caminar
    // soltando y volviendo a pulsar W de forma natural.
    static constexpr double DOUBLE_TAP_WINDOW = 0.35;

public:
    explicit InputManager(const MovementConfig* c) : cfg(c) {}

    // ---- Entrada de teclado (llamar desde el callback) ----

    // ⭐ W con deteccion de doble toque.
    void setForward(bool v) {
        // Flanco de subida: la tecla pasa de suelta a pulsada.
        if (v && !prevForward) {
            if (currentTime - lastTapTime <= DOUBLE_TAP_WINDOW) {
                doubleTapSprint = true;   // segundo toque dentro de la ventana
                lastTapTime = -1000.0;    // consumir, para que un tercer toque
                                          // no encadene otro par
            } else {
                lastTapTime = currentTime; // primer toque: abrir la ventana
            }
        }

        // Soltar W cancela el sprint por doble toque (requisito: "solo se
        // detiene cuando dejas de presionar W").
        if (!v && prevForward) {
            doubleTapSprint = false;
        }

        prevForward = v;
        keyForward = v;
    }

    void setBack(bool v)    { keyBack    = v; }
    void setLeft(bool v)    { keyLeft    = v; }
    void setRight(bool v)   { keyRight   = v; }
    void setJump(bool v)    { keyJump    = v; }
    void setSprint(bool v)  { keySprint  = v; }
    void setCrouch(bool v)  { keyCrouch  = v; }

    // El motor inyecta su reloj: asi el modulo no depende de GLFW y los
    // tests pueden avanzar el tiempo de forma determinista.
    void setTime(double t) { currentTime = t; }

    bool isDoubleTapSprinting() const { return doubleTapSprint; }

    // Cancelar el sprint por doble toque desde fuera (p.ej. al agacharse).
    void cancelDoubleTapSprint() { doubleTapSprint = false; }

    // ---- Entrada de raton (llamar desde el callback, puede ser N veces) ----
    void addMouseDelta(float dx, float dy) {
        if (!std::isfinite(dx) || !std::isfinite(dy)) return;
        pendingMouseDX += dx;
        pendingMouseDY += dy;
    }

    // Desactiva el movimiento sin perder el estado de teclas (menus, pausa).
    void setEnabled(bool e) {
        inputEnabled = e;
        if (!e) {
            pendingMouseDX = pendingMouseDY = 0.0f;
        }
    }

    bool isEnabled() const { return inputEnabled; }

    // Limpia todo (al abrir un menu o perder el foco).
    //
    // NOTA: reset() NO toca prevForward ni lastTapTime a proposito.
    // El motor llama a reset() y acto seguido reinyecta el estado real del
    // teclado en CADA frame; si aqui se falseara prevForward=false, el
    // siguiente setForward(true) pareceria un flanco nuevo y se contarian
    // toques fantasma con W simplemente mantenida pulsada.
    // Para limpiar de verdad el estado del doble toque existe
    // cancelDoubleTapSprint().
    void reset() {
        keyForward = keyBack = keyLeft = keyRight = false;
        keyJump = keySprint = keyCrouch = false;
        pendingMouseDX = pendingMouseDY = 0.0f;
    }

    // ========================================================================
    // CONSTRUIR EL COMANDO DEL FRAME
    // ========================================================================
    // Consume los deltas de raton acumulados.
    InputCommand buildCommand() {
        InputCommand cmd;

        if (!inputEnabled) {
            pendingMouseDX = pendingMouseDY = 0.0f;
            return cmd; // comando vacio: el jugador frena por friccion
        }

        // Ejes digitales -> analogicos [-1, 1].
        // Pulsar W y S simultaneamente se cancela (en vez de que gane una).
        cmd.moveForward = (keyForward ? 1.0f : 0.0f) - (keyBack ? 1.0f : 0.0f);
        cmd.moveRight   = (keyRight   ? 1.0f : 0.0f) - (keyLeft ? 1.0f : 0.0f);

        cmd.jump    = keyJump;
        // Sprint por tecla dedicada (CTRL) O por doble toque de W.
        // Se exige que W siga pulsada: si el jugador la suelta, el sprint por
        // doble toque desaparece aunque el flag no se hubiera limpiado aun.
        cmd.sprint  = keySprint || (doubleTapSprint && keyForward);
        cmd.crouch  = keyCrouch;

        // En vuelo, salto/agacharse controlan la altura.
        cmd.flyUp   = keyJump;
        cmd.flyDown = keyCrouch;

        // ---- RATON ----
        // Escalado SOLO por sensibilidad, nunca por delta time (ver
        // CameraSystem para la justificacion).
        cmd.lookDeltaYaw   = pendingMouseDX * cfg->mouseSensitivity;
        cmd.lookDeltaPitch = pendingMouseDY * cfg->mouseSensitivity;

        pendingMouseDX = 0.0f;
        pendingMouseDY = 0.0f;

        return cmd;
    }
};

} // namespace PlayerSys

#endif // INPUT_MANAGER_H
