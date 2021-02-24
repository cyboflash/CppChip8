#include <iostream>
#include "Chip8Emulator.hxx"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    Chip8Emulator emu;
    
    emu.run();

    return 0;
}
