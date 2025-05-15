//--------------------------------------------------
// Image Processing Pipeline
// projectScript.cpp
// Date: 2025-05-13
// By Breno Cunha Queiroz
//--------------------------------------------------
#include "projectScript.h"
#include "imgui.h"
#include "implot.h"
#include <atta/graphics/interface.h>
#include <atta/resource/interface.h>
#include <random>

void Project::onLoad() {
    // TODO get project path
    // Save name of all test images when the project is first loaded
    for (const auto& file : fs::directory_iterator("resources")) {
        if (file.path().extension() == ".png") {
            std::string filename = file.path().filename().string();
            _testImages.push_back(filename);
        }
    }
    LOG_INFO("Project", "The project was loaded with [w]$0[] test images", _testImages.size());

    // Default image info
    res::Image::CreateInfo info;
    info.width = 100;
    info.height = 100;
    info.format = res::Image::Format::RGB8;

    // Images to store output of each pipeline stage
    res::Image* ref = res::create<res::Image>("reference", info);
    ref->load(fs::absolute("resources/" + _testImages[_selectedImage]));
    info.width = ref->getWidth();
    info.height = ref->getHeight();

    // Image degradation pipeline
    res::create<res::Image>("deg_black_level", info);
    res::create<res::Image>("deg_dead_pixel", info);
    res::create<res::Image>("deg_white_balance", info);
    res::create<res::Image>("deg_output", info);
}

