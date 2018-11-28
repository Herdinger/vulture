/* NetHack may be freely redistributed.  See license for details. */

#include "window.h"

#include "vulture_gra.h"
#include "vulture_win.h"
#include "vulture_sdl.h"

window *ROOTWIN = NULL;
window::window(window *p) : parent(p)
{
	first_child = last_child = NULL;
	sib_next = sib_prev = NULL;
	abs_x = abs_y = x = y = 0;
    w = vulture_screen->w;
    h = vulture_screen->h;

	caption = "";
	accelerator = '\0';
	group_accelerator = '\0';
	background = NULL;
	autobg = false;
	menu_id_v = NULL;
	
	visible = true;
	
	if (parent != NULL)
	{
		/* add win to the parent's ll of children */
		sib_prev = parent->last_child;
		if (parent->first_child)
			parent->last_child->sib_next = this;
		else
			parent->first_child = this;
		parent->last_child = this;
	} 
	
	v_type = V_WINTYPE_CUSTOM;
}


window::~window()
{
	/* the root window has no parent */
	if (parent) {
		/* unlink the window everywhere */
		if (parent->first_child == this)
			parent->first_child = sib_next;

		if (parent->last_child == this)
			parent->last_child = sib_prev;
	}

	/* remove from the linked list of siblings */
	if (sib_prev)
		sib_prev->sib_next = sib_next;

	if (sib_next)
		sib_next->sib_prev = sib_prev;


	/* destroy it's children. note that the child's destroy function will manipulate ->first_child */
	while (first_child) {
		delete first_child; // deleting a child will unlink it; eventually first_child will be NULL
	}
    if(ROOTWIN == this)
        ROOTWIN = nullptr;
}


void window::set_caption(std::string str)
{
	caption = str;
}

bool window::draw() {
    SDL_FillRect(vulture_screen, NULL, CLR32_BLACK);
    return true;
}

void window::hide()
{
	visible = 0;
	need_redraw = 0;
}


/* find the window that has the accelerator "accel" */
std::vector<window*> window::find_accel(char accel)
{
  window *child;
  std::vector<window *> accelerators_found;

  for (child = first_child; child; child = child->sib_next)
      if (child->accelerator == accel || child->group_accelerator == accel )
          accelerators_found.push_back( child );

  return accelerators_found;
}


/****************************
* window drawing functions
****************************/
/* walks the list of windows and draws all of them, depending on their type and status */
void window::draw_windows()
{
    if(!visible)
        return;

    if(!draw()) {
        return;
    }

	for (window* current = first_child; current; current = current->sib_next) {
		/* recalc absolute position */
        current->abs_x = current->parent->abs_x + current->x;
        current->abs_y = current->parent->abs_y + current->y;
        current->draw_windows();
	}
}


window *window::walk_winlist(bool *descend)
{
	/* 1) try to descend to child */
	if (*descend && first_child)
		return first_child;

	/* 2) try sibling*/
	if (sib_next) {
		*descend = true;
		return sib_next;
	}

	/* 3) ascend to parent and set *descend = false to prevent infinite loops */
	*descend = false;
	return parent;
}


/* find the window under the mouse, starting from here */
window *window::get_window_from_point(point mouse)
{
	window *winptr, *nextwin;

	winptr = this;

	/* as each child window is completely contained by its parent we can descend
	* into every child window that is under the cursor until no further descent
	* is possible. The child lists are traversed in reverse because newer child
	* windows are considered to be on top if child windows overlap */
	while (winptr->last_child) {
		nextwin = winptr->last_child;

		while (nextwin && (nextwin->abs_x > mouse.x || nextwin->abs_y > mouse.y ||
						(nextwin->abs_x + nextwin->w) < mouse.x ||
						(nextwin->abs_y + nextwin->h) < mouse.y || !nextwin->visible))
			nextwin = nextwin->sib_prev;

		if (nextwin)
			winptr = nextwin;
		else
			return winptr;
	}
	return winptr;
}


eventresult window::event_handler(window *target, void *result, SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_TIMEREVENT:
			return handle_timer_event(target, result, event->user.code);

		case SDL_MOUSEMOTION:
			return handle_mousemotion_event(target, result, event->motion.xrel,
			                           event->motion.yrel, event->motion.state);

		case SDL_MOUSEBUTTONUP:
			return handle_mousebuttonup_event(target, result, event->button.x,
			        event->button.y, event->button.button, event->button.state);

		case SDL_KEYDOWN:
			return handle_keydown_event(target, result, event->key.keysym.sym,
			                  event->key.keysym.mod, 0);
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                return handle_resize_event(target, result, event->window.data1, event->window.data2);
            }
            else {
                return handle_other_event(target, result, event);
            }
            break;
		default:
            return handle_other_event(target, result, event);
    }

	return V_EVENT_UNHANDLED;
}


/* default event handling functions which do nothing */
eventresult window::handle_timer_event(window* target, void* result, int time)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_other_event(window* target, void* result, SDL_Event *event)
{
	return V_EVENT_UNHANDLED;
}

