/* NetHack may be freely redistributed.  See license for details. */

#include "imagewin.h"

#include <SDL.h>

#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_gra.h"
#include "vulture_gfl.h"
#include "vulture_txt.h"
#include "vulture_mou.h"
#include "vulture_tile.h"

imagewin::imagewin(window *p, const std::string &imagename, int x, int y) :
                   window(p), imagename(imagename)
{
	image = vulture_load_graphic(imagename);
	w = image->w;
	h = image->h;
    if(x == -1)
        this->x = (parent->w / 2) - w / 2;
    else
        this->x = x;
    if(y == -1)
        this->y = (parent->h / 2) - h / 2;
    else
        this->y = y;


}


imagewin::~imagewin()
{
	if (image)
		SDL_FreeSurface(image);
}


bool imagewin::draw()
{

    vulture_put_img(x, y, image);
	return false;
}


