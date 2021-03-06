/* NetHack may be freely redistributed.  See license for details. */

#include "introwin.h"

#include <SDL.h>

#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_gra.h"
#include "vulture_gfl.h"
#include "vulture_txt.h"
#include "vulture_mou.h"
#include "vulture_tile.h"

introwin::introwin(window *p, std::vector<std::string> &imagenames, std::vector< std::vector<std::string> > &subtitles) :
                   window(p), imagenames(imagenames), subtitles(subtitles)
{
	current_scene = -1;
	
	w = vulture_screen->w;
	h = vulture_screen->h;
	
	image = NULL;
	image_changed = false;
	starttick = 0;
	need_redraw = true;
	scenetime = 0;
}


introwin::~introwin()
{
	if (image)
		SDL_FreeSurface(image);
}


bool introwin::draw()
{
	int j, pos_x, pos_y, img_x, img_y;
	int lineheight = vulture_get_lineheight(V_FONT_INTRO);
	
	vulture_set_draw_region(0, 0, w - 1, h - 1);
	if (image) {
		img_x = (w - image->w) / 2;
		img_y = (h - image->h) / 6;
		
		if (image_changed)
			vulture_fade_out(0.2);
		
		SDL_FillRect(vulture_screen, NULL, CLR32_BLACK);
		vulture_put_img(img_x, img_y, image);
		if (image_changed)
			vulture_fade_in(0.2);
		
		image_changed = false;
		
		for (j = 0; j < (int)subtitles[current_scene].size(); j++) {
			pos_x = (w - vulture_text_length(V_FONT_INTRO, subtitles[current_scene][j])) / 2;
			pos_y = 2 * img_y + image->h + lineheight * j;
			vulture_put_text(V_FONT_INTRO, subtitles[current_scene][j], vulture_screen,
								pos_x, pos_y, V_COLOR_INTRO_TEXT);
			scenetime += subtitles[current_scene][j].length() * MSEC_PER_CHAR;
		}
	} else
		SDL_FillRect(vulture_screen, NULL, CLR32_BLACK);
	
	return false;
}


eventresult introwin::handle_timer_event(window* target, void* result, int time)
{
	/* next scene? */
	if (SDL_GetTicks() - starttick > scenetime)
		return next_scene();
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult introwin::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	/* hide cursor */
	vulture_set_mcursor(V_TILE_NONE);
	
	/* next scene? */
	if (SDL_GetTicks() - starttick > scenetime)
		return next_scene();
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult introwin::handle_mousebuttonup_event(window* target, void* result,
                                int mouse_x, int mouse_y, int button, int state)
{
	/* cancel intro */
	return V_EVENT_HANDLED_FINAL;
}


eventresult introwin::handle_keydown_event(window* target, void* result, int sym,
                                           int mod, int unicode)
{
	/* cancel intro */
	return V_EVENT_HANDLED_FINAL;
}


eventresult introwin::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	w = res_w;
	h = res_h;
	return V_EVENT_HANDLED_REDRAW;
}


eventresult introwin::next_scene()
{
	starttick = SDL_GetTicks();
	
	current_scene++;
	scenetime = 0;
	if (current_scene >= (int)imagenames.size())
		return V_EVENT_HANDLED_FINAL;
	
	if (image)
		SDL_FreeSurface(image);
	
	image = vulture_load_graphic(imagenames[current_scene]);
	if (current_scene > 0 && imagenames[current_scene - 1] != imagenames[current_scene])
		image_changed = true;
	
	need_redraw = true;
	return V_EVENT_HANDLED_REDRAW;
}
