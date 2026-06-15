#include <SDL3/SDL.h>
#include <Scene.hpp>
#include <Config.hpp>
#include <Input.hpp>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

// ---------------------------------------------------------------------------
// Headless screenshot capture (used to generate tutorial visuals).
// Writes 24-bit BMP files; no window required.
// ---------------------------------------------------------------------------

static void writeBMP(const std::string& path, const unsigned char* rgba, int w, int h)
{
    const int rowSize  = (w * 3 + 3) & ~3;
    const int dataSize = rowSize * h;
    const int fileSize = 54 + dataSize;

    std::vector<unsigned char> file(fileSize, 0);
    unsigned char* p = file.data();
    p[0] = 'B'; p[1] = 'M';
    *(int32_t*)(p + 2)  = fileSize;
    *(int32_t*)(p + 10) = 54;
    *(int32_t*)(p + 14) = 40;
    *(int32_t*)(p + 18) = w;
    *(int32_t*)(p + 22) = h;
    *(int16_t*)(p + 26) = 1;
    *(int16_t*)(p + 28) = 24;
    *(int32_t*)(p + 34) = dataSize;

    unsigned char* data = p + 54;
    for (int y = 0; y < h; ++y) {
        const unsigned char* src = rgba + (size_t)(h - 1 - y) * w * 4; // BMP is bottom-up
        unsigned char*       dst = data + (size_t)y * rowSize;
        for (int x = 0; x < w; ++x) {
            dst[x * 3 + 0] = src[x * 4 + 2]; // B
            dst[x * 3 + 1] = src[x * 4 + 1]; // G
            dst[x * 3 + 2] = src[x * 4 + 0]; // R
        }
    }

    FILE* f = fopen(path.c_str(), "wb");
    if (f) { fwrite(file.data(), 1, fileSize, f); fclose(f); }
}

static void runCapture(const std::string& outDir)
{
    const int W = Config::kScreenWidth;
    const int H = Config::kScreenHeight;
    unsigned char* buf = new unsigned char[W * H * 4];

    gpu::g_bilinear = false; // tutorial visuals were authored with point sampling

    Scene scene;
    scene.ScreenBuffer = buf;
    scene.LoadLegacy(Config::scene_path + "car_scene_2.json");

    auto shot = [&](const std::string& name, int dv) {
        gpu::g_debugView = dv;
        scene.Render();
        writeBMP(outDir + "/" + name + ".bmp", buf, W, H);
        printf("shot: %s\n", name.c_str());
    };

    // --- Single-object pipeline views at the default camera angle ---
    shot("final_front",   gpu::DV_NONE);
    shot("albedo",        gpu::DV_ALBEDO);
    shot("normal_geom",   gpu::DV_NORMAL_GEOM);
    shot("normal_mapped", gpu::DV_NORMAL_MAPPED);
    shot("uv",            gpu::DV_UV);
    shot("wire",          gpu::DV_WIRE);

    // --- Multi-thread strip visualization ---
    Render::Get().SetNumThreads(Render::Get().GetHardwareThreads());
    shot("threads", gpu::DV_THREADS);

    // --- Depth buffer -> auto-contrast grayscale (near = white) ---
    gpu::g_debugView = gpu::DV_NONE;
    scene.Render();
    {
        float dmin = 1e9f, dmax = -1e9f;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                float d = Render::Get().GetDepth(x, y);
                if (d < 1.0f) { dmin = std::min(dmin, d); dmax = std::max(dmax, d); }
            }
        if (dmax <= dmin) { dmin = 0.0f; dmax = 1.0f; }

        std::vector<unsigned char> d8((size_t)W * H * 4, 255);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                float d = Render::Get().GetDepth(x, H - 1 - y); // match color Y-flip
                float t = (d >= 1.0f) ? 0.0f : (1.0f - (d - dmin) / (dmax - dmin));
                unsigned char v = (unsigned char)(gpu::saturate(t) * 255.0f);
                int i = (y * W + x) * 4;
                d8[i] = v; d8[i + 1] = v; d8[i + 2] = v;
            }
        writeBMP(outDir + "/depth.bmp", d8.data(), W, H);
        printf("shot: depth\n");
    }

    // --- Orbit montage (final render at four angles) ---
    {
        float a0 = scene.GetOrbitAngleDeg();
        gpu::g_debugView = gpu::DV_NONE;
        for (int i = 0; i < 4; ++i) {
            scene.SetOrbitAngleDeg(a0 + i * 90.0f);
            scene.Render();
            char nm[64];
            snprintf(nm, sizeof(nm), "orbit_%d", i);
            writeBMP(outDir + "/" + nm + ".bmp", buf, W, H);
            printf("shot: %s\n", nm);
        }
        scene.SetOrbitAngleDeg(a0);
    }

    // --- Clipping demo: narrow FOV overflows the frustum; wireframe shows cuts ---
    {
        Camera& cam   = scene.GetCamera();
        float   saved = cam.Fov;
        cam.Fov = 20.0f;
        shot("wire_clip", gpu::DV_WIRE);
        cam.Fov = saved;
    }

    // --- Single vs. multi-thread timing ---
    {
        gpu::g_debugView = gpu::DV_NONE;
        auto bench = [&](unsigned n) {
            Render::Get().SetNumThreads(n);
            const int N = 30;
            auto t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < N; ++i) scene.Render();
            auto t1 = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double, std::milli>(t1 - t0).count() / N;
        };
        unsigned hw   = Render::Get().GetHardwareThreads();
        double   one  = bench(1);
        double   many = bench(hw);
        printf("BENCH single=%.2fms multi(%u)=%.2fms speedup=%.2fx\n", one, hw, many, one / many);
        FILE* f = fopen((outDir + "/bench.txt").c_str(), "w");
        if (f) {
            fprintf(f, "single_thread_ms=%.2f\nthreads=%u\nmulti_thread_ms=%.2f\nspeedup=%.2f\n",
                    one, hw, many, one / many);
            fclose(f);
        }
    }

    delete[] buf;
    printf("capture done -> %s\n", outDir.c_str());
}

