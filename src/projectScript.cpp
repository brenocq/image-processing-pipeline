//--------------------------------------------------
// Image Processing Pipeline
// projectScript.cpp
// Date: 2025-05-13
// By Breno Cunha Queiroz
//--------------------------------------------------
#include "projectScript.h"
#include "imgui.h"
#include "implot.h"

void Project::onLoad() { LOG_DEBUG("Project::onLoad", "Hey"); }

void Project::onUIRender() {
    ImGui::SetNextWindowSize({1000, 750}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Processing Setup")) {
        // Corruption stages
        if (ImPlot::BeginPlot("Corruption stages", {-1, 350})) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
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
