/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_gra.h"
#include "vulture_txt.h"
#include "vulture_win.h"
#include "vulture_sdl.h"

#include "textwin.h"


textwin::textwin(window *p, std::string cap) : window(p)
{
	v_type = V_WINTYPE_TEXT;
	caption = cap;
	is_input = false;
	textcolor = V_COLOR_TEXT;
	w = vulture_text_length(V_FONT_MENU, caption) + 10;
}

textwin::textwin(window *p, int destsize) : window(p)
{
	v_type = V_WINTYPE_TEXT;
	caption = "";
	
	is_input = true;
	textcolor = V_COLOR_TEXT;
	autobg = true;
}


bool textwin::draw()
{
	int textlen = 0;

	vulture_put_text_shadow(V_FONT_MENU, caption, vulture_screen,
	                         abs_x, abs_y, textcolor, V_COLOR_BACKGROUND);

	textlen = vulture_text_length(V_FONT_MENU, caption);


	if (is_input)
		/* draw prompt */
		vulture_rect(abs_x + textlen + 1, abs_y, abs_x + textlen + 1,
				abs_y + h - 2, V_COLOR_TEXT);

	return false;
}


void textwin::set_caption(std::string str)
{
	window::set_caption(str);
	w = vulture_text_length(V_FONT_MENU, caption) + 10;
}
