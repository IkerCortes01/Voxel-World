#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <vector>
#include <algorithm>
#include "ItemTextureManager.h"

// ============================================================================
// UI RENDERER - Renderizador 2D con batching
// ============================================================================
// RESPONSABILIDAD UNICA: acumular sprites 2D y emitirlos agrupados por
// textura, en el menor número de draw calls posible.
//
// ----------------------------------------------------------------------------
// INDEPENDENCIA DEL MUNDO 3D
// ----------------------------------------------------------------------------
// Este renderer no conoce chunks, cámara, frustum, niebla, iluminación ni
// posición del jugador. Trabaja exclusivamente en coordenadas de PANTALLA
// (píxeles, origen arriba-izquierda), con una proyección ortográfica derivada
// del tamaño actual de la ventana.
//
// Establece explícitamente el estado GL que necesita al empezar (beginFrame)
// y lo restaura al terminar (endFrame), de modo que:
//   - Ningún estado dejado por el render del mundo puede afectarle
//     (depth test, fog, lighting, alpha test, culling, color actual).
//   - No deja estado contaminado para lo que venga después.
// Esto cumple el requisito de que la UI nunca quede detrás de un bloque ni
// se vea afectada por la profundidad del mundo.
//
// ----------------------------------------------------------------------------
// BATCHING
// ----------------------------------------------------------------------------
// Los sprites se acumulan en una lista y se ORDENAN POR TEXTURA antes de
// dibujar. Como el pipeline es fixed-function y un cambio de textura obliga a
// cerrar el glBegin/glEnd en curso, agrupar por textura reduce los cambios de
// bind al mínimo teórico (uno por textura distinta) en lugar de uno por
// sprite.
//
// Con 9 slots de hotbar que comparten textura (p.ej. 9 pilas de piedra), se
// pasa de 9 binds + 9 begin/end a 1 bind + 1 begin/end.
//
// El orden dentro de cada capa se preserva mediante `layer`, para que el
// fondo del slot quede siempre por debajo del icono y el icono por debajo del
// contador y del borde de selección.
// ============================================================================

namespace UI {

// Un sprite listo para dibujar. Sin punteros: es un valor puro.
struct UISprite {
    float x, y, w, h;          // rectángulo en píxeles de pantalla
    float u0, v0, u1, v1;      // UV (permite atlas y volteo)
    TextureHandle texture;     // 0 = quad de color plano
    float r, g, b, a;          // tinte (blanco = textura sin modificar)
    int   layer;               // orden de dibujado (menor = más al fondo)
};

// Capas estándar del HUD. Definidas aquí para que todo el UI comparta el
// mismo orden y no haya que adivinarlo en cada sitio.
namespace Layer {
    constexpr int Background   = 0;   // fondos de slot
    constexpr int Item         = 10;  // icono del item
    constexpr int ItemOverlay  = 20;  // barra de durabilidad, etc.
    constexpr int Count        = 30;  // número de cantidad
    constexpr int Selection    = 40;  // borde del slot seleccionado
    constexpr int Tooltip      = 50;  // por encima de todo
}

class UIRenderer {
private:
    std::vector<UISprite> sprites;

    int screenWidth = 0;
    int screenHeight = 0;

    // Estadísticas del último frame (DEBUG_HOTBAR)
    int lastDrawCalls = 0;
    int lastSpriteCount = 0;
    int lastTextureSwitches = 0;

public:
    UIRenderer() { sprites.reserve(256); }

    // ------------------------------------------------------------------------
    // PROYECCIÓN
    // ------------------------------------------------------------------------
    // Se recalcula con el tamaño REAL de la ventana cada frame, así que un
    // resize se refleja de inmediato sin perder texturas (las texturas viven
    // en ItemTextureManager, que no se toca en un resize).
    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    int getScreenWidth() const  { return screenWidth; }
    int getScreenHeight() const { return screenHeight; }

    // ------------------------------------------------------------------------
    // ACUMULAR SPRITES
    // ------------------------------------------------------------------------
    void addSprite(const UISprite& s) { sprites.push_back(s); }

    // Quad texturizado (UV completas).
    void drawTextured(float x, float y, float w, float h,
                      TextureHandle tex, int layer,
                      float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
        UISprite s;
        s.x = x; s.y = y; s.w = w; s.h = h;
        s.u0 = 0.0f; s.v0 = 0.0f; s.u1 = 1.0f; s.v1 = 1.0f;
        s.texture = tex;
        s.r = r; s.g = g; s.b = b; s.a = a;
        s.layer = layer;
        sprites.push_back(s);
    }

    // Quad texturizado con UV explícitas (atlas, volteo).
    void drawTexturedUV(float x, float y, float w, float h,
                        TextureHandle tex,
                        float u0, float v0, float u1, float v1,
                        int layer,
                        float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
        UISprite s;
        s.x = x; s.y = y; s.w = w; s.h = h;
        s.u0 = u0; s.v0 = v0; s.u1 = u1; s.v1 = v1;
        s.texture = tex;
        s.r = r; s.g = g; s.b = b; s.a = a;
        s.layer = layer;
        sprites.push_back(s);
    }

    // Quad de color plano (sin textura).
    void drawRect(float x, float y, float w, float h,
                  float r, float g, float b, float a, int layer) {
        UISprite s;
        s.x = x; s.y = y; s.w = w; s.h = h;
        s.u0 = 0.0f; s.v0 = 0.0f; s.u1 = 1.0f; s.v1 = 1.0f;
        s.texture = 0;
        s.r = r; s.g = g; s.b = b; s.a = a;
        s.layer = layer;
        sprites.push_back(s);
    }

    void clear() { sprites.clear(); }

    size_t pendingCount() const { return sprites.size(); }

    // ------------------------------------------------------------------------
    // ORDENACIÓN PARA BATCHING
    // ------------------------------------------------------------------------
    // Criterio: primero por capa (para respetar el orden visual), luego por
    // textura (para agrupar y minimizar binds dentro de cada capa).
    //
    // Se usa stable_sort para que dos sprites con la misma capa y textura
    // conserven el orden de inserción: sin esto, el orden de dibujado sería
    // impredecible y elementos superpuestos podrían parpadear entre frames.
    void sortForBatching() {
        std::stable_sort(sprites.begin(), sprites.end(),
            [](const UISprite& a, const UISprite& b) {
                if (a.layer != b.layer) return a.layer < b.layer;
                return a.texture < b.texture;
            });
    }

    const std::vector<UISprite>& getSprites() const { return sprites; }

    // Estadísticas
    void setStats(int drawCalls, int spriteCount, int texSwitches) {
        lastDrawCalls = drawCalls;
        lastSpriteCount = spriteCount;
        lastTextureSwitches = texSwitches;
    }
    int getDrawCalls() const       { return lastDrawCalls; }
    int getSpriteCount() const     { return lastSpriteCount; }
    int getTextureSwitches() const { return lastTextureSwitches; }
};

} // namespace UI

#endif // UI_RENDERER_H
