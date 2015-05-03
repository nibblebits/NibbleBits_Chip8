#ifndef DEF_H_INCLUDED
#define DEF_H_INCLUDED

#define CHIP8_DEBUG_MODE true
#define CHIP8_NO_DELAY true

/* Definitions that need to be used throughout the project should be declared here */
#define CHIP8_ORIGINAL_DISPLAY_WIDTH 64
#define CHIP8_ORIGINAL_DISPLAY_HEIGHT 32
#define CHIP8_RESOLUTION CHIP8_ORIGINAL_DISPLAY_WIDTH * CHIP8_ORIGINAL_DISPLAY_HEIGHT
#define CHIP8_MEMORY_SIZE 0xfff
#define CHIP8_STACK_SIZE 16
#define CHIP8_TOTAL_GENERAL_PURPOSE_REGISTERS 16
#define CHIP8_CHARSET_SIZE 80
#define CHIP8_SPEED_MS 17

typedef unsigned short u16;
typedef unsigned char u8;
typedef signed short s16;
typedef signed char s8;
typedef unsigned int u32;
typedef signed int s32;

typedef u32 PIXEL_COLOUR;

#endif // DEF_H_INCLUDED
