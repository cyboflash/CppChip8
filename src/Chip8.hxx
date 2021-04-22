#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <spdlog/logger.h>
#include "Bitset2D.txx"

class Chip8
{
    public:
    static constexpr uint8_t GFX_ROWS = 32;
    static constexpr uint8_t GFX_COLS = 64;

    Chip8(std::shared_ptr<spdlog::logger> logger = nullptr);
    const Bitset2D<GFX_ROWS, GFX_COLS>& getGfx(void) const;
    uint8_t getLastGeneratedRnd(void) const;
    void loadRom(const std::string& filename);
    void displayState(void) const;
    void displayMemoryContents(uint16_t startAddr = 0x0, uint16_t endAddr = 0xFFF) const;
    std::string gfxString() const;
    bool isDrw(void) const;
    void reset(void);
    void run(void);
    std::vector<uint8_t> readMemory(uint16_t startAddr, uint16_t endAddr) const;
#ifdef TEST_PACKAGE
    void writeProgramMemory(uint16_t startAddr, const std::vector<uint8_t>& data);
#endif
    uint16_t getPC() const;
    uint8_t getSP() const;
    const std::stack<uint16_t>& getStack() const;
    uint8_t getV(uint8_t nbr) const;
    uint16_t getI(void) const;
    bool getKey(uint8_t nbr);
    void setKey(uint8_t nbr, bool isPressed);
    uint8_t getDelayTimer() const;
    uint8_t getSoundTimer() const;

    void emulateCycle(void);

    static constexpr uint16_t PROGRAM_START_ADDR = 0x200; // 512
    static constexpr uint16_t PROGRAM_END_ADDR = 0xFFF; // 4095

    static constexpr uint8_t REGISTER_CNT = 16;
    static constexpr uint8_t REGISTER_RESET_VALUE = 0;

    static constexpr uint8_t SP_RESET_VALUE = static_cast<uint8_t>(-1);

    static constexpr uint16_t FONT_SPRITES_START_ADDR = 0x050;
    static constexpr uint16_t FONT_SPRITES_END_ADDR = 0x09F;
    static const std::vector<std::vector<uint8_t>> FONT_SPRITES;

    static constexpr uint8_t INSTRUCTION_SIZE_B = 2;


    static constexpr uint8_t KEYBOARD_SIZE = 16;
    static constexpr bool KEY_PRESSED_VALUE = true;
    static constexpr bool KEY_NOT_PRESSED_VALUE = false;
    static constexpr bool KEYBOARD_RESET_VALUE = KEY_NOT_PRESSED_VALUE;

    private:
    // https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
    std::string m_LoggerName;
    std::shared_ptr<spdlog::logger> m_Logger;

    typedef void (Chip8::*InstructionHandler)(void);

    // 0x000-0x1FF - Chip 8 interpreter (contains font set)
    // 0x050-0x09F - Used for the built in 8x5 pixel font set (0-F)
    // 0x200-0xFFF - Program ROM and work RAM

    static constexpr uint16_t MEMORY_SIZE_B = 4096;
    static constexpr uint16_t REGISTER_I_RESET_VALUE = 0;
    static constexpr uint8_t MEMORY_RESET_VALUE = 0;
    static constexpr uint16_t DISPLAY_REFRESH_START_ADDR = 0xF00;
    static constexpr uint16_t DISPLAY_REFRESH_END_ADDR = 0xEFF;
    // static constexpr uint16_t STACK_START_ADDR = 0xEA0;
    // static constexpr uint16_t STACK_END_ADDR = 0xEFF;
    static constexpr uint8_t STACK_SIZE = 16;
    static constexpr bool GFX_RESET_VALUE = false;

    uint8_t generateRandomUint8(void) const;

    void fetchOp(void);
    void executeOp(void);

    void displayOp(void) const; 
    void displayRegisters(void) const;

    void resetKeyboard(void);
    void resetGfx(void);
    void resetTimers(void);
    void resetStack(void);
    void resetPC(void);
    void resetMemory(void);
    void resetRegisters(void);
    void loadFont(void);
    // void emulateCycle(void);
    void incrementPC(void);
    void decrementPC(void);

    // instruction handlers
    void op_sys(void);
    void op_cls(void);    
    void op_ret(void);
    void op_jp(void);
    void op_call(void);
    void op_se(void);
    void op_sne(void);
    void op_sker(void);
    void op_ldx(void);
    void op_add(void);
    void op_ldr(void);
    void op_or(void);
    void op_and(void);
    void op_xor(void);
    void op_addr(void);
    void op_sub(void);
    void op_shr(void);
    void op_subn(void);
    void op_shl(void);
    void op_sner(void);
    void op_ldi(void);
    void op_jpr(void);
    void op_rnd(void);
    void op_drw(void);
    void op_skp(void);
    void op_sknp(void);
    void op_ldrdt(void);
    void op_ldk(void);
    void op_lddt(void);
    void op_ldst(void);
    void op_addi(void);
    void op_ldf(void);
    void op_ldb(void);
    void op_ldix(void);
    void op_ldxi(void);

    void op8(void);
    void extendedOp(void);

    void setupOpTbl(void);
    void setupOp0Tbl(void);
    void setupOp8Tbl(void);
    void setupOpETbl(void);
    void setupOpFTbl(void);
    
    std::unordered_map<uint8_t, InstructionHandler> m_op_tbl;
    std::unordered_map<uint8_t, InstructionHandler> m_op0_tbl;
    std::unordered_map<uint8_t, InstructionHandler> m_op8_tbl;
    std::unordered_map<uint8_t, InstructionHandler> m_opE_tbl;
    std::unordered_map<uint8_t, InstructionHandler> m_opF_tbl;

    uint64_t m_CycleCnt;
    std::bitset<KEYBOARD_SIZE> m_Keyboard;
    bool m_IsDrw;
    std::array<uint8_t, MEMORY_SIZE_B> m_Memory;
    uint8_t m_SP;
    uint16_t m_PC;
    std::vector<uint8_t> m_V;
    uint16_t m_I;
    std::stack<uint16_t> m_Stack;
    uint16_t m_op;
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_n;
    uint8_t m_kk;
    uint8_t m_rnd;
    uint16_t m_nnn;
    uint8_t m_OpId;
    uint8_t m_DelayTimer;
    uint8_t m_SoundTimer;
    Bitset2D<GFX_ROWS, GFX_COLS> m_Gfx;
    
};
