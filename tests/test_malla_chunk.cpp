#include <doctest/doctest.h>
#include "render/MallaChunk.h"

// ============================================================================
// LA FRONTERA ENTRE EL HILO QUE MALLA Y EL QUE DIBUJA
// ============================================================================
// `MallaChunk` es lo que permite construir la geometria de un chunk FUERA del
// hilo principal. Su valor no esta en lo que hace -- es un contenedor -- sino
// en una propiedad: NO TOCA OPENGL. Por eso puede viajar entre hilos.
//
// El motivo, medido en el juego real (90 s de partida):
//
//     estado estacionario:  chunks = 0.06 ms
//     durante la carga:     chunks = 25.3 ms   <- FPS de 199 a 30
//
// Esos 25 ms son de MALLAR, no de generar. Los 2 workers producen chunks en
// paralelo y el hilo principal los malla en serie.
//
// Estos tests fijan las dos cosas que hacen segura esa separacion:
//   1. Que la geometria incoherente se descarte ANTES de cruzar de hilo.
//   2. Que un chunk vacio sea un resultado valido, no un error.
//
// El propio hecho de que este archivo COMPILE ya prueba algo: MallaChunk.h no
// arrastra OpenGL. Si alguien metiera un GLuint o un gl*() dentro, este test
// dejaria de compilar -- que es exactamente el aviso que se quiere.

using namespace Render;

// ----------------------------------------------------------------------------
// Ayuda: un batch bien formado de `quads` quads.
// ----------------------------------------------------------------------------
static BatchCPU batchValido(TexID tex, int quads) {
    BatchCPU b;
    b.textura = tex;
    const int n = quads * 4;               // 4 vertices por quad
    for (int i = 0; i < n; ++i) {
        b.vertices.insert(b.vertices.end(), { (float)i, 0.0f, 0.0f });
        b.colores .insert(b.colores .end(), { 1.0f, 1.0f, 1.0f, 1.0f });
        b.uvs     .insert(b.uvs     .end(), { 0.0f, 0.0f });
    }
    return b;
}

// ============================================================================
// COHERENCIA DE UN BATCH
// ============================================================================

TEST_CASE("Batch: uno bien formado es coherente") {
    const BatchCPU b = batchValido(7, 3);
    CHECK(b.coherente());
    CHECK(b.numVertices() == 12);
    CHECK_FALSE(b.vacio());
}

TEST_CASE("Batch: se rechaza el que tiene los vectores descuadrados") {
    // ⚠️ ESTA ES LA VALIDACION QUE IMPORTA.
    //
    // Si un batch con vectores de tamaños distintos llegara a la subida de
    // GPU, glBufferData leeria fuera del vector mas corto. Es corrupcion de
    // memoria silenciosa, y por eso se descarta en el lado del worker.
    SUBCASE("faltan colores") {
        BatchCPU b = batchValido(1, 2);
        b.colores.pop_back();
        CHECK_FALSE(b.coherente());
    }
    SUBCASE("faltan UVs") {
        BatchCPU b = batchValido(1, 2);
        b.uvs.pop_back();
        CHECK_FALSE(b.coherente());
    }
    SUBCASE("los vertices no son multiplo de 3") {
        BatchCPU b = batchValido(1, 2);
        b.vertices.pop_back();
        CHECK_FALSE(b.coherente());
    }
}

TEST_CASE("Batch: se rechaza el que no cierra quads") {
    // El render dibuja con GL_QUADS: los vertices van de cuatro en cuatro.
    // Un batch con 6 vertices dejaria dos sueltos y GL leeria basura.
    BatchCPU b;
    b.textura = 1;
    for (int i = 0; i < 6; ++i) {          // 6 vertices = 1.5 quads
        b.vertices.insert(b.vertices.end(), { 0.0f, 0.0f, 0.0f });
        b.colores .insert(b.colores .end(), { 1.0f, 1.0f, 1.0f, 1.0f });
        b.uvs     .insert(b.uvs     .end(), { 0.0f, 0.0f });
    }
    CHECK(b.numVertices() == 6);
    CHECK_FALSE(b.coherente());
}

TEST_CASE("Batch: uno vacio no es coherente ni se sube") {
    BatchCPU b;
    CHECK(b.vacio());
    CHECK_FALSE(b.coherente());
}

// ============================================================================
// CONSTRUCCION DESDE LOS MAPAS DEL MESHER
// ============================================================================
// El mesher acumula en tres std::map paralelos indexados por textura. Este es
// el punto donde ese layout se convierte en algo que cruza de hilo.

