#include "Display.h"

Display::Display()
{
    screen = NULL;
}

Display::~Display()
{
    if (screen != NULL)
    {
        SDL_FreeSurface(screen);
    }
}

void Display::Init(u32 w, u32 h, PIXEL_COLOUR pcol_on, PIXEL_COLOUR pcol_off)
{
    // Free the screen surface if it already exists
    if (this->screen != NULL)
    {
        SDL_FreeSurface(this->screen);
    }
    this->screen = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE);
    this->w = w;
    this->h = h;
    this->pw = this->w / CHIP8_ORIGINAL_DISPLAY_WIDTH;
    this->ph = this->h / CHIP8_ORIGINAL_DISPLAY_HEIGHT;

    this->pcol_on = pcol_on;
    this->pcol_off = pcol_off;
}

// Clears the display
void Display::clear()
{
    // Turn all of the pixels off
    for (int i = 0; i < CHIP8_RESOLUTION; i++)
    {
        this->pixels[i] = false;
    }
}

void Display::setPixel(u16 x, u16 y, bool on, bool XOR)
{
    // Wrap around the pixels if they breach the screen coordinates
    if (x >= CHIP8_ORIGINAL_DISPLAY_WIDTH)
    {
        x -= CHIP8_ORIGINAL_DISPLAY_WIDTH;
    }

    if (y >= CHIP8_ORIGINAL_DISPLAY_HEIGHT)
    {
        y -= CHIP8_ORIGINAL_DISPLAY_HEIGHT;
    }

    // Set the pixel to either on or off based on the bool provided
    if (XOR)
    {
        this->pixels[(y * CHIP8_ORIGINAL_DISPLAY_WIDTH) + x] ^= on;
    }
    else
    {
        this->pixels[(y * CHIP8_ORIGINAL_DISPLAY_WIDTH) + x] = on;
    }
}

// getPixel returns true when the pixel is set and false when it is not
bool Display::getPixel(u16 x, u16 y)
{
    return this->pixels[(y * CHIP8_ORIGINAL_DISPLAY_WIDTH) + x];
}
void Display::drawPixel(u16 x, u16 y, bool on)
{
    u32* screenPixels = (u32*)this->screen->pixels;
    u32 pixelColour;
    if (on)
        pixelColour = this->pcol_on;
    else
        pixelColour = this->pcol_off;

    // Calculate real coordinates
    u16 xr = x * pw;
    u16 yr = y * ph;

    // Draw the pixels until correct size is reached
    for (u32 x1 = xr; x1 < xr + pw; x1++)
    {
        for (u32 y2 = yr; y2 < yr + pw; y2++)
        {
            screenPixels[(y2 * this->w) + x1] = pixelColour;
        }
    }
}

void Display::process()
{
   draw();
}

bool* Display::getPixels()
{
    return this->pixels;
}

void Display::draw()
{
   for (u16 x = 0; x < CHIP8_ORIGINAL_DISPLAY_WIDTH; x++)
   {
       for (u16 y = 0; y < CHIP8_ORIGINAL_DISPLAY_HEIGHT; y++)
       {
            bool p = this->pixels[(y * CHIP8_ORIGINAL_DISPLAY_WIDTH) + x];
            // If the pixel is set then draw it to the screen
            if (p)
            {
                drawPixel(x, y, true);
            }
            else
                drawPixel(x, y, false);
       }
   }

    // Update the screen
    SDL_Flip(screen);
}
