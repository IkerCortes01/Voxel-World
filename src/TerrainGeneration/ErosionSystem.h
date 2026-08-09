#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

// ============================================================================
// ADVANCED EROSION SIMULATION SYSTEM
// ============================================================================
// Realistic geological erosion that creates natural terrain features through
// simulated physical processes over geological time.
//
// Features:
// - Thermal erosion (weathering, talus slopes, scree)
// - Hydraulic erosion (water carving terrain)
// - Sediment transport and deposition
// - Slope-based erosion rates
// - Bedrock vs sediment layers
// - Erosion strength based on rock type
// - Multi-pass simulation for realistic results
// ============================================================================

namespace VoxelWorld {
namespace TerrainGen {

// Rock hardness affects erosion resistance
enum class RockType {
    SEDIMENTARY_SOFT,    // Sandstone, shale (erodes quickly)
    SEDIMENTARY_HARD,    // Limestone, dolomite
    IGNEOUS,             // Granite, basalt (erosion resistant)
    METAMORPHIC,         // Marble, quartzite
    VOLCANIC,            // Pumice, obsidian
};

// Erosion parameters for a location
struct ErosionData {
    float sediment;           // Accumulated sediment depth
    float waterFlow;          // Water flow accumulation
    float erosionAmount;      // Total erosion applied
    float depositionAmount;   // Total deposition
    float slope;              // Terrain slope at this point
    RockType rockType;        // Type of bedrock
};

class ErosionSystem {
private:
    struct HeightCell {
        float height;         // Current height
        float bedrock;        // Original bedrock height
        float sediment;       // Accumulated sediment
        float water;          // Water amount
        float waterVelocity;  // Water flow speed
        float suspendedSediment; // Sediment carried by water
        RockType rockType;    // Bedrock type
    };

    int resolution;           // Simulation grid resolution
    float worldScale;         // Blocks per cell
    unsigned int seed;

    // Erosion constants
    static constexpr float THERMAL_EROSION_RATE = 0.1f;
    static constexpr float THERMAL_TALUS_ANGLE = 0.7f; // ~35 degrees
    static constexpr float HYDRAULIC_EROSION_RATE = 0.05f;
    static constexpr float SEDIMENT_CAPACITY_CONSTANT = 4.0f;
    static constexpr float DEPOSITION_RATE = 0.3f;
    static constexpr float EVAPORATION_RATE = 0.01f;
    static constexpr float GRAVITY = 9.81f;

    // Get rock hardness factor (0 = soft, 1 = hard)
    float getRockHardness(RockType type) const {
        switch (type) {
            case RockType::SEDIMENTARY_SOFT: return 0.2f;
            case RockType::SEDIMENTARY_HARD: return 0.5f;
            case RockType::IGNEOUS: return 0.9f;
            case RockType::METAMORPHIC: return 0.8f;
            case RockType::VOLCANIC: return 0.6f;
            default: return 0.5f;
        }
    }

    // Hash for deterministic randomness
    unsigned int hash(unsigned int x, unsigned int y) const {
        unsigned int h = seed + x * 374761393 + y * 668265263;
        h = (h ^ (h >> 13)) * 1274126177;
        h = h ^ (h >> 16);
        return h;
    }

    float hashFloat(unsigned int x, unsigned int y) const {
        return (float)hash(x, y) / (float)UINT32_MAX;
    }

public:
    ErosionSystem(unsigned int worldSeed) : seed(worldSeed), resolution(512), worldScale(4.0f) {}

