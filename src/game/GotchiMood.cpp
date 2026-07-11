#include "GotchiMood.hpp"
#include "GotchiStats.hpp"
#include <algorithm>
#include <iostream>

GotchiMood::GotchiMood()
    : currentMood(GotchiMoodType::MOOD_00_HAPPY),
      overlayTimer(0.0f) {
    // Initialize mood cache with default properties
    moodCache.clear();
}

GotchiMoodType GotchiMood::getCurrentMood() const {
    return currentMood;
}

void GotchiMood::setCurrentMood(GotchiMoodType mood) {
    currentMood = mood;
}

void GotchiMood::updateMood(float deltaTime, const GotchiStats& stats) {
    // Calculate mood score based on stats
    // Higher score = more dominant mood

    // Hunger impacts: MOOD_04_HUNGRY, MOOD_03_BORED
    float hungerScore = stats.getNormalizedStat(SecondaryStat::FOOD_LEVEL) * 30.0f;

    // Thirst impacts: MOOD_05_THIRSTY
    float thirstScore = stats.getNormalizedStat(SecondaryStat::HYDRATION) * 25.0f;

    // Sleep impacts: MOOD_06_SLEEPY
    float sleepScore = stats.getNormalizedStat(SecondaryStat::SLEEP_DEBT) * 30.0f;

    // Energy impacts: MOOD_01_EXCITED (high), MOOD_06_SLEEPY (low)
    float energy = stats.getNormalizedStat(SecondaryStat::ENERGY);
    float energyScore = (energy > 0.7f) ? (energy - 0.7f) * 30.0f : 0.0f;
    float lowEnergyScore = (1.0f - energy) * 20.0f;

    // Hygiene impacts: MOOD_07_SICK, MOOD_03_BORED
    float hygiene = stats.getNormalizedStat(SecondaryStat::CLEANLINESS);
    float hygieneScore = (1.0f - hygiene) * 20.0f;

    // Happiness impacts: MOOD_00_HAPPY, MOOD_10_JOYFUL, MOOD_08_SAD
    float happiness = stats.getNormalizedStat(EmotionalStat::HAPPINESS);
    float happyScore = happiness * 40.0f;
    float sadScore = (1.0f - happiness) * 25.0f;

    // Excitement/pleasure: MOOD_01_EXCITED, MOOD_31_PLAYFUL
    float excitement = stats.getNormalizedStat(EmotionalStat::EXCITEMENT);
    float excitedScore = excitement * 30.0f;

    // Boredom: MOOD_03_BORED
    float boredom = 1.0f - stats.getNormalizedStat(EmotionalStat::SATISFACTION);
    float boredomScore = boredom * 30.0f;

    // Social needs: MOOD_29_AFFECTIONATE, MOOD_25_LOVED
    float social = stats.getNormalizedStat(SocialStat::FRIENDS);
    float lonelyScore = (1.0f - social) * 15.0f;

    // Health impacts: MOOD_07_SICK
    float health = stats.getNormalizedStat(SecondaryStat::FITALITY);
    float sickScore = (1.0f - health) * 35.0f;

    // Find dominant mood
    float bestScore = 0.0f;
    GotchiMoodType bestMood = GotchiMoodType::MOOD_00_HAPPY;

    // Primary moods (priority 1)
    struct MoodScore { GotchiMoodType mood; float score; };
    MoodScore moodScores[] = {
        {GotchiMoodType::MOOD_00_HAPPY, happyScore},
        {GotchiMoodType::MOOD_01_EXCITED, excitedScore},
        {GotchiMoodType::MOOD_02_SATISFIED, stats.getNormalizedStat(EmotionalStat::SATISFACTION) * 35.0f},
        {GotchiMoodType::MOOD_03_BORED, boredomScore},
        {GotchiMoodType::MOOD_04_HUNGRY, hungerScore},
        {GotchiMoodType::MOOD_05_THIRSTY, thirstScore},
        {GotchiMoodType::MOOD_06_SLEEPY, std::max(sleepScore, lowEnergyScore)},
        {GotchiMoodType::MOOD_07_SICK, sickScore},
        {GotchiMoodType::MOOD_08_SAD, sadScore},
        {GotchiMoodType::MOOD_09_ANGRY, std::max(hungerScore * 0.5f, sickScore * 0.5f)},
    };

    for (const auto& ms : moodScores) {
        if (ms.score > bestScore) {
            bestScore = ms.score;
            bestMood = ms.mood;
        }
    }

    // Apply dominant mood
    if (bestScore > 10.0f) {
        currentMood = bestMood;
    } else {
        // Default to satisfied if no strong needs
        currentMood = GotchiMoodType::MOOD_02_SATISFIED;
    }
}

