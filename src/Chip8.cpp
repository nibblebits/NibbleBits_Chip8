#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <sstream>
#include <stdio.h>
#include "Chip8.h"
#include "Display.h"
#include "Keyboard.h"

const u8 Chip8::charset[CHIP8_CHARSET_SIZE] = {0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                                               0x20, 0x60, 0x20, 0x20, 0x70, // 1
                                               0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                                               0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                                               0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                                               0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                                               0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                                               0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                                               0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                                               0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                                               0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                                               0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                                               0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                                               0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                                               0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                                               0xF0, 0x80, 0xF0, 0x80, 0x80};// F

Chip8::Chip8()
{
    display = new Display();
    keyboard = new Keyboard();

    // Initialise SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // Set the title
    SDL_WM_SetCaption("NibbleBits - Chip8 Interpreter", "");
    // Set the output and error streams back to the console as SDL_Init changes this
    freopen( "CON", "w", stdout );
}

Chip8::~Chip8()
{
    delete display;
    delete keyboard;

    // Close the sound thread
    CloseHandle(sThread);

     // Quit SDL
    SDL_Quit();
}

// A sound thread to handle the bleeps separately
LPTHREAD_START_ROUTINE Chip8::soundThread(LPVOID lpvoid)
{
    Chip8* chip8 = (Chip8*)(lpvoid);
    while (true)
    {
        REGISTERS regs = chip8->getRegs();
        if (regs.ST > 0)
        {
            Beep(400, 1000);
        }

        Sleep(100);
    }

    return 0;
}

void Chip8::Init(u32 w, u32 h, PIXEL_COLOUR pcol_on, PIXEL_COLOUR pcol_off)
{
    // Reset the chip8 Interpreter
    this->reset();

    // Copy the charset into the begining of the main memory
    memcpy(&this->memory, &this->charset, sizeof(this->charset));
    // Initialise the display
    display->Init(w, h, pcol_on, pcol_off);

    // Create the sound thread
    sThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &soundThread, (PVOID) this, (DWORD) 0, (PDWORD) 0);
}


// The "loadFile" method is in charge of loading the file into the Chip8 memory
bool Chip8::loadFile(char* fname)
{
    // Load the chip8 file into memory location 0x200
    std::ifstream file;
    file.open(fname, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        int fileSize = file.tellg();
        if (fileSize > 0xfff)
        {
            // File size is too big!
            return false;
        }

        // We have to position the pointer back to the start of the file as std::ios::ate puts it at the end
        file.seekg(file.beg);

        // Standard chip 8 programs load into memory at 0x200
        file.read((char*)&memory[0x200], fileSize);
        if (file.fail())
        {
            file.clear();
            return false;
        }
        else
        {
            // Reset the chip8
            this->reset();
            return true;
        }
    }
    else
        return false;

}

// Resets all the registers
void Chip8::reset()
{
   // 0x200 in memory is where the chip8 program is loaded, begin executing their.
   PC = 0x200;
   I = 0;
   SP = 0;
   for (int i = 0; i < 16; i++)
   {
       stack[i] = 0;
       V[i] = 0;
   }
   // Clear the display
   display->clear();
   // Stop the chip8 emulator
   this->stop();
   // Set the last cycle time
   lastCycleTime = SDL_GetTicks();
}

// Call this "run" method to run the chip8
void Chip8::run()
{
    this->running = true;
}

// Call this "stop" method to stop the chip8
void Chip8::stop()
{
    this->running = false;
}

// The "isRunning" method will return true if the chip8 is currently running, otherwise it will return false
bool Chip8::isRunning()
{
    return running;
}

// The process method should be called at frequent intervals and is in charge of processing the chip8
void Chip8::process()
{
    // Return if their is currently a break point
    if (hasBreakPoint(PC))
    {
        // Stop executing
        this->stop();
        return;
    }

    this->processSDLEvent();
    this->processOpcode();
    display->process();

    #if CHIP8_NO_DELAY == false
        /* Chip8 uses a 60 Hz clock speed.
         * Their is approximately 16.7 Ms for a 60 Hz clock speed. We will round it to 17 Ms. */

        // The total Ms it took for this cycle
       u32 msForCycle = SDL_GetTicks() - this->lastCycleTime;
       int delayTimeMs = (CHIP8_SPEED_MS - msForCycle);

       // Only if it is above 1 do we delay
       if (delayTimeMs > 0)
       {
            SDL_Delay(delayTimeMs);
       }

       this->lastCycleTime = SDL_GetTicks();
    #endif // CHIP8_NO_DELAY
}

