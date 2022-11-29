#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "config.h"
#include "debug.h"
#include "timer.h"

// Constants.
#define PADDLE_COUNT 2
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define PADDLE_HUMAN_SPEED 2
#define PADDLE_CPU_SPEED 1
#define PADDLE_HUMAN_INDEX 0
#define PADDLE_AI_INDEX 1

#define BALL_WIDTH 10
#define BALL_HEIGHT 10
#define BALL_SPEED 2

#define SCOREBOARD_FONT_SIZE 28
#define SCOREBOARD_TEXT_SIZE 32
#define SCOREBOARD_LIMIT 5

// SDL Stuff.
extern bool subystem_init(void);
extern void subsystem_close(void);

enum {
	TEXTURE_STARTUP_MENU_INDEX,
	TEXTURE_PAUSE_MENU_INDEX,
	TEXTURE_GAME_OVER_INDEX,
	TEXTURE_GAME_WON_INDEX,
	TEXTURE_SCOREBOARD_HUMAN,
	TEXTURE_SCOREBOARD_AI,

	TEXTURE_COUNT
} game_textures_indices;

// Types.
typedef enum {
	GAME_STATUS_MAIN_MENU,
	GAME_STATUS_PLAYING,
	GAME_STATUS_PAUSED,
	GAME_STATUS_GAME_OVER,
	GAME_STATUS_GAME_WON
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

typedef enum {
	PAD_GO_UP,
	PAD_GO_DOWN
} pad_direction_t;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} texture_t;

// Global Variables.
static SDL_Window   *g_window = 0;
static SDL_Renderer *g_renderer = 0;
static texture_t    *g_textures[TEXTURE_COUNT];
static game_status_t g_game_status = GAME_STATUS_MAIN_MENU;
static paddle_t      g_paddles[PADDLE_COUNT];
static ball_t        g_ball;
static int           g_scores[PADDLE_COUNT];
static TTF_Font     *g_font;

// Game Functions.
static bool game_init(void);
static void game_close(void);
static bool game_load_texture_from_img(texture_t **, const char *);
static void game_loop(void);

static void game_draw_texture(texture_t *);
static void game_draw_paddles(void);
static void game_draw_ball(void);
static void game_draw_net(void);
static void game_draw_scores(void);

static void game_handle_input(SDL_Event *, bool *);
static void game_handle_input_main_menu(SDL_Event *);
static void game_handle_input_playing(SDL_Event *);
static void game_handle_input_paused(SDL_Event *);
static void game_handle_input_game_ended(SDL_Event *);

static void game_render(void);
static void game_update(void);
static void game_set_initial_positions(void);
static void game_reset_scoreboard(void);
static void game_update_player_pad(pad_direction_t);
static bool game_ball_collision_with_paddle(paddle_t);
static bool game_load_texture_from_text(const char *, SDL_Color, texture_t **);
static void game_check_win_condition(void);

int
main(void)
{
	atexit(subsystem_close);
	atexit(game_close);

	if (!subystem_init())
	{
		exit(EXIT_FAILURE);
	}

	if (!game_init())
	{
		exit(EXIT_FAILURE);
	}

	game_loop();

	return EXIT_SUCCESS;
}

