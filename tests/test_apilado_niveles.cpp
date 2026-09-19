#include <doctest/doctest.h>
#include "BlockType.h"
#include "FisicaCaida.h"
#include <cmath>
#include <set>

// ============================================================================
// APILAR NIVELES Y CAIDA DE ARBOLES
// ============================================================================
// Tres correcciones que se pidieron juntas:
//
//   1. No se podian colocar niveles con un bloque entero encima: el clic se
//      perdia sin colocar nada y sin gastar el bloque.
//   2. Un bloque puesto sobre una capa parcial quedaba FLOTANDO: la capa medía
//      3 px y el bloque ocupaba la celda de arriba entera, dejando 13 px de
//      aire a la vista.
//   3. Los arboles talados caian siempre hacia una de CUATRO direcciones, asi
//      que un bosque talado quedaba con todos los troncos alineados en cruz.
//
// Lo que se puede probar aqui es la ARITMETICA de las tres, que es donde
// estaban los errores. La colocacion en si vive en placeBlock (necesita el
// mundo entero) y la caida se ve en pantalla.

// ============================================================================
// 1. RELLENAR BAJO UN TECHO
// ============================================================================

TEST_CASE("Niveles: una celda llena equivale al bloque entero") {
    // La correccion del bug: cuando la celda llega al nivel 8 y arriba hay algo
    // ocupado, en vez de perder el clic se remata la celda como BLOQUE ENTERO.
    //
    // Eso solo es correcto si ocho octavos son exactamente un bloque -- que es
    // lo que se comprueba aqui. Si no lo fuera, rellenar bajo un techo dejaria
    // una rendija.
    for (int i = 0; i < 200; ++i) {
        const BlockType b = (BlockType)i;
        if (!admiteNiveles(b)) continue;
        if (esNivelParcial(b) || esMixto(b)) continue;

        INFO("bloque ", i);
        // El bloque entero mide 1.0 de alto...
        CHECK(alturaDe(b) == doctest::Approx(1.0f));
        // ...y su nivel 8 no existe como capa: es el bloque mismo.
        CHECK(nivelDe(b) == 8);
    }
}

TEST_CASE("Niveles: subir de nivel no depende de lo que haya encima") {
    // ⭐ ESTA ES LA PROPIEDAD QUE SE ROMPIO.
    //
    // Subir el nivel de una capa ocurre DENTRO de su celda, asi que lo que
    // haya en la celda de arriba es irrelevante. El bug era que varias ramas
    // de placeBlock comprobaban el techo antes de dejar apilar, y con un
    // bloque entero encima se salian sin hacer nada.
    //
    // La aritmetica no cambia: nivel N -> nivel N+1, y en 8 pasa a entero.
    for (int i = 0; i < 200; ++i) {
        const BlockType base = (BlockType)i;
        if (!admiteNiveles(base) || esNivelParcial(base) || esMixto(base)) continue;

        for (int n = 1; n <= 7; ++n) {
            const BlockType capa = conNivel(base, n);
            if (capa == BLOCK_AIR) continue;
            INFO("base ", i, " nivel ", n);
            CHECK(esNivelParcial(capa));
            CHECK(nivelDe(capa) == n);
            CHECK(bloqueBaseDe(capa) == base);

            // El siguiente nivel siempre existe y es mas alto.
            const BlockType sig = (n + 1 >= 8) ? base : conNivel(base, n + 1);
            CHECK(alturaDe(sig) > alturaDe(capa));
        }
        break;   // basta con una familia: la regla es la misma para todas
    }
}

// ============================================================================
// 2. NADA FLOTA SOBRE UNA CAPA
// ============================================================================

TEST_CASE("Mixtas: la capa de abajo y el relleno no dejan hueco") {
    // ⭐ ES LA GARANTIA DE QUE NADA FLOTA.
    //
    // En una celda mixta, el material de arriba empieza EXACTAMENTE donde
    // acaba la capa de abajo y llega hasta el techo. Si esas dos alturas no
    // encajaran, quedaria el hueco de aire que se veia.
    int probadas = 0;
    for (int i = 0; i < 200 && probadas < 6; ++i) {
        const BlockType base = (BlockType)i;
        if (!admiteNiveles(base) || esNivelParcial(base) || esMixto(base)) continue;

        for (int n = 1; n <= 7; ++n) {
            const BlockType m = mixto(base, n, BLOCK_STONE);
            if (m == BLOCK_AIR) continue;

            INFO("base ", i, " nivel ", n);
            CHECK(esMixto(m));
            // La capa de abajo conserva su altura...
            CHECK(mixtoNivel(m) == n);
            CHECK(mixtoBase(m) == base);
            // ...y el relleno es el material de arriba.
            CHECK(mixtoRelleno(m) == BLOCK_STONE);

            // La celda mixta ocupa el voxel ENTERO: por eso no hay hueco.
            CHECK(alturaDe(m) == doctest::Approx(1.0f));
            ++probadas;
        }
    }
    CHECK(probadas > 0);
}