// Headless one-shot of a Unity-exported scene -> single BMP. No window.
static void runCaptureUnity(const std::string& sceneDir, const std::string& outFile, int debugView = gpu::DV_NONE)
{
    const int W = Config::kScreenWidth;
    const int H = Config::kScreenHeight;
    unsigned char* buf = new unsigned char[W * H * 4];

    Config::scene_path = sceneDir;
    if (Config::scene_path.back() != '/' && Config::scene_path.back() != '\\')
        Config::scene_path += '/';

    Scene scene;
    scene.ScreenBuffer = buf;
    scene.LoadUnity("scene.json");
    gpu::g_debugView = debugView;
    scene.Render();
    writeBMP(outFile, buf, W, H);
    printf("unity capture -> %s\n", outFile.c_str());
    delete[] buf;
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    Config::scene_path = "assets/car/";

    // Headless screenshot mode: MyRender.exe --capture [outDir]
    if (argc > 1 && std::string(argv[1]) == "--capture") {
        std::string outDir = (argc > 2) ? argv[2] : "tech_doc/shots";
        runCapture(outDir);
        SDL_Quit();
        return 0;
    }

    // Headless Unity one-shot: MyRender.exe --capture-unity [sceneDir] [outFile]
    if (argc > 1 && std::string(argv[1]) == "--capture-unity") {
        std::string dir = (argc > 2) ? argv[2] : "assets/unity_export/ValidationScene";
        std::string out = (argc > 3) ? argv[3] : "out_validation.bmp";
        int dv = (argc > 4) ? atoi(argv[4]) : gpu::DV_NONE; // 1=albedo 2=normalGeom 3=normalMapped 4=uv
        runCaptureUnity(dir, out, dv);
        SDL_Quit();
        return 0;
    }

    // Unity scene mode: MyRender.exe --unity [sceneDir]
    //   sceneDir defaults to the exported ValidationScene; must contain scene.json.
    bool        useUnity = false;
    std::string unityDir = "assets/unity_export/ValidationScene/";
    if (argc > 1 && std::string(argv[1]) == "--unity") {
        useUnity = true;
        if (argc > 2) unityDir = argv[2];
        if (unityDir.back() != '/' && unityDir.back() != '\\') unityDir += '/';
        Config::scene_path = unityDir;
    }

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
    if (useUnity) scene.LoadUnity("scene.json");
    else          scene.LoadLegacy(Config::scene_path + "car_scene_2.json");

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
