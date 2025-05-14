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
    res::Image* input = res::create<res::Image>("input", info);
    input->load(fs::absolute("resources/" + _testImages[_selectedImage]));
    info.width = input->getWidth();
    info.height = input->getHeight();
    res::create<res::Image>("black_level", info);
    res::create<res::Image>("output", info);
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

        ImTextureID inputImg = (ImTextureID)gfx::getImGuiImage("input");
        ImTextureID blackLevelImg = (ImTextureID)gfx::getImGuiImage("black_level");
        ImTextureID outputImg = (ImTextureID)gfx::getImGuiImage("output");

        // Corruption stages
        if (ImPlot::BeginPlot("Corruption stages", {-1, 350})) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
            ImPlot::PlotImage("Original image", inputImg, {0, 0}, {1, 1});
            ImPlot::PlotImage("Black level image", blackLevelImg, {1, 0}, {2, 1});
            ImPlot::PlotImage("Corrupted image", outputImg, {2, 0}, {3, 1});
            ImPlot::EndPlot();
        }

        // Correction stages
        if (ImPlot::BeginPlot("Correction stages", {-1, 350})) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

void Project::onAttaLoop() {
    if (_shouldReprocess) {
        fs::path testImgPath = fs::absolute("resources/" + _testImages[_selectedImage]);
        LOG_INFO("Project", "Processing test image [w]$0[]...", testImgPath);

        res::Image* inputImg = res::get<res::Image>("input");
        uint8_t* inputData = inputImg->getData();
        uint32_t w = inputImg->getWidth();
        uint32_t h = inputImg->getHeight();
        uint32_t ch = inputImg->getChannels();

        res::Image* blackLevelImg = res::get<res::Image>("black_level");
        uint8_t* blackLevelData = blackLevelImg->getData();

        res::Image* outputImg = res::get<res::Image>("output");
        uint8_t* outputData = outputImg->getData();

        // Load test image
        // inputImg->load(testImgPath);
        // Resize images
        // blackLevelImg->resize(inputImg->getWidth(), inputImg->getHeight());
        // outputImg->resize(inputImg->getWidth(), inputImg->getHeight());

        //---------- Image degradation pipeline ----------//
        // Black level offset
        for (uint32_t i = 0; i < w * h * ch; i++) {
            if (uint32_t(inputData[i]) + _blackLevelOffset >= 255)
                blackLevelData[i] = 255;
            else
                blackLevelData[i] = inputData[i] + _blackLevelOffset;
        }
        blackLevelImg->update();

        // Output image
        for (uint32_t i = 0; i < w * h * ch; i++)
            outputData[i] = blackLevelData[i] - _blackLevelOffset;
        outputImg->update();

        _shouldReprocess = false;
    }
}
