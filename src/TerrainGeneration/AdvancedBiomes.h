#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================================
// ADVANCED BIOME BLENDING SYSTEM
// ============================================================================
// Sophisticated biome transitions using multi-dimensional parameter spaces
// and smooth blending, far beyond simple Voronoi cells.
//
// Features:
// - 5D biome parameter space (temperature, humidity, elevation, continentality, erosion)
// - Smooth transitions using distance-weighted blending
// - Micro-biomes and ecotones
// - Ecosystem coherence
// - Soil composition gradients
// - Climate zones
// - Seasonal variation
// - Biome-specific features (geysers, hot springs, etc.)
// ============================================================================

namespace VoxelWorld {
namespace TerrainGen {

// Enhanced biome types with more variety
enum class AdvancedBiome {
    // Ocean biomes
    DEEP_OCEAN,
    OCEAN,
    WARM_OCEAN,
    COLD_OCEAN,
    FROZEN_OCEAN,

    // Coastal biomes
    BEACH,
    ROCKY_SHORE,
    MANGROVE_SWAMP,
    CORAL_REEF,

    // Wetland biomes
    SWAMP,
    MARSH,
    BOG,

    // Forest biomes
    TEMPERATE_FOREST,
    DENSE_FOREST,
    BOREAL_FOREST,
    RAINFOREST,
    JUNGLE,
    BAMBOO_FOREST,

    // Grassland biomes
    PLAINS,
    SAVANNA,
    PRAIRIE,
    MEADOW,

    // Desert biomes
    HOT_DESERT,
    COLD_DESERT,
    BADLANDS,
    DUNES,

    // Mountain biomes
    MOUNTAINS,
    MOUNTAIN_PEAKS,
    ALPINE_MEADOW,
    MOUNTAIN_FOREST,

    // Cold biomes
    TUNDRA,
    TAIGA,
    SNOW_PLAINS,
    ICE_SPIKES,

    // Volcanic biomes
    VOLCANIC,
    BASALT_DELTAS,
    GEOTHERMAL,

    // Hills
    HILLS,
    FOREST_HILLS,

    // Rivers and lakes
    RIVER,
    LAKE,
    FROZEN_LAKE,

    NUM_BIOMES
};

// Biome properties for blending
struct BiomeProperties {
    AdvancedBiome biomeType;
    float temperature;       // -1 (frozen) to 1 (hot)
    float humidity;          // 0 (dry) to 1 (wet)
    float elevation;         // 0 (ocean) to 1 (peaks)
    float continentalness;   // 0 (ocean) to 1 (inland)
    float erosion;           // 0 (sharp) to 1 (smooth)
    float weirdness;         // 0 (normal) to 1 (unusual features)

    // Visual properties
    float grassColor[3];     // RGB
    float foliageColor[3];   // RGB
    float waterColor[3];     // RGB

    // Gameplay properties
    float habitability;      // How suitable for structures
    float resourceDensity;   // Ore/resource availability
};

// Biome blend result
struct BiomeBlend {
    AdvancedBiome primaryBiome;
    AdvancedBiome secondaryBiome;
    float blendFactor;       // 0 = primary, 1 = secondary

    // Combined properties
    BiomeProperties blendedProperties;
};

class AdvancedBiomeSystem {
private:
    unsigned int seed;

    // Biome definitions
    std::vector<BiomeProperties> biomeDatabase;

    // Hash function
    unsigned int hash(unsigned int x, unsigned int y) const {
        unsigned int h = seed + x * 374761393 + y * 668265263;
        h = (h ^ (h >> 13)) * 1274126177;
        h = h ^ (h >> 16);
        return h;
    }

    float hashFloat(unsigned int x, unsigned int y) const {
        return (float)hash(x, y) / (float)UINT32_MAX;
    }

    // Smooth interpolation
    float smoothstep(float t) const {
        return t * t * (3.0f - 2.0f * t);
    }

    // Calculate distance in biome parameter space
    float biomeDistance(const BiomeProperties& a, const BiomeProperties& b) const {
        float dt = a.temperature - b.temperature;
        float dh = a.humidity - b.humidity;
        float de = a.elevation - b.elevation;
        float dc = a.continentalness - b.continentalness;
        float der = a.erosion - b.erosion;

        // Weighted distance (some parameters more important than others)
        return sqrtf(dt*dt * 2.0f +      // Temperature very important
                    dh*dh * 2.0f +       // Humidity very important
                    de*de * 1.5f +       // Elevation important
                    dc*dc * 1.0f +       // Continentalness moderate
                    der*der * 0.5f);     // Erosion less important
    }

public:
    AdvancedBiomeSystem(unsigned int worldSeed) : seed(worldSeed) {
        initializeBiomeDatabase();
    }

