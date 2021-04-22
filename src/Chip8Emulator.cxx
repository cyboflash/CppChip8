#include <SDL.h>
#include <fmt/core.h>
#include <unistd.h>
#include <exception>
#include <chrono>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Chip8Emulator.hxx"

void Chip8Emulator::loadRom(const std::string& romPath)
{
    cpu->loadRom(romPath);
}

Chip8Emulator::Chip8Emulator() : 
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
    const auto& gfx = cpu->getGfx();
    for (auto row = 0; row < Chip8::GFX_ROWS; row++)
    {
        for (auto col = 0; col < Chip8::GFX_COLS; col++)
        {
                Block *const p_B = (true == gfx(row, col)) ? m_ForegroundBlock.get() : m_BackgroundBlock.get();
                p_B->render(col*p_B->getWidth(), row*p_B->getHeight());
        }
    }
}

uint8_t Chip8Emulator::handleKeyPress(const SDL_KeyboardEvent &e)
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

    uint8_t pressedKey = keyMap[e.keysym.sym];
    SPDLOG_LOGGER_TRACE(m_Logger, "Pressed {} key", pressedKey);
    cpu->setKey(pressedKey, Chip8::KEY_PRESSED_VALUE);
    return pressedKey;
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

typedef std::chrono::nanoseconds ClockResolution;
void Chip8Emulator::run(void)
{
    clearScreen();

    SDL_Event e;
    bool isQuit = false;
         // isKeyPressed = false;
    // uint8_t pressedKey;
    while(not isQuit)
    {
        while (0 != SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    isQuit = true;
                    break;

                    // https://retrocomputing.stackexchange.com/a/361
                    // https://retrocomputing.stackexchange.com/questions/358/how-are-held-down-keys-handled-in-chip-8/361#361
                case SDL_KEYUP:
                    if (SDL_RELEASED == e.key.state)
                    {
                        // pressedKey = handleKeyPress(e.key);
                        // isKeyPressed = true;
                    }
                    break;

                default:
                    break;
            }
        }

        if (isQuit)
        {
            break;
        }

        cpu->emulateCycle();

        // if(isKeyPressed)
        // {
        //     isKeyPressed = false;
        //     cpu->setKey(pressedKey, Chip8::KEY_NOT_PRESSED_VALUE);
        // }

        if (cpu->isDrw())
        {
            drawGfx();
        }

        // Update screen
        SDL_RenderPresent(m_Renderer.get());
    }
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
