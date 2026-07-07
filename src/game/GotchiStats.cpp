#include "GotchiStats.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

// Stat property defaults
// Core vital stats
const float DEFAULT_HEALTH = 100.0f;
const float DEFAULT_HUNGER = 0.0f;
const float DEFAULT_THIRST = 0.0f;
const float DEFAULT_ENERGY = 100.0f;
const float DEFAULT_SLEEP = 0.0f;
const float DEFAULT_HYGIENE = 100.0f;
const float DEFAULT_VITALITY = 100.0f;
const float DEFAULT_COMFORT = 50.0f;
const float DEFAULT_PAIN = 0.0f;
const float DEFAULT_STAMINA = 100.0f;

// GotchiStats constructor
GotchiStats::GotchiStats() {
    reset();
}

// Reset all stats to default values
void GotchiStats::reset() {
    // Core stats (10)
    coreStats_[0] = DEFAULT_HEALTH;   // HEALTH
    coreStats_[1] = DEFAULT_HUNGER;   // HUNGER
    coreStats_[2] = DEFAULT_THIRST;   // THIRST
    coreStats_[3] = DEFAULT_ENERGY;   // ENERGY
    coreStats_[4] = DEFAULT_SLEEP;    // SLEEP
    coreStats_[5] = DEFAULT_HYGIENE;  // HYGIENE
    coreStats_[6] = DEFAULT_VITALITY; // VITALITY
    coreStats_[7] = DEFAULT_COMFORT;  // COMFORT
    coreStats_[8] = DEFAULT_PAIN;     // PAIN
    coreStats_[9] = DEFAULT_STAMINA;  // STAMINA

    // Secondary stats (20)
    // WEIGHT, FITNESS, GROWTH, AGE, BURDEN, EXPOSURE, DISEASE, IMMUNITY, FERTILITY, STAGE
    // SIZE, SPEED, JUMP, CLIMB, SWIM, FLY, CAMOUFLAGE, RESILIENCE, TOUGHNESS, FLEXIBILITY
    secondaryStats_.fill(50.0f);
    secondaryStats_[0] = 50.0f;   // WEIGHT
    secondaryStats_[1] = 50.0f;   // FITNESS
    secondaryStats_[2] = 0.0f;    // GROWTH
    secondaryStats_[3] = 0.0f;    // AGE
    secondaryStats_[4] = 0.0f;    // BURDEN
    secondaryStats_[5] = 0.0f;    // EXPOSURE
    secondaryStats_[6] = 0.0f;    // DISEASE
    secondaryStats_[7] = 100.0f;  // IMMUNITY
    secondaryStats_[8] = 0.0f;    // FERTILITY
    secondaryStats_[9] = 0.0f;    // STAGE
    secondaryStats_[10] = 50.0f;  // SIZE
    secondaryStats_[11] = 50.0f;  // SPEED
    secondaryStats_[12] = 50.0f;  // JUMP
    secondaryStats_[13] = 50.0f;  // CLIMB
    secondaryStats_[14] = 50.0f;  // SWIM
    secondaryStats_[15] = 0.0f;   // FLY
    secondaryStats_[16] = 0.0f;   // CAMOUFLAGE
    secondaryStats_[17] = 50.0f;  // RESILIENCE
    secondaryStats_[18] = 50.0f;  // TOUGHNESS
    secondaryStats_[19] = 50.0f;  // FLEXIBILITY

    // Add convenience aliases
    secondaryStats_[20] = 100.0f; // FITALITY (alias for HEALTH)
    secondaryStats_[21] = 0.0f;   // FOOD_LEVEL (alias for HUNGER)
    secondaryStats_[22] = 0.0f;   // HYDRATION (alias for THIRST)
    secondaryStats_[23] = 100.0f; // ENERGY (alias for ENERGY)
    secondaryStats_[24] = 0.0f;   // SLEEP_DEBT (alias for SLEEP)
    secondaryStats_[25] = 100.0f; // CLEANLINESS (alias for HYGIENE)

    // Emotional stats (20)
    // HAPPINESS, EXCITEMENT, SATISFACTION, BOREDOM, ANXIETY, FEAR, ANGER, SADNESS
    // LOVE, FRIEDNSHIP, TRUST, CONFIDENCE, HOPE, CURIOSITY, CREATIVITY, FOCUS
    // PATIENCE, GRATITUDE, PEACE, JOY
    emotionalStats_.fill(50.0f);
    emotionalStats_[0] = 80.0f;   // HAPPINESS
    emotionalStats_[1] = 30.0f;   // EXCITEMENT
    emotionalStats_[2] = 60.0f;   // SATISFACTION
    emotionalStats_[3] = 0.0f;    // BOREDOM
    emotionalStats_[4] = 0.0f;    // ANXIETY
    emotionalStats_[5] = 0.0f;    // FEAR
    emotionalStats_[6] = 0.0f;    // ANGER
    emotionalStats_[7] = 0.0f;    // SADNESS
    emotionalStats_[8] = 50.0f;   // LOVE
    emotionalStats_[9] = 0.0f;    // FRIENDSHIP
    emotionalStats_[10] = 70.0f;  // TRUST
    emotionalStats_[11] = 60.0f;  // CONFIDENCE
    emotionalStats_[12] = 80.0f;  // HOPE
    emotionalStats_[13] = 40.0f;  // CURIOSITY
    emotionalStats_[14] = 30.0f;  // CREATIVITY
    emotionalStats_[15] = 60.0f;  // FOCUS
    emotionalStats_[16] = 70.0f;  // PATIENCE
    emotionalStats_[17] = 80.0f;  // GRATITUDE
    emotionalStats_[18] = 80.0f;  // PEACE
    emotionalStats_[19] = 70.0f;  // JOY

    // Physical stats (20)
    // STRENGTH, AGILITY, ENDURANCE, REFLEX, BALANCE, COORDINATION, DEXTERITY
    // VISION, HEARING, SMELL, TASTE, TOUCH, HEALTHY_FUR, HEALTHY_EYES
    // HEALTHY_TEETH, HEALTHY_CLAWS, POSTURE, MOVEMENT_EFFICIENCY, THERMOREGULATION
    // METABOLISM
    physicalStats_.fill(50.0f);
    physicalStats_[0] = 50.0f;    // STRENGTH
    physicalStats_[1] = 50.0f;    // AGILITY
    physicalStats_[2] = 50.0f;    // ENDURANCE
    physicalStats_[3] = 50.0f;    // REFLEX
    physicalStats_[4] = 50.0f;    // BALANCE
    physicalStats_[5] = 50.0f;    // COORDINATION
    physicalStats_[6] = 50.0f;    // DEXTERITY
    physicalStats_[7] = 100.0f;   // VISION
    physicalStats_[8] = 100.0f;   // HEARING
    physicalStats_[9] = 80.0f;    // SMELL
    physicalStats_[10] = 80.0f;   // TASTE
    physicalStats_[11] = 80.0f;   // TOUCH
    physicalStats_[12] = 100.0f;  // HEALTHY_FUR
    physicalStats_[13] = 100.0f;  // HEALTHY_EYES
    physicalStats_[14] = 100.0f;  // HEALTHY_TEETH
    physicalStats_[15] = 80.0f;   // HEALTHY_CLAWS
    physicalStats_[16] = 80.0f;   // POSTURE
    physicalStats_[17] = 80.0f;   // MOVEMENT_EFFICIENCY
    physicalStats_[18] = 80.0f;   // THERMOREGULATION
    physicalStats_[19] = 80.0f;   // METABOLISM

    // Social stats (10)
    // POPULARITY, FRIENDS, ALLIANCES, LEADERSHIP, CHARISMA, COMMUNICATION
    // EMPATHY, COOPERATION, RESPECT, REPUTATION
    socialStats_.fill(50.0f);
    socialStats_[0] = 50.0f;      // POPULARITY
    socialStats_[1] = 0.0f;       // FRIENDS (count)
    socialStats_[2] = 0.0f;       // ALLIANCES
    socialStats_[3] = 50.0f;      // LEADERSHIP
    socialStats_[4] = 50.0f;      // CHARISMA
    socialStats_[5] = 60.0f;      // COMMUNICATION
    socialStats_[6] = 70.0f;      // EMPATHY
    socialStats_[7] = 60.0f;      // COOPERATION
    socialStats_[8] = 50.0f;      // RESPECT
    socialStats_[9] = 50.0f;      // REPUTATION

    // Environmental stats (10)
    // FAMILIARITY, SAFETY, TERRITORY, HOME, RESOURCE_ACCESS, CLIMATE_COMPFORT
    // LIGHT_LEVEL, NOISE_LEVEL, SOCIAL_SPACE, ISOLATION
    environmentalStats_.fill(50.0f);
    environmentalStats_[0] = 50.0f;  // FAMILIARITY
    environmentalStats_[1] = 70.0f;  // SAFETY
    environmentalStats_[2] = 0.0f;   // TERRITORY
    environmentalStats_[3] = 60.0f;  // HOME
    environmentalStats_[4] = 80.0f;  // RESOURCE_ACCESS
    environmentalStats_[5] = 70.0f;  // CLIMATE_COMPFORT
    environmentalStats_[6] = 50.0f;  // LIGHT_LEVEL
    environmentalStats_[7] = 50.0f;  // NOISE_LEVEL
    environmentalStats_[8] = 60.0f;  // SOCIAL_SPACE
    environmentalStats_[9] = 50.0f;  // ISOLATION

    // Skill stats (10)
    // LEARNING, MEMORY, PROBLEM_SOLVING, INNOVATION, TECHNIQUE, ADAPTATION
    // SURVIVAL, FORAGING, BUILDING, COMBAT
    skillStats_.fill(30.0f);
    skillStats_[0] = 40.0f;       // LEARNING
    skillStats_[1] = 40.0f;       // MEMORY
    skillStats_[2] = 30.0f;       // PROBLEM_SOLVING
    skillStats_[3] = 30.0f;       // INNOVATION
    skillStats_[4] = 30.0f;       // TECHNIQUE
    skillStats_[5] = 40.0f;       // ADAPTATION
    skillStats_[6] = 50.0f;       // SURVIVAL
    skillStats_[7] = 40.0f;       // FORAGING
    skillStats_[8] = 30.0f;       // BUILDING
    skillStats_[9] = 20.0f;       // COMBAT
}

