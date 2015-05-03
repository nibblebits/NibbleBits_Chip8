#include "Keyboard.h"

Keyboard::Keyboard()
{
    lastKeyPressed = 0;
}

Keyboard::~Keyboard()
{

}

bool Keyboard::keyDown(u16 key)
{
    if (keys[key] == true)
        return true;
    return false;
}
bool Keyboard::keyUp(u16 key)
{
    if (keys[key] == false)
        return true;
    return false;
}
void Keyboard::setKeyDown(u16 key)
{
    keys[key] = true;
    this->lastKeyPressed = key;
}
void Keyboard::setKeyUp(u16 key)
{
    keys[key] = false;
}

u16 Keyboard::getLastKeyPressed()
{
    return this->lastKeyPressed;
}
