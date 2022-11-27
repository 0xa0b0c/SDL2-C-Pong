#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "config.h"
#include "debug.h"

// Constants.
#define NUM_PADDLES 2
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define PADDLE_MOVE_FACTOR 1
#define BALL_WIDTH 10
#define BALL_HEIGHT 10

// SDL Stuff.
extern bool subystem_init(void);
extern void subsystem_close(void);

// Types.
typedef enum {
	GAME_STATUS_MAIN_MENU,
	GAME_STATUS_PLAYING,
	GAME_STATUS_PAUSED
} game_status_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
	int dy;
} paddle_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
	int dx;
	int dy;
} ball_t;

typedef struct {
	Uint64 start_ticks;
} p_timer_t;

typedef enum {
	PAD_GO_UP,
	PAD_GO_DOWN
} pad_direction;

// Global Variables.
static SDL_Window   *g_window = 0;
static SDL_Renderer *g_renderer = 0;
static SDL_Texture  *g_tex_startup_menu = 0;
static SDL_Texture  *g_tex_pause_menu = 0;
static int           g_tex_startup_menu_width = 0;
static int           g_tex_startup_menu_height = 0;
static int           g_tex_pause_menu_width = 0;
static int           g_tex_pause_menu_height = 0;
static game_status_t g_game_status = GAME_STATUS_MAIN_MENU;
static paddle_t      g_paddles[NUM_PADDLES];
static ball_t        g_ball;

// Game Functions.
static bool game_init(void);
static void game_close(void);
// @TODO(lev): this could be refactored!
static bool game_load_main_menu(void);
static bool game_load_pause_menu(void);
static void game_draw_menu(void);
static void game_draw_pause_menu(void);
//
static void game_loop(void);
static void game_draw_paddles(void);
static void game_draw_ball(void);
static void game_draw_net(void);
static void game_handle_input(SDL_Event *, bool *);
static void game_handle_input_main_menu(SDL_Event *);
static void game_handle_input_playing(SDL_Event *);
static void game_handle_input_paused(SDL_Event *);
static void game_render(void);
static void game_update(void);
static void game_set_initial_positions(void);
static void game_update_player_pad(pad_direction);

// @TODO(lev): move somewhere else and declare extern.
// Timer functions to manually cap fps.
static void   timer_start(p_timer_t *);
static Uint64 timer_get_ticks(p_timer_t *);

int
main(void)
{
	atexit(subsystem_close);
	atexit(game_close);

	if (!subystem_init())
	{
		(void)fprintf(stdout, "Could not initialize game's subystems.\n");
		exit(EXIT_FAILURE);
	}

	if (!game_init())
	{
		(void)fprintf(stdout, "Could not initialize game.\n");
		exit(EXIT_FAILURE);
	}

	if (!game_load_main_menu())
	{
		(void)fprintf(stdout, "Could not load main menu.\n");
		exit(EXIT_FAILURE);
	}

	if (!game_load_pause_menu())
	{
		(void)fprintf(stdout, "Could not load pause menu.\n");
		exit(EXIT_FAILURE);
	}

	game_loop();

	return EXIT_SUCCESS;
}

static bool
game_init(void)
{
	if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT,
									SDL_WINDOW_MAXIMIZED, &g_window,
									&g_renderer) < 0)
	{
		debug_print("Could not create SDL window/renderer: %s.\n", SDL_GetError());
		return false;
	}

	game_set_initial_positions();

	return true;
}

static void
game_close(void)
{
	SDL_DestroyTexture(g_tex_pause_menu);
	g_tex_pause_menu = 0;

	SDL_DestroyTexture(g_tex_startup_menu);
	g_tex_startup_menu = 0;

	SDL_DestroyRenderer(g_renderer);
	g_renderer = 0;

	SDL_DestroyWindow(g_window);
	g_window = 0;
}

