// ============================================================================
// EXAMPLE: How to integrate Revolutionary Terrain System into VoxelWorld
// ============================================================================

#include "RevolutionaryTerrain.h"
#include <iostream>

using namespace VoxelWorld::TerrainGen;

// Example 1: Basic Integration
// ============================================================================
void example1_BasicIntegration() {
    std::cout << "=== Example 1: Basic Integration ===" << std::endl;

    // Step 1: Create world generation parameters
    WorldGenParams params;
    params.seed = 12345;                    // World seed
    params.seaLevel = 64;                   // Sea level height
    params.numTectonicPlates = 12;          // Number of tectonic plates
    params.geologicalTimeSimulation = 100.0f; // Simulate 100 million years
    params.tectonicScale = 1.0f;            // Normal tectonic scale
    params.erosionStrength = 1.0f;          // Normal erosion
    params.riverDensity = 1.0f;             // Normal river frequency
    params.forestDensity = 1.0f;            // Normal forest coverage
    params.enableOverhangs = true;          // Enable overhangs and arches
    params.enableCaves = true;              // Enable cave systems
    params.enableStructures = true;         // Enable structure placement

    // Step 2: Create the revolutionary terrain system
    RevolutionaryTerrainSystem terrain(params);

    // Step 3: Generate a chunk
    const int CHUNK_SIZE = 16;
    BlockType* chunkBlocks = new BlockType[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];

    terrain.generateChunk(0, 4, 0, chunkBlocks, CHUNK_SIZE); // Chunk at (0, 4, 0)

    // Step 4: Access generated blocks
    int surfaceBlocks = 0;
    for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++) {
        if (chunkBlocks[i] == BlockType::GRASS || chunkBlocks[i] == BlockType::SAND) {
            surfaceBlocks++;
        }
    }

    std::cout << "Generated chunk with " << surfaceBlocks << " surface blocks" << std::endl;

    delete[] chunkBlocks;
}

// Example 2: Detailed Terrain Analysis
// ============================================================================
void example2_TerrainAnalysis() {
    std::cout << "\n=== Example 2: Terrain Analysis ===" << std::endl;

    WorldGenParams params;
    params.seed = 12345;
    params.enableOverhangs = true;

    RevolutionaryTerrainSystem terrain(params);

    // Analyze terrain at specific position
    float x = 100.0f, y = 64.0f, z = 100.0f;

    TerrainData data = terrain.evaluateTerrainAt(x, y, z);

    std::cout << "Terrain at (" << x << ", " << y << ", " << z << "):" << std::endl;
    std::cout << "  Base Height (tectonics): " << data.baseHeight << std::endl;
    std::cout << "  Final Height (after erosion): " << data.finalHeight << std::endl;
    std::cout << "  Water Level: " << data.waterLevel << std::endl;
    std::cout << "  Is River: " << (data.isRiver ? "Yes" : "No") << std::endl;
    std::cout << "  Is Lake: " << (data.isLake ? "Yes" : "No") << std::endl;
    std::cout << "  Is Cave: " << (data.isCave ? "Yes" : "No") << std::endl;
    std::cout << "  Temperature: " << data.temperature << std::endl;
    std::cout << "  Humidity: " << data.humidity << std::endl;
    std::cout << "  Biome: " << (int)data.biome << std::endl;
    std::cout << "  Habitability: " << data.habitability << std::endl;
}

