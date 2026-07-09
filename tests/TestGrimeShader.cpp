// Test program for GrimeShader
// Tests the grime shader with synthetic test patterns
#include "effects/GrimeShader.hpp"
#include "engine/GameState.h"
#include "flags/Keys.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <filesystem>

// raylib includes (must be after system headers for web compatibility)
#include "raylib.h"

// Test dimensions
const int TEST_WIDTH = 128;
const int TEST_HEIGHT = 128;

// Test color - reddish with good saturation
static Color TEST_COLOR = {200, 50, 50, 255};

// Calculate mean luminance and saturation over an image
static void calculateStats(Image img, float& outLuma, float& outSat) {
    float sumLuma = 0.0f;
    float sumSat = 0.0f;
    int count = 0;

    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            Color pixel = GetImageColor(img, x, y);

            // Skip transparent pixels
            if (pixel.a < 10) continue;

            // Calculate RGB in 0-1 range
            float r = pixel.r / 255.0f;
            float g = pixel.g / 255.0f;
            float b = pixel.b / 255.0f;

            // Luminance (Rec. 709 coefficients)
            float luma = r * 0.2126f + g * 0.7152f + b * 0.0722f;
            sumLuma += luma;

            // Saturation: max - min of RGB
            float maxC = fmaxf(fmaxf(r, g), b);
            float minC = fminf(fminf(r, g), b);
            float sat = maxC - minC;
            sumSat += sat;

            count++;
        }
    }

    if (count > 0) {
        outLuma = sumLuma / count;
        outSat = sumSat / count;
    } else {
        outLuma = 0.0f;
        outSat = 0.0f;
    }
}

// Load the grime shader and verify it compiled
bool testShaderLoading() {
    std::cout << "Test: Shader loading..." << std::endl;

    GrimeShader& shader = GrimeShader::instance();

    // Lazy initialization - shader is compiled on first begin() call
    // Call begin() with 0 grime to trigger initialization
    shader.begin(0.0f);
    shader.end();

    if (!shader.loaded()) {
        std::cerr << "  FAILED: GrimeShader not loaded!" << std::endl;
        return false;
    }

    std::cout << "  PASSED: Shader loaded successfully!" << std::endl;
    return true;
}

// Render the test pattern with the shader at a given grime level and return stats
// Uses DrawRectangle instead of DrawTexture for simpler testing
static bool renderAndStats(float grimeAmount, float& outLuma, float& outSat) {
    // Create render texture
    RenderTexture2D target = LoadRenderTexture(TEST_WIDTH, TEST_HEIGHT);

    // Begin rendering to target
    BeginTextureMode(target);
        ClearBackground({0, 0, 0, 0});

        // Draw with shader
        GrimeShader& shader = GrimeShader::instance();
        shader.begin(grimeAmount);
        DrawRectangle(0, 0, TEST_WIDTH, TEST_HEIGHT, TEST_COLOR);
        shader.end();

    EndTextureMode();

    // Convert render texture to image using raylib
    Texture2D tex = target.texture;
    Image img = LoadImageFromTexture(tex);
    calculateStats(img, outLuma, outSat);
    UnloadImage(img);

    // Cleanup
    UnloadRenderTexture(target);

    return true;
}

