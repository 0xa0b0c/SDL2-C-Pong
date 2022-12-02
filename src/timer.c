#include "timer.h"

#include <SDL2/SDL.h>

void
timer_start(p_timer_t *timer)
{
	timer->start_ticks = SDL_GetTicks64();
}

uint64_t
timer_get_ticks(p_timer_t *timer)
{
	return SDL_GetTicks64() - timer->start_ticks;
}