// Returns the memory at the location specified
u8 Chip8::getMemory(u16 mLocation)
{
    if (mLocation < CHIP8_MEMORY_SIZE)
        return this->memory[mLocation];

    return 0;
}
// Pushes a 16 bit value on to the stack then increments the "SP" by 1
bool Chip8::stack_push(u16 value)
{
    bool ok = false;
    // If the SP(Stack Pointer) is greater than the stack size then their is a problem and it should be handled
    if (SP > CHIP8_STACK_SIZE-1)
    {
        // Error
        std::cout << "Problem pushing to stack" << std::endl;
        quit = true;
        ok = false;
    }
    else
    {
        // Set the memory pointed to by the "SP(Stack Pointer)" to "value"
        SP++;
        stack[SP] = value;
        ok = true;
    }

    return ok;
}
// Pops a 16 value off the stack then decrements the "SP" by 1
u16 Chip8::stack_pop()
{
    u16 value = 0;
    /* No need to check if the SP(Stack Pointer) is zero or above as "SP" is not signed but the check will be their anyway
       Just in case the data type is ever changed in the future for some reason and its then signed
    */
    if (SP >= 0 && SP < CHIP8_STACK_SIZE)
    {
        value = stack[SP];
        SP--;
    }
    else
    {
        // Error, handle the error here
        std::cout << "Problem popping from stack" << std::endl;
        quit = true;

    }

    return value;
}

bool Chip8::hasQuit()
{
    return this->quit;
}

void Chip8::processSDLEvent()
{
    if(SDL_PollEvent(&sdl_event))
    {
        if (sdl_event.type == SDL_KEYDOWN || sdl_event.type == SDL_KEYUP)
        {
           processKeyboard();
        }
        else if(sdl_event.type == SDL_QUIT)
        {
            quit = true;
        }
    }
}
void Chip8::processKeyboard()
{
    // Ideally their should be an array with 16 total elements that specify the scan codes but this will do for now
    if (sdl_event.type == SDL_KEYDOWN)
    {
        if (sdl_event.key.keysym.sym == SDLK_0)
            keyboard->setKeyDown(0);
        else if(sdl_event.key.keysym.sym == SDLK_1)
            keyboard->setKeyDown(1);
        else if(sdl_event.key.keysym.sym == SDLK_2)
            keyboard->setKeyDown(2);
        else if(sdl_event.key.keysym.sym == SDLK_3)
            keyboard->setKeyDown(3);
        else if(sdl_event.key.keysym.sym == SDLK_4)
            keyboard->setKeyDown(4);
        else if(sdl_event.key.keysym.sym == SDLK_5)
            keyboard->setKeyDown(5);
        else if(sdl_event.key.keysym.sym == SDLK_6)
            keyboard->setKeyDown(6);
        else if(sdl_event.key.keysym.sym == SDLK_7)
            keyboard->setKeyDown(7);
        else if(sdl_event.key.keysym.sym == SDLK_8)
            keyboard->setKeyDown(8);
        else if(sdl_event.key.keysym.sym == SDLK_9)
            keyboard->setKeyDown(9);
        else if(sdl_event.key.keysym.sym == SDLK_a)
            keyboard->setKeyDown(0xa);
        else if(sdl_event.key.keysym.sym == SDLK_b)
            keyboard->setKeyDown(0xb);
        else if(sdl_event.key.keysym.sym == SDLK_c)
            keyboard->setKeyDown(0xc);
        else if(sdl_event.key.keysym.sym == SDLK_d)
            keyboard->setKeyDown(0xd);
        else if(sdl_event.key.keysym.sym == SDLK_e)
            keyboard->setKeyDown(0xe);
        else if(sdl_event.key.keysym.sym == SDLK_f)
            keyboard->setKeyDown(0xf);
    }
    else if(sdl_event.type == SDL_KEYUP)
    {
        if (sdl_event.key.keysym.sym == SDLK_0)
            keyboard->setKeyUp(0);
        else if(sdl_event.key.keysym.sym == SDLK_1)
            keyboard->setKeyUp(1);
        else if(sdl_event.key.keysym.sym == SDLK_2)
            keyboard->setKeyUp(2);
        else if(sdl_event.key.keysym.sym == SDLK_3)
            keyboard->setKeyUp(3);
        else if(sdl_event.key.keysym.sym == SDLK_4)
            keyboard->setKeyUp(4);
        else if(sdl_event.key.keysym.sym == SDLK_5)
            keyboard->setKeyUp(5);
        else if(sdl_event.key.keysym.sym == SDLK_6)
            keyboard->setKeyUp(6);
        else if(sdl_event.key.keysym.sym == SDLK_7)
            keyboard->setKeyUp(7);
        else if(sdl_event.key.keysym.sym == SDLK_8)
            keyboard->setKeyUp(8);
        else if(sdl_event.key.keysym.sym == SDLK_9)
            keyboard->setKeyUp(9);
        else if(sdl_event.key.keysym.sym == SDLK_a)
            keyboard->setKeyUp(0xa);
        else if(sdl_event.key.keysym.sym == SDLK_b)
            keyboard->setKeyUp(0xb);
        else if(sdl_event.key.keysym.sym == SDLK_c)
            keyboard->setKeyUp(0xc);
        else if(sdl_event.key.keysym.sym == SDLK_d)
            keyboard->setKeyUp(0xd);
        else if(sdl_event.key.keysym.sym == SDLK_e)
            keyboard->setKeyUp(0xe);
        else if(sdl_event.key.keysym.sym == SDLK_f)
            keyboard->setKeyUp(0xf);
    }
}