// Clamp value to range
float GotchiStats::clampValue(float value, float minVal, float maxVal) const {
    return std::max(minVal, std::min(maxVal, value));
}

// Get index from enum
int GotchiStats::getIndex(SecondaryStat stat) const {
    return static_cast<int>(stat);
}

int GotchiStats::getIndex(EmotionalStat stat) const {
    return static_cast<int>(stat);
}

int GotchiStats::getIndex(PhysicalStat stat) const {
    return static_cast<int>(stat);
}

int GotchiStats::getIndex(SocialStat stat) const {
    return static_cast<int>(stat);
}

int GotchiStats::getIndex(EnvironmentalStat stat) const {
    return static_cast<int>(stat);
}

int GotchiStats::getIndex(SkillStat stat) const {
    return static_cast<int>(stat);
}

// Get stat value
float GotchiStats::getStat(SecondaryStat stat) const {
    return secondaryStats_[getIndex(stat)];
}

float GotchiStats::getStat(EmotionalStat stat) const {
    return emotionalStats_[getIndex(stat)];
}

float GotchiStats::getStat(PhysicalStat stat) const {
    return physicalStats_[getIndex(stat)];
}

float GotchiStats::getStat(SocialStat stat) const {
    return socialStats_[getIndex(stat)];
}

float GotchiStats::getStat(EnvironmentalStat stat) const {
    return environmentalStats_[getIndex(stat)];
}

