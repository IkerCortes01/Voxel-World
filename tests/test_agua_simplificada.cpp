#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include <vector>
#include <cmath>

// ============================================================================
// LA SIMPLIFICACION DEL AGUA
// ============================================================================
// Tres cambios, y cada uno tiene aqui su red:
//
//   1. RADIO DE 8. El agua solo se expande hasta 8 bloques de donde nacio.
//      Antes el reparto lateral no tenia tope y una sola celda podia acabar
//      mojando media llanura.
//
//   2. SIN GRAVEDAD. El nivel baja de golpe en vez de escurrir 2 octavos por
//      tick. No hay aceleracion ni velocidad: solo el nivel cayendo rapido.
//
//   3. ALTURA REAL AL DIBUJAR. Una celda con 3 octavos se ve a 3/8 de alto,
//      no como un cubo lleno.
//
// El motor de flujo vive dentro de World, que necesita OpenGL y no se puede
// instanciar en un test. Se sigue el patron que ya usa test_agua_flujo.cpp:
// reproducir las REGLAS exactamente como las aplica updateWaterFlow, que es
// aritmetica sobre niveles y no depende de World.
//
// Lo que NO puede romperse pase lo que pase: el agua no se crea ni se
// destruye. Todos los tests de abajo lo comprueban de paso.
// ============================================================================

using namespace Compuesto;

namespace {

// La distancia con la que se mide el radio: Chebyshev, el mayor de los dos
// ejes. Es la misma que aplica updateWaterFlow, y dibuja un CUADRADO de 17x17
// alrededor del origen.
//
// Se eligio esta y no la euclidea a proposito: con un circulo, el agua llegaria
// mas lejos en recto que en diagonal, que es justo lo que despista al construir
// un estanque.
int distChebyshev(int ax, int az, int bx, int bz) {
    const int dx = std::abs(ax - bx);
    const int dz = std::abs(az - bz);
    return (dx > dz) ? dx : dz;
}

constexpr int RADIO = 8;

// El reparto lateral tal como quedo: igual que antes, pero el destino tiene
// que estar dentro del radio medido DESDE EL ORIGEN.
bool puedeRepartirse(int destX, int destZ, int origenX, int origenZ) {
    return distChebyshev(destX, destZ, origenX, origenZ) <= RADIO;
}

// La caida, ya sin caudal limitado: baja TODO lo que quepa.
// Devuelve cuanto bajo.
int caerDeGolpe(int& nivelArriba, int& nivelAbajo) {
    const int hueco = (int)Agua::LLENA - nivelAbajo;
    if (hueco <= 0) return 0;
    const int baja = (nivelArriba < hueco) ? nivelArriba : hueco;
    nivelArriba -= baja;
    nivelAbajo  += baja;
    return baja;
}

// La altura a la que el mesher dibuja una celda, en fraccion de bloque.
// Reproduce la rama nueva del mesher.
float alturaDibujada(int nivel, bool cayendo) {
    if (nivel >= (int)Agua::LLENA || cayendo) return 1.0f;
    return (float)nivel / (float)Agua::LLENA;
}

} // namespace

// ============================================================================
// 1. EL RADIO DE 8 BLOQUES
// ============================================================================

TEST_CASE("Radio: el agua no pasa de 8 bloques desde su origen") {
    const int ox = 100, oz = 100;

    // Justo en el borde: entra.
    CHECK(puedeRepartirse(ox + 8, oz,     ox, oz));
    CHECK(puedeRepartirse(ox - 8, oz,     ox, oz));
    CHECK(puedeRepartirse(ox,     oz + 8, ox, oz));
    CHECK(puedeRepartirse(ox,     oz - 8, ox, oz));

    // Un bloque mas alla: no.
    CHECK_FALSE(puedeRepartirse(ox + 9, oz,     ox, oz));
    CHECK_FALSE(puedeRepartirse(ox - 9, oz,     ox, oz));
    CHECK_FALSE(puedeRepartirse(ox,     oz + 9, ox, oz));
    CHECK_FALSE(puedeRepartirse(ox,     oz - 9, ox, oz));
}

