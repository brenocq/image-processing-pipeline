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
    int _selectedImage = 0;
    bool _shouldReprocess = true;

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
};

ATTA_REGISTER_PROJECT_SCRIPT(Project)
#endif // PROJECT_SCRIPT_H
