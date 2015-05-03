#ifndef CHIP8_H
#define CHIP8_H

#include <iostream>
#include <vector>
#include <SDL/SDL.h>
#include "Def.h"

struct REGISTERS
{
        public:
            // 16 8 bit general purpose registers
            u8 V[CHIP8_TOTAL_GENERAL_PURPOSE_REGISTERS];
            // I register 16 bit
            u16 I;
            // The PC(Program counter) 16 bit holds the address that holds the instruction to execute
            u16 PC;
            // The SP(Stack Pointer) holds the address on the stack ready for when a subroutine returns
            u8 SP;

            // Delay Timer, decrements at 60Mhz while non-zero
            u16 DT;
            // Sound Timer, while non-zero will sound a buzzer and decrement at 60Mhz
            u16 ST;
};

class Display;
class Keyboard;
class Chip8
{
    public:
        Chip8();
        virtual ~Chip8();
        void Init(u32 w, u32 h, PIXEL_COLOUR pcol_on, PIXEL_COLOUR pcol_off);
        bool loadFile(char* fname);
        bool isRunning();
        bool hasQuit();
        void process();
        void reset();
        void run();
        void stop();
        bool setReg(std::string reg, u16 value);
        bool setBreakPoint(u16 location);
        bool hasBreakPoint(u16 location);
        u8 getMemory(u16 mLocation);
        REGISTERS getRegs();
    protected:
    private:
        // A sound thread to handle the bleeps separately
        static LPTHREAD_START_ROUTINE soundThread(LPVOID lpvoid);
        // Pushes a 16 bit value on to the stack then increments the "SP" by 1
        bool stack_push(u16 value);
        // Pops a 16 value off the stack then decrements the "SP" by 1
        u16 stack_pop();
        // Processes the keyboard
        void processKeyboard();
        // Process the SDL event if their is any
        void processSDLEvent();
        // Decodes and processes the current opcode
        void processOpcode();

        // A handle to the sound thread
        HANDLE sThread;

        // This is true if the chip8 is running
        bool running;
        // This is true if the chip8 has quit
        bool quit;
        // The chip 8 memory
        u8 memory[CHIP8_MEMORY_SIZE];
        // The charset
        const static u8 charset[CHIP8_CHARSET_SIZE];
        // The stack memory, chip8 uses 16 bit values and up to 16 elements
        u16 stack[CHIP8_STACK_SIZE];
        // 16 8 bit general purpose registers
        u8 V[CHIP8_TOTAL_GENERAL_PURPOSE_REGISTERS];
        // I register 16 bit
        u16 I;
        // The PC(Program counter) 16 bit holds the address that holds the instruction to execute
        u16 PC;
        // The SP(Stack Pointer) holds the address on the stack ready for when a subroutine returns
        u8 SP;

        // Delay Timer, decrements at 60Mhz while non-zero
        u16 DT;
        // Sound Timer, while non-zero will sound a buzzer and decrement at 60Mhz
        u16 ST;

        // The time of the last cycle the ticks are based on when SDL started rather than system time.
        u32 lastCycleTime;

        // This is the display of the chip8
        Display* display;

        // This is the chip8's keyboard
        Keyboard* keyboard;

        // This is the SDL event that contains information about an event by SDL
        SDL_Event sdl_event;

        // Break points, used for debugging
        std::vector<u16> breakPoints;
};

#endif // CHIP8_H
