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

    // Image processing pipeline
    res::create<res::Image>("pro_dead_pixel", info);
    res::create<res::Image>("pro_black_level", info);
    res::create<res::Image>("pro_vignetting", info);
    res::create<res::Image>("pro_chromatic_aberration", info);
    res::create<res::Image>("pro_color_shading", info);
    res::create<res::Image>("pro_lens", info);
    res::create<res::Image>("pro_white_balance", info);
    res::create<res::Image>("pro_output", info);
}

void plotImage(const char* label, ImTextureID img, float x, float y, float w, float h) {
    ImPlot::PlotImage(label, img, {x, y}, {x + w, y + h});
    ImPlot::PlotText(label, x + 0.5f, y + h + 0.05f);
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

        ImTextureID proDeadPixelImg = (ImTextureID)gfx::getImGuiImage("pro_dead_pixel");
        ImTextureID proBlackLevelImg = (ImTextureID)gfx::getImGuiImage("pro_black_level");
        ImTextureID proOutputImg = (ImTextureID)gfx::getImGuiImage("pro_output");

        // Plot image degradation stages
        const ImPlotAxisFlags axisFlags = ImPlotAxisFlags_NoTickLabels;
        if (ImPlot::BeginPlot("Image pipeline", {-1, 350}, ImPlotFlags_Equal)) {
            ImPlot::SetupAxes(nullptr, nullptr, axisFlags, axisFlags);
            float x = 0.0f;
            float y = 0.0f;

            plotImage("Reference image", refImg, x, y, 1.0f, ratio);
            x += 1.5f;
            plotImage("Degraded image", degOutputImg, x, y, 1.0f, ratio);
            x += 1.5f;
            plotImage("Processed image", proOutputImg, x, y, 1.0f, ratio);

            // Plot degradation stages
            y -= 1.5f;
            x = 0.0f;

            plotImage("White balance error", degWhiteBalanceImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Lens distortion", degLensImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Color shading error", degColorShadingImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Chromatic aberration", degChromaticAberrationImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Vignetting", degVignettingImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Black level offset", degBlackLevelImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Dead pixel injection", degDeadPixelImg, x, y, 1.0f, ratio);

            // Plot image processing stages
            y -= 1.5f;
            x = 0.0f;

            plotImage("Dead pixel correction", proDeadPixelImg, x, y, 1.0f, ratio);
            x += 1.1f;
            plotImage("Black level correction", proBlackLevelImg, x, y, 1.0f, ratio);
            x += 1.1f;

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

        // Load test image
        // refImg->load(testImgPath);
        // Resize images
        // blackLevelImg->resize(refImg->getWidth(), refImg->getHeight());
        // outputImg->resize(refImg->getWidth(), refImg->getHeight());

        //---------- Image degradation pipeline ----------//
        // White balance error
        res::Image* whiteBalanceImg = res::get<res::Image>("deg_white_balance");
        uint8_t* whiteBalanceData = whiteBalanceImg->getData();
        degWhiteBalanceError(refData, whiteBalanceData, w, h, ch);
        whiteBalanceImg->update();

        // Barrel lens distortion
        res::Image* lensImg = res::get<res::Image>("deg_lens");
        uint8_t* lensData = lensImg->getData();
        degLensDistortion(whiteBalanceData, lensData, w, h, ch);
        lensImg->update();

        // Color shading error
        res::Image* colorShadingImg = res::get<res::Image>("deg_color_shading");
        uint8_t* colorShadingData = colorShadingImg->getData();
        degColorShadingError(lensData, colorShadingData, w, h, ch);
        colorShadingImg->update();

        // Chromatic aberration
        res::Image* chromaticAberrationImg = res::get<res::Image>("deg_chromatic_aberration");
        uint8_t* chromaticAberrationData = chromaticAberrationImg->getData();
        degChromaticAberrationError(colorShadingData, chromaticAberrationData, w, h, ch);
        chromaticAberrationImg->update();

        // Vignetting error
        res::Image* vignettingImg = res::get<res::Image>("deg_vignetting");
        uint8_t* vignettingData = vignettingImg->getData();
        degVignettingError(chromaticAberrationData, vignettingData, w, h, ch);
        vignettingImg->update();

        // Black level offset
        res::Image* blackLevelImg = res::get<res::Image>("deg_black_level");
        uint8_t* blackLevelData = blackLevelImg->getData();
        degBlackLevelOffset(vignettingData, blackLevelData, w, h, ch);
        blackLevelImg->update();

        // Dead pixel injection
        res::Image* deadPixelImg = res::get<res::Image>("deg_dead_pixel");
        uint8_t* deadPixelData = deadPixelImg->getData();
        degDeadPixelInjection(blackLevelData, deadPixelData, w, h, ch);
        deadPixelImg->update();

        // Degradation output image
        res::Image* outputImg = res::get<res::Image>("deg_output");
        uint8_t* outputData = outputImg->getData();
        for (uint32_t i = 0; i < w * h * ch; i++)
            outputData[i] = deadPixelData[i];
        outputImg->update();

        //---------- Image processing pipeline ----------//
        // Dead pixel correction
        res::Image* proDeadPixelImg = res::get<res::Image>("pro_dead_pixel");
        uint8_t* proDeadPixelData = proDeadPixelImg->getData();
        proDeadPixelCorrection(outputData, proDeadPixelData, w, h, ch);
        proDeadPixelImg->update();

        // Black level correction
        res::Image* proBlackLevelImg = res::get<res::Image>("pro_black_level");
        uint8_t* proBlackLevelData = proBlackLevelImg->getData();
        proBlackLevelCorrection(proDeadPixelData, proBlackLevelData, w, h, ch);
        proBlackLevelImg->update();

        // Processed output
        res::Image* proOutputImg = res::get<res::Image>("pro_output");
        uint8_t* proOutputData = proOutputImg->getData();
        for (uint32_t i = 0; i < w * h * ch; i++)
            proOutputData[i] = proBlackLevelData[i];
        proOutputImg->update();

        _shouldReprocess = false;
    }
}

