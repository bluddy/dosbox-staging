#ifndef SDL_MAPPER_UI_H
#define SDL_MAPPER_UI_H

#include <vector>
#include <memory>

class CButton;

class MapperUI {
    // Class for the mapper's binding UI
    MapperUI() = default;

    void DrawButtons() const;
    void CreateLayout();
    void MappingEvents();
    void Display();

private:
    std::vector<std::unique_ptr<CButton>> buttons;

};

#endif