TEST_CASE("Radio: la frontera es un CUADRADO, no un circulo") {
    // Con distancia euclidea, la esquina (8,8) quedaria a 11.3 y se
    // rechazaria: el agua llegaria a 8 en recto pero solo a 5 en diagonal.
    // Con Chebyshev la esquina entra, y el area mojada es un cuadrado limpio.
    const int ox = 0, oz = 0;

    CHECK(puedeRepartirse(8, 8, ox, oz));     // la esquina ENTRA
    CHECK(puedeRepartirse(-8, 8, ox, oz));
    CHECK(puedeRepartirse(8, -8, ox, oz));
    CHECK(puedeRepartirse(-8, -8, ox, oz));

    // Y lo de mas alla de la esquina, no.
    CHECK_FALSE(puedeRepartirse(9, 9, ox, oz));

    // Comprobacion de que NO se esta usando la euclidea: si se usara, esta
    // distancia (11.31) superaria 8 y la esquina se habria rechazado arriba.
    const float euclidea = std::sqrt(8.0f * 8.0f + 8.0f * 8.0f);
    CHECK(euclidea > (float)RADIO);
}

TEST_CASE("Radio: el area mojada esta acotada, no crece sin fin") {
    // La propiedad que de verdad importa: cuantas celdas puede llegar a mojar
    // como maximo un derrame. Antes no habia respuesta -- dependia del terreno
    // y podia ser toda la llanura.
    const int ox = 0, oz = 0;
    int dentro = 0;

    for (int x = -20; x <= 20; ++x)
        for (int z = -20; z <= 20; ++z)
            if (puedeRepartirse(x, z, ox, oz)) ++dentro;

    // 17x17 = 289 celdas. Ni una mas.
    CHECK(dentro == 17 * 17);
}

TEST_CASE("Radio: heredar el origen es lo que impide la expansion infinita") {
    // EL FALLO QUE ESTO PREVIENE: si cada celda fuera su propio origen, el
    // agua avanzaria de 8 en 8 para siempre -- el limite no serviria de nada.
    //
    // Se simula una cadena de saltos. Con herencia, la cadena se para; sin
    // ella, seguiria indefinidamente.
    const int ox = 0, oz = 0;

    // CON herencia (lo correcto): el origen no cambia nunca.
    int x = 0;
    int saltos = 0;
    while (puedeRepartirse(x + 1, 0, ox, oz) && saltos < 1000) {
        ++x;
        ++saltos;
    }
    CHECK(x == RADIO);          // se para exactamente en 8
    CHECK(saltos < 1000);       // y se para de verdad

    // SIN herencia (el fallo): cada celda seria origen de si misma y la
    // condicion se cumpliria siempre.
    int y = 0;
    int saltosMalos = 0;
    while (distChebyshev(y + 1, 0, y, 0) <= RADIO && saltosMalos < 1000) {
        ++y;
        ++saltosMalos;
    }
    CHECK(saltosMalos == 1000);   // nunca se para: por eso hace falta heredar
}

// ============================================================================
// 2. LA CAIDA SIN GRAVEDAD
// ============================================================================

TEST_CASE("Caida: una celda llena se vacia en UN solo tick") {
    // Antes bajaban 2 octavos por tick (CAUDAL_CAIDA), asi que una celda llena
    // tardaba 4 ticks -- unos 2 segundos -- en descolgarse. Ahora baja entero.
    int arriba = 8, abajo = 0;

    const int bajo = caerDeGolpe(arriba, abajo);

    CHECK(bajo == 8);
    CHECK(arriba == 0);
    CHECK(abajo == 8);
}

TEST_CASE("Caida: no se crea ni se destruye agua") {
    // La regla de la que cuelga todo el sistema, comprobada en la ruta que
    // mas cambio.
    for (int a = 1; a <= 8; ++a) {
        for (int b = 0; b <= 8; ++b) {
            int arriba = a, abajo = b;
            const int total = arriba + abajo;

            caerDeGolpe(arriba, abajo);

            CHECK(arriba + abajo == total);
            CHECK(arriba >= 0);
            CHECK(abajo <= (int)Agua::LLENA);
        }
    }
}

TEST_CASE("Caida: si abajo no cabe todo, arriba se queda el resto") {
    // 5 octavos cayendo sobre una celda que ya tiene 6: solo caben 2.
    int arriba = 5, abajo = 6;

    const int bajo = caerDeGolpe(arriba, abajo);

    CHECK(bajo == 2);
    CHECK(abajo == 8);      // llena
    CHECK(arriba == 3);     // el resto se queda
    CHECK(arriba + abajo == 11);
}

TEST_CASE("Caida: sobre celda llena no baja nada") {
    int arriba = 4, abajo = 8;

    const int bajo = caerDeGolpe(arriba, abajo);

    CHECK(bajo == 0);
    CHECK(arriba == 4);
    CHECK(abajo == 8);
}

