#include <SDL3/SDL.h>
#include <Scene.hpp>
#include <Config.hpp>
#include <Input.hpp>

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    Config::scene_path = "assets/car/";

    const int W = Config::kScreenWidth;
    const int H = Config::kScreenHeight;

    unsigned char* frameBuffer = new unsigned char[W * H * 4];
    memset(frameBuffer, 0, W * H * 4);

    SDL_Window*   window       = SDL_CreateWindow("MyRender", W, H, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer     = SDL_CreateRenderer(window, NULL, 0);
    SDL_Texture*  frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                   SDL_TEXTUREACCESS_TARGET, W, H);

    Scene scene;
    scene.ScreenBuffer = frameBuffer;
    scene.Load(Config::scene_path + "car_scene_2.json");

    while (true) {
        SDL_Event e;
        SDL_PollEvent(&e);

        if (e.type == SDL_EVENT_QUIT) {
            break;
        }
        else if (e.type == SDL_EVENT_KEY_DOWN) {
            Input::Get().SetKeyDown(SDL_GetKeyName(e.key.keysym.sym));
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseDown: x:%3.0f y:%3.0f\n", x, y);
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseUp: x:%3.0f y:%3.0f\n", x, y);
        }
        else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
            printf("MouseWheel: y:%3.0f\n", e.wheel.y);
        }
        else if (e.type == SDL_EVENT_MOUSE_MOTION) {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseMove: x:%3.0f y:%3.0f\n", x, y);
        }

        scene.Update(17);
        scene.Render();

        if (SDL_UpdateTexture(frameTexture, NULL, frameBuffer, W * 4) != 0)
            printf("SDL_UpdateTexture error: %s\n", SDL_GetError());

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, frameTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    delete[] frameBuffer;
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