// Test that shader produces expected results
bool testShaderBehavior() {
    std::cout << "Test: Shader behavior..." << std::endl;

    // First, verify the test color has expected properties
    float r = TEST_COLOR.r / 255.0f;
    float g = TEST_COLOR.g / 255.0f;
    float b = TEST_COLOR.b / 255.0f;
    float luma = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    float maxC = fmaxf(fmaxf(r, g), b);
    float minC = fminf(fminf(r, g), b);
    float sat = maxC - minC;

    std::cout << "  Test color: RGB(" << (int)(r*255) << "," << (int)(g*255) << "," << (int)(b*255) << ")" << std::endl;
    std::cout << "  Expected luma: " << luma << ", saturation: " << sat << std::endl;

    float luma0, sat0;
    if (!renderAndStats(0.0f, luma0, sat0)) {
        std::cerr << "  FAILED: Could not render at grime=0" << std::endl;
        return false;
    }

    float luma50, sat50;
    if (!renderAndStats(0.5f, luma50, sat50)) {
        std::cerr << "  FAILED: Could not render at grime=0.5" << std::endl;
        return false;
    }

    float luma100, sat100;
    if (!renderAndStats(1.0f, luma100, sat100)) {
        std::cerr << "  FAILED: Could not render at grime=1" << std::endl;
        return false;
    }

    // Test results
    const float EPSILON = 0.05f;

    std::cout << "  grime=0:  luma=" << luma0 << ", sat=" << sat0 << std::endl;
    std::cout << "  grime=0.5: luma=" << luma50 << ", sat=" << sat50 << std::endl;
    std::cout << "  grime=1:  luma=" << luma100 << ", sat=" << sat100 << std::endl;

    // Identity at zero: luma at grime=0 should be close to expected
    bool identityPass = std::abs(luma0 - luma) < 0.05f;
    if (!identityPass) {
        std::cerr << "  FAILED: Luminance at grime=0 should be ~" << luma << ", got " << luma0 << std::endl;
        return false;
    }
    std::cout << "  PASSED: Identity at zero (luma ~" << luma << ")" << std::endl;

    // Saturation at grime=0 should match expected
    bool satIdentityPass = std::abs(sat0 - sat) < 0.05f;
    if (!satIdentityPass) {
        std::cerr << "  FAILED: Saturation at grime=0 should be ~" << sat << ", got " << sat0 << std::endl;
        return false;
    }
    std::cout << "  PASSED: Saturation identity at zero (sat ~" << sat << ")" << std::endl;

    // Desaturation: sat at grime=1 should be less than at grime=0
    bool desatPass = sat100 < sat0;
    if (!desatPass) {
        std::cerr << "  FAILED: Saturation should decrease with grime" << std::endl;
        return false;
    }
    std::cout << "  PASSED: Saturation decreased (sat 0->1: " << sat0 << " -> " << sat100 << ")" << std::endl;

    // Darkening: luma at grime=1 should be less than at grime=0
    bool darkPass = luma100 < luma0;
    if (!darkPass) {
        std::cerr << "  FAILED: Luminance should decrease with grime" << std::endl;
        return false;
    }
    std::cout << "  PASSED: Luminance decreased (luma 0->1: " << luma0 << " -> " << luma100 << ")" << std::endl;

    return true;
}

// Export test images at different grime levels
bool exportTestImages() {
    std::cout << "Test: Export test images..." << std::endl;

    const char* baseDir = "./build/build/";
    std::filesystem::create_directories(baseDir);

    float grimeLevels[] = {0.0f, 0.5f, 1.0f};
    const char* filenames[] = {"grime_0.png", "grime_50.png", "grime_100.png"};

    for (int i = 0; i < 3; i++) {
        RenderTexture2D target = LoadRenderTexture(TEST_WIDTH, TEST_HEIGHT);

        BeginTextureMode(target);
            ClearBackground({0, 0, 0, 0});

            GrimeShader& shader = GrimeShader::instance();
            shader.begin(grimeLevels[i]);
            DrawRectangle(0, 0, TEST_WIDTH, TEST_HEIGHT, TEST_COLOR);
            shader.end();

        EndTextureMode();

        // Convert render texture to image
        Texture2D tex = target.texture;
        Image img = LoadImageFromTexture(tex);
        std::string path = std::string(baseDir) + filenames[i];
        ExportImage(img, path.c_str());
        UnloadImage(img);
        UnloadRenderTexture(target);

        std::cout << "  Exported: " << path << std::endl;
    }

    return true;
}

int main() {
#if !defined(PLATFORM_WEB)
    // Initialize window for raylib
    InitWindow(512, 512, "GrimeShader Test");
    SetTargetFPS(60);

    std::cout << "=== GrimeShader Test Suite ===" << std::endl;
    std::cout << std::endl;

    bool allPassed = true;

    // Test 1: Shader loading
    if (!testShaderLoading()) {
        allPassed = false;
    }
    std::cout << std::endl;

    // Test 2: Shader behavior
    if (!testShaderBehavior()) {
        allPassed = false;
    }
    std::cout << std::endl;

    // Test 3: Export images
    if (!exportTestImages()) {
        allPassed = false;
    }
    std::cout << std::endl;

    // Cleanup
    CloseWindow();

    if (allPassed) {
        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    } else {
        std::cout << "=== Some tests failed! ===" << std::endl;
        return 1;
    }
#else
    std::cout << "Test skipped on Web (requires window)" << std::endl;
    return 0;
#endif
}