void Project::onUIRender() {
    ImGui::SetNextWindowSize({1000, 750}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Processing Setup")) {
        // Combo to select test image
        auto imgGetter = [](void* user_data, int idx) -> const char* {
            const auto* vec = static_cast<const std::vector<std::string>*>(user_data);
            if (idx < 0 || idx >= static_cast<int>(vec->size()))
                return nullptr;
            return vec->at(idx).c_str();
        };
        if (ImGui::Combo("Test Image", &_selectedImage, imgGetter, static_cast<void*>(&_testImages), _testImages.size()))
            _shouldReprocess = true;

        // Compute image ratio
        res::Image* refImgRes = res::get<res::Image>("reference");
        float ratio = float(refImgRes->getHeight()) / float(refImgRes->getWidth());

        // Get ImGui images
        ImTextureID refImg = (ImTextureID)gfx::getImGuiImage("reference");
        ImTextureID degBlackLevelImg = (ImTextureID)gfx::getImGuiImage("deg_black_level");
        ImTextureID degDeadPixelImg = (ImTextureID)gfx::getImGuiImage("deg_dead_pixel");
        ImTextureID degWhiteBalanceImg = (ImTextureID)gfx::getImGuiImage("deg_white_balance");
        ImTextureID degOutputImg = (ImTextureID)gfx::getImGuiImage("deg_output");

        // Plot image degradation stages
        const ImPlotAxisFlags axisFlags = ImPlotAxisFlags_NoTickLabels;
        if (ImPlot::BeginPlot("Image degradation pipeline", {-1, 350}, ImPlotFlags_Equal)) {
            ImPlot::SetupAxes(nullptr, nullptr, axisFlags, axisFlags);
            float x = 0.0f;
            ImPlot::PlotImage("Reference image", refImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotImage("Black level offset", degBlackLevelImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotImage("Dead pixel injection", degDeadPixelImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotImage("White balance error", degWhiteBalanceImg, {x, 0}, {x + 1, ratio});
            x += 1.3f;
            ImPlot::PlotImage("Degraded image", degOutputImg, {x, 0}, {x + 1, ratio});
            ImPlot::EndPlot();
        }

        // Plot image processing stages
        if (ImPlot::BeginPlot("Image processing pipeline", {-1, 350}, ImPlotFlags_Equal)) {
            ImPlot::SetupAxes(nullptr, nullptr, axisFlags, axisFlags);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

void Project::onAttaLoop() {
    if (_shouldReprocess) {
        fs::path testImgPath = fs::absolute("resources/" + _testImages[_selectedImage]);
        LOG_INFO("Project", "Processing test image [w]$0[]...", testImgPath);

        res::Image* refImg = res::get<res::Image>("reference");
        uint8_t* refData = refImg->getData();
        uint32_t w = refImg->getWidth();
        uint32_t h = refImg->getHeight();
        uint32_t ch = refImg->getChannels();

        res::Image* blackLevelImg = res::get<res::Image>("deg_black_level");
        uint8_t* blackLevelData = blackLevelImg->getData();

        res::Image* deadPixelImg = res::get<res::Image>("deg_dead_pixel");
        uint8_t* deadPixelData = deadPixelImg->getData();

        res::Image* whiteBalanceImg = res::get<res::Image>("deg_white_balance");
        uint8_t* whiteBalanceData = whiteBalanceImg->getData();

        res::Image* outputImg = res::get<res::Image>("deg_output");
        uint8_t* outputData = outputImg->getData();

        // Load test image
        // refImg->load(testImgPath);
        // Resize images
        // blackLevelImg->resize(refImg->getWidth(), refImg->getHeight());
        // outputImg->resize(refImg->getWidth(), refImg->getHeight());

        //---------- Image degradation pipeline ----------//
        // Black level offset
        for (uint32_t i = 0; i < w * h * ch; i++) {
            if (uint32_t(refData[i]) + _blackLevelOffset >= 255)
                blackLevelData[i] = 255;
            else
                blackLevelData[i] = refData[i] + _blackLevelOffset;
        }
        blackLevelImg->update();

        // Dead pixel injection (randomly set a channel to 0 - simulate photosite failure)
        std::mt19937 gen(42);                           // Random number generator
        std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution in [0, 1]
        for (uint32_t i = 0; i < w * h * ch; i++)
            deadPixelData[i] = dis(gen) < _percentDeadPixels ? 0 : blackLevelData[i];
        deadPixelImg->update();

        // White balance error
        for (uint32_t i = 0; i < w * h; i++) {
            // Get the RGB values for the current pixel
            uint8_t r = deadPixelData[i * ch];
            uint8_t g = deadPixelData[i * ch + 1];
            uint8_t b = deadPixelData[i * ch + 2];

            // Apply the temperature gain to each channel
            const atta::vec3 gains = tempToGain(_colorTemperature);
            whiteBalanceData[i * ch] = static_cast<uint8_t>(std::min(255.0f, r * gains.x));
            whiteBalanceData[i * ch + 1] = static_cast<uint8_t>(std::min(255.0f, g * gains.y));
            whiteBalanceData[i * ch + 2] = static_cast<uint8_t>(std::min(255.0f, b * gains.z));
        }
        whiteBalanceImg->update();

        // Output image
        for (uint32_t i = 0; i < w * h * ch; i++)
            outputData[i] = whiteBalanceData[i];
        outputImg->update();

        _shouldReprocess = false;
    }
}

atta::vec3 Project::tempToGain(float temp) {
    // Clamp temperature to the table's range
    if (temp <= TEMPERATURE_GAIN_MIN)
        return _temperatureGainMap[0];
    if (temp >= TEMPERATURE_GAIN_MAX)
        return _temperatureGainMap[TEMPERATURE_GAIN_COUNT - 1];

    // Calculate the fractional index in the table
    float fractionalIndex = (temp - TEMPERATURE_GAIN_MIN) / TEMPERATURE_GAIN_STEP;

    size_t index1 = static_cast<size_t>(fractionalIndex);
    size_t index2 = index1 + 1;

    // Basic bounds check (should be mostly covered by temp clamping)
    if (index1 >= TEMPERATURE_GAIN_COUNT)
        index1 = TEMPERATURE_GAIN_COUNT - 1;
    if (index2 >= TEMPERATURE_GAIN_COUNT)
        index2 = TEMPERATURE_GAIN_COUNT - 1;

    const atta::vec3& gains1 = _temperatureGainMap[index1];
    const atta::vec3& gains2 = _temperatureGainMap[index2];

    // Calculate the interpolation factor (t)
    float t = fractionalIndex - static_cast<float>(index1);

    // Linear interpolation
    return (1.0f - t) * gains1 + t * gains2;
}