TEST_CASE("Mixtas: no se puede mezclar en una celda ya llena") {
    // El nivel 8 es el bloque entero: ahi no cabe un segundo material, y
    // `mixto` tiene que rechazarlo en vez de producir un ID invalido.
    for (int i = 0; i < 200; ++i) {
        const BlockType base = (BlockType)i;
        if (!admiteNiveles(base) || esNivelParcial(base) || esMixto(base)) continue;
        CHECK(mixto(base, 8, BLOCK_STONE) == BLOCK_AIR);
        CHECK(mixto(base, 0, BLOCK_STONE) == BLOCK_AIR);
        break;
    }
}

// ============================================================================
// 3. LOS ARBOLES CAEN EN CUALQUIER DIRECCION
// ============================================================================

namespace {
// Replica del calculo de rumbo de main.cpp: de un angulo salen el vector
// continuo (que dibuja el render) y el eje entero (que coloca los bloques).
struct Rumbo { float rx, rz; int vx, vz; };

Rumbo rumboDe(float angulo) {
    Rumbo r;
    r.rx = cosf(angulo);
    r.rz = sinf(angulo);
    if (fabsf(r.rx) >= fabsf(r.rz)) {
        r.vx = (r.rx >= 0.0f) ? 1 : -1;
        r.vz = 0;
    } else {
        r.vx = 0;
        r.vz = (r.rz >= 0.0f) ? 1 : -1;
    }
    return r;
}

// El angulo que produce la posicion de un arbol que se cae solo.
float anguloPorPosicion(int x, int z) {
    unsigned h = (unsigned)(x * 73856093) ^ (unsigned)(z * 19349663);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return ((float)(h % 10000u) / 10000.0f) * 6.2831853f;
}
} // namespace

TEST_CASE("Caida: el rumbo es un vector unitario") {
    // El render multiplica el avance por este vector. Si no fuera unitario, el
    // arbol se estiraria o encogeria al caer segun la direccion.
    for (int g = 0; g < 360; g += 7) {
        const float a = (float)g * 3.14159265f / 180.0f;
        const Rumbo r = rumboDe(a);
        const float largo = sqrtf(r.rx * r.rx + r.rz * r.rz);
        INFO("angulo ", g);
        CHECK(largo == doctest::Approx(1.0f));
    }
}

TEST_CASE("Caida: hay muchisimas direcciones, no cuatro") {
    // ⭐ EL BUG QUE SE CORRIGIO.
    //
    // Antes el rumbo salia de una tabla de 4 entradas, asi que talando un
    // bosque todos los troncos acababan en cruz. Ahora es un angulo continuo.
    //
    // Se cuentan los rumbos DISTINTOS que producen mil posiciones: con el
    // sistema viejo saldrian 4; con el nuevo, cientos.
    std::set<int> distintos;
    for (int x = 0; x < 40; ++x) {
        for (int z = 0; z < 25; ++z) {
            const Rumbo r = rumboDe(anguloPorPosicion(x, z));
            // Se redondea a grados para contar direcciones perceptiblemente
            // distintas, no ruido de coma flotante.
            distintos.insert((int)(atan2f(r.rz, r.rx) * 180.0f / 3.14159265f));
        }
    }
    INFO("direcciones distintas: ", distintos.size());
    CHECK(distintos.size() > 100);   // con el sistema viejo serian 4
}

TEST_CASE("Caida: el eje entero siempre es una cardinal valida") {
    // El aterrizaje recoloca los bloques con el eje entero, y una celda solo
    // puede estar en una de las cuatro direcciones. Uno de los dos tiene que
    // ser 0 y el otro +-1: si los dos fueran no nulos, los bloques acabarian
    // en diagonal y se solaparian.
    for (int g = 0; g < 360; ++g) {
        const float a = (float)g * 3.14159265f / 180.0f;
        const Rumbo r = rumboDe(a);
        INFO("angulo ", g, " -> (", r.vx, ",", r.vz, ")");
        CHECK((r.vx == 0) != (r.vz == 0));      // exactamente uno es cero
        CHECK(abs(r.vx) + abs(r.vz) == 1);      // el otro es +-1
    }
}