static bool
game_init(void)
{
	g_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_MAXIMIZED);

	if (!g_window)
	{
		(void)fprintf(stderr, "Could not create SDL window: %s.\n", SDL_GetError());
		return false;
	}

	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);

	if (!g_renderer)
	{
		(void)fprintf(stderr, "Could not create SDL renderer: %s.\n", SDL_GetError());
		return false;
	}

	g_font = TTF_OpenFont(FILEPATH_FONT, SCOREBOARD_FONT_SIZE);

	if (!g_font)
	{
		(void)fprintf(stderr, "Could not open TTF Font: %s.\n", TTF_GetError());
		return false;
	}

	for (size_t i = 0; i < TEXTURE_COUNT; ++i)
	{
		g_textures[i] = malloc(sizeof(texture_t));
		g_textures[i]->texture = 0;
	}

	if (!game_load_texture_from_img(&g_textures[TEXTURE_STARTUP_MENU_INDEX], FILEPATH_STARTUP_MENU_IMAGE))
	{
		(void)fprintf(stderr, "Could not load main menu.\n");
		return false;
	}

	if (!game_load_texture_from_img(&g_textures[TEXTURE_PAUSE_MENU_INDEX], FILEPATH_MENU_PAUSE_IMAGE))
	{
		(void)fprintf(stderr, "Could not load pause menu.\n");
		return false;
	}

	if (!game_load_texture_from_img(&g_textures[TEXTURE_GAME_OVER_INDEX], FILEPATH_GAME_OVER_IMAGE))
	{
		(void)fprintf(stderr, "Could not load pause menu.\n");
		return false;
	}

	if (!game_load_texture_from_img(&g_textures[TEXTURE_GAME_WON_INDEX], FILEPATH_GAME_WON_IMAGE))
	{
		(void)fprintf(stderr, "Could not load pause menu.\n");
		return false;
	}

	game_set_initial_positions();

	return true;
}

static void
game_close(void)
{
	TTF_CloseFont(g_font);
	g_font = 0;

	for (size_t i = 0; i < TEXTURE_COUNT; ++i)
	{
		SDL_DestroyTexture(g_textures[i]->texture);
		g_textures[i]->texture = 0;
		free(g_textures[i]);
	}

	SDL_DestroyRenderer(g_renderer);
	g_renderer = 0;

	SDL_DestroyWindow(g_window);
	g_window = 0;
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
			case GAME_STATUS_GAME_OVER:
			case GAME_STATUS_GAME_WON:
				game_handle_input_game_ended(event);
			}
		}
	}
}

static void
game_render(void)
{
	SDL_RenderClear(g_renderer);
	SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
	switch (g_game_status)
	{
	case GAME_STATUS_MAIN_MENU:
		game_draw_texture(g_textures[TEXTURE_STARTUP_MENU_INDEX]);
		break;
	case GAME_STATUS_PAUSED:
		game_draw_texture(g_textures[TEXTURE_PAUSE_MENU_INDEX]);
		break;
	case GAME_STATUS_PLAYING:
		game_update();
		game_draw_paddles();
		game_draw_ball();
		game_draw_net();
		game_draw_scores();
		break;
	case GAME_STATUS_GAME_OVER:
		game_draw_texture(g_textures[TEXTURE_GAME_OVER_INDEX]);
		break;
	case GAME_STATUS_GAME_WON:
		game_draw_texture(g_textures[TEXTURE_GAME_WON_INDEX]);
		break;
	}
	SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderPresent(g_renderer);
}

static void
game_draw_paddles(void)
{
	SDL_Rect src;

	for (size_t i = 0; i < PADDLE_COUNT; ++i)
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
	for (size_t i = 0; i < PADDLE_COUNT; ++i)
	{
		g_paddles[i].y += g_paddles[i].dy;

		if (g_paddles[i].y <= 0)
		{
			g_paddles[i].y = 0;
		}
		else if (g_paddles[i].y + g_paddles[i].h > WINDOW_HEIGHT)
		{
			g_paddles[i].y = WINDOW_HEIGHT - g_paddles[i].h;
		}
	}

	// Update AI.
	if (g_ball.y < g_paddles[1].y)
	{
		g_paddles[PADDLE_AI_INDEX].dy = -PADDLE_CPU_SPEED;
	}
	else
	{
		g_paddles[PADDLE_AI_INDEX].dy = PADDLE_CPU_SPEED;
	}

	g_ball.x += g_ball.dx;
	g_ball.y += g_ball.dy;

	// Check ball's collision with pads.
	for (size_t i = 0; i < PADDLE_COUNT; ++i)
	{
		if (game_ball_collision_with_paddle(g_paddles[i]))
		{
			// @TODO(lev): refactor.
			// Change direction.
			if (g_ball.dx < 0)
			{
				g_ball.dx = 1;
			}
			else
			{
				g_ball.dx = -1;
			}
		}
	}

	// CPU scored.
	if (g_ball.x < 0)
	{
		++g_scores[PADDLE_AI_INDEX];
		game_set_initial_positions();
	}

	// Human scored.
	if (g_ball.x > WINDOW_WIDTH - g_ball.w)
	{
		++g_scores[PADDLE_HUMAN_INDEX];
		game_set_initial_positions();
	}

	game_check_win_condition();

	// Bounce.
	if (g_ball.y < 0 || g_ball.y > WINDOW_HEIGHT - g_ball.h)
	{
		g_ball.dy = -g_ball.dy;
	}
}

