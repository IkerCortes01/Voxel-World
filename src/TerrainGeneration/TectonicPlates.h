#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include <random>

// ============================================================================
// TECTONIC PLATE SIMULATION SYSTEM
// ============================================================================
// Revolutionary geological simulation that creates realistic continent
// formation through plate tectonics. This is the foundation of all terrain.
//
// Features:
// - Voronoi-based plate generation
// - Plate movement and rotation
// - Convergent boundaries (mountains, subduction)
// - Divergent boundaries (mid-ocean ridges)
// - Transform boundaries (fault lines)
// - Plate thickness and density (oceanic vs continental)
// - Geological time simulation
// ============================================================================

namespace VoxelWorld {
namespace TerrainGen {

// Plate types based on composition and density
enum class PlateType {
    OCEANIC,     // Dense, thin, younger rock (basalt)
    CONTINENTAL, // Less dense, thick, older rock (granite)
    TRANSITIONAL // Mixed composition (coastal regions)
};

// Plate boundary types
enum class BoundaryType {
    CONVERGENT,  // Plates colliding (mountains/trenches)
    DIVERGENT,   // Plates separating (ridges/rifts)
    TRANSFORM,   // Plates sliding past (fault lines)
    NONE         // Interior of plate
};

// Represents a point defining a tectonic plate
struct PlatePoint {
    float x, z;              // World position
    float vx, vz;            // Velocity vector
    float rotation;          // Angular velocity
    PlateType type;          // Oceanic or continental
    float thickness;         // Plate thickness (km)
    float density;           // Plate density (g/cm³)
    float age;               // Geological age (millions of years)
    int plateID;             // Unique plate identifier
};

// Boundary information between plates
struct PlateBoundary {
    int plateA, plateB;      // IDs of adjacent plates
    BoundaryType type;       // Type of boundary
    float strength;          // Interaction strength (0-1)
    float angle;             // Collision angle
};

// Data at a specific world location
struct TectonicData {
    int plateID;             // Which plate owns this location
    PlateType plateType;     // Type of plate
    float distanceToEdge;    // Distance to nearest plate boundary
    BoundaryType boundaryType; // Type of nearest boundary
    float uplift;            // Vertical displacement (meters)
    float baseHeight;        // Base elevation from tectonics
    float crustalThickness;  // Thickness of crust here
    float geologicalAge;     // Age of rock formation
    float stress;            // Tectonic stress level
};

class TectonicPlateSystem {
private:
    std::vector<PlatePoint> plates;
    std::vector<PlateBoundary> boundaries;
    unsigned int seed;
    int numPlates;
    float simulationTime; // Millions of years simulated

    // Noise for natural variation
    std::mt19937 rng;

    // Constants
    static constexpr float PLATE_SPEED_SCALE = 0.00001f; // cm/year to blocks
    static constexpr float CONVERGENT_UPLIFT = 4000.0f;  // Max mountain height (meters)
    static constexpr float DIVERGENT_DEPTH = 2000.0f;    // Max ocean trench depth
    static constexpr float OCEANIC_DENSITY = 3.0f;       // g/cm³
    static constexpr float CONTINENTAL_DENSITY = 2.7f;   // g/cm³
    static constexpr float OCEANIC_THICKNESS = 7.0f;     // km
    static constexpr float CONTINENTAL_THICKNESS = 35.0f; // km

    // Hash function for deterministic randomness
    unsigned int hash(unsigned int x) const {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

    float hashFloat(unsigned int x) const {
        return (float)hash(x) / (float)UINT32_MAX;
    }

    // Find nearest plate to a world position
    int findNearestPlate(float x, float z) const {
        if (plates.empty()) return 0;

        int nearest = 0;
        float minDist = INFINITY;

        for (size_t i = 0; i < plates.size(); i++) {
            float dx = x - plates[i].x;
            float dz = z - plates[i].z;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist < minDist) {
                minDist = dist;
                nearest = (int)i;
            }
        }

        return nearest;
    }

    // Find second nearest plate for boundary detection
    int findSecondNearestPlate(float x, float z, int firstPlate) const {
        if (plates.size() < 2) return firstPlate;

        int secondNearest = (firstPlate == 0) ? 1 : 0;
        float minDist = INFINITY;

        for (size_t i = 0; i < plates.size(); i++) {
            if ((int)i == firstPlate) continue;

            float dx = x - plates[i].x;
            float dz = z - plates[i].z;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist < minDist) {
                minDist = dist;
                secondNearest = (int)i;
            }
        }

        return secondNearest;
    }

