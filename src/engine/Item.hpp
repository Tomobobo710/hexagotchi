#ifndef ITEM_HPP
#define ITEM_HPP

#include <string>
#include "GotchiStats.hpp"

// Item types that map to specific needs
enum class ItemType {
    FOOD,        // Satisfies hunger (FOOD_LEVEL stat)
    WATER,       // Satisfies thirst (HYDRATION stat)
    MEDICINE,    // Heals health (FITALITY stat)
    TOY,         // Boosts happiness/excitement (HAPPINESS, EXCITEMENT stats)
    CLEANING,    // Improves hygiene (CLEANLINESS stat)
    SLEEP,       // Reduces sleep debt (SLEEP_DEBT stat)
    ENERGY,      // Direct energy boost (ENERGY stat)
    HAPPINESS    // Direct happiness boost (HAPPINESS stat)
};

// Represents an item placed on a hex tile
struct Item {
    ItemType type;
    float value;      // How much the item satisfies the need (0-100 scale)
    int hexQ;         // Hex coordinate where item is placed
    int hexR;         // Hex coordinate where item is placed
    bool consumed;    // Whether the item has been consumed

    Item(ItemType t, float v, int q, int r)
        : type(t), value(v), hexQ(q), hexR(r), consumed(false) {}

    // Get the stat name that this item satisfies (for debugging)
    std::string getStatName() const {
        switch (type) {
            case ItemType::FOOD:       return "FOOD_LEVEL";
            case ItemType::WATER:      return "HYDRATION";
            case ItemType::MEDICINE:   return "FITALITY";
            case ItemType::TOY:        return "HAPPINESS";
            case ItemType::CLEANING:   return "CLEANLINESS";
            case ItemType::SLEEP:      return "SLEEP_DEBT";
            case ItemType::ENERGY:     return "ENERGY";
            case ItemType::HAPPINESS:  return "HAPPINESS";
            default:                   return "UNKNOWN";
        }
    }

    // Get whether this stat is improved by increasing or decreasing the value
    // (e.g., hunger is 0=full, 100=starving, so food decreases it)
    bool decreasesStat() const {
        switch (type) {
            case ItemType::FOOD:       return true;   // Hunger: 0=full, 100=starving
            case ItemType::WATER:      return true;   // Thirst: 0=hydrated, 100=dehydrated
            case ItemType::MEDICINE:   return false;  // Health: 0=dead, 100=perfect
            case ItemType::TOY:        return false;  // Happiness: 0=sad, 100=ecstatic
            case ItemType::CLEANING:   return false;  // Cleanliness: 0=dirty, 100=clean
            case ItemType::SLEEP:      return true;   // Sleep debt: 0=rested, 100=exhausted
            case ItemType::ENERGY:     return false;  // Energy: 0=tired, 100=energetic
            case ItemType::HAPPINESS:  return false;  // Happiness: 0=sad, 100=ecstatic
            default:                   return false;
        }
    }

    // Apply this item's effect to the gotchi's stats
    void applyEffect(GotchiStats& stats) const {
        switch (type) {
            case ItemType::FOOD:
                stats.addStat(SecondaryStat::FOOD_LEVEL, -value);
                break;
            case ItemType::WATER:
                stats.addStat(SecondaryStat::HYDRATION, -value);
                break;
            case ItemType::MEDICINE:
                stats.addStat(SecondaryStat::FITALITY, value);
                break;
            case ItemType::TOY:
            case ItemType::HAPPINESS:
                stats.addStat(EmotionalStat::HAPPINESS, value);
                stats.addStat(EmotionalStat::EXCITEMENT, value * 0.5f);
                break;
            case ItemType::CLEANING:
                stats.addStat(SecondaryStat::CLEANLINESS, value);
                break;
            case ItemType::SLEEP:
                stats.addStat(SecondaryStat::SLEEP_DEBT, -value);
                break;
            case ItemType::ENERGY:
                stats.addStat(SecondaryStat::ENERGY, value);
                break;
            default:
                break;
        }

        // Clamp stats to valid range
        stats.setStat(SecondaryStat::FOOD_LEVEL, stats.getStat(SecondaryStat::FOOD_LEVEL));
        stats.setStat(SecondaryStat::HYDRATION, stats.getStat(SecondaryStat::HYDRATION));
        stats.setStat(SecondaryStat::FITALITY, stats.getStat(SecondaryStat::FITALITY));
        stats.setStat(EmotionalStat::HAPPINESS, stats.getStat(EmotionalStat::HAPPINESS));
        stats.setStat(EmotionalStat::EXCITEMENT, stats.getStat(EmotionalStat::EXCITEMENT));
        stats.setStat(SecondaryStat::CLEANLINESS, stats.getStat(SecondaryStat::CLEANLINESS));
        stats.setStat(SecondaryStat::SLEEP_DEBT, stats.getStat(SecondaryStat::SLEEP_DEBT));
        stats.setStat(SecondaryStat::ENERGY, stats.getStat(SecondaryStat::ENERGY));
    }
};

#endif // ITEM_HPP