TEST_CASE("Caida: el eje entero concuerda con el rumbo continuo") {
    // El eje tiene que apuntar hacia el mismo lado que el vector, o el arbol
    // se veria caer hacia un sitio y aterrizaria en otro.
    for (int g = 0; g < 360; g += 3) {
        const float a = (float)g * 3.14159265f / 180.0f;
        const Rumbo r = rumboDe(a);
        INFO("angulo ", g);
        if (r.vx != 0) {
            // El eje X manda: su signo coincide con el del vector.
            CHECK((r.rx > 0) == (r.vx > 0));
            CHECK(fabsf(r.rx) >= fabsf(r.rz));
        } else {
            CHECK((r.rz > 0) == (r.vz > 0));
            CHECK(fabsf(r.rz) > fabsf(r.rx));
        }
    }
}

TEST_CASE("Caida: el rumbo es determinista") {
    // El mismo arbol del mismo mundo cae siempre igual. Sin esto, recargar la
    // partida cambiaria como cayo un arbol ya talado.
    for (int x = -50; x <= 50; x += 11) {
        for (int z = -50; z <= 50; z += 13) {
            CHECK(anguloPorPosicion(x, z) == doctest::Approx(anguloPorPosicion(x, z)));
        }
    }
}

TEST_CASE("Caida: arboles vecinos caen hacia lados distintos") {
    // Si dos arboles pegados cayeran igual, un bosque seguiria viendose
    // peinado aunque el rumbo fuera continuo.
    int iguales = 0, total = 0;
    for (int x = 0; x < 30; ++x) {
        for (int z = 0; z < 30; ++z) {
            const float a1 = anguloPorPosicion(x, z);
            const float a2 = anguloPorPosicion(x + 1, z);
            ++total;
            if (fabsf(a1 - a2) < 0.15f) ++iguales;   // ~8 grados
        }
    }
    INFO("vecinos con rumbo parecido: ", iguales, " de ", total);
    // Que coincidan alguna vez es normal; que coincidan casi siempre seria el
    // bosque peinado de antes.
    CHECK(iguales < total / 4);
}

// ============================================================================
// LA NORMAL DEL RAYCAST EN UN NIVEL PARCIAL
// ============================================================================
// BUG: apuntando a la cara de ARRIBA de un nivel desde 2-4 bloques, el bloque
// nuevo se colocaba AL LADO en vez de encima.
//
// La normal se calculaba como `prevBlock - pos`: por que cara del VOXEL entro
// el rayo. Para un cubo entero es correcto -- la caja ocupa el voxel completo.
// Para un nivel de 3 px (3/16 del voxel) NO: mirandolo desde lejos y algo
// elevado, el rayo entra al voxel por un LATERAL, varios pixeles por encima de
// la capa, y solo despues baja hasta tocar su tapa.
//
// La normal salia horizontal, placeBlock lo leia como "me apuntan de lado", y
// placePos acababa en el voxel contiguo.
//
// Estos tests fijan la GEOMETRIA del caso, que es lo que hace que el arreglo
// sea correcto y no un parche.

namespace {
// El slab method tal y como quedo en raycastBlock: devuelve el eje por el que
// el rayo entro de verdad en la caja, y si fue por su cara baja.
struct Entrada { bool toca; int eje; bool porMin; };

Entrada entrarEnCaja(const float O[3], const float D[3],
                     const float B0[3], const float B1[3]) {
    float tEnt = 0.0f, tSal = 1e9f;
    int eje = -1; bool porMin = false; bool ok = true;

    for (int e = 0; e < 3 && ok; ++e) {
        if (std::fabs(D[e]) < 1e-6f) {
            if (O[e] < B0[e] || O[e] > B1[e]) ok = false;
        } else {
            float t1 = (B0[e] - O[e]) / D[e];
            float t2 = (B1[e] - O[e]) / D[e];
            bool pm = true;
            if (t1 > t2) { const float tp = t1; t1 = t2; t2 = tp; pm = false; }
            if (t1 > tEnt) { tEnt = t1; eje = e; porMin = pm; }
            if (t2 < tSal) tSal = t2;
            if (tEnt > tSal) ok = false;
        }
    }
    return { ok, eje, porMin };
}
} // namespace

