#ifndef ITEM_TEXTURE_MANAGER_H
#define ITEM_TEXTURE_MANAGER_H

#include <map>
#include <string>
#include <vector>
#include <cstdint>

// ============================================================================
// ITEM TEXTURE MANAGER - Resolución estable ItemID -> TextureID
// ============================================================================
// RESPONSABILIDAD UNICA: dado un item, devolver SIEMPRE un handle de textura
// GL válido, sin tocar el disco durante el render.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA QUE RESUELVE (causa raíz de las desapariciones)
// ----------------------------------------------------------------------------
// El sistema anterior tenía un bucle de retroalimentación destructivo:
//
//   1. getItemTexture() devolvía 0 si el PNG no cargaba, SIN cachear el fallo.
//   2. renderHotbar veía el 0 y llamaba a loadAllBlockTextures() DENTRO del
//      bucle de dibujado de slots.
//   3. loadAllBlockTextures() -> loadDestroyStageTextures() hacía
//      destroyStageTextures.clear() SIN glDeleteTextures, filtrando 10
//      texturas GL, y loadWaterAnimation() hacía push_back sin clear,
//      creciendo sin límite.
//   4. Con 9 slots fallando: hasta 90 lecturas de disco y 90 texturas GL
//      filtradas POR FRAME.
//   5. Al agotarse los handles/VRAM, glGenTextures empezaba a fallar, más
//      texturas devolvían 0, y el ciclo se realimentaba hasta que TODO
//      caía al fallback de color plano.
//
// Es decir: las texturas no "desaparecían" por azar. El propio mecanismo de
// recuperación era el que destruía los recursos.
//
// ----------------------------------------------------------------------------
// GARANTÍAS DE ESTE MÓDULO
// ----------------------------------------------------------------------------
//   [G1] CERO I/O de disco durante el render. Todo se resuelve por caché.
//   [G2] Caché NEGATIVA: un fallo se recuerda; nunca se reintenta en bucle.
//   [G3] Handle SIEMPRE válido: si no hay textura real, devuelve la textura
//        "missing" (damero magenta/negro generada en memoria), nunca 0.
//   [G4] Idempotente: llamar a prewarm() N veces no crea recursos nuevos.
//   [G5] Sin liberación prematura: las texturas viven mientras viva el
//        manager, que sobrevive a chunks, cámara, resize y dimensiones.
//
// Este header NO incluye OpenGL: recibe las funciones de carga por callback,
// para no depender del orden de includes de main.cpp.
// ============================================================================

namespace UI {

// Identificador de textura opaco (equivale a GLuint).
typedef unsigned int TextureHandle;

class ItemTextureManager {
public:
    // Callback que el motor provee para cargar un PNG y devolver su handle.
    // Debe devolver 0 si falla.
    typedef TextureHandle (*LoadFn)(void* ctx, const char* path);
    // Callback para crear una textura RGBA desde píxeles en memoria.
    typedef TextureHandle (*CreateFn)(void* ctx, const unsigned char* rgba,
                                      int w, int h);

private:
    struct Entry {
        TextureHandle handle = 0;
        bool resolved = false;   // ya se intentó resolver
        bool valid = false;      // la resolución tuvo éxito
    };

    // itemId -> entrada. El itemId es el valor del enum BlockType del motor.
    std::map<int, Entry> cache;

    // path -> handle, para no cargar dos veces el mismo PNG.
    std::map<std::string, TextureHandle> pathCache;

    TextureHandle missingTexture = 0;

    void*    ctx      = nullptr;
    LoadFn   loadFn   = nullptr;
    CreateFn createFn = nullptr;

    bool initialized = false;

    // Estadísticas (modo depuración)
    mutable int statHits = 0;
    mutable int statMisses = 0;
    mutable int statMissingUsed = 0;

public:
    // ------------------------------------------------------------------------
    // INICIALIZACIÓN
    // ------------------------------------------------------------------------
    void init(void* context, LoadFn loader, CreateFn creator) {
        if (initialized) return;   // [G4] idempotente
        ctx = context;
        loadFn = loader;
        createFn = creator;

        buildMissingTexture();
        initialized = true;
    }

    bool isInitialized() const { return initialized; }

