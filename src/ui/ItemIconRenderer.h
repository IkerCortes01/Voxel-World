#ifndef ITEM_ICON_RENDERER_H
#define ITEM_ICON_RENDERER_H

// ============================================================================
// ITEM ICON RENDERER - Iconos isométricos 3D para el inventario
// ============================================================================
// RESPONSABILIDAD UNICA: dibujar un bloque como icono 3D en coordenadas de
// pantalla. No resuelve texturas (eso es del ItemTextureManager) ni conoce
// slots, cantidades ni el inventario.
//
// ----------------------------------------------------------------------------
// POR QUE SE DIBUJA A MANO Y NO CON UNA CAMARA 3D
// ----------------------------------------------------------------------------
// Podria montarse una proyeccion 3D real (glFrustum + rotaciones) para cada
// icono, pero seria un error:
//   - Habria que salvar y restaurar las matrices por cada icono (9 en la
//     hotbar + 45 en el inventario = 54 cambios de matriz por frame).
//   - El icono quedaria sujeto al depth buffer y al culling del mundo.
//   - Un cambio de FOV o de aspect ratio deformaria los iconos.
//
// En su lugar se PROYECTAN LOS VERTICES A MANO. Las tres caras visibles de un
// cubo en vista isometrica son rombos cuyas coordenadas se pueden calcular
// directamente en 2D. Ventajas:
//   - Se dibuja con la misma proyeccion ortografica del HUD (sin cambios de
//     matriz, sin depth, sin culling).
//   - El resultado es identico a cualquier resolucion y FOV.
//   - Es exactamente igual de barato que un sprite plano: 3 quads.
//
// ----------------------------------------------------------------------------
// GEOMETRIA
// ----------------------------------------------------------------------------
// Vista isometrica clasica de sandbox voxel: el cubo se gira ~45 grados sobre
// el eje vertical y se inclina hacia abajo, de modo que se ven TOP, IZQUIERDA
// y DERECHA. En pantalla:
//
//              A                A = arriba
//            /   \
//          B       C            rombo superior = cara TOP
//          | \   / |
//          |   D   |            D = centro
//          E   |   F            izquierda = cara WEST
//            \ | /              derecha   = cara EAST
//              G
//
// Los seis puntos se derivan de dos semiejes:
//   hx = mitad del ancho del icono
//   hy = altura de la "punta" del rombo superior (hx * ISO_RATIO)
//
// ISO_RATIO = 0.5 da la proporcion 2:1 de la isometria de pixel art, que es
// la que produce el aspecto reconocible de estos iconos.
//
// ----------------------------------------------------------------------------
// SOMBREADO
// ----------------------------------------------------------------------------
// Cada cara lleva un brillo distinto y fijo (top mas clara, laterales mas
// oscuras y con distinto tono entre si). Es lo que da la sensacion de volumen
// sin necesidad de iluminacion real. Sin esto, las tres caras con la misma
// textura se ven como una mancha plana y el cubo no se lee.
// ============================================================================

namespace UI {

class ItemIconRenderer {
public:
    // Proporcion vertical del rombo. 0.5 = isometria 2:1 clasica.
    static constexpr float ISO_RATIO = 0.5f;

    // Brillo por cara. La diferencia entre laterales es lo que hace legible
    // la arista vertical central del cubo.
    static constexpr float SHADE_TOP   = 1.00f;
    static constexpr float SHADE_LEFT  = 0.72f;
    static constexpr float SHADE_RIGHT = 0.55f;

    // Un vértice proyectado, en píxeles de pantalla.
    struct P { float x, y; };

    // ------------------------------------------------------------------------
    // CALCULAR LOS 7 PUNTOS DEL CUBO ISOMETRICO
    // ------------------------------------------------------------------------
    // cx, cy : centro del icono
    // size   : ancho total del icono en píxeles
    //
    // Salida (índices):
    //   0 = A  (vértice superior)
    //   1 = B  (izquierda del rombo superior)
    //   2 = C  (derecha del rombo superior)
    //   3 = D  (centro: donde concurren las tres caras)
    //   4 = E  (esquina inferior izquierda)
    //   5 = F  (esquina inferior derecha)
    //   6 = G  (vértice inferior)
    static void computeIso(float cx, float cy, float size, P out[7]) {
        const float hx = size * 0.5f;
        const float t  = hx * ISO_RATIO;   // medio alto del rombo
        const float sh = size * 0.55f;     // alto de las caras laterales

        // Centrado óptico: la silueta total mide 2t + sh de alto.
        const float top = cy - (2.0f * t + sh) * 0.5f;

        out[0] = { cx,      top };                 // A  punta superior
        out[1] = { cx - hx, top + t };             // B  izq. del rombo
        out[2] = { cx + hx, top + t };             // C  der. del rombo
        out[3] = { cx,      top + 2.0f * t };      // D  centro
        out[4] = { cx - hx, top + t + sh };        // E  esq. inf. izq.
        out[5] = { cx + hx, top + t + sh };        // F  esq. inf. der.
        out[6] = { cx,      top + 2.0f * t + sh }; // G  punta inferior
    }
};

} // namespace UI

#endif // ITEM_ICON_RENDERER_H
