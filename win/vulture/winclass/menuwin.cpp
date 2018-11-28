/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_sdl.h" /* XXX this must be the first include,
                             no idea why but it won't compile otherwise */

extern "C" {
	#include "hack.h"
}

#include "vulture_main.h"
#include "vulture_win.h"
#include "vulture_txt.h"
#include "vulture_mou.h"
#include "vulture_tile.h"
#include "vulture_gen.h"

#include "menuwin.h"
#include "scrollwin.h"
#include "textwin.h"
#include "optionwin.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))



menuwin::menuwin(window *p, std::list<menuitem> &menuitems, int how) : 
                 mainwin(p), items(menuitems), select_how(how)
{
	v_type = V_WINTYPE_MENU;
	
	scrollarea = NULL;
	count = 0;
	
	assign_accelerators();
}


menuwin::~menuwin()
{
	items.clear();
}


bool menuwin::draw()
{
    return mainwin::draw();
}


void menuwin::select_option(optionwin *target, int count)
{
	window *winelem;
	optionwin *opt;

	if (!target->is_checkbox) {
		/* unselect everything else */
		for (winelem = first_child; winelem; winelem = winelem->sib_next) {
			if (winelem->v_type == V_WINTYPE_OPTION ||
			    winelem->v_type == V_WINTYPE_OBJITEM) {
				opt = static_cast<optionwin*>(winelem);
				opt->item->selected = false;
				opt->item->count = -1;
				
			}
		}
		target->item->selected = true;
	}
	else
		target->item->selected = !target->item->selected;

	if (target->item->selected)
		target->item->count = count;
}


eventresult menuwin::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vulture_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult menuwin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	optionwin *opt;

	if (target == this)
		/* clicks on the menu window itself are not interesting */
		return V_EVENT_HANDLED_NOREDRAW;


	/* click on an option / checkbox */
	if (target->v_type == V_WINTYPE_OPTION)
	{
		opt = static_cast<optionwin*>(target);
		select_option(opt, count ? count : -1);
		count = 0;
		if (select_how == PICK_ONE) {
			*(int*)result = V_MENU_ACCEPT;
			return V_EVENT_HANDLED_FINAL;
		}

		need_redraw = 1;
		return V_EVENT_HANDLED_REDRAW;
	}

	/* a click on a button */
	else if (target->v_type == V_WINTYPE_BUTTON && target->menu_id)
	{
		*(int*)result = target->menu_id;
		return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult menuwin::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	optionwin *opt;
	window *winelem;
	char * str_to_find;

	need_redraw = 1;
	switch (sym)
	{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			*(int*)result = V_MENU_ACCEPT;
			return V_EVENT_HANDLED_FINAL;

		case SDLK_SPACE:
		case SDLK_ESCAPE:
			*(int*)result = (select_how == PICK_NONE) ? V_MENU_ACCEPT : V_MENU_CANCEL;
			return V_EVENT_HANDLED_FINAL;

		/* scroll via arrow keys */
		case SDLK_KP_2:
		case SDLK_DOWN:
			return scrollarea->scrollto(V_SCROLL_LINE_REL, 1);

		case SDLK_KP_8:
		case SDLK_UP:
			return scrollarea->scrollto(V_SCROLL_LINE_REL, -1);

		case SDLK_BACKSPACE:
			count = count / 10;

        case SDLK_PAGEUP:
		case MENU_PREVIOUS_PAGE:
			return scrollarea->scrollto(V_SCROLL_PAGE_REL, -1);

        case SDLK_PAGEDOWN:
		case MENU_NEXT_PAGE:
			return scrollarea->scrollto(V_SCROLL_PAGE_REL, 1);
        case SDLK_HOME:
		case MENU_FIRST_PAGE:
			return scrollarea->scrollto(V_SCROLL_PAGE_ABS, 0);
        case SDLK_END:
		case MENU_LAST_PAGE:
			return scrollarea->scrollto(V_SCROLL_PAGE_ABS, 9999);


		case MENU_SELECT_ALL:
		case MENU_UNSELECT_ALL:
			/* invalid for single selection menus */
			if (select_how == PICK_ONE)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OPTION) {
					opt = static_cast<optionwin*>(winelem);
					opt->item->selected = (sym == MENU_SELECT_ALL);
					opt->item->count = -1;
				}
			}
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;


		case MENU_INVERT_ALL:
			/* invalid for single selection menus */
			if (select_how == PICK_ONE)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OPTION) {
					opt = static_cast<optionwin*>(winelem);
					opt->item->selected = !opt->item->selected;
					opt->item->count = -1;
				}
			}
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;


		case MENU_SELECT_PAGE:
		case MENU_UNSELECT_PAGE:
			/* invalid for single selection menus */
			if (select_how == PICK_ONE)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible) {
					opt = static_cast<optionwin*>(winelem);
					opt->item->selected = (sym == MENU_SELECT_PAGE);
					opt->item->count = -1;
				}
			}
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;


		case MENU_INVERT_PAGE:
			/* invalid for single selection menus */
			if (select_how == PICK_ONE)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible) {
					opt = static_cast<optionwin*>(winelem);
					opt->item->selected = !opt->item->selected;
					opt->item->count = -1;
				}
			}
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;


		case MENU_SEARCH:
			str_to_find = (char *)malloc(512);
			str_to_find[0] = '\0';
			if (vulture_get_input(-1, -1, "What are you looking for?", str_to_find) != -1) {
				for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
					if (winelem->caption.find(str_to_find)) {
						scrollarea->scrollto(V_SCROLL_PIXEL_ABS, winelem->y);
						break;
					}
				}
			}
			free(str_to_find);
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;

		default:
			/* numbers are part of a count */
			if (select_how == PICK_ANY && sym >= '0' && sym <= '9') {
				count = count * 10 + (sym - '0');
				break;
			}
		
			/* try to match the key to an accelerator */
			int key = vulture_make_nh_key(sym, mod);
      std::vector<window *> targets_found( scrollarea->find_accel( key ) );
      for ( std::vector<window *>::iterator target = targets_found.begin();
            target != targets_found.end();
            ++target )
      {
				select_option(static_cast<optionwin*>(*target), count ? count : -1);
				count = 0;
				if (select_how == PICK_ONE) {
					*(int*)result = V_MENU_ACCEPT;
					return V_EVENT_HANDLED_FINAL;
				}

				/* if the selected element isn't visible bring it into view */
				if (!(*target)->visible)
					scrollarea->scrollto(V_SCROLL_PIXEL_ABS, (*target)->y);

				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;
			}
			break;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult menuwin::handle_resize_event(window* target, void* result, int w, int h)
{
	layout();
	return V_EVENT_HANDLED_NOREDRAW;
}


