/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb/stb_image_write.h"

#include "vulture_gfl.h"
#include "vulture_gen.h"
#include "vulture_win.h"
#include "vulture_sdl.h"

#include "winclass/messagewin.h"


/*--------------------------------------------------------------------------
truecolor png image loader (backend)
takes a buffer containing raw image data and returns an SDL_Surface
srcbuf       : [in]  buffer containing a complete image file
buflen       : [in]  length of the input buffer
--------------------------------------------------------------------------*/
SDL_Surface *vulture_load_surface(char *srcbuf, unsigned int buflen)
{
	Uint32 Rmask, Gmask, Bmask, Amask;
	int width, height;
	SDL_Surface *img, *convert;

	/* vulture_load_surface converts everything to screen format for faster blitting
	* so we can't continue if we don't have a screen yet */
	if (!vulture_screen)
		return NULL;

	/* no NULL pointers, please */
	if (!srcbuf)
		return NULL;

	img = NULL;

	/* get the component mask for the surface */
	if ( SDL_BYTEORDER == SDL_LIL_ENDIAN )
	{
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = 0xFF000000;
	} else {
		Rmask = 0xFF000000;
		Gmask = 0x00FF0000;
		Bmask = 0x0000FF00;
		Amask = 0x000000FF;
	}

    unsigned char* result = stbi_load_from_memory(reinterpret_cast<unsigned char*>(srcbuf), buflen, &width, &height, nullptr, 4); 

    if(!result)
        return NULL;

    img = SDL_CreateRGBSurfaceFrom(result, width, height, 32, width*4, Rmask, Gmask, Bmask, Amask);

	if (!img)
        return NULL;
    //SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_ADD);

    /*TODO update with info from renderer
    convert = SDL_DisplayFormatAlpha(img);

	SDL_FreeSurface(img);
	img = convert;
    */

    return img;
}



/*-------------------------------------
truecolor image loader (front end)
loads the given file into a buffer and calls vulture_load_surface
subdir:  [in] subdirectory in which to look
name:    [in] filename of the image
---------------------------------------*/
SDL_Surface *vulture_load_graphic_from(std::string folder, std::string subfolder, std::string name)
{
	SDL_Surface *image;
	int fsize;
  std::string filename;
	FILE * fp;
	char * srcbuf;

	name += ".png";
	filename = vulture_make_filename(folder, subfolder, name);

	fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return NULL;

	/* obtain file size. */
	fseek(fp , 0 , SEEK_END);
	fsize = ftell(fp);
	rewind(fp);

	srcbuf = new char[fsize];
	if (!srcbuf)
		return 0;

	fread(srcbuf, fsize, 1, fp);
	fclose(fp);

	image = vulture_load_surface(srcbuf, fsize);

	delete srcbuf;

	return image;
}

SDL_Surface *vulture_load_graphic_from(std::string folder, std::string name)
{
  return vulture_load_graphic_from(folder,"",name);
}

SDL_Surface *vulture_load_graphic(std::string name)
{
  return vulture_load_graphic_from(V_GRAPHICS_DIRECTORY,name);
}

SDL_Surface *vulture_load_map(std::string theme, std::string name)
{
  return vulture_load_graphic_from(V_MAP_DIRECTORY,theme,name);
}

/*--------------------------------------------
Save the contents of the surface to a png file
--------------------------------------------*/
void vulture_save_png(SDL_Surface *surface, std::string filename, int with_alpha)
{
	FILE * fp;
	int i, j;
	unsigned int *in_pixels = (unsigned int*)surface->pixels;
	unsigned char *buf = new unsigned char[surface->w * surface->h * (with_alpha ? 4 : 3)];
    unsigned char *output = buf;

	/* strip out alpha bytes if neccessary and reorder the image bytes to RGB */
	for (j = 0; j < surface->h; j++)
	{
		for (i = 0; i < surface->w; i++)
		{
			*output++ = (((*in_pixels) & surface->format->Rmask) >> surface->format->Rshift);       /* red   */
			*output++ = (((*in_pixels) & surface->format->Gmask) >> surface->format->Gshift);       /* green */
			*output++ = (((*in_pixels) & surface->format->Bmask) >> surface->format->Bshift);     /* blue  */
			if ( with_alpha )
				*output++ = (((*in_pixels) & surface->format->Amask) >> surface->format->Ashift); /* alpha */
			in_pixels++;
		}
	}
    stbi_write_png(filename.c_str(), surface->w, surface->h, (with_alpha ? 4 : 3), buf, 0);

	delete[] buf;
}


/*-------------------------------------
Save the contents of the screen (ie.
back buffer) into a BMP file.
---------------------------------------*/
void vulture_save_screenshot(void)
{
  std::string filename;
	int i;
  std::string msg;
	char namebuf[20];

	for (i = 0; i < 1000; i++)
	{
		sprintf(namebuf, "screenshot-%03d.png", i);
		filename = vulture_make_filename("", "", namebuf);
		if (access(filename.c_str(), R_OK) != 0)
		{
			vulture_save_png(vulture_screen, filename, 0);
			msg = std::string("Screenshot saved as ") + filename + ".";

			if (!vulture_windows_inited)
				vulture_messagebox(msg);
			else
				msgwin->add_message(msg);

			return;
		}
	}
	vulture_messagebox("Too many screenshots already saved.");
}
