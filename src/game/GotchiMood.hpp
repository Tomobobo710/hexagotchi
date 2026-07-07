#ifndef GOTCHI_MOOD_HPP
#define GOTCHI_MOOD_HPP

#include "raylib.h"
#include <string>
#include <vector>
#include <map>

// Gotchi moods - 100 unique emotional/behavioral states
// First 10 are primary survival/emotional states
// Remaining 90 extend for nuanced tracking
enum class GotchiMoodType {
    // Primary moods (10 core states)
    MOOD_00_HAPPY,        // Content and playful
    MOOD_01_EXCITED,      // Energetic and active
    MOOD_02_SATISFIED,    // Peaceful and relaxed
    MOOD_03_BORED,        // Unstimulated, needs interaction
    MOOD_04_HUNGRY,       // Needs food
    MOOD_05_THIRSTY,      // Needs drink/water
    MOOD_06_SLEEPY,       // Needs rest/sleep
    MOOD_07_SICK,         // Ill, needs medicine/care
    MOOD_08_SAD,          // Upset, needs comfort
    MOOD_09_ANGRY,        // Frustrated, needs calming

    // Extended moods for 100 total tracked states
    MOOD_10_JOYFUL,       // Peak happiness
    MOOD_11_CONTENT,      // Mild happiness
    MOOD_12_RELIEVED,     // Stress removed
    MOOD_13_CALM,         // No arousal
    MOOD_14_PEACEFUL,     // Deep calm
    MOOD_15_SERENE,       // Ultra-calming state
    MOOD_16_OPTIMISTIC,   // Positive outlook
    MOOD_17_HOPEFUL,      // Looking forward
    MOOD_18_CURIOUS,      // Wanting to explore
    MOOD_19_INTERESTED,   // Focused attention
    MOOD_20_FOCUSED,      // Deep concentration
    MOOD_21_DETERMINED,   // Goal-oriented
    MOOD_22_PROUD,        // Achievement feel
    MOOD_23_SUCCESSFUL,   // Goal accomplished
    MOOD_24_GRATEFUL,     // Appreciative
    MOOD_25_LOVED,        // Cared for
    MOOD_26_SAFE,         // No threats
    MOOD_27_SECURE,       // Stable environment
    MOOD_28_TRUSTING,     // Open to connection
    MOOD_29_AFFECTIONATE, // Wanting closeness
    MOOD_30_LOVING,       // Deep care for others
    MOOD_31_PLAYFUL,      // In game mood
    MOOD_32_EXPLORING,    // Active movement
    MOOD_33_ADVENTUROUS,  // Seeking new places
    MOOD_34_CREATING,     // Building/making
    MOOD_35_LEARNING,     // Acquiring knowledge
    MOOD_36_ACHIEVING,    // Progressing goals
    MOOD_37_COMPETITIVE,  // Challenging others
    MOOD_38_COLLABORATIVE,// Working with others
    MOOD_39_TEACHING,     // Sharing knowledge
    MOOD_40_HELPING,      // Assisting others
    MOOD_41_GIVING,       // Selfless acts
    MOOD_42_GENEROUS,     // Abundant sharing
    MOOD_43_ABUNDANT,     // Plenty available
    MOOD_44_PROSPEROUS,   // Thriving
    MOOD_45_HEALTHY,      // Good physical state
    MOOD_46_FIT,          // Physical condition
    MOOD_47_ENERGETIC,    // High energy
    MOOD_48_VIBRANT,      // Lively presence
    MOOD_49_ALIVE,        // Full of life
    MOOD_50_SPIRITED,     // Animated
    MOOD_51_CHARMING,     // Attractive presence
    MOOD_52_BEAUTIFUL,    // Aesthetic state
    MOOD_53_GLORIOUS,     // Magnificent
    MOOD_54_MAGNIFICENT,  // Splendid
    MOOD_55_WONDERFUL,    // Excellent
    MOOD_56_AMAZING,      // Extraordinary
    MOOD_57_INCREDIBLE,   // Unbelievable
    MOOD_58_EXTRAORDINARY,// Beyond normal
    MOOD_59_SUPERB,       // Peak quality
    MOOD_60_FANTASTIC,    // Excellent
    MOOD_61_AWESOME,      // Inspiring awe
    MOOD_62_MIRACULOUS,   // miracle-like
    MOOD_63_DIVINE,       // godlike
    MOOD_64_ETHEREAL,     // Light, airy
    MOOD_65_SPIRITUAL,    // Connected to spirit
    MOOD_66_MINDFUL,      // Present awareness
    MOOD_67_WISE,         // Knowledgeable
    MOOD_68_KNOWLEDGEABLE,// Informed
    MOOD_69_INTELLIGENT,  // Smart
    MOOD_70_CLEVER,       // Quick-witted
    MOOD_71_SKILLFUL,     // Capable
    MOOD_72_EXPERT,       // Masterful
    MOOD_73_MASTER,       // Ultimate level
    MOOD_74_GRANDMASTER,  // Legendary level
    MOOD_75_LEGENDARY,    // Story-level
    MOOD_76_MYTHICAL,     // Legend status
    MOOD_77_EPIC,         // Legendary quality
    MOOD_78_HEROIC,       // Brave deeds
    MOOD_79_VALIANT,      // Courageous
    MOOD_80_NOBLE,        // Honorable
    MOOD_81_VIRTUOUS,     // Moral excellence
    MOOD_82_RIGHTEOUS,    // Just behavior
    MOOD_83_FAIR,         // Equitable
    MOOD_84_JUST,         // Proper justice
    MOOD_85_BALANCED,     // Harmony
    MOOD_86_HARMONIOUS,   // Unity
    MOOD_87_SYNCED,       // Aligned
    MOOD_88_ALIGNED,      // In agreement
    MOOD_89_CONNECTED,    // Linked to others
    MOOD_90_UNITED,       // Together
    MOOD_91_ONE,          // Undivided
    MOOD_92_WHOLE,        // Complete
    MOOD_93_COMPLETE,     // All parts present
    MOOD_94_FULL,         // Not empty
    MOOD_95_BLOOMING,     // Flourishing
    MOOD_96_THRIVING,     // Growing well
    MOOD_97_GROWING,      // Developing
    MOOD_98_EVOLVING,     // Changing
    MOOD_99_TRANSFORMING  // Changing form
};

