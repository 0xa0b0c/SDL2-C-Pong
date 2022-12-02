#pragma once

#include <stdint.h>

typedef struct {
	uint64_t start_ticks;
} p_timer_t;

// Timer functions to manually cap fps.
void   timer_start(p_timer_t *);
uint64_t timer_get_ticks(p_timer_t *);
