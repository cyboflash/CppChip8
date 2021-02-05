#include <iostream>
#include "Chip8.hxx"

int main([[maybe_unused]] int argc, char** argv)
{
    Chip8 chip8;
    chip8.loadFile(argv[1]);
    
    // char k = '\0';
    // while ('q' != k)
    while (true)
    {
        chip8.emulateCycle();
        if (chip8.isDrw())
        {
            chip8.displayGfx();
            // std::cin >> k;
        }
    }

    return 0;
}