std::string GotchiMood::getMoodName() const {
    switch (currentMood) {
        case GotchiMoodType::MOOD_00_HAPPY: return "Happy";
        case GotchiMoodType::MOOD_01_EXCITED: return "Excited";
        case GotchiMoodType::MOOD_02_SATISFIED: return "Satisfied";
        case GotchiMoodType::MOOD_03_BORED: return "Bored";
        case GotchiMoodType::MOOD_04_HUNGRY: return "Hungry";
        case GotchiMoodType::MOOD_05_THIRSTY: return "Thirsty";
        case GotchiMoodType::MOOD_06_SLEEPY: return "Sleepy";
        case GotchiMoodType::MOOD_07_SICK: return "Sick";
        case GotchiMoodType::MOOD_08_SAD: return "Sad";
        case GotchiMoodType::MOOD_09_ANGRY: return "Angry";
        case GotchiMoodType::MOOD_10_JOYFUL: return "Joyful";
        case GotchiMoodType::MOOD_11_CONTENT: return "Content";
        case GotchiMoodType::MOOD_12_RELIEVED: return "Relieved";
        case GotchiMoodType::MOOD_13_CALM: return "Calm";
        case GotchiMoodType::MOOD_14_PEACEFUL: return "Peaceful";
        case GotchiMoodType::MOOD_15_SERENE: return "Serene";
        case GotchiMoodType::MOOD_16_OPTIMISTIC: return "Optimistic";
        case GotchiMoodType::MOOD_17_HOPEFUL: return "Hopeful";
        case GotchiMoodType::MOOD_18_CURIOUS: return "Curious";
        case GotchiMoodType::MOOD_19_INTERESTED: return "Interested";
        case GotchiMoodType::MOOD_20_FOCUSED: return "Focused";
        case GotchiMoodType::MOOD_21_DETERMINED: return "Determined";
        case GotchiMoodType::MOOD_22_PROUD: return "Proud";
        case GotchiMoodType::MOOD_23_SUCCESSFUL: return "Successful";
        case GotchiMoodType::MOOD_24_GRATEFUL: return "Grateful";
        case GotchiMoodType::MOOD_25_LOVED: return "Loved";
        case GotchiMoodType::MOOD_26_SAFE: return "Safe";
        case GotchiMoodType::MOOD_27_SECURE: return "Secure";
        case GotchiMoodType::MOOD_28_TRUSTING: return "Trusting";
        case GotchiMoodType::MOOD_29_AFFECTIONATE: return "Affectionate";
        case GotchiMoodType::MOOD_30_LOVING: return "Loving";
        case GotchiMoodType::MOOD_31_PLAYFUL: return "Playful";
        case GotchiMoodType::MOOD_32_EXPLORING: return "Exploring";
        case GotchiMoodType::MOOD_33_ADVENTUROUS: return "Adventurous";
        case GotchiMoodType::MOOD_34_CREATING: return "Creating";
        case GotchiMoodType::MOOD_35_LEARNING: return "Learning";
        case GotchiMoodType::MOOD_36_ACHIEVING: return "Achieving";
        case GotchiMoodType::MOOD_37_COMPETITIVE: return "Competitive";
        case GotchiMoodType::MOOD_38_COLLABORATIVE: return "Collaborative";
        case GotchiMoodType::MOOD_39_TEACHING: return "Teaching";
        case GotchiMoodType::MOOD_40_HELPING: return "Helping";
        case GotchiMoodType::MOOD_41_GIVING: return "Giving";
        case GotchiMoodType::MOOD_42_GENEROUS: return "Generous";
        case GotchiMoodType::MOOD_43_ABUNDANT: return "Abundant";
        case GotchiMoodType::MOOD_44_PROSPEROUS: return "Prosperous";
        case GotchiMoodType::MOOD_45_HEALTHY: return "Healthy";
        case GotchiMoodType::MOOD_46_FIT: return "Fit";
        case GotchiMoodType::MOOD_47_ENERGETIC: return "Energetic";
        case GotchiMoodType::MOOD_48_VIBRANT: return "Vibrant";
        case GotchiMoodType::MOOD_49_ALIVE: return "Alive";
        case GotchiMoodType::MOOD_50_SPIRITED: return "Spirited";
        case GotchiMoodType::MOOD_51_CHARMING: return "Charming";
        case GotchiMoodType::MOOD_52_BEAUTIFUL: return "Beautiful";
        case GotchiMoodType::MOOD_53_GLORIOUS: return "Glorious";
        case GotchiMoodType::MOOD_54_MAGNIFICENT: return "Magnificent";
        case GotchiMoodType::MOOD_55_WONDERFUL: return "Wonderful";
        case GotchiMoodType::MOOD_56_AMAZING: return "Amazing";
        case GotchiMoodType::MOOD_57_INCREDIBLE: return "Incredible";
        case GotchiMoodType::MOOD_58_EXTRAORDINARY: return "Extraordinary";
        case GotchiMoodType::MOOD_59_SUPERB: return "Superb";
        case GotchiMoodType::MOOD_60_FANTASTIC: return "Fantastic";
        case GotchiMoodType::MOOD_61_AWESOME: return "Awesome";
        case GotchiMoodType::MOOD_62_MIRACULOUS: return "Miraculous";
        case GotchiMoodType::MOOD_63_DIVINE: return "Divine";
        case GotchiMoodType::MOOD_64_ETHEREAL: return "Ethereal";
        case GotchiMoodType::MOOD_65_SPIRITUAL: return "Spiritual";
        case GotchiMoodType::MOOD_66_MINDFUL: return "Mindful";
        case GotchiMoodType::MOOD_67_WISE: return "Wise";
        case GotchiMoodType::MOOD_68_KNOWLEDGEABLE: return "Knowledgeable";
        case GotchiMoodType::MOOD_69_INTELLIGENT: return "Intelligent";
        case GotchiMoodType::MOOD_70_CLEVER: return "Clever";
        case GotchiMoodType::MOOD_71_SKILLFUL: return "Skillful";
        case GotchiMoodType::MOOD_72_EXPERT: return "Expert";
        case GotchiMoodType::MOOD_73_MASTER: return "Master";
        case GotchiMoodType::MOOD_74_GRANDMASTER: return "Grandmaster";
        case GotchiMoodType::MOOD_75_LEGENDARY: return "Legendary";
        case GotchiMoodType::MOOD_76_MYTHICAL: return "Mythical";
        case GotchiMoodType::MOOD_77_EPIC: return "Epic";
        case GotchiMoodType::MOOD_78_HEROIC: return "Heroic";
        case GotchiMoodType::MOOD_79_VALIANT: return "Valiant";
        case GotchiMoodType::MOOD_80_NOBLE: return "Noble";
        case GotchiMoodType::MOOD_81_VIRTUOUS: return "Virtuous";
        case GotchiMoodType::MOOD_82_RIGHTEOUS: return "Righteous";
        case GotchiMoodType::MOOD_83_FAIR: return "Fair";
        case GotchiMoodType::MOOD_84_JUST: return "Just";
        case GotchiMoodType::MOOD_85_BALANCED: return "Balanced";
        case GotchiMoodType::MOOD_86_HARMONIOUS: return "Harmonious";
        case GotchiMoodType::MOOD_87_SYNCED: return "Synced";
        case GotchiMoodType::MOOD_88_ALIGNED: return "Aligned";
        case GotchiMoodType::MOOD_89_CONNECTED: return "Connected";
        case GotchiMoodType::MOOD_90_UNITED: return "United";
        case GotchiMoodType::MOOD_91_ONE: return "One";
        case GotchiMoodType::MOOD_92_WHOLE: return "Whole";
        case GotchiMoodType::MOOD_93_COMPLETE: return "Complete";
        case GotchiMoodType::MOOD_94_FULL: return "Full";
        case GotchiMoodType::MOOD_95_BLOOMING: return "Blooming";
        case GotchiMoodType::MOOD_96_THRIVING: return "Thriving";
        case GotchiMoodType::MOOD_97_GROWING: return "Growing";
        case GotchiMoodType::MOOD_98_EVOLVING: return "Evolving";
        case GotchiMoodType::MOOD_99_TRANSFORMING: return "Transforming";
        default: return "Unknown";
    }
}

