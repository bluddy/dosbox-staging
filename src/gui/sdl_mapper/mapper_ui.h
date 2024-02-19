#ifndef SDL_MAPPER_RENDERER_H
#define SDL_MAPPER_RENDERER_H

class MapperRenderer {
    MapperRenderer() = default;

    void DrawText(int32_t x, int32_t y, const char* text, const Rgb888& color);


}

#endif