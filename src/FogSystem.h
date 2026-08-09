#ifndef FOG_SYSTEM_H
#define FOG_SYSTEM_H

#include <cmath>

// ============================================================================
// SISTEMA DE NIEBLA VOLUMÉTRICA AVANZADO
// ============================================================================
// Inspirado en Minecraft pero técnicamente superior:
// - Múltiples modos de fog (Linear, Exp, Exp2)
// - Color dinámico según hora del día / dimensión
// - Fog por altura (más denso cerca del suelo)
// - Integración con agua/lava
// - Transiciones suaves
// - Optimizado para tiempo real
// ============================================================================

namespace VoxelFog {

// ============================================================================
// MODOS DE FOG
// ============================================================================
enum FogMode {
    FOG_LINEAR = 0,     // Niebla lineal: f = (end - dist) / (end - start)
    FOG_EXP = 1,        // Niebla exponencial: f = exp(-density * dist)
    FOG_EXP2 = 2,       // Niebla exp cuadrática: f = exp(-(density * dist)^2)
    FOG_DISABLED = 3    // Sin niebla
};

// ============================================================================
// PERFILES DE DIMENSIÓN
// ============================================================================
enum DimensionType {
    DIM_OVERWORLD = 0,  // Mundo normal
    DIM_NETHER = 1,     // Infierno
    DIM_END = 2,        // El End
    DIM_CAVE = 3        // Cuevas profundas
};

// ============================================================================
// ESTADOS DE CLIMA
// ============================================================================
enum WeatherType {
    WEATHER_CLEAR = 0,      // Despejado
    WEATHER_RAIN = 1,       // Lluvia
    WEATHER_STORM = 2,      // Tormenta
    WEATHER_SNOW = 3        // Nieve
};

// ============================================================================
// CONFIGURACIÓN DE FOG (Modificable en tiempo real)
// ============================================================================
struct FogConfig {
    // ⭐ ESTADO GLOBAL
    bool enabled;           // Activar/desactivar fog
    FogMode mode;           // Modo de fog (Linear/Exp/Exp2)

    // ⭐ DISTANCIA (para Linear)
    float start;            // Distancia donde empieza el fog (bloques)
    float end;              // Distancia donde termina el fog (bloques)

    // ⭐ DENSIDAD (para Exp/Exp2)
    float density;          // Densidad del fog (0.0 - 1.0)

    // ⭐ COLOR BASE
    float color[4];         // Color RGBA del fog (0.0 - 1.0)

    // ⭐ FOG POR ALTURA
    bool heightFogEnabled;  // Activar fog por altura
    float heightMin;        // Y mínimo (fog más denso)
    float heightMax;        // Y máximo (fog más ligero)
    float heightDensity;    // Multiplicador de densidad por altura

    // ⭐ AGUA / LAVA
    bool isUnderwater;      // Jugador bajo agua
    bool isInLava;          // Jugador en lava
    float underwaterDensity;  // Densidad bajo agua (0.0 - 1.0)
    float lavaDensity;      // Densidad en lava (0.0 - 1.0)
    float underwaterColor[4];  // Color fog bajo agua (azul)
    float lavaColor[4];     // Color fog en lava (rojo/naranja)

    // ⭐ TRANSICIONES
    float transitionSpeed;  // Velocidad de transición entre estados (0.0 - 1.0)

    // ⭐ DIMENSIÓN Y CLIMA
    DimensionType dimension;  // Dimensión actual
    WeatherType weather;      // Clima actual
    float timeOfDay;        // Hora del día (0.0 - 1.0, 0=medianoche, 0.5=mediodía)

    // ⭐ ILUMINACIÓN
    float sunIntensity;     // Intensidad del sol (0.0 - 1.0)
    float ambientLight;     // Luz ambiental (0.0 - 1.0)

