#include <SDL.h>
#include <fmt/core.h>
#include <unistd.h>
#include <exception>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "Chip8Emulator.hxx"

void Chip8Emulator::loadRom(const std::string& romPath)
{
    cpu.loadRom(romPath);
}

Chip8Emulator::Chip8Emulator() : 
    m_LoggerName{fmt::format("{}-Chip8Emulator", getpid())}, 
    m_Logger{spdlog::stdout_color_mt(m_LoggerName)},
    m_Window{nullptr, SDL_DestroyWindow}
{
    m_Logger->set_level(spdlog::level::debug);

    m_Logger->debug("Initializing SDL");
    if (0 != SDL_Init(SDL_INIT_EVERYTHING))
    {
        std::string err = fmt::format("Unable to initialize SDL: {}", SDL_GetError());
        m_Logger->error(err);
        throw std::runtime_error(err);
    }

    m_Logger->debug("Creating a window");
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
        m_Logger->error(err);
        SDL_Quit();
        throw std::runtime_error(err);
    }

    m_Logger->debug("Creating a renderer");
    m_Renderer.reset(
            SDL_CreateRenderer(m_Window.get(), -1, SDL_RENDERER_ACCELERATED),
            SDL_RendererDeleter()
            );
    if (nullptr == m_Renderer)
    {
        std::string err = fmt::format("Unable to create a renderer for main window: {}", SDL_GetError());
        m_Logger->error(err);
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

void Chip8Emulator::run(void)
{
    // Clear screan
    SDL_SetRenderDrawColor(m_Renderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer.get());

    SDL_Event e;
    bool isQuit = false;
    while(not isQuit)
    {
        while (0 != SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    isQuit = true;
                    break;
                default:
                    break;
            }
        }

        if (isQuit)
        {
            break;
        }

        cpu.emulateCycle();

        if (cpu.isDrw())
        {
            const auto& gfx = cpu.getGfx();
            uint8_t rowIdx = 0; 
            for (auto row : gfx)
            {
                uint8_t colIdx = 0;
                for (auto pixel : row)
                {
                    m_Logger->debug(fmt::format("Drawing pixel at ({}, {})", rowIdx, colIdx));
                    Block *const p_B = (true == pixel) ? m_ForegroundBlock.get() : m_BackgroundBlock.get();
                    p_B->render(colIdx*p_B->getWidth(), rowIdx*p_B->getHeight());
                    colIdx++;
                }
                rowIdx++;
            }
            rowIdx = 0;
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
    m_Logger->debug("Creating a block");
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
    // m_Logger->debug(msg);
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
