#pragma once

#include <vector>
#include <map>
#include <set>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================================================
// LA GEOMETRIA DE UN CHUNK, SIN OPENGL
// ============================================================================
// Este archivo existe para separar UNA cosa de otra:
//
//     construir la geometria   (CPU pura -- se puede hacer en cualquier hilo)
//     subirla a la GPU         (OpenGL   -- SOLO el hilo principal)
//
// ----------------------------------------------------------------------------
// POR QUE HACIA FALTA
// ----------------------------------------------------------------------------
// Medido en el juego real, 90 segundos de partida:
//
//     estado estacionario:  chunks = 0.06 ms   (nada)
//     durante la carga:     chunks = 25.3 ms   <- el tiron, FPS de 199 a 30
//
// Los 25 ms no son de GENERAR terreno: son de MALLARLO. Los 2 hilos de
// generacion producen chunks en paralelo y luego el hilo principal los malla
// de uno en uno, en serie, con un presupuesto de 2.5 ms que un solo chunk
// puede desbordar entero (un chunk son ~330.000 iteraciones de celda).
//
// Subir el numero de hilos de generacion no arregla esto: la generacion ya
// cuesta 0.06 ms. Lo que hay que mover de hilo es el MALLADO.
//
// ----------------------------------------------------------------------------
// POR QUE SE PUEDE
// ----------------------------------------------------------------------------
// Se comprobo dónde vive OpenGL dentro de buildChunkMesh: TODAS las llamadas
// (glGenBuffers, glBufferData, glDeleteBuffers) estan concentradas en las
// ultimas ~100 lineas. Las ~5.600 anteriores solo llenan tres
// `std::map<GLuint, std::vector<float>>` con vertices, colores y UVs.
//
// O sea: la parte cara ya es CPU pura, solo estaba pegada a la parte de GPU.
// Este tipo es la frontera entre las dos.
//
// ----------------------------------------------------------------------------
// QUE ES Y QUE NO ES
// ----------------------------------------------------------------------------
// `MallaChunk` es un CONTENEDOR DE DATOS, no un objeto de render. No tiene
// handles de GL, no llama a ninguna funcion de GL y no necesita contexto.
// Por eso puede viajar entre hilos: se construye en un worker, se pasa al
// hilo principal, y alli se convierte en VBOs.
//
// Es deliberadamente el MISMO layout que ya usaba el mesher (tres mapas
// indexados por textura), para que el cambio sea mecanico y no toque las
// 5.600 lineas de geometria.
// ============================================================================

namespace Render {

// El identificador de textura. Se usa un alias en vez de GLuint para que este
// header NO tenga que incluir OpenGL: es lo que le permite compilarse en un
// worker y, de paso, en los tests.
using TexID = unsigned int;

// ----------------------------------------------------------------------------
// LA GEOMETRIA DE UN BATCH (una textura)
// ----------------------------------------------------------------------------
struct BatchCPU {
    TexID              textura = 0;
    std::vector<float> vertices;   // x,y,z por vertice
    std::vector<float> colores;    // r,g,b,a por vertice
    std::vector<float> uvs;        // u,v por vertice

    // Las dos marcas que el render necesita y que el mesher ya calculaba:
    //   transparente -> agua/lava: va en el pase con blending
    //   recortado    -> hierba/hojas: necesita GL_ALPHA_TEST
    bool transparente = false;
    bool recortado    = false;

    // ⭐ LOS BYTES TAL CUAL LOS QUIERE LA GPU
    //
    // Formato GL_T2F_C4UB_V3F, 24 bytes por vertice:
    //     u,v (2 floats) | r,g,b,a (4 bytes) | x,y,z (3 floats)
    //
    // Lo llena entrelazar(). Hasta entonces la geometria vive en los tres
    // vectores de floats de arriba; despues vive SOLO aqui (los vectores se
    // liberan). Se hace en el worker para que el hilo principal no tenga que
    // copiar ni validar nada: coge estos bytes y los sube.
    static constexpr size_t BYTES_POR_VERTICE = 24;
    std::vector<uint8_t> entrelazado;

    // Cuantos vertices hay. Se deriva, no se guarda por duplicado.
    size_t numVertices() const {
        return vertices.empty() ? entrelazado.size() / BYTES_POR_VERTICE
                                : vertices.size() / 3;
    }