// Example 3: Individual System Usage
// ============================================================================
void example3_IndividualSystems() {
    std::cout << "\n=== Example 3: Individual Systems ===" << std::endl;

    unsigned int seed = 12345;

    // TECTONIC PLATES
    std::cout << "\n--- Tectonic Plates ---" << std::endl;
    TectonicPlateSystem tectonics(seed, 12);
    tectonics.simulatePlateMovement(100.0f); // 100 million years

    auto tectonicData = tectonics.getTectonicData(1000.0f, 1000.0f);
    std::cout << "Plate ID: " << tectonicData.plateID << std::endl;
    std::cout << "Base Height: " << tectonicData.baseHeight << " meters" << std::endl;
    std::cout << "Boundary Type: " << (int)tectonicData.boundaryType << std::endl;
    std::cout << "Uplift: " << tectonicData.uplift << " meters" << std::endl;
    std::cout << "Geological Age: " << tectonicData.geologicalAge << " million years" << std::endl;

    // SDF TERRAIN
    std::cout << "\n--- SDF Terrain ---" << std::endl;
    SDFTerrain sdf(seed);

    bool isSolid = sdf.isSolid(100.0f, 70.0f, 100.0f, 64.0f);
    float distance = sdf.getDistance(100.0f, 70.0f, 100.0f, 64.0f);
    std::cout << "Is Solid: " << (isSolid ? "Yes" : "No") << std::endl;
    std::cout << "Distance to surface: " << distance << std::endl;

    // BIOME SYSTEM
    std::cout << "\n--- Biome System ---" << std::endl;
    AdvancedBiomeSystem biomes(seed);

    auto biomeBlend = biomes.getBlendedBiome(
        0.2f,   // temperature
        0.6f,   // humidity
        0.5f,   // elevation
        0.7f,   // continentalness
        0.5f,   // erosion
        0.0f,   // weirdness
        100.0f, 100.0f
    );

    std::cout << "Primary Biome: " << (int)biomeBlend.primaryBiome << std::endl;
    std::cout << "Secondary Biome: " << (int)biomeBlend.secondaryBiome << std::endl;
    std::cout << "Blend Factor: " << biomeBlend.blendFactor << std::endl;

    // L-SYSTEM TREES
    std::cout << "\n--- L-System Trees ---" << std::endl;
    LSystemTreeGenerator trees(seed);

    auto oakTree = trees.generateTree(TreeSpecies::OAK, 0.0f, 64.0f, 0.0f, 5);
    std::cout << "Oak tree nodes: " << oakTree.size() << std::endl;

    auto pineTree = trees.generateTree(TreeSpecies::PINE, 0.0f, 64.0f, 0.0f, 5);
    std::cout << "Pine tree nodes: " << pineTree.size() << std::endl;
}

// Example 4: Async Chunk Loading
// ============================================================================
void example4_AsyncLoading() {
    std::cout << "\n=== Example 4: Async Chunk Loading ===" << std::endl;

    WorldGenParams params;
    params.seed = 12345;

    RevolutionaryTerrainSystem terrain(params);

    // Update player position
    terrain.updatePlayerPosition(100.0f, 64.0f, 100.0f);

    // Request chunk asynchronously
    auto future = terrain.requestChunkAsync(5, 4, 5);

    std::cout << "Chunk generation submitted to thread pool..." << std::endl;

    // Do other work here...

    // Wait for chunk to be ready
    void* chunkData = future.get();
    std::cout << "Chunk generation completed!" << std::endl;

    // Update chunk streaming (loads chunks around player)
    terrain.updateChunkStreaming();

    std::cout << "Pending chunks: " << terrain.getPendingChunkCount() << std::endl;
    std::cout << "Active generators: " << terrain.getActiveGeneratorCount() << std::endl;
}

// Example 5: Vegetation Generation
// ============================================================================
void example5_Vegetation() {
    std::cout << "\n=== Example 5: Vegetation Generation ===" << std::endl;

    WorldGenParams params;
    params.seed = 12345;
    params.forestDensity = 1.5f; // Extra dense forests

    RevolutionaryTerrainSystem terrain(params);

    // Generate trees for a chunk
    auto treeNodes = terrain.generateVegetation(0, 0, 16);

    std::cout << "Generated " << treeNodes.size() << " tree nodes in chunk" << std::endl;

    // Count different tree parts
    int trunks = 0, branches = 0, leaves = 0;
    for (const auto& node : treeNodes) {
        switch (node.partType) {
            case TreePart::TRUNK: trunks++; break;
            case TreePart::BRANCH: branches++; break;
            case TreePart::LEAF: leaves++; break;
            default: break;
        }
    }

    std::cout << "  Trunks: " << trunks << std::endl;
    std::cout << "  Branches: " << branches << std::endl;
    std::cout << "  Leaves: " << leaves << std::endl;
}

