#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <SDL.h>

#include "Chip8.hxx"

struct SDL_RendererDeleter
{
    void operator()(SDL_Renderer *r);
};

class Chip8Emulator
{
    public:
        Chip8Emulator();
        ~Chip8Emulator();
        void run(void);
        void loadRom(const std::string& romPath);
    private:
                                                                                              //cols, rows
        static constexpr std::pair<uint32_t, uint32_t> SCREEN_SIZE_1280x1024 = std::make_pair(1280, 1024);
        static constexpr SDL_Color BACKGROUND_COLOR = {0, 0, 0, 255}; //Black
        static constexpr SDL_Color FOREGROUND_COLOR = {255, 255, 255, 255}; //White
        static constexpr SDL_Color DEFAULT_BLOCK_COLOR = FOREGROUND_COLOR;
        static constexpr SDL_Color CLEAR_SCREEN_COLOR = BACKGROUND_COLOR;
        static constexpr uint16_t FREQUENCY_MHZ = 500;

        class Block
        {
            public:
                Block(std::shared_ptr<spdlog::logger> logger, std::shared_ptr<SDL_Renderer> renderer,
                        int width, int height, const SDL_Color& color);
                void render(int x, int y);
                int getWidth(void) const;
                int getHeight(void) const;
            private:
                std::shared_ptr<spdlog::logger> m_Logger;
                std::shared_ptr<SDL_Renderer> m_Renderer;
                std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_Texture;
                int m_Width, m_Height;
        };

        std::unique_ptr<Chip8> cpu;
        // https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
        std::string m_LoggerName;
        std::shared_ptr<spdlog::logger> m_Logger;
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_Window;
        std::shared_ptr<SDL_Renderer> m_Renderer;

        void drawGfx(void);
        void clearScreen(void);
        uint8_t handleKeyPress(const SDL_KeyboardEvent &e);

        std::unique_ptr<Block> m_BackgroundBlock;
        std::unique_ptr<Block> m_ForegroundBlock;
};

