# MyRender × Unity URP Importer — Development Tasks

Status: v1 · 2026-06-14
Companions: [Unity_Importer_Design.md](Unity_Importer_Design.md) (why) · [MyRender_AssetFormat.md](MyRender_AssetFormat.md) (the contract)

This is the **execution** document: the design split into small, ordered, independently-verifiable
steps. Each step states *what to build*, *which files*, and a concrete *test / verification gate* that
must pass before moving on. The goal is a renderer that stays **clean and readable** at every step —
this project is for learning rendering, so clarity beats cleverness.

---

## 0. How to use this document

- Work **top to bottom**. Each task `Tn.m` has a **Done when** line — do not start the next task until it
  is green. Never batch several tasks and "verify at the end".
- Every task ends in one of three verification kinds:
  - **Unit** — an assertion in the `test` executable (golden values, parse round-trips).
  - **Visual** — a `--capture` BMP eyeballed (and later, diffed) against a Unity reference screenshot.
  - **Smoke** — it builds, loads, and runs without crashing / with expected log output.
- Keep commits small: one task ≈ one commit, message `M1.2: binary .mesh loader`.

### Build & run reference

```powershell
cmake -B build -S .
cmake --build build --config Release          # builds MyRender.exe and test.exe
.\build\Release\test.exe                        # unit tests
.\build\Release\MyRender.exe                    # interactive window
.\build\Release\MyRender.exe --capture out\shots  # headless BMP capture
```

Unity side (export the validation scene):
`Unity ▸ menu MyRender ▸ Build Validation Scene`, then `MyRender ▸ Export Active Scene`
→ writes `G:/MyRender/assets/unity_export/ValidationScene/`.

### Definition of "clean" for this codebase

- Match the existing idiom: a `XxxData` POD + a free `from_json`, and a runtime class with `Load(data)`.
- One responsibility per file; loaders separate from runtime types; no parsing logic inside the rasterizer.
- Comment the *why* (conventions, handedness, math), not the *what*.
- No dead code left behind a feature flag "just in case" — delete retired paths once their replacement is green.

---

## 1. Architecture target (the spine of the refactor)

The single most important refactor: **one matrix-based in-memory scene model, fed by two front-end loaders.**

```
                       ┌─────────────────────────────┐
  scene.json (Unity) ──►  UnitySceneLoader            │
                       │                              ├──►  SceneModel  ──►  Render (rasterizer + URP shaders)
  car_scene.json ──────►  LegacySceneLoader (adapter) │      (matrices)        (UNCHANGED core)
   (+ .obj)            └─────────────────────────────┘
```

- **SceneModel** holds only what the renderer consumes: a `CameraState` (V, P matrices + near/far/pos),
  a `LightState` (direction, color), and `RenderObject[]` (each: `Mesh*`, `Material*[]` per submesh,
  `localToWorld`, `worldToLocal`). No euler angles, no orbit logic in the model.
- **UnitySceneLoader** reads the §6 schema and copies matrices **verbatim** (the design's whole point).
- **LegacySceneLoader** keeps the tutorial car scene alive: it builds the *same* matrices from the old
  TRS+euler JSON, so the euler→matrix code lives in exactly one place and the runtime has a single path.
  *(Default decision: keep the tutorial path via this adapter. If we later decide to retire it, delete
  one file — nothing else changes.)*
- **Render** loses `UpdateViewMatrix` / `UpdateProjectMatrix` / `UpdateModelMatrix`'s euler math; it just
  uploads matrices from the model. The orbit camera animation moves into the legacy loader / a camera
  controller, not the core.

What **does not** change: `RasterizeTri`, clipping, barycentric/perspective interpolation,
`UniversalFragmentPBR` and the BRDF/lighting/IBL files, the TGA loader, the threading model.

---

## M1 — Static scene renders correctly (the core)

Goal: the exported ValidationScene renders a correct still frame with Unity-matched geometry, camera,
and flat-lit shading. **Gate: side-by-side with a Unity screenshot, silhouettes and positions match.**

### T1.1 — New asset data model
- **Files**: new `src/core/SceneAsset.hpp` (replaces the parsing half of `MetaData.hpp`).
- Define PODs matching [MyRender_AssetFormat.md](MyRender_AssetFormat.md): `MeshRef`, `MaterialAsset`
  (camelCase fields: `shaderModel`, `baseColor`, `baseMap`, `normalMap`, `metallicGlossMap`,
  `smoothnessChannel`, `occlusionMap`, `emissionColor`, `cull`, `surfaceType`, …), `CameraAsset`
  (`worldToCameraMatrix`, `projectionMatrix`, near/far/pos), `LightAsset` (`direction`, `color`,
  `intensity`), `ObjectAsset` (`mesh`, `materials[]`, `matrix`, `worldToLocal`, `skinned`), `SceneAsset`.
- Add a `from_json` for `float4x4` that reads 16 row-major floats into `createMatrix4x4`.
- Mark **all** `from_json` free functions `inline` (current `MetaData.hpp` ones are not — latent ODR hazard).
- **Done when (Unit)**: `test.exe` parses `assets/unity_export/ValidationScene/scene.json` and asserts
  object count, first object's `matrix[0]`, and camera `projectionMatrix[0]` equal hand-checked values.

### T1.2 — Binary `.mesh` loader with submeshes
- **Files**: `src/core/Mesh.hpp` (add a `.mesh` branch; keep `.obj` for the legacy path).
- Parse the `MRSH` header, submesh index ranges, interleaved vertices (pos/normal/tangent/uv0 + optional
  uv1/color), u32 indices. Store geometry **verbatim** — no `x = -x`, no winding reversal.
- Represent submeshes: `triangles` per submesh (e.g. `std::vector<std::vector<std::array<Vertex,3>>>`
  or a flat list + `[start,count]` ranges). Keep it simple and documented.
- `Vertex` gains `color` (uv1 optional); skin fields deferred to M4.
- Tangents come from the file → MikkTSpace recompute is now optional (only when the file lacks tangents).
- **Done when (Unit)**: load a known ValidationScene cube `.mesh`; assert `vertexCount`, `submeshCount`,
  and that triangle 0's positions match the values written by the exporter (cross-check against a small
  hand-dumped reference). **(Smoke)**: every mesh in ValidationScene loads without assert.

