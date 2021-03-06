/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_win.h"
#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_txt.h"

#include "button.h"


button::button(window *p, std::string caption, int menuid, char accel) : window(p)
{
	v_type = V_WINTYPE_BUTTON;

	this->caption = caption;

	menu_id = menuid;
	accelerator = accel;
	
	image = NULL;
	selected = false;
	autobg = true;

	w = vulture_text_length(V_FONT_MENU, caption) + 14;
	h = vulture_text_height(V_FONT_MENU, caption) + 10;
}


button::~button()
{
	if (image)
		SDL_FreeSurface(image);
	image = NULL;
}


bool button::draw()
{
	int x = abs_x;
	int y = abs_y;
    if(selected) {
        x+=2;
        y+=2;
    }


	int text_start_x, text_start_y;

	/* Black edge */
	vulture_rect(x+1, y+1, x+w-2, y+h-2, V_COLOR_BACKGROUND);

	/* Outer edge (lowered) */
	vulture_draw_lowered_frame(x, y, x + w - 1, y + h - 1);
	/* Inner edge (raised) */
	vulture_draw_raised_frame(x + 2, y + 2, x + w - 3, y + h - 3);

	if (!caption.empty()) {
		text_start_x = x + (w - vulture_text_length(V_FONT_BUTTON, caption))/2;
		text_start_y = y + 5;

		vulture_put_text_shadow(V_FONT_BUTTON, caption, vulture_screen, text_start_x,
								text_start_y, V_COLOR_TEXT, V_COLOR_BACKGROUND);
	} else if (image) {
		vulture_put_img(x + (w - image->w) / 2, y + (h - image->h)/2, image);
	}

	return false;
}


eventresult button::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	if (selected)
		need_redraw = 1;
	selected = 0;
	return V_EVENT_UNHANDLED_REDRAW;
}


eventresult button::handle_other_event(window* target, void* result, SDL_Event *event)
{
	if (this == target && event->type == SDL_MOUSEBUTTONDOWN &&
	    event->button.button == SDL_BUTTON_LEFT) {
		selected = 1;
		need_redraw = 1;
	} else if (event->type == SDL_MOUSEMOVEOUT) {
		if (selected)
			need_redraw = 1;
		selected = 0;
	}
	
	if (need_redraw)
		return V_EVENT_UNHANDLED_REDRAW;

	return V_EVENT_UNHANDLED;
}
