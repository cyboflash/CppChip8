#include <gtest/gtest.h>
#include <fmt/core.h>
#include <fstream>
#include <ios>
#include <string>
#include <cstddef>
#include <random>
#include <limits>


#include "Chip8.hxx"

class RomWriter
{
    public:
    RomWriter(const std::string filename) : filename(filename)
    {
        rom.open(filename, std::ios::out | std::ios::binary);
    }

    RomWriter() : RomWriter("rom.ch8")
    {
    }

    void writeOp(uint16_t op)
    {
        char byte = static_cast<char>((op & 0xFF00) >> 8);
        rom.write(&byte, sizeof(byte));
        byte = static_cast<char>(op & 0x00FF);
        rom.write(&byte, sizeof(byte));
    }

    void reset(void)
    {
        done();
        rom.open(filename, std::ios::out | std::ios::binary);
    }

    void done(void)
    {
        rom.close();
    }

    std::string filename;
    std::ofstream rom;
};

struct Chip8Fixture : public ::testing::Test
{
    Chip8 chip8;
    RomWriter w;

    void SetUp(void)
    {
    }

    template <typename T = uint8_t>
    T getRandomIntValue (T minLimit, T maxLimit) {
        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<T> distribution(minLimit, maxLimit);
        return distribution(generator);
    }

    uint8_t getRandomRegister()
    {
        //register F is not used for calcualations. It is an over/underflow register
        return getRandomIntValue(0, Chip8::REGISTER_CNT-2);
    }

    uint8_t getRandomUint8()
    {
        return getRandomIntValue(0, 255);
    }

    uint16_t getRandomMemAddr()
    {
        return getRandomIntValue<uint16_t>(
                Chip8::PROGRAM_START_ADDR, 
                Chip8::PROGRAM_END_ADDR);
    }

    uint16_t getRandomUint16()
    {
        return getRandomIntValue<uint16_t>(
                0, 
                std::numeric_limits<uint16_t>::max()
                );
    }
    
    void TearDown(void) 
    {
        w.done();
    }

};


TEST_F(Chip8Fixture, TestInitialization)
{
    // EXPECT_EQ(Chip8::SP_RESET_VALUE, chip8.getSP());
    EXPECT_EQ(Chip8::PROGRAM_START_ADDR, chip8.getPC());

    for (auto i = 0; i < Chip8::REGISTER_CNT; i++)
    {
        EXPECT_EQ(Chip8::REGISTER_RESET_VALUE, chip8.getV(i));
    }

    auto sprites = chip8.readMemory(
            Chip8::FONT_SPRITES_START_ADDR, 
            Chip8::FONT_SPRITES_END_ADDR
            );

    EXPECT_EQ(
            Chip8::FONT_SPRITES_END_ADDR - Chip8::FONT_SPRITES_START_ADDR + 1,
            sprites.size())
        ;

    // Go through each sprite and verify it
    size_t i = 0;
    for (auto s : Chip8::FONT_SPRITES)
    {
        for (auto byte : s)
        {
            EXPECT_EQ(byte, sprites[i])
                << fmt::format("i = {}\n", i)
                ;
            i++;
        }
    }
}

TEST_F(Chip8Fixture, Test_op_jp)
{
    for (auto i = 0; i < 100; i++)
    {
        uint16_t nnn = getRandomMemAddr();
        uint16_t op = 0x1000 | nnn;
        w.writeOp(op);
        w.done();

        chip8.loadFile(w.filename);
        chip8.emulateCycle();

        EXPECT_EQ(nnn, chip8.getPC())
            << fmt::format("iteration: {}\n", i)
            << fmt::format("nnn: 0x{:03X}\n", nnn)
            << fmt::format("PC: {03X}", chip8.getPC())
            ;

        w.reset();
        chip8.reset();
    }

}