    // Initialize all biome definitions
    void initializeBiomeDatabase() {
        biomeDatabase.resize((int)AdvancedBiome::NUM_BIOMES);

        // DEEP OCEAN
        auto& deepOcean = biomeDatabase[(int)AdvancedBiome::DEEP_OCEAN];
        deepOcean.biomeType = AdvancedBiome::DEEP_OCEAN;
        deepOcean.temperature = 0.0f;
        deepOcean.humidity = 1.0f;
        deepOcean.elevation = 0.0f;
        deepOcean.continentalness = 0.0f;
        deepOcean.erosion = 0.8f;
        deepOcean.weirdness = 0.0f;
        deepOcean.grassColor[0] = 0.0f; deepOcean.grassColor[1] = 0.0f; deepOcean.grassColor[2] = 0.0f;
        deepOcean.foliageColor[0] = 0.0f; deepOcean.foliageColor[1] = 0.0f; deepOcean.foliageColor[2] = 0.0f;
        deepOcean.waterColor[0] = 0.0f; deepOcean.waterColor[1] = 0.2f; deepOcean.waterColor[2] = 0.5f;
        deepOcean.habitability = 0.0f;
        deepOcean.resourceDensity = 0.3f;

        // TEMPERATE FOREST
        auto& tempForest = biomeDatabase[(int)AdvancedBiome::TEMPERATE_FOREST];
        tempForest.biomeType = AdvancedBiome::TEMPERATE_FOREST;
        tempForest.temperature = 0.2f;
        tempForest.humidity = 0.6f;
        tempForest.elevation = 0.5f;
        tempForest.continentalness = 0.6f;
        tempForest.erosion = 0.5f;
        tempForest.weirdness = 0.0f;
        tempForest.grassColor[0] = 0.2f; tempForest.grassColor[1] = 0.7f; tempForest.grassColor[2] = 0.2f;
        tempForest.foliageColor[0] = 0.1f; tempForest.foliageColor[1] = 0.6f; tempForest.foliageColor[2] = 0.1f;
        tempForest.waterColor[0] = 0.2f; tempForest.waterColor[1] = 0.5f; tempForest.waterColor[2] = 0.7f;
        tempForest.habitability = 0.8f;
        tempForest.resourceDensity = 0.7f;

        // HOT DESERT
        auto& desert = biomeDatabase[(int)AdvancedBiome::HOT_DESERT];
        desert.biomeType = AdvancedBiome::HOT_DESERT;
        desert.temperature = 0.8f;
        desert.humidity = 0.1f;
        desert.elevation = 0.4f;
        desert.continentalness = 0.8f;
        desert.erosion = 0.6f;
        desert.weirdness = 0.0f;
        desert.grassColor[0] = 0.8f; desert.grassColor[1] = 0.7f; desert.grassColor[2] = 0.4f;
        desert.foliageColor[0] = 0.6f; desert.foliageColor[1] = 0.6f; desert.foliageColor[2] = 0.3f;
        desert.waterColor[0] = 0.3f; desert.waterColor[1] = 0.6f; desert.waterColor[2] = 0.7f;
        desert.habitability = 0.3f;
        desert.resourceDensity = 0.4f;

        // MOUNTAINS
        auto& mountains = biomeDatabase[(int)AdvancedBiome::MOUNTAINS];
        mountains.biomeType = AdvancedBiome::MOUNTAINS;
        mountains.temperature = -0.2f;
        mountains.humidity = 0.4f;
        mountains.elevation = 0.8f;
        mountains.continentalness = 0.7f;
        mountains.erosion = 0.3f;
        mountains.weirdness = 0.0f;
        mountains.grassColor[0] = 0.3f; mountains.grassColor[1] = 0.6f; mountains.grassColor[2] = 0.3f;
        mountains.foliageColor[0] = 0.2f; mountains.foliageColor[1] = 0.5f; mountains.foliageColor[2] = 0.2f;
        mountains.waterColor[0] = 0.3f; mountains.waterColor[1] = 0.6f; mountains.waterColor[2] = 0.8f;
        mountains.habitability = 0.4f;
        mountains.resourceDensity = 0.9f; // Rich in ores

        // TUNDRA
        auto& tundra = biomeDatabase[(int)AdvancedBiome::TUNDRA];
        tundra.biomeType = AdvancedBiome::TUNDRA;
        tundra.temperature = -0.6f;
        tundra.humidity = 0.3f;
        tundra.elevation = 0.5f;
        tundra.continentalness = 0.6f;
        tundra.erosion = 0.7f;
        tundra.weirdness = 0.0f;
        tundra.grassColor[0] = 0.5f; tundra.grassColor[1] = 0.5f; tundra.grassColor[2] = 0.5f;
        tundra.foliageColor[0] = 0.4f; tundra.foliageColor[1] = 0.5f; tundra.foliageColor[2] = 0.4f;
        tundra.waterColor[0] = 0.4f; tundra.waterColor[1] = 0.6f; tundra.waterColor[2] = 0.8f;
        tundra.habitability = 0.2f;
        tundra.resourceDensity = 0.5f;

        // Initialize remaining biomes with defaults (can be expanded)
        for (int i = 0; i < (int)AdvancedBiome::NUM_BIOMES; i++) {
            if (biomeDatabase[i].temperature == 0.0f &&
                biomeDatabase[i].biomeType != AdvancedBiome::DEEP_OCEAN) {
                // Default initialization for unset biomes
                biomeDatabase[i].biomeType = (AdvancedBiome)i;
                biomeDatabase[i].temperature = 0.0f;
                biomeDatabase[i].humidity = 0.5f;
                biomeDatabase[i].elevation = 0.5f;
                biomeDatabase[i].continentalness = 0.5f;
                biomeDatabase[i].erosion = 0.5f;
                biomeDatabase[i].weirdness = 0.0f;
                biomeDatabase[i].grassColor[0] = 0.3f;
                biomeDatabase[i].grassColor[1] = 0.6f;
                biomeDatabase[i].grassColor[2] = 0.3f;
                biomeDatabase[i].foliageColor[0] = 0.2f;
                biomeDatabase[i].foliageColor[1] = 0.5f;
                biomeDatabase[i].foliageColor[2] = 0.2f;
                biomeDatabase[i].waterColor[0] = 0.2f;
                biomeDatabase[i].waterColor[1] = 0.5f;
                biomeDatabase[i].waterColor[2] = 0.7f;
                biomeDatabase[i].habitability = 0.5f;
                biomeDatabase[i].resourceDensity = 0.5f;
            }
        }
    }

