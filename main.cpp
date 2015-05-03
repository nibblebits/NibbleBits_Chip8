#include <memory>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include "Chip8.h"
using namespace std;

 std::shared_ptr<Chip8> chip8;

u32 getHexOrDecFromTerminal()
{
    u8 prefix;
    u32 value = -1;
    cin >> prefix;
    if (prefix == 'x')
    {
        cin >> std::hex >> value;
    }
    else if(prefix == 'd')
    {
        cin >> std::dec >> value;
    }
    else
    {
        cout << "Bad formatting use 'd1024' for decimal or 'x200' for hexadecimal" << endl;
    }
    return value;
}

void terminal_command()
{
    std::string command;
    std::string reg;
    cout << ">";
    cin >> command;
        if (command == "run")
        {
            chip8->run();
        }
        else if(command == "stop")
        {
            chip8->stop();
        }
        else if(command == "memory")
        {
            u16 memLoc = getHexOrDecFromTerminal();
            if (memLoc != -1)
            {
                u8 value = chip8->getMemory(memLoc);
                cout << "Value: x" << std::hex << (int)value << endl;
            }
        }
        else if (command == "regs")
        {
            REGISTERS regs = chip8->getRegs();
            cout << "General Purpose Registers: " << endl;
            for (int i = 0; i < 16; i++)
            {
                cout << "   Register: V" << std::hex << i << " = " << std::dec << (int)regs.V[i] << endl;
            }

                cout << "PC = " << "x" << std::hex << regs.PC << ", SP = x" << std::hex << (int)regs.SP << ", I = "
                                << std::hex << "x" << regs.I << ", ST = x" << std::hex << regs.ST << ", DT = x" << std::hex << regs.DT << endl;
            } else if(command == "set")
        {
           cin >> reg;
           u16 value = getHexOrDecFromTerminal();
           if (value != -1)
           {
               bool ok = chip8->setReg(reg, value);
               if (!ok)
               {
                   cout << "Bad register" << endl;
               }
           }
        } else if(command == "break")
        {
            // The memory location in chip8 to break at
            u16 loc = getHexOrDecFromTerminal();
            if (loc != -1)
            {
                chip8->setBreakPoint(loc);
            }
        } else if(command == "continue")
        {
            REGISTERS regs = chip8->getRegs();
            regs.PC+=2;
            chip8->setReg("PC", regs.PC);
            // Run chip8 again
            chip8->run();
        } else if(command == "help")
        {
            std::cout << "run ; Runs the Chip8 Program currently set" << std::endl
                      << "stop ; Stops the Chip8 Program currently set" << std::endl
                      << "memory xff ; View the memory at a hexadecimal location, use 'd' instead of 'x' for decimal locations" << std::endl
                      << "regs ; Displays all the registers of the Chip8 and their values" << std::endl
                      << "set V0 xff ; Set a Chip8 register where 'V0' is register 'V0' and 'xff' is hexadecimal value to set 'V0' to. Use 'd' instead of 'x' for decimal values." << std::endl
                      << "break xff ; Set a break point at either a hexadecimal location or a decimal location. Use 'd' for decimal and 'x' for hexadecimal" << std::endl
                      << "continue ; Continue running the program after a breakpoint." << std::endl
                      << "help ; Display the functions that are possible to use" << std::endl;

        }

}
/* The breakListener thread listens to any breaks their may be in the chip8*/
LPTHREAD_START_ROUTINE breakListener(LPVOID lpvoid)
{
    s32 lastAddress = -1;
    while(true)
    {
        REGISTERS regs = chip8->getRegs();
        // Check to see if their is a break point on the currently executing instruction
        if (lastAddress != regs.PC && chip8->hasBreakPoint(regs.PC))
        {
            cout << "Break on " << regs.PC << " use command 'continue' to continue executing" << endl;
        }

        lastAddress = regs.PC;
    }

    return 0;
}

/* In this function we process all input from the console and provide a kind of terminal to the chip8 interpreter
 * this terminal can be used for debugging purposes ect.*/
 LPTHREAD_START_ROUTINE terminal(LPVOID lpvoid)
 {
    cout << "Terminal for the Chip8 emulator, type 'help' for more information, use command run to start emulating" << std::endl;
    while(true)
    {
        terminal_command();
    }
    return 0;
 }

int main(int argc, char* argv[])
{
    /* Check to see if their is no arguments, if so then display a message and return
     * a file is required to be passed to this program this file should be the file to load*/
    if (argc <= 1)
    {
        MessageBoxA(0, (LPCSTR)"You must specify a chip8 file to load", (LPCSTR)"Chip8 file required", 0);
        return 1;
    }

    chip8 = std::make_shared<Chip8>();
    // Initialise the chip8
    chip8->Init(1024, 512, 0xffffffff, 0x00000000);

    // Load the chip8 file
    if(!chip8->loadFile(argv[1]))
    {
        MessageBoxA(0, (LPCSTR)"Failed to load chip8 file", (LPCSTR)"Loading error", 0);
        exit(1);
    }

    #if CHIP8_DEBUG_MODE == true
        // Start the terminal
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &terminal, (PVOID) 0, (DWORD) 0, (PDWORD) 0);
        // Listen for break points
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &breakListener, (PVOID) 0, (DWORD) 0, (PDWORD) 0);
        #else
            chip8->run();
            std::cout << "Debug mode is off. " << std::endl <<
             "Recompile with debug mode on to access the terminal and debugger" << std::endl;
    #endif // CHIP8_DEBUG_MODE

    while(!chip8->hasQuit())
    {
        if (chip8->isRunning())
        {
            // Process the chip8 file
            chip8->process();
        }
    }

    return 0;
}