TEST_F(Chip8Fixture, Test_op_call)
{

    for (auto i = 0; i < 100; i++)
    {
        uint16_t nnn = getRandomMemAddr();
        uint16_t op = 0x2000 | nnn;
        w.writeOp(op);
        w.done();

        auto oldPC = chip8.getPC() + Chip8::INSTRUCTION_SIZE_B;

        chip8.loadFile(w.filename);
        chip8.emulateCycle();


        std::string err = 
            fmt::format("iteration: {}\n", i) +
            fmt::format("nnn: 0x{:03X}\n", nnn) +
            fmt::format("PC: 0x{:03X}", chip8.getPC())
            ;

        EXPECT_EQ(nnn, chip8.getPC())
            << err
            ;

        auto stack = chip8.getStack();
        EXPECT_EQ(1, stack.size())
            << err
            ;

        EXPECT_EQ(oldPC, stack.top())
            << err
            ;

        w.reset();
        chip8.reset();
    }
}

TEST_F(Chip8Fixture, Test_op_ldx)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t reg = getRandomRegister();
        uint8_t val = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | reg) << 8) | val;
            
        w.writeOp(op);
        w.done();
        chip8.loadFile(w.filename);
        chip8.emulateCycle();

        auto actualVal = chip8.getV(reg);
        auto err = fmt::format(
                "op: 0x{:04X}\n"
                "reg: {}\n"
                "val: {}\n"
                "iteration: {}\n", 
                op, reg, val, i
                );
        EXPECT_EQ(val, actualVal)
            << err
            ;


        chip8.reset();
        w.reset();
    }
}

TEST_F(Chip8Fixture, Test_op_se)
{
    for (auto i = 0; i < 100; i++)
    {
        // write a value to a register
        uint8_t reg = getRandomRegister();
        uint8_t val = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | reg) << 8) | val;

        w.writeOp(op);

        // write an op 
        op = 0x3000 | ((0x0000 | reg) << 8) | val;
        w.writeOp(op);
        w.done();

        chip8.loadFile(w.filename);

        auto oldPC = chip8.getPC();
        chip8.emulateCycle();
        chip8.emulateCycle();

        EXPECT_EQ(oldPC + Chip8::INSTRUCTION_SIZE_B*3, chip8.getPC())
            << fmt::format("Iteration: {}", i)
            ;

        chip8.reset();
        w.reset();
    }
}

TEST_F(Chip8Fixture, Test_op_sne)
{
    for (auto i = 0; i < 100; i++)
    {
        // write a value to a register
        uint8_t reg = getRandomRegister();
        uint8_t val = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | reg) << 8) | (val + 1);

        w.writeOp(op);

        // write an op 
        op = 0x4000 | ((0x0000 | reg) << 8) | val;
        w.writeOp(op);
        w.done();

        chip8.loadFile(w.filename);

        auto oldPC = chip8.getPC();
        chip8.emulateCycle();
        chip8.emulateCycle();

        EXPECT_EQ(oldPC + Chip8::INSTRUCTION_SIZE_B*3, chip8.getPC())
            << fmt::format("Iteration: {}", i)
            ;

        chip8.reset();
        w.reset();
    }
}