    bool vacio() const { return vertices.empty() && entrelazado.empty(); }

    // ⭐ VALIDACION QUE ANTES ESTABA SUELTA EN EL MESHER
    //
    // El mesher comprobaba a mano que los tres vectores cuadraran y que no
    // hubiera NaN. Se trae aqui para que la comprobacion viaje CON los datos:
    // asi el hilo principal puede rechazar un batch corrupto sin tener que
    // repetir la logica, y un test puede verificarla sin arrancar OpenGL.
    bool coherente() const {
        if (vertices.empty()) {
            // Ya entrelazado: solo queda comprobar la forma.
            if (entrelazado.empty()) return false;
            if (entrelazado.size() % BYTES_POR_VERTICE != 0) return false;
            return numVertices() % 4 == 0;
        }
        const size_t n = numVertices();
        if (vertices.size() % 3 != 0) return false;
        if (colores.size() != n * 4)  return false;
        if (uvs.size()     != n * 2)  return false;
        // GL_QUADS: los vertices van de cuatro en cuatro.
        if (n % 4 != 0) return false;
        return true;
    }

    // ⭐ DE TRES VECTORES DE FLOATS A LOS BYTES DE LA GPU
    //
    // Devuelve false, y deja el batch como estaba, si no es coherente o si
    // algun valor es NaN/Inf: esa era la validacion que el hilo principal
    // hacia por su cuenta al subir, y aqui viaja con los datos.
    //
    // El color pasa de 4 floats a 4 bytes. El mesher produce factores de luz
    // en [0,1], asi que no se pierde nada que la GPU fuera a distinguir: el
    // framebuffer tampoco tiene mas de 8 bits por canal.
    bool entrelazar() {
        if (!entrelazado.empty()) return true;
        if (!coherente()) return false;
        for (float f : vertices) if (!std::isfinite(f)) return false;
        for (float f : colores)  if (!std::isfinite(f)) return false;
        for (float f : uvs)      if (!std::isfinite(f)) return false;

        const size_t n = numVertices();
        entrelazado.resize(n * BYTES_POR_VERTICE);
        uint8_t* d = entrelazado.data();
        for (size_t i = 0; i < n; ++i, d += BYTES_POR_VERTICE) {
            std::memcpy(d, &uvs[i * 2], 2 * sizeof(float));
            for (int c = 0; c < 4; ++c) {
                float v = colores[i * 4 + c];
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
                d[8 + c] = (uint8_t)(v * 255.0f + 0.5f);
            }
            std::memcpy(d + 12, &vertices[i * 3], 3 * sizeof(float));
        }
        // Los floats ya no hacen falta: se libera su memoria de verdad
        // (clear() sola conservaria la capacidad).
        std::vector<float>().swap(vertices);
        std::vector<float>().swap(colores);
        std::vector<float>().swap(uvs);
        return true;
    }
};

// ----------------------------------------------------------------------------
// LA MALLA COMPLETA DE UN CHUNK
// ----------------------------------------------------------------------------
struct MallaChunk {
    std::vector<BatchCPU> batches;

    // Si el mesher no pudo resolver alguna textura. El hilo principal lo usa
    // para decidir si reintentar mas tarde en vez de dar la malla por buena.
    bool texturasFaltantes = false;

    // ⭐ ¿ESTA MALLA LLEGO A CONSTRUIRSE?
    //
    // No es lo mismo que `vacia()`, y confundirlas deja el mundo invisible.
    //
    //   vacia()        SI se mallo, y el resultado es que no hay nada que
    //                  dibujar. Es lo normal en un chunk de puro aire.
    //   valida = false NO se mallo: el mesher salio antes de construir (por
    //                  ejemplo esperando a que carguen los vecinos).
    //
    // Sin esta distincion, el hilo principal trata las dos igual y sube una
    // malla sin batches como si fuera buena -- borrando la geometria anterior
    // del chunk y dejandolo invisible. Es justo el fallo que se veia como "no
    // se ve ninguna textura".
    bool valida = true;

