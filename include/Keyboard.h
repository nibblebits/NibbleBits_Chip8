#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "Def.h"
class Keyboard
{
    public:
        Keyboard();
        virtual ~Keyboard();
        bool keyDown(u16 key);
        bool keyUp(u16 key);
        void setKeyDown(u16 key);
        void setKeyUp(u16 key);
        u16 getLastKeyPressed();
    protected:
    private:
        // 0 to F are the keys available for chip8
        bool keys[0xf];
        u16 lastKeyPressed;
};

#endif // KEYBOARD_H