static void
game_set_initial_positions(void)
{
	{
		g_paddles[PADDLE_HUMAN_INDEX].x = 0;
		g_paddles[PADDLE_HUMAN_INDEX].y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT;
		g_paddles[PADDLE_HUMAN_INDEX].w = PADDLE_WIDTH;
		g_paddles[PADDLE_HUMAN_INDEX].h = PADDLE_HEIGHT;
		g_paddles[PADDLE_HUMAN_INDEX].dy = PADDLE_HUMAN_SPEED;
	}

	{
		g_paddles[PADDLE_AI_INDEX].x = WINDOW_WIDTH - PADDLE_WIDTH;
		g_paddles[PADDLE_AI_INDEX].y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT;
		g_paddles[PADDLE_AI_INDEX].w = PADDLE_WIDTH;
		g_paddles[PADDLE_AI_INDEX].h = PADDLE_HEIGHT;
		g_paddles[PADDLE_AI_INDEX].dy = PADDLE_CPU_SPEED;
	}

	if (g_scores[PADDLE_HUMAN_INDEX] > g_scores[PADDLE_AI_INDEX])
	{
		g_ball.dx = 1;
	}
	else
	{
		g_ball.dx = -1;
	}

	{
		g_ball.x = WINDOW_WIDTH / 2 - BALL_WIDTH;
		g_ball.y = WINDOW_HEIGHT / 2 - BALL_HEIGHT;
		g_ball.w = BALL_WIDTH;
		g_ball.h = BALL_HEIGHT;
		g_ball.dy = BALL_SPEED;
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
game_update_player_pad(pad_direction_t direction)
{
	if (direction == PAD_GO_UP && g_paddles[PADDLE_HUMAN_INDEX].dy > 0)
	{
		g_paddles[PADDLE_HUMAN_INDEX].dy = -g_paddles[PADDLE_HUMAN_INDEX].dy;
	}
	else if (direction == PAD_GO_DOWN && g_paddles[PADDLE_HUMAN_INDEX].dy < 0)
	{
		g_paddles[PADDLE_HUMAN_INDEX].dy = abs(g_paddles[PADDLE_HUMAN_INDEX].dy);
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

static bool
game_ball_collision_with_paddle(paddle_t paddle)
{
	const int left_a = paddle.x;
	const int right_a = paddle.x + paddle.w;
	const int top_a = paddle.y;
	const int bottom_a = paddle.y + paddle.h;

	const int left_b = g_ball.x;
	const int right_b = g_ball.x + g_ball.w;
	const int top_b = g_ball.y;
	const int bottom_b = g_ball.y + g_ball.h;

	if (bottom_a <= top_b)
	{
		return false;
	}

	if (top_a >= bottom_b)
	{
		return false;
	}

	if (right_a <= left_b)
	{
		return false;
	}

	if (left_a >= right_b)
	{
		return false;
	}

	return true;
}

static void
game_draw_scores(void)
{
	char text[SCOREBOARD_FONT_SIZE];
	const SDL_Color white = {0xff, 0xff, 0xff, 0xff};

	snprintf(text, SCOREBOARD_FONT_SIZE, "%d", g_scores[PADDLE_HUMAN_INDEX]);

	if (game_load_texture_from_text(text, white, &g_textures[TEXTURE_SCOREBOARD_HUMAN]))
	{
		SDL_Rect render_quad = {WINDOW_WIDTH * 0.25, 10, g_textures[TEXTURE_SCOREBOARD_HUMAN]->width, g_textures[TEXTURE_SCOREBOARD_HUMAN]->height};
		SDL_RenderCopy(g_renderer, g_textures[TEXTURE_SCOREBOARD_HUMAN]->texture, 0, &render_quad);
	}
	else
	{
		debug_print("Could not load texture from player's score text.\n");
	}

	snprintf(text, SCOREBOARD_FONT_SIZE, "%d", g_scores[PADDLE_AI_INDEX]);

	if (game_load_texture_from_text(text, white, &g_textures[TEXTURE_SCOREBOARD_AI]))
	{
		SDL_Rect render_quad = {WINDOW_WIDTH * 0.75, 10, g_textures[TEXTURE_SCOREBOARD_AI]->width, g_textures[TEXTURE_SCOREBOARD_AI]->height};
		SDL_RenderCopy(g_renderer, g_textures[TEXTURE_SCOREBOARD_AI]->texture, 0, &render_quad);
	}
	else
	{
		debug_print("Could not load texture from AI's score text.\n");
	}
}

static bool
game_load_texture_from_text(const char *text, SDL_Color colour, texture_t **t)
{
	// Override previous texture.
	if (*t)
	{
		SDL_DestroyTexture((*t)->texture);
		(*t)->texture = 0;
	}

	SDL_Surface *surface = TTF_RenderText_Solid(g_font, text, colour);

	if (!surface)
	{
		debug_print("Could not create surface from text: %s.\n", TTF_GetError());
		return false;
	}

	(*t)->texture = SDL_CreateTextureFromSurface(g_renderer, surface);

	if (!(*t)->texture)
	{
		debug_print("Could not create texture from surface: %s.\n", SDL_GetError());
	}

	(*t)->width = surface->w;
	(*t)->height = surface->h;

	SDL_FreeSurface(surface);

	return (*t)->texture != 0;
}

static bool
game_load_texture_from_img(texture_t **t, const char *filepath)
{
	SDL_Surface *s = IMG_Load(filepath);

	if (!s)
	{
		debug_print("Could not load menu: %s.\n", IMG_GetError());
		return false;
	}

	(*t)->texture = SDL_CreateTextureFromSurface(g_renderer, s);

	if (!(*t)->texture)
	{
		debug_print("Could not create texture from surface: %s.\n", SDL_GetError());
	}

	(*t)->width = s->w;
	(*t)->height = s->h;

	SDL_FreeSurface(s);

	return (*t)->texture != 0;
}

static void
game_handle_input_game_ended(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_RETURN)
	{
		g_game_status = GAME_STATUS_PLAYING;
	}
}

static void
game_check_win_condition(void)
{
	bool end = false;

	if (g_scores[PADDLE_HUMAN_INDEX] == SCOREBOARD_LIMIT)
	{
		g_game_status = GAME_STATUS_GAME_WON;
		end = true;
	}
	else if (g_scores[PADDLE_AI_INDEX] == SCOREBOARD_LIMIT)
	{
		g_game_status = GAME_STATUS_GAME_OVER;
		end = true;
	}

	if (end)
	{
		game_reset_scoreboard();
	}
}

static void
game_reset_scoreboard(void)
{
	g_scores[PADDLE_AI_INDEX] = g_scores[PADDLE_HUMAN_INDEX] = 0;
}

static void
game_draw_texture(texture_t *t)
{
	SDL_Rect render_quad = {0, 0, t->width, t->height};
	SDL_RenderCopy(g_renderer, t->texture, 0, &render_quad);
}
