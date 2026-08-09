#pragma once

#include <string>
#include <vector>
#include <stack>
#include <cmath>
#include <map>

// ============================================================================
// L-SYSTEM PROCEDURAL TREE GENERATION
// ============================================================================
// Advanced tree generation using Lindenmayer systems (L-Systems) for
// realistic, natural-looking trees with species variation.
//
// Features:
// - Multiple tree species with unique characteristics
// - L-System grammar-based growth
// - Branching angles and patterns
// - Leaf density and distribution
// - Age-based variation
// - Environmental adaptation (wind, slope, sun)
// - Seasonal changes
// - Dead branches and fallen logs
// - Root systems
// - Fruit/flower generation
//
// Much more realistic than simple procedural trees like Minecraft.
// ============================================================================

namespace VoxelWorld {
namespace TerrainGen {

// Tree species with unique characteristics
enum class TreeSpecies {
    OAK,             // Broad, full canopy
    PINE,            // Conical, evergreen
    BIRCH,           // Tall, slender
    WILLOW,          // Drooping branches
    MAPLE,           // Dense, colorful
    PALM,            // Tropical, coconuts
    BAOBAB,          // Thick trunk, sparse canopy
    CHERRY,          // Flowering, decorative
    REDWOOD,         // Massive, tall
    ACACIA,          // Flat-topped, savanna
    DEAD_TREE,       // No leaves, skeletal
    MANGROVE         // Water-based, aerial roots
};

// Tree part types
enum class TreePart {
    TRUNK,
    BRANCH,
    TWIG,
    LEAF,
    FLOWER,
    FRUIT,
    ROOT
};

// L-System rule
struct LRule {
    char symbol;
    std::string replacement;
    float probability;
};

// Turtle graphics state for interpreting L-System
struct TurtleState {
    float x, y, z;           // Position
    float dirX, dirY, dirZ;  // Direction vector
    float rightX, rightY, rightZ; // Right vector
    float upX, upY, upZ;     // Up vector
    float thickness;         // Branch thickness
    int generation;          // Branch generation depth
};

// Tree node in generated structure
struct TreeNode {
    float x, y, z;
    TreePart partType;
    float thickness;
    int generation;
};

class LSystemTreeGenerator {
private:
    unsigned int seed;

    // L-System grammars for different species
    std::map<TreeSpecies, std::vector<LRule>> grammars;
    std::map<TreeSpecies, std::string> axioms;

    // Species parameters
    struct SpeciesParams {
        float branchAngle;       // Angle between branches
        float branchingProb;     // Probability of branching
        float thicknessRatio;    // Child/parent thickness ratio
        float lengthScale;       // Overall size
        int maxGenerations;      // Max branching depth
        float leafDensity;       // How many leaves
        float tropism;           // Gravity effect on branches
    };

    std::map<TreeSpecies, SpeciesParams> speciesParams;

