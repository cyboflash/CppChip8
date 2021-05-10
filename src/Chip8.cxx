#include <filesystem>
#include <fstream>
#include <exception>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <ios>
#include <iostream>
#include <random>

#include <unistd.h>

#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Chip8.hxx"

/* Chip 8 CPU
 * Note 1: 
     Why use lambda expression instead of just creating a strig and printing it?
     If we are compiling with a level above trace then according to spdlog 
     documenation trace level macros are not compiled into the code, i.e. removed. 
     By using lambda I'm embedding code into the macro which depending on the 
     logging level will either be present or not compiled into the final binary.
     Essentially I'm removing logging from the code when I don't need it.
 * */

const std::vector<std::vector<uint8_t>> Chip8::FONT_SPRITES = 
{
    //  "0" Binary   Hex
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    { 0xF0, 0x90, 0x90, 0x90, 0xF0 },

    //  "1" Binary   Hex
    //   *  00100000 0x20
    //  **  01100000 0x60
    //   *  00100000 0x20
    //   *  00100000 0x20
    //  *** 01110000 0x70
    { 0x20, 0x60, 0x20, 0x20, 0x70 },

    //  "2" Binary   Hex
    // **** 11110000 0xF0
    //    * 00010000 0x10
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    { 0xF0, 0x10, 0xF0, 0x80, 0xF0 },

    //  "3" Binary   Hex
    // **** 11110000 0xF0
    //    * 00010000 0x10
    // **** 11110000 0xF0
    //    * 00010000 0x10
    // **** 11110000 0xF0
    { 0xF0, 0x10, 0xF0, 0x10, 0xF0 },

    //  "4" Binary   Hex
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    //    * 00010000 0x10
    //    * 00010000 0x10
    { 0x90, 0x90, 0xF0, 0x10, 0x10 },

    //  "5" Binary   Hex
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    //    * 00010000 0x10
    // **** 11110000 0xF0
    { 0xF0, 0x80, 0xF0, 0x10, 0xF0 },

    //  "6" Binary   Hex
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    { 0xF0, 0x80, 0xF0, 0x90, 0xF0 },

    //  "7" Binary   Hex
    // **** 11110000 0xF0
    //    * 00010000 0x10
    //   *  00100000 0x20
    //  *   01000000 0x40
    //  *   01000000 0x40
    { 0xF0, 0x10, 0x20, 0x40, 0x40 },

    //  "8" Binary   Hex
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    { 0xF0, 0x90, 0xF0, 0x90, 0xF0 },

    //  "9" Binary   Hex
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    //    * 00010000 0x10
    // **** 11110000 0xF0
    { 0xF0, 0x90, 0xF0, 0x10, 0xF0 },

    //  "A" Binary   Hex
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // **** 11110000 0xF0
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    { 0xF0, 0x90, 0xF0, 0x90, 0x90 },

    //  "B" Binary   Hex
    // ***  11100000 0xE0
    // *  * 10010000 0x90
    // ***  11100000 0xE0
    // *  * 10010000 0x90
    // ***  11100000 0xE0
    { 0xE0, 0x90, 0xE0, 0x90, 0xE0 },

    //  "C" Binary   Hex
    // **** 11110000 0xF0
    // *    10000000 0x80
    // *    10000000 0x80
    // *    10000000 0x80
    // **** 11110000 0xF0
    { 0xF0, 0x80, 0x80, 0x80, 0xF0 },

    //  "D" Binary   Hex
    // ***  11100000 0xE0
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    // *  * 10010000 0x90
    // ***  11100000 0xE0
    { 0xE0, 0x90, 0x90, 0x90, 0xE0 },

    //  "E" Binary   Hex
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    { 0xF0, 0x80, 0xF0, 0x80, 0xF0 },

    //  "F" Binary   Hex
    // **** 11110000 0xF0
    // *    10000000 0x80
    // **** 11110000 0xF0
    // *    10000000 0x80
    // *    10000000 0x80
    { 0xF0, 0x80, 0xF0, 0x80, 0x80 }
};

void Chip8::setKey(uint8_t nbr, bool isPressed)
{
    if (nbr >= KEYBOARD_SIZE)
    {
        std::string err = fmt::format(
                "Keyboard key: 0x{:0X} is nvalid. Valid range is [0,0x{:0X}]",
                nbr, KEYBOARD_SIZE-1
                );
        throw std::runtime_error(err);
    }
    m_PreviousKeyboard[nbr] = m_Keyboard[nbr];
    m_Keyboard[nbr] = isPressed;
}

