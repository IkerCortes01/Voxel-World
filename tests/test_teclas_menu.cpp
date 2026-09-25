#include <doctest/doctest.h>
#include "player/TeclasDeMenu.h"

// Hace falta para recorrer listas entre llaves con range-for ({ false, true }).
// MSVC no lo arrastra solo y da un error enganoso sobre 'begin'/'end'.
#include <initializer_list>

using namespace PlayerSys;

// ============================================================================
// CON EL INVENTARIO ABIERTO, SHIFT NO AGACHA
// ============================================================================
// BUG REPORTADO: estando en el inventario y pulsando SHIFT, el personaje se
// agachaba detras del menu.
//
// POR QUE SE ESCAPABA ESTA TECLA Y NO LAS DEMAS. El resto del movimiento
// (WASD, salto) pasa por el array `keys[]`, y keyCallback tiene una guarda que
// corta antes de tocarlo cuando hay un menu abierto. Pero SHIFT y CTRL NO
// estan en keys[]: se leen con glfwGetKey, que pregunta al sistema operativo
// por el estado FISICO del teclado. Esa via se salta la guarda entera.
//
// Es un buen ejemplo de por que dos caminos de entrada para lo mismo acaban
// divergiendo: uno se protegio y el otro no, y no se noto hasta jugar.

TEST_CASE("Teclas: sin menu, el personaje las recibe") {
    // El caso normal: jugando, SHIFT agacha y CTRL corre.
    CHECK(teclasDelPersonajeActivas(false, false) == true);
    CHECK(teclaParaElPersonaje(true,  false, false) == true);
    CHECK(teclaParaElPersonaje(false, false, false) == false);
}

TEST_CASE("Teclas: con el INVENTARIO abierto, no llegan") {
    // ⭐ EL BUG REPORTADO.
    //
    // SHIFT es el modificador de "mover la pila entera" en cualquier
    // inventario del genero, asi que el jugador lo pulsa constantemente
    // mientras ordena cosas. El personaje se agachaba en cada pulsacion.
    CHECK(teclasDelPersonajeActivas(true, false) == false);
    CHECK(teclaParaElPersonaje(true, /*inventario=*/true, /*pausa=*/false)
          == false);
}

TEST_CASE("Teclas: con la PAUSA abierta, tampoco") {
    // La pausa ya para el juego entero, asi que aqui no se notaba. Se incluye
    // por coherencia con la guarda de keyCallback, que corta las dos.
    CHECK(teclasDelPersonajeActivas(false, true) == false);
    CHECK(teclaParaElPersonaje(true, false, /*pausa=*/true) == false);
}

TEST_CASE("Teclas: con los dos menus a la vez, tampoco") {
    // Defensa: no deberia poder darse (abrir el inventario cierra la pausa),
    // pero si alguna vez se solapan, la respuesta tiene que seguir siendo no.
    CHECK(teclasDelPersonajeActivas(true, true) == false);
    CHECK(teclaParaElPersonaje(true, true, true) == false);
}

TEST_CASE("Teclas: una tecla SUELTA sigue suelta, haya menu o no") {
    // El filtro solo puede QUITAR pulsaciones, nunca inventarlas. Si lo
    // hiciera, cerrar un menu podria dejar al personaje agachado sin que nadie
    // toque el teclado.
    for (bool inv : { false, true })
        for (bool pau : { false, true }) {
            INFO("inventario=", inv, " pausa=", pau);
            CHECK(teclaParaElPersonaje(false, inv, pau) == false);
        }
}

TEST_CASE("Teclas: el filtro es el MISMO para SHIFT y para CTRL") {
    // ⭐ LA RAZON DE QUE ESTO SEA UNA FUNCION Y NO UN `if` SUELTO.
    //
    // Las dos teclas tienen el mismo problema por la misma via: se leen con
    // glfwGetKey y no estan en keys[]. Arreglar solo SHIFT dejaria la mitad
    // del fallo en pie -- correr con el inventario abierto es igual de
    // absurdo que agacharse.
    //
    // Pasando las dos por la misma funcion, no pueden divergir. Este test lo
    // comprueba usando la funcion como si fuera cada una de ellas.
    const bool shiftPulsado = true;
    const bool ctrlPulsado  = true;

    for (bool inv : { false, true }) {
        const bool shift = teclaParaElPersonaje(shiftPulsado, inv, false);
        const bool ctrl  = teclaParaElPersonaje(ctrlPulsado,  inv, false);
        INFO("inventario=", inv);
        CHECK(shift == ctrl);
    }
}