void Project::degWhiteBalanceError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
    for (uint32_t i = 0; i < w * h; i++) {
        // Get the RGB values for the current pixel
        uint8_t r = inData[i * ch];
        uint8_t g = inData[i * ch + 1];
        uint8_t b = inData[i * ch + 2];

        // Apply the temperature gain to each channel
        const atta::vec3 gains = tempToGain(_colorTemperature);
        outData[i * ch] = static_cast<uint8_t>(std::min(255.0f, r * gains.x));
        outData[i * ch + 1] = static_cast<uint8_t>(std::min(255.0f, g * gains.y));
        outData[i * ch + 2] = static_cast<uint8_t>(std::min(255.0f, b * gains.z));
    }
}

void Project::degLensDistortion(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
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
            atta::vec3 pixel = bilinearSampling(inData, w, h, ch, xDist, yDist);
            outData[idx + 0] = static_cast<uint8_t>(pixel.x);
            outData[idx + 1] = static_cast<uint8_t>(pixel.y);
            outData[idx + 2] = static_cast<uint8_t>(pixel.z);
        }
    }
}

void Project::degColorShadingError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
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

            const uint8_t* inPix = &inData[idx];
            atta::vec3 pixel(inPix[0], inPix[1], inPix[2]);
            atta::vec3 shadedPixel = pixel * gain;

            // Save shaded pixel
            outData[idx] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.x));
            outData[idx + 1] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.y));
            outData[idx + 2] = static_cast<uint8_t>(std::min(255.0f, shadedPixel.z));
        }
    }
}

void Project::degChromaticAberrationError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
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
            // outData[idx + 0] = (uint8_t)nearestNeighborSampling(inData, w, h, ch, sxR_float, syR_float).x;
            // outData[idx + 1] = inData[(y * w + x) * ch + 1];
            // outData[idx + 2] = (uint8_t)nearestNeighborSampling(inData, w, h, ch, sxB_float, syB_float).z;

            // Sample from vignettingData (bilinear sampling)
            outData[idx + 0] = (uint8_t)bilinearSampling(inData, w, h, ch, sxR_float, syR_float).x;
            outData[idx + 1] = inData[(y * w + x) * ch + 1];
            outData[idx + 2] = (uint8_t)bilinearSampling(inData, w, h, ch, sxB_float, syB_float).z;
        }
    }
}