    // ⭐ ¿SE CONSTRUYO CON EL BORDE INCOMPLETO?
    //
    // Distinto de `valida`, y la diferencia importa:
    //
    //   valida = false        no se mallo nada. Se descarta y se reintenta.
    //   bordeProvisional      SI se mallo, y la geometria es buena para el
    //                         interior del chunk -- pero las caras de la
    //                         frontera se decidieron contra un vecino que aun
    //                         no estaba. Se SUBE (mejor visible que invisible)
    //                         y se anota para revisarla cuando el vecino llegue.
    //
    // Sin esta marca, una malla con el borde roto es indistinguible de una
    // buena: el chunk pasa a LISTO y su costura se queda hasta que algo la
    // toque por casualidad. Con ella, el motor sabe que tiene una deuda
    // pendiente y la salda sola.
    //
    // Es lo que cierra el parpadeo al caminar: medido andando 40 s, 40 mallas
    // se horneaban con el borde incompleto y luego cambiaban de aspecto al
    // rehacerse.
    bool bordeProvisional = false;

    // Un chunk sin nada que dibujar es un resultado VALIDO (aire puro), no un
    // error: hay que distinguirlo de "no se pudo mallar".
    bool vacia() const { return batches.empty(); }

    size_t totalVertices() const {
        size_t t = 0;
        for (const BatchCPU& b : batches) t += b.numVertices();
        return t;
    }

    void limpiar() {
        batches.clear();
        texturasFaltantes = false;
        valida = true;
        bordeProvisional = false;
    }

    // Entrelaza todos los batches para la GPU. El que no pase (NaN/Inf) se
    // descarta aqui, en el worker, en vez de llegar al hilo principal.
    void entrelazarTodo() {
        batches.erase(
            std::remove_if(batches.begin(), batches.end(),
                           [](BatchCPU& b) { return !b.entrelazar(); }),
            batches.end());
    }

    // ⭐ CONSTRUIR DESDE EL LAYOUT QUE YA USA EL MESHER
    //
    // El mesher acumula en tres mapas paralelos indexados por textura. Esto
    // los convierte en la lista de batches, aplicando de paso la validacion.
    //
    // Se pasa por parametro y no se toca el mesher por dentro: el objetivo es
    // que la separacion de hilos NO obligue a reescribir las 5.600 lineas de
    // geometria, que es donde estarian los bugs.
    //
    // ⭐ Los tres mapas se toman POR VALOR y se vacian: el mesher los pasa
    // con std::move y la geometria (varios MB por chunk) cambia de dueño sin
    // copiarse. Medido, la copia era la mitad de la fase de empaquetado.
    // Quien pase lvalues (los tests) paga una copia, que ahi no importa.
    static MallaChunk desdeMapas(
            std::map<TexID, std::vector<float>> vertices,
            std::map<TexID, std::vector<float>> colores,
            std::map<TexID, std::vector<float>> uvs,
            const std::set<TexID>& transparentes,
            const std::set<TexID>& recortadas,
            bool faltanTexturas,
            // ⭐ Viaja DENTRO de la malla, no se escribe aparte en el chunk.
            //
            // El mesher termina con `*salidaCPU = mallaCPU;`, que sobrescribe
            // el objeto entero: una marca puesta antes directamente en
            // `salidaCPU` se perderia ahi. Pasandola por aqui, llega al hilo
            // principal junto con la geometria a la que se refiere.
            bool bordeIncompleto = false) {

        MallaChunk m;
        m.texturasFaltantes  = faltanTexturas;
        m.bordeProvisional   = bordeIncompleto;
        m.batches.reserve(vertices.size());

        for (auto& par : vertices) {
            const TexID tex = par.first;
            if (par.second.empty()) continue;

            auto itC = colores.find(tex);
            auto itU = uvs.find(tex);
            if (itC == colores.end() || itU == uvs.end()) continue;

            BatchCPU b;
            b.textura      = tex;
            b.vertices     = std::move(par.second);
            b.colores      = std::move(itC->second);
            b.uvs          = std::move(itU->second);
            b.transparente = (transparentes.count(tex) != 0);
            b.recortado    = (recortadas.count(tex) != 0);

            // Un batch incoherente se DESCARTA aqui, antes de cruzar de hilo.
            // Asi el hilo principal solo recibe geometria que ya se sabe
            // valida, y no hay que decidir que hacer con basura a mitad de la
            // subida a GPU.
            if (!b.coherente()) continue;

            m.batches.push_back(std::move(b));
        }
        return m;
    }
};

} // namespace Render
