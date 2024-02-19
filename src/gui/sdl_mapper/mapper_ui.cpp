#include <sdl>

#include "mapper_renderer.h"

void MapperRenderer::DrawText(int32_t x, int32_t y, const char* text, const Rgb888& color)
{
	SDL_Rect character_rect = {0, 0, 8, 14};
	SDL_Rect dest_rect      = {x, y, 8, 14};
	SDL_SetTextureColorMod(mapper.font_atlas, color.red, color.green, color.blue);
	while (*text) {
		character_rect.y = *text * character_rect.h;
		SDL_RenderCopy(mapper.renderer,
		               mapper.font_atlas,
		               &character_rect,
		               &dest_rect);
		text++;
		dest_rect.x += character_rect.w;
	}
}