const Bitset2D<Chip8::GFX_ROWS, Chip8::GFX_COLS>& Chip8::getGfx(void) const
{
    return m_Gfx;
}

const std::vector<Chip8::GfxPixelState>& Chip8::getUpdatedPixelsState(void) const
{
    return m_UpdatedPixels;
}

uint8_t Chip8::getLastGeneratedRnd(void) const
{
    return m_rnd;
}

uint8_t Chip8::generateRandomUint8(void) const
{

    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<uint8_t> distribution(0, 255);

    return distribution(generator);
}
#ifdef TEST_PACKAGE
void Chip8::writeProgramMemory(uint16_t startAddr, const std::vector<uint8_t>& data)
{
    if ((startAddr < PROGRAM_START_ADDR) or ((startAddr + data.size()) > PROGRAM_END_ADDR))
    {
        std::string err = "Unable to write data to memory. ";
        err += "One of the following is happening: ";
        err += "1. Starting address is less then allowed program start address, ";
        err += fmt::format("starting address: 0x{:X}, program start address: 0x{:X}\n", 
                startAddr, PROGRAM_START_ADDR);
        err += "2. Starting address is greater then allowed program end address, ";
        err += fmt::format("starting address: 0x{:X}, program end address: 0x{:X}\n",
                startAddr, PROGRAM_END_ADDR);
        err += "3. Combination of starting address and data size is greater then allowed program end address, ";
        err += fmt::format("starting address: 0x{:X}, data size: {}, program end address: 0x{:X}\n",
                startAddr, data.size(), PROGRAM_END_ADDR);

        throw std::runtime_error(err);
    }

    size_t addr = startAddr;
    for (auto byte : data)
    {
        m_Memory[addr] = byte;
        addr++;
    }
}
#endif

// I think it is Ok to return a whole vector. Compiler should be able to do
// a return value optimization, RVO
std::vector<uint8_t> Chip8::readMemory(uint16_t startAddr, uint16_t endAddr) const
{
    if ((startAddr > PROGRAM_END_ADDR) or (endAddr > PROGRAM_END_ADDR))
    {
        std::string err = "Address is outside the valid memory range. ";
        err += fmt::format("Must be between 0x{:04X} and 0x{:04X}.", PROGRAM_START_ADDR, PROGRAM_END_ADDR);
        err += fmt::format("starting address is 0x{:04X}", startAddr);
        err += fmt::format("ending address is 0x{:04X}", endAddr);
        throw std::runtime_error(err);
    }

    std::vector<uint8_t> ret(m_Memory.begin() + startAddr, m_Memory.begin() + endAddr + 1);
    return ret;
}

uint16_t Chip8::getPC() const
{
    return m_PC;
}
uint8_t Chip8::getSP() const
{
    return m_SP;
}

// I don't think it is a bad idea to return the whole stack.
// It is fairly small for
const std::stack<uint16_t>& Chip8::getStack() const
{
    return m_Stack;
}

uint8_t Chip8::getV(uint8_t nbr) const
{
    if (nbr >= REGISTER_CNT)
    {
        std::string err = "Register number must be between 0 and F. ";
        err += fmt::format("Actual value is 0x{:X}", nbr);
        throw std::runtime_error(err.c_str());
    }
    return m_V[nbr];
}

uint16_t Chip8::getI(void) const
{
    return m_I;
}

bool Chip8::getKey(uint8_t nbr) const
{    
    if (nbr >= KEYBOARD_SIZE)
    {
        std::string err = fmt::format(
                "Keyboard key: 0x{:0X} is invalid. Valid range is [0,0x{:0X}]",
                nbr, KEYBOARD_SIZE-1
                );
        throw std::runtime_error(err);
    }
    return m_Keyboard[nbr];
}

void Chip8::decrementTimers(void)
{
    if (0 != m_DelayTimer)
    {
        m_DelayTimer--;
    }

    if (0 != m_SoundTimer)
    {
        m_SoundTimer--;
    }
}

uint8_t Chip8::getDelayTimer()
{
    return m_DelayTimer;
}

uint8_t Chip8::getSoundTimer()
{
    return m_SoundTimer;
}

