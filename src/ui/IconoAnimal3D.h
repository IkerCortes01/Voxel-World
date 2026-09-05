#pragma once

// ============================================================================
// ICONO DE ANIMAL EN 3D
// ============================================================================
// Dibuja el MODELO REAL de un animal como icono del inventario, en vez de un
// PNG plano.
//
// POR QUE NO UNA TEXTURA
// ----------------------
// El resto de items del motor son un dibujo en PNG: un palo, un hacha, un
// tazon. Un animal no cabe en ese molde por dos razones:
//
//   1. NO EXISTE el PNG. Habria que dibujarlo a mano, y el dibujo se quedaria
//      desfasado en cuanto cambiara la anatomia -- que es justo lo que este
//      proyecto lleva ajustando con datos medidos.
//
//   2. YA EXISTE EL MODELO. La malla del pecari se genera de su anatomia
//      (PecariCuerpo.h a partir de 13_PECARI_ANATOMIA.json). Usarla como
//      icono significa que el icono ES el animal: si manana se corrige el
//      largo del hocico, el icono se corrige solo.
//
// COMO SE DIBUJA
// --------------
// Se reutiliza la MISMA malla cacheada que usa el mundo (CacheMallas), en su
// LOD mas bajo -- un icono de 32 px no necesita mas -- y se pinta en
// proyeccion ortografica dentro del cuadro del slot, girado en tres cuartos
// para que se lea el volumen.
//
// COSTE: cero mallas nuevas. El caché ya tiene esa geometria porque el mundo
// la esta usando; aqui solo se pide prestada.

#include <cmath>
#include "../fauna/AnimalMallaCache.h"

