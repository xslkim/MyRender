#include <SDL3/SDL.h>
#include <Scene.hpp>
#include <Config.hpp>
#include <Input.hpp>
#include <SyntheticSkinScene.hpp>
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

// Box-downsample bigBuf (ssW x ssH, RGBA bytes) -> outBuf (W x H) by factor N=ss.
static void boxDownsample(const unsigned char* bigBuf, int ssW, int ssH, int ss,
                          unsigned char* outBuf, int W, int H)
{
    int n2 = ss * ss;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int r = 0, g = 0, b = 0, a = 0;
            for (int dy = 0; dy < ss; ++dy) {
                for (int dx = 0; dx < ss; ++dx) {
                    int si = ((y * ss + dy) * ssW + (x * ss + dx)) * 4;
                    r += bigBuf[si + 0];
                    g += bigBuf[si + 1];
                    b += bigBuf[si + 2];
                    a += bigBuf[si + 3];
                }
            }
            int di = (y * W + x) * 4;
            outBuf[di + 0] = (unsigned char)(r / n2);
            outBuf[di + 1] = (unsigned char)(g / n2);
            outBuf[di + 2] = (unsigned char)(b / n2);
            outBuf[di + 3] = (unsigned char)(a / n2);
        }
    }
}

// Headless one-shot of a Unity-exported scene -> single BMP. No window.
// ssScale=1: no SSAA. ssScale=2: render at 2× in each dimension, box-downsample.
static void runCaptureUnity(const std::string& sceneDir, const std::string& outFile,
                            int debugView = gpu::DV_NONE, float animTime = 0.0f,
                            int ssScale = 1, float lightmapIntensity = -1.0f)
{
    const int W = Config::kScreenWidth;
    const int H = Config::kScreenHeight;
    const int ss = std::max(1, ssScale);
    const int ssW = W * ss;
    const int ssH = H * ss;

    // Temporarily set render resolution to the supersample size.
    Config::kScreenWidth  = ssW;
    Config::kScreenHeight = ssH;

    unsigned char* bigBuf = new unsigned char[ssW * ssH * 4];

    Config::scene_path = sceneDir;
    if (Config::scene_path.back() != '/' && Config::scene_path.back() != '\\')
        Config::scene_path += '/';

    Scene scene;
    scene.ScreenBuffer = bigBuf;

    auto t0 = std::chrono::high_resolution_clock::now();
    scene.LoadUnity("scene.json");
    auto t1 = std::chrono::high_resolution_clock::now();
    gpu::g_debugView = debugView;
    // Optional lightmap intensity override (for tuning sweeps). -1 = keep default 1.0.
    if (lightmapIntensity >= 0.0f) {
        gpu::_LIGHTMAP_INTENSITY = lightmapIntensity;
        printf("[lm] intensity = %.3f\n", lightmapIntensity);
    }
    if (animTime > 0.0f) scene.AdvanceAnimations(animTime);
    scene.Render();
    auto t2 = std::chrono::high_resolution_clock::now();
    scene.Render(); // second frame = steady-state render cost
    auto t3 = std::chrono::high_resolution_clock::now();

    auto ms = [](auto a, auto b){ return std::chrono::duration_cast<std::chrono::milliseconds>(b-a).count(); };
    printf("[bench] load=%lldms  render1=%lldms  render2=%lldms  ss=%dx\n",
           ms(t0,t1), ms(t1,t2), ms(t2,t3), ss);

    if (ss == 1) {
        writeBMP(outFile, bigBuf, W, H);
    } else {
        unsigned char* outBuf = new unsigned char[W * H * 4];
        boxDownsample(bigBuf, ssW, ssH, ss, outBuf, W, H);
        writeBMP(outFile, outBuf, W, H);
        delete[] outBuf;
    }

    // Restore resolution so later callers (interactive window) see the right config.
    Config::kScreenWidth  = W;
    Config::kScreenHeight = H;

    printf("unity capture -> %s\n", outFile.c_str());
    delete[] bigBuf;
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

    // Dev tool: synthesize a skinned validation scene (no Unity needed), then
    // capture it. MyRender.exe --gen-synth-skin [dir] [outFile]
    if (argc > 1 && std::string(argv[1]) == "--gen-synth-skin") {
        std::string dir = (argc > 2) ? argv[2] : "out/_synthscene";
        std::string out = (argc > 3) ? argv[3] : "out/synth_skin.bmp";
        SyntheticSkinScene::Generate(dir);
        runCaptureUnity(dir, out);
        SDL_Quit();
        return 0;
    }

    // Headless Unity one-shot: MyRender.exe --capture-unity [sceneDir] [outFile] [dv] [animTime] [ssScale]
    if (argc > 1 && std::string(argv[1]) == "--capture-unity") {
        std::string dir = (argc > 2) ? argv[2] : "assets/unity_export/ValidationScene";
        std::string out = (argc > 3) ? argv[3] : "out_validation.bmp";
        int   dv = (argc > 4) ? atoi(argv[4]) : gpu::DV_NONE; // 1=albedo 2=normalGeom 3=normalMapped 4=uv
        float at = (argc > 5) ? (float)atof(argv[5]) : 0.0f;   // animation seek time (s)
        int   ss = (argc > 6) ? atoi(argv[6]) : 1;              // SSAA scale: 1=off, 2=2x
        float li = (argc > 7) ? (float)atof(argv[7]) : -1.0f;   // lightmap intensity override
        runCaptureUnity(dir, out, dv, at, ss, li);
        SDL_Quit();
        return 0;
    }

    // Window is always this size; the internal render resolution may be lower
    // (set via the --unity scale arg) and SDL stretches it up to the window.
    const int winW = 960, winH = 540;

    // Unity scene mode: MyRender.exe --unity [sceneDir] [resScale]
    //   sceneDir defaults to the exported ValidationScene; must contain scene.json.
    //   resScale (0..1) lowers the internal framebuffer for heavy scenes, e.g.
    //   `--unity assets/unity_export/GardenScene 0.5` renders at 480x270.
    bool        useUnity = false;
    std::string unityDir = "assets/unity_export/ValidationScene/";
    if (argc > 1 && std::string(argv[1]) == "--unity") {
        useUnity = true;
        if (argc > 2) unityDir = argv[2];
        if (unityDir.back() != '/' && unityDir.back() != '\\') unityDir += '/';
        Config::scene_path = unityDir;

        if (argc > 3) {
            float s = (float)atof(argv[3]);
            if (s > 0.05f && s <= 1.0f) {
                Config::kScreenWidth  = (int)(winW * s);
                Config::kScreenHeight = (int)(winH * s);
            }
        }
    }

    // Internal render resolution (== window unless --unity scaled it down).
    const int rW = Config::kScreenWidth;
    const int rH = Config::kScreenHeight;

    unsigned char* frameBuffer = new unsigned char[rW * rH * 4];
    memset(frameBuffer, 0, rW * rH * 4);

    SDL_Window*   window       = SDL_CreateWindow("MyRender", winW, winH, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer     = SDL_CreateRenderer(window, NULL, 0);
    SDL_Texture*  frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                   SDL_TEXTUREACCESS_TARGET, rW, rH);

    Scene scene;
    scene.ScreenBuffer = frameBuffer;
    if (useUnity) { scene.LoadUnity("scene.json"); scene.EnableFlyCamera(); }
    else          scene.LoadLegacy(Config::scene_path + "car_scene_2.json");

    bool running = true;
    while (running) {
        // Drain all pending events so key state is current (frames can be slow).
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                else Input::Get().SetKeyDown(SDL_GetKeyName(e.key.keysym.sym));
            }
            else if (e.type == SDL_EVENT_KEY_UP) {
                Input::Get().SetKeyUp(SDL_GetKeyName(e.key.keysym.sym));
            }
        }

        scene.Update(17);
        scene.Render();

        if (SDL_UpdateTexture(frameTexture, NULL, frameBuffer, rW * 4) != 0)
            printf("SDL_UpdateTexture error: %s\n", SDL_GetError());

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, frameTexture, NULL, NULL); // stretches to window
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    delete[] frameBuffer;
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
