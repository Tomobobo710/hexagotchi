#ifndef GOTCHI_STATS_HPP
#define GOTCHI_STATS_HPP

#include "raylib.h"
#include <string>
#include <array>
#include <map>

// Gotchi vital stats - 100 things to track
// Categories:
// - 10 Core Vital Stats (health, hunger, etc.)
// - 20 Secondary Stats (energy, hygiene, etc.)
// - 20 Emotional Stats (happiness, excitement, etc.)
// - 20 Physical Stats (strength, agility, etc.)
// - 10 Social Stats (popularity, friendship, etc.)
// - 10 Environmental Stats (familiarity, safety, etc.)
// - 10 Skill/Development Stats (learning, mastery, etc.)

// Core Vital Stats (10)
enum class CoreStat {
    HEALTH,           // Overall health status
    HUNGER,           // Hunger level (0=full, 100=starving)
    THIRST,           // Thirst level (0=hydrated, 100=dehydrated)
    ENERGY,           // Energy reserves
    SLEEP,            // Sleep need (0=rested, 100=exhausted)
    HYGIENE,          // Cleanliness
    VITALITY,         // General life force
    COMFORT,          // Physical comfort
    PAIN,             // Pain level
    STAMINA,          // Physical endurance
};

// Secondary Stats (30)
enum class SecondaryStat {
    // Core vital stats (10)
    WEIGHT,           // Current weight
    FITNESS,          // Physical fitness level
    GROWTH,           // Growth progress
    AGE,              // Age in game ticks
    BURDEN,           // Carried weight
    EXPOSURE,         // Environmental exposure
    DISEASE,          // Disease level
    IMMUNITY,         // Disease resistance
    FERTILITY,        // Reproductive capability
    STAGE,            // Life stage (baby, adult, elder)
    // Extended secondary stats (20)
    SIZE,             // Physical size
    SPEED,            // Movement speed
    JUMP,             // Jump capability
    CLIMB,            // Climbing ability
    SWIM,             // Swimming ability
    FLY,              // Flying ability
    CAMOUFLAGE,       // Hidden from view
    RESILIENCE,       // Recovery rate
    TOUGHNESS,        // Damage resistance
    FLEXIBILITY,      // Mobility
    // Convenience aliases (6 more to reach 30)
    FITALITY,         // Health (alias for vitality)
    FOOD_LEVEL,       // Hunger level
    HYDRATION,        // Thirst level
    ENERGY,           // Energy reserves
    SLEEP_DEBT,       // Sleep need
    CLEANLINESS,      // Hygiene level
};

// Emotional Stats (20)
enum class EmotionalStat {
    HAPPINESS,        // Overall joy
    EXCITEMENT,       // Arousal level
    SATISFACTION,     // Contentment
    BOREDOM,          // Stimulation need
    ANXIETY,          // Worry level
    FEAR,             // Fear response
    ANGER,            // Anger level
    SADNESS,          // Sadness level
    LOVE,             // Affection
    FRIEDNSHIP,       // Social connection
    TRUST,            // Trust level
    CONFIDENCE,       // Self-assurance
    HOPE,             // Optimism
    CURIOSITY,        // Desire to explore
    CREATIVITY,       // Creative drive
    FOCUS,            // Attention span
    PATIENCE,         // Tolerance
    GRATITUDE,        // Thankfulness
    PEACE,            // Inner calm
    JOY,              // Pure happiness
};

// Physical Stats (20)
enum class PhysicalStat {
    STRENGTH,         // Physical power
    AGILITY,          // Quickness
    ENDURANCE,        // Long-duration capability
    REFLEX,           // Reaction speed
    BALANCE,          // Stability
    COORDINATION,     // Motor skills
    DEXTERITY,        // Manual skill
    VISION,           // Sight acuity
    HEARING,          // Hearing acuity
    SMELL,            // Scent detection
    TASTE,            // Taste sensitivity
    TOUCH,            // Tactile sensitivity
    HEALTHY_FUR,      // Coat/skin condition
    HEALTHY_EYES,     // Eye health
    HEALTHY_TEETH,    // Dental health
    HEALTHY_CLAWS,    // Nail/claw condition
    POSTURE,          // Body alignment
    MOVEMENT_EFFICIENCY,// Energy use per distance
    THERMOREGULATION,// Temperature control
    METABOLISM,       // Food conversion rate
};

// Social Stats (10)
enum class SocialStat {
    POPULARITY,       // Liked by others
    FRIENDS,          // Number of friends
    ALLIANCES,        // Support network
    LEADERSHIP,       // Influence over others
    CHARISMA,         // charm ability
    COMMUNICATION,    // Expression ability
    EMPATHY,          // Understanding others
    COOPERATION,      // Teamwork ability
    RESPECT,          // Earned regard
    REPUTATION,       // Overall perception
};

