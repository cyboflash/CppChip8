#include <unistd.h>
#include <exception>
#include <chrono>
#include <thread>
#include <cmath>

#include <SDL.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Chip8Emulator.hxx"

using namespace std::chrono_literals;

void Chip8Emulator::loadRom(const std::string& romPath)
{
    cpu->loadRom(romPath);
}

Chip8Emulator::Chip8Emulator(unsigned clkHz, unsigned cycleSleep_ms) : 
    m_ClkHz{clkHz},
    m_CycleSleep_ms{cycleSleep_ms},
    m_LoggerName{fmt::format("{}-Chip8Emulator", getpid())}, 
    m_Logger{spdlog::stdout_color_mt(m_LoggerName)},
    m_Window{nullptr, SDL_DestroyWindow}
{
    m_Logger->set_level(spdlog::level::trace);

    cpu = std::make_unique<Chip8>(m_Logger);

    SPDLOG_LOGGER_TRACE(m_Logger, "Initializing SDL");
    if (0 != SDL_Init(SDL_INIT_EVERYTHING))
    {
        std::string err = fmt::format("Unable to initialize SDL: {}", SDL_GetError());
        SPDLOG_LOGGER_ERROR(m_Logger, err);
        throw std::runtime_error(err);
    }

    SPDLOG_LOGGER_TRACE(m_Logger, "Creating a window");
    m_Window.reset(SDL_CreateWindow(
                "Chip8 Emulator", 
                SDL_WINDOWPOS_CENTERED, 
                SDL_WINDOWPOS_CENTERED,
                SCREEN_SIZE_1280x1024.first,
                SCREEN_SIZE_1280x1024.second,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
                ));
    if (nullptr == m_Window)
    {
        std::string err = fmt::format("Unable to create a main window: {}", SDL_GetError());
        SPDLOG_LOGGER_ERROR(m_Logger, err);
        SDL_Quit();
        throw std::runtime_error(err);
    }

    SPDLOG_LOGGER_TRACE(m_Logger, "Creating a renderer");
    m_Renderer.reset(
            SDL_CreateRenderer(m_Window.get(), -1, SDL_RENDERER_ACCELERATED),
            SDL_RendererDeleter()
            );
    if (nullptr == m_Renderer)
    {
        std::string err = fmt::format("Unable to create a renderer for main window: {}", SDL_GetError());
        SPDLOG_LOGGER_ERROR(m_Logger, err);
        SDL_DestroyRenderer(m_Renderer.get());
        SDL_Quit();
        throw std::runtime_error(err);
    }

    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_Window.get(), &windowWidth, &windowHeight);
    m_BackgroundBlock = std::make_unique<Block>(
            m_Logger,
            m_Renderer,
            static_cast<uint32_t>(windowWidth/Chip8::GFX_COLS),
            static_cast<uint32_t>(windowHeight/Chip8::GFX_ROWS),
            BACKGROUND_COLOR
            );
    m_ForegroundBlock = std::make_unique<Block>(
            m_Logger,
            m_Renderer,
            static_cast<uint32_t>(windowWidth/Chip8::GFX_COLS),
            static_cast<uint32_t>(windowHeight/Chip8::GFX_ROWS),
            FOREGROUND_COLOR
            );
}

Chip8Emulator::~Chip8Emulator() 
{
    SDL_Quit();
}

void Chip8Emulator::drawGfx(void)
{
    const auto& updatedPixels = cpu->getUpdatedPixelsState();
    for (const auto& pixel : updatedPixels)
    {
        Block *const p_B = (true == pixel.isOn) ? m_ForegroundBlock.get() : m_BackgroundBlock.get();
        p_B->render(pixel.col*p_B->getWidth(), pixel.row*p_B->getHeight());
    }

    // Update screen
    SDL_RenderPresent(m_Renderer.get());
}

void Chip8Emulator::handleKeyboard(const SDL_Event &e)
{
    // 1 2 3 4        1 2 3 C
    // Q W E R  --->  4 5 6 D
    // A S D F        7 8 9 E
    // Z X C V        A 0 B F
    //
    // 7 8 9 0        1 2 3 C
    // u i o p  --->  4 5 6 D
    // j k l ;        7 8 9 E
    // n m , .        A 0 B F
    static std::unordered_map<SDL_Keycode, uint8_t> keyMap = 
    {
        {SDLK_1, 1  }, {SDLK_2, 2}, {SDLK_3, 3  }, {SDLK_4, 0xC},
        {SDLK_q, 4  }, {SDLK_w, 5}, {SDLK_e, 6  }, {SDLK_r, 0xD},
        {SDLK_a, 7  }, {SDLK_s, 8}, {SDLK_d, 9  }, {SDLK_f, 0xE},
        {SDLK_z, 0xA}, {SDLK_x, 0}, {SDLK_c, 0xB}, {SDLK_v, 0xF},

        {SDLK_7, 1  }, {SDLK_8, 2}, {SDLK_9, 3      }, {SDLK_0, 0xC        },
        {SDLK_u, 4  }, {SDLK_i, 5}, {SDLK_o, 6      }, {SDLK_p, 0xD        },
        {SDLK_j, 7  }, {SDLK_k, 8}, {SDLK_l, 9      }, {SDLK_SEMICOLON, 0xE},
        {SDLK_n, 0xA}, {SDLK_m, 0}, {SDLK_COMMA, 0xB}, {SDLK_PERIOD, 0xF   }
    };

    // uint8_t* keyboardState = SDL_GetKeyboardState(nullptr);

    uint8_t pressedKey = keyMap[e.key.keysym.sym];

    SPDLOG_LOGGER_TRACE(m_Logger, "Pressed {} key", pressedKey);

    cpu->setKey(pressedKey, (e.type == SDL_KEYDOWN) ? 
            Chip8::KEY_PRESSED_VALUE : Chip8::KEY_NOT_PRESSED_VALUE);
}