Color GotchiMood::getMoodTint() const {
    switch (currentMood) {
        case GotchiMoodType::MOOD_00_HAPPY: return {255, 255, 0, 50};       // Yellow
        case GotchiMoodType::MOOD_01_EXCITED: return {255, 100, 0, 50};     // Orange
        case GotchiMoodType::MOOD_02_SATISFIED: return {200, 255, 200, 50}; // Light green
        case GotchiMoodType::MOOD_03_BORED: return {150, 150, 150, 50};     // Grey
        case GotchiMoodType::MOOD_04_HUNGRY: return {255, 200, 50, 50};     // Amber
        case GotchiMoodType::MOOD_05_THIRSTY: return {0, 150, 255, 50};     // Blue
        case GotchiMoodType::MOOD_06_SLEEPY: return {100, 50, 150, 50};     // Purple
        case GotchiMoodType::MOOD_07_SICK: return {150, 255, 150, 50};      // sickly green
        case GotchiMoodType::MOOD_08_SAD: return {50, 100, 200, 50};        // Blue
        case GotchiMoodType::MOOD_09_ANGRY: return {255, 50, 50, 50};       // Red
        case GotchiMoodType::MOOD_10_JOYFUL: return {255, 255, 150, 50};    // Bright yellow
        case GotchiMoodType::MOOD_13_CALM: return {150, 200, 200, 50};      // Light blue
        case GotchiMoodType::MOOD_14_PEACEFUL: return {200, 255, 200, 50};  // Soft green
        case GotchiMoodType::MOOD_15_SERENE: return {220, 240, 255, 50};    // Pale blue
        case GotchiMoodType::MOOD_25_LOVED: return {255, 150, 200, 50};     // Pink
        case GotchiMoodType::MOOD_29_AFFECTIONATE: return {255, 180, 200, 50}; // Soft pink
        case GotchiMoodType::MOOD_31_PLAYFUL: return {255, 200, 100, 50};   // Coral
        case GotchiMoodType::MOOD_45_HEALTHY: return {100, 255, 100, 50};   // Healthy green
        case GotchiMoodType::MOOD_53_GLORIOUS: return {255, 215, 0, 50};    // Gold
        case GotchiMoodType::MOOD_54_MAGNIFICENT: return {255, 160, 0, 50}; // Gold-orange
        case GotchiMoodType::MOOD_63_DIVINE: return {255, 255, 200, 50};    // Divine white
        default: return {255, 255, 255, 30};  // Default white
    }
}

