/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
vulture_sdl.c : SDL API calls for Vulture's windowing system.
Requires SDL 1.1 or newer.
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdarg.h>
#include "vulture_sound.h"
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"
#include "vulture_gen.h"
#include "vulture_gfl.h"
#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_main.h"
#include "vulture_mou.h"
#include "vulture_opt.h"
#include "vulture_tile.h"

#include "winclass/levelwin.h"

#include "date.h"


/* Definitions */


static int fs_message_printed = 0;
static int have_mouse_focus = 1;

/* Graphics objects */
SDL_Surface *vulture_screen;             /* Graphics surface */
SDL_Texture *vulture_screen_texture;
SDL_Renderer *vulture_renderer;
SDL_Window *vulture_window;


Uint32 vulture_timer_callback(Uint32 interval, void * param);
int vulture_handle_global_event(SDL_Event * event);



void vulture_wait_event(SDL_Event * event, int wait_timeout)
{
	int done = 0;
	SDL_TimerID sleeptimer = NULL;

	if (wait_timeout > 0)
		sleeptimer = SDL_AddTimer(wait_timeout, vulture_timer_callback, NULL);


	while (!done)
	{
		event->type = 0;

		while (SDL_PollEvent(event) && event->type == SDL_MOUSEMOTION)
			/* do nothing, we're merely dequeueing unhandled mousemotion events */ ;

		if (!event->type && !SDL_WaitEvent(event))
			continue;

		vulture_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
            event->type == SDL_MOUSEWHEEL ||
            /* send timer events only while the mouse is in the window */
			(event->type == SDL_TIMEREVENT && have_mouse_focus) ||
			event->type == SDL_MOUSEMOTION)
			done = 1;
	}

	SDL_RemoveTimer(sleeptimer);
}



int vulture_poll_event(SDL_Event * event)
{
	int done = 0;

	while (!done)
	{
		event->type = 0;

		while (SDL_PollEvent(event) && event->type == SDL_MOUSEMOTION)
			/* do nothing, we're merely dequeueing unhandled mousemotion events */ ;

		if (!event->type)
			/* no events queued */
			return 0;

		vulture_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
            event->type == SDL_MOUSEWHEEL ||
            event->type == SDL_MOUSEMOTION)
			/* an interesting event, leave the loop */
			done = 1;

		/* else: an uninteresting event, like those handled in vulture_handle_global_event */
	}
	return 1;
}


/* wait for an input event, but stop waiting after wait_timeout milliseconds */
void vulture_wait_input(SDL_Event * event, int wait_timeout)
{
	int done = 0;
	SDL_TimerID sleeptimer = NULL;

	if (wait_timeout > 0)
		sleeptimer = SDL_AddTimer(wait_timeout, vulture_timer_callback, NULL);

	/* loop until we have an interesting event */
	while (!done)
	{
		if (!SDL_WaitEvent(event))
			continue;

		vulture_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
			event->type == SDL_TIMEREVENT)
			done = 1;
	}

	SDL_RemoveTimer(sleeptimer);
}




void vulture_wait_key(SDL_Event * event)
{
	int done = 0;

	while (!done)
	{
		if (!SDL_WaitEvent(event))
			continue;

		vulture_handle_global_event(event);

		if (event->type == SDL_KEYDOWN)
			done = 1;
	}
}


Uint32 vulture_timer_callback(Uint32 interval, void * param)
{
	SDL_Event event;
	event.type = SDL_TIMEREVENT;
	event.user.data1 = param;

	SDL_PushEvent(&event);

	return interval;
}

static void build_render_texture(void)
{
    int w, h;
    SDL_GetRendererOutputSize(vulture_renderer, &w, &h);
    SDL_DestroyTexture(vulture_screen_texture);
    SDL_FreeSurface(vulture_screen);
    vulture_screen = SDL_CreateRGBSurface(0, w, h, 32,
                                          0x00FF0000,
                                          0x0000FF00,
                                          0x000000FF,
                                          0xFF000000);
    vulture_screen_texture = SDL_CreateTexture(vulture_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,w,h);
}