void menuwin::assign_accelerators()
{
	int new_accel;
	char used_accels[128]; /* 65 should be max # of accels, the other 63 bytes are safety :P */

	used_accels[0] = '\0';
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		if (i->accelerator == 0 && i->identifier) {
			new_accel = vulture_find_menu_accelerator(used_accels);
			if (new_accel >= 0)
				i->accelerator = new_accel;
		}
	}
}

void menuwin::layout()
{
	int scrollheight = 0;
	int buttonheight = vulture_get_lineheight(V_FONT_MENU) + 15;
	int menu_height_limit;
  std::string newcaption;
	
	// remove existing scrollarea
	if (scrollarea)
		delete scrollarea;
	
	/* create & populate new scrollarea */
	scrollarea = new scrollwin(this, (select_how == PICK_NONE));
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		newcaption = "";
		if (i->accelerator) {
			newcaption += "[ ] - ";
			newcaption[1] = i->accelerator;
		}
		newcaption += i->str;

		if (select_how == PICK_NONE)
			new textwin(scrollarea, newcaption);
		else
			new optionwin(scrollarea, &(*i), newcaption, i->accelerator, 
                   i->group_accelerator, i->glyph, i->preselected,
                   select_how == PICK_ANY);
	}
	scrollarea->layout();
	scrollheight = scrollarea->get_scrollheight();
	
	/* set actual scroll area height */
	h = scrollheight + buttonheight;
	menu_height_limit = parent->get_h() - get_frameheight() - buttonheight - 30;
	if (h > menu_height_limit) {
		h = min(menu_height_limit, MAX_MENU_HEIGHT);
		scrollheight = h - buttonheight;
		scrollarea->set_height(scrollheight);
	}
	
	mainwin::layout();
	
	/* enlarge scroll area so that the scrollbar will be on the right edge */
	scrollarea->w = w - border_left - border_right;
}


/* selection iterator */
menuwin::selection_iterator::selection_iterator(item_iterator start, item_iterator end) : iter(start), end(end)
{
	while (iter != end && !iter->selected)
		iter++;
}

menuwin::selection_iterator& menuwin::selection_iterator::operator++(void)
{
	iter++;
	while (iter != end && !iter->selected)
		iter++;
	
	return *this;
}

bool menuwin::selection_iterator::operator!=(selection_iterator rhs) const
{
	return this->iter != rhs.iter;
}

menuitem& menuwin::selection_iterator::operator*()
{
	return *iter;
}
