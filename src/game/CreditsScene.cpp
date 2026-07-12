#include "CreditsScene.hpp"
#include "GameConstants.hpp"
#include "GameState.h"
#include "HappinessCheckpoints.hpp"
#include "SceneManager.hpp"
#include "ToyAnimationScene.hpp"
#include "SceneInputHandler.hpp"
#include "TutorialController.hpp"
#include "AssetPack.hpp"

CreditsScene::CreditsScene()
    : Scene((float)GAME_W, (float)GAME_H, BLACK) {
    // Base ctor centers the camera at (w/2, h/2), zoom 1.0 -- exactly what we
    // want; no background, black clear color.
}

void CreditsScene::init() {
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);

    // "PLAY AGAIN?" button -- same styling/size/placement as DeathScene's TRY
    // AGAIN, just a different label and reset entry point. Stays hidden
    // (skipped in draw/update) until the cast parade + rank reveal finish.
    float buttonWidth = 200.0f;
    float buttonHeight = 56.0f;
    Vector2 pos = { (GAME_W - buttonWidth) / 2.0f, (float)GAME_H / 2.0f + 90.0f };
    playAgainButton_ = std::unique_ptr<Button>(new Button(pos, buttonWidth, buttonHeight, "PLAY AGAIN?"));
    playAgainButton_->setAnchor("top-left");
    playAgainButton_->setFontSize(22);
    playAgainButton_->setBackgroundColor({60, 60, 100, 220});
    playAgainButton_->setHoverColor({100, 100, 160, 240});
    playAgainButton_->setBorderColor({150, 150, 200, 255});
    playAgainButton_->setOnClick([this]() { onPlayAgain(); });

    // Cast parade order -- everyone who appears across the story, happy pose
    // only. Narrator/Phone have no pose art (CharacterRegistry) so they're
    // excluded; Mark only has one neutral pose, included anyway for laughs.
    static const CharacterId kCastOrder[] = {
        CharacterId::Tom, CharacterId::Karen, CharacterId::Ronzer,
        CharacterId::Jimmy, CharacterId::Bimmy, CharacterId::Judy,
        CharacterId::Larry, CharacterId::Loraine, CharacterId::Mark,
    };
    cast_.clear();
    for (CharacterId id : kCastOrder) {
        Texture2D pose = CharacterRegistry::loadPose(id, PoseEmotion::Happy);
        if (pose.id != 0) cast_.push_back({id, pose});
    }
    castIndex_ = 0;
    castElapsed_ = 0.0f;
    phase_ = cast_.empty() ? Phase::RankLabel : Phase::Cast;
    phaseElapsed_ = 0.0f;

    // Rank from the 3-checkpoint happiness ledger: 0/3=D-, 1/3=C, 2/3=B, 3/3=A+.
    int passed = happyCheckpointsPassed(globalGameState);
    switch (passed) {
        case 3:  rankLetter_ = "A+"; rankColor_ = Color{80, 220, 120, 255};  break;
        case 2:  rankLetter_ = "B";  rankColor_ = Color{120, 190, 230, 255}; break;
        case 1:  rankLetter_ = "C";  rankColor_ = Color{230, 200, 60, 255};  break;
        default: rankLetter_ = "D-"; rankColor_ = Color{220, 90, 90, 255};   break;
    }
}

void CreditsScene::update(float deltaTime) {
    Scene::update(deltaTime);

    switch (phase_) {
        case Phase::Cast:
            castElapsed_ += deltaTime;
            if (castElapsed_ >= STEP_DURATION) {
                castElapsed_ = 0.0f;
                castIndex_++;
                if (castIndex_ >= (int)cast_.size()) {
                    phase_ = Phase::RankLabel;
                    phaseElapsed_ = 0.0f;
                }
            }
            break;
        case Phase::RankLabel:
            phaseElapsed_ += deltaTime;
            if (phaseElapsed_ >= RANK_LABEL_DURATION) { phase_ = Phase::RankPause; phaseElapsed_ = 0.0f; }
            break;
        case Phase::RankPause:
            phaseElapsed_ += deltaTime;
            if (phaseElapsed_ >= RANK_PAUSE_DURATION) { phase_ = Phase::RankReveal; phaseElapsed_ = 0.0f; }
            break;
        case Phase::RankReveal:
            phaseElapsed_ += deltaTime;
            if (phaseElapsed_ >= RANK_REVEAL_DURATION) { phase_ = Phase::Done; phaseElapsed_ = 0.0f; }
            break;
        case Phase::Done:
            break;
    }

    // Button only interactive once everything has played out.
    if (playAgainButton_ && phase_ == Phase::Done) {
        playAgainButton_->update(getInputHandler(), deltaTime);
    }
}

void CreditsScene::onPlayAgain() {
    // Fresh-run reset (see DeathScene::onTryAgain). Overwrites the shared global
    // by value so every GameState* holder sees it at once. PRESERVES
    // tutorial_seen -- someone who just finished the whole game replaying it
    // shouldn't be re-tutorialized. Audio/dialog preferences are external
    // globals, unaffected either way.
    ResetRunKeepingTutorial(globalGameState);

    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("toy_animation");
        ToyAnimationScene* toyAnim = static_cast<ToyAnimationScene*>(mgr->getScene("toy_animation"));
        if (toyAnim) toyAnim->startIntro("gotchi");
    }
}