MoodProperties GotchiMood::getMoodProperties(GotchiMoodType mood) const {
    // Return mood properties based on type
    switch (mood) {
        case GotchiMoodType::MOOD_00_HAPPY:
            return {"Happy", "Content and playful", {255, 255, 0, 50}, 1.0f, false, "none", 0.0f, 0.5f};
        case GotchiMoodType::MOOD_04_HUNGRY:
            return {"Hungry", "Needs food", {255, 200, 50, 50}, 2.0f, true, "feed", 0.1f, -0.2f};
        case GotchiMoodType::MOOD_05_THIRSTY:
            return {"Thirsty", "Needs drink", {0, 150, 255, 50}, 2.0f, true, "drink", 0.05f, -0.1f};
        case GotchiMoodType::MOOD_06_SLEEPY:
            return {"Sleepy", "Needs rest", {100, 50, 150, 50}, 2.0f, true, "sleep", -0.1f, -0.5f};
        case GotchiMoodType::MOOD_07_SICK:
            return {"Sick", "Needs care", {150, 255, 150, 50}, 3.0f, true, "heal", -0.2f, -0.3f};
        case GotchiMoodType::MOOD_08_SAD:
            return {"Sad", "Needs comfort", {50, 100, 200, 50}, 1.5f, true, "comfort", 0.0f, 0.3f};
        case GotchiMoodType::MOOD_09_ANGRY:
            return {"Angry", "Needs calming", {255, 50, 50, 50}, 2.0f, true, "calm", 0.1f, -0.4f};
        case GotchiMoodType::MOOD_03_BORED:
            return {"Bored", "Needs interaction", {150, 150, 150, 50}, 1.5f, true, "play", 0.05f, 0.2f};
        default:
            return {getMoodName(), "Standard mood", {255, 255, 255, 30}, 0.5f, false, "none", 0.0f, 0.0f};
    }
}

void GotchiMood::addMoodOverlay(GotchiMoodType mood, float duration) {
    // Add a temporary mood overlay
    activeOverlays.push_back({mood, duration});
}

void GotchiMood::clearMoodOverlays() {
    activeOverlays.clear();
}

void GotchiMood::processMoodOverlays(float deltaTime) {
    // Decay overlay durations
    for (auto& overlay : activeOverlays) {
        overlay.second -= deltaTime;
    }

    // Remove expired overlays
    activeOverlays.erase(
        std::remove_if(activeOverlays.begin(), activeOverlays.end(),
            [](const std::pair<GotchiMoodType, float>& o) { return o.second <= 0; }),
        activeOverlays.end()
    );

    // If overlays exist, temporarily boost their mood
    if (!activeOverlays.empty()) {
        // Find most recent overlay
        auto& topOverlay = activeOverlays.back();
        if (topOverlay.second > 0) {
            overlayTimer = topOverlay.second;
        }
    }
}

// Full update - called every tick (combines updateMood and processMoodOverlays)
void GotchiMood::update(float deltaTime, const GotchiStats& stats) {
    updateMood(deltaTime, stats);
    processMoodOverlays(deltaTime);
}

