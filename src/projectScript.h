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

    // Degradation pipeline
    void degWhiteBalanceError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void degLensDistortion(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void degColorShadingError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void degChromaticAberrationError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void degVignettingError(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void degBlackLevelOffset(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch);
    void degDeadPixelInjection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch);

    // Image processing pipeline
    void proDeadPixelCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proBlackLevelCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proVignettingCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proChromaticAberrationCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proColorShadingCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proLensCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proWhiteBalanceCorrection(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;
    void proWhiteBalanceCorrectionAuto(const uint8_t* inData, uint8_t* outData, uint32_t w, uint32_t h, uint32_t ch) const;

    static atta::vec3 nearestNeighborSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y);
    static atta::vec3 bilinearSampling(const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch, float x, float y);

    //----------  Image degradation pipeline setup ----------//
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

    //--- Barrel lens distortion ---//
    // Barrel distortion will be modeled as a simple polynomial that is dependent on the normalized radial distance
    // D(r) = r * (a + b⋅r^2 + c⋅r^4)

    std::array<float, 3> _barrelDistortionCoeffs = {0.7f, 0.3f, -0.1f}; // Coefficients for the barrel distortion polynomial (a, b, c)

    //--- Color shading error ---//
    static constexpr size_t COLOR_SHADING_COUNT = 10;
    // Assuming that the color shading error has rotation symmetry, we define the gains on a line from the center of the image to the corner
    static std::array<atta::vec3, COLOR_SHADING_COUNT> _colorShadingError;

    //--- Chromatic aberration error ---//
    // Chromatic aberration will be modeled as a simple polynomial that is dependent on the normalized radial distance
    // C(r) = a⋅r^2 + b⋅r^3
    // A different polynomial will be used for the red and blue channels (green channel will be the reference)
    std::array<float, 2> _chromaticAberrationCoeffsR = {0.006f, 0.003f};   // Chromatic aberration polynomial for the red channel
    std::array<float, 2> _chromaticAberrationCoeffsB = {-0.006f, -0.003f}; // Chromatic aberration polynomial for the blue channel

    //--- Vignetting error ---//
    // Vignetting will be modeled as a simply multiplier that is dependent on the normalized radial distance
    // V(r) = a⋅r^4 + b⋅r^3 + c⋅r^2 + d⋅r + e
    std::array<float, 5> _vignettingCoeffs = {-0.5f, 0.0f, 0.0f, -0.2f, 1.0f}; // Coefficients for the vignetting polynomial (a, b, c, d, e)

    //--- Black level offset ---//
    uint8_t _blackLevelOffset = 20;

    //--- Dead pixel injection ---//
    float _percentDeadPixels = 0.0001f; // 0.01% of dead pixels

    //---------- Image processing pipeline setup ----------//
    //--- Dead pixel correction ---//
    // There can be an offline process to detect pixels in which the intensity does not change over multiple frames, or that significantly deviates
    // from the neighboring pixels. Ideally, this should not be done for every frame, but during the calibration process or with a few selected
    // frames.
    //
    // A list of dead pixels should be generated during the dead pixel calibration process. The stored list can later be used during the dead pixel
    // correction process, which will interpolate the values of the neighboring pixels.
    std::vector<uint32_t> _deadPixels; // List of dead pixels in the image (index in the image buffer)

    //--- Black level correction ---//
    // The image sensor may have optical black (OB) pixels, in this case, we can just subtract the average value of the optical black pixels from the
    // image
    //
    // If that is not possible, it is also possible to perform calibration by measuring the average value of the pixels in a dark scene (or with
    // the lens cap on). Note that this will be less accurate over time because the black level may change depending on sensor temperature and
    // exposure time.
    //
    // For the sake of this implementation, we'll assume that the camera sensor has 10 optical black pixels. Gaussian noise will be
    // added to the black pixels during the degradation stage.
    std::array<atta::vec3, 10> _obPixels;

    //--- Vignetting correction ---//
    // The vignetting correction will be done by applying the inverse of the vignetting polynomial to the image. Since the vignetting effect is
    // determined by the lens/physical design, the vignetting calibration can be done once per camera design (or once for each camera during factory
    // calibration).

    //--- Chromatic aberration correction ---//
    // The chromatic aberration correction will be done by applying the inverse of the chromatic aberration polynomial to the image.
    // Since chromatic aberration is caused by the lens design, the CA correction profile can be calibrated once per lens design (or once for each
    // camera during factory calibration).

    //--- Color shading correction ---//
    // The color shading correction will be done by applying the inverse of the color shading polynomial to the image.
    // Since color shading is caused by the lens design, the color shading correction profile can be calibrated once per lens design (or once for
    // each camera during factory calibration).

    //--- Lens correction ---//
    // The lens correction will be done by applying the inverse of the lens distortion polynomial to the image.

    //--- White balance correction ---//
    // The white balance correction will be done by applying the inverse of the color temperature gain to the image.
};

ATTA_REGISTER_PROJECT_SCRIPT(Project)

#endif // PROJECT_SCRIPT_H
