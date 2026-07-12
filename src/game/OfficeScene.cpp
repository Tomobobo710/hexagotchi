#include "OfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "AudioManager.hpp"
#include "CharacterRegistry.hpp"
#include "GameState.h"
#include "HappinessCheckpoints.hpp"
#include <cmath>
#include <cstdio>

namespace {
// Round a live stat to a whole number for the "reports" beat -- Loraine reads
// them aloud, so a clean integer reads better than a float.
int statPct(float value) {
    return (int)(value + 0.5f);
}
}

// Defined in main.cpp -- true for the frame in which a click was consumed by
// the global Tom-world pause button/menu.
extern bool IsPauseUiClaimingClick();

OfficeScene::OfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {20, 22, 28, 255}), dialog(sharedDialog) {
}

void OfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/officebg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]     = CharacterRegistry::loadPose(CharacterId::Tom,     (PoseEmotion)e);
        larryPoses[e]   = CharacterRegistry::loadPose(CharacterId::Larry,   (PoseEmotion)e);
        lorainePoses[e] = CharacterRegistry::loadPose(CharacterId::Loraine, (PoseEmotion)e);
    }

    // Positions from the scene editor's layout.json. Tom stands at open
    // (left), the scenario zooms on him then walks him to (546,450). Larry
    // starts off the bottom-right and walks up to Tom. Loraine waits off the
    // bottom until Larry calls her, then walks up to Tom's left.
    tom = new SceneActor({90.0f, 306.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    larry = new SceneActor({1175.0f, 861.0f}, 50.0f, 76.0f);
    larry->setTag("larry");
    larry->setVisible(false);
    addActor(larry);

    loraine = new SceneActor({170.0f, 919.0f}, 50.0f, 76.0f);
    loraine->setTag("loraine");
    loraine->setVisible(false);
    addActor(loraine);

    // Build Loraine's live "report" lines from the player's REAL vitals so the
    // numbers on screen are the actual creature the player has been keeping (or
    // neglecting). Corporate-speak names hide gotchi wellbeing stats -- the
    // joke is HR reading a suffering pet's vitals as quarterly KPIs. Falls back
    // to a default GotchiStats if no GameState is wired (isolated tests).
    GotchiStats fallback;
    const GotchiStats& v = gameState_ ? gameState_->vitals : fallback;
    char buf[256];

    // Line 1: the "engagement" framing -- Happiness dressed up as Engagement,
    // Focus as Attention Capital.
    std::snprintf(buf, sizeof(buf),
        "Engagement's at %d%%, sir.\nAttention Capital, %d%%.",
        statPct(v.getHappiness()), statPct(v.getStat(EmotionalStat::FOCUS)));
    std::string loraineStats1 = buf;

    // Line 2: vitals reframed -- Health as Operational Readiness, Energy as
    // Bandwidth, Hunger inverted into a "Fuel Reserve" so low food reads as a
    // red flag.
    std::snprintf(buf, sizeof(buf),
        "Operational Readiness, %d%%.\nBandwidth's down to %d%%.\nFuel Reserves at %d%%.",
        statPct(v.getHealth()), statPct(v.getEnergy()),
        statPct(100.0f - v.getHunger()));
    std::string loraineStats2 = buf;

    // Line 3: the soft stuff -- Hygiene as Presentation, Anxiety surfaced
    // plainly as a "Risk Indicator," and the gut-punch: zero Friends.
    std::snprintf(buf, sizeof(buf),
        "Presentation, %d%%.\nRisk Indicator -- that's anxiety --\nflagged at %d%%.",
        statPct(v.getHygiene()), statPct(v.getStat(EmotionalStat::ANXIETY)));
    std::string loraineStats3 = buf;

    // Line 4: the gut-punch, on its own line so it lands -- zero friends.
    std::snprintf(buf, sizeof(buf),
        "And... social connections: %d.",
        statPct(v.getStat(SocialStat::FRIENDS)));
    std::string loraineStats4 = buf;

    // --- Projection-tagged stat lines for Scenario E (index 4) -------------
    // Stats shown are live flavor; the verdict at the end of each line is the
    // real signal. Proj_N reads happiness checkpoint N straight off the ledger:
    // "looking good" = that check passed, "didn't meet target" = it failed.
    auto projTag = [](bool passed) -> const char* {
        return passed ? "The projection is looking\ngood for this quarter."
                      : "We didn't quite meet\ntarget here.";
    };
    GameState fallbackState;
    const GameState& gs = gameState_ ? *gameState_ : fallbackState;

    // E-Line 1: Engagement (Happiness) + Vitality (Health).
    std::snprintf(buf, sizeof(buf),
        "Engagement's at %d%%. Vitality, %d%%.\n%s",
        statPct(v.getHappiness()), statPct(v.getHealth()),
        projTag(getHappinessCheckpoint(gs, 1)));
    std::string loraineProj1 = buf;

    // E-Line 2: Bandwidth (Energy) + Fuel (100 - Hunger).
    std::snprintf(buf, sizeof(buf),
        "Bandwidth, %d%%. Fuel Reserves, %d%%.\n%s",
        statPct(v.getEnergy()), statPct(100.0f - v.getHunger()),
        projTag(getHappinessCheckpoint(gs, 2)));
    std::string loraineProj2 = buf;

    // E-Line 3: Presentation (Hygiene) + the anxiety Risk Indicator.
    std::snprintf(buf, sizeof(buf),
        "Presentation, %d%%. Risk Indicator, %d%%.\n%s",
        statPct(v.getHygiene()), statPct(v.getStat(EmotionalStat::ANXIETY)),
        projTag(getHappinessCheckpoint(gs, 3)));
    std::string loraineProj3 = buf;

    // --- Scenario 0 (Scenario A): the first merge into Tom's world ---------
    // Player merges in, Tom's already standing there feeling sick from it,
    // Larry strides up to greet him. Camera is smooth (followPosition) by
    // default; cutCamera=true is reserved for dramatic beats.
    scenarios.push_back({
        { CharacterId::Tom, "Ugh, that always makes me nauseous.",
          0, false, false, PortraitEmotion::Sad, "Tom Gatchi",
          {}, {}, PoseEmotion::Sad },

        // Larry's greeting: fire BOTH walks the instant this line starts, so
        // they stroll into position while he's talking.
        { CharacterId::Larry, "Tom A. Gotchi!\nJust the man I was looking for!",
          1, false, false, PortraitEmotion::Mid, "Larry (Tom's boss)",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 140.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 260.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Mid },

        { CharacterId::Larry, "How was the merge?\nDid you remember the three E's?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "Engagement. Engagement.\nEngagement.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        // Larry lights up here -- pose flips Mid -> Happy for the rest of A.
        { CharacterId::Larry, "Exactly! Did the kid engage?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I don't know.\nI can't perform right now.\nSo much in my head.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },

        // Larry bellows for his secretary; fire her walk-in the instant he
        // starts yelling so she strides up from the bottom to Tom's left
        // while he's still shouting.
        { CharacterId::Larry, "LORAAAINE!!!!",
          1, true, false, PortraitEmotion::Happy, "",
          /*movesAtStart*/ {
              ActorMove{2, {{206.0f, 342.0f}}, 280.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Loraine, "Yes sir?",
          2, false, false, PortraitEmotion::Mid, "Loraine (the secretary)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy },
        // --- Larry asks for Tom's own vitals (the player-managed stats);
        // Loraine reads them. These are TOM'S numbers -- his performance as a
        // well-kept toy -- not the alien's. See Scenario D's frame comment.
        { CharacterId::Larry, "Loraine, pull up the numbers\non Tom's latest merge.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Loraine, "Here are the reports, sir.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },

        // Larry squints at a wall of live vitals and can't parse a single one.
        { CharacterId::Larry, "Mm. The numbers don't lie, Tom.\nAnd the numbers are...\nHard to read...",
          1, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        // --- Injected: Larry punts to Loraine, she reads the REAL stats ------
        { CharacterId::Larry, "Loraine. Walk me through it.\nIn corporate English.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, loraineStats1,
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, loraineStats2,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Loraine, loraineStats3,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Loraine, loraineStats4,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Loraine, loraineProj1,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        // Tom clocks that every number just went down. Larry, of course, does not.
        { CharacterId::Tom, "...Those numbers always just go down.\nEvery single one of them.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Larry, "Down means room to grow, Tom!\nUpside! I love it.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Mid },

        // --- Comedy beat: lands on Tom coming in 15 minutes earlier ---------
        { CharacterId::Tom, "I'll do better next time. I just need to get a good night's sleep.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "And that's the beauty of accountability!\nWe grow together.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Larry, "Loraine, put Tom down for the\nEarly Bird Optimization Initiative.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Tom, "The what?",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        // Loraine drops from Happy to Mid as she delivers the bad news.
        { CharacterId::Loraine, "EBOI. It just means you come in fifteen\nminutes earlier. Every day.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "Fifteen minutes earlier.\nForever?",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "That's the spirit!\nWelcome to the winning team, Tom.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "...I need to sit down.\nWe don't even have chairs here.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
    });

    // --- Scenario 1 (Scenario B): Tom's two minutes late -------------------
    // A quick back-and-forth: Tom slinks in apologetic, Larry hammers him and
    // shoves him back out to "make the kid engage." Ends with Tom heading
    // back to merge out. Larry oblivious-Happy throughout, Tom sad/scared.
    scenarios.push_back({
        // Place both at their talking spots the instant the scenario opens.
        { CharacterId::Tom, "Sorry! Sorry. I'm so sorry.\nI'm two minutes late, I know.",
          0, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 900.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 900.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Two minutes, Tom.\nDo you know what two minutes IS?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "...A hundred and twenty seconds?",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "It's a MINDSET, Tom.\nIt's a hundred and twenty seconds of\nnot being a team player.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "The train was --",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Winners don't take the train, Tom.\nThe winners ARE the train.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "That doesn't make any sense.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Now get out there and make that\nkid ENGAGE. Engagement, Tom!",
          1, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy },
        { CharacterId::Tom, "...Right. Engaging.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Narrator, "Tom goes back out there.",
          -1, false },
    });

    // --- Scenario 2 (Scenario Z): the endcap -------------------------------
    // A tiny reusable "return Tom to the gotchi side" beat: Tom walks in from
    // off the left edge to the middle of the office, mutters one psych-himself-
    // up line, and that's it. Use this scenario whenever a merge sequence just
    // needs to end on the office and hand back to the tomagotchi side without a
    // full scripted scene. The specific mutter is picked at trigger time from a
    // small pool (see triggerScenario) so repeat visits don't always read the
    // same line -- the text baked here is just the default/fallback.
    scenarios.push_back({
        { CharacterId::Tom, "Keep it together. You need this job.",
          0, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ { ActorMove{0, {{500.0f, 320.0f}}, 200.0f} }, {},
          PoseEmotion::Sad },
    });

    // --- Scenario 3 (Scenario D): "what does Tom actually DO?" -------------
    // THE CORE FRAME: Tom IS a tomagotchi. Larry runs a business of people who
    // are tomagotchis for players in another universe -- Tom is one of his
    // assets. Tom's JOB is to BE the toy: to get fed, cleaned, kept happy, and
    // to perform for the alien kid (= the actual PLAYER) pressing the buttons.
    // Loraine reads TOM'S OWN live vitals -- the stats the player has been
    // managing -- as his performance review. The KPI is the alien's engagement
    // (they keep playing), and a well-kept toy drives it. Larry is fully in on
    // the reality; this is just his industry. Comedy = the gap between how
    // gravely Larry treats it and how absurd "professional pet" is; dark =
    // Tom slowly getting that his whole worth is being a fun toy.
    scenarios.push_back({
        { CharacterId::Larry, "Tom. Sit. Well -- stand. We don't\nhave chairs. This is a big one.",
          1, false, false, PortraitEmotion::Mid, "",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 900.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 900.0f},
          }, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Larry, "Your player's up for their quarterly\nEngagement review. Your player, Tom.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "...The kid. The little alien\nwho won't stop poking me.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Larry, "The CLIENT, Tom. The player. You are\ntheir toy. A happy toy keeps them playing.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        // Indignant outburst -- Scared pose for the flare-up, shake.
        { CharacterId::Tom, "Why does some snot-faced alien kid\nget to press buttons on ME all day?",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy },
        { CharacterId::Tom, "I'm a grown man. I've gone through divorce.\nI have my own apartment.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy },
        { CharacterId::Larry, "And a book of business worth MILLIONS\nin engagement. Loraine -- his numbers.",
          1, false, false, PortraitEmotion::Mid, "",
          /*movesAtStart*/ {
              ActorMove{2, {{206.0f, 342.0f}}, 320.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Loraine, "Right away, sir.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        // Loraine reads TOM'S OWN live vitals -- the ones the player manages.
        // This is his performance review as a toy; a fed/clean/happy Tom is
        // what keeps the alien engaged.
        { CharacterId::Loraine, loraineStats1,
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, loraineStats2,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Loraine, loraineProj2,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Larry, "You SEE that, Tom? When YOUR numbers\ndip, the player logs off. Bad for everyone.",
          1, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Sad },
        { CharacterId::Tom, "...So my job is to let myself\nbe taken care of. By a child. An alien one.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "Now you're getting it!\nThat's the whole job, baby!",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "Eat when they feed me.\nSleep when they let me. Smile for the glass.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Loraine, "It's more stable than most careers,\nhonestly. Full benefits, too.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "...I don't know what to do with that.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "Nothing! Do NOTHING harder! Go out there\nand get PAMPERED, champ! Engagement!",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
    });

    // --- Scenario 4 (Scenario E): the final office check-in ----------------
    // Used by the merge-5 sequence (the last happiness checkpoint). Larry asks
    // how the day went; Tom says he's doing his best; Larry quips and calls
    // Loraine in for the numbers. She reads three projection-tagged stat lines
    // (each ending in a "looking good this quarter" / "didn't quite meet
    // target" verdict off the REAL vitals). Larry does his usual oblivious
    // spin, Tom laments, scene. Same actor layout as A/B/D.
    scenarios.push_back({
        { CharacterId::Larry, "Tom! There he is. How'd we do today?",
          1, false, false, PortraitEmotion::Happy, "",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 900.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 900.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I'm... doing my best, Larry.\nHonestly. I'm trying.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Your best! I love it. You know what\nthey say -- best is just good, squared.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "That's not a saying, Larry.\nThat's not... math.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "It's a MINDSET. Loraine! Bring me\nthe end-of-cycle numbers on Tom.",
          1, false, false, PortraitEmotion::Mid, "",
          /*movesAtStart*/ {
              ActorMove{2, {{206.0f, 342.0f}}, 320.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Loraine, "Already pulled them, sir.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, loraineProj3,
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Larry, "See that? Numbers on a page.\nThat's a life, Tom. That's a LEGACY.",
          1, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Sad },
        { CharacterId::Tom, "It's a spreadsheet of whether\na child was entertained by me.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Sad },
        { CharacterId::Larry, "And what a beautiful spreadsheet it is!\nKeep it UP, champ. We believe in you.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "...I don't know how much longer\nI can keep smiling for the glass.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Sad },
    });
}

void OfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (isPaused()) return;

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (activeScenario < 0) {
        // Ambient (pre-scenario / after it ends): Tom stands at his open spot
        // where the scenario begins, with a gentle idle bob. Not the yoga-ball
        // wobble anymore -- Scenario A opens on Tom already standing here and
        // then walks him in, so pinning him elsewhere would teleport him on
        // the first move. Camera is left alone; the scenario's first line
        // zooms in on him itself.
        tomWobbleTimer += deltaTime * 3.0f;
        tom->setPosition({90.0f, 306.0f + sinf(tomWobbleTimer) * 4.0f});
    }

    // Keep the camera locked on the speaking actor's LIVE position every
    // frame, so it tracks them smoothly as they moveTo()-walk into place
    // rather than easing once to where they stood when the line started.
    // Skipped in wide view (debug) so the numpad-0 override isn't fought.
    if (activeScenario >= 0 && currentFocusActor >= 0
        && !getCamera()->isShaking() && !getCamera()->isWideViewEnabled()) {
        Vector2 t;
        if (cameraTargetFor(currentFocusActor, t)) {
            getCamera()->followPosition(t, 8.0f);

            // Fire a deferred line-shake once the camera has settled on the
            // speaker -- within SETTLE_RADIUS px of the focus target. Lands the
            // punch on a stable frame aimed at the right actor, not mid-pan.
            if (pendingShake_) {
                Vector2 cam = getCamera()->getPosition();
                float dx = cam.x - t.x, dy = cam.y - t.y;
                const float SETTLE_RADIUS = 24.0f;
                if (dx * dx + dy * dy <= SETTLE_RADIUS * SETTLE_RADIUS) {
                    getCamera()->shake(5.0f, 0.3f);
                    pendingShake_ = false;
                }
            }
        }
    }

    if (activeScenario >= 0 && dialog->isVisible()) {
        SceneInputHandler* ih = getInputHandler();
        // Check for skip action (jump to end of scenario)
        if (ih && ih->isActionPressed(INPUT_ACTION_SKIP)) {
            endScenario();
            return;
        }
        // Check for normal advance (next line)
        if (dialog->isFinished()) {
            bool manualAdvance = ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE) ||
                                        (ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsPauseUiClaimingClick()));
            if (manualAdvance) AudioManager::Get().playClick();  // no click on auto-advance
            if (dialog->consumeAutoAdvance() || manualAdvance) {
                advanceLine();
            }
        }
    }
}

void OfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();

    // Background art first (its own 2D pass).
    BeginMode2D(cam);
    drawOffice();
    EndMode2D();

    // Actors last. Once the scenario is ending (the
    // black fade is ramping), stop drawing every actor so none of them show
    // under/after the fade -- the scene reads as fully cleared before it
    // hands off.
    bool ending = endElapsed >= 0.0f;
    BeginMode2D(cam);
    if (!ending) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) drawLarry(larry->getPosition());
        // Loraine starts off the bottom edge and is only walked in on the
        // "LORAAAINE!!!!" beat, so drawing her whenever the scenario is live is
        // fine -- she's simply off-screen until she strides up.
        if (activeScenario >= 0) drawLoraine(loraine->getPosition());
    }
    EndMode2D();

    // Starts ramping immediately when endScenario() fires (endElapsed jumps
    // to 0.0f right then) and climbs continuously to fully opaque over
    // END_FADE_DURATION seconds -- no held/dead time before it starts.
    // isPlayingScenario() (below) stays true for that whole span, so
    // StorySequencer doesn't react and start its own scene-switch FADE until
    // this screen is already fully black -- that transition's fade-in half
    // then continues seamlessly straight out of this one.
    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void OfficeScene::cleanup() {
    Scene::cleanup();
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't
    // accumulate duplicates and a mid-scenario exit doesn't permanently
    // block triggerScenario().
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;

    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0) UnloadTexture(tomPoses[i]);
        if (larryPoses[i].id != 0) UnloadTexture(larryPoses[i]);
        if (lorainePoses[i].id != 0) UnloadTexture(lorainePoses[i]);
    }
}

void OfficeScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    // Scenario Z (endcap) picks Tom's mutter from a small pool so repeat uses
    // don't always read the same line. Rotates deterministically (no rand seed
    // needed); overwrites just this scenario's single line's text in place.
    if (index == 2 && !scenarios[2].empty()) {
        static const char* Z_MUTTERS[] = {
            "Keep it together. You need this job.",
            "Ugh. Mondays.",
            "Just smile. Just nod. Just breathe.",
            "One more day. It's just one more day.",
            "You're a professional. You're a professional.",
        };
        static int zPick = 0;
        int n = (int)(sizeof(Z_MUTTERS) / sizeof(Z_MUTTERS[0]));
        scenarios[2][0].text = Z_MUTTERS[zPick % n];
        zPick++;
    }

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void OfficeScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool OfficeScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void OfficeScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[3] = {tom, larry, loraine};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 3);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void OfficeScene::playLine(const OfficeLine& line) {
    // setCharacter() naturally handles Narrator too -- no portrait art
    // registered for it, so the portrait clears on its own; only the name
    // plate and its color show.
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    // Pose emotions persist between lines -- each line carries the full set,
    // so a line just restates whatever should currently be showing.
    tomPoseEmotion = line.tomPoseEmotion;
    larryPoseEmotion = line.larryPoseEmotion;
    lorainePoseEmotion = line.lorainePoseEmotion;

    SceneActor* actorsByIndex[3] = {tom, larry, loraine};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void OfficeScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

// The point the camera aims at for a given actor, from its LIVE position.
// Actor position is now the pose's top-left (editor convention), so offset
// into the visible body rather than using getCenter() (which is based on the
// small 48x64 placeholder box, not the drawn pose). Returns false if there's
// no such actor (e.g. Narrator, actorIndex -1) so the caller leaves the
// camera where it is.
bool OfficeScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = larry;
    else if (actorIndex == 2) target = loraine;
    if (!target) return false;

    Vector2 p = target->getPosition();
    // Same recipe as ApartmentScene: poses draw at ~full native size, so aim
    // well right and low into the body to frame the torso (not the head) and
    // keep the figure off the frame edge.
    out = { p.x + 160.0f, p.y + 240.0f };
    return true;
}

void OfficeScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
    currentFocusActor = actorIndex;  // update() keeps following this every frame

    Vector2 t;
    bool haveTarget = cameraTargetFor(actorIndex, t);
    if (haveTarget) {
        // Smooth ease (PizzaParlorScene's normal look) unless the line asks
        // for a hard cut for a dramatic beat. Either way update()'s per-frame
        // follow takes over from here to track the actor as they walk.
        if (cut) getCamera()->setPosition(t.x, t.y);
        else getCamera()->followPosition(t, 8.0f);
        getCamera()->zoomTo(2.0f, 0.5f);
    }

    // Shake timing: on a hard cut the camera is already on the actor, so punch
    // now. On a smooth ease, DEFER -- update() fires it once the camera has
    // settled on the speaker, so the shake lands on a stable frame instead of
    // mid-pan (see pendingShake_).
    pendingShake_ = false;
    if (shake) {
        if (cut || !haveTarget) getCamera()->shake(5.0f, 0.3f);
        else pendingShake_ = true;
    }
}

// --- Set dressing ---------------------------------------------------------
void OfficeScene::drawOffice() {
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
}

// Draw a pose EXACTLY the way tools/scene_editor.cpp draws it: texture
// top-left pinned at the actor's position, native size scaled by POSE_SCALE,
// origin {0,0}. This is the shared convention that keeps the editor and the
// game aligned -- an actor's (x,y) from the editor JSON drops straight into
// its SceneActor construction with no anchor translation. flipX mirrors
// horizontally (negative source width), same as the editor's flipX toggle.
// Per-actor scale (from the scene editor's layout.json), same as
// ApartmentScene -- pose top-left at pos, native size * scale, flipX via
// negative source width.
static void drawPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void OfficeScene::drawTom(Vector2 pos) {
    drawPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right
}

void OfficeScene::drawLarry(Vector2 pos) {
    drawPose(larryPoses[(int)larryPoseEmotion], pos, /*flipX*/ false, 1.0f);  // faces left
}

void OfficeScene::drawLoraine(Vector2 pos) {
    drawPose(lorainePoses[(int)lorainePoseEmotion], pos, /*flipX*/ true, 1.0f);  // to Tom's left, faces right toward him
}