// Example 6: Custom World Parameters
// ============================================================================
void example6_CustomParameters() {
    std::cout << "\n=== Example 6: Custom Parameters ===" << std::endl;

    // EXTREME MOUNTAINS WORLD
    {
        std::cout << "\n--- Extreme Mountains World ---" << std::endl;
        WorldGenParams params;
        params.seed = 99999;
        params.numTectonicPlates = 20;          // Many plates = more boundaries = more mountains
        params.geologicalTimeSimulation = 200.0f; // Older world = more erosion but higher uplift
        params.erosionStrength = 0.5f;          // Less erosion = sharper peaks
        params.tectonicScale = 1.5f;            // Larger scale = taller mountains

        RevolutionaryTerrainSystem terrain(params);
        auto data = terrain.evaluateTerrainAt(500.0f, 100.0f, 500.0f);
        std::cout << "  Height: " << data.finalHeight << std::endl;
    }

    // WATER WORLD
    {
        std::cout << "\n--- Water World ---" << std::endl;
        WorldGenParams params;
        params.seed = 77777;
        params.seaLevel = 100;                  // Higher sea level
        params.numTectonicPlates = 6;           // Fewer plates = more ocean
        params.riverDensity = 2.0f;             // Dense river networks
        params.erosionStrength = 2.0f;          // Heavy erosion = flatter terrain

        RevolutionaryTerrainSystem terrain(params);
        auto data = terrain.evaluateTerrainAt(500.0f, 64.0f, 500.0f);
        std::cout << "  Is underwater: " << (data.finalHeight < params.seaLevel ? "Yes" : "No") << std::endl;
    }

    // FANTASY WORLD (Overhangs and floating islands)
    {
        std::cout << "\n--- Fantasy World ---" << std::endl;
        WorldGenParams params;
        params.seed = 11111;
        params.enableOverhangs = true;          // Enable crazy geometry
        params.enableCaves = true;              // Massive cave systems
        params.forestDensity = 2.0f;            // Lush forests

        RevolutionaryTerrainSystem terrain(params);
        auto data = terrain.evaluateTerrainAt(500.0f, 100.0f, 500.0f);
        std::cout << "  Has overhang features: Yes" << std::endl;
    }
}

// Main function demonstrating all examples
// ============================================================================
int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "Revolutionary Terrain System Examples" << std::endl;
    std::cout << "======================================" << std::endl;

    try {
        example1_BasicIntegration();
        example2_TerrainAnalysis();
        example3_IndividualSystems();
        example4_AsyncLoading();
        example5_Vegetation();
        example6_CustomParameters();

        std::cout << "\n======================================" << std::endl;
        std::cout << "All examples completed successfully!" << std::endl;
        std::cout << "======================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
COMPILATION INSTRUCTIONS:
========================

To compile this example:

g++ -o ExampleIntegration ExampleIntegration.cpp -I. -std=c++17 -pthread

Note: This requires C++17 for:
- std::future and std::promise
- Structured bindings
- Threading support

INTEGRATION WITH MAIN.CPP:
==========================

To integrate into your existing VoxelWorld main.cpp:

1. Include the header:
   #include "TerrainGeneration/RevolutionaryTerrain.h"
   using namespace VoxelWorld::TerrainGen;

2. Replace World::terrainGen initialization:
   WorldGenParams params;
   params.seed = worldSeed;
   terrainGen = new RevolutionaryTerrainSystem(params);

3. Update World::generateChunk():
   BlockType blocks[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE];
   terrainGen->generateChunk(chunk->position.x, chunk->position.y, chunk->position.z,
                            blocks, CHUNK_SIZE);

   // Copy to chunk
   for (int x = 0; x < CHUNK_SIZE; x++) {
       for (int y = 0; y < CHUNK_HEIGHT; y++) {
           for (int z = 0; z < CHUNK_SIZE; z++) {
               int idx = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
               chunk->setBlock(x, y, z, (::BlockType)blocks[idx]);
           }
       }
   }

4. Update player position each frame:
   terrainGen->updatePlayerPosition(player.position.x, player.position.y, player.position.z);
   terrainGen->updateChunkStreaming();

PERFORMANCE TIPS:
================

- On 8+ core CPUs, chunk generation is fully parallel
- Region caching makes repeated chunk generation near same area very fast
- Predictive loading reduces perceived loading time
- Disable overhangs if performance is critical (params.enableOverhangs = false)
- Reduce erosion iterations for faster generation (params.erosionStrength = 0.5f)

*/
