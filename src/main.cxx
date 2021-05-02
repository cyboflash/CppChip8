#include <iostream>
#include <string>

#include "Chip8Emulator.hxx"


int main([[maybe_unused]] int argc, char** argv)
{

    Chip8Emulator emu(static_cast<unsigned>(std::stoul(std::string(argv[2]))));
    emu.loadRom(std::string(argv[1]));

    emu.run();
    
    return 0;
}