// 0nnn - SYS addr
// Jump to a machine code routine at nnn.
//
// This instruction is only used on the old computers on which Chip-8 was originally implemented. It is ignored by modern interpreters.
void Chip8::op_sys(void)
{
    return;
}

// 00E0 - CLS
// Clear the display.
void Chip8::op_cls(void)
{
    resetGfx();
}

// 00EE - RET
// Return from a subroutine.
// The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.
void Chip8::op_ret(void)
{
    m_PC = m_Stack.top();
    m_Stack.pop();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_SP -= 1;
#pragma GCC diagnostic pop
}


// 1nnn - JP addr
// Jump to location nnn.
// The interpreter sets the program counter to nnn.
void Chip8::op_jp(void)
{
    m_PC = m_nnn;
}

// 2nnn - CALL addr
// Call subroutine at nnn.
// The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.
void Chip8::op_call(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_SP += 1;
#pragma GCC diagnostic pop
    m_Stack.push(m_PC);
    m_PC = m_nnn;
}

// 3xkk - SE Vx, byte
// Skip next instruction if Vx = kk.
// The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
void Chip8::op_se(void)
{
    if (m_kk == m_V[m_x])
    {
        incrementPC();
    }
}


// 4xkk - SNE Vx, byte
// Skip next instruction if Vx != kk.
//
// The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.
void Chip8::op_sne(void)
{
    if (m_kk != m_V[m_x])
    {
        incrementPC();
    }
}


// 5xy0 - SE Vx, Vy
// Skip next instruction if Vx = Vy.
//
// The interpreter compares register Vx to register Vy, and if they are equal, increments the program counter by 2.
void Chip8::op_sker(void)
{
    if (m_V[m_y] == m_V[m_x])
    {
        incrementPC();
    }
}

// 6xkk - LD Vx, byte
// Set Vx = kk.
//
// The interpreter puts the value kk into register Vx.
void Chip8::op_ldx(void)
{
    m_V[m_x] = m_kk;
}

// 7xkk - ADD Vx, byte
// Set Vx = Vx + kk.
//
// Adds the value kk to the value of register Vx, then stores the result in Vx.
void Chip8::op_add(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] += m_kk;
#pragma GCC diagnostic pop
}

// 8xy0 - LD Vx, Vy
// Set Vx = Vy.
//
// Stores the value of register Vy in register Vx.
void Chip8::op_ldr(void)
{
    m_V[m_x] = m_V[m_y];
}

// 8xy1 - OR Vx, Vy
// Set Vx = Vx OR Vy.
//
// Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A bitwise OR compares the corrseponding bits from two values, and if either bit is 1, then the same bit in the result is also 1. Otherwise, it is 0.
void Chip8::op_or(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] |= m_V[m_y];
#pragma GCC diagnostic pop
}

// 8xy2 - AND Vx, Vy
// Set Vx = Vx AND Vy.
//
// Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A bitwise AND compares the corrseponding bits from two values, and if both bits are 1, then the same bit in the result is also 1. Otherwise, it is 0.
void Chip8::op_and(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] &= m_V[m_y];
#pragma GCC diagnostic pop
}

// 8xy3 - XOR Vx, Vy
// Set Vx = Vx XOR Vy.
//
// Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx. An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, then the corresponding bit in the result is set to 1. Otherwise, it is 0.
void Chip8::op_xor(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] ^= m_V[m_y];
#pragma GCC diagnostic pop
}

// 8xy4 - ADD Vx, Vy
// Set Vx = Vx + Vy, set VF = carry.
//
// The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
void Chip8::op_addr(void)
{
    uint16_t result = static_cast<uint16_t>(m_V[m_x] + m_V[m_y]);
    m_V[0xF] = static_cast<uint8_t>(result > 255);
    m_V[m_x] = static_cast<uint8_t>(result & 0x00FF);
}

// 8xy5 - SUB Vx, Vy
// Set Vx = Vx - Vy, set VF = NOT borrow.
//
// If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
void Chip8::op_sub(void)
{
    m_V[0xF] = static_cast<uint8_t>(m_V[m_x] > m_V[m_y]);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] -= m_V[m_y];
#pragma GCC diagnostic pop
}

// 8xy6 - SHR Vx {, Vy}
// Set Vx = Vx SHR 1.
//
// If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
void Chip8::op_shr(void)
{
    m_V[0xF] = m_V[m_x] & 0x01;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] >>= 1;