// Environmental Stats (10)
enum class EnvironmentalStat {
    FAMILIARITY,      // Knowledge of area
    SAFETY,           // Perceived safety
    TERRITORY,        // Owned space
    HOME,             // Shelter quality
    RESOURCE_ACCESS,  // Available resources
    CLIMATE_COMPFORT,// Weather tolerance
    LIGHT_LEVEL,      // Light preference
    NOISE_LEVEL,      // Sound tolerance
    SOCIAL_SPACE,     // Crowd tolerance
    ISOLATION,        // Solitude preference
};

// Skill/Development Stats (10)
enum class SkillStat {
    LEARNING,         // Knowledge acquisition
    MEMORY,           // Recall ability
    PROBLEM_SOLVING,  // Logic capability
    INNOVATION,       // Creativity application
    TECHNIQUE,        // Method proficiency
    ADAPTATION,       // Change response
    SURVIVAL,         // Survival knowledge
    FORAGING,         // Food finding
    BUILDING,         // Structure creation
    COMBAT,           // Fighting ability
};

// Stat property structure
struct StatProperties {
    std::string name;
    std::string description;
    float defaultValue;
    float minRange;
    float maxRange;
    bool isDiscrete;  // Integer values only
    std::string unit; // Display unit
};

// GotchiStats class - manages 100 stats
class GotchiStats {
public:
    GotchiStats();

    // Core lifecycle
    void update(float deltaTime);
    void reset();

    // Stat accessors - using enums
    float getStat(SecondaryStat stat) const;
    float getStat(EmotionalStat stat) const;
    float getStat(PhysicalStat stat) const;
    float getStat(SocialStat stat) const;
    float getStat(EnvironmentalStat stat) const;
    float getStat(SkillStat stat) const;

    // Set stat with clamping
    void setStat(SecondaryStat stat, float value);
    void setStat(EmotionalStat stat, float value);
    void setStat(PhysicalStat stat, float value);
    void setStat(SocialStat stat, float value);
    void setStat(EnvironmentalStat stat, float value);
    void setStat(SkillStat stat, float value);

    // Modify stat by delta
    void addStat(SecondaryStat stat, float delta);
    void addStat(EmotionalStat stat, float delta);
    void addStat(PhysicalStat stat, float delta);
    void addStat(SocialStat stat, float delta);
    void addStat(EnvironmentalStat stat, float delta);
    void addStat(SkillStat stat, float delta);

    // Stat utilities
    float getNormalizedStat(SecondaryStat stat) const;  // 0-1 range
    float getNormalizedStat(EmotionalStat stat) const;
    float getNormalizedStat(PhysicalStat stat) const;
    float getNormalizedStat(SocialStat stat) const;
    float getNormalizedStat(EnvironmentalStat stat) const;
    float getNormalizedStat(SkillStat stat) const;

    bool isStatAtMax(SecondaryStat stat) const;
    bool isStatAtMin(SecondaryStat stat) const;

    // Get all stat names for debugging
    std::string getStatName(SecondaryStat stat) const;
    std::string getStatName(EmotionalStat stat) const;
    std::string getStatName(PhysicalStat stat) const;
    std::string getStatName(SocialStat stat) const;
    std::string getStatName(EnvironmentalStat stat) const;
    std::string getStatName(SkillStat stat) const;

    // Core vital stats (convenience methods)
    float getHealth() const { return getStat(SecondaryStat::FITALITY); }
    float getHunger() const { return getStat(SecondaryStat::FOOD_LEVEL); }
    float getThirst() const { return getStat(SecondaryStat::HYDRATION); }
    float getEnergy() const { return getStat(SecondaryStat::ENERGY); }
    float getSleep() const { return getStat(SecondaryStat::SLEEP_DEBT); }
    float getHappiness() const { return getStat(EmotionalStat::HAPPINESS); }
    float getHygiene() const { return getStat(SecondaryStat::CLEANLINESS); }

    // Debug
    void dumpAllStats() const;

private:
    // Storage for 100 stats as floats
    // Using separate arrays by category for organization
    std::array<float, 10> coreStats_;
    std::array<float, 20> secondaryStats_;
    std::array<float, 20> emotionalStats_;
    std::array<float, 20> physicalStats_;
    std::array<float, 10> socialStats_;
    std::array<float, 10> environmentalStats_;
    std::array<float, 10> skillStats_;

    // Stat properties lookup
    StatProperties getProperties(SecondaryStat stat) const;
    StatProperties getProperties(EmotionalStat stat) const;
    StatProperties getProperties(PhysicalStat stat) const;
    StatProperties getProperties(SocialStat stat) const;
    StatProperties getProperties(EnvironmentalStat stat) const;
    StatProperties getProperties(SkillStat stat) const;

    // Clamp value to stat's range
    float clampValue(float value, float minVal, float maxVal) const;

    // Get index from enum
    int getIndex(SecondaryStat stat) const;
    int getIndex(EmotionalStat stat) const;
    int getIndex(PhysicalStat stat) const;
    int getIndex(SocialStat stat) const;
    int getIndex(EnvironmentalStat stat) const;
    int getIndex(SkillStat stat) const;
};

#endif // GOTCHI_STATS_HPP