    // Constructor con valores por defecto
    FogConfig() :
        enabled(true),
        mode(FOG_EXP2),
        start(100.0f),
        end(200.0f),
        density(0.015f),
        heightFogEnabled(true),
        heightMin(0.0f),
        heightMax(128.0f),
        heightDensity(0.5f),
        isUnderwater(false),
        isInLava(false),
        underwaterDensity(0.1f),
        lavaDensity(0.3f),
        transitionSpeed(0.05f),
        dimension(DIM_OVERWORLD),
        weather(WEATHER_CLEAR),
        timeOfDay(0.5f),
        sunIntensity(1.0f),
        ambientLight(0.3f)
    {
        // Color cielo por defecto (cian celeste)
        color[0] = 0.53f;
        color[1] = 0.81f;
        color[2] = 0.92f;
        color[3] = 1.0f;

        // Color bajo agua (azul profundo)
        underwaterColor[0] = 0.0f;
        underwaterColor[1] = 0.3f;
        underwaterColor[2] = 0.6f;
        underwaterColor[3] = 1.0f;

        // Color lava (rojo brillante)
        lavaColor[0] = 1.0f;
        lavaColor[1] = 0.3f;
        lavaColor[2] = 0.1f;
        lavaColor[3] = 1.0f;
    }
};

// ============================================================================
// SISTEMA DE FOG
// ============================================================================
class FogSystem {
private:
    FogConfig config;
    FogConfig targetConfig;  // Config objetivo para transiciones suaves

public:
    FogSystem() : config(), targetConfig(config) {}

    // ⭐ OBTENER CONFIGURACIÓN ACTUAL
    const FogConfig& getConfig() const { return config; }
    FogConfig& getConfigMutable() { return config; }

    // ⭐ ACTUALIZAR (llamar cada frame)
    void update(float deltaTime) {
        // Transiciones suaves (lerp)
        float t = config.transitionSpeed * deltaTime * 60.0f;  // Normalizado a 60 FPS
        if (t > 1.0f) t = 1.0f;

        // Lerp densidad
        config.density = lerp(config.density, targetConfig.density, t);

        // Lerp color
        for (int i = 0; i < 4; i++) {
            config.color[i] = lerp(config.color[i], targetConfig.color[i], t);
        }

        // Lerp altura
        config.heightDensity = lerp(config.heightDensity, targetConfig.heightDensity, t);

        // Actualizar color dinámico según hora del día
        updateDynamicColor();
    }

    // ⭐ SETTERS (con transiciones suaves)
    void setDensity(float density) {
        targetConfig.density = density;
    }

    void setColor(float r, float g, float b, float a = 1.0f) {
        targetConfig.color[0] = r;
        targetConfig.color[1] = g;
        targetConfig.color[2] = b;
        targetConfig.color[3] = a;
    }

    void setHeightDensity(float density) {
        targetConfig.heightDensity = density;
    }

    void setTimeOfDay(float time) {
        config.timeOfDay = time;
        targetConfig.timeOfDay = time;
    }

    void setDimension(DimensionType dim) {
        config.dimension = dim;
        targetConfig.dimension = dim;
        updateDimensionPreset();
    }

    void setWeather(WeatherType weather) {
        config.weather = weather;
        targetConfig.weather = weather;
        updateWeatherPreset();
    }

    void setUnderwater(bool underwater) {
        config.isUnderwater = underwater;
        targetConfig.isUnderwater = underwater;
    }

    void setInLava(bool inLava) {
        config.isInLava = inLava;
        targetConfig.isInLava = inLava;
    }

    // ⭐ GETTERS
    bool isEnabled() const { return config.enabled; }
    FogMode getMode() const { return config.mode; }
    float getDensity() const { return config.density; }

    // ⭐ OBTENER COLOR FINAL (con integración agua/lava)
    void getFinalColor(float out[4]) const {
        if (config.isInLava) {
            // Color lava (prioritario)
            for (int i = 0; i < 4; i++) {
                out[i] = config.lavaColor[i];
            }
        } else if (config.isUnderwater) {
            // Color agua
            for (int i = 0; i < 4; i++) {
                out[i] = config.underwaterColor[i];
            }
        } else {
            // Color normal
            for (int i = 0; i < 4; i++) {
                out[i] = config.color[i];
            }
        }
    }

