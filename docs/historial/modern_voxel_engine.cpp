// MODERN VOXEL ENGINE - HIGH PERFORMANCE RENDERING SYSTEM
// This file contains the optimized voxel rendering architecture
// To be integrated into main.cpp

#include <vector>
#include <unordered_map>

// ============================================================================
// MODERN CHUNK MESH SYSTEM WITH VBO/VAO
// ============================================================================

struct Vertex {
    float x, y, z;          // Position
    float nx, ny, nz;       // Normal
    float u, v;             // Texture coordinates
    float r, g, b;          // Color

    Vertex(float x, float y, float z, float nx, float ny, float nz,
           float u, float v, float r, float g, float b)
        : x(x), y(y), z(z), nx(nx), ny(ny), nz(nz),
          u(u), v(v), r(r), g(g), b(b) {}
};

struct ChunkMesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    bool needsUpload;
    int faceCount;

    ChunkMesh() : VAO(0), VBO(0), EBO(0), needsUpload(true), faceCount(0) {}

    void clear() {
        vertices.clear();
        indices.clear();
        faceCount = 0;
        needsUpload = true;
    }

    void addQuad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        unsigned int startIdx = vertices.size();
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);

        // Two triangles per quad
        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 2);

        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 2);
        indices.push_back(startIdx + 3);

        faceCount++;
    }
};

// ============================================================================
// FACE CULLING - Only render visible faces
// ============================================================================

enum class Face {
    TOP = 0,
    BOTTOM,
    NORTH,
    SOUTH,
    EAST,
    WEST
};

inline bool shouldRenderFace(BlockType currentBlock, BlockType neighborBlock) {
    if (currentBlock == BLOCK_AIR) return false;
    if (neighborBlock == BLOCK_AIR) return true;

    // Transparent blocks
    if (neighborBlock == BLOCK_WATER || neighborBlock == BLOCK_GLASS) {
        return currentBlock != neighborBlock;
    }

    return false;
}

// ============================================================================
// GREEDY MESHING - Merge adjacent faces for fewer draw calls
// ============================================================================

struct GreedyQuad {
    int x, y, z;
    int width, height;
    Face face;
    BlockType blockType;
    uint8_t lightLevel;

    GreedyQuad(int x, int y, int z, int w, int h, Face f, BlockType type, uint8_t light)
        : x(x), y(y), z(z), width(w), height(h), face(f), blockType(type), lightLevel(light) {}
};

class GreedyMesher {
public:
    static std::vector<GreedyQuad> meshChunk(
        BlockType blocks[16][256][16],
        uint8_t lightLevels[16][256][16])
    {
        std::vector<GreedyQuad> quads;

        // Mesh each axis (X, Y, Z) and each direction (+/-)
        meshAxis(blocks, lightLevels, quads, 0);  // X axis
        meshAxis(blocks, lightLevels, quads, 1);  // Y axis
        meshAxis(blocks, lightLevels, quads, 2);  // Z axis

        return quads;
    }

private:
    static void meshAxis(
        BlockType blocks[16][256][16],
        uint8_t lightLevels[16][256][16],
        std::vector<GreedyQuad>& quads,
        int axis)
    {
        const int CHUNK_SIZE = 16;
        const int CHUNK_HEIGHT = 256;

        int dims[3] = {CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE};

        // For each direction (positive and negative)
        for (int dir = -1; dir <= 1; dir += 2) {
            // Sweep through each layer
            for (int d = 0; d < dims[axis]; d++) {
                bool visited[256][16] = {false};

                // For each position in the layer
                for (int i = 0; i < dims[(axis + 1) % 3]; i++) {
                    for (int j = 0; j < dims[(axis + 2) % 3]; j++) {
                        if (visited[i][j]) continue;

                        // Get block coordinates
                        int pos[3];
                        pos[axis] = d;
                        pos[(axis + 1) % 3] = i;
                        pos[(axis + 2) % 3] = j;

                        BlockType block = getBlock(blocks, pos[0], pos[1], pos[2]);
                        if (block == BLOCK_AIR) continue;

                        // Check if face should be rendered
                        int neighborPos[3] = {pos[0], pos[1], pos[2]};
                        neighborPos[axis] += dir;

                        BlockType neighbor = getBlock(blocks, neighborPos[0], neighborPos[1], neighborPos[2]);
                        if (!shouldRenderFace(block, neighbor)) continue;

                        uint8_t light = getLight(lightLevels, pos[0], pos[1], pos[2]);

                        // Greedy meshing: expand quad as much as possible
                        int width = 1, height = 1;

                        // Expand width
                        for (width = 1; j + width < dims[(axis + 2) % 3]; width++) {
                            pos[(axis + 2) % 3] = j + width;
                            BlockType nextBlock = getBlock(blocks, pos[0], pos[1], pos[2]);
                            uint8_t nextLight = getLight(lightLevels, pos[0], pos[1], pos[2]);

                            if (nextBlock != block || nextLight != light || visited[i][j + width])
                                break;

                            neighborPos[(axis + 2) % 3] = j + width;
                            BlockType nextNeighbor = getBlock(blocks, neighborPos[0], neighborPos[1], neighborPos[2]);
                            if (!shouldRenderFace(nextBlock, nextNeighbor))
                                break;
                        }

                        // Expand height
                        bool canExpand = true;
                        for (height = 1; i + height < dims[(axis + 1) % 3] && canExpand; height++) {
                            for (int w = 0; w < width; w++) {
                                pos[(axis + 1) % 3] = i + height;
                                pos[(axis + 2) % 3] = j + w;

                                BlockType nextBlock = getBlock(blocks, pos[0], pos[1], pos[2]);
                                uint8_t nextLight = getLight(lightLevels, pos[0], pos[1], pos[2]);

                                if (nextBlock != block || nextLight != light || visited[i + height][j + w]) {
                                    canExpand = false;
                                    break;
                                }

                                neighborPos[(axis + 1) % 3] = i + height;
                                neighborPos[(axis + 2) % 3] = j + w;
                                BlockType nextNeighbor = getBlock(blocks, neighborPos[0], neighborPos[1], neighborPos[2]);
                                if (!shouldRenderFace(nextBlock, nextNeighbor)) {
                                    canExpand = false;
                                    break;
                                }
                            }
                        }
                        if (!canExpand) height--;

                        // Mark as visited
                        for (int h = 0; h < height; h++) {
                            for (int w = 0; w < width; w++) {
                                visited[i + h][j + w] = true;
                            }
                        }

                        // Create quad
                        Face face = getFace(axis, dir);
                        quads.emplace_back(pos[0], pos[1], pos[2], width, height, face, block, light);
                    }
                }
            }
        }
    }