// The "processOpcode" method will be in charge of decoding and processing the opcode
void Chip8::processOpcode()
{
    u16 opcode = (memory[PC] << 8) | memory[PC+1];

    PC+=2;

    // CLS
    if (opcode == 0x00E0)
    {
        display->clear();
    }
    // Return from subroutine
    else if(opcode == 0x00EE)
    {
        // Pop the address off of the stack
        u16 addr = stack_pop();
        // Set the PC(Program Counter) to the address popped off of the stack
        PC = addr;
    }
    else
    {
        u16 nnn = 0;
        u8 x = 0;
        u8 y = 0;
        u8 kk = 0;
        u8 n = 0;
        u16 t = 0;
        u16 op_h_nibble = opcode >> 12;
        switch(op_h_nibble)
        {
            case 1: // JP, Jump to memory location "nnn"
            {
                // Get the location to jump to from the opcode
                nnn = opcode & 0x0fff;
                // Set the PC(Program Counter) to the location to jump to
                PC = nnn;
            }
            break;

            case 2: // CALL, calls a subroutine at the address in memory location "nnn"
            {
                nnn = opcode & 0x0fff;
                // Push the PC(Program Counter) onto the stack, this is used to return from the sub-routine
                stack_push(PC);
                // Set the PC(Program Counter) to the new address specified in "nnn"
                PC = nnn;
            }
            break;

            case 3: // SE, Skip the next instruction if register "Vx" = "kk"
            {
                x = (opcode & 0x0f00) >> 8;
                kk = opcode & 0x00ff;

                // If the register "Vx" is equal to the value "kk" then skip the next instruction
                if (V[x] == kk)
                    PC+=2;
            }
            break;

            case 4: // SNE, Skip the next instruction if register "Vx" does not equal "kk"
            {
                x = (opcode & 0x0f00) >> 8;
                kk = opcode & 0x00ff;

                // If the register "Vx" is not equal to the value "kk" then skip the next instruction
                if (V[x] != kk)
                    PC+=2;
            }
            break;

            case 5: // SE Vx, Vy, Skip the next instruction if register "Vx" equals register "Vy"
            {
                x = (opcode & 0x0f00) >> 8;
                y = (opcode & 0x00f0) >> 4;

                // If the value in register "Vx" equals the value in register "Vy" then skip the next instruction
                if (V[x] == V[y])
                    PC+=2;
            }
            break;

            case 6: // LD, Loads the value "kk" into register "Vx"
            {
                // "x" is the register to load the value into
                x = (opcode & 0x0f00) >> 8;
                // "kk" is the value to load into the register
                kk = opcode & 0x00ff;

                // Load the value in "kk" into register "Vx"
                V[x] = kk;
            }
            break;

            case 7: // ADD, adds "kk" to "Vx"
            {
                x = (opcode & 0x0f00) >> 8;
                kk = opcode & 0x00ff;

                // Add the value in "kk" to register "Vx"
                V[x] += kk;
            }
            break;

            case 8:
            {
                x = (opcode & 0x0f00) >> 8;
                y = (opcode & 0x00f0) >> 4;
                n = opcode & 0x000f;

                if (n == 0)  // LD, store value of "Vy" into "Vx"
                {
                    // Set value of register "Vx" to value of register "Vy"
                    V[x] = V[y];
                }
                else if(n == 1) // OR, preforms a bitwize OR with "Vx" and "Vy" then stores the result in "Vx"
                {
                    // Preform bitwize OR on "Vx" and "Vy" then store the result in "Vx"
                    V[x] = V[x] | V[y];
                }
                else if (n == 2) // AND, preforms a bitwize AND with "Vx" and "Vy" then stores the result in "Vx"
                {
                    V[x] = V[x] & V[y];
                }
                else if (n == 3) // XOR, preforms a bitwize XOR with "Vx" and "Vy" then stores the result in "Vx"
                {
                    V[x] = V[x] ^ V[y];
                }
                else if(n == 4) // ADD, adds "Vx" and "Vy" together the "VF" carry flag is set if result is above 8 bits otherwise its unset. The first 8 bits of the result are then stored in "Vx"
                {
                    u16 result = V[x] + V[y];
                    if (result > 255)
                        V[0xf] = 1;
                    else
                        V[0xf] = 0;

                    V[x] = result & 0x00ff;
                }
                else if(n == 5) // SUB, if "Vx" is above "Vy" then set the "VF" carry flag otherwise its unset. Subtract "Vx" by "Vy" then store the result in "Vx"
                {
                    if (V[x] > V[y])
                        V[0xf] = 1;
                    else
                        V[0xf] = 0;

                    V[x] = V[x] - V[y];
                }
                else if(n == 6) // SHR, if the least significant bit of "Vx" is 1 then "VF" is set to 1 otherwise 0. "Vx" is then divided by 2
                {
                    // Set the carry flag if the least significant bit of "Vx" is 1 otherwise unset it
                    if (V[x] & 0x01)
                        V[0xf] = 1;
                    else
                        V[0xf] = 0;

                    // Divide Vx by two
                    V[x] /= 2;
                }

                else if(n == 7) // SUBN, If "Vy" is greater than "Vx" then the carry flag "VF" is set to 1 otherwise 0. "Vx" is then subtracted from "Vy" and the resultresult is stored in "Vx"
                {
                    if (V[y] > V[x])
                        V[0xf] = 1;
                    else
                        V[0xf] = 0;

                    V[x] = V[y] - V[x];
                }

                else if(n == 0xE) // SHL, If the most significant bit of "Vx" is 1 then then the carry flag "VF" is set to 1 otherwise 0. "Vx" is then multiplied by 2.
                {
                    if (V[x] & 0x8000)
                        V[0xf] = 1;
                    else
                        V[0xf] = 0;

                    V[x] *= 2;
                }
            }
            break;

            case 9: // SNE, skip next instruction if "Vx" does not equal "Vy"
            {
                x = (opcode & 0x0f00) >> 8;
                y = (opcode & 0x00f0) >> 4;

                if (V[x] != V[y])
                    PC+=2;
            }
            break;

            case 0xA: // LD, Set register "I" to "nnn"
            {
                nnn = opcode & 0x0fff;
                I = nnn;
            }
            break;

            case 0xB: // JP, the PC(Program Counter) is set to "nnn" added with register "V0"
            {
                nnn = opcode & 0x0fff;
                PC = nnn + V[0];
            }
            break;

            case 0xC: // RND, a random number from 0 to 255 is generated. The random number is then ANDED with the value "kk" and stored in register "Vx"
            {
                x = (opcode & 0x0f00) >> 8;
                kk = opcode & 0x00ff;

                // Generate a random number from 0 to 255 and then AND the number with the value "kk" then store in "Vx"
                V[x] = (rand() % 256) & kk;
            }
            break;

            case 0xD: // DRW, Draws a sprite to the screen at screen coordinates X: "Vx", Y: "Vy"
            {
                // Sprite array, will hold the sprite to display
                u8 sprite[16];
                // The physical positions are stored in the registers and the operands received state the registers to read from
                x = V[(opcode & 0x0f00) >> 8];
                y = V[(opcode & 0x00f0) >> 4];


                n = opcode & 0x000f;

                V[0xf] = false;

                // Load "n" bytes from memory
                for (int c = 0; c < n; c++)
                {
                    // Load sprite from memory where "I" is the segment and "c" is the offset
                    sprite[c] = memory[I + c];
                }

                for (int yc = 0; yc < n; yc++)
                {
                    for (int xc = 0; xc < 8; xc++)
                    {
                        bool s_on = (sprite[yc] & (0x80 >> xc));
                        bool p_on = display->getPixel(xc + x, yc + y);
                        // Collision detection
                        // Check is pixel is already set

                        if (s_on)
                        {
                            if (p_on)
                            {
                                V[0xf] = true;
                            }


                            // Set the pixel on the display
                            display->setPixel(xc + x, yc + y, 1, true);
                        }

                    }
                }

            }
            break;

            case 0xE:
            {
                x = (opcode & 0x0f00) >> 8;
                t = opcode & 0x00ff;
                if (t == 0x9E) // SKP, Skip the next instruction if the key with the value of "Vx" is pressed
                {
                    if (keyboard->keyDown(V[x]))
                        PC+=2;
                }
                else if(t == 0xA1) // SKNP, Skip the next instruction if the key with the value of "Vx" is not pressed
                {
                   if (keyboard->keyUp(V[x]))
                        PC+=2;
                }
            }
            break;

            case 0xF:
            {
                x = (opcode & 0x0f00) >> 8;
                t = opcode & 0x00ff;
                if (t == 0x07) // LD, Set "Vx" to the delay timer value
                {
                    V[x] = DT;
                }
                else if(t == 0x0A) // LD, waits for a key press and stores the key in the "Vx" register.
                {
                    while(true)
                    {
                        if (sdl_event.type == SDL_KEYDOWN)
                        {
                            V[x] = keyboard->getLastKeyPressed();
                            break;
                        }
                    }
                }
                else if(t == 0x15) // LD, set delay timer to the value of "Vx"
                {
                    DT = V[x];
                }
                else if(t == 0x18) // LD, set sound timer to value of "Vx"
                {
                    ST = V[x];
                }
                else if(t == 0x1E) // ADD, I is added with "Vx" and the result is stored in I
                {
                    I = V[x] + I;
                }
                else if(t == 0x29) // LD, set I to the location of the sprite specified in "x"
                {
                    // Sprites start at position "0" in memory
                    I = V[x] * 5;
                }
                else if(t == 0x33) // LD, stores BCD(Binary Coded Decimal) of Vx in memory locations I, I+1, and I+2
                {
                    int hundreds = (V[x] / 100);
                    int tens = (V[x] / 10) % 10;
                    int units = (V[x] % 10);
                    memory[I] = hundreds;
                    memory[I+1] = tens;
                    memory[I+2] = units;
                }
                else if(t == 0x55) // LD, stores registers V0 to Vx into memory starting at location I
                {
                    for (int c = 0; c <= x; c++)
                    {
                        memory[I+c] = V[c];
                    }
                }
                else if(t == 0x65) // LD, reads memory in memory location I into registers V0 to Vx
                {
                    for (int c = 0; c <= x; c++)
                    {
                        V[c] = memory[I+c];
                    }
                }
            }
            break;

            default:
                {
                    MessageBoxA(0, (LPCSTR)"Opcode not supported", (LPCSTR) "Bad opcode", 0);
                    std::cout << "Bad opcode: x" << std::hex << opcode << " at memory location: x" << std::hex << PC << std::endl;
                }
            break;
        }
    }

    // Decrement the delay timer if its non-zero
    if (DT > 0)
        DT--;

    // Decrement the sound timer if its non-zero
    if (ST > 0)
        ST--;
}

