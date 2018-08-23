/* NetHack may be freely redistributed.  See license for details. */

#ifndef _imagewin_h_
#define _imagewin_h_

#include "window.h"

#include <string>
#include <vector>

#define MSEC_PER_CHAR 50

class imagewin : public window
{
public:
	imagewin(window *p, const std::string& imagename, int x, int y);
	virtual ~imagewin();
	virtual bool draw();

private:
	
	SDL_Surface *image;
    std::string imagename;
};


#endif