TEST_F(Chip8Fixture, Test_op_sker)
{
    for (auto i = 0; i < 100; i++)
    {
        // write a values to a register
        uint8_t reg1 = getRandomRegister();
        uint8_t reg2 = getRandomRegister();
        uint8_t val = getRandomUint8();

        uint16_t op = 0x6000 | ((0x0000 | reg1) << 8) | val;
        w.writeOp(op);
        op = 0x6000 | ((0x0000 | reg2) << 8) | val;
        w.writeOp(op);

        // write an op 
        op = 0x5000 | ((0x0000 | reg1) << 8) | ((0x0000 | reg2) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        auto oldPC = chip8.getPC();
        chip8.emulateCycle();
        chip8.emulateCycle();
        chip8.emulateCycle();

        EXPECT_EQ(oldPC + Chip8::INSTRUCTION_SIZE_B*4, chip8.getPC())
            << fmt::format(
                    "Iteration: {}\n"
                    "op: {:X}\n"
                    "reg1: {:X}\n"
                    "reg2: {:X}\n"
                    "val: {:X}\n"
                    "V[{}]: {:X}\n"
                    "V[{}]: {:X}\n"
                    , i, op, reg1, reg2, val , reg1, chip8.getV(reg1), reg2, chip8.getV(reg2)
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

TEST_F(Chip8Fixture, Test_op_add)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t reg = getRandomRegister();
        uint8_t val = getRandomUint8();

        uint16_t op = 0x6000 | ((0x0000 | reg) << 8) | val;
        w.writeOp(op);

        uint8_t byte = getRandomUint8();
        op = 0x7000 | ((0x0000 | reg) << 8) | byte;
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        EXPECT_EQ(static_cast<uint8_t>(val + byte), chip8.getV(reg))
            << fmt::format(
                    "Iteration: {}\n"
                    "op: {:X}\n"
                    "reg: {:X}\n"
                    "val: {}\n"
                    "byte: {}\n"
                    "V[0x{:X}]: {}\n"
                    , i, op, reg, val, byte, reg, chip8.getV(reg)
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy0 - LD Vx, Vy
// Set Vx = Vy.
//
// Stores the value of register Vy in register Vx.
TEST_F(Chip8Fixture, Test_op_ldr)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();

        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        op = 0x8000 | ((0x0000 | regX) << 8) | regY << 4;
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        EXPECT_EQ(valY, chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:Y}\n"
                    , fmt::arg("i", i), fmt::arg("op", op), fmt::arg("regX", regX),
                      fmt::arg("valY", valY), fmt::arg("regXVal", chip8.getV(regX)),
                      fmt::arg("regYVal", chip8.getV(regY))
                      )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy1 - OR Vx, Vy
// Set Vx = Vx OR Vy.
TEST_F(Chip8Fixture, Test_op_or)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        op = 0x8001 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        EXPECT_EQ(valX | valY, chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:Y}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy2 - AND Vx, Vy
// Set Vx = Vx AND Vy.
TEST_F(Chip8Fixture, Test_op_and)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        op = 0x8002 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        EXPECT_EQ(valX & valY, chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:Y}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy3 - XOR Vx, Vy
// Set Vx = Vx XOR Vy.
TEST_F(Chip8Fixture, Test_op_xor)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        op = 0x8003 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        EXPECT_EQ(valX ^ valY, chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:Y}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}
// 8xy4 - ADD Vx, Vy
// Set Vx = Vx + Vy, set VF = carry.
TEST_F(Chip8Fixture, Test_op_addr)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        op = 0x8004 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        uint16_t result = valX + valY;

        EXPECT_EQ(static_cast<uint8_t>(result), chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:X}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;
        EXPECT_EQ(static_cast<uint8_t>(result > 255), chip8.getV(0xF))
            << fmt::format(
                    "iteration: {}\n"
                    "result: {}\n"
                    ,i
                    ,result
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy5 - SUB Vx, Vy
// Set Vx = Vx - Vy, set VF = NOT borrow.
TEST_F(Chip8Fixture, Test_op_sub)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        op = 0x8005 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        EXPECT_EQ(static_cast<uint8_t>(valX - valY), chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:X}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;
        EXPECT_EQ(static_cast<uint8_t>(valX > valY), chip8.getV(0xF))
            << fmt::format(
                    "iteration: {}\n"
                    "V[0xF]: 0x{:X}\n"
                    ,i
                    ,chip8.getV(0xF)
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy6 - SHR Vx {, Vy}
// Set Vx = Vx SHR 1.
// If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
TEST_F(Chip8Fixture, Test_op_shr)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        op = 0x8006 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        // get lsb value
        uint8_t lsb = valX & 0x01;
        uint8_t regFVal = chip8.getV(0xF);
        
        EXPECT_EQ(lsb, regFVal) 
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    )
            ; 

        EXPECT_EQ(chip8.getV(regX), valX >> 1)
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "V[0x{regX:X}]: {regXVal:X}\n"
                    "V[0x{regY:X}]: {regYVal:X}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}

// 8xy7 - SUBN Vx, Vy
// Set Vx = Vy - Vx, set VF = NOT borrow.
//
// If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
TEST_F(Chip8Fixture, Test_op_subn)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        op = 0x8007 | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        uint8_t regFVal = chip8.getV(0xF);
        uint8_t regXVal = chip8.getV(regX);
        uint8_t regYVal = chip8.getV(regY);
        EXPECT_EQ((valY > valX) ? 0x01 : 0x00, regFVal) 
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "regFVal: {regFVal:X}\n"
                    "regXVal: {regXVal:X}\n"
                    "regYVal: {regYVal:X}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("regFVal", regFVal)
                    ,fmt::arg("regXVal", regXVal)
                    ,fmt::arg("regYVal", regYVal)
                    )
            ; 

        EXPECT_EQ(static_cast<uint8_t>(valY - valX), chip8.getV(regX))
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:}\n"
                    "valY: {valY:}\n"
                    "V[0x{regX:X}]: {regXVal:}\n"
                    "V[0x{regY:X}]: {regYVal:}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}
// 8xyE - SHL Vx {, Vy}
// Set Vx = Vx SHL 1.
//
// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
TEST_F(Chip8Fixture, Test_op_shl)
{
    for (auto i = 0; i < 100; i++)
    {
        uint8_t regX = getRandomRegister();
        uint8_t valX = getRandomUint8();
        uint16_t op = 0x6000 | ((0x0000 | regX) << 8) | valX;
        w.writeOp(op);

        uint8_t regY = getRandomRegister();
        uint8_t valY = getRandomUint8();
        op = 0x6000 | ((0x0000 | regY) << 8) | valY;
        w.writeOp(op);

        op = 0x800E | ((0x0000 | regX) << 8) | ((0x0000 | regY) << 4);
        w.writeOp(op);

        w.done();

        chip8.loadFile(w.filename);

        chip8.emulateCycle();
        chip8.emulateCycle();

        valX = chip8.getV(regX);
        valY = chip8.getV(regY);

        chip8.emulateCycle();

        uint8_t regFVal = chip8.getV(0xF);
        uint8_t regXVal = chip8.getV(regX);
        uint8_t regYVal = chip8.getV(regY);
        EXPECT_EQ((valX & 0x80) ? 0x01 : 0x00, regFVal) 
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "valX: {valX:X}\n"
                    "valY: {valY:X}\n"
                    "regFVal: {regFVal:X}\n"
                    "regXVal: {regXVal:X}\n"
                    "regYVal: {regYVal:X}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("regFVal", regFVal)
                    ,fmt::arg("regXVal", regXVal)
                    ,fmt::arg("regYVal", regYVal)
                    )
            ; 

        EXPECT_EQ(static_cast<uint8_t>(2*valX), regXVal)
            << fmt::format(
                    "Iteration: {i:}\n"
                    "op: {op:X}\n"
                    "regX: {regX:}\n"
                    "regY: {regY:}\n"
                    "valX: {valX:}\n"
                    "valY: {valY:}\n"
                    "V[0x{regX:X}]: {regXVal:}\n"
                    "V[0x{regY:X}]: {regYVal:}\n"
                    ,fmt::arg("i", i)
                    ,fmt::arg("op", op)
                    ,fmt::arg("regX", regX)
                    ,fmt::arg("regY", regY)
                    ,fmt::arg("valY", valY)
                    ,fmt::arg("valX", valX)
                    ,fmt::arg("regXVal", chip8.getV(regX))
                    ,fmt::arg("regYVal", chip8.getV(regY))
                    )
            ;

        chip8.reset();
        w.reset();
    }
}
