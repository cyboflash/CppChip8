#include <iostream>
#include "Chip8Emulator.hxx"

int main([[maybe_unused]] int argc, char** argv)
{
    Chip8Emulator emu;
    emu.loadRom(std::string(argv[1]));
    
    emu.run();

    return 0;
}