#pragma GCC diagnostic pop
}
//
//
// 8xy7 - SUBN Vx, Vy
// Set Vx = Vy - Vx, set VF = NOT borrow.
//
// If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
void Chip8::op_subn(void)
{
    m_V[0xF] = static_cast<uint8_t>(m_V[m_y] > m_V[m_x]);
    m_V[m_x] = static_cast<uint8_t>(m_V[m_y] - m_V[m_x]);
}
//
//
// 8xyE - SHL Vx {, Vy}
// Set Vx = Vx SHL 1.
//
// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
void Chip8::op_shl(void)
{
    m_V[0xF] = (m_V[m_x] & 0x80) ? 0x01 : 0x00;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    m_V[m_x] <<= 1;
#pragma GCC diagnostic pop
}
//
//
// 9xy0 - SNE Vx, Vy
// Skip next instruction if Vx != Vy.
//
// The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
void Chip8::op_sner(void)
{
    if (m_V[m_x] != m_V[m_y])
    {
        incrementPC();
    }
}
//
//
// Annn - LD I, addr
// Set I = nnn.
//
// The value of register I is set to nnn.
void Chip8::op_ldi(void)
{
    m_I = m_nnn;
}
//
//
// Bnnn - JP V0, addr
// Jump to location nnn + V0.
//
// The program counter is set to nnn plus the value of V0.
void Chip8::op_jpr(void)
{
    m_PC = (m_nnn + m_V[0x0]) & 0xFFF;
}
//
//
// Cxkk - RND Vx, byte
// Set Vx = random byte AND kk.
//
// The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. The results are stored in Vx. See instruction 8xy2 for more information on AND.
//
void Chip8::op_rnd(void)
{
    m_rnd = generateRandomUint8();
    m_V[m_x] = m_rnd & m_kk;
}

// Dxyn - DRW Vx, Vy, nibble
// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
//
// The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen. See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on the Chip-8 screen and sprites.
void Chip8::op_drw(void)
{
    m_V[0xF] = 0;
    m_UpdatedPixels.clear();
    for (uint8_t spriteRow = 0; spriteRow < m_n; spriteRow++)
    {
        SPDLOG_LOGGER_TRACE(m_Logger, fmt::format("Accessing memory[0x{addr:X}, {addr}]", 
                    fmt::arg("addr", m_I + spriteRow)));
        uint8_t spriteByte = m_Memory[m_I + spriteRow];
        for (uint8_t spriteCol = 0; spriteCol < 8; spriteCol++)
        {
            bool spritePixel = (0 == static_cast<uint8_t>(spriteByte & (0x80 >> spriteCol))) ? false : true;
            SPDLOG_LOGGER_TRACE(m_Logger, 
                    fmt::format("Accessing registers {} and {}", m_y, m_x));
            uint8_t gfxRow = static_cast<uint8_t>((m_V[m_y] + spriteRow) % GFX_ROWS);
            uint8_t gfxCol = static_cast<uint8_t>((m_V[m_x] + spriteCol) % GFX_COLS);

            SPDLOG_LOGGER_TRACE(m_Logger, 
                    "Accessing graphics pixel at row: {} and col: {}", gfxRow, gfxCol);
            bool oldPixel = m_Gfx(gfxRow, gfxCol);
            bool newPixel = oldPixel xor spritePixel;
            m_Gfx(gfxRow, gfxCol) = newPixel;

            if (oldPixel != newPixel)
            {
                m_UpdatedPixels.push_back({.row = gfxRow, .col = gfxCol, .isOn = newPixel});
            }

            if (0 == m_V[0xF])
            {
                // set the flag explicilty instead of relying on implicit conversion
                // of bool to int. i think it makes the code more understandable
                m_V[0xF] = ((true == oldPixel) and (false == newPixel)) ? 1 : 0;
            }
        }
    }
    m_IsDrw = true;
}

// Ex9E - SKP Vx
// Skip next instruction if key with the value of Vx is pressed.
//
// Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, PC is increased by 2.
void Chip8::op_skp(void)
{
    // the remainder operator, %, makes sure that the
    // value read from m_V[m_x] is not larger than
    // the size of the keyboard. another approach
    // would be to generate an exception. for now let's 
    // stick with the remainder
    if (m_Keyboard[m_V[m_x] % KEYBOARD_SIZE])
    {
        incrementPC();
    }
}

