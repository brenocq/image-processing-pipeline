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

std::array<atta::vec3, Project::COLOR_SHADING_COUNT> Project::_colorShadingError = {
    // {R_gain, G_gain, B_gain} // Distance from center (Index 0 = center, Index N = corner)
    atta::vec3{1.000f, 1.000f, 1.000f}, // Index 0 (Center)
    atta::vec3{1.022f, 0.978f, 1.022f}, // Index 1
    atta::vec3{1.044f, 0.956f, 1.044f}, // Index 2
    atta::vec3{1.067f, 0.933f, 1.067f}, // Index 3
    atta::vec3{1.089f, 0.911f, 1.089f}, // Index 4
    atta::vec3{1.111f, 0.889f, 1.111f}, // Index 5 (Mid-way)
    atta::vec3{1.133f, 0.867f, 1.133f}, // Index 6
    atta::vec3{1.156f, 0.844f, 1.156f}, // Index 7
    atta::vec3{1.178f, 0.822f, 1.178f}, // Index 8
    atta::vec3{1.200f, 0.800f, 1.200f}  // Index 9 (Corner - strong magenta cast)
};

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
    res::create<res::Image>("deg_white_balance", info);
    res::create<res::Image>("deg_lens", info);
    res::create<res::Image>("deg_color_shading", info);
    res::create<res::Image>("deg_chromatic_aberration", info);
    res::create<res::Image>("deg_vignetting", info);
    res::create<res::Image>("deg_black_level", info);
    res::create<res::Image>("deg_dead_pixel", info);
    res::create<res::Image>("deg_output", info);
}

