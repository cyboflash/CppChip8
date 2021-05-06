#include <iostream>
#include <string>
#include <cstdlib>

#include <cxxopts.hpp>

#include "Chip8Emulator.hxx"


int main(int argc, char** argv)
{
    cxxopts::Options options(std::string{argv[0]}, "Chip 8 Emulator");

    options.add_options()
        ("c,clk-hz", "Clock frequency in herz", 
         cxxopts::value<unsigned>()->default_value(
             std::to_string(Chip8Emulator::DEFAULT_CLK_HZ)))
        ("s,sleep-ms", "Amount of sleep time in ms after instruction have been executed", 
         cxxopts::value<unsigned>()->default_value(
             std::to_string(Chip8Emulator::DEFAULT_CYCLE_SLEEP_mS)))
        ("h,help", "Display usage")
        ;

    options.parse_positional({"rom-path"});

    auto result = options.parse(argc, argv);

    auto romPathCount = result.count("rom-path");
    if (result.count("help") or (0 == romPathCount) or (romPathCount > 1))
    {
        std::cout << options.help() << std::endl;
        std::exit(0);
    }

    Chip8Emulator emu(result["clk-hz"].as<unsigned>(), result["sleep-ms"].as<unsigned>());
    emu.loadRom(result["rom-path"].as<std::string>());
    emu.run();
    
    return 0;
}