// ExA1 - SKNP Vx
// Skip next instruction if key with the value of Vx is not pressed.
//
// Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.
void Chip8::op_sknp(void)
{
    // the remainder operator, %, makes sure that the
    // value read from m_V[m_x] is not larger than
    // the size of the keyboard. another approach
    // would be to generate an exception. for now let's 
    // stick with the remainder
    if (not (m_Keyboard[m_V[m_x] % KEYBOARD_SIZE]))
    {
        incrementPC();
    }
}

// Fx07 - LD Vx, DT
// Set Vx = delay timer value.
//
// The value of DT is placed into Vx.
void Chip8::op_ldrdt(void)
{
    m_V[m_x] = m_DelayTimer;
}

// Fx0A - LD Vx, K
// Wait for a key press, store the value of the key in Vx.
//
// All execution stops until a key is pressed, then the value of that key is stored in Vx.
void Chip8::op_ldk(void)
{
    bool isPressed = false;
    for (uint8_t i = 0; i < KEYBOARD_SIZE; i++)
    {
        // https://retrocomputing.stackexchange.com/a/361/21550
        // Based on this post need to check if a key has been released.
        // For now will not do a timer
        if ((not m_Keyboard[i]) and m_PreviousKeyboard[i])
        {
            m_V[m_x] = i;
            isPressed = true;
            break;
        }
    }

    // if no key was pressed then go back to the previous instruction
    // this will stop all execution
    if (not isPressed)
    {
        decrementPC();
    }
}

// Fx15 - LD DT, Vx
// Set delay timer = Vx.
//
// DT is set equal to the value of Vx.
void Chip8::op_lddt(void)
{
    m_DelayTimer = m_V[m_x];
}
//
//
// Fx18 - LD ST, Vx
// Set sound timer = Vx.
//
// ST is set equal to the value of Vx.
void Chip8::op_ldst(void)
{
    m_SoundTimer = m_V[m_x];
}
//
//
// Fx1E - ADD I, Vx
// Set I = I + Vx.
//
// The values of I and Vx are added, and the results are stored in I.
void Chip8::op_addi(void)
{
    m_I = (m_I + m_V[m_x]) & 0xFFF;
}
//
//
// Fx29 - LD F, Vx
// Set I = location of sprite for digit Vx.
//
// The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal font.
void Chip8::op_ldf(void)
{
    m_I = static_cast<uint16_t>(FONT_SPRITES_START_ADDR + 5*(m_V[m_x] & 0x0F));
}
//
//
// Fx33 - LD B, Vx
// Store BCD representation of Vx in memory locations I, I+1, and I+2.
//
// The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
//
void Chip8::op_ldb(void)
{
    // make sure that I, I+1 and I+2 don't go outside the program memory boundaries
    if ((m_I + 2 > PROGRAM_END_ADDR) || (m_I < PROGRAM_START_ADDR))
    {
        std::string err = fmt::format(
                "Unable to execute 0x{op:04X} "
                "I + 2 = 0x{I:04X} + 2 = 0x{result:04X}. "
                "Result must be within valid range, [0x{start:03X}, 0x{end:03X}] "
                ,fmt::arg("op", m_op)
                ,fmt::arg("I", m_I)
                ,fmt::arg("result", m_I + 2)
                ,fmt::arg("start", PROGRAM_START_ADDR)
                ,fmt::arg("end", PROGRAM_END_ADDR)
                );

        throw std::runtime_error(err);
    }
    uint8_t value = m_V[m_x];

    uint8_t hundreds = value/100;
    value = static_cast<uint8_t>(value - hundreds*100);

    uint8_t tens = value/10;
    value = static_cast<uint8_t>(value - tens*10); // what is currently left in value are the units

    m_Memory[m_I] = hundreds;
    m_Memory[m_I + 1] = tens;
    m_Memory[m_I + 2] = value;
}
//
// Fx55 - LD [I], Vx
// Store registers V0 through Vx in memory starting at location I.
//
// The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
void Chip8::op_ldix(void)
{

    if ((m_I + m_x > PROGRAM_END_ADDR) || (m_I < PROGRAM_START_ADDR))
    {
        std::string err = fmt::format(
                "Unable to execute 0x{op:04X} "
                "I + V[0x{regX:X}] = 0x{I:04X} + 0x{valX:02X} = 0x{result:04X}. "
                "Result must be within valid range, [0x{start:03X}, 0x{end:03X}] "
                ,fmt::arg("op", m_op)
                ,fmt::arg("regX", m_x)
                ,fmt::arg("I", m_I)
                ,fmt::arg("valX", m_V[m_x])
                ,fmt::arg("result", m_I + m_V[m_x])
                ,fmt::arg("start", PROGRAM_START_ADDR)
                ,fmt::arg("end", PROGRAM_END_ADDR)
                );

        throw std::runtime_error(err);
    }

    uint16_t addr = m_I;
    for (uint8_t i = 0; i <= m_x; i++)
    {
        m_Memory[addr] = m_V[i];
        addr++;
    }
}