// Eases a 0..1 phase fraction into a walk-on/hold/walk-off screen-X offset
// (in pixels, from off-left to on-center to off-right), plus alpha for a
// slight fade at the very edges of walk-in/walk-out.
static void castStepTransform(float elapsed, float& outX, unsigned char& outAlpha) {
    const float walkIn = CreditsScene::WALK_IN_DURATION;
    const float hold = CreditsScene::HOLD_DURATION;
    const float walkOut = CreditsScene::WALK_OUT_DURATION;
    const float offscreen = (float)GAME_W * 0.75f;

    if (elapsed < walkIn) {
        float t = elapsed / walkIn;
        t = t * t * (3.0f - 2.0f * t); // smoothstep
        outX = -offscreen + t * offscreen;
        outAlpha = (unsigned char)(255.0f * t);
    } else if (elapsed < walkIn + hold) {
        outX = 0.0f;
        outAlpha = 255;
    } else {
        float t = (elapsed - walkIn - hold) / walkOut;
        if (t > 1.0f) t = 1.0f;
        t = t * t * (3.0f - 2.0f * t);
        outX = t * offscreen;
        outAlpha = (unsigned char)(255.0f * (1.0f - t));
    }
}

void CreditsScene::draw() {
    Scene::draw();

    // "CAST" header, white, centered horizontally near the top -- stays up
    // for the whole parade.
    if (phase_ == Phase::Cast) {
        const char* title = "CAST";
        int titleSize = 56;
        int titleWidth = MeasureText(title, titleSize);
        int titleX = (GAME_W - titleWidth) / 2;
        int titleY = 40;
        DrawText(title, titleX, titleY, titleSize, WHITE);
    }

    if (phase_ == Phase::Cast && castIndex_ < (int)cast_.size()) {
        const CastEntry& entry = cast_[castIndex_];
        const CharacterInfo& info = CharacterRegistry::get(entry.id);

        float offsetX = 0.0f;
        unsigned char alpha = 255;
        castStepTransform(castElapsed_, offsetX, alpha);

        float scale = 2.0f;
        float poseW = entry.pose.width * scale;
        float poseH = entry.pose.height * scale;
        float poseX = (GAME_W - poseW) / 2.0f + offsetX;
        float poseY = (GAME_H - poseH) / 2.0f + 20.0f;

        Color tint = { 255, 255, 255, alpha };
        Rectangle src = { 0.0f, 0.0f, (float)entry.pose.width, (float)entry.pose.height };
        Rectangle dest = { poseX, poseY, poseW, poseH };
        DrawTexturePro(entry.pose, src, dest, {0.0f, 0.0f}, 0.0f, tint);

        // Name label below the pose, in the character's identity color.
        int nameSize = 32;
        int nameWidth = MeasureText(info.displayName.c_str(), nameSize);
        int nameX = (int)((GAME_W - nameWidth) / 2.0f + offsetX);
        int nameY = (int)(poseY + poseH) + 16;
        Color nameTint = info.nameColor;
        nameTint.a = alpha;
        DrawText(info.displayName.c_str(), nameX, nameY, nameSize, nameTint);
    }

    if (phase_ == Phase::RankLabel || phase_ == Phase::RankPause || phase_ == Phase::RankReveal || phase_ == Phase::Done) {
        const char* label = "Rank:";
        int labelSize = 48;
        int labelWidth = MeasureText(label, labelSize);

        bool showRank = (phase_ == Phase::RankReveal || phase_ == Phase::Done);
        unsigned char rankAlpha = 255;
        float rankScale = 1.0f;
        if (phase_ == Phase::RankReveal) {
            float t = phaseElapsed_ / RANK_REVEAL_DURATION;
            if (t > 1.0f) t = 1.0f;
            rankAlpha = (unsigned char)(255.0f * t);
            rankScale = 0.7f + 0.3f * t; // slight pop-in
        }

        int rankSize = showRank ? (int)(64 * rankScale) : 0;
        int rankWidth = showRank ? MeasureText(rankLetter_.c_str(), rankSize) : 0;
        int gap = showRank ? 20 : 0;

        int totalWidth = labelWidth + gap + rankWidth;
        int startX = (GAME_W - totalWidth) / 2;
        int centerY = GAME_H / 2;

        DrawText(label, startX, centerY - labelSize / 2, labelSize, WHITE);

        if (showRank) {
            Color tint = rankColor_;
            tint.a = rankAlpha;
            DrawText(rankLetter_.c_str(), startX + labelWidth + gap, centerY - rankSize / 2, rankSize, tint);
        }
    }

    if (playAgainButton_ && phase_ == Phase::Done) {
        playAgainButton_->draw();
    }
}

void CreditsScene::cleanup() {
    Scene::cleanup();
    playAgainButton_.reset();
    for (auto& entry : cast_) {
        if (entry.pose.id != 0) UnloadTexture(entry.pose);
    }
    cast_.clear();
    castIndex_ = 0;
    castElapsed_ = 0.0f;
    phase_ = Phase::Cast;
    phaseElapsed_ = 0.0f;
}
