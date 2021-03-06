/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_mou.h"
#include "vulture_txt.h"
#include "vulture_gen.h"
#include "vulture_tile.h"


#include "anykeydialog.h"
#include "textwin.h"
#include "button.h"


anykeydialog::anykeydialog(window *p, std::string ques) : mainwin(p)
{
	caption = ques;
	count = 0;

	/* add it as a menuitem so that menuwin::layout will dtrt */
	txt = new textwin(this, "(type any key)");
	txt->y = 0;
	txt->h = vulture_text_height(V_FONT_MENU, txt->caption) + 10;

	/* create buttons */
	new button(this, "Show choices", 2, '?');
	new button(this, "Show inventory", 3, '*');
	new button(this, "Cancel", 4, '\033');
	
	mainwin::layout();
}

eventresult anykeydialog::handle_mousemotion_event(window* target, void* result,
                                                   int xrel, int yrel, int state)
{
	vulture_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult anykeydialog::handle_mousebuttonup_event(window* target, void* result, int mouse_x, int mouse_y, int button, int state)
{
	if (target->accelerator && button == SDL_BUTTON_LEFT) {
		*(char*)result = target->accelerator;
		return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult anykeydialog::handle_keydown_event(window* target, void* result,
                                               int sym, int mod, int unicode)
{
	char key;
	int i;
	char buffer[32];

	switch (sym) {
		case SDLK_ESCAPE:
			*(char*)result = '\033';
			return V_EVENT_HANDLED_FINAL;

		case SDLK_BACKSPACE:
			count = count / 10;
			if (count > 0)
				sprintf(buffer, "Count: %d", count);
			else
				sprintf(buffer, "(press any key)");
			txt->set_caption(buffer);
			txt->need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;

		default:
			key = vulture_make_nh_key(sym, mod);
            //TODO Hack, we have to have a different mapping for our accels
            //Function keys get lost for example have to research how those get mapped
            if (find_accel(key).size() > 0) {
                *(char*)result = key;
				return V_EVENT_HANDLED_FINAL;
			}


			if (!key)
				/* no translation, it must have been a function key */
				return V_EVENT_HANDLED_NOREDRAW;

			if (isdigit(key)) {
				/* we got a digit and only modify the count */
				if (count < 10000000)
					count = count * 10 + (key - 0x30);
				sprintf(buffer, "Count: %d", count);
				txt->set_caption(buffer);
				txt->need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;
			}

			/* non-digit, non-function-key, non-accelerator: we have a winner! */
			if (count) {
				/* retrieve the count and push most of it onto the eventstack */
				memset(buffer, 0, 16);
				snprintf(buffer, 16, "%d", count);
				vulture_eventstack_add(key, -1 , -1, V_RESPOND_ANY);
				for (i=15; i > 0; i--)
					if (buffer[i])
						vulture_eventstack_add(buffer[i], -1, -1, V_RESPOND_ANY);

				/* we return the first digit of the count */
				key = buffer[0];
			}

			/* return our key */
			*(char*)result = key;
			return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult anykeydialog::handle_resize_event(window* target, void* result, int w, int h)
{
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	return V_EVENT_HANDLED_NOREDRAW;
}

bool anykeydialog::draw()
{
	return mainwin::draw();
}