    // ------------------------------------------------------------------------
    // TEXTURA "MISSING" (damero magenta/negro)
    // ------------------------------------------------------------------------
    // [G3] Se genera en memoria, sin depender de ningún archivo: es el único
    // recurso que NO puede fallar, así que sirve de garantía última de que
    // siempre hay algo válido que bindear.
    //
    // El damero magenta es la convención estándar de "textura ausente" en
    // motores (Source, Unreal): es imposible confundirlo con arte real, así
    // que un fallo se ve de inmediato en vez de pasar por un bloque de piedra.
    void buildMissingTexture() {
        if (missingTexture != 0 || !createFn) return;

        constexpr int S = 16;
        unsigned char px[S * S * 4];
        for (int y = 0; y < S; ++y) {
            for (int x = 0; x < S; ++x) {
                const bool a = ((x / 4) + (y / 4)) % 2 == 0;
                unsigned char* p = &px[(y * S + x) * 4];
                p[0] = a ? 255 : 0;    // R
                p[1] = 0;              // G
                p[2] = a ? 255 : 0;    // B
                p[3] = 255;            // A
            }
        }
        missingTexture = createFn(ctx, px, S, S);
    }

    TextureHandle getMissingTexture() const { return missingTexture; }

    // ------------------------------------------------------------------------
    // REGISTRAR UNA TEXTURA DE ITEM
    // ------------------------------------------------------------------------
    // Se llama en la fase de precarga (prewarm), NUNCA durante el render.
    void registerItem(int itemId, const char* path) {
        if (!loadFn || path == nullptr) return;

        Entry& e = cache[itemId];
        if (e.resolved) return;   // [G2] ya intentado: no repetir I/O

        const std::string key(path);
        auto it = pathCache.find(key);
        if (it != pathCache.end()) {
            // [G5] Mismo PNG ya cargado por otro item: se comparte el handle
            // en vez de crear una segunda copia en VRAM.
            e.handle = it->second;
            e.valid = (e.handle != 0);
            e.resolved = true;
            return;
        }

        const TextureHandle h = loadFn(ctx, path);
        pathCache[key] = h;        // se cachea incluso el fallo (h == 0)
        e.handle = h;
        e.valid = (h != 0);
        e.resolved = true;         // [G2] resuelto pase lo que pase
    }

    // Registrar un handle ya obtenido por otra vía (p.ej. textura de bloque).
    void registerItemHandle(int itemId, TextureHandle handle) {
        Entry& e = cache[itemId];
        if (e.resolved && e.valid) return;
        e.handle = handle;
        e.valid = (handle != 0);
        e.resolved = true;
    }

    // ------------------------------------------------------------------------
    // RESOLVER (ruta caliente: se llama por slot y por frame)
    // ------------------------------------------------------------------------
    // [G1] Nunca toca el disco. [G3] Nunca devuelve 0.
    TextureHandle resolve(int itemId) const {
        auto it = cache.find(itemId);
        if (it != cache.end() && it->second.valid) {
            ++statHits;
            return it->second.handle;
        }
        ++statMisses;
        ++statMissingUsed;
        return missingTexture;   // handle siempre válido
    }

    // Igual que resolve pero informa si era una textura real.
    TextureHandle resolveChecked(int itemId, bool* outWasValid) const {
        auto it = cache.find(itemId);
        if (it != cache.end() && it->second.valid) {
            if (outWasValid) *outWasValid = true;
            ++statHits;
            return it->second.handle;
        }
        if (outWasValid) *outWasValid = false;
        ++statMisses;
        ++statMissingUsed;
        return missingTexture;
    }

    bool hasValidTexture(int itemId) const {
        auto it = cache.find(itemId);
        return it != cache.end() && it->second.valid;
    }

    bool isResolved(int itemId) const {
        auto it = cache.find(itemId);
        return it != cache.end() && it->second.resolved;
    }

    // ------------------------------------------------------------------------
    // ESTADÍSTICAS (DEBUG_HOTBAR)
    // ------------------------------------------------------------------------
    int getHits() const { return statHits; }
    int getMisses() const { return statMisses; }
    int getMissingUsed() const { return statMissingUsed; }
    int getCachedCount() const { return (int)cache.size(); }
    int getValidCount() const {
        int n = 0;
        for (const auto& kv : cache) if (kv.second.valid) ++n;
        return n;
    }
    void resetStats() const { statHits = statMisses = statMissingUsed = 0; }
};

} // namespace UI

#endif // ITEM_TEXTURE_MANAGER_H