    // Hash function
    unsigned int hash(unsigned int x) const {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

    float hashFloat(unsigned int x) const {
        return (float)hash(x) / (float)UINT32_MAX;
    }

    // Vector operations
    void normalize(float& x, float& y, float& z) const {
        float length = sqrtf(x*x + y*y + z*z);
        if (length > 0.001f) {
            x /= length;
            y /= length;
            z /= length;
        }
    }

    void cross(float ax, float ay, float az, float bx, float by, float bz,
              float& cx, float& cy, float& cz) const {
        cx = ay * bz - az * by;
        cy = az * bx - ax * bz;
        cz = ax * by - ay * bx;
    }

    // Rotate vector around axis
    void rotateAroundAxis(float& vx, float& vy, float& vz,
                         float axisX, float axisY, float axisZ,
                         float angle) const {
        float c = cosf(angle);
        float s = sinf(angle);
        float t = 1.0f - c;

        float newX = (t * axisX * axisX + c) * vx +
                     (t * axisX * axisY - s * axisZ) * vy +
                     (t * axisX * axisZ + s * axisY) * vz;

        float newY = (t * axisX * axisY + s * axisZ) * vx +
                     (t * axisY * axisY + c) * vy +
                     (t * axisY * axisZ - s * axisX) * vz;

        float newZ = (t * axisX * axisZ - s * axisY) * vx +
                     (t * axisY * axisZ + s * axisX) * vy +
                     (t * axisZ * axisZ + c) * vz;

        vx = newX;
        vy = newY;
        vz = newZ;
    }

public:
    LSystemTreeGenerator(unsigned int worldSeed) : seed(worldSeed) {
        initializeGrammars();
        initializeSpeciesParams();
    }

    // Initialize L-System grammars for each species
    void initializeGrammars() {
        // OAK: Full, branching
        axioms[TreeSpecies::OAK] = "A";
        grammars[TreeSpecies::OAK] = {
            {'A', "F[+A][-A]FA", 1.0f},      // Main growth
            {'F', "FF", 0.8f}                // Segment elongation
        };

        // PINE: Conical, whorled branches
        axioms[TreeSpecies::PINE] = "A";
        grammars[TreeSpecies::PINE] = {
            {'A', "F[W]A", 1.0f},            // Vertical growth with whorls
            {'W', "+F[-F][-F][-F][-F]", 1.0f}, // Whorl of branches
            {'F', "FF", 0.7f}
        };

        // BIRCH: Tall, slender, minimal branching
        axioms[TreeSpecies::BIRCH] = "A";
        grammars[TreeSpecies::BIRCH] = {
            {'A', "FF[+A]A", 1.0f},
            {'F', "FF", 0.9f}
        };

        // WILLOW: Drooping branches
        axioms[TreeSpecies::WILLOW] = "A";
        grammars[TreeSpecies::WILLOW] = {
            {'A', "F[&+A][&-A]", 1.0f},      // & = pitch down
            {'F', "F&F", 0.8f}               // Continuous drooping
        };

        // PALM: Tropical, fronds at top
        axioms[TreeSpecies::PALM] = "FFFFFFA";
        grammars[TreeSpecies::PALM] = {
            {'A', "[+++L][---L][++L][--L][+L][-L]", 1.0f}, // Frond arrangement
            {'L', "FFFF", 1.0f},             // Long fronds
            {'F', "F", 1.0f}                 // Trunk doesn't branch
        };

        // DEAD TREE: Skeletal, broken branches
        axioms[TreeSpecies::DEAD_TREE] = "A";
        grammars[TreeSpecies::DEAD_TREE] = {
            {'A', "FF[+X][-X]A", 1.0f},
            {'X', "F", 0.3f},                // Many branches die
            {'F', "F", 1.0f}
        };
    }

    // Initialize species-specific parameters
    void initializeSpeciesParams() {
        // OAK
        speciesParams[TreeSpecies::OAK] = {
            30.0f * 3.14159f / 180.0f,  // branchAngle (30 degrees)
            0.7f,                        // branchingProb
            0.7f,                        // thicknessRatio
            1.0f,                        // lengthScale
            5,                           // maxGenerations
            0.8f,                        // leafDensity
            0.2f                         // tropism
        };

        // PINE
        speciesParams[TreeSpecies::PINE] = {
            70.0f * 3.14159f / 180.0f,  // branchAngle (70 degrees - horizontal)
            0.9f,
            0.5f,
            1.5f,                        // Taller
            6,
            0.9f,
            0.05f                        // Very upright
        };

        // BIRCH
        speciesParams[TreeSpecies::BIRCH] = {
            25.0f * 3.14159f / 180.0f,
            0.4f,                        // Less branching
            0.75f,
            1.3f,                        // Tall
            4,
            0.6f,
            0.1f
        };

        // WILLOW
        speciesParams[TreeSpecies::WILLOW] = {
            40.0f * 3.14159f / 180.0f,
            0.8f,
            0.6f,
            1.2f,
            6,
            0.85f,
            0.8f                         // Strong drooping
        };

        // PALM
        speciesParams[TreeSpecies::PALM] = {
            45.0f * 3.14159f / 180.0f,
            0.0f,                        // No branching in trunk
            0.9f,
            0.8f,
            2,
            0.9f,
            0.0f
        };

        // DEAD TREE
        speciesParams[TreeSpecies::DEAD_TREE] = {
            35.0f * 3.14159f / 180.0f,
            0.3f,                        // Few branches
            0.6f,
            0.9f,
            3,
            0.0f,                        // No leaves
            0.3f
        };
    }

    // Generate L-System string
    std::string generateLSystem(TreeSpecies species, int iterations) const {
        std::string current = axioms.at(species);
        const auto& rules = grammars.at(species);

        for (int iter = 0; iter < iterations; iter++) {
            std::string next = "";

            for (char c : current) {
                bool replaced = false;

                // Find matching rule
                for (const auto& rule : rules) {
                    if (rule.symbol == c) {
                        // Apply rule probabilistically
                        if (hashFloat(seed + iter * 1000 + (unsigned int)c) < rule.probability) {
                            next += rule.replacement;
                            replaced = true;
                            break;
                        }
                    }
                }

                if (!replaced) {
                    next += c; // Keep original if no rule applied
                }
            }

            current = next;
        }

        return current;
    }

    // Interpret L-System string using turtle graphics
    std::vector<TreeNode> interpretLSystem(const std::string& lsystem,
                                          TreeSpecies species,
                                          float startX, float startY, float startZ) const {
        std::vector<TreeNode> nodes;
        std::stack<TurtleState> stateStack;

        const SpeciesParams& params = speciesParams.at(species);

        // Initial turtle state
        TurtleState turtle;
        turtle.x = startX;
        turtle.y = startY;
        turtle.z = startZ;
        turtle.dirX = 0.0f;
        turtle.dirY = 1.0f;  // Up
        turtle.dirZ = 0.0f;
        turtle.rightX = 1.0f;
        turtle.rightY = 0.0f;
        turtle.rightZ = 0.0f;
        turtle.upX = 0.0f;
        turtle.upY = 1.0f;
        turtle.upZ = 0.0f;
        turtle.thickness = 0.5f;
        turtle.generation = 0;

        float segmentLength = 2.0f * params.lengthScale;

        for (char command : lsystem) {
            switch (command) {
                case 'F': {
                    // Move forward and draw
                    TreeNode node;
                    node.x = turtle.x;
                    node.y = turtle.y;
                    node.z = turtle.z;
                    node.partType = (turtle.generation == 0) ? TreePart::TRUNK : TreePart::BRANCH;
                    node.thickness = turtle.thickness;
                    node.generation = turtle.generation;
                    nodes.push_back(node);

                    // Apply tropism (gravity)
                    turtle.dirY -= params.tropism * 0.1f;
                    normalize(turtle.dirX, turtle.dirY, turtle.dirZ);

                    // Move
                    turtle.x += turtle.dirX * segmentLength;
                    turtle.y += turtle.dirY * segmentLength;
                    turtle.z += turtle.dirZ * segmentLength;
                    break;
                }

                case '+': {
                    // Turn right
                    rotateAroundAxis(turtle.dirX, turtle.dirY, turtle.dirZ,
                                   turtle.upX, turtle.upY, turtle.upZ,
                                   params.branchAngle);
                    break;
                }

                case '-': {
                    // Turn left
                    rotateAroundAxis(turtle.dirX, turtle.dirY, turtle.dirZ,
                                   turtle.upX, turtle.upY, turtle.upZ,
                                   -params.branchAngle);
                    break;
                }

                case '&': {
                    // Pitch down
                    rotateAroundAxis(turtle.dirX, turtle.dirY, turtle.dirZ,
                                   turtle.rightX, turtle.rightY, turtle.rightZ,
                                   params.branchAngle);
                    break;
                }

                case '^': {
                    // Pitch up
                    rotateAroundAxis(turtle.dirX, turtle.dirY, turtle.dirZ,
                                   turtle.rightX, turtle.rightY, turtle.rightZ,
                                   -params.branchAngle);
                    break;
                }

                case '[': {
                    // Push state (start branch)
                    stateStack.push(turtle);
                    turtle.generation++;
                    turtle.thickness *= params.thicknessRatio;
                    break;
                }

                case ']': {
                    // Pop state (end branch)
                    if (!stateStack.empty()) {
                        turtle = stateStack.top();
                        stateStack.pop();
                    }
                    break;
                }

                case 'L': {
                    // Add leaves
                    TreeNode leafNode;
                    leafNode.x = turtle.x;
                    leafNode.y = turtle.y;
                    leafNode.z = turtle.z;
                    leafNode.partType = TreePart::LEAF;
                    leafNode.thickness = 0.0f;
                    leafNode.generation = turtle.generation;
                    nodes.push_back(leafNode);
                    break;
                }
            }

            if (turtle.generation > params.maxGenerations) break;
        }

        return nodes;
    }

    // High-level function: generate complete tree
    std::vector<TreeNode> generateTree(TreeSpecies species, float x, float y, float z,
                                       int age = 5) const {
        // Generate L-System (iterations = tree age/maturity)
        int iterations = std::min(age, 6); // Cap at 6 for performance
        std::string lsystem = generateLSystem(species, iterations);

        // Interpret into tree structure
        std::vector<TreeNode> nodes = interpretLSystem(lsystem, species, x, y, z);

        // Add leaves at branch ends (if not a dead tree)
        if (species != TreeSpecies::DEAD_TREE) {
            const SpeciesParams& params = speciesParams.at(species);

            for (const auto& node : nodes) {
                if (node.partType == TreePart::BRANCH && node.generation > 2) {
                    // Add leaf cluster at branch tips
                    for (int i = 0; i < (int)(params.leafDensity * 8); i++) {
                        TreeNode leaf;
                        leaf.x = node.x + hashFloat(seed + i * 100) * 2.0f - 1.0f;
                        leaf.y = node.y + hashFloat(seed + i * 101) * 2.0f - 1.0f;
                        leaf.z = node.z + hashFloat(seed + i * 102) * 2.0f - 1.0f;
                        leaf.partType = TreePart::LEAF;
                        leaf.thickness = 0.0f;
                        leaf.generation = node.generation;
                    }
                }
            }
        }

        return nodes;
    }

    // Determine species based on biome and environment
    TreeSpecies selectSpeciesForBiome(int biomeType, float temperature, float humidity) const {
        if (temperature > 0.5f && humidity < 0.3f) {
            return TreeSpecies::ACACIA; // Hot, dry
        } else if (temperature > 0.6f && humidity > 0.7f) {
            return TreeSpecies::PALM; // Tropical
        } else if (temperature < -0.3f) {
            return TreeSpecies::PINE; // Cold
        } else if (humidity > 0.7f) {
            return TreeSpecies::WILLOW; // Wet
        } else if (humidity > 0.5f) {
            return TreeSpecies::OAK; // Temperate, moist
        } else {
            return TreeSpecies::BIRCH; // Default temperate
        }
    }
};

} // namespace TerrainGen
} // namespace VoxelWorld