### T1.3 — SceneModel + the two loaders
- **Files**: new `src/core/SceneModel.hpp` (the runtime model from §1), `src/core/UnitySceneLoader.hpp`,
  refactor `src/core/Scene.hpp` to host both loaders and own a `SceneModel`.
- `UnitySceneLoader`: `SceneAsset` → `SceneModel`, copying matrices verbatim; resolve mesh/material/texture
  paths relative to the scene root via the existing caches; one material per submesh.
- `LegacySceneLoader`: move the euler→matrix math (currently in `Render::UpdateModelMatrix` /
  `UpdateViewMatrix` / `UpdateProjectMatrix`) here, producing the same `SceneModel`. Orbit animation stays
  here too (operates on `CameraState`).
- **Done when (Smoke)**: both `car_scene_2.json` (legacy) and `ValidationScene/scene.json` (Unity) load into
  a `SceneModel` with the expected object counts; logged, no crash.

### T1.4 — Matrix upload (retire the euler core)
- **Files**: `src/core/Render.hpp`.
- Replace `UpdateModelMatrix(obj)` with `SetModelMatrices(localToWorld, worldToLocal)` →
  `UNITY_MATRIX_M` / `UNITY_MATRIX_I_M`. Replace `UpdateViewMatrix`/`UpdateProjectMatrix` with
  `SetCamera(cameraState)` → `UNITY_MATRIX_V = worldToCameraMatrix`, `UNITY_MATRIX_P = projectionMatrix`,
  `UNITY_MATRIX_VP = P · V`, plus `_WorldSpaceCameraPos`, `_ProjectionParams`.
- `FrameStart` takes `LightState`; `_MainLightPosition.xyz = -light.direction` (toward the light).
- Delete the euler rotation builders from `Render`. Keep the matrix-golden unit tests passing by pointing
  them at the legacy loader (which now owns that math).
- **Done when (Unit)**: a golden test transforms one ValidationScene vertex through the uploaded
  `UNITY_MATRIX_VP` and matches the clip-space position Unity computes for the same vertex (export a few
  golden `posOS → posCS` pairs from a tiny Unity script; tolerance 1e-3).

### T1.5 — Cull mode + front-face winding resolution
- **Files**: `src/core/Render.hpp` (`RasterizeTri` / `IsFrontFace`), `Material` (carry `cull`).
- Implement per-material cull: `back` / `front` / `off`. Resolve the winding sign empirically (design §5):
  render the validation cube, pick the sign/cull that makes outward faces visible and back faces hidden.
- Document the chosen convention in a comment next to `IsFrontFace`.
- **Done when (Visual)**: `--capture` of the ValidationScene shows solid, correctly-culled objects (no
  inside-out/missing front faces); a `cull:off` material shows both sides.

### T1.6 — M1 integration gate
- Wire `MyRender.cpp` to load `ValidationScene/scene.json` (add a CLI arg or config switch; keep the
  legacy default available).
- **Done when (Visual)**: capture ValidationScene and place it beside a Unity Game-view screenshot of the
  same scene. Object **positions, scale, silhouette, and camera framing match**. Lighting may differ
  (mapping comes in M2) but geometry must be right. Save both to `docs/video/` or a scratch folder for the record.

---

## M2 — Full PBR material mapping