void Project::degVignettingError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
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
            outData[idx] = static_cast<uint8_t>(std::clamp(inData[idx] * vignetting, 0.0f, 255.0f));
            outData[idx + 1] = static_cast<uint8_t>(std::clamp(inData[idx + 1] * vignetting, 0.0f, 255.0f));
            outData[idx + 2] = static_cast<uint8_t>(std::clamp(inData[idx + 2] * vignetting, 0.0f, 255.0f));
        }
    }
}

void Project::degBlackLevelOffset(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) {
    // Apply black level offset
    for (uint32_t i = 0; i < w * h * ch; i++) {
        if (uint32_t(inData[i]) + _blackLevelOffset >= 255)
            outData[i] = 255;
        else
            outData[i] = inData[i] + _blackLevelOffset;
    }

    // Generate optical black pixel measurements
    std::default_random_engine gen(42);
    std::normal_distribution<float> dist(0.0f, 5.0f); // Gaussian distribution with mean 0 and stddev 5.0
    for (size_t i = 0; i < _obPixels.size(); i++) {
        // Generate perfect measurement
        atta::vec3 obPixel(_blackLevelOffset, _blackLevelOffset, _blackLevelOffset);

        // Add Gaussian noise to each channel
        for (uint32_t c = 0; c < ch; c++)
            obPixel[c] = std::round(std::clamp(obPixel[c] + dist(gen), 0.0f, 255.0f));

        _obPixels[i] = obPixel;
    }
}

void Project::degDeadPixelInjection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) {
    // Dead pixel injection (randomly set a channel to 0 - simulate photosite failure)
    std::mt19937 gen(42);                           // Random number generator
    std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution in [0, 1]

    _deadPixels.clear();
    for (uint32_t i = 0; i < w * h * ch; i++) {
        bool isDeadPixel = dis(gen) < _percentDeadPixels;
        if (isDeadPixel) {
            outData[i] = 0;
            _deadPixels.push_back(i); // This list should be generated during calibration in practice in practice
        } else {
            outData[i] = inData[i];
        }
    }
}

void Project::proDeadPixelCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
    // Copy input data to output data
    for (uint32_t i = 0; i < w * h * ch; i++)
        outData[i] = inData[i];

    // Dead pixel correction (nearest neighbor sampling)
    for (uint32_t i = 0; i < _deadPixels.size(); i++) {
        uint32_t idx = _deadPixels[i];
        uint32_t sum = 0;
        uint32_t count = 0;

        // TODO should not use neighbor if the neighbor is also a dead pixel
        if (idx >= ch) {
            sum += inData[idx - ch];
            count++;
        }
        if (idx + ch < w * h * ch) {
            sum += inData[idx + ch];
            count++;
        }
        if (idx >= ch * w) {
            sum += inData[idx - ch * w];
            count++;
        }
        if (idx + ch * w < w * h * ch) {
            sum += inData[idx + ch * w];
            count++;
        }

        // Average of 4 neighbors
        outData[idx] = sum / count;
    }
}

void Project::proBlackLevelCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const {
    // Copy input data to output data
    for (uint32_t i = 0; i < w * h * ch; i++)
        outData[i] = inData[i];

    // Compute black level from optical black pixels
    uint32_t blackLevelSum = 0;
    for (size_t i = 0; i < _obPixels.size(); i++) {
        // Get the optical black pixel value
        const atta::vec3& obPixel = _obPixels[i];
        // Sum channel values
        blackLevelSum += static_cast<uint32_t>(obPixel.x + obPixel.y + obPixel.z);
    }
    uint8_t blackLevel = blackLevelSum / (3 * _obPixels.size());

    // Black level correction
    for (uint32_t i = 0; i < w * h * ch; i++) {
        if (outData[i] >= blackLevel)
            outData[i] -= blackLevel;
        else
            outData[i] = 0;
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
