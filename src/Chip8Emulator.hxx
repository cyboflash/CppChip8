#pragma once
#include "Chip8.hxx"
class Chip8Emulator
{
    public:
        Chip8Emulator();
        void run(void);
    private:
        Chip8 cpu;
};
