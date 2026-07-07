#include "HexWorld.hpp"
#include <random>
#include <cmath>
#include <algorithm>

// Simple Perlin-like noise implementation
HexWorld::HexWorld(const HexWorldConfig& cfg)
    : config(cfg) {
}

HexWorld::~HexWorld() {
    for (auto tile : tiles) {
        delete tile;
    }
    tiles.clear();
}

// Interpolation helper for smooth noise
float HexWorld::interpolate(float a, float b, float weight) const {
    // Smoothstep interpolation for smoother transitions
    float smoothWeight = weight * weight * (3.0f - 2.0f * weight);
    return a + smoothWeight * (b - a);
}

// Generate pseudo-random number for gradient
float HexWorld::smoothNoise(int x, int y) const {
    // Simple hash-based random noise
    unsigned int n = x + y * 57;
    n = (n << 13) ^ n;
    float result = (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    return result; // Range: -1 to 1
}

// Perlin noise at given coordinates
float HexWorld::perlinNoise(float x, float y) const {
    // Get integer part and fractional part
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    // Fade curves for smooth interpolation
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);

    // Corners of the cell
    float A = smoothNoise(X, Y);
    float B = smoothNoise(X + 1, Y);
    float C = smoothNoise(X, Y + 1);
    float D = smoothNoise(X + 1, Y + 1);

    // Interpolate
    float XY = interpolate(A, B, u);
    float XY2 = interpolate(C, D, u);
    float value = interpolate(XY, XY2, v);

    return value;
}


BiomeType HexWorld::getBiomeForHeight(float height) const {
    // Map noise value (-1 to 1) to biomes
    // -1 to -0.6: Ocean
    // -0.6 to -0.4: Sand
    // -0.4 to 0.3: Grass
    // 0.3 to 0.5: Dirt
    // 0.5 to 0.7: Swamp
    // 0.7 to 0.85: Ice
    // 0.85 to 1.0: Lava

    float h = (height + 1.0f) / 2.0f; // Convert to 0-1 range

    if (h < 0.10f) return BiomeType::OCEAN;
    if (h < 0.20f) return BiomeType::SAND;
    if (h < 0.50f) return BiomeType::GRASS;
    if (h < 0.65f) return BiomeType::DIRT;
    if (h < 0.75f) return BiomeType::SWAMP;
    if (h < 0.85f) return BiomeType::ICE;
    return BiomeType::LAVA;
}

std::vector<TileType*> HexWorld::createBiomeTileTypes(BiomeType biome) const {
    std::vector<TileType*> types;

    switch (biome) {
        case BiomeType::OCEAN:
            for (int i = 0; i <= 3; i++) {
                types.push_back(new TileType("ocean_" + std::to_string(i), "ocean", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::SAND:
            for (int i = 0; i <= 3; i++) {
                types.push_back(new TileType("sand_" + std::to_string(i), "sand", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::GRASS:
            for (int i = 0; i <= 10; i++) {
                types.push_back(new TileType("grass_" + std::to_string(i), "grass", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::DIRT:
            for (int i = 0; i <= 7; i++) {
                types.push_back(new TileType("dirt_" + std::to_string(i), "dirt", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::SWAMP:
            for (int i = 0; i <= 4; i++) {
                types.push_back(new TileType("swamp_" + std::to_string(i), "swamp", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::ICE:
            for (int i = 0; i <= 10; i++) {
                types.push_back(new TileType("ice_" + std::to_string(i), "ice", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::LAVA:
            for (int i = 0; i <= 1; i++) {
                types.push_back(new TileType("lava_" + std::to_string(i), "lava", TileDepth::SHALLOW));
            }
            break;
        case BiomeType::SPECIAL:
            types.push_back(new TileType("special_000", "special", TileDepth::SHALLOW));
            types.push_back(new TileType("special_001", "special", TileDepth::SHALLOW));
            break;
    }

    return types;
}

void HexWorld::generate() {
    // Clear existing tiles
    for (auto tile : tiles) {
        delete tile;
    }
    tiles.clear();
    biomeGroups.clear();

    // Generate perlin noise terrain
    float persistence = 0.5f;  // How much each octave contributes
    int octaves = 4;           // Number of noise layers

    // Calculate a reasonable scale for the noise
    // Larger world needs lower frequency to get interesting patterns
    float baseFrequency = 1.0f / std::max(config.width, config.height) * 3.0f;

    // Create tile type pools for each biome
    std::map<BiomeType, std::vector<TileType*>> biomeTilePools;
    for (int b = 0; b < static_cast<int>(BiomeType::NUM_BIOMES); b++) {
        biomeTilePools[static_cast<BiomeType>(b)] = createBiomeTileTypes(static_cast<BiomeType>(b));
    }

    // Random number generator for selecting tiles
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1000);

    // Generate each hex tile
    for (int r = 0; r < config.height; r++) {
        for (int q = 0; q < config.width; q++) {
            // Convert hex coordinates to world position for noise sampling
            Vector2 pixelPos = HexCoords(q, r).toPixel(config.hexSize);

            // Sample perlin noise at this position with multiple octaves
            float x = pixelPos.x * baseFrequency;
            float y = pixelPos.y * baseFrequency;

            // Add some offset to make patterns less uniform
            float noiseValue = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float maxAmplitude = 0.0f;

            for (int i = 0; i < octaves; i++) {
                // Add offset to make each octave different
                float offsetX = i * 137.5f;
                float offsetY = i * 243.7f;

                noiseValue += perlinNoise((x + offsetX) * frequency, (y + offsetY) * frequency) * amplitude;
                maxAmplitude += amplitude;
                amplitude *= persistence;
                frequency *= 2.0f;
            }

            noiseValue /= maxAmplitude; // Normalize

            BiomeType biome = getBiomeForHeight(noiseValue);
            std::string biomeName = BiomeConfig::getConfig(biome).name;

            // Select a random tile type from the biome pool
            auto& pool = biomeTilePools[biome];
            if (!pool.empty()) {
                int idx = dist(gen) % pool.size();
                TileType* tileType = pool[idx];

                // Create the hex tile
                HexTile* tile = new HexTile(HexCoords(q, r), tileType, config.hexSize);
                tiles.push_back(tile);

                // Add to biome group for easy rendering
                biomeGroups[biomeName].push_back(tile);
            }
        }
    }

    // Load textures for all tiles
    for (auto tile : tiles) {
        tile->loadTexture();
    }
}

HexTile* HexWorld::getTileAt(int q, int r) const {
    if (q < 0 || q >= config.width || r < 0 || r >= config.height) {
        return nullptr;
    }

    // Convert hex coordinates to index
    int index = r * config.width + q;
    if (index >= 0 && index < static_cast<int>(tiles.size())) {
        return tiles[index];
    }
    return nullptr;
}

std::string HexWorld::getBiomeAt(float x, float y) const {
    // Convert pixel position to hex coordinates
    float inv32 = 1.0f / (config.hexSize * 3.0f / 2.0f);
    float q = x * inv32;
    float r = (y / (config.hexSize * std::sqrt(3.0f) / 2.0f) - q) / 2.0f;

    int hexQ = static_cast<int>(std::round(q));
    int hexR = static_cast<int>(std::round(r));

    HexTile* tile = getTileAt(hexQ, hexR);
    if (tile && tile->getTileType()) {
        return tile->getTileType()->getBiome();
    }

    return "unknown";
}