TEST_CASE("Caida: una columna se drena en pocos pasos, no en decenas") {
    // Una cascada de 10 bloques con la celda de arriba llena. Con el caudal
    // limitado de antes esto tardaba ~4 ticks por bloque; ahora el agua llega
    // al fondo en una sola pasada por columna.
    std::vector<int> col(10, 0);
    col[9] = 8;   // arriba del todo

    const int total = 8;
    int pasadas = 0;

    // Se recorre de abajo arriba, como hace el simulador al procesar la cola.
    bool movio = true;
    while (movio && pasadas < 100) {
        movio = false;
        for (int y = 9; y > 0; --y) {
            if (col[y] > 0 && caerDeGolpe(col[y], col[y - 1]) > 0) movio = true;
        }
        ++pasadas;

        int suma = 0;
        for (int v : col) suma += v;
        CHECK(suma == total);     // conservacion en CADA pasada
    }

    CHECK(col[0] == 8);           // todo el agua acabo abajo
    CHECK(pasadas < 5);           // y en muy pocas pasadas
}

// ============================================================================
// 3. EL DIBUJO A LA ALTURA DEL NIVEL
// ============================================================================

TEST_CASE("Dibujo: cada nivel se dibuja a su altura") {
    // EL FALLO QUE ESTO FIJA: una celda con 1 octavo se veia como un CUBO
    // LLENO. El nivel existia, se guardaba y se simulaba, pero no se veia.
    //
    // La causa era que esNivelParcial() solo mira la tabla de niveles del
    // TERRENO, y el agua con volumen vive en el espacio de compuestos: nunca
    // entraba en la rama de altura reducida.
    CHECK(alturaDibujada(1, false) == doctest::Approx(0.125f));
    CHECK(alturaDibujada(2, false) == doctest::Approx(0.250f));
    CHECK(alturaDibujada(4, false) == doctest::Approx(0.500f));
    CHECK(alturaDibujada(6, false) == doctest::Approx(0.750f));
    CHECK(alturaDibujada(7, false) == doctest::Approx(0.875f));

    // 8 octavos = celda llena = el voxel entero.
    CHECK(alturaDibujada(8, false) == doctest::Approx(1.0f));
}

TEST_CASE("Dibujo: la altura crece con el nivel, sin saltos") {
    float anterior = 0.0f;
    for (int n = 1; n <= 8; ++n) {
        const float h = alturaDibujada(n, false);
        CHECK(h > anterior);        // estrictamente creciente
        CHECK(h > 0.0f);
        CHECK(h <= 1.0f);
        anterior = h;
    }
}

TEST_CASE("Dibujo: el agua que CAE llena el voxel aunque tenga poco nivel") {
    // Un chorro fino sigue siendo un chorro de arriba abajo, no un charco
    // flotando en el aire a media altura. Es lo que ya decia el bit CAYENDO de
    // BloqueCompuesto.h y que el mesher no respetaba.
    for (int n = 1; n <= 8; ++n) {
        CHECK(alturaDibujada(n, true) == doctest::Approx(1.0f));
    }

    // Y en reposo, el mismo nivel bajo SI se ve bajo.
    CHECK(alturaDibujada(1, false) < 0.2f);
}

TEST_CASE("Dibujo: la altura sale del ID, sin datos aparte") {
    // El nivel viaja DENTRO del ID del bloque, asi que el mesher lo lee del
    // propio voxel: no hay mapa lateral que consultar ni que purgar.
    for (int n = 1; n <= 8; ++n) {
        const BlockType b = Agua::nuevo((uint16_t)n);
        REQUIRE(Agua::esAgua(b));
        CHECK((int)Agua::nivelDe(b) == n);
        CHECK(alturaDibujada((int)Agua::nivelDe(b), Agua::estaCayendo(b))
              == doctest::Approx((float)n / 8.0f));
    }
}

TEST_CASE("Dibujo: el agua que cae se marca y se distingue") {
    const BlockType cae    = Agua::nuevo(3, true);
    const BlockType quieta = Agua::nuevo(3, false);

    CHECK(Agua::estaCayendo(cae));
    CHECK_FALSE(Agua::estaCayendo(quieta));

    // Mismo nivel, distinta altura dibujada.
    CHECK((int)Agua::nivelDe(cae) == (int)Agua::nivelDe(quieta));
    CHECK(alturaDibujada(3, true) > alturaDibujada(3, false));
}