namespace UI {

// ----------------------------------------------------------------------------
// EL ENCUADRE
// ----------------------------------------------------------------------------
// Tres cuartos, ligeramente desde arriba: la pose de catalogo de toda la vida.
// De perfil puro se pierde el volumen y de frente no se reconoce al animal.
constexpr float ICONO_GIRO_Y_GRADOS = 215.0f;  // le vemos el costado y algo de morro
constexpr float ICONO_GIRO_X_GRADOS = 18.0f;   // un poco desde arriba

// La luz del icono es fija y frontal-alta: el slot del inventario no tiene
// hora del dia, asi que aqui no se consulta la luz del mundo.
constexpr float ICONO_LUZ_X = 0.40f;
constexpr float ICONO_LUZ_Y = 0.80f;
constexpr float ICONO_LUZ_Z = 0.45f;

// Cuanto se oscurece la cara que da la espalda a la luz.
constexpr float ICONO_CONTRASTE = 0.55f;

// ----------------------------------------------------------------------------
// DIBUJAR EL ANIMAL EN UN CUADRO DE PANTALLA
// ----------------------------------------------------------------------------
// (cx, cy) es el centro del slot en pixeles de pantalla; 'tam' su lado.
//
// Se asume el mismo estado de GL que usa el resto de iconos del inventario:
// proyeccion ortografica ya montada y GL_TEXTURE_2D desactivable a voluntad.
inline void dibujarIconoAnimal3D(Fauna::CacheMallas& cache,
                                 Fauna::EspecieAnimal especie,
                                 int etapa,
                                 float cx, float cy, float tam) {
    // LOD mas bajo: es un icono, no el animal a dos metros. El caché ya lo
    // tiene si el mundo ha dibujado alguno.
    const Fauna::MallaAnimal* malla = cache.obtener(especie, etapa, 3);
    if (!malla || malla->indices.empty()) return;

    // --- ENCAJAR EL BICHO EN EL CUADRO ---
    // La malla viene en metros y con su propio tamano. Se mide su caja y se
    // escala para que el lado mayor ocupe el cuadro, con un margen.
    const float anchoM = malla->maximo.x - malla->minimo.x;
    const float altoM  = malla->maximo.y - malla->minimo.y;
    const float largoM = malla->maximo.z - malla->minimo.z;

    // El giro de 3/4 mete el LARGO en diagonal, asi que lo que de verdad
    // ocupa a lo ancho es una mezcla de largo y ancho. Se toma el mayor de
    // los tres y se deja aire de sobra: es preferible un icono algo pequeno
    // a uno recortado.
    float mayor = anchoM;
    if (largoM > mayor) mayor = largoM;
    if (altoM  > mayor) mayor = altoM;
    if (mayor < 1e-4f) return;

    const float MARGEN = 0.78f;              // 22% de aire alrededor
    const float esc = (tam * MARGEN) / mayor;

    // Centro geometrico de la malla: se lleva al centro del slot para que el
    // animal no quede pegado a un borde.
    const float mediaX = (malla->maximo.x + malla->minimo.x) * 0.5f;
    const float mediaY = (malla->maximo.y + malla->minimo.y) * 0.5f;
    const float mediaZ = (malla->maximo.z + malla->minimo.z) * 0.5f;

    const float ry = ICONO_GIRO_Y_GRADOS * 3.14159265f / 180.0f;
    const float rx = ICONO_GIRO_X_GRADOS * 3.14159265f / 180.0f;
    const float cosY = std::cos(ry), sinY = std::sin(ry);
    const float cosX = std::cos(rx), sinX = std::sin(rx);

    const bool teniaTextura = glIsEnabled(GL_TEXTURE_2D);
    const bool teniaCull    = glIsEnabled(GL_CULL_FACE);
    const bool teniaDepth   = glIsEnabled(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);   // color por vertice, como en el mundo
    glDisable(GL_CULL_FACE);    // el icono es pequeno: no merece la pena

    // ⭐ EL DEPTH TEST SI HACE FALTA, y es la diferencia entre un animal y una
    // mancha. Sin el, los triangulos se pintan en el orden del indice y las
    // patas del otro lado acaban encima del lomo.
    //
    // El inventario dibuja en 2D con el depth apagado, asi que hay que
    // encenderlo aqui y limpiarlo para no chocar con lo ya pintado.
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    for (size_t t = 0; t + 2 < malla->indices.size(); t += 3) {
        for (int k = 0; k < 3; ++k) {
            const Fauna::VerticeAnimal& v = malla->vertices[malla->indices[t + k]];

            // Centrar
            const float x0 = v.pos.x - mediaX;
            const float y0 = v.pos.y - mediaY;
            const float z0 = v.pos.z - mediaZ;

            // Girar en Y (dejarlo de tres cuartos)
            const float x1 =  x0 * cosY + z0 * sinY;
            const float z1 = -x0 * sinY + z0 * cosY;

            // Inclinar en X (mirarlo un poco desde arriba)
            const float y2 = y0 * cosX - z1 * sinX;

            // Lo mismo con la normal, para que la luz siga al modelo.
            const float nx0 = v.normal.x, ny0 = v.normal.y, nz0 = v.normal.z;
            const float nx1 =  nx0 * cosY + nz0 * sinY;
            const float nz1 = -nx0 * sinY + nz0 * cosY;
            const float ny2 = ny0 * cosX - nz1 * sinX;
            const float nz2 = ny0 * sinX + nz1 * cosX;

            float d = nx1 * ICONO_LUZ_X + ny2 * ICONO_LUZ_Y + nz2 * ICONO_LUZ_Z;
            if (d < 0.0f) d = 0.0f;
            const float f = 1.0f - ICONO_CONTRASTE * (1.0f - d);

            glColor3f(v.r * f, v.g * f, v.b * f);
            // Y de pantalla crece hacia abajo: por eso el signo menos.
            glVertex3f(cx + x1 * esc, cy - y2 * esc, 0.0f);
        }
    }
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    if (!teniaDepth)   glDisable(GL_DEPTH_TEST);
    if (teniaCull)     glEnable(GL_CULL_FACE);
    if (teniaTextura)  glEnable(GL_TEXTURE_2D);
}

} // namespace UI
