# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MyRender is a CPU-based software renderer that emulates visual effects achievable through shaders in Unity's Universal Render Pipeline (URP). It uses SDL3 for windowing/display and renders a 960×540 framebuffer each frame.

Renderer source lives under `src/` (`src/base`, `src/core`, `src/gpu`, `src/MyRender.cpp`). Video scripts and authoring docs live under `docs/` (`docs/video/output1..3`, `docs/authoring`). Build output and `--capture` output are git-ignored.

## Build

CMake + MSVC, C++20. Build with Visual Studio or via CMake CLI:

```powershell
cmake -B build -S .
cmake --build build --config Release
```

Two executables: `MyRender` (renderer) and `test` (unit tests).
SDL3 is linked statically. Windows system libs (`Setupapi`, `Winmm`, `Imm32`, `Version`) are auto-linked.

## Architecture

**Entry point** (`src/MyRender.cpp`): Initializes SDL3, creates a `frameBuffer` (RGBA bytes), loads a scene from JSON, runs the game loop calling `Scene::Update()` and `Scene::Render()`.

**Rendering pipeline** (`src/core/Render.hpp` — singleton `Render::Get()`):
1. `FrameStart` — clears color/depth buffers, uploads camera/light data to `gpu::` globals
2. Per-object: `UpdateModelMatrix` → `Draw` (calls vertex shader per vertex)
3. Per-triangle: Sutherland-Hodgman frustum clipping (7 planes) → rasterize via bounding box + barycentric coordinates → perspective-correct interpolation → fragment shader → depth test → alpha blend
4. After all draws: `Scene::Render` converts the float `ColorBuffer` to linear→sRGB RGBA bytes, which SDL uploads as a texture

**Shader system** (`src/gpu/`): Mirrors Unity URP shader structure — intentionally written to feel like HLSL.
- `src/gpu/ShaderGlobal.hpp` — all global shader state as `namespace gpu` variables (`UNITY_MATRIX_M/V/P/VP`, `_MainLightPosition`, `_BaseMap`, etc.)
- `src/gpu/ShaderFunction.hpp` — common HLSL built-ins (`mul`, `normalize`, `lerp`, transform functions, etc.)
- `src/gpu/LitShader.hpp`, `src/gpu/SimpleLitShader.hpp`, `src/gpu/UnLitShader.hpp` — vertex/fragment shader implementations
- `src/gpu/BRDF.hpp`, `src/gpu/Lighting.hpp`, `src/gpu/ImageBasedLighting.hpp`, `src/gpu/GlobalIllumination.hpp` — PBR lighting

**Materials** (`src/core/`): Abstract `Material` base holds `vertex_shader`/`fragment_shader` function pointers. Concrete classes: `LitMat`, `SimpleLitMat`, `UnLitMat`. Each material's `UpdateGpuParameter()` writes to `gpu::` globals before draw.

**Math** (`src/base/`): Template `Vector<N,T>` with specializations (`float2/3/4`, `half3/4`, etc.) and `Matrix<R,C,T>`. `Varyings` and `Attributes` are the vertex shader I/O structs.

**Scene** (`src/core/Scene.hpp`): Loads from JSON via `nlohmann::json`. Scene JSON specifies camera, directional light, and game objects. Use `json = nlohmann::json` type alias is available from MetaData.hpp. Asset paths are set via `Config::scene_path`.

**Caches**: `TextureCache`, `MaterialCache`, `MeshCache` (all singletons) avoid reloading assets.

**Per-frame allocator**: `Render` owns two pre-allocated pools (`_attributesBuffer`, `_varyingsBuffer`) that are reset each frame to avoid per-triangle heap allocation.