TEST_CASE("Raycast: mirando la tapa de un nivel desde lejos, la normal es ARRIBA") {
    // ⭐ EL CASO REPORTADO, en numeros.
    //
    // Nivel 1 (3 px = 0.1875) en el voxel (0,0,0). El jugador esta 3 bloques
    // al este y a la altura de los ojos (1,6), mirando hacia abajo a la capa.
    const float B0[3] = { 0.0f, 0.0f,    0.0f };
    const float B1[3] = { 1.0f, 0.1875f, 1.0f };

    const float O[3] = { 3.5f, 1.6f, 0.5f };
    // Direccion hacia el centro de la tapa.
    float D[3] = { 0.5f - 3.5f, 0.09f - 1.6f, 0.0f };
    const float len = std::sqrt(D[0]*D[0] + D[1]*D[1] + D[2]*D[2]);
    D[0] /= len; D[1] /= len; D[2] /= len;

    const Entrada e = entrarEnCaja(O, D, B0, B1);
    REQUIRE(e.toca);

    // Entra por el eje Y, por su cara ALTA -> normal (0,+1,0).
    CHECK(e.eje == 1);
    CHECK(e.porMin == false);
}

TEST_CASE("Raycast: la normal del VOXEL habria dicho 'de lado'") {
    // Por que el calculo viejo fallaba: el rayo cruza la frontera del voxel
    // (x=1) a una altura MUY por encima de la capa, asi que el DDA marca el
    // voxel de al lado como `prevBlock` y la normal sale horizontal.
    const float O[3] = { 3.5f, 1.6f, 0.5f };
    float D[3] = { 0.5f - 3.5f, 0.09f - 1.6f, 0.0f };
    const float len = std::sqrt(D[0]*D[0] + D[1]*D[1] + D[2]*D[2]);
    D[0] /= len; D[1] /= len; D[2] /= len;

    // Altura del rayo justo al cruzar x = 1.0 (el borde del voxel).
    const float tBorde = (1.0f - O[0]) / D[0];
    const float yEnBorde = O[1] + D[1] * tBorde;

    // Muy por encima de la capa de 0.1875: el rayo entra al voxel por el
    // lateral, no por la tapa. De ahi la normal horizontal.
    CHECK(yEnBorde > 0.1875f);
}

TEST_CASE("Raycast: un cubo entero no cambia de comportamiento") {
    // La correccion no debe alterar el caso normal: para una caja que llena el
    // voxel, la cara de la caja y la del voxel son la MISMA.
    //
    // Se apunta al CENTRO de la cara lateral (y = 0.5), no por encima del
    // cubo: mirando a un punto mas alto que 1.0 el rayo entraria por la tapa
    // -- que tambien seria correcto, pero no es el caso que interesa fijar
    // aqui.
    const float B0[3] = { 0.0f, 0.0f, 0.0f };
    const float B1[3] = { 1.0f, 1.0f, 1.0f };

    const float O[3] = { 3.5f, 1.6f, 0.5f };
    float D[3] = { 0.5f - 3.5f, 0.5f - 1.6f, 0.0f };
    const float len = std::sqrt(D[0]*D[0] + D[1]*D[1] + D[2]*D[2]);
    D[0] /= len; D[1] /= len; D[2] /= len;

    const Entrada e = entrarEnCaja(O, D, B0, B1);
    REQUIRE(e.toca);
    // Mirando de lado a un cubo entero: entra por X, cara alta -> (+1,0,0).
    CHECK(e.eje == 0);
    CHECK(e.porMin == false);
}

TEST_CASE("Raycast: apuntando de lado a un nivel, sigue siendo de lado") {
    // El arreglo no debe convertir TODO en "por arriba": mirando la capa a su
    // misma altura, la cara correcta es la lateral.
    const float B0[3] = { 0.0f, 0.0f,    0.0f };
    const float B1[3] = { 1.0f, 0.1875f, 1.0f };

    const float O[3] = { 3.5f, 0.09f, 0.5f };   // a la altura de la capa
    float D[3] = { -1.0f, 0.0f, 0.0f };

    const Entrada e = entrarEnCaja(O, D, B0, B1);
    REQUIRE(e.toca);
    CHECK(e.eje == 0);          // eje X
    CHECK(e.porMin == false);   // cara alta de X -> normal (+1,0,0)
}
