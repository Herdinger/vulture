/* NetHack may be freely redistributed.  See license for details. */

#ifndef _mainwin_h_
#define _mainwin_h_

#include <vector>
#include "window.h"


class mainwin : public window
{
public:
	mainwin(window *p);
    virtual ~mainwin();
	virtual bool draw();
	virtual void layout();

protected:
	int border_left, border_right, border_top, border_bottom;
	int get_frameheight();
private:
    static std::vector<mainwin*> main_stack;
};


#endif
