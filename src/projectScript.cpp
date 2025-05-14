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
    res::create<res::Image>("input", info)->load(fs::absolute("resources/" + _testImages[_selectedImage]));
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

        ImTextureID imguiImg = (ImTextureID)gfx::getImGuiImage("input");

        // Corruption stages
        if (ImPlot::BeginPlot("Corruption stages", {-1, 350})) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
            ImPlot::PlotImage("Original Image", imguiImg, {0, 0}, {1, 1});
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
        res::Image* blackLevelImg = res::get<res::Image>("black_level");
        res::Image* outputImg = res::get<res::Image>("output");

        // Load test image
        inputImg->load(testImgPath);

        // Resize images
        blackLevelImg->resize(inputImg->getWidth(), inputImg->getHeight());
        outputImg->resize(inputImg->getWidth(), inputImg->getHeight());

        _shouldReprocess = false;
    }
}