void Chip8Emulator::clearScreen(void)
{
    // Clear screan
    SDL_SetRenderDrawColor(m_Renderer.get(), 
            CLEAR_SCREEN_COLOR.r, 
            CLEAR_SCREEN_COLOR.g, 
            CLEAR_SCREEN_COLOR.b, 
            CLEAR_SCREEN_COLOR.a
            );
    SDL_RenderClear(m_Renderer.get());
}


void Chip8Emulator::runTimers(void)
{
    while (true)
    {
        cpu->decrementTimers();
        std::this_thread::sleep_for(Chip8::TIMER_PERIOD_mS);
    }
}

void Chip8Emulator::emulate(void)
{
    clearScreen();

    SDL_Event e;
    
    auto prevTime = std::chrono::high_resolution_clock::now();
    while(true)
    {
        auto currTime = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(currTime - prevTime);
        auto instructionCount = std::lroundf(delta.count()*static_cast<float>(m_ClkHz));
        prevTime = currTime;
        SPDLOG_LOGGER_TRACE(m_Logger, fmt::format("Instruction count: {}", instructionCount));
        for(decltype(instructionCount) cnt = 0; cnt < instructionCount; cnt++)
        {
            while (0 != SDL_PollEvent(&e))
            {
                switch (e.type)
                {
                    case SDL_QUIT:
                        goto Chip8Emulator_run_exit;
                        break;

                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        handleKeyboard(e);
                        break;

                    default:
                        break;
                }
            }

            cpu->emulateCycle();

            if (cpu->isDrw())
            {
                drawGfx();
            }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(m_CycleSleep_ms));
    }

// This might be a contreversial but I think going with goto might be
// a good idea. Makes the code a little cleaner. As 
// long as we are moving down the code and not up.  
// At least that's the rule in Linux kernel.
Chip8Emulator_run_exit:;
}

void Chip8Emulator::run(void)
{
    auto emulationThread = std::thread(&Chip8Emulator::emulate, this);
    auto timerThread = std::thread(&Chip8Emulator::runTimers, this);

    timerThread.detach();
    emulationThread.join();
    
}

Chip8Emulator::Block::Block(std::shared_ptr<spdlog::logger> logger, std::shared_ptr<SDL_Renderer> renderer,
        int width, int height, const SDL_Color& color = DEFAULT_BLOCK_COLOR) : 
    m_Logger{logger},
    m_Renderer{renderer},
    m_Texture{nullptr, SDL_DestroyTexture}, m_Width{width}, m_Height{height}
{
    SPDLOG_LOGGER_TRACE(m_Logger, "Creating a block");
    m_Texture.reset(SDL_CreateTexture(
                m_Renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                m_Width, m_Height)
            );
    SDL_Rect r{0, 0, width, height};
    SDL_SetRenderTarget(m_Renderer.get(), m_Texture.get());
    SDL_SetRenderDrawColor(m_Renderer.get(), 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(m_Renderer.get());
    SDL_SetRenderDrawColor(m_Renderer.get(), color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_Renderer.get(), &r);
    SDL_SetRenderTarget(m_Renderer.get(), nullptr);
}

void Chip8Emulator::Block::render(int x, int y)
{
    // std::string msg = fmt::format("Rendering a block at coordinates ({}, {})", x, y);
    // SPDLOG_LOGGER_TRACE(m_Logger, msg);
    SDL_Rect dst{x, y, m_Width, m_Height};
    SDL_RenderCopy(m_Renderer.get(), m_Texture.get(), nullptr, &dst);
}

int Chip8Emulator::Block::getWidth(void) const
{
    return m_Width;
}

int Chip8Emulator::Block::getHeight(void) const
{
    return m_Height;
}

void SDL_RendererDeleter::operator()(SDL_Renderer *r)
{
    SDL_DestroyRenderer(r);
}
