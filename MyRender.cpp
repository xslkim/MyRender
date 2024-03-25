
#include <SDL3//SDL.h>
#include <Scene.hpp>
#include <json.hpp>
#include <Config.hpp>
#include <Input.hpp>


using namespace std;
using json = nlohmann::json;

// ...




int main(int argc, char* argv[])
{
    float2 a(1,1);
    Vec2f b(2, 2);
    a = b;
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER ) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    

    Vec4f v4;
    Vec2f v2;


    unsigned char* frameBuffer = new unsigned char[Config::kScreenWidth * Config::kScreenHeight * 4];
    memset(frameBuffer, 0, Config::kScreenWidth * Config::kScreenHeight * 4);
    
    SDL_Window* window = SDL_CreateWindow("MyRender", Config::kScreenWidth, Config::kScreenHeight, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL, 0);
    SDL_Texture* frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, Config::kScreenWidth, Config::kScreenHeight);

    //for (int x = 60; x < 100; ++x)
    //{
    //    for (int y = 80; y < 100; ++y)
    //    {
    //        int bufferIndex = (y * Config::kScreenWidth * 4) + x * 4;
    //        frameBuffer[bufferIndex] = 255;
    //        frameBuffer[bufferIndex + 1] = 255;
    //        frameBuffer[bufferIndex + 2] = 255;
    //        frameBuffer[bufferIndex + 3] = 255;
    //    }
    //}

    Scene scene;
    scene.ScreenBuffer = frameBuffer;
    //Config::scene_path = "assets/db/";
    //Config::scene_path = "assets/cube/";
    Config::scene_path = "assets/car/";
    //scene.Load(Config::scene_path + "db.scene.json");
    scene.Load(Config::scene_path + "car_scene_2.json");
    //scene.Load(Config::scene_path + "plane_scene.json");
    //scene.Load(Config::scene_path + "cube.scene.json");
    //scene.Load(Config::scene_path + "panel_scene2.json");

    while (true)
    {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (SDL_EVENT_QUIT == e.type)
        {
            break;
        }
        else if (e.type == SDL_EVENT_KEY_DOWN)
        {
            Input::Get().SetKeyDown(SDL_GetKeyName(e.key.keysym.sym));
            //printf("KeyDown: %s\n", SDL_GetKeyName(e.key.keysym.sym));
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseDown: x:%3.0f y:%3.0f\n", x, y);
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseUp: x:%3.0f y:%3.0f\n", x, y);
        }
        else if (e.type == SDL_EVENT_MOUSE_WHEEL)
        {
            printf("MouseWheel: y:%3.0f\n", e.wheel.y);
        }
        else if (e.type == SDL_EVENT_MOUSE_MOTION)
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            printf("MouseMove: x:%3.0f y:%3.0f\n", x, y);
        }
        
        scene.Update(17);
        scene.Render();

        if (SDL_UpdateTexture(frameTexture, NULL, frameBuffer, Config::kScreenWidth * 4) != 0) {
            printf("Error: %s\n", SDL_GetError());
        }
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
