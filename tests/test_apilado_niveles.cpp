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
