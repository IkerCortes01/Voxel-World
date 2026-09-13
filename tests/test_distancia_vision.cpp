#include <doctest/doctest.h>
#include "render/DistanciaVision.h"
#include <string>

using namespace Render;

// ============================================================================
// LA BARRA DE DISTANCIA: 2 A 100 SIN MATAR LA MAQUINA
// ============================================================================
// Lo que se pidio: una barra de 2 a 100, con difuminado progresivo que imite
// como el ojo pierde detalle en la distancia, y que a 100 el juego siga yendo
// a ~100 FPS sin comerse la RAM.
//
// Lo que NO se puede hacer: cargar 100 chunks de radio de verdad. Son 40.401
// columnas, ~8 GB. Estos tests fijan el compromiso.

TEST_CASE("Barra: se acota a los topes pedidos") {
    CHECK(acotarBarra(0)    == DISTANCIA_BARRA_MIN);
    CHECK(acotarBarra(-50)  == DISTANCIA_BARRA_MIN);
    CHECK(acotarBarra(1)    == 2);
    CHECK(acotarBarra(500)  == DISTANCIA_BARRA_MAX);
    CHECK(DISTANCIA_BARRA_MIN == 2);
    CHECK(DISTANCIA_BARRA_MAX == 100);
}

TEST_CASE("Radio: hasta 16 la barra es literal") {
    // En el rango normal de juego, pedir 8 tiene que cargar 8. Sin sorpresas.
    for (int b = 2; b <= 16; ++b) {
        CHECK(radioCargado(b) == b);
    }
}

TEST_CASE("Radio: por encima de 16 crece, pero MUCHO mas despacio") {
    // El area crece con el cuadrado, asi que el radio real no puede seguir a la
    // barra. Pero tiene que seguir CRECIENDO: si 70 y 100 cargaran lo mismo, la
    // mitad de la barra no serviria para nada.
    CHECK(radioCargado(40) > radioCargado(16));
    CHECK(radioCargado(70) > radioCargado(40));
    CHECK(radioCargado(100) > radioCargado(70));

    // Y muy por debajo de la barra: es lo que protege la memoria.
    CHECK(radioCargado(100) < 40);
}

TEST_CASE("Radio: el tope de memoria se respeta") {
    // ~3.249 columnas a 200 KB son ~650 MB. Pasar de ahi es pedir un crash por
    // falta de memoria en una maquina normal.
    const int r = radioCargado(DISTANCIA_BARRA_MAX);
    const int columnas = (2 * r + 1) * (2 * r + 1);
    CHECK(r <= 28);
    CHECK(columnas <= 3400);
}

TEST_CASE("Radio: nunca decrece al subir la barra") {
    // Una barra que a veces carga MENOS al pedir mas seria incomprensible.
    for (int b = DISTANCIA_BARRA_MIN; b < DISTANCIA_BARRA_MAX; ++b) {
        CHECK(radioCargado(b + 1) >= radioCargado(b));
    }
}

TEST_CASE("Niebla: se calibra con el radio REAL, no con la barra") {
    // ⭐ ESTE ES EL TEST QUE PROTEGE CONTRA EL BORDE VISIBLE.
    //
    // Si la niebla se abriera hasta donde dice la barra (100 chunks = 1.600
    // bloques) mientras el mundo solo llega a 28 chunks (448 bloques), se veria
    // el corte de lo cargado: un borde recto de terreno flotando en el vacio.
    const int barra = 100;
    const int r = radioCargado(barra);
    const float fin = nieblaFin(barra, r);

    // La niebla cierra DENTRO de lo cargado.
    CHECK(fin <= (float)r * 16.0f);
    // Y muy por debajo de lo que la barra sugiere.
    CHECK(fin < (float)barra * 16.0f);
}

TEST_CASE("Difuminado: a 40 es leve, a 100 es fuerte") {
    // Lo que se pidio literalmente: "a los 40 chunks haga que difumine un poco
    // pero sea poco notorio", y "al maximo a los 70,80,90 y 100 sea totalmente
    // difuminado".
    //
    // Menor fraccion de inicio = la niebla empieza antes = mas difuminado.
    const float f16  = nieblaInicioFraccion(16);
    const float f40  = nieblaInicioFraccion(40);
    const float f70  = nieblaInicioFraccion(70);
    const float f100 = nieblaInicioFraccion(100);

    // En el rango normal la niebla casi no se nota: solo el ultimo cuarto.
    CHECK(f16 >= 0.70f);

    // A 40, ya se nota -- pero el terreno sigue leyendose en mas de la mitad.
    CHECK(f40 < f16);
    CHECK(f40 > 0.45f);

    // A 70 y 100, el fondo es bruma.
    CHECK(f70 < f40);
    CHECK(f100 < f70);
    CHECK(f100 <= 0.30f);
}

TEST_CASE("Difuminado: crece de forma continua, sin saltos") {
    // Un salto brusco entre dos valores contiguos de la barra se veria como un
    // parpadeo de la niebla al mover el ajuste.
    for (int b = DISTANCIA_BARRA_MIN; b < DISTANCIA_BARRA_MAX; ++b) {
        const float a = nieblaInicioFraccion(b);
        const float c = nieblaInicioFraccion(b + 1);
        CHECK(c <= a + 0.0001f);          // nunca sube
        CHECK(a - c < 0.05f);             // y no da saltos
    }
}

TEST_CASE("Simplificado: por debajo de 41 NO se simplifica nada") {
    // Hasta 40 el terreno se ve entero, como se pidio ("17-40 completo").
    CHECK(simplificarVegetacion(16, 10.0f) == false);
    CHECK(simplificarVegetacion(40, 30.0f) == false);
    CHECK(simplificarVegetacion(40, 100.0f) == false);
}

TEST_CASE("Simplificado: de 41 en adelante, solo LO LEJANO") {
    // Lo cercano nunca se simplifica: es donde el jugador esta mirando.
    CHECK(simplificarVegetacion(100, 2.0f) == false);
    CHECK(simplificarVegetacion(100, 5.0f) == false);

    // Lo lejano si.
    CHECK(simplificarVegetacion(100, 25.0f) == true);
    CHECK(simplificarVegetacion(70, 25.0f) == true);
}

TEST_CASE("Simplificado: el umbral nunca baja de 8 chunks") {
    // Simplificar a 3 chunks del jugador se veria: el ocote de al lado perderia
    // sus aciculas de golpe.
    for (int b = 41; b <= DISTANCIA_BARRA_MAX; ++b) {
        CHECK(distanciaSimplificado(b) >= 8);
    }
}

TEST_CASE("Etiqueta: cada tramo tiene nombre") {
    // La barra sin texto es un numero sin significado.
    CHECK(std::string(nombreCalidad(2))   == "Cerca");
    CHECK(std::string(nombreCalidad(8))   == "Normal");
    CHECK(std::string(nombreCalidad(30))  == "Lejos");
    CHECK(std::string(nombreCalidad(60))  == "Muy lejos");
    CHECK(std::string(nombreCalidad(100)) == "Extremo");
}