float GotchiStats::getStat(SkillStat stat) const {
    return skillStats_[getIndex(stat)];
}

// Set stat value with clamping
void GotchiStats::setStat(SecondaryStat stat, float value) {
    auto props = getProperties(stat);
    secondaryStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

void GotchiStats::setStat(EmotionalStat stat, float value) {
    auto props = getProperties(stat);
    emotionalStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

void GotchiStats::setStat(PhysicalStat stat, float value) {
    auto props = getProperties(stat);
    physicalStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

void GotchiStats::setStat(SocialStat stat, float value) {
    auto props = getProperties(stat);
    socialStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

void GotchiStats::setStat(EnvironmentalStat stat, float value) {
    auto props = getProperties(stat);
    environmentalStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

void GotchiStats::setStat(SkillStat stat, float value) {
    auto props = getProperties(stat);
    skillStats_[getIndex(stat)] = clampValue(value, props.minRange, props.maxRange);
}

// Modify stat by delta
void GotchiStats::addStat(SecondaryStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

void GotchiStats::addStat(EmotionalStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

void GotchiStats::addStat(PhysicalStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

void GotchiStats::addStat(SocialStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

void GotchiStats::addStat(EnvironmentalStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

void GotchiStats::addStat(SkillStat stat, float delta) {
    float newValue = getStat(stat) + delta;
    setStat(stat, newValue);
}

// Get normalized stat (0-1 range)
float GotchiStats::getNormalizedStat(SecondaryStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

float GotchiStats::getNormalizedStat(EmotionalStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

float GotchiStats::getNormalizedStat(PhysicalStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

float GotchiStats::getNormalizedStat(SocialStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

float GotchiStats::getNormalizedStat(EnvironmentalStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

float GotchiStats::getNormalizedStat(SkillStat stat) const {
    auto props = getProperties(stat);
    return (getStat(stat) - props.minRange) / (props.maxRange - props.minRange);
}

// Check if stat at max/min
bool GotchiStats::isStatAtMax(SecondaryStat stat) const {
    auto props = getProperties(stat);
    return getStat(stat) >= props.maxRange;
}

bool GotchiStats::isStatAtMin(SecondaryStat stat) const {
    auto props = getProperties(stat);
    return getStat(stat) <= props.minRange;
}

// Get stat name
std::string GotchiStats::getStatName(SecondaryStat stat) const {
    switch (stat) {
        case SecondaryStat::WEIGHT: return "Weight";
        case SecondaryStat::FITNESS: return "Fitness";
        case SecondaryStat::GROWTH: return "Growth";
        case SecondaryStat::AGE: return "Age";
        case SecondaryStat::BURDEN: return "Burden";
        case SecondaryStat::EXPOSURE: return "Exposure";
        case SecondaryStat::DISEASE: return "Disease";
        case SecondaryStat::IMMUNITY: return "Immunity";
        case SecondaryStat::FERTILITY: return "Fertility";
        case SecondaryStat::STAGE: return "Stage";
        case SecondaryStat::SIZE: return "Size";
        case SecondaryStat::SPEED: return "Speed";
        case SecondaryStat::JUMP: return "Jump";
        case SecondaryStat::CLIMB: return "Climb";
        case SecondaryStat::SWIM: return "Swim";
        case SecondaryStat::FLY: return "Fly";
        case SecondaryStat::CAMOUFLAGE: return "Camouflage";
        case SecondaryStat::RESILIENCE: return "Resilience";
        case SecondaryStat::TOUGHNESS: return "Toughness";
        case SecondaryStat::FLEXIBILITY: return "Flexibility";
        case SecondaryStat::FITALITY: return "Health";
        case SecondaryStat::FOOD_LEVEL: return "Hunger";
        case SecondaryStat::HYDRATION: return "Thirst";
        case SecondaryStat::ENERGY: return "Energy";
        case SecondaryStat::SLEEP_DEBT: return "Sleep";
        case SecondaryStat::CLEANLINESS: return "Hygiene";
        default: return "Unknown";
    }
}

std::string GotchiStats::getStatName(EmotionalStat stat) const {
    switch (stat) {
        case EmotionalStat::HAPPINESS: return "Happiness";
        case EmotionalStat::EXCITEMENT: return "Excitement";
        case EmotionalStat::SATISFACTION: return "Satisfaction";
        case EmotionalStat::BOREDOM: return "Boredom";
        case EmotionalStat::ANXIETY: return "Anxiety";
        case EmotionalStat::FEAR: return "Fear";
        case EmotionalStat::ANGER: return "Anger";
        case EmotionalStat::SADNESS: return "Sadness";
        case EmotionalStat::LOVE: return "Love";
        case EmotionalStat::FRIEDNSHIP: return "Friendship";
        case EmotionalStat::TRUST: return "Trust";
        case EmotionalStat::CONFIDENCE: return "Confidence";
        case EmotionalStat::HOPE: return "Hope";
        case EmotionalStat::CURIOSITY: return "Curiosity";
        case EmotionalStat::CREATIVITY: return "Creativity";
        case EmotionalStat::FOCUS: return "Focus";
        case EmotionalStat::PATIENCE: return "Patience";
        case EmotionalStat::GRATITUDE: return "Gratitude";
        case EmotionalStat::PEACE: return "Peace";
        case EmotionalStat::JOY: return "Joy";
        default: return "Unknown";
    }
}

std::string GotchiStats::getStatName(PhysicalStat stat) const {
    switch (stat) {
        case PhysicalStat::STRENGTH: return "Strength";
        case PhysicalStat::AGILITY: return "Agility";
        case PhysicalStat::ENDURANCE: return "Endurance";
        case PhysicalStat::REFLEX: return "Reflex";
        case PhysicalStat::BALANCE: return "Balance";
        case PhysicalStat::COORDINATION: return "Coordination";
        case PhysicalStat::DEXTERITY: return "Dexterity";
        case PhysicalStat::VISION: return "Vision";
        case PhysicalStat::HEARING: return "Hearing";
        case PhysicalStat::SMELL: return "Smell";
        case PhysicalStat::TASTE: return "Taste";
        case PhysicalStat::TOUCH: return "Touch";
        case PhysicalStat::HEALTHY_FUR: return "Fur Health";
        case PhysicalStat::HEALTHY_EYES: return "Eye Health";
        case PhysicalStat::HEALTHY_TEETH: return "Teeth Health";
        case PhysicalStat::HEALTHY_CLAWS: return "Claw Health";
        case PhysicalStat::POSTURE: return "Posture";
        case PhysicalStat::MOVEMENT_EFFICIENCY: return "Movement Efficiency";
        case PhysicalStat::THERMOREGULATION: return "Thermoregulation";
        case PhysicalStat::METABOLISM: return "Metabolism";
        default: return "Unknown";
    }
}

std::string GotchiStats::getStatName(SocialStat stat) const {
    switch (stat) {
        case SocialStat::POPULARITY: return "Popularity";
        case SocialStat::FRIENDS: return "Friends";
        case SocialStat::ALLIANCES: return "Alliances";
        case SocialStat::LEADERSHIP: return "Leadership";
        case SocialStat::CHARISMA: return "Charisma";
        case SocialStat::COMMUNICATION: return "Communication";
        case SocialStat::EMPATHY: return "Empathy";
        case SocialStat::COOPERATION: return "Cooperation";
        case SocialStat::RESPECT: return "Respect";
        case SocialStat::REPUTATION: return "Reputation";
        default: return "Unknown";
    }
}

std::string GotchiStats::getStatName(EnvironmentalStat stat) const {
    switch (stat) {
        case EnvironmentalStat::FAMILIARITY: return "Familiarity";
        case EnvironmentalStat::SAFETY: return "Safety";
        case EnvironmentalStat::TERRITORY: return "Territory";
        case EnvironmentalStat::HOME: return "Home";
        case EnvironmentalStat::RESOURCE_ACCESS: return "Resource Access";
        case EnvironmentalStat::CLIMATE_COMPFORT: return "Climate Comfort";
        case EnvironmentalStat::LIGHT_LEVEL: return "Light Level";
        case EnvironmentalStat::NOISE_LEVEL: return "Noise Level";
        case EnvironmentalStat::SOCIAL_SPACE: return "Social Space";
        case EnvironmentalStat::ISOLATION: return "Isolation";
        default: return "Unknown";
    }
}

std::string GotchiStats::getStatName(SkillStat stat) const {
    switch (stat) {
        case SkillStat::LEARNING: return "Learning";
        case SkillStat::MEMORY: return "Memory";
        case SkillStat::PROBLEM_SOLVING: return "Problem Solving";
        case SkillStat::INNOVATION: return "Innovation";
        case SkillStat::TECHNIQUE: return "Technique";
        case SkillStat::ADAPTATION: return "Adaptation";
        case SkillStat::SURVIVAL: return "Survival";
        case SkillStat::FORAGING: return "Foraging";
        case SkillStat::BUILDING: return "Building";
        case SkillStat::COMBAT: return "Combat";
        default: return "Unknown";
    }
}

// Stat properties
StatProperties GotchiStats::getProperties(SecondaryStat stat) const {
    switch (stat) {
        case SecondaryStat::WEIGHT: return {"Weight", "Current body weight", 50.0f, 0.0f, 100.0f, true, "kg"};
        case SecondaryStat::FITNESS: return {"Fitness", "Physical fitness level", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::GROWTH: return {"Growth", "Growth progress", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::AGE: return {"Age", "Age in game ticks", 0.0f, 0.0f, 10000.0f, true, "ticks"};
        case SecondaryStat::BURDEN: return {"Burden", "Carried weight", 0.0f, 0.0f, 50.0f, true, "kg"};
        case SecondaryStat::EXPOSURE: return {"Exposure", "Environmental exposure", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::DISEASE: return {"Disease", "Disease level", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::IMMUNITY: return {"Immunity", "Disease resistance", 100.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::FERTILITY: return {"Fertility", "Reproductive capability", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::STAGE: return {"Stage", "Life stage", 0.0f, 0.0f, 3.0f, true, "stage"};
        case SecondaryStat::SIZE: return {"Size", "Physical size", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::SPEED: return {"Speed", "Movement speed", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::JUMP: return {"Jump", "Jump capability", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::CLIMB: return {"Climb", "Climbing ability", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::SWIM: return {"Swim", "Swimming ability", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::FLY: return {"Fly", "Flying ability", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::CAMOUFLAGE: return {"Camouflage", "Hidden from view", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::RESILIENCE: return {"Resilience", "Recovery rate", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::TOUGHNESS: return {"Toughness", "Damage resistance", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::FLEXIBILITY: return {"Flexibility", "Mobility", 50.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::FITALITY: return {"Health", "Overall health", 100.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::FOOD_LEVEL: return {"Hunger", "Hunger level (0=full, 100=starving)", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::HYDRATION: return {"Thirst", "Thirst level (0=hydrated, 100=dehydrated)", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::ENERGY: return {"Energy", "Energy reserves", 100.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::SLEEP_DEBT: return {"Sleep", "Sleep need (0=rested, 100=exhausted)", 0.0f, 0.0f, 100.0f, true, "%"};
        case SecondaryStat::CLEANLINESS: return {"Hygiene", "Cleanliness", 100.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown stat", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

StatProperties GotchiStats::getProperties(EmotionalStat stat) const {
    switch (stat) {
        case EmotionalStat::HAPPINESS: return {"Happiness", "Overall joy", 80.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::EXCITEMENT: return {"Excitement", "Arousal level", 30.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::SATISFACTION: return {"Satisfaction", "Contentment", 60.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::BOREDOM: return {"Boredom", "Stimulation need", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::ANXIETY: return {"Anxiety", "Worry level", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::FEAR: return {"Fear", "Fear response", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::ANGER: return {"Anger", "Anger level", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::SADNESS: return {"Sadness", "Sadness level", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::LOVE: return {"Love", "Affection", 50.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::FRIEDNSHIP: return {"Friendship", "Social connection", 0.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::TRUST: return {"Trust", "Trust level", 70.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::CONFIDENCE: return {"Confidence", "Self-assurance", 60.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::HOPE: return {"Hope", "Optimism", 80.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::CURIOSITY: return {"Curiosity", "Desire to explore", 40.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::CREATIVITY: return {"Creativity", "Creative drive", 30.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::FOCUS: return {"Focus", "Attention span", 60.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::PATIENCE: return {"Patience", "Tolerance", 70.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::GRATITUDE: return {"Gratitude", "Thankfulness", 80.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::PEACE: return {"Peace", "Inner calm", 80.0f, 0.0f, 100.0f, true, "%"};
        case EmotionalStat::JOY: return {"Joy", "Pure happiness", 70.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown emotion", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

StatProperties GotchiStats::getProperties(PhysicalStat stat) const {
    switch (stat) {
        case PhysicalStat::STRENGTH: return {"Strength", "Physical power", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::AGILITY: return {"Agility", "Quickness", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::ENDURANCE: return {"Endurance", "Long-duration capability", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::REFLEX: return {"Reflex", "Reaction speed", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::BALANCE: return {"Balance", "Stability", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::COORDINATION: return {"Coordination", "Motor skills", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::DEXTERITY: return {"Dexterity", "Manual skill", 50.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::VISION: return {"Vision", "Sight acuity", 100.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::HEARING: return {"Hearing", "Hearing acuity", 100.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::SMELL: return {"Smell", "Scent detection", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::TASTE: return {"Taste", "Taste sensitivity", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::TOUCH: return {"Touch", "Tactile sensitivity", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::HEALTHY_FUR: return {"Fur Health", "Coat/skin condition", 100.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::HEALTHY_EYES: return {"Eye Health", "Eye health", 100.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::HEALTHY_TEETH: return {"Teeth Health", "Dental health", 100.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::HEALTHY_CLAWS: return {"Claw Health", "Nail/claw condition", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::POSTURE: return {"Posture", "Body alignment", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::MOVEMENT_EFFICIENCY: return {"Movement Efficiency", "Energy use per distance", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::THERMOREGULATION: return {"Thermoregulation", "Temperature control", 80.0f, 0.0f, 100.0f, true, "%"};
        case PhysicalStat::METABOLISM: return {"Metabolism", "Food conversion rate", 80.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown physical stat", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

StatProperties GotchiStats::getProperties(SocialStat stat) const {
    switch (stat) {
        case SocialStat::POPULARITY: return {"Popularity", "Liked by others", 50.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::FRIENDS: return {"Friends", "Number of friends", 0.0f, 0.0f, 100.0f, true, "#"};
        case SocialStat::ALLIANCES: return {"Alliances", "Support network", 0.0f, 0.0f, 100.0f, true, "#"};
        case SocialStat::LEADERSHIP: return {"Leadership", "Influence over others", 50.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::CHARISMA: return {"Charisma", "Charm ability", 50.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::COMMUNICATION: return {"Communication", "Expression ability", 60.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::EMPATHY: return {"Empathy", "Understanding others", 70.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::COOPERATION: return {"Cooperation", "Teamwork ability", 60.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::RESPECT: return {"Respect", "Earned regard", 50.0f, 0.0f, 100.0f, true, "%"};
        case SocialStat::REPUTATION: return {"Reputation", "Overall perception", 50.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown social stat", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

StatProperties GotchiStats::getProperties(EnvironmentalStat stat) const {
    switch (stat) {
        case EnvironmentalStat::FAMILIARITY: return {"Familiarity", "Knowledge of area", 50.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::SAFETY: return {"Safety", "Perceived safety", 70.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::TERRITORY: return {"Territory", "Owned space", 0.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::HOME: return {"Home", "Shelter quality", 60.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::RESOURCE_ACCESS: return {"Resource Access", "Available resources", 80.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::CLIMATE_COMPFORT: return {"Climate Comfort", "Weather tolerance", 70.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::LIGHT_LEVEL: return {"Light Level", "Light preference", 50.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::NOISE_LEVEL: return {"Noise Level", "Sound tolerance", 50.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::SOCIAL_SPACE: return {"Social Space", "Crowd tolerance", 60.0f, 0.0f, 100.0f, true, "%"};
        case EnvironmentalStat::ISOLATION: return {"Isolation", "Solitude preference", 50.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown environmental stat", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

StatProperties GotchiStats::getProperties(SkillStat stat) const {
    switch (stat) {
        case SkillStat::LEARNING: return {"Learning", "Knowledge acquisition", 40.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::MEMORY: return {"Memory", "Recall ability", 40.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::PROBLEM_SOLVING: return {"Problem Solving", "Logic capability", 30.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::INNOVATION: return {"Innovation", "Creativity application", 30.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::TECHNIQUE: return {"Technique", "Method proficiency", 30.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::ADAPTATION: return {"Adaptation", "Change response", 40.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::SURVIVAL: return {"Survival", "Survival knowledge", 50.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::FORAGING: return {"Foraging", "Food finding", 40.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::BUILDING: return {"Building", "Structure creation", 30.0f, 0.0f, 100.0f, true, "%"};
        case SkillStat::COMBAT: return {"Combat", "Fighting ability", 20.0f, 0.0f, 100.0f, true, "%"};
        default: return {"Unknown", "Unknown skill stat", 50.0f, 0.0f, 100.0f, true, "%"};
    }
}

// Debug: dump all stats
void GotchiStats::dumpAllStats() const {
    std::cout << "=== Gotchi Stats Dump ===\n";

    std::cout << "\n--- Secondary Stats ---\n";
    for (int i = 0; i < 20; i++) {
        // Cast to enum to get name
        auto stat = static_cast<SecondaryStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n--- Emotional Stats ---\n";
    for (int i = 0; i < 20; i++) {
        auto stat = static_cast<EmotionalStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n--- Physical Stats ---\n";
    for (int i = 0; i < 20; i++) {
        auto stat = static_cast<PhysicalStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n--- Social Stats ---\n";
    for (int i = 0; i < 10; i++) {
        auto stat = static_cast<SocialStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n--- Environmental Stats ---\n";
    for (int i = 0; i < 10; i++) {
        auto stat = static_cast<EnvironmentalStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n--- Skill Stats ---\n";
    for (int i = 0; i < 10; i++) {
        auto stat = static_cast<SkillStat>(i);
        std::cout << std::setw(25) << getStatName(stat)
                  << ": " << std::setw(6) << getStat(stat)
                  << " (" << std::setw(5) << std::fixed << std::setprecision(1)
                  << getNormalizedStat(stat) * 100 << "%)\n";
    }

    std::cout << "\n=========================\n";
}