    static BlockType getBlock(BlockType blocks[16][256][16], int x, int y, int z) {
        if (x < 0 || x >= 16 || y < 0 || y >= 256 || z < 0 || z >= 16)
            return BLOCK_AIR;
        return blocks[x][y][z];
    }

    static uint8_t getLight(uint8_t lightLevels[16][256][16], int x, int y, int z) {
        if (x < 0 || x >= 16 || y < 0 || y >= 256 || z < 0 || z >= 16)
            return 18;
        return lightLevels[x][y][z];
    }

    static Face getFace(int axis, int dir) {
        if (axis == 0) return dir > 0 ? Face::EAST : Face::WEST;
        if (axis == 1) return dir > 0 ? Face::TOP : Face::BOTTOM;
        return dir > 0 ? Face::NORTH : Face::SOUTH;
    }
};

// ============================================================================
// FRUSTUM CULLING - Only render chunks in view
// ============================================================================

struct Frustum {
    float planes[6][4];  // 6 planes: left, right, bottom, top, near, far

    void extractFromMatrix(const float* viewProj) {
        // Extract frustum planes from view-projection matrix
        // Left plane
        planes[0][0] = viewProj[3] + viewProj[0];
        planes[0][1] = viewProj[7] + viewProj[4];
        planes[0][2] = viewProj[11] + viewProj[8];
        planes[0][3] = viewProj[15] + viewProj[12];

        // Right plane
        planes[1][0] = viewProj[3] - viewProj[0];
        planes[1][1] = viewProj[7] - viewProj[4];
        planes[1][2] = viewProj[11] - viewProj[8];
        planes[1][3] = viewProj[15] - viewProj[12];

        // Bottom plane
        planes[2][0] = viewProj[3] + viewProj[1];
        planes[2][1] = viewProj[7] + viewProj[5];
        planes[2][2] = viewProj[11] + viewProj[9];
        planes[2][3] = viewProj[15] + viewProj[13];

        // Top plane
        planes[3][0] = viewProj[3] - viewProj[1];
        planes[3][1] = viewProj[7] - viewProj[5];
        planes[3][2] = viewProj[11] - viewProj[9];
        planes[3][3] = viewProj[15] - viewProj[13];

        // Near plane
        planes[4][0] = viewProj[3] + viewProj[2];
        planes[4][1] = viewProj[7] + viewProj[6];
        planes[4][2] = viewProj[11] + viewProj[10];
        planes[4][3] = viewProj[15] + viewProj[14];

        // Far plane
        planes[5][0] = viewProj[3] - viewProj[2];
        planes[5][1] = viewProj[7] - viewProj[6];
        planes[5][2] = viewProj[11] - viewProj[10];
        planes[5][3] = viewProj[15] - viewProj[14];

        // Normalize planes
        for (int i = 0; i < 6; i++) {
            float length = sqrtf(planes[i][0] * planes[i][0] +
                                planes[i][1] * planes[i][1] +
                                planes[i][2] * planes[i][2]);
            planes[i][0] /= length;
            planes[i][1] /= length;
            planes[i][2] /= length;
            planes[i][3] /= length;
        }
    }

    bool isChunkVisible(float chunkX, float chunkY, float chunkZ, float chunkSize) {
        // AABB (Axis-Aligned Bounding Box) test
        float minX = chunkX;
        float minY = chunkY;
        float minZ = chunkZ;
        float maxX = chunkX + chunkSize;
        float maxY = chunkY + 256.0f;
        float maxZ = chunkZ + chunkSize;

        for (int i = 0; i < 6; i++) {
            float px = (planes[i][0] > 0) ? maxX : minX;
            float py = (planes[i][1] > 0) ? maxY : minY;
            float pz = (planes[i][2] > 0) ? maxZ : minZ;

            float dot = planes[i][0] * px + planes[i][1] * py +
                       planes[i][2] * pz + planes[i][3];

            if (dot < 0) return false;  // Outside frustum
        }

        return true;
    }
};

// ============================================================================
// USAGE EXAMPLE
// ============================================================================

/*
// In your chunk building code:

ChunkMesh mesh;

// 1. Get greedy-meshed quads
auto quads = GreedyMesher::meshChunk(chunk->blocks, chunk->lightLevels);

// 2. Build mesh from quads
for (const auto& quad : quads) {
    addQuadToMesh(mesh, quad);
}

// 3. Upload to GPU
uploadMeshToGPU(mesh);

// 4. In render loop with frustum culling:
Frustum frustum;
frustum.extractFromMatrix(viewProjMatrix);

for (auto& chunk : chunks) {
    if (frustum.isChunkVisible(chunk.x * 16, 0, chunk.z * 16, 16.0f)) {
        renderChunkMesh(chunk.mesh);
    }
}
*/