void GotchiMood::setDebugMoodIndex(int index) {
    if (index >= 0 && index <= 99) {
        // Convert index to mood enum value
        switch (index) {
            case 0: currentMood = GotchiMoodType::MOOD_00_HAPPY; break;
            case 1: currentMood = GotchiMoodType::MOOD_01_EXCITED; break;
            case 2: currentMood = GotchiMoodType::MOOD_02_SATISFIED; break;
            case 3: currentMood = GotchiMoodType::MOOD_03_BORED; break;
            case 4: currentMood = GotchiMoodType::MOOD_04_HUNGRY; break;
            case 5: currentMood = GotchiMoodType::MOOD_05_THIRSTY; break;
            case 6: currentMood = GotchiMoodType::MOOD_06_SLEEPY; break;
            case 7: currentMood = GotchiMoodType::MOOD_07_SICK; break;
            case 8: currentMood = GotchiMoodType::MOOD_08_SAD; break;
            case 9: currentMood = GotchiMoodType::MOOD_09_ANGRY; break;
            case 10: currentMood = GotchiMoodType::MOOD_10_JOYFUL; break;
            case 11: currentMood = GotchiMoodType::MOOD_11_CONTENT; break;
            case 12: currentMood = GotchiMoodType::MOOD_12_RELIEVED; break;
            case 13: currentMood = GotchiMoodType::MOOD_13_CALM; break;
            case 14: currentMood = GotchiMoodType::MOOD_14_PEACEFUL; break;
            case 15: currentMood = GotchiMoodType::MOOD_15_SERENE; break;
            case 16: currentMood = GotchiMoodType::MOOD_16_OPTIMISTIC; break;
            case 17: currentMood = GotchiMoodType::MOOD_17_HOPEFUL; break;
            case 18: currentMood = GotchiMoodType::MOOD_18_CURIOUS; break;
            case 19: currentMood = GotchiMoodType::MOOD_19_INTERESTED; break;
            case 20: currentMood = GotchiMoodType::MOOD_20_FOCUSED; break;
            case 21: currentMood = GotchiMoodType::MOOD_21_DETERMINED; break;
            case 22: currentMood = GotchiMoodType::MOOD_22_PROUD; break;
            case 23: currentMood = GotchiMoodType::MOOD_23_SUCCESSFUL; break;
            case 24: currentMood = GotchiMoodType::MOOD_24_GRATEFUL; break;
            case 25: currentMood = GotchiMoodType::MOOD_25_LOVED; break;
            case 26: currentMood = GotchiMoodType::MOOD_26_SAFE; break;
            case 27: currentMood = GotchiMoodType::MOOD_27_SECURE; break;
            case 28: currentMood = GotchiMoodType::MOOD_28_TRUSTING; break;
            case 29: currentMood = GotchiMoodType::MOOD_29_AFFECTIONATE; break;
            case 30: currentMood = GotchiMoodType::MOOD_30_LOVING; break;
            case 31: currentMood = GotchiMoodType::MOOD_31_PLAYFUL; break;
            case 32: currentMood = GotchiMoodType::MOOD_32_EXPLORING; break;
            case 33: currentMood = GotchiMoodType::MOOD_33_ADVENTUROUS; break;
            case 34: currentMood = GotchiMoodType::MOOD_34_CREATING; break;
            case 35: currentMood = GotchiMoodType::MOOD_35_LEARNING; break;
            case 36: currentMood = GotchiMoodType::MOOD_36_ACHIEVING; break;
            case 37: currentMood = GotchiMoodType::MOOD_37_COMPETITIVE; break;
            case 38: currentMood = GotchiMoodType::MOOD_38_COLLABORATIVE; break;
            case 39: currentMood = GotchiMoodType::MOOD_39_TEACHING; break;
            case 40: currentMood = GotchiMoodType::MOOD_40_HELPING; break;
            case 41: currentMood = GotchiMoodType::MOOD_41_GIVING; break;
            case 42: currentMood = GotchiMoodType::MOOD_42_GENEROUS; break;
            case 43: currentMood = GotchiMoodType::MOOD_43_ABUNDANT; break;
            case 44: currentMood = GotchiMoodType::MOOD_44_PROSPEROUS; break;
            case 45: currentMood = GotchiMoodType::MOOD_45_HEALTHY; break;
            case 46: currentMood = GotchiMoodType::MOOD_46_FIT; break;
            case 47: currentMood = GotchiMoodType::MOOD_47_ENERGETIC; break;
            case 48: currentMood = GotchiMoodType::MOOD_48_VIBRANT; break;
            case 49: currentMood = GotchiMoodType::MOOD_49_ALIVE; break;
            case 50: currentMood = GotchiMoodType::MOOD_50_SPIRITED; break;
            case 51: currentMood = GotchiMoodType::MOOD_51_CHARMING; break;
            case 52: currentMood = GotchiMoodType::MOOD_52_BEAUTIFUL; break;
            case 53: currentMood = GotchiMoodType::MOOD_53_GLORIOUS; break;
            case 54: currentMood = GotchiMoodType::MOOD_54_MAGNIFICENT; break;
            case 55: currentMood = GotchiMoodType::MOOD_55_WONDERFUL; break;
            case 56: currentMood = GotchiMoodType::MOOD_56_AMAZING; break;
            case 57: currentMood = GotchiMoodType::MOOD_57_INCREDIBLE; break;
            case 58: currentMood = GotchiMoodType::MOOD_58_EXTRAORDINARY; break;
            case 59: currentMood = GotchiMoodType::MOOD_59_SUPERB; break;
            case 60: currentMood = GotchiMoodType::MOOD_60_FANTASTIC; break;
            case 61: currentMood = GotchiMoodType::MOOD_61_AWESOME; break;
            case 62: currentMood = GotchiMoodType::MOOD_62_MIRACULOUS; break;
            case 63: currentMood = GotchiMoodType::MOOD_63_DIVINE; break;
            case 64: currentMood = GotchiMoodType::MOOD_64_ETHEREAL; break;
            case 65: currentMood = GotchiMoodType::MOOD_65_SPIRITUAL; break;
            case 66: currentMood = GotchiMoodType::MOOD_66_MINDFUL; break;
            case 67: currentMood = GotchiMoodType::MOOD_67_WISE; break;
            case 68: currentMood = GotchiMoodType::MOOD_68_KNOWLEDGEABLE; break;
            case 69: currentMood = GotchiMoodType::MOOD_69_INTELLIGENT; break;
            case 70: currentMood = GotchiMoodType::MOOD_70_CLEVER; break;
            case 71: currentMood = GotchiMoodType::MOOD_71_SKILLFUL; break;
            case 72: currentMood = GotchiMoodType::MOOD_72_EXPERT; break;
            case 73: currentMood = GotchiMoodType::MOOD_73_MASTER; break;
            case 74: currentMood = GotchiMoodType::MOOD_74_GRANDMASTER; break;
            case 75: currentMood = GotchiMoodType::MOOD_75_LEGENDARY; break;
            case 76: currentMood = GotchiMoodType::MOOD_76_MYTHICAL; break;
            case 77: currentMood = GotchiMoodType::MOOD_77_EPIC; break;
            case 78: currentMood = GotchiMoodType::MOOD_78_HEROIC; break;
            case 79: currentMood = GotchiMoodType::MOOD_79_VALIANT; break;
            case 80: currentMood = GotchiMoodType::MOOD_80_NOBLE; break;
            case 81: currentMood = GotchiMoodType::MOOD_81_VIRTUOUS; break;
            case 82: currentMood = GotchiMoodType::MOOD_82_RIGHTEOUS; break;
            case 83: currentMood = GotchiMoodType::MOOD_83_FAIR; break;
            case 84: currentMood = GotchiMoodType::MOOD_84_JUST; break;
            case 85: currentMood = GotchiMoodType::MOOD_85_BALANCED; break;
            case 86: currentMood = GotchiMoodType::MOOD_86_HARMONIOUS; break;
            case 87: currentMood = GotchiMoodType::MOOD_87_SYNCED; break;
            case 88: currentMood = GotchiMoodType::MOOD_88_ALIGNED; break;
            case 89: currentMood = GotchiMoodType::MOOD_89_CONNECTED; break;
            case 90: currentMood = GotchiMoodType::MOOD_90_UNITED; break;
            case 91: currentMood = GotchiMoodType::MOOD_91_ONE; break;
            case 92: currentMood = GotchiMoodType::MOOD_92_WHOLE; break;
            case 93: currentMood = GotchiMoodType::MOOD_93_COMPLETE; break;
            case 94: currentMood = GotchiMoodType::MOOD_94_FULL; break;
            case 95: currentMood = GotchiMoodType::MOOD_95_BLOOMING; break;
            case 96: currentMood = GotchiMoodType::MOOD_96_THRIVING; break;
            case 97: currentMood = GotchiMoodType::MOOD_97_GROWING; break;
            case 98: currentMood = GotchiMoodType::MOOD_98_EVOLVING; break;
            case 99: currentMood = GotchiMoodType::MOOD_99_TRANSFORMING; break;
        }
    }
}

