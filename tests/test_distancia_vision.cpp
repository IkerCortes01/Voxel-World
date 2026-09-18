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

// ============================================================================
// DIFUMINADO DE RENDERIZADO POR DISTANCIA
// ============================================================================
// Da, por chunk, cuanto lo va a tapar la niebla: 0 nitido, 1 fundido del todo.
// Sirve para dos cosas a la vez -- que el terreno lejano entre desvaneciendose
// en vez de aparecer de golpe, y descartar lo que ya es indistinguible del
// color del cielo (pixeles que cuestan y no se ven).

TEST_CASE("Difuminado: lo cercano esta nitido") {
    // Lo que el jugador tiene delante no se toca nunca, sea cual sea la barra.
    for (int barra : { 2, 8, 16, 40, 70, 100 }) {
        const int r = radioCargado(barra);
        CHECK(difuminadoDeChunk(barra, 0.0f, r) == doctest::Approx(0.0f));
        CHECK(difuminadoDeChunk(barra, 1.0f, r) < 0.2f);
    }
}

TEST_CASE("Difuminado: crece con la distancia, nunca al reves") {
    // Un chunk mas lejos NUNCA puede verse mas nitido que uno mas cerca.
    const int barra = 40;
    const int r = radioCargado(barra);
    float anterior = -1.0f;
    for (float d = 0.0f; d <= (float)r; d += 0.25f) {
        const float dif = difuminadoDeChunk(barra, d, r);
        CHECK(dif >= anterior - 0.0001f);
        CHECK(dif >= 0.0f);
        CHECK(dif <= 1.0f);
        anterior = dif;
    }
}

TEST_CASE("Difuminado: en el borde de lo cargado esta fundido del todo") {
    // ⭐ ES LO QUE EVITA EL BORDE RECTO DE TERRENO.
    //
    // Si el ultimo anillo llegara nitido, se veria el corte de lo cargado --
    // justo lo que la niebla existe para tapar. Al llegar a 1 en el borde, el
    // chunk se ha fundido antes de acabarse el mundo.
    for (int barra : { 8, 40, 100 }) {
        const int r = radioCargado(barra);
        CHECK(difuminadoDeChunk(barra, (float)r, r) == doctest::Approx(1.0f));
    }
}

TEST_CASE("Difuminado: va acompasado con la niebla") {
    // Los dos salen de nieblaInicioFraccion, asi que no pueden separarse: donde
    // la niebla empieza a cerrar, el difuminado empieza a subir. Si se
    // calcularan aparte, habria un tramo con niebla y sin difuminado (o al
    // reves) y se veria como un escalon.
    const int barra = 70;
    const int r = radioCargado(barra);
    const float inicio = (float)r * nieblaInicioFraccion(barra);

    // Justo antes de que empiece la niebla: nitido.
    CHECK(difuminadoDeChunk(barra, inicio - 0.5f, r) == doctest::Approx(0.0f));
    // Justo despues: ya ha empezado.
    CHECK(difuminadoDeChunk(barra, inicio + 0.5f, r) > 0.0f);
}

TEST_CASE("Difuminado: con la barra alta tapa antes que con la baja") {
    // Es lo que permite pedir mucha distancia sin pagarla entera: cuanto mas
    // lejos quiere ver el jugador, antes empieza la bruma -- igual que la
    // perspectiva aerea real.
    //
    // Se compara a la MISMA fraccion del radio de cada uno, que es la
    // comparacion justa (los radios son distintos).
    const int rBaja = radioCargado(8);
    const int rAlta = radioCargado(100);
    const float difBaja = difuminadoDeChunk(8,   (float)rBaja * 0.6f, rBaja);
    const float difAlta = difuminadoDeChunk(100, (float)rAlta * 0.6f, rAlta);
    CHECK(difAlta > difBaja);
}

TEST_CASE("Difuminado: lo invisible se descarta, lo demas no") {
    // El umbral esta en 0.97 y no en 1.0 porque el ultimo 3% ya no aporta nada
    // perceptible, y es donde MAS chunks hay (el area crece con el cuadrado).
    CHECK(chunkInvisiblePorNiebla(1.0f)  == true);
    CHECK(chunkInvisiblePorNiebla(0.99f) == true);
    CHECK(chunkInvisiblePorNiebla(0.90f) == false);
    CHECK(chunkInvisiblePorNiebla(0.5f)  == false);
    CHECK(chunkInvisiblePorNiebla(0.0f)  == false);
}

TEST_CASE("Difuminado: la curva es suave, sin bandas visibles") {
    // Una rampa lineal produce una "banda" donde arranca, porque el ojo detecta
    // los cambios de PENDIENTE, no solo de valor. Con smoothstep la derivada es
    // nula en los dos extremos.
    //
    // Se comprueba que no hay ningun salto brusco entre pasos contiguos.
    const int barra = 40;
    const int r = radioCargado(barra);
    float anterior = difuminadoDeChunk(barra, 0.0f, r);
    for (float d = 0.1f; d <= (float)r; d += 0.1f) {
        const float dif = difuminadoDeChunk(barra, d, r);
        CHECK(dif - anterior < 0.08f);   // sin escalones
        anterior = dif;
    }
}

TEST_CASE("Difuminado: radio invalido no rompe nada") {
    // Defensa: durante una carga el radio puede ser 0 un instante.
    CHECK(difuminadoDeChunk(8, 5.0f, 0) == doctest::Approx(0.0f));
    CHECK(difuminadoDeChunk(8, 5.0f, -3) == doctest::Approx(0.0f));
}
