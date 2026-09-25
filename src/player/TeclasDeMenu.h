#pragma once

// ============================================================================
// CON UN MENU DELANTE, LAS TECLAS SON DEL MENU
// ============================================================================
// BUG REPORTADO: estando en el inventario y pulsando SHIFT, el personaje se
// agachaba detras del menu.
//
// ----------------------------------------------------------------------------
// POR QUE SE ESCAPABA ESTA TECLA Y NO LAS DEMAS
// ----------------------------------------------------------------------------
// El resto del movimiento --WASD y el salto-- pasa por el array `keys[]`, y
// `keyCallback` tiene esta guarda antes de tocarlo:
//
//     if (isPaused || inventoryOpen) return;   // no procesar movimiento
//
// asi que con un menu abierto esas teclas ni llegan a registrarse.
//
// Pero SHIFT y CTRL NO estan en keys[]: se leen directamente con glfwGetKey,
// que pregunta al sistema operativo por el estado FISICO del teclado. Esa via
// no pasa por keyCallback, asi que se salta la guarda entera.
//
// Es un buen ejemplo de por que dos caminos de entrada para lo mismo acaban
// divergiendo: uno se protegio y el otro no, y nadie lo noto hasta jugar.
//
// ----------------------------------------------------------------------------
// POR QUE SHIFT ES JUSTO LA PEOR TECLA PARA QUE PASE ESTO
// ----------------------------------------------------------------------------
// SHIFT es el modificador de "mover la pila entera" en cualquier inventario
// del genero. O sea que el jugador lo pulsa CONSTANTEMENTE mientras ordena
// cosas -- y el personaje se agachaba en cada pulsacion.
//
// CTRL entra por lo mismo: se lee igual, tampoco esta en keys[], y correr con
// el inventario abierto es igual de absurdo que agacharse. Arreglar solo SHIFT
// dejaria la mitad del fallo en pie.
// ============================================================================

namespace PlayerSys {

// ¿Debe llegar al personaje una tecla que se lee por estado fisico del teclado
// (SHIFT, CTRL) cuando hay un menu abierto?
//
// `inventario` y `pausa` son los dos menus que capturan el teclado. Se pasan
// por separado --en vez de un solo bool-- porque quien llama los tiene
// separados y unirlos aqui deja la intencion mas clara en el sitio de uso.
inline bool teclasDelPersonajeActivas(bool inventario, bool pausa) {
    return !inventario && !pausa;
}

// Aplica la regla a una tecla concreta. Devuelve el estado que debe ver el
// personaje: la tecla tal cual si no hay menu, o "suelta" si lo hay.
//
// Existe como funcion --en vez de un `if` suelto-- para que SHIFT y CTRL se
// filtren exactamente igual. Cuando el filtro se escribe dos veces, tarde o
// temprano uno de los dos se queda sin actualizar.
inline bool teclaParaElPersonaje(bool pulsada, bool inventario, bool pausa) {
    return pulsada && teclasDelPersonajeActivas(inventario, pausa);
}

} // namespace PlayerSys