Goal: Lit objects shade like Unity; Shader Graph materials get a sane Lit fallback.
**Gate: a Lit sphere with albedo + normal + metallic/smoothness mask visually matches Unity.**

### T2.1 — MaterialAsset → LitMat rework
- **Files**: `src/core/LitMat.hpp`, `src/core/MetaData.hpp`/`SceneAsset.hpp`, `src/gpu/ShaderGlobal.hpp`.
- Extend `LitMat` to carry: `metallicGlossMap`, `occlusionMap`, `smoothnessChannel`, `normalScale`,
  `emissionColor`/`emissionMap`, scalar `metallic`/`smoothness` fallbacks, `cull`, `surfaceType`,
  `alphaClip`/`cutoff`. `UpdateGpuParameter()` sets the matching `gpu::` globals and the keyword toggles
  `_SPECGLOSSMAP` / `_OCCLUSIONMAP` / `_EMISSION` / `_NORMALMAP` per the mapping table (design §10).
- **Done when (Unit)**: load a ValidationScene `.mat.json`; assert the toggles and scalar/texture pointers
  are set as expected (e.g. mask present ⇒ `_SPECGLOSSMAP == true`).

### T2.2 — Mask / occlusion sampling parity
- **Files**: `src/gpu/LitInput.hpp` / `SampleMetallicSpecGloss` / `SampleOcclusion`.
- Honor `smoothnessChannel` (metallicAlpha vs albedoAlpha); wire `_OCCLUSIONMAP` so occlusion actually
  reads the map. Verify URP packing: metallic=`.r`, smoothness=`.a` of the mask.
- **Done when (Visual)**: a sphere with a metallic gradient mask shows the gradient (use the debug views
  `DV_ALBEDO` / `DV_NORMAL_MAPPED` to isolate channels).

### T2.3 — Fallback (Shader Graph) materials
- **Files**: material factory (`MaterialCache` / `Material` creation).
- `shaderModel == "Fallback"` ⇒ build a `LitMat` from whatever `baseMap`/`baseColor` exist; default the
  rest. Log one line per fallback so coverage is visible.
- **Done when (Smoke)**: a scene with a Shader Graph material renders it as Lit (base color/texture), no crash.

### T2.4 — Bilinear texture sampling (optional but cheap quality win)
- **Files**: `src/core/Texture.hpp` (`SamplerLinear` is currently a TODO that calls point).
- Implement bilinear; respect wrap modes. Keep point as a switch for the tutorial visuals.
- **Done when (Visual)**: a textured plane viewed at a grazing angle shows smooth (not blocky) texels.

### T2.5 — M2 gate
- **Done when (Visual)**: side-by-side of a Lit test object (albedo+normal+mask+emission) vs Unity —
  base color, normal detail, metallic highlight, and emission read correctly. Document known gaps
  (GI/indirect, filtering) per design §11.

---

## M3 — Directional shadow map

Goal: objects cast and receive a directional shadow. **Gate: the validation cube casts a shadow on the ground.**

### T3.1 — Depth-only rasterization mode
- **Files**: `src/core/Render.hpp` (a `DrawDepthOnly` path or a flag on `Draw`).
- Reuse the rasterizer with no fragment shader: write nearest depth into a separate
  `shadowDepth[res²]` (e.g. 2048²). Only opaque shadow-casters.
- **Done when (Unit/Smoke)**: render the cube depth-only from a known view; assert the depth buffer has
  the expected near/far spread (min < max, center texel closer than corners).

### T3.2 — Light camera (ortho fit)
- **Files**: new `src/core/ShadowPass.hpp`.
- Compute scene AABB from object world bounds; build `lightView` (look along `light.direction`, sane up)
  and an ortho `lightProj` fit to the AABB; `lightVP = lightProj · lightView`. v1 = single map, no cascades.
- **Done when (Unit)**: a corner of the AABB projects to roughly the shadow-map border (sanity assert on
  projected extents).

### T3.3 — Sample shadows in the fragment path
- **Files**: `src/gpu/ShaderFunction.hpp` (`MainLightRealtimeShadow` is stubbed to return 1.0 at
  [ShaderFunction.hpp:454](../src/gpu/ShaderFunction.hpp)), `src/gpu/ShaderGlobal.hpp` (upload
  `shadowDepth`, `lightVP`, bias/PCF params).
- Implement: `shadowCoord = lightVP · float4(positionWS,1)` → shadow-map UV+depth → depth compare with
  slope/normal bias → 3×3 PCF average. It already flows into `light.shadowAttenuation` →
  `UniversalFragmentPBR`.
- **Done when (Visual)**: cube-on-ground capture shows a soft-edged shadow; tune bias until no acne /
  no peter-panning at the contact line.

### T3.4 — M3 gate
- **Done when (Visual)**: ValidationScene with multiple objects shows mutually-correct cast/receive
  shadows from the main light. Compare shadow direction/length against Unity.