// Fx65 - LD Vx, [I]
// Read registers V0 through Vx from memory starting at location I.
//
// The interpreter reads values from memory starting at location I into registers V0 through Vx.
void Chip8::op_ldxi(void)
{
    // Allow reading of data before PROGRAM_START.
    if (m_I + m_x > PROGRAM_END_ADDR)
    {
        std::string err = fmt::format(
                "Unable to execute 0x{op:04X} "
                "I + V[0x{regX:X}] = 0x{I:04X} + 0x{valX:02X} = 0x{result:04X}. "
                "Result must be within valid range, [0x0, 0x{end:03X}] "
                ,fmt::arg("op", m_op)
                ,fmt::arg("regX", m_x)
                ,fmt::arg("I", m_I)
                ,fmt::arg("valX", m_V[m_x])
                ,fmt::arg("result", m_I + m_V[m_x])
                ,fmt::arg("end", PROGRAM_END_ADDR)
                );

        throw std::runtime_error(err);
    }

    uint16_t addr = m_I;
    for (uint8_t i = 0; i <= m_x; i++)
    {
        m_V[i] = m_Memory[addr];
        addr++;
    }
}

void Chip8::run(void)
{
    emulateCycle();
}

Chip8::Chip8(std::shared_ptr<spdlog::logger> logger)
{
    if (nullptr == logger)
    {
        m_LoggerName = fmt::format("{}-Chip8", getpid());
        m_Logger = spdlog::stdout_color_mt(m_LoggerName);
    }
    else
    {
        m_Logger = logger;
    }

    setupOpTbl();
    reset();
}

void Chip8::setupOpTbl(void)
{
    setupOp0Tbl();
    setupOp8Tbl();
    setupOpETbl();
    setupOpFTbl();

    m_op_tbl = 
    {
        {0x0, &Chip8::extendedOp },
        {0x1, &Chip8::op_jp      },
        {0x2, &Chip8::op_call    },
        {0x3, &Chip8::op_se      },
        {0x4, &Chip8::op_sne     },
        {0x5, &Chip8::op_sker    },
        {0x6, &Chip8::op_ldx     },
        {0x7, &Chip8::op_add     },
        {0x8, &Chip8::op8        },
        {0x9, &Chip8::op_sner    },
        {0xA, &Chip8::op_ldi     },
        {0xB, &Chip8::op_jpr     },
        {0xC, &Chip8::op_rnd     },
        {0xD, &Chip8::op_drw     },
        {0xE, &Chip8::extendedOp },
        {0xF, &Chip8::extendedOp },
    };
}

void Chip8::extendedOp(void)
{
    std::unordered_map<uint8_t, InstructionHandler>* op_tbl{nullptr};
    switch(m_OpId)
    {
        case 0x0:
            op_tbl = &m_op0_tbl;
            break;
        case 0xE:
            op_tbl = &m_opE_tbl;
            break;
        case 0xF:
            op_tbl = &m_opF_tbl;
            break;
        default:
            std::string err = fmt::format(
                    "Unknown op identifier, 0x{:0X}", m_OpId
                    );
            throw std::runtime_error(err);
    }

    (this->*(op_tbl->at(m_kk)))();
}

void Chip8::op8(void)
{
    (this->*m_op8_tbl.at(m_n))();
}

void Chip8::setupOp0Tbl(void)
{
    m_op0_tbl = 
    {
        {0xE0, &Chip8::op_cls},
        {0xEE, &Chip8::op_ret},
    };
}

void Chip8::setupOp8Tbl(void)
{
    m_op8_tbl = 
    {
        {0x0, &Chip8::op_ldr },
        {0x1, &Chip8::op_or  },
        {0x2, &Chip8::op_and },
        {0x3, &Chip8::op_xor },
        {0x4, &Chip8::op_addr},
        {0x5, &Chip8::op_sub },
        {0x6, &Chip8::op_shr },
        {0x7, &Chip8::op_subn},
        {0xE, &Chip8::op_shl },
    };
}

