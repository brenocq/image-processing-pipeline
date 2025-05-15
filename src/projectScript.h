//--------------------------------------------------
// Image Processing Pipeline
// projectScript.h
// Date: 2025-05-13
// By Breno Cunha Queiroz
//--------------------------------------------------
#ifndef PROJECT_SCRIPT_H
#define PROJECT_SCRIPT_H
#include <atta/script/projectScript.h>

class Project : public scr::ProjectScript {
  public:
    void onLoad() override;

    void onUIRender() override;

    void onAttaLoop() override;

  private:
    std::vector<std::string> _testImages;
    int _selectedImage = 2;
    bool _shouldReprocess = true;

    static atta::vec3 nearestNeighborSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y);
    static atta::vec3 bilinearSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y);

    //----- Image degradation pipeline -----//
    //--- Black level offset ---//
    uint8_t _blackLevelOffset = 20;

    //--- Dead pixel injection ---//
    float _percentDeadPixels = 0.001f; // 0.1% of pixels

    //--- White balance error ---//
    float _colorTemperature = 3500.0f; // Temperature in Kelvin

    // Number of color temperatures in the table (2500K to 10000K, step 500K)
    static constexpr size_t TEMPERATURE_GAIN_COUNT = 16;
    static constexpr float TEMPERATURE_GAIN_STEP = 500.0f;
    static constexpr float TEMPERATURE_GAIN_MIN = 2500.0f;
    static constexpr float TEMPERATURE_GAIN_MAX = TEMPERATURE_GAIN_MIN + TEMPERATURE_GAIN_STEP * TEMPERATURE_GAIN_STEP;
    // Approximate RGB scaling factors to apply to a 5500K-balanced linear RGB image
    static constexpr std::array<atta::vec3, TEMPERATURE_GAIN_COUNT> _temperatureGainMap = {
        // {R_gain, G_gain, B_gain} // Approximate Correlated Color Temperature (K)
        atta::vec3{1.67f, 1.0f, 0.58f}, // 2500K  (Very Warm)
        atta::vec3{1.46f, 1.0f, 0.71f}, // 3000K  (Warm Incandescent)
        atta::vec3{1.31f, 1.0f, 0.82f}, // 3500K
        atta::vec3{1.20f, 1.0f, 0.91f}, // 4000K  (Cool White Fluorescent)
        atta::vec3{1.11f, 1.0f, 0.98f}, // 4500K
        atta::vec3{1.05f, 1.0f, 1.03f}, // 5000K  (Horizon Daylight, D50)
        atta::vec3{1.0f, 1.0f, 1.0f},   // 5500K  (Mid-day Sunlight, Flash - Reference: No Cast)
        atta::vec3{0.96f, 1.0f, 1.07f}, // 6000K
        atta::vec3{0.92f, 1.0f, 1.14f}, // 6500K  (Standard Daylight, D65 - Common Display White Point)
        atta::vec3{0.89f, 1.0f, 1.20f}, // 7000K
        atta::vec3{0.86f, 1.0f, 1.25f}, // 7500K  (North Sky Daylight, D75)
        atta::vec3{0.84f, 1.0f, 1.30f}, // 8000K
        atta::vec3{0.82f, 1.0f, 1.35f}, // 8500K
        atta::vec3{0.80f, 1.0f, 1.39f}, // 9000K
        atta::vec3{0.79f, 1.0f, 1.43f}, // 9500K
        atta::vec3{0.78f, 1.0f, 1.47f}  // 10000K (Clear Blue Sky)
    };

    // Given the temperature in kelvin, use the color temperature table to compute the corresponding gain
    static atta::vec3 tempToGain(float temp);

    //--- Color shading error ---//
    static constexpr size_t COLOR_SHADING_COUNT = 10;
    // Assuming that the color shading error has rotation symmetry, we define the gains on a line from the center of the image to the corner
    static constexpr std::array<atta::vec3, COLOR_SHADING_COUNT> _colorShadingError = {
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

    //--- Vignetting error ---//
    // Vignetting will be modeled as a simply multiplier that is dependent on the normalized radial distance
    // V(r) = a⋅r^4 + b⋅r^3 + c⋅r^2 + d⋅r + e
    std::array<float, 5> _vignettingCoeffs = {-0.5f, 0.0f, 0.0f, -0.2f, 1.0f}; // Coefficients for the vignetting polynomial (a, b, c, d, e)

    //--- Chromatic aberration error ---//
    // Chromatic aberration will be modeled as a simple polynomial that is dependent on the normalized radial distance
    // C(r) = a⋅r^2 + b⋅r^3
    // A different polynomial will be used for the red and blue channels (green channel will be the reference)
    std::array<float, 2> _chromaticAberrationCoeffsR = {0.006f, 0.003f};   // Chromatic aberration polynomial for the red channel
    std::array<float, 2> _chromaticAberrationCoeffsB = {-0.006f, -0.003f}; // Chromatic aberration polynomial for the blue channel

    //--- Barrel lens distortion ---//
    // Barrel distortion will be modeled as a simple polynomial that is dependent on the normalized radial distance
    // D(r) = r * (a + b⋅r^2 + c⋅r^4)
    std::array<float, 3> _barrelDistortionCoeffs = {0.7f, 0.3f, -0.1f}; // Coefficients for the barrel distortion polynomial (a, b, c)
};

ATTA_REGISTER_PROJECT_SCRIPT(Project)

#endif // PROJECT_SCRIPT_H