---

## M4 — Skeletal animation (bake + linear blend skinning)

Goal: an animated skinned character plays a looping clip. **Gate: the character animates smoothly.**

### T4.1 — Exporter: skin + baked animation
- **Files**: Unity `MeshWriter.cs` (skin block), new `AnimationExporter.cs`.
- Mesh: write `boneIndex[4]`/`boneWeight[4]` + bindposes (`flags.hasSkin`, per format §`.mesh`).
- Anim: `AnimationMode.BeginSampling()` → `SampleAnimationClip(go, clip, t)` per frame → write
  `S[b] = bone.localToWorld[b] · bindpose[b]`; `.anim = {fps, frameCount, boneCount, S[frame][bone]}`.
- **Done when (Smoke)**: exporting a skinned character writes a `.mesh` with skin flag set and a `.anim`
  with `frameCount > 1`; header fields log as expected.

### T4.2 — Runtime: skin data + bone upload
- **Files**: `Mesh` (skin block), `Attributes` (`boneIndex[4]`/`boneWeight[4]`), `src/core/AnimationClip.hpp`,
  `src/core/AnimationPlayer.hpp`, a bone-matrix `gpu::` array.
- `AnimationPlayer` advances by wall clock → current frame → uploads that frame's `S[]`.
- **Done when (Unit)**: load `.anim`; assert frame selection at a few timestamps (t=0, t=mid, wrap) picks
  the right frame index.

### T4.3 — Skinned vertex path
- **Files**: new `SkinnedLitMat` (or a skin flag in the Lit vertex stage).
- `positionWS = Σ_b w_b · (S[b] · posOS)`; same for normals (upper-3×3). `positionCS = VP · positionWS`.
  Skinned objects skip the per-object `M` (the skin matrices already map mesh-local→world; design §9.2).
- **Done when (Visual)**: capture three frames of the clip; the mesh deforms plausibly (no exploded
  verts, no collapsed bones). **(Smoke)**: plays on loop in the interactive window.

### T4.4 — M4 gate
- **Done when (Visual)**: the skinned character plays its clip smoothly under correct lighting/shadows.

---

## M5 — GardenScene end-to-end

Goal: a real Unity scene renders recognizably. **Gate: Garden is identifiable beside Unity.**

### T5.1 — Real-asset texture export path
- **Files**: Unity `TextureExporter.cs`.
- Add the source-asset reimport path for BC5/DXT5nm normal & mask maps (temporarily set uncompressed +
  `sRGBTexture=false` + readable, read, restore). Design §7 / §13.2.
- **Done when (Visual)**: a Garden normal map exports without swizzle garbage (compare a known brick/normal).

### T5.2 — Export & load GardenScene
- **Done when (Smoke)**: `MyRender ▸ Export Active Scene` on Garden completes; MyRender loads it without
  crashing; log material coverage (Lit vs Fallback counts).

### T5.3 — Performance pass
- **Files**: profile the single-threaded vertex/clip pass (design §13.4).
- Measure with the existing bench in `MyRender.cpp`. If the Pass-1 vertex/clip stage bottlenecks,
  parallelize per object. Record before/after ms.
- **Done when (Smoke)**: Garden renders at an interactive-ish rate; numbers recorded.

### T5.4 — M5 gate
- **Done when (Visual)**: Garden capture beside Unity — layout, major materials, shadows, and lighting
  are recognizably the same scene. Document the accepted gaps (SG fallback, GI, particles) per §11.

---

## 2. Cross-cutting test strategy

- **Golden matrices/positions (Unit)**: small Unity helper script dumps `worldToCameraMatrix`,
  `projectionMatrix`, and a few `posOS→posCS` pairs to JSON; `test.exe` asserts MyRender reproduces them.
  This is the cheapest guard against handedness/row-major regressions and reuses the existing
  `test_view_matrix`-style harness.
- **Parse round-trips (Unit)**: every new `from_json` gets a load-and-assert test against a real exported file.
- **Visual references (Visual)**: keep a `references/` folder of Unity Game-view screenshots per milestone;
  `--capture` outputs are compared against them. Manual eyeball at first; a simple BMP mean-abs-diff check
  can be added once framing is locked.
- **No silent skips**: if a test can't run (missing asset), it must fail loudly, not pass vacuously.

## 3. Open decisions carried from design §13

1. Front-face winding sign — locked empirically in **T1.5**.
2. Negative-scale tangents (`GetOddNegativeScale` returns 1) — detect `det(M)<0` in exporter and warn;
   full support post-v1.
3. Animated (Timeline) camera — v1 exports a static snapshot; per-frame camera bake is a later add.
4. Legacy tutorial path — kept via `LegacySceneLoader` adapter (T1.3); retiring it later is a one-file delete.