TEST_CASE("Malla: se construye desde los mapas del mesher") {
    std::map<TexID, std::vector<float>> V, C, U;
    const BatchCPU modelo = batchValido(42, 2);
    V[42] = modelo.vertices;
    C[42] = modelo.colores;
    U[42] = modelo.uvs;

    std::set<TexID> transp, recort;
    recort.insert(42);

    const MallaChunk m = MallaChunk::desdeMapas(V, C, U, transp, recort, false);

    REQUIRE(m.batches.size() == 1);
    CHECK(m.batches[0].textura == 42);
    CHECK(m.batches[0].numVertices() == 8);
    CHECK(m.batches[0].recortado);          // la marca se conserva
    CHECK_FALSE(m.batches[0].transparente);
    CHECK(m.totalVertices() == 8);
    CHECK_FALSE(m.vacia());
}

TEST_CASE("Malla: la geometria corrupta no cruza de hilo") {
    // El batch bueno pasa; el descuadrado se queda en el worker. Es la
    // garantia de que el hilo principal solo recibe geometria ya validada y
    // no tiene que decidir que hacer con basura a mitad de la subida a GPU.
    std::map<TexID, std::vector<float>> V, C, U;

    const BatchCPU bueno = batchValido(1, 1);
    V[1] = bueno.vertices; C[1] = bueno.colores; U[1] = bueno.uvs;

    const BatchCPU malo = batchValido(2, 1);
    V[2] = malo.vertices;
    C[2] = malo.colores;
    U[2] = malo.uvs;
    U[2].pop_back();                        // <- descuadrado a proposito

    const MallaChunk m = MallaChunk::desdeMapas(V, C, U, {}, {}, false);

    REQUIRE(m.batches.size() == 1);
    CHECK(m.batches[0].textura == 1);       // solo sobrevive el bueno
}

TEST_CASE("Malla: un batch sin pareja de colores o UVs se descarta") {
    // Puede pasar si el mesher aborta a medias: hay vertices de una textura
    // pero sus colores nunca se escribieron.
    std::map<TexID, std::vector<float>> V, C, U;
    V[9] = { 0.0f, 0.0f, 0.0f };            // vertices sin C ni U

    const MallaChunk m = MallaChunk::desdeMapas(V, C, U, {}, {}, false);
    CHECK(m.vacia());
}

TEST_CASE("Malla: un chunk de aire da malla vacia, que es valido") {
    // ⚠️ Vacio NO es lo mismo que fallido.
    //
    // Un chunk de aire puro no tiene nada que dibujar, y eso es un resultado
    // correcto. Si el hilo principal lo tratara como error, reintentaria el
    // mallado sin fin sobre chunks que nunca van a producir geometria.
    const MallaChunk m = MallaChunk::desdeMapas({}, {}, {}, {}, {}, false);
    CHECK(m.vacia());
    CHECK(m.totalVertices() == 0);
    CHECK_FALSE(m.texturasFaltantes);       // vacia por aire, no por fallo
}

TEST_CASE("Malla: la falta de texturas se propaga aparte del contenido") {
    // Las dos condiciones son independientes y el hilo principal las trata
    // distinto: una malla vacia se acepta; una con texturas faltantes se
    // reintenta.
    const MallaChunk m = MallaChunk::desdeMapas({}, {}, {}, {}, {}, true);
    CHECK(m.vacia());
    CHECK(m.texturasFaltantes);
}

TEST_CASE("Malla: las marcas de transparente y recortado llegan al batch") {
    // Sin ellas el render no sabe en que pase va cada batch: el agua acabaria
    // en el pase opaco (placas azules tapando el terreno) y la hierba
    // perderia el alpha test.
    std::map<TexID, std::vector<float>> V, C, U;
    for (TexID t : { (TexID)10, (TexID)20 }) {
        const BatchCPU b = batchValido(t, 1);
        V[t] = b.vertices; C[t] = b.colores; U[t] = b.uvs;
    }
    std::set<TexID> transp{ 10 }, recort{ 20 };

    const MallaChunk m = MallaChunk::desdeMapas(V, C, U, transp, recort, false);
    REQUIRE(m.batches.size() == 2);

    for (const BatchCPU& b : m.batches) {
        if (b.textura == 10) {
            CHECK(b.transparente);
            CHECK_FALSE(b.recortado);
        } else {
            CHECK_FALSE(b.transparente);
            CHECK(b.recortado);
        }
    }
}

// ============================================================================
// MOVIBLE ENTRE HILOS
// ============================================================================

TEST_CASE("Malla: se puede mover sin copiar (es lo que cruza de hilo)") {
    std::map<TexID, std::vector<float>> V, C, U;
    const BatchCPU b = batchValido(5, 50);   // 200 vertices
    V[5] = b.vertices; C[5] = b.colores; U[5] = b.uvs;

    MallaChunk origen = MallaChunk::desdeMapas(V, C, U, {}, {}, false);
    const size_t antes = origen.totalVertices();
    REQUIRE(antes == 200);

    // El traspaso worker -> hilo principal es un move: si esto copiara,
    // cada chunk pagaria una copia de toda su geometria al cambiar de hilo.
    MallaChunk destino = std::move(origen);
    CHECK(destino.totalVertices() == antes);
}