static void vulture_set_fullscreen(void)
{
    SDL_SetWindowFullscreen(vulture_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    build_render_texture();
    int w, h;
    SDL_GetRendererOutputSize(vulture_renderer, &w, &h);
    vulture_win_resize(w, h);
}



static void vulture_set_windowed()
{
    SDL_SetWindowFullscreen(vulture_window, 0);
    SDL_SetWindowSize(vulture_window, vulture_opts.width, vulture_opts.height);
    build_render_texture();
    vulture_win_resize(vulture_opts.width, vulture_opts.height);
}



void vulture_set_screensize(void)
{
	if (vulture_opts.fullscreen)
		vulture_set_fullscreen();
	else
		vulture_set_windowed();
}



int vulture_handle_global_event(SDL_Event * event)
{
	static int quitting = 0;
	static int need_kludge = 0;
	static point actual_winsize = {0,0};
	
	/* blarg. see the SDL_VIDEORESIZE case */
	if (need_kludge &&
		(actual_winsize.x != vulture_opts.width ||
		actual_winsize.y != vulture_opts.height))
	{
    vulture_set_screensize();
		actual_winsize.x = vulture_opts.width;
		actual_winsize.y = vulture_opts.height;
		need_kludge = 0;
	}

	switch (event->type)
	{

		case SDL_QUIT:
			if (quitting)
				break;

			/* prevent recursive quit dialogs... */
			quitting++;

			/* exit gracefully */
			if (program_state.gameover)
			{
				/* assume the user really meant this, as the game is already over... */
				/* to make sure we still save bones, just set stop printing flag */
				program_state.stopprint++;
				/* and send keyboard input as if user pressed ESC */
				vulture_eventstack_add('\033', -1, -1, V_RESPOND_POSKEY); 
			}
			else if (!program_state.something_worth_saving)
			{
				/* User exited before the game started, e.g. during splash display */
				/* Just get out. */
				vulture_bail(NULL);
			}
			else
			{
				switch (vulture_yn_function("Save and quit?", "yn", 'n'))
				{
					case 'y':
						vulture_eventstack_add('y', -1 , -1, V_RESPOND_CHARACTER);
						dosave();
						break;
					case 'n':
						vulture_eventstack_add('\033', -1, -1, V_RESPOND_CHARACTER);
						break;
					default:
						break;
				}
			}
			quitting--;
			break;

		case SDL_MOUSEMOTION:
			vulture_set_mouse_pos(event->motion.x, event->motion.y);
			break;

		case SDL_KEYDOWN:
			if (event->key.keysym.sym == SDLK_TAB && (event->key.keysym.mod & KMOD_ALT) && vulture_opts.fullscreen)
			{
				/* go out of fullscreen for alt + tab */
				vulture_opts.fullscreen = 0;
				vulture_set_windowed();
				if (!vulture_opts.fullscreen && !fs_message_printed++)
					/* This gets displayed only once */
					pline("You have left fullscreen mode. Press ALT+RETURN to reenter it.");

				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			else if(event->key.keysym.sym == SDLK_RETURN && (event->key.keysym.mod & KMOD_ALT))
			{
				/* toggle fullscreen with ctrl + enter */
				if (vulture_opts.fullscreen)
				{
					vulture_opts.fullscreen = 0;
					vulture_set_windowed();
				}
				else
				{
					vulture_opts.fullscreen = 1;
					vulture_set_fullscreen();
				}
				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			else if(event->key.keysym.sym == SDLK_SYSREQ ||
					event->key.keysym.sym == SDLK_PRINTSCREEN ||
					event->key.keysym.sym == SDLK_F12)
			{
				vulture_save_screenshot();
				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			/* magic tileconfig reload key */
			else if (event->key.keysym.sym == SDLK_F11 &&
					(event->key.keysym.mod & KMOD_ALT) &&
					(event->key.keysym.mod & KMOD_SHIFT))
			{
				vulture_unload_gametiles();
				vulture_load_gametiles();
				ROOTWIN->draw_windows();
				pline("tileconfig reloaded!");
			}

			/* object highlight */
			if (vulture_map_highlight_objects == 0 &&
				((event->key.keysym.mod & KMOD_RCTRL) ||
				event->key.keysym.sym == SDLK_RCTRL))
			{
				/* enable highlighting */
				vulture_map_highlight_objects = 1;

				/* set the max clipping rect */

				/* redraw with the object highlight */
				ROOTWIN->draw_windows();

				/* the mouse got painted over, restore it */
				vulture_mouse_draw();

				/* bring the buffer onto the screen */
				vulture_refresh_window_region();

				/* make sure no after-images of the mouse are left over */
				vulture_mouse_restore_bg();
			}

			break;

		case SDL_KEYUP:
			if (vulture_map_highlight_objects == 1)
			{
				/* disable highlighting */
				vulture_map_highlight_objects = 0;

				/* redraw without the object highlight */
				ROOTWIN->draw_windows();

				/* the mouse got painted over, restore it */
				vulture_mouse_draw();

				/* bring the buffer onto the screen */
				vulture_refresh_window_region();

				/* make sure no after-images of the mouse are left over */
				vulture_mouse_restore_bg();
			}
			break;

		case SDL_WINDOWEVENT:
            switch(event->window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    actual_winsize.x = event->window.data1;
                    actual_winsize.y = event->window.data2;

                    vulture_opts.width = event->window.data1;
                    if (event->window.data1 < 540)
                    {
                        need_kludge = 1;
                        vulture_opts.width = 540;
                    }
                    vulture_opts.height = event->window.data2;
                    if (event->window.data2 < 300)
                    {
                        need_kludge = 1;
                        vulture_opts.height = 300;
                    }

                    /* SDL will not actually change the size of the window here, only the size of the buffer.
                     * therefore we may need_kludge to call SDL_SetVideoMode AGAIN to set the window size. */
                    vulture_set_screensize();
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    vulture_refresh();
                    break;
                case SDL_WINDOWEVENT_ENTER:
                    have_mouse_focus = true;
                    break;
                case SDL_WINDOWEVENT_LEAVE:
                    have_mouse_focus = false;
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    vulture_refresh();
                    break;
            }
            break;
    }
    return 0;
}



static void vulture_sdl_error(const char *file, int line, const char *what)
{
	vulture_write_log(V_LOG_ERROR, file, line, "%s: %s\n", what, SDL_GetError());
}


void vulture_enter_graphics_mode()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) == -1)
	{
		vulture_sdl_error(__FILE__, __LINE__, "Could not initialize SDL");
		exit(1);
	}

	/* Initialize the event handlers */
	atexit(SDL_Quit);
        //Create Initial window and renderer
    vulture_window = SDL_CreateWindow(VERSION_ID,SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, vulture_opts.width, vulture_opts.height,
                                      vulture_opts.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    vulture_renderer = SDL_CreateRenderer(vulture_window, -1, SDL_RENDERER_ACCELERATED);
    int actualW, actualH;
    SDL_GetRendererOutputSize(vulture_renderer, &actualW, &actualH);
    vulture_screen = SDL_CreateRGBSurface(0, actualW, actualH, 32,
                                          0x00FF0000,
                                          0x0000FF00,
                                          0x000000FF,
                                          0xFF000000);
    vulture_screen_texture = SDL_CreateTexture(vulture_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, actualW, actualH);
    /* Filter key, mouse and quit events */
    /*  SDL_SetEventFilter(FilterEvents); */
    //TODO Have to do key repeat in software
    vulture_set_screensize();
    /* still no screen: nothing more to be done */
    if (!vulture_screen)
    {
        vulture_sdl_error(__FILE__, __LINE__, "Could not initialize video mode");
        exit(1);
    }

	/* Don't show double cursor */
	SDL_ShowCursor(SDL_DISABLE);

}



void vulture_exit_graphics_mode(void)
{
	SDL_ShowCursor(SDL_ENABLE);

	vulture_stop_music();
	SDL_Quit();
}



void vulture_refresh_region
(
	int x1, int y1, 
	int x2, int y2 
)
{
	SDL_Rect rect;

	/* resizing support makes this check necessary: we may try to refresh
	* a region that used to be inside the window, but isn't anymore */
	x1 = (x1 >= vulture_screen->w) ? vulture_screen->w - 1 : x1;
	y1 = (y1 >= vulture_screen->h) ? vulture_screen->h - 1 : y1;
	
	/* Clip edges (yes, really, otherwise we crash...) */
	rect.x = (x1 < 0) ? 0 : x1;
	rect.y = (y1 < 0) ? 0 : y1;
	rect.w = ((x2 >= vulture_screen->w) ? vulture_screen->w - 1 : x2) - rect.x + 1;
	rect.h = ((y2 >= vulture_screen->h) ? vulture_screen->h - 1 : y2) - rect.y + 1;

	//TODO temporary nop SDL_UpdateRects(vulture_screen, 1, &rect);
}



void vulture_refresh(void)
{
  SDL_UpdateTexture(vulture_screen_texture, NULL, vulture_screen->pixels, vulture_screen->pitch);
  SDL_RenderClear(vulture_renderer);
  SDL_RenderCopy(vulture_renderer, vulture_screen_texture, NULL, NULL);
  SDL_RenderPresent(vulture_renderer);
}