    // ========================================================================
    // THERMAL EROSION (Weathering and Gravity-Based Erosion)
    // ========================================================================
    // Simulates:
    // - Freeze-thaw weathering
    // - Gravity-driven mass movement
    // - Talus slope formation
    // - Scree accumulation
    void applyThermalErosion(std::vector<HeightCell>& grid, int width, int height, int iterations) {
        for (int iter = 0; iter < iterations; iter++) {
            std::vector<HeightCell> newGrid = grid;

            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    int idx = y * width + x;
                    float centerHeight = grid[idx].height;

                    // Check all 8 neighbors for slope
                    float maxHeightDiff = 0.0f;
                    int maxDiffIdx = idx;

                    // 8-directional neighbors
                    int offsets[8][2] = {
                        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
                        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
                    };

                    for (int i = 0; i < 8; i++) {
                        int nx = x + offsets[i][0];
                        int ny = y + offsets[i][1];
                        int nidx = ny * width + nx;

                        float heightDiff = centerHeight - grid[nidx].height;
                        if (heightDiff > maxHeightDiff) {
                            maxHeightDiff = heightDiff;
                            maxDiffIdx = nidx;
                        }
                    }

                    // If slope exceeds talus angle, erode
                    float distance = (maxDiffIdx != idx) ? sqrtf((float)((offsets[0][0] * offsets[0][0]) + (offsets[0][1] * offsets[0][1]))) : 1.0f;
                    float slope = maxHeightDiff / distance;

                    if (slope > THERMAL_TALUS_ANGLE) {
                        // Calculate erosion amount based on slope and rock hardness
                        float hardness = getRockHardness(grid[idx].rockType);
                        float erosionAmount = (slope - THERMAL_TALUS_ANGLE) * THERMAL_EROSION_RATE * (1.0f - hardness);

                        // Remove material from high point
                        newGrid[idx].height -= erosionAmount;
                        newGrid[idx].sediment += erosionAmount * 0.3f; // Some stays as sediment

                        // Deposit at low point
                        newGrid[maxDiffIdx].height += erosionAmount * 0.7f;
                        newGrid[maxDiffIdx].sediment += erosionAmount * 0.7f;
                    }
                }
            }

            grid = newGrid;
        }
    }

    // ========================================================================
    // HYDRAULIC EROSION (Water-Based Erosion)
    // ========================================================================
    // Simulates:
    // - Rainfall and water accumulation
    // - Water flow following gravity
    // - Erosion by flowing water
    // - Sediment transport
    // - Sediment deposition in low-energy areas
    void applyHydraulicErosion(std::vector<HeightCell>& grid, int width, int height,
                               int iterations, float rainAmount = 1.0f) {

        for (int iter = 0; iter < iterations; iter++) {
            // Step 1: Add rainfall
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int idx = y * width + x;
                    // Randomized rainfall
                    grid[idx].water += rainAmount * (0.8f + hashFloat(x + iter, y + iter) * 0.4f);
                }
            }

