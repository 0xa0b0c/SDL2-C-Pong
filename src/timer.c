#include "timer.h"

void
timer_start(p_timer_t *timer)
{
	timer->start_ticks = SDL_GetTicks64();
}

Uint64
timer_get_ticks(p_timer_t *timer)
{
	return SDL_GetTicks64() - timer->start_ticks;
}