// Mood properties for behavior and visuals
struct MoodProperties {
    std::string name;
    std::string description;
    Color tint;              // Color overlay for the gotchi
    float priority;          // How strongly this mood drives behavior
    bool requiresAction;     // Does this mood need player intervention?
    std::string actionNeeded;// What action does the gotchi need?
    float energyImpact;      // How mood affects energy drain rate
    float socialImpact;      // How mood affects desire for interaction
};

class GotchiMood {
public:
    GotchiMood();

    // Core mood management
    GotchiMoodType getCurrentMood() const;
    void setCurrentMood(GotchiMoodType mood);
    void updateMood(float deltaTime, const class GotchiStats& stats);

    // Mood utilities
    std::string getMoodName() const;
    Color getMoodTint() const;
    MoodProperties getMoodProperties() const;
    MoodProperties getMoodProperties(GotchiMoodType mood) const;

    // Mood transitions
    void addMoodOverlay(GotchiMoodType mood, float duration);
    void clearMoodOverlays();
    void processMoodOverlays(float deltaTime);

    // Debug/testing
    void setDebugMoodIndex(int index);
    int getDebugMoodIndex() const;

private:
    GotchiMoodType currentMood;
    std::vector<std::pair<GotchiMoodType, float>> activeOverlays;
    float overlayTimer;

    // Mood properties lookup
    MoodProperties getProperties(GotchiMoodType mood) const;
    std::map<GotchiMoodType, MoodProperties> moodCache;
};

#endif // GOTCHI_MOOD_HPP