REGISTERS Chip8::getRegs()
{
    REGISTERS regs;
    regs.I = this->I;
    regs.PC = this->PC;
    regs.SP = this->SP;
    regs.ST = this->ST;
    regs.DT = this->DT;
    for (int i = 0; i < 16; i++)
    {
        regs.V[i] = this->V[i];
    }

    return regs;
}

bool Chip8::setReg(std::string reg, u16 value)
{
    bool ok = true;
    std::stringstream ss;

    // General purpose registers
    if (reg[0] == 'V' || reg[0] == 'v')
    {
      /* If the size of the register is above 2 then its also invalid, this is used for the general purpose registers
      to check their not above VF*/
        if (reg.size() > 2)
        {
            ok = false;
        }
        else
        {
            // The general purpose register to set 0-F
            u16 vReg;
            ss << reg[1];
            ss >> std::hex >> vReg;
            this->V[vReg] = value;
        }
    }
    // Other registers
    else if(reg == "I" || reg == "i")
    {
        I = value;
    }
    else if(reg == "PC" || reg == "pc")
    {
        PC = value;
    }
    else if(reg == "DT" || reg == "dt")
    {
        DT = value;
    }
    else if(reg == "ST" || reg == "st")
    {
        ST = value;
    }
    else
    {
        // Set "ok" to false if none of the registers were attempted to be set, in this case it was an invalid register
        ok = false;
    }
    return ok;
}

bool Chip8::setBreakPoint(u16 location)
{
    breakPoints.push_back(location);
    return true;
}

bool Chip8::hasBreakPoint(u16 location)
{
    for (u16 i = 0; i < breakPoints.size(); i++)
    {
        if (breakPoints[i] == location)
        {
            return true;
        }
    }

    return false;
}