int GotchiMood::getDebugMoodIndex() const {
    switch (currentMood) {
        case GotchiMoodType::MOOD_00_HAPPY: return 0;
        case GotchiMoodType::MOOD_01_EXCITED: return 1;
        case GotchiMoodType::MOOD_02_SATISFIED: return 2;
        case GotchiMoodType::MOOD_03_BORED: return 3;
        case GotchiMoodType::MOOD_04_HUNGRY: return 4;
        case GotchiMoodType::MOOD_05_THIRSTY: return 5;
        case GotchiMoodType::MOOD_06_SLEEPY: return 6;
        case GotchiMoodType::MOOD_07_SICK: return 7;
        case GotchiMoodType::MOOD_08_SAD: return 8;
        case GotchiMoodType::MOOD_09_ANGRY: return 9;
        case GotchiMoodType::MOOD_10_JOYFUL: return 10;
        case GotchiMoodType::MOOD_11_CONTENT: return 11;
        case GotchiMoodType::MOOD_12_RELIEVED: return 12;
        case GotchiMoodType::MOOD_13_CALM: return 13;
        case GotchiMoodType::MOOD_14_PEACEFUL: return 14;
        case GotchiMoodType::MOOD_15_SERENE: return 15;
        case GotchiMoodType::MOOD_16_OPTIMISTIC: return 16;
        case GotchiMoodType::MOOD_17_HOPEFUL: return 17;
        case GotchiMoodType::MOOD_18_CURIOUS: return 18;
        case GotchiMoodType::MOOD_19_INTERESTED: return 19;
        case GotchiMoodType::MOOD_20_FOCUSED: return 20;
        case GotchiMoodType::MOOD_21_DETERMINED: return 21;
        case GotchiMoodType::MOOD_22_PROUD: return 22;
        case GotchiMoodType::MOOD_23_SUCCESSFUL: return 23;
        case GotchiMoodType::MOOD_24_GRATEFUL: return 24;
        case GotchiMoodType::MOOD_25_LOVED: return 25;
        case GotchiMoodType::MOOD_26_SAFE: return 26;
        case GotchiMoodType::MOOD_27_SECURE: return 27;
        case GotchiMoodType::MOOD_28_TRUSTING: return 28;
        case GotchiMoodType::MOOD_29_AFFECTIONATE: return 29;
        case GotchiMoodType::MOOD_30_LOVING: return 30;
        case GotchiMoodType::MOOD_31_PLAYFUL: return 31;
        case GotchiMoodType::MOOD_32_EXPLORING: return 32;
        case GotchiMoodType::MOOD_33_ADVENTUROUS: return 33;
        case GotchiMoodType::MOOD_34_CREATING: return 34;
        case GotchiMoodType::MOOD_35_LEARNING: return 35;
        case GotchiMoodType::MOOD_36_ACHIEVING: return 36;
        case GotchiMoodType::MOOD_37_COMPETITIVE: return 37;
        case GotchiMoodType::MOOD_38_COLLABORATIVE: return 38;
        case GotchiMoodType::MOOD_39_TEACHING: return 39;
        case GotchiMoodType::MOOD_40_HELPING: return 40;
        case GotchiMoodType::MOOD_41_GIVING: return 41;
        case GotchiMoodType::MOOD_42_GENEROUS: return 42;
        case GotchiMoodType::MOOD_43_ABUNDANT: return 43;
        case GotchiMoodType::MOOD_44_PROSPEROUS: return 44;
        case GotchiMoodType::MOOD_45_HEALTHY: return 45;
        case GotchiMoodType::MOOD_46_FIT: return 46;
        case GotchiMoodType::MOOD_47_ENERGETIC: return 47;
        case GotchiMoodType::MOOD_48_VIBRANT: return 48;
        case GotchiMoodType::MOOD_49_ALIVE: return 49;
        case GotchiMoodType::MOOD_50_SPIRITED: return 50;
        case GotchiMoodType::MOOD_51_CHARMING: return 51;
        case GotchiMoodType::MOOD_52_BEAUTIFUL: return 52;
        case GotchiMoodType::MOOD_53_GLORIOUS: return 53;
        case GotchiMoodType::MOOD_54_MAGNIFICENT: return 54;
        case GotchiMoodType::MOOD_55_WONDERFUL: return 55;
        case GotchiMoodType::MOOD_56_AMAZING: return 56;
        case GotchiMoodType::MOOD_57_INCREDIBLE: return 57;
        case GotchiMoodType::MOOD_58_EXTRAORDINARY: return 58;
        case GotchiMoodType::MOOD_59_SUPERB: return 59;
        case GotchiMoodType::MOOD_60_FANTASTIC: return 60;
        case GotchiMoodType::MOOD_61_AWESOME: return 61;
        case GotchiMoodType::MOOD_62_MIRACULOUS: return 62;
        case GotchiMoodType::MOOD_63_DIVINE: return 63;
        case GotchiMoodType::MOOD_64_ETHEREAL: return 64;
        case GotchiMoodType::MOOD_65_SPIRITUAL: return 65;
        case GotchiMoodType::MOOD_66_MINDFUL: return 66;
        case GotchiMoodType::MOOD_67_WISE: return 67;
        case GotchiMoodType::MOOD_68_KNOWLEDGEABLE: return 68;
        case GotchiMoodType::MOOD_69_INTELLIGENT: return 69;
        case GotchiMoodType::MOOD_70_CLEVER: return 70;
        case GotchiMoodType::MOOD_71_SKILLFUL: return 71;
        case GotchiMoodType::MOOD_72_EXPERT: return 72;
        case GotchiMoodType::MOOD_73_MASTER: return 73;
        case GotchiMoodType::MOOD_74_GRANDMASTER: return 74;
        case GotchiMoodType::MOOD_75_LEGENDARY: return 75;
        case GotchiMoodType::MOOD_76_MYTHICAL: return 76;
        case GotchiMoodType::MOOD_77_EPIC: return 77;
        case GotchiMoodType::MOOD_78_HEROIC: return 78;
        case GotchiMoodType::MOOD_79_VALIANT: return 79;
        case GotchiMoodType::MOOD_80_NOBLE: return 80;
        case GotchiMoodType::MOOD_81_VIRTUOUS: return 81;
        case GotchiMoodType::MOOD_82_RIGHTEOUS: return 82;
        case GotchiMoodType::MOOD_83_FAIR: return 83;
        case GotchiMoodType::MOOD_84_JUST: return 84;
        case GotchiMoodType::MOOD_85_BALANCED: return 85;
        case GotchiMoodType::MOOD_86_HARMONIOUS: return 86;
        case GotchiMoodType::MOOD_87_SYNCED: return 87;
        case GotchiMoodType::MOOD_88_ALIGNED: return 88;
        case GotchiMoodType::MOOD_89_CONNECTED: return 89;
        case GotchiMoodType::MOOD_90_UNITED: return 90;
        case GotchiMoodType::MOOD_91_ONE: return 91;
        case GotchiMoodType::MOOD_92_WHOLE: return 92;
        case GotchiMoodType::MOOD_93_COMPLETE: return 93;
        case GotchiMoodType::MOOD_94_FULL: return 94;
        case GotchiMoodType::MOOD_95_BLOOMING: return 95;
        case GotchiMoodType::MOOD_96_THRIVING: return 96;
        case GotchiMoodType::MOOD_97_GROWING: return 97;
        case GotchiMoodType::MOOD_98_EVOLVING: return 98;
        case GotchiMoodType::MOOD_99_TRANSFORMING: return 99;
        default: return -1;
    }
}

