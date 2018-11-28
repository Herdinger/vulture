/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include <string.h>

#include "vulture_sdl.h"
#include "vulture_gra.h"


Uint32 CLR32_BLACK;
Uint32 CLR32_BLACK_A30;
Uint32 CLR32_BLACK_A50;
Uint32 CLR32_BLACK_A70;
Uint32 CLR32_GREEN;
Uint32 CLR32_YELLOW;
Uint32 CLR32_ORANGE;
Uint32 CLR32_RED;
Uint32 CLR32_GRAY20;
Uint32 CLR32_GRAY70;
Uint32 CLR32_GRAY77;
Uint32 CLR32_PURPLE44;
Uint32 CLR32_LIGHTPINK;
Uint32 CLR32_LIGHTGREEN;
Uint32 CLR32_BROWN;
Uint32 CLR32_WHITE;
Uint32 CLR32_BLESS_BLUE;
Uint32 CLR32_CURSE_RED;
Uint32 CLR32_GOLD_SHADE;


SDL_PixelFormat * vulture_px_format = NULL;


#define FADESTEPS_PER_SEC 20

//TODO rework for rendering api
static void dofade(double n_secs, SDL_Surface * blendimg)
{
	unsigned int cur_clock, end_clock, start_clock, sleeptime, elapsed;
    SDL_BlendMode oldBlendMode;
    uint8_t oldAlpha;

    //save old surface state
    SDL_GetSurfaceAlphaMod(blendimg, &oldAlpha);
    SDL_GetSurfaceBlendMode(blendimg, &oldBlendMode);
    //if we don't have an alpha channel AlphaMod is used as an alpha channel else, they are interpolated.

    /* calculate how many individual blends we want to do */
	int n_steps = FADESTEPS_PER_SEC * n_secs;
    SDL_SetSurfaceAlphaMod(blendimg, (SDL_ALPHA_OPAQUE / n_steps));
    SDL_SetSurfaceBlendMode(blendimg, SDL_BLENDMODE_ADD);

    start_clock = cur_clock = SDL_GetTicks();
	end_clock = start_clock + (n_secs * 1000);
	while (cur_clock < end_clock)
	{
		/* blitting the image onto the screen repeatedly
		* will slowly add up to full brightness, while
		* drawing over the screen with semi-transparent
		* darkness repeatedly gives us a fade-out effect*/
		SDL_BlitSurface(blendimg, NULL, vulture_screen, NULL);
		vulture_refresh();

		elapsed = SDL_GetTicks() - cur_clock;
		sleeptime = ((1000/FADESTEPS_PER_SEC) > elapsed) ? ((1000/FADESTEPS_PER_SEC) - elapsed) : 0;
		SDL_Delay(sleeptime);
		cur_clock = SDL_GetTicks();
	}

    //again only valid if we asume blendimg has no alpha channel
    /* ensure that the screen is fully faded in */
    SDL_SetSurfaceBlendMode(blendimg, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceAlphaMod(blendimg, 255);
    SDL_BlitSurface(blendimg, NULL, vulture_screen, NULL);
    vulture_refresh();

    //restore old surface state
    SDL_SetSurfaceAlphaMod(blendimg, oldAlpha);
    SDL_SetSurfaceBlendMode(blendimg, oldBlendMode);
}


void vulture_fade_out(double n_secs)
{
	/* create a screen sized pure black surface without an alpha channel */
    SDL_Surface * img = SDL_CreateRGBSurface(0, vulture_screen->w, vulture_screen->h,
                                             vulture_px_format->BitsPerPixel,
                                             vulture_px_format->Rmask,
                                             vulture_px_format->Gmask,
                                             vulture_px_format->Bmask, 0);
    SDL_FillRect(img, NULL, 0);

	/* blend the blackness with the existing image */
    dofade(n_secs, img);

	SDL_FreeSurface(img);
}


void vulture_fade_in(double n_secs)
{
	/* create a copy of the screen without an alpha-channel (ie an alpha mask == 0) */
	SDL_Surface * img = SDL_CreateRGBSurface(0, vulture_screen->w, vulture_screen->h,
                                             vulture_px_format->BitsPerPixel,
                                             vulture_px_format->Rmask,
                                             vulture_px_format->Gmask,
                                             vulture_px_format->Bmask, 0);
    SDL_BlitSurface(vulture_screen, NULL, img, NULL);

	/* set the screen to all-black and display that */
	SDL_FillRect(vulture_screen, NULL, CLR32_BLACK);
	vulture_refresh();

	dofade(n_secs, img);

	SDL_FreeSurface(img);
}


/*--------------------------------------------------------------------------
Basic graphics plotting
--------------------------------------------------------------------------*/
void vulture_set_draw_region
(
int _drx1, int _dry1,
int _drx2, int _dry2
)
{
	SDL_Rect rect;

	rect.x = _drx1;
	rect.y = _dry1;
	rect.w = _drx2+1-_drx1;
	rect.h = _dry2+1-_dry1;
	SDL_SetClipRect( vulture_screen, &rect );
}



/* draws an unfilled rectangle onto the given surface;
* this function may blow up if given invalid coordinates */
void vulture_rect_surface
(
	SDL_Surface *surface,
	int x1, int y1,
	int x2, int y2,
	Uint32 color
)
{
	int i,j; 
	Uint32 *screen;

	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
	screen = (Uint32 *)surface->pixels;

	for (i = x1; i <= x2; i++)
	{
		screen[y1*surface->w+i] = color;
		screen[y2*surface->w+i] = color;
	}
	for (j = y1; j <= y2; j++)
	{
		screen[j*surface->w+x1] = color;
		screen[j*surface->w+x2] = color;
	}

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}


/* draws a filled rectangle onto the given surface;
* this function blows up if given invalid coordinates */
void vulture_fill_rect_surface
(
	SDL_Surface *surface,
	int x1, int y1,
	int x2, int y2,
	Uint32 color
)
{
	SDL_Rect dstrect, srcrect;

	dstrect.x = x1;
	dstrect.y = y1;
	dstrect.w = x2+1-x1;
	dstrect.h = y2+1-y1;

	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = dstrect.w;
	srcrect.h = dstrect.h;

	/* to get alpha-blending with our rect-filling we create a temp surface, fill that
	* and then let SDL_BlitSurface do the blending for us */
        //TODO rework using rendering api
        //https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlsetalpha.html
        //https://wiki.libsdl.org/SDL_SetSurfaceBlendMode?highlight=%28%5CbCategorySurface%5Cb%29%7C%28CategoryEnum%29%7C%28CategoryStruct%29
        //SRCALPHA seems to indicate an additive mode
	SDL_Surface * tmp_surface = SDL_CreateRGBSurface(
			0,
			srcrect.w,
			srcrect.h,
			32, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, DEF_AMASK);

        //SDL_SetSurfaceBlendMode(tmp_surface, SDL_BLENDMODE_ADD);

	SDL_FillRect(tmp_surface, &srcrect, color);
	
	SDL_BlitSurface(tmp_surface, &srcrect, surface, &dstrect);
	SDL_FreeSurface(tmp_surface);
}


void vulture_draw_raised_frame(int x1, int y1, int x2, int y2)
{
	/* bottom edge */
	vulture_line(x1 + 1, y2, x2,     y2,     CLR32_GRAY70);
	/* top edge */
	vulture_line(x1,     y1, x2 - 1, y1,     CLR32_GRAY20);
	/* right edge */
	vulture_line(x2,     y1, x2,     y2,     CLR32_GRAY77);
	/* left edge */
	vulture_line(x1,     y1, x1,     y2 - 1, CLR32_GRAY20);
}


void vulture_draw_lowered_frame(int x1, int y1, int x2, int y2)
{
	/* top edge */
	vulture_line(x1,     y1,     x2 - 1, y1,     CLR32_GRAY70);
	/* left edge */
	vulture_line(x1,     y1 + 1, x1,     y2 - 1, CLR32_GRAY77);
	/* bottom edge */
	vulture_line(x1 + 1, y2,     x2,     y2,     CLR32_GRAY20);
	/* right edge */
	vulture_line(x2,     y1 + 1, x2,     y2,     CLR32_GRAY20);
}

/*--------------------------------------------------------------------------
Bitmap handling
--------------------------------------------------------------------------*/
SDL_Surface *vulture_get_img_src
(
	int x1, int y1,
	int x2, int y2,
	SDL_Surface *img_source
)
{
	SDL_Surface *toSurface;
	int i;
	unsigned char * srcpixels = (unsigned char*)img_source->pixels;


	toSurface = SDL_CreateRGBSurface(
			SDL_SWSURFACE,
			x2+1-x1,
			y2+1-y1,
			img_source->format->BitsPerPixel,
			img_source->format->Rmask,
			img_source->format->Gmask,
			img_source->format->Bmask,
			img_source->format->Amask);

        //TODO do we want additive blending here?
        //SDL_SetSurfaceBlendMode(toSurface, SDL_BLENDMODE_ADD);

	/* we might have been passed coordinates that are outside the source which will blow up our memcpy */
	int leftoffset = (x1 < 0) ? -x1 : 0;
	int rightoffset = (x2 >= img_source->w) ? (x2 - img_source->w + 1) : 0;
	int topoffset = (y1 < 0) ? -y1: 0;
	int bottomoffset = (y2 >= img_source->h) ? (y2 - img_source->h + 1) : 0;

	int destwidth = x2+1-x1-leftoffset-rightoffset;
	int destheight = y2+1-y1-topoffset-bottomoffset;
	if (destwidth <= 0 || destheight <= 0)
		return toSurface;

	/* Blitting would be a mistake here, because we need to preserve
	* the alpha channel, which blitting does not do */
	for (i = topoffset; i < (y2+1-y1-bottomoffset); i++)
		memcpy((char*)toSurface->pixels + toSurface->pitch*i + (leftoffset * 4),
			&srcpixels[(i+y1)*img_source->pitch + (x1 + leftoffset)*4],
			destwidth*4);

	return toSurface;
}



void vulture_put_img
(
	int x, int y,
	SDL_Surface *a
)
{
	if (!a)
		return;

	SDL_Rect toRect;

	toRect.x = x;
	toRect.y = y;
	toRect.w = a->w;
	toRect.h = a->h;

	SDL_BlitSurface( a, NULL, vulture_screen, &toRect);
}

