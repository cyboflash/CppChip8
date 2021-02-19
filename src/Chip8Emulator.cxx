#include <SDL.h>
#include <spdlog/spdlog.h>
#include "Chip8Emulator.hxx"

Chip8Emulator::Chip8Emulator()
{
    SDL_Init(SDL_INIT_EVERYTHING);
}