static bool
game_load_main_menu(void)
{
	SDL_Surface *s = IMG_Load("menu.png");

	if (!s)
	{
		debug_print("Could not load menu: %s.\n", IMG_GetError());
		return false;
	}

	g_tex_startup_menu = SDL_CreateTextureFromSurface(g_renderer, s);

	SDL_QueryTexture(g_tex_startup_menu, 0, 0, &g_tex_startup_menu_width, &g_tex_startup_menu_height);

	SDL_FreeSurface(s);

	return g_tex_startup_menu != 0;
}

static bool
game_load_pause_menu(void)
{
	SDL_Surface *s = IMG_Load("pause_menu.png");

	if (!s)
	{
		debug_print("Could not pause menu: %s.\n", IMG_GetError());
		return false;
	}

	g_tex_pause_menu = SDL_CreateTextureFromSurface(g_renderer, s);

	SDL_QueryTexture(g_tex_pause_menu, 0, 0, &g_tex_pause_menu_width, &g_tex_pause_menu_height);

	SDL_FreeSurface(s);

	return g_tex_pause_menu != 0;
}

static void
game_loop(void)
{
	int current_frames = 0;
	bool quit = false;
	SDL_Event event;
	p_timer_t fps_timer;
	p_timer_t cap_timer;

	timer_start(&fps_timer);
	while (!quit)
	{
		timer_start(&cap_timer);

		game_handle_input(&event, &quit);

		float avg_fps = current_frames / (timer_get_ticks(&fps_timer) / 1000.f);

		if (avg_fps > 2000000)
		{
			avg_fps = 0;
		}

		if (!quit)
		{
			game_render();
		}

		++current_frames;

		if (timer_get_ticks(&cap_timer) < WINDOW_TICKS_PER_FRAME)
		{
			SDL_Delay(WINDOW_TICKS_PER_FRAME - timer_get_ticks(&cap_timer));
		}
	}
}

static void
game_draw_menu(void)
{
	SDL_Rect render_quad = {0, 0, g_tex_startup_menu_width, g_tex_startup_menu_height};
	SDL_RenderCopy(g_renderer, g_tex_startup_menu, 0, &render_quad);
}

static void
game_handle_input(SDL_Event *event, bool *quit)
{
	while (SDL_PollEvent(event))
	{
		if (event->type == SDL_QUIT)
		{
			*quit = true;
		}

		if (*quit == false)
		{
			switch (g_game_status)
			{
			case GAME_STATUS_MAIN_MENU:
				game_handle_input_main_menu(event);
				break;
			case GAME_STATUS_PAUSED:
				game_handle_input_paused(event);
				break;
			case GAME_STATUS_PLAYING:
				game_handle_input_playing(event);
				break;
			}
		}
	}
}

static void
game_render(void)
{
	SDL_RenderClear(g_renderer);

	switch (g_game_status)
	{
	case GAME_STATUS_MAIN_MENU:
		SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
		game_draw_menu();
		SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0x00);
		break;
	case GAME_STATUS_PAUSED:
		SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
		game_draw_pause_menu();
		SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0x00);
		break;
	case GAME_STATUS_PLAYING:
		game_update();
		SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
		game_draw_paddles();
		game_draw_ball();
		game_draw_net();
		SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0x00);
		break;
	}

	SDL_RenderPresent(g_renderer);
}

static void
game_draw_paddles(void)
{
	SDL_Rect src;

	for (size_t i = 0; i < NUM_PADDLES; ++i)
	{
		src.x = g_paddles[i].x;
		src.y = g_paddles[i].y;
		src.w = g_paddles[i].w;
		src.h = g_paddles[i].h;

		if (SDL_RenderFillRect(g_renderer, &src) < 0)
		{
			debug_print("Could not draw paddle: %s\n", SDL_GetError());
		}
	}
}