void Chip8::setupOpETbl(void)
{
    m_opE_tbl = 
    {
        {0x9E, &Chip8::op_skp },
        {0xA1, &Chip8::op_sknp},
    };
}

void Chip8::setupOpFTbl(void)
{
    m_opF_tbl = 
    {
        {0x07, &Chip8::op_ldrdt},
        {0x0A, &Chip8::op_ldk  },
        {0x15, &Chip8::op_lddt },
        {0x18, &Chip8::op_ldst },
        {0x1E, &Chip8::op_addi },
        {0x29, &Chip8::op_ldf  },
        {0x33, &Chip8::op_ldb  },
        {0x55, &Chip8::op_ldix },
        {0x65, &Chip8::op_ldxi },
    };
}

void Chip8::resetKeyboard(void)
{
    m_Keyboard.reset();
    m_PreviousKeyboard.reset();
}

void Chip8::reset(void)
{
    m_IsDrw = false;
    m_CycleCnt = 0;

    resetKeyboard();
    resetPC();
    resetStack();
    resetRegisters();
    resetTimers();
    resetMemory();
    resetGfx();
}

void Chip8::resetGfx(void)
{
    m_Gfx.reset();
}

void Chip8::emulateCycle(void)
{
    m_IsDrw = false;

    fetchOp();
    incrementPC();
    executeOp();

    m_CycleCnt++;
}

void Chip8::executeOp(void)
{
    try
    {
        displayState();
        (this->*m_op_tbl.at(m_OpId))();
    }
    catch (const std::out_of_range &e)
    {
        std::string err = fmt::format(
                "Unsupported opcode: 0x{:04X}", m_op
                );

        m_Logger->error(err);
        throw;
    }
}

void Chip8::incrementPC(void)
{
    m_PC = (m_PC + INSTRUCTION_SIZE_B) & 0x0FFF;
}

void Chip8::decrementPC(void)
{
    m_PC = (m_PC - INSTRUCTION_SIZE_B) & 0xFFF;
}

void Chip8::fetchOp(void)
{
    m_op = static_cast<uint16_t>((m_Memory[m_PC] << 8 ) | m_Memory[m_PC + 1]);

    // decode op
    m_OpId = static_cast<uint8_t>((m_op & 0xF000) >> 12);
    m_x    = (m_op & 0x0F00) >> 8;
    m_y    = (m_op & 0x00F0) >> 4;
    m_n    =  m_op & 0x000F;
    m_kk   = static_cast<uint8_t>(m_op & 0x00FF);
    m_nnn  =  m_op & 0x0FFF;
}

void Chip8::resetTimers(void)
{
    m_DelayTimer = 0;
    m_SoundTimer = 0;
}

void Chip8::resetRegisters(void)
{
    m_V.resize(REGISTER_CNT);
    std::fill(m_V.begin(), m_V.end(), REGISTER_RESET_VALUE);
    m_I = REGISTER_I_RESET_VALUE;
}

void Chip8::resetStack(void)
{
    m_Stack = std::stack<uint16_t>();
    m_SP = -1;
}

void Chip8::resetPC(void)
{
    m_PC = PROGRAM_START_ADDR;
}

void Chip8::loadRom(const std::string& filename)
{
    // TODO:
    // check if file exists
    // check if able to load the file
    // Read https://gehrcke.de/2011/06/reading-files-in-c-using-ifstream-dealing-correctly-with-badbit-failbit-eofbit-and-perror/

    std::ifstream rom;
    rom.open(filename, std::ifstream::in | std::ifstream::binary);

    if (not rom.good())
    {
        throw std::runtime_error("Unable to open " + filename);
    }

    // Stop eating new lines in binary mode!!! Just a little paranoia :). std::ifsteam::binary should
    // not ignore whitespace, but based on my online reading it doesn't always happen.
    rom.unsetf(std::ios::skipws);

    rom.read(reinterpret_cast<char *>(&m_Memory[PROGRAM_START_ADDR]), PROGRAM_END_ADDR - PROGRAM_START_ADDR + 1);
}
void Chip8::resetMemory(void)
{
    // do this in case large rom was loaded. this is a precaution.
    std::fill(m_Memory.begin(), m_Memory.begin() + FONT_SPRITES_START_ADDR, MEMORY_RESET_VALUE);
    std::fill(m_Memory.begin() + FONT_SPRITES_END_ADDR + 1, m_Memory.end(), MEMORY_RESET_VALUE);

    loadFont();
}

