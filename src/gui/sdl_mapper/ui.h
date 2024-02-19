#ifndef SDL_MAPPER_UI_H
#define SDL_MAPPER_UI_H

class MapperUI {
    MapperUI() = default;

    void DrawButtons() const;
    void CreateLayout();
    void MappingEvents();
    void Display();

};

#endif