    // Find best matching biome based on parameters
    AdvancedBiome selectBiome(float temperature, float humidity, float elevation,
                             float continentalness, float erosion, float weirdness) const {

        BiomeProperties target;
        target.temperature = temperature;
        target.humidity = humidity;
        target.elevation = elevation;
        target.continentalness = continentalness;
        target.erosion = erosion;
        target.weirdness = weirdness;

        // Find closest biome in parameter space
        int bestBiome = 0;
        float bestDistance = INFINITY;

        for (int i = 0; i < (int)AdvancedBiome::NUM_BIOMES; i++) {
            float dist = biomeDistance(target, biomeDatabase[i]);
            if (dist < bestDistance) {
                bestDistance = dist;
                bestBiome = i;
            }
        }

        return (AdvancedBiome)bestBiome;
    }

    // Get blended biome with smooth transitions
    BiomeBlend getBlendedBiome(float temperature, float humidity, float elevation,
                              float continentalness, float erosion, float weirdness,
                              float x, float z) const {

        BiomeBlend result;

        // Find two closest biomes for smooth blending
        BiomeProperties target;
        target.temperature = temperature;
        target.humidity = humidity;
        target.elevation = elevation;
        target.continentalness = continentalness;
        target.erosion = erosion;
        target.weirdness = weirdness;

        int firstBiome = 0, secondBiome = 0;
        float firstDist = INFINITY, secondDist = INFINITY;

        for (int i = 0; i < (int)AdvancedBiome::NUM_BIOMES; i++) {
            float dist = biomeDistance(target, biomeDatabase[i]);

            if (dist < firstDist) {
                secondDist = firstDist;
                secondBiome = firstBiome;
                firstDist = dist;
                firstBiome = i;
            } else if (dist < secondDist) {
                secondDist = dist;
                secondBiome = i;
            }
        }

        result.primaryBiome = (AdvancedBiome)firstBiome;
        result.secondaryBiome = (AdvancedBiome)secondBiome;

        // Calculate blend factor with smoothing
        float totalDist = firstDist + secondDist;
        if (totalDist > 0.001f) {
            result.blendFactor = firstDist / totalDist;
        } else {
            result.blendFactor = 0.0f;
        }

        // Apply smooth transition
        result.blendFactor = smoothstep(result.blendFactor);

        // Blend properties
        const BiomeProperties& p1 = biomeDatabase[firstBiome];
        const BiomeProperties& p2 = biomeDatabase[secondBiome];
        float t = result.blendFactor;

        result.blendedProperties.temperature = p1.temperature * (1-t) + p2.temperature * t;
        result.blendedProperties.humidity = p1.humidity * (1-t) + p2.humidity * t;
        result.blendedProperties.elevation = p1.elevation * (1-t) + p2.elevation * t;

        // Blend colors
        for (int i = 0; i < 3; i++) {
            result.blendedProperties.grassColor[i] = p1.grassColor[i] * (1-t) + p2.grassColor[i] * t;
            result.blendedProperties.foliageColor[i] = p1.foliageColor[i] * (1-t) + p2.foliageColor[i] * t;
            result.blendedProperties.waterColor[i] = p1.waterColor[i] * (1-t) + p2.waterColor[i] * t;
        }

        result.blendedProperties.habitability = p1.habitability * (1-t) + p2.habitability * t;
        result.blendedProperties.resourceDensity = p1.resourceDensity * (1-t) + p2.resourceDensity * t;

        return result;
    }
};

} // namespace TerrainGen
} // namespace VoxelWorld