void Chip8::loadFont(void)
{
    auto memoryOffset = FONT_SPRITES_START_ADDR;
    for (auto sprite : FONT_SPRITES)
    {
        for (auto byte : sprite)
        {
            m_Memory[memoryOffset] = byte;
            memoryOffset++;
        }
    }
}

bool Chip8::isDrw() const
{
    return m_IsDrw;
}

std::string Chip8::gfxString() const
{
    // need extra GFX_ROWS-1  for new lines
    std::string output(GFX_ROWS*GFX_COLS + GFX_ROWS-1, '\n');

    // the size of the output is slightly larger than the size of the gfx screen
    // because we need to add new lines to the output
    unsigned strOffset = 0;
    for (std::size_t row = 0; row < GFX_ROWS; row++)
    {
        for(std::size_t col = 0; col < GFX_COLS; col++)
        {
            output[row*GFX_COLS + col + strOffset] = m_Gfx(row, col) ? '*' : ' ';
        }
        strOffset++;
    }

    return output;
}

void Chip8::displayOp(void) const
{
    SPDLOG_LOGGER_TRACE(m_Logger, 
            fmt::format(
                "m_op: 0x{:>04X}"
                ", m_OpId: 0x{:>01X}"
                ", m_x: 0x{:>01X}"
                ", m_y: 0x{:>01X}"
                ", m_n: 0x{:>01X}"
                ", m_kk: 0x{:>02X}"
                ", m_nnn: 0x{:>03X}"
                , m_op 
                , m_OpId
                , m_x
                , m_y
                , m_n
                , m_kk
                , m_nnn
                )
            );
}

void Chip8::displayRegisters(void) const
{
    SPDLOG_LOGGER_TRACE
    (
        m_Logger,
        // See Note 1 in the beginning on this file to read on why I'm using lambda here.
        [&]()
        {
            std::string result = fmt::format(fmt::format("\nPC = 0x{:>03X}", m_PC));
            result += fmt::format("\nSP = {}", m_SP);
            for (decltype(m_V.size()) i = 0; i < m_V.size(); i++)
            {
                result += fmt::format("\nV[0x{:>01X}] = 0x{:>02X}", i, m_V[i]);
            }
            // remove last character, new line, '\n'
            result.pop_back();
            return result;
        }()
    );
}

void Chip8::displayState(void) const
{
    displayOp();
    displayRegisters();
    displayMemoryContents();
}

// When code is compiled for release the logging macro is eliminated, which in turn
// makes function arguments not used by anything. This in turn generates an error:
// error: unused parameter [-Werror=unused-parameter]. Hence the use of [[maybe_unused]]
// attribute.
void Chip8::displayMemoryContents(
        [[maybe_unused]] uint16_t startAddr, 
        [[maybe_unused]] uint16_t endAddr) const
{
    SPDLOG_LOGGER_TRACE
    (
        m_Logger, 
        // See Note 1 in the beginning on this file to read on why I'm using lambda here.
        [&]()
        {
            if ((startAddr > PROGRAM_END_ADDR) or (endAddr > PROGRAM_END_ADDR))
            {
                throw std::runtime_error(
                        "startAddr or endAddr is greater than memory size");
            }

            // display header
            uint16_t nearestQuotientInteger = static_cast<uint16_t>((startAddr/16) * 16);
            std::string result = "\n        ";
            for (auto i = 0; i < 16; i++)
            {
                result += fmt::format("0x{:02X} ", i);
            }
            result += "\n      +-";
            result += std::string(16*4 + 15, '-');

            // https://stackoverflow.com/a/19760152/1636521 
            uint16_t remainder = startAddr & (16 - 1);
            if (nearestQuotientInteger != startAddr)
            {
                result += fmt::format("\n0x{:03X} | ", nearestQuotientInteger);
                for (auto i = 0; i < remainder; i++)
                {
                    result += "     ";
                }
            }

            for (uint16_t i = startAddr; i <= endAddr; i++)
            {
                // figure out if we need to display the address 
                if (0 == (i & (16 - 1)))
                {
                    result += fmt::format("\n0x{:03X} | ", i);
                }

                // display memory contents
                result += fmt::format("0x{:02X} ", m_Memory[i]);
            }
            return result;
        }()
    );
}
