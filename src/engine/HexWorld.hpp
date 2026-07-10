#ifndef HEX_WORLD_HPP
#define HEX_WORLD_HPP

#include "HexTile.hpp"
#include "Item.hpp"
#include <vector>
#include <map>
#include <string>

// Biome types based on available assets
enum class BiomeType {
    OCEAN,
    SAND,
    GRASS,
    DIRT,
    ICE,
    SWAMP,
    LAVA,
    SPECIAL,
    NUM_BIOMES
};

struct BiomeConfig {
    std::string name;
    Color color;  // For debug rendering

    static BiomeConfig getConfig(BiomeType type) {
        switch (type) {
            case BiomeType::OCEAN:   return {"ocean", {30, 70, 120, 255}};
            case BiomeType::SAND:    return {"sand", {240, 210, 160, 255}};
            case BiomeType::GRASS:   return {"grass", {80, 160, 80, 255}};
            case BiomeType::DIRT:    return {"dirt", {130, 90, 60, 255}};
            case BiomeType::ICE:     return {"ice", {200, 220, 255, 255}};
            case BiomeType::SWAMP:   return {"swamp", {60, 100, 40, 255}};
            case BiomeType::LAVA:    return {"lava", {200, 60, 20, 255}};
            case BiomeType::SPECIAL: return {"special", {180, 80, 180, 255}};
            default:                 return {"unknown", {128, 128, 128, 255}};
        }
    }
};

// World configuration
struct HexWorldConfig {
    int width;      // Number of hexes horizontally
    int height;     // Number of hexes vertically
    float hexSize;  // Size of each hex in pixels

    static HexWorldConfig defaultConfig() {
        return {32, 18, 64.0f};
    }
};

class HexWorld {
public:
    HexWorld(const HexWorldConfig& config = HexWorldConfig::defaultConfig());
    ~HexWorld();

    // Generate the world using perlin noise for biomes
    void generate();

    // Getters
    const std::vector<HexTile*>& getTiles() const { return tiles; }
    const std::map<std::string, std::vector<HexTile*>>& getBiomeGroups() const { return biomeGroups; }
    int getWidth() const { return config.width; }
    int getHeight() const { return config.height; }
    float getHexSize() const { return config.hexSize; }

    // Access tile at hex coordinates
    HexTile* getTileAt(int q, int r) const;

    // Get biome type at a position (returns string name)
    std::string getBiomeAt(float x, float y) const;

    // Item management
    void placeItem(const Item& item);
    Item* getItemAt(int q, int r);
    const std::vector<Item>& getItems() const { return items; }
    void removeConsumedItems();

    // Find items near a hex position
    std::vector<Item*> getItemsNear(int q, int r, int radius = 1);

private:
    HexWorldConfig config;
    std::vector<HexTile*> tiles;
    std::map<std::string, std::vector<HexTile*>> biomeGroups;
    std::vector<Item> items;

    // Perlin noise generation
    float perlinNoise(float x, float y) const;
    float smoothNoise(int x, int y) const;
    float interpolate(float a, float b, float weight) const;

    // Biome selection based on noise value
    BiomeType getBiomeForHeight(float height) const;

    // Initialize tile types for each biome
    std::vector<TileType*> createBiomeTileTypes(BiomeType biome) const;
};

#endif // HEX_WORLD_HPP
