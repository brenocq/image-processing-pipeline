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

void Project::onLoad() {
    // Save name of all test images when the project is first loaded
    for (const auto& file : fs::directory_iterator("resources")) {
        if (file.path().extension() == ".png") {
            std::string filename = file.path().filename().string();
            _testImages.push_back(filename);
        }
    }
}

void Project::onUIRender() {
    ImGui::SetNextWindowSize({1000, 750}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Processing Setup")) {
        // Combo to select test image
        static int selectedImage = 0;
        auto imgGetter = [](void* user_data, int idx) -> const char* {
            const auto* vec = static_cast<const std::vector<std::string>*>(user_data);
            if (idx < 0 || idx >= static_cast<int>(vec->size()))
                return nullptr;
            return vec->at(idx).c_str();
        };
        if (ImGui::Combo("Test Image", &selectedImage, imgGetter, static_cast<void*>(&_testImages), _testImages.size())) {
            LOG_DEBUG("Project::onUIRender", "Selected image: $0", _testImages[selectedImage]);
        }

        ImTextureID imguiImg = (ImTextureID)gfx::getImGuiImage(_testImages[selectedImage]);

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
    // LOG_DEBUG("Project::onAttaLoop", "Hey");
}