void Project::onUIRender() {

    ImGui::SetNextWindowSize({500, 750}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Camera setup")) {
        if (ImGui::CollapsingHeader("White balance error", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Color temperature (K)", &_colorTemperature, 2500.0f, 10000.0f, "%.0f K"))
                _shouldReprocess = true;
        }

        if (ImGui::CollapsingHeader("Barrel lens distortion", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Barrel distortion coefficients");
            if (ImGui::SliderFloat("k1", &_barrelDistortionCoeffs[0], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("k2", &_barrelDistortionCoeffs[1], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("k3", &_barrelDistortionCoeffs[2], -1.0f, 1.0f))
                _shouldReprocess = true;
        }

        if (ImGui::CollapsingHeader("Color shading error", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Color shading coefficients");
            for (size_t i = 0; i < _colorShadingError.size(); i++)
                if (ImGui::SliderFloat3(std::to_string(i).c_str(), &_colorShadingError[i].x, 0.5f, 1.5f))
                    _shouldReprocess = true;
        }

        if (ImGui::CollapsingHeader("Chromatic aberration", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Chromatic aberration coefficients");
            if (ImGui::SliderFloat("a (R)", &_chromaticAberrationCoeffsR[0], -0.02f, 0.02f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("b (R)", &_chromaticAberrationCoeffsR[1], -0.02f, 0.02f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("a (B)", &_chromaticAberrationCoeffsB[0], -0.02f, 0.02f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("b (B)", &_chromaticAberrationCoeffsB[1], -0.02f, 0.02f))
                _shouldReprocess = true;
        }

        if (ImGui::CollapsingHeader("Vignetting error", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Vignetting coefficients");
            if (ImGui::SliderFloat("a", &_vignettingCoeffs[0], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("b", &_vignettingCoeffs[1], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("c", &_vignettingCoeffs[2], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("d", &_vignettingCoeffs[3], -1.0f, 1.0f))
                _shouldReprocess = true;
            if (ImGui::SliderFloat("e", &_vignettingCoeffs[4], -1.0f, 1.0f))
                _shouldReprocess = true;
        }

        if (ImGui::CollapsingHeader("Black level offset", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            int blackLevelOffset = (int)_blackLevelOffset;
            if (ImGui::SliderInt("Black level offset##BLO", &blackLevelOffset, 0, 50)) {
                _blackLevelOffset = (uint8_t)blackLevelOffset;
                _shouldReprocess = true;
            }
        }

        if (ImGui::CollapsingHeader("Dead pixel injection", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            float percentDeadPixels = _percentDeadPixels * 100.0f;
            if (ImGui::SliderFloat("Percent of dead pixels", &percentDeadPixels, 0.0f, 1.0f, "%.2f%%")) {
                _percentDeadPixels = percentDeadPixels / 100.0f;
                _shouldReprocess = true;
            }
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize({1000, 750}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Pipeline")) {
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
        ImTextureID degWhiteBalanceImg = (ImTextureID)gfx::getImGuiImage("deg_white_balance");
        ImTextureID degLensImg = (ImTextureID)gfx::getImGuiImage("deg_lens");
        ImTextureID degColorShadingImg = (ImTextureID)gfx::getImGuiImage("deg_color_shading");
        ImTextureID degChromaticAberrationImg = (ImTextureID)gfx::getImGuiImage("deg_chromatic_aberration");
        ImTextureID degVignettingImg = (ImTextureID)gfx::getImGuiImage("deg_vignetting");
        ImTextureID degBlackLevelImg = (ImTextureID)gfx::getImGuiImage("deg_black_level");
        ImTextureID degDeadPixelImg = (ImTextureID)gfx::getImGuiImage("deg_dead_pixel");
        ImTextureID degOutputImg = (ImTextureID)gfx::getImGuiImage("deg_output");

        // Plot image degradation stages
        const ImPlotAxisFlags axisFlags = ImPlotAxisFlags_NoTickLabels;
        if (ImPlot::BeginPlot("Image pipeline", {-1, 350}, ImPlotFlags_Equal)) {
            ImPlot::SetupAxes(nullptr, nullptr, axisFlags, axisFlags);
            float x = 0.0f;
            ImPlot::PlotImage("Reference image", refImg, {x, 0}, {x + 1, ratio});
            ImPlot::PlotText("Reference image", x + 0.5f, ratio + 0.05f);
            x += 1.1f;
            ImPlot::PlotImage("White balance error", degWhiteBalanceImg, {x, 0}, {x + 1, ratio});
            ImPlot::PlotText("White balance error", x + 0.5f, ratio + 0.05f);
            x += 1.1f;
            ImPlot::PlotText("Lens distortion", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Lens distortion", degLensImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotText("Color shading error", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Color shading error", degColorShadingImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotText("Chromatic aberration", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Chromatic aberration", degChromaticAberrationImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotText("Vignetting", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Vignetting", degVignettingImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotText("Black level offset", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Black level offset", degBlackLevelImg, {x, 0}, {x + 1, ratio});
            x += 1.1f;
            ImPlot::PlotText("Dead pixel injection", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Dead pixel injection", degDeadPixelImg, {x, 0}, {x + 1, ratio});

            x += 1.3f;
            ImPlot::PlotText("Degraded image", x + 0.5f, ratio + 0.05f);
            ImPlot::PlotImage("Degraded image", degOutputImg, {x, 0}, {x + 1, ratio});
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
        atta::vec2 center(w / 2.0f, h / 2.0f);
        uint32_t ch = refImg->getChannels();

        res::Image* whiteBalanceImg = res::get<res::Image>("deg_white_balance");
        uint8_t* whiteBalanceData = whiteBalanceImg->getData();

        res::Image* lensImg = res::get<res::Image>("deg_lens");
        uint8_t* lensData = lensImg->getData();

        res::Image* colorShadingImg = res::get<res::Image>("deg_color_shading");
        uint8_t* colorShadingData = colorShadingImg->getData();

        res::Image* chromaticAberrationImg = res::get<res::Image>("deg_chromatic_aberration");
        uint8_t* chromaticAberrationData = chromaticAberrationImg->getData();

        res::Image* vignettingImg = res::get<res::Image>("deg_vignetting");
        uint8_t* vignettingData = vignettingImg->getData();

        res::Image* blackLevelImg = res::get<res::Image>("deg_black_level");
        uint8_t* blackLevelData = blackLevelImg->getData();

        res::Image* deadPixelImg = res::get<res::Image>("deg_dead_pixel");
        uint8_t* deadPixelData = deadPixelImg->getData();

        res::Image* outputImg = res::get<res::Image>("deg_output");
        uint8_t* outputData = outputImg->getData();

        // Load test image
        // refImg->load(testImgPath);
        // Resize images
        // blackLevelImg->resize(refImg->getWidth(), refImg->getHeight());
        // outputImg->resize(refImg->getWidth(), refImg->getHeight());

        //---------- Image degradation pipeline ----------//
        // White balance error
        degWhiteBalanceError(refData, whiteBalanceData, w, h, ch);
        whiteBalanceImg->update();

        // Barrel lens distortion
        degLensDistortion(whiteBalanceData, lensData, w, h, ch);
        lensImg->update();

        // Color shading error
        degColorShadingError(lensData, colorShadingData, w, h, ch);
        colorShadingImg->update();

        // Chromatic aberration
        degChromaticAberrationError(colorShadingData, chromaticAberrationData, w, h, ch);
        chromaticAberrationImg->update();

        // Vignetting error
        degVignettingError(chromaticAberrationData, vignettingData, w, h, ch);
        vignettingImg->update();

        // Black level offset
        degBlackLevelOffset(vignettingData, blackLevelData, w, h, ch);
        blackLevelImg->update();

        // Dead pixel injection
        degDeadPixelInjection(blackLevelData, deadPixelData, w, h, ch);
        deadPixelImg->update();

        // Output image
        for (uint32_t i = 0; i < w * h * ch; i++)
            outputData[i] = deadPixelData[i];
        outputImg->update();

        _shouldReprocess = false;
    }
}

void Project::degWhiteBalanceError(const uint8_t* refData, uint8_t* whiteBalanceData, uint32_t w, uint32_t h, uint32_t ch) const {
    for (uint32_t i = 0; i < w * h; i++) {
        // Get the RGB values for the current pixel
        uint8_t r = refData[i * ch];
        uint8_t g = refData[i * ch + 1];
        uint8_t b = refData[i * ch + 2];

        // Apply the temperature gain to each channel
        const atta::vec3 gains = tempToGain(_colorTemperature);
        whiteBalanceData[i * ch] = static_cast<uint8_t>(std::min(255.0f, r * gains.x));
        whiteBalanceData[i * ch + 1] = static_cast<uint8_t>(std::min(255.0f, g * gains.y));
        whiteBalanceData[i * ch + 2] = static_cast<uint8_t>(std::min(255.0f, b * gains.z));
    }
}

void Project::degLensDistortion(const uint8_t* refData, uint8_t* lensData, uint32_t w, uint32_t h, uint32_t ch) const {
    atta::vec2 center(w / 2.0f, h / 2.0f);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * ch;

            // Compute normalized radial distance
            atta::vec2 delta = atta::vec2(x, y) - center;
            float r = delta.length() / center.length();
            float r2 = r * r;
            float r4 = r2 * r2;

            // Compute barrel distortion polynomial (source radius)
            float lensR = r * (_barrelDistortionCoeffs[0] + _barrelDistortionCoeffs[1] * r2 + _barrelDistortionCoeffs[2] * r4);

            // Compute angle
            float angle = 0.0f;
            if (delta.squareLength() > 1e-5f)
                angle = std::atan2(delta.y, delta.x); // Avoid division by zero at the exact center

            // Compute source pixel coordinates
            float xDist = center.x + lensR * std::cos(angle) * center.length();
            float yDist = center.y + lensR * std::sin(angle) * center.length();

            // Sample distorted coordinate in source image
            atta::vec3 pixel = bilinearSampling(refData, w, h, ch, xDist, yDist);
            lensData[idx + 0] = static_cast<uint8_t>(pixel.x);
            lensData[idx + 1] = static_cast<uint8_t>(pixel.y);
            lensData[idx + 2] = static_cast<uint8_t>(pixel.z);
        }
    }
}

void Project::degColorShadingError(const uint8_t* refData, uint8_t* colorShadingData, uint32_t w, uint32_t h, uint32_t ch) const {
    atta::vec2 center(w / 2.0f, h / 2.0f);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * ch;

            // Compute normalized radial distance
            float r = (atta::vec2(x, y) - center).length() / center.length();

            // Compute color shading indices
            uint32_t gainIdx1 = static_cast<uint32_t>(r * (COLOR_SHADING_COUNT - 1));
            uint32_t gainIdx2 = gainIdx1 + 1;
            if (gainIdx2 >= COLOR_SHADING_COUNT)
                gainIdx2 = COLOR_SHADING_COUNT - 1;

            // Interpolate gain
            float t = r * (COLOR_SHADING_COUNT - 1) - static_cast<float>(gainIdx1);
            const atta::vec3& gain1 = _colorShadingError[gainIdx1];
            const atta::vec3& gain2 = _colorShadingError[gainIdx2];
            atta::vec3 gain = (1.0f - t) * gain1 + t * gain2;

            const uint8_t* inPix = &refData[idx];
            atta::vec3 pixel(inPix[0], inPix[1], inPix[2]);
            atta::vec3 shadedPixel = pixel * gain;

            // Save shaded pixel
            colorShadingData[idx] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.x));
            colorShadingData[idx + 1] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.y));
            colorShadingData[idx + 2] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.z));
        }
    }
}

void Project::degChromaticAberrationError(const uint8_t* refData, uint8_t* chromaticAberrationData, uint32_t w, uint32_t h, uint32_t ch) const {
    atta::vec2 center(w / 2.0f, h / 2.0f);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * ch;
            // Compute normalized radial distance
            atta::vec2 delta = atta::vec2(x, y) - center;
            float r = delta.length() / center.length();
            float r2 = r * r;
            float r3 = r2 * r;

            // Calculate chromatic aberration displacement for Red channel
            float displacementR = (_chromaticAberrationCoeffsR[0] * r2 + _chromaticAberrationCoeffsR[1] * r3);
            float sxR_float = center.x + delta.x * (1.0f + displacementR);
            float syR_float = center.y + delta.y * (1.0f + displacementR);

            // Calculate chromatic aberration displacement for Blue channel
            float displacementB = (_chromaticAberrationCoeffsB[0] * r2 + _chromaticAberrationCoeffsB[1] * r3);
            float sxB_float = center.x + delta.x * (1.0f + displacementB);
            float syB_float = center.y + delta.y * (1.0f + displacementB);

            // Sample from vignettingData (nearest neighbor sampling)
            // chromaticAberrationData[idx + 0] = (uint8_t)nearestNeighborSampling(colorShadingData, w, h, ch, sxR_float, syR_float).x;
            // chromaticAberrationData[idx + 1] = vignettingData[(y * w + x) * ch + 1];
            // chromaticAberrationData[idx + 2] = (uint8_t)nearestNeighborSampling(colorShadingData, w, h, ch, sxB_float, syB_float).z;

            // Sample from vignettingData (bilinear sampling)
            chromaticAberrationData[idx + 0] = (uint8_t)bilinearSampling(refData, w, h, ch, sxR_float, syR_float).x;
            chromaticAberrationData[idx + 1] = refData[(y * w + x) * ch + 1];
            chromaticAberrationData[idx + 2] = (uint8_t)bilinearSampling(refData, w, h, ch, sxB_float, syB_float).z;
        }
    }
}

void Project::degVignettingError(const uint8_t* refData, uint8_t* vignettingData, uint32_t w, uint32_t h, uint32_t ch) const {
    atta::vec2 center(w / 2.0f, h / 2.0f);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * ch;

            // Compute normalized radial distance
            float r = (atta::vec2(x, y) - center).length() / center.length();
            float r2 = r * r;
            float r3 = r2 * r;
            float r4 = r2 * r2;

            // Compute vignetting polynomial
            float vignetting =
                _vignettingCoeffs[0] * r4 + _vignettingCoeffs[1] * r3 + _vignettingCoeffs[2] * r2 + _vignettingCoeffs[3] * r + _vignettingCoeffs[4];

            // Apply vignetting to the pixel
            vignettingData[idx] = static_cast<uint8_t>(std::clamp(refData[idx] * vignetting, 0.0f, 255.0f));
            vignettingData[idx + 1] = static_cast<uint8_t>(std::clamp(refData[idx + 1] * vignetting, 0.0f, 255.0f));
            vignettingData[idx + 2] = static_cast<uint8_t>(std::clamp(refData[idx + 2] * vignetting, 0.0f, 255.0f));
        }
    }
}

void Project::degBlackLevelOffset(const uint8_t* refData, uint8_t* blackLevelData, uint32_t w, uint32_t h, uint32_t ch) const {
    for (uint32_t i = 0; i < w * h * ch; i++) {
        if (uint32_t(refData[i]) + _blackLevelOffset >= 255)
            blackLevelData[i] = 255;
        else
            blackLevelData[i] = refData[i] + _blackLevelOffset;
    }
}

void Project::degDeadPixelInjection(const uint8_t* refData, uint8_t* deadPixelData, uint32_t w, uint32_t h, uint32_t ch) {
    // Dead pixel injection (randomly set a channel to 0 - simulate photosite failure)
    std::mt19937 gen(42);                           // Random number generator
    std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution in [0, 1]

    _deadPixels.clear();
    for (uint32_t i = 0; i < w * h * ch; i++) {
        bool isDeadPixel = dis(gen) < _percentDeadPixels;
        if (isDeadPixel) {
            deadPixelData[i] = 0;
            _deadPixels.push_back(i); // This list should be generated during calibration in practice in practice
        } else {
            deadPixelData[i] = refData[i];
        }
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

    uint32_t index1 = static_cast<uint32_t>(fractionalIndex);
    uint32_t index2 = index1 + 1;

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

atta::vec3 Project::nearestNeighborSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y) {
    atta::vec3 result;

    // Convert to integer coordinates and clamp (Nearest Neighbor sampling)
    uint32_t sx = std::clamp(int(std::round(x)), 0, int(w) - 1);
    uint32_t sy = std::clamp(int(std::round(y)), 0, int(h) - 1);

    // Calculate source pixel index
    uint32_t srcIdx = (sy * w + sx) * ch;

    // Sample from source image
    result[0] = data[srcIdx + 0];
    result[1] = data[srcIdx + 1];
    result[2] = data[srcIdx + 2];

    return result;
}

atta::vec3 Project::bilinearSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y) {
    // Determine the integer coordinates of the top-left pixel of the 2x2 grid
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Calculate fractional parts for interpolation
    float fx = x - static_cast<float>(x0);
    float fy = y - static_cast<float>(y0);

    // Helper lambda to get pixel value with clamping and conversion to float atta::vec3
    auto get_pixel = [&](int xi, int yi) {
        // Clamp coordinates to be within image bounds
        int clamped_x = std::clamp(xi, 0, static_cast<int>(w) - 1);
        int clamped_y = std::clamp(yi, 0, static_cast<int>(h) - 1);

        uint32_t idx = (clamped_y * w + clamped_x) * ch;

        // Assuming ch >= 3 for R, G, B
        return atta::vec3(static_cast<float>(data[idx + 0]), // R
                          static_cast<float>(data[idx + 1]), // G
                          static_cast<float>(data[idx + 2])  // B
        );
    };

    // Get the color values of the four surrounding pixels
    atta::vec3 q00 = get_pixel(x0, y0); // Top-left
    atta::vec3 q10 = get_pixel(x1, y0); // Top-right
    atta::vec3 q01 = get_pixel(x0, y1); // Bottom-left
    atta::vec3 q11 = get_pixel(x1, y1); // Bottom-right

    // Interpolate along the x-axis for the top row
    atta::vec3 p0 = q00 * (1.0f - fx) + q10 * fx;

    // Interpolate along the x-axis for the bottom row
    atta::vec3 p1 = q01 * (1.0f - fx) + q11 * fx;

    // Interpolate along the y-axis between the results of the x-interpolations
    atta::vec3 result = p0 * (1.0f - fy) + p1 * fy;

    return result;
}