    // Calculate plate boundary type based on movement vectors
    BoundaryType calculateBoundaryType(const PlatePoint& p1, const PlatePoint& p2,
                                       float x, float z) const {
        // Vector from boundary point to plate centers
        float dx1 = p1.x - x;
        float dz1 = p1.z - z;
        float dx2 = p2.x - x;
        float dz2 = p2.z - z;

        // Normalize
        float len1 = sqrtf(dx1 * dx1 + dz1 * dz1);
        float len2 = sqrtf(dx2 * dx2 + dz2 * dz2);
        if (len1 > 0.001f) { dx1 /= len1; dz1 /= len1; }
        if (len2 > 0.001f) { dx2 /= len2; dz2 /= len2; }

        // Relative velocity
        float dvx = p1.vx - p2.vx;
        float dvz = p1.vz - p2.vz;

        // Dot product with boundary normal (average of plate directions)
        float nx = (dx1 + dx2) * 0.5f;
        float nz = (dz1 + dz2) * 0.5f;
        float dot = dvx * nx + dvz * nz;

        // Cross product for transform detection
        float cross = dvx * nz - dvz * nx;

        if (fabs(dot) > fabs(cross)) {
            // Primarily moving toward/away from each other
            return (dot > 0.0f) ? BoundaryType::CONVERGENT : BoundaryType::DIVERGENT;
        } else {
            // Primarily sliding past each other
            return BoundaryType::TRANSFORM;
        }
    }

    // Calculate uplift/subsidence at boundary
    float calculateUplift(BoundaryType boundaryType, const PlatePoint& p1,
                         const PlatePoint& p2, float distToEdge) const {
        // Smooth transition from boundary to interior
        float edgeFactor = expf(-distToEdge * 0.001f); // Decay over distance

        switch (boundaryType) {
            case BoundaryType::CONVERGENT: {
                // Mountain formation
                // Continental-continental: Himalayas-style
                // Oceanic-continental: Andes-style
                // Oceanic-oceanic: Island arcs

                float uplift = CONVERGENT_UPLIFT;

                if (p1.type == PlateType::OCEANIC && p2.type == PlateType::OCEANIC) {
                    // Subduction zone - one plate goes under
                    uplift *= 0.4f; // Island arcs are lower
                } else if (p1.type == PlateType::CONTINENTAL && p2.type == PlateType::CONTINENTAL) {
                    // Continental collision - massive mountains
                    uplift *= 1.2f; // Himalayas-style
                } else {
                    // Oceanic-continental subduction
                    uplift *= 0.8f; // Andes-style
                }

                return uplift * edgeFactor;
            }

            case BoundaryType::DIVERGENT: {
                // Rift valleys and mid-ocean ridges
                // Creates lower elevation (spreading center)
                return -DIVERGENT_DEPTH * edgeFactor * 0.5f;
            }

            case BoundaryType::TRANSFORM: {
                // Fault lines - minimal vertical displacement
                // But creates rough terrain
                return 0.0f;
            }

            default:
                return 0.0f;
        }
    }

public:
    TectonicPlateSystem(unsigned int worldSeed, int plateCount = 12)
        : seed(worldSeed), numPlates(plateCount), simulationTime(0.0f), rng(worldSeed) {

        generatePlates();
        simulatePlateMovement(100.0f); // Simulate 100 million years
    }

    // Generate initial tectonic plates using Voronoi distribution
    void generatePlates() {
        plates.clear();
        plates.reserve(numPlates);

        // Generate evenly distributed plate centers using Poisson disk sampling
        // This creates more natural distribution than pure random

        for (int i = 0; i < numPlates; i++) {
            PlatePoint plate;

            // Distribute plates across world
            float angle = (float)i / (float)numPlates * 2.0f * 3.14159f;
            float radius = 5000.0f + hashFloat(seed + i * 1000) * 10000.0f;

            plate.x = cosf(angle) * radius;
            plate.z = sinf(angle) * radius;

            // Random velocity (simulates millions of years of movement)
            float speed = 0.01f + hashFloat(seed + i * 2000) * 0.05f; // 1-6 cm/year
            float direction = hashFloat(seed + i * 3000) * 6.28318f;
            plate.vx = cosf(direction) * speed;
            plate.vz = sinf(direction) * speed;

            // Random rotation
            plate.rotation = (hashFloat(seed + i * 4000) - 0.5f) * 0.001f;

            // Determine plate type (70% oceanic, 30% continental - like Earth)
            if (hashFloat(seed + i * 5000) > 0.3f) {
                plate.type = PlateType::OCEANIC;
                plate.thickness = OCEANIC_THICKNESS;
                plate.density = OCEANIC_DENSITY;
                plate.age = hashFloat(seed + i * 6000) * 200.0f; // 0-200 million years
            } else {
                plate.type = PlateType::CONTINENTAL;
                plate.thickness = CONTINENTAL_THICKNESS;
                plate.density = CONTINENTAL_DENSITY;
                plate.age = 500.0f + hashFloat(seed + i * 6000) * 2500.0f; // 500-3000 million years (older)
            }

            plate.plateID = i;
            plates.push_back(plate);
        }
    }

