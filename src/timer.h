#pragma once

#include <SDL2/SDL.h>

typedef struct {
	Uint64 start_ticks;
} p_timer_t;

// Timer functions to manually cap fps.
void   timer_start(p_timer_t *);
Uint64 timer_get_ticks(p_timer_t *);