static void
game_draw_ball(void)
{
	SDL_Rect src;

	src.x = g_ball.x;
	src.y = g_ball.y;
	src.w = g_ball.w;
	src.h = g_ball.h;

	if (SDL_RenderFillRect(g_renderer, &src) < 0)
	{
		debug_print("Could not draw ball: %s\n", SDL_GetError());
	}
}

static void
game_draw_net(void)
{
	for (int i = 0; i < WINDOW_HEIGHT; i += 4)
	{
		SDL_RenderDrawPoint(g_renderer, WINDOW_WIDTH / 2, i);
	}
}

static void
game_update(void)
{
	g_paddles[0].y += g_paddles[0].dy;

	// Clamp player's pad.
	if (g_paddles[0].y <= 0)
	{
		g_paddles[0].y = 0;
	}
	else if (g_paddles[0].y + g_paddles[0].h > WINDOW_HEIGHT)
	{
		g_paddles[0].y = WINDOW_HEIGHT - g_paddles[0].h;
	}

	g_ball.x += g_ball.dx;
	g_ball.y += g_ball.dy;

	if (g_ball.x - g_ball.w < 0)
	{
		game_set_initial_positions();
	}

	if (g_ball.x > WINDOW_WIDTH - g_ball.w)
	{
		game_set_initial_positions();
	}

	if (g_ball.y < 0 || g_ball.y > WINDOW_HEIGHT - g_ball.h)
	{
		g_ball.dy = -g_ball.dy;
	}
}

static void
timer_start(p_timer_t *timer)
{
	timer->start_ticks = SDL_GetTicks64();
}

static Uint64
timer_get_ticks(p_timer_t *timer)
{
	return SDL_GetTicks64() - timer->start_ticks;
}

static void
game_set_initial_positions(void)
{
	{
		g_paddles[0].x = 0;
		g_paddles[0].y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT;
		g_paddles[0].w = PADDLE_WIDTH;
		g_paddles[0].h = PADDLE_HEIGHT;
		g_paddles[0].dy = PADDLE_MOVE_FACTOR;
	}

	{
		g_paddles[1].x = WINDOW_WIDTH - PADDLE_WIDTH;
		g_paddles[1].y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT;
		g_paddles[1].w = PADDLE_WIDTH;
		g_paddles[1].h = PADDLE_HEIGHT;
		g_paddles[1].dy = PADDLE_MOVE_FACTOR; // it doesn't really need it
	}

	{
		g_ball.x = WINDOW_WIDTH / 2 - BALL_WIDTH;
		g_ball.y = WINDOW_HEIGHT / 2 - BALL_HEIGHT;
		g_ball.w = BALL_WIDTH;
		g_ball.h = BALL_HEIGHT;
		g_ball.dx = 1;
		g_ball.dy = 1;
	}
}

static void
game_handle_input_main_menu(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_RETURN)
	{
		g_game_status = GAME_STATUS_PLAYING;
	}
}

static void
game_handle_input_playing(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN)
	{
		switch (event->key.keysym.sym)
		{
		case SDLK_UP:
			game_update_player_pad(PAD_GO_UP);
			break;
		case SDLK_DOWN:
			game_update_player_pad(PAD_GO_DOWN);
			break;
		case SDLK_ESCAPE:
			g_game_status = GAME_STATUS_PAUSED;
			break;
		}
	}
}

static void
game_update_player_pad(pad_direction direction)
{
	if (direction == PAD_GO_UP)
	{
		g_paddles[0].dy = -g_paddles[0].dy;
	}
	else
	{
		g_paddles[0].dy = abs(g_paddles[0].dy);
	}
}

static void
game_handle_input_paused(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
	{
		g_game_status = GAME_STATUS_PLAYING;
	}
}

static void
game_draw_pause_menu(void)
{
	SDL_Rect render_quad = {0, 0, g_tex_pause_menu_width, g_tex_pause_menu_height};
	SDL_RenderCopy(g_renderer, g_tex_pause_menu, 0, &render_quad);
}