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
    // Black level offset
    uint8_t _blackLevelOffset = 20;
};

ATTA_REGISTER_PROJECT_SCRIPT(Project)
#endif // PROJECT_SCRIPT_H