    // Simulate plate movement over geological time
    void simulatePlateMovement(float millionYears) {
        simulationTime += millionYears;

        // Move plates based on velocity
        for (auto& plate : plates) {
            plate.x += plate.vx * millionYears * PLATE_SPEED_SCALE;
            plate.z += plate.vz * millionYears * PLATE_SPEED_SCALE;
            plate.age += millionYears;
        }

        // Detect and store boundaries
        detectBoundaries();
    }

    // Detect plate boundaries
    void detectBoundaries() {
        boundaries.clear();

        // Sample points between plates to find boundaries
        // In production, this would be done more efficiently
        for (size_t i = 0; i < plates.size(); i++) {
            for (size_t j = i + 1; j < plates.size(); j++) {
                PlateBoundary boundary;
                boundary.plateA = (int)i;
                boundary.plateB = (int)j;

                // Calculate boundary type at midpoint
                float midX = (plates[i].x + plates[j].x) * 0.5f;
                float midZ = (plates[i].z + plates[j].z) * 0.5f;
                boundary.type = calculateBoundaryType(plates[i], plates[j], midX, midZ);

                // Calculate interaction strength
                float dx = plates[j].x - plates[i].x;
                float dz = plates[j].z - plates[i].z;
                float dist = sqrtf(dx * dx + dz * dz);
                boundary.strength = 1.0f / (1.0f + dist * 0.0001f);

                boundaries.push_back(boundary);
            }
        }
    }

    // Get tectonic data at a world position
    TectonicData getTectonicData(float x, float z) const {
        TectonicData data;

        if (plates.empty()) {
            // Default data if no plates
            data.plateID = 0;
            data.plateType = PlateType::OCEANIC;
            data.distanceToEdge = INFINITY;
            data.boundaryType = BoundaryType::NONE;
            data.uplift = 0.0f;
            data.baseHeight = 0.0f;
            data.crustalThickness = OCEANIC_THICKNESS;
            data.geologicalAge = 0.0f;
            data.stress = 0.0f;
            return data;
        }

        // Find which plate this location belongs to (Voronoi cell)
        int nearestPlate = findNearestPlate(x, z);
        int secondPlate = findSecondNearestPlate(x, z, nearestPlate);

        const PlatePoint& plate1 = plates[nearestPlate];
        const PlatePoint& plate2 = plates[secondPlate];

        // Distance to plate centers
        float dx1 = x - plate1.x;
        float dz1 = z - plate1.z;
        float dist1 = sqrtf(dx1 * dx1 + dz1 * dz1);

        float dx2 = x - plate2.x;
        float dz2 = z - plate2.z;
        float dist2 = sqrtf(dx2 * dx2 + dz2 * dz2);

        // Distance to plate boundary (edge)
        data.distanceToEdge = fabs(dist1 - dist2);

        // Fill basic data
        data.plateID = nearestPlate;
        data.plateType = plate1.type;
        data.crustalThickness = plate1.thickness;
        data.geologicalAge = plate1.age;

        // Determine if we're at a boundary
        float boundaryThreshold = 500.0f; // Within 500 blocks of boundary
        if (data.distanceToEdge < boundaryThreshold) {
            // We're at a plate boundary
            data.boundaryType = calculateBoundaryType(plate1, plate2, x, z);
            data.uplift = calculateUplift(data.boundaryType, plate1, plate2, data.distanceToEdge);
            data.stress = 1.0f - (data.distanceToEdge / boundaryThreshold);
        } else {
            // Interior of plate
            data.boundaryType = BoundaryType::NONE;
            data.uplift = 0.0f;
            data.stress = 0.0f;
        }

        // Calculate base height from tectonics
        if (plate1.type == PlateType::CONTINENTAL) {
            data.baseHeight = 200.0f; // Continental platforms above sea level
        } else {
            data.baseHeight = -200.0f; // Oceanic crust below sea level
        }

        // Add uplift from boundaries
        data.baseHeight += data.uplift;

        return data;
    }

    // Get number of plates
    int getPlateCount() const { return (int)plates.size(); }

    // Get simulation time
    float getSimulationTime() const { return simulationTime; }
};

} // namespace TerrainGen
} // namespace VoxelWorld
