#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL/SDL.h>
#include "Def.h"
class Display
{
    public:
        Display();
        virtual ~Display();
        void Init(u32 w, u32 h, PIXEL_COLOUR pcol_on, PIXEL_COLOUR pcol_off);
        void process();
        bool* getPixels();
        void setPixel(u16 x, u16 y, bool on, bool XOR = false);
        bool getPixel(u16 x, u16 y);
        void clear();
    protected:
    private:
        void draw();
        void drawPixel(u16 x, u16 y, bool on);
        // The screen SDL surface
        SDL_Surface* screen;
        // The width of the display
        u32 w;
        // The height of the display
        u32 h;

        // Pixel Width
        u16 pw;
        // Pixel height
        u16 ph;

        // The 32 bit RGB colour when the pixel is turned on
        PIXEL_COLOUR pcol_on;
        // The 32 bit RGB colour when the pixel is turned off
        PIXEL_COLOUR pcol_off;

        /* Pixel array for chip8 display, chip 8 uses a 64*32 display you can use bools as the display is monochrome
         * and only uses two colours one for on and one for off. E.g black for off white for on.*/
        bool pixels[CHIP8_RESOLUTION];
};
#endif // DISPLAY_H