MoodProperties GotchiMood::getMoodProperties() const {
    return getProperties(currentMood);
}

MoodProperties GotchiMood::getProperties(GotchiMoodType mood) const {
    // Return mood properties based on type
    switch (mood) {
        case GotchiMoodType::MOOD_00_HAPPY:
            return {"Happy", "Content and playful", {255, 255, 0, 50}, 1.0f, false, "none", 0.0f, 0.5f};
        case GotchiMoodType::MOOD_04_HUNGRY:
            return {"Hungry", "Needs food", {255, 200, 50, 50}, 2.0f, true, "feed", 0.1f, -0.2f};
        case GotchiMoodType::MOOD_05_THIRSTY:
            return {"Thirsty", "Needs drink", {0, 150, 255, 50}, 2.0f, true, "drink", 0.05f, -0.1f};
        case GotchiMoodType::MOOD_06_SLEEPY:
            return {"Sleepy", "Needs rest", {100, 50, 150, 50}, 2.0f, true, "sleep", -0.1f, -0.5f};
        case GotchiMoodType::MOOD_07_SICK:
            return {"Sick", "Needs care", {150, 255, 150, 50}, 3.0f, true, "heal", -0.2f, -0.3f};
        case GotchiMoodType::MOOD_08_SAD:
            return {"Sad", "Needs comfort", {50, 100, 200, 50}, 1.5f, true, "comfort", 0.0f, 0.3f};
        case GotchiMoodType::MOOD_09_ANGRY:
            return {"Angry", "Needs calming", {255, 50, 50, 50}, 2.0f, true, "calm", 0.1f, -0.4f};
        case GotchiMoodType::MOOD_03_BORED:
            return {"Bored", "Needs interaction", {150, 150, 150, 50}, 1.5f, true, "play", 0.05f, 0.2f};
        default:
            return {getMoodName(), "Standard mood", {255, 255, 255, 30}, 0.5f, false, "none", 0.0f, 0.0f};
    }
}