            // Step 2: Flow simulation (multiple sub-iterations for stability)
            for (int subIter = 0; subIter < 5; subIter++) {
                std::vector<float> newWater(width * height, 0.0f);
                std::vector<float> newSediment(width * height, 0.0f);

                for (int y = 1; y < height - 1; y++) {
                    for (int x = 1; x < width - 1; x++) {
                        int idx = y * width + x;

                        if (grid[idx].water < 0.01f) continue;

                        float totalHeight = grid[idx].height + grid[idx].water;
                        float totalOutflow = 0.0f;
                        float outflow[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // N, S, E, W

                        // Calculate outflow to neighbors
                        int neighbors[4][2] = {{0, -1}, {0, 1}, {1, 0}, {-1, 0}};

                        for (int i = 0; i < 4; i++) {
                            int nx = x + neighbors[i][0];
                            int ny = y + neighbors[i][1];
                            int nidx = ny * width + nx;

                            float neighborHeight = grid[nidx].height + grid[nidx].water;
                            float heightDiff = totalHeight - neighborHeight;

                            if (heightDiff > 0.0f) {
                                outflow[i] = fminf(grid[idx].water, heightDiff);
                                totalOutflow += outflow[i];
                            }
                        }

                        // Normalize outflow to not exceed available water
                        if (totalOutflow > grid[idx].water) {
                            float scale = grid[idx].water / totalOutflow;
                            for (int i = 0; i < 4; i++) {
                                outflow[i] *= scale;
                            }
                            totalOutflow = grid[idx].water;
                        }

                        // Calculate water velocity (for erosion)
                        float velocity = totalOutflow / fmaxf(grid[idx].water, 0.01f);
                        grid[idx].waterVelocity = velocity;

                        // Erosion capacity based on velocity and slope
                        float slope = 0.0f;
                        for (int i = 0; i < 4; i++) {
                            int nx = x + neighbors[i][0];
                            int ny = y + neighbors[i][1];
                            int nidx = ny * width + nx;
                            float s = fabs(grid[idx].height - grid[nidx].height);
                            slope = fmaxf(slope, s);
                        }

                        float sedimentCapacity = SEDIMENT_CAPACITY_CONSTANT * velocity * grid[idx].water * slope;

                        // Erosion or deposition
                        float hardness = getRockHardness(grid[idx].rockType);

                        if (grid[idx].suspendedSediment < sedimentCapacity) {
                            // Erode
                            float erosionAmount = (sedimentCapacity - grid[idx].suspendedSediment) *
                                                 HYDRAULIC_EROSION_RATE * (1.0f - hardness);

                            if (grid[idx].sediment > 0.0f) {
                                // Erode sediment first (easier)
                                float sedimentEroded = fminf(erosionAmount, grid[idx].sediment);
                                grid[idx].sediment -= sedimentEroded;
                                grid[idx].suspendedSediment += sedimentEroded;
                                erosionAmount -= sedimentEroded;
                            }

                            // Then erode bedrock
                            if (erosionAmount > 0.0f) {
                                grid[idx].height -= erosionAmount * 0.5f; // Bedrock erodes slower
                                grid[idx].suspendedSediment += erosionAmount * 0.5f;
                            }

                        } else {
                            // Deposit
                            float depositionAmount = (grid[idx].suspendedSediment - sedimentCapacity) * DEPOSITION_RATE;
                            grid[idx].suspendedSediment -= depositionAmount;
                            grid[idx].sediment += depositionAmount;
                            grid[idx].height += depositionAmount;
                        }

                        // Distribute water and sediment to neighbors
                        newWater[idx] += grid[idx].water - totalOutflow;
                        newSediment[idx] += grid[idx].suspendedSediment;

                        for (int i = 0; i < 4; i++) {
                            if (outflow[i] > 0.0f) {
                                int nx = x + neighbors[i][0];
                                int ny = y + neighbors[i][1];
                                int nidx = ny * width + nx;

                                float flowFraction = outflow[i] / totalOutflow;
                                newWater[nidx] += outflow[i];
                                newSediment[nidx] += grid[idx].suspendedSediment * flowFraction;
                            }
                        }

                        newSediment[idx] -= grid[idx].suspendedSediment; // Remove distributed sediment
                    }
                }

                // Update water and sediment
                for (int i = 0; i < width * height; i++) {
                    grid[i].water = fmaxf(0.0f, newWater[i]);
                    grid[i].suspendedSediment = fmaxf(0.0f, newSediment[i]);
                }
            }

            // Step 3: Evaporation
            for (int i = 0; i < width * height; i++) {
                grid[i].water *= (1.0f - EVAPORATION_RATE);

                // Deposit remaining sediment when water evaporates
                if (grid[i].water < 0.1f && grid[i].suspendedSediment > 0.0f) {
                    grid[i].sediment += grid[i].suspendedSediment;
                    grid[i].height += grid[i].suspendedSediment;
                    grid[i].suspendedSediment = 0.0f;
                }
            }
        }
    }

    // Get erosion strength at world position (for integration with other systems)
    float getErosionStrength(float x, float z, float baseHeight, RockType rockType) const {
        // Calculate expected erosion based on height and rock type
        float hardness = getRockHardness(rockType);

        // Higher elevations erode more (exposed to weathering)
        float elevationFactor = fminf(1.0f, baseHeight / 1000.0f);

        // Base erosion strength
        float erosion = 0.5f;
        erosion += elevationFactor * 0.3f;
        erosion *= (1.0f - hardness * 0.5f);

        return fmaxf(0.0f, fminf(1.0f, erosion));
    }
};

} // namespace TerrainGen
} // namespace VoxelWorld