    // ⭐ OBTENER DENSIDAD FINAL (con integración agua/lava/altura)
    float getFinalDensity(float playerY) const {
        if (!config.enabled) return 0.0f;

        float baseDensity = config.density;

        // Multiplicador por agua/lava
        if (config.isInLava) {
            baseDensity = config.lavaDensity;
        } else if (config.isUnderwater) {
            baseDensity = config.underwaterDensity;
        }

        // Multiplicador por altura (más denso abajo, menos denso arriba)
        if (config.heightFogEnabled && !config.isUnderwater && !config.isInLava) {
            float heightFactor = 1.0f;
            if (playerY < config.heightMax) {
                float t = (playerY - config.heightMin) / (config.heightMax - config.heightMin);
                t = clamp(t, 0.0f, 1.0f);
                heightFactor = lerp(1.0f + config.heightDensity, 1.0f, t);
            }
            baseDensity *= heightFactor;
        }

        return baseDensity;
    }

private:
    // ⭐ UTILIDADES MATEMÁTICAS
    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    static float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // ⭐ ACTUALIZAR COLOR DINÁMICO SEGÚN HORA DEL DÍA
    void updateDynamicColor() {
        if (config.dimension != DIM_OVERWORLD) return;

        float time = config.timeOfDay;

        // Colores clave (RGB normalizado)
        float midnight[3]  = {0.05f, 0.05f, 0.15f};  // Azul muy oscuro
        float sunrise[3]   = {0.90f, 0.50f, 0.30f};  // Naranja
        float noon[3]      = {0.53f, 0.81f, 0.92f};  // Cian celeste
        float sunset[3]    = {0.95f, 0.45f, 0.25f};  // Naranja rojizo

        float* color1;
        float* color2;
        float t;

        // Interpolar según hora
        if (time < 0.25f) {
            // 0.0 (medianoche) -> 0.25 (amanecer)
            color1 = midnight;
            color2 = sunrise;
            t = time / 0.25f;
        } else if (time < 0.5f) {
            // 0.25 (amanecer) -> 0.5 (mediodía)
            color1 = sunrise;
            color2 = noon;
            t = (time - 0.25f) / 0.25f;
        } else if (time < 0.75f) {
            // 0.5 (mediodía) -> 0.75 (atardecer)
            color1 = noon;
            color2 = sunset;
            t = (time - 0.5f) / 0.25f;
        } else {
            // 0.75 (atardecer) -> 1.0 (medianoche)
            color1 = sunset;
            color2 = midnight;
            t = (time - 0.75f) / 0.25f;
        }

        // Lerp entre colores
        targetConfig.color[0] = lerp(color1[0], color2[0], t);
        targetConfig.color[1] = lerp(color1[1], color2[1], t);
        targetConfig.color[2] = lerp(color1[2], color2[2], t);
        targetConfig.color[3] = 1.0f;

        // Modificar según clima
        if (config.weather == WEATHER_RAIN) {
            // Más gris en lluvia
            targetConfig.color[0] *= 0.7f;
            targetConfig.color[1] *= 0.7f;
            targetConfig.color[2] *= 0.7f;
        } else if (config.weather == WEATHER_STORM) {
            // Muy oscuro en tormenta
            targetConfig.color[0] *= 0.4f;
            targetConfig.color[1] *= 0.4f;
            targetConfig.color[2] *= 0.4f;
        }
    }

    // ⭐ PRESETS POR DIMENSIÓN
    void updateDimensionPreset() {
        switch (config.dimension) {
            case DIM_OVERWORLD:
                // Ya manejado por updateDynamicColor()
                targetConfig.density = 0.015f;
                targetConfig.heightFogEnabled = true;
                break;

            case DIM_NETHER:
                // Fog rojizo y denso
                targetConfig.color[0] = 0.6f;
                targetConfig.color[1] = 0.1f;
                targetConfig.color[2] = 0.1f;
                targetConfig.density = 0.05f;
                targetConfig.heightFogEnabled = false;
                break;

            case DIM_END:
                // Fog violeta/negro
                targetConfig.color[0] = 0.1f;
                targetConfig.color[1] = 0.0f;
                targetConfig.color[2] = 0.2f;
                targetConfig.density = 0.03f;
                targetConfig.heightFogEnabled = false;
                break;

            case DIM_CAVE:
                // Fog muy denso y oscuro
                targetConfig.color[0] = 0.05f;
                targetConfig.color[1] = 0.05f;
                targetConfig.color[2] = 0.05f;
                targetConfig.density = 0.08f;
                targetConfig.heightFogEnabled = false;
                break;
        }
    }

    // ⭐ PRESETS POR CLIMA
    void updateWeatherPreset() {
        switch (config.weather) {
            case WEATHER_RAIN:
                targetConfig.density = 0.025f;  // Más denso en lluvia
                break;

            case WEATHER_STORM:
                targetConfig.density = 0.04f;   // Muy denso en tormenta
                break;

            case WEATHER_CLEAR:
            default:
                targetConfig.density = 0.015f;  // Normal
                break;
        }
    }
};

} // namespace VoxelFog

#endif // FOG_SYSTEM_H
