# MyRender × Unity URP Importer — Technical Design

Status: draft v1 · 2026-06-14
Companion: [MyRender_AssetFormat.md](MyRender_AssetFormat.md) (the binary/JSON contract)

This document is the full technical plan for upgrading MyRender from "load a hand-authored
JSON scene" to "import and render a Unity 2022 URP scene". It records what was investigated,
the concrete design for each subsystem, what is decided vs. open, and the milestone order.

---

## 1. Goal & scope

- **Input**: a Unity 2022.3 (URP 14) project. First concrete target: the official URP 3D Sample
  at `G:\unity_demo\urp_sample` — start with a self-built minimal validation scene, then `GardenScene`.
- **Mechanism**: a Unity C# Editor tool exports the assets MyRender needs (one-click on the active scene).
- **Renderer**: MyRender may be **fully refactored**. CPU software renderer, 960×540, URP-flavored shaders.
- **Feature target (all requested)**: meshes + transforms, camera, **directional-light shadows**,
  **skeletal animation**, PBR materials. Shader Graph materials get a Lit fallback.
- **Non-goals (v1)**: VFX Graph / particles, post-processing, lightmaps/GI bake, reflection probes,
  multiple/point/spot lights, decals, transparency depth sorting beyond per-object order.

---

## 2. Current MyRender architecture (as-is)

Pipeline (singleton `Render::Get()`, [Render.hpp](../src/core/Render.hpp)):

1. `FrameStart` clears buffers, uploads camera/light to `gpu::` globals.
2. Per object: `UpdateModelMatrix` → `Draw(mesh, material)`.
3. Two-pass `Draw`: **Pass 1** (single-thread) vertex shader + Sutherland-Hodgman clip (7 planes)
   → screen-space triangles; **Pass 2** (multi-thread by Y-strip) rasterize via bbox + barycentric,
   perspective-correct interpolate, fragment shader, depth test, blend.
4. `Scene::Render` converts the float color buffer linear→sRGB bytes for SDL.

Conventions that matter ([RenderUtils.hpp](../src/core/RenderUtils.hpp), [Render.hpp](../src/core/Render.hpp)):

- **Clip/NDC**: OpenGL convention. `positionCS = VP · posWS`; NDC = `clip.xyz/clip.w` in `[-1,1]³`;
  screen `x=(ndc.x+1)/2·(W-1)`, `y=(ndc.y+1)/2·(H-1)`; depth `ndc.z·0.5+0.5` in `[0,1]`,
  cleared to 1.0, test `Less` = closer. `Scene::Render` flips Y on copy → +ndc.y is up.
- **Matrices** ([Matrix.hpp](../src/base/Matrix.hpp)): row-major, row-vector-on-left and
  column-vector-on-right both defined; `M·v` treats `v` as column. `createMatrix4x4` takes row-major args.
- **Shaders** ([ShaderFunction.hpp](../src/gpu/ShaderFunction.hpp), [LitShader.hpp](../src/gpu/LitShader.hpp)):
  faithful URP port. `GetVertexPositionInputs` uses `UNITY_MATRIX_M` then `UNITY_MATRIX_VP`;
  `TransformObjectToWorldNormal` uses the upper-3×3 of `UNITY_MATRIX_I_M` as the inverse-transpose;
  `GetVertexNormalInputs` builds the TBN; fragment runs `UniversalFragmentPBR`.
- **Shadows**: structurally present but stubbed — `InputData.shadowCoord` exists,
  `MainLightRealtimeShadow` returns `1.0` ([ShaderFunction.hpp:454](../src/gpu/ShaderFunction.hpp)).
- **Texture sampling**: point only; `SamplerLinear` is a TODO that calls point
  ([Texture.hpp:37](../src/core/Texture.hpp)). Images: **TGA only**.

The "hacky" conventions we will retire in the refactor:

- OBJ loader flips `pos.x = -pos.x` and reverses winding to convert right-handed OBJ → internal space
  ([Mesh.hpp:48](../src/core/Mesh.hpp)). Replaced by verbatim Unity data + `.mesh` binary.
- Euler-based `UpdateViewMatrix`/`UpdateModelMatrix` with a bespoke rotation order
  ([Render.hpp:86-187](../src/core/Render.hpp)). Replaced by feeding Unity matrices directly.
- Hard-coded orbit camera in `Scene::Update` ([Scene.hpp:67](../src/core/Scene.hpp)). Replaced by the
  exported camera (and later, animated camera tracks).

---

## 3. Target project reality (investigated)

`G:\unity_demo\urp_sample` = Unity official URP 3D Sample, Unity 2022.3.62f1, URP 14.0.12.
Scenes: Cockpit, Garden, Oasis, Terminal, Benchmark. Totals: **206 materials / 250 FBX / 84 Shader Graph / 11 anim**.

| Scene | materials | stock URP/Lit | rest | scene size |
|---|---|---|---|---|
| Cockpit | 49 | 0 | all Shader Graph | 0.6 MB |
| Garden | 59 | 30 | Shader Graph | 5 MB |
| Oasis | 46 | 19 | Shader Graph | 10 MB |
| Terminal | 48 | 24 | Shader Graph | 0.9 MB |

Implications baked into the plan:
- ~half of materials are **Shader Graph** → cannot map 1:1; they take the **Lit fallback** (probe main tex + base color).
- Cameras in real scenes are likely **Timeline/Cinemachine**-driven (Cockpit has `CockpitFlythrough.playable`).
  v1 exports a **static snapshot** of the camera; animated camera tracks are a later add.
- Heavy VFX/particles/volumes/probes are **out of scope** and will simply be missing.

---

## 4. Overall pipeline

```
Unity Editor (urp_sample)                         MyRender (C++)
┌─────────────────────────────┐                  ┌──────────────────────────────┐
│ MyRender ▸ Export Active     │   files on disk  │ AssetLoader                  │
│   Scene                      │ ───────────────► │  scene.json → Scene          │
│  • walk renderers            │  scene.json      │  .mesh      → Mesh (binary)  │
│  • Mesh → .mesh (binary)     │  meshes/*.mesh   │  .mat.json  → Material       │
│  • Material → .mat.json      │  materials/*.json│  *.tga      → Texture2D      │
│  • Texture → .tga            │  textures/*.tga  │  .anim      → AnimationClip  │
│  • Camera/Light → scene.json │  anims/*.anim    ├──────────────────────────────┤
│  • (v2) bake skin+anim       │                  │ ShadowPass → main Render     │
└─────────────────────────────┘                  │  + AnimationPlayer (skinning)│
                                                  └──────────────────────────────┘
```

Output root: `G:/MyRender/assets/unity_export/<SceneName>/`. Assets de-duplicated by source object.

---

## 5. Coordinate system & matrices — the key decision

**Principle: export Unity values verbatim, feed Unity's own matrices to the URP-port shaders.**
Because the shader layer already consumes `UNITY_MATRIX_M/I_M/V/P/VP` exactly the URP way, we make
those globals *be* Unity's matrices and the whole handedness problem disappears.

| MyRender global | Source from Unity | Notes |
|---|---|---|
| `UNITY_MATRIX_M`   | `renderer.transform.localToWorldMatrix` | per object |
| `UNITY_MATRIX_I_M` | `renderer.transform.worldToLocalMatrix` | inverse-transpose for normals comes from its upper-3×3 |
| `UNITY_MATRIX_V`   | `camera.worldToCameraMatrix`            | OpenGL-convention view (RH, looks down −Z) |
| `UNITY_MATRIX_P`   | `camera.projectionMatrix`               | OpenGL-convention proj, NDC z∈[−1,1] — matches the rasterizer |
| `UNITY_MATRIX_VP`  | `P · V` (computed in C++)               | |
| `_WorldSpaceCameraPos` | `camera.transform.position`         | |
| `_MainLightPosition.xyz` | `−light.transform.forward`        | URP convention: direction *toward* the light |

World geometry (positions/normals/tangents) is Unity world space (left-handed, +Y up, +Z forward, meters),
written verbatim — **no `x=-x`, no winding reversal**. `camera.projectionMatrix` is already in Unity's
platform-agnostic OpenGL form (z∈[−1,1], −Z view), which is exactly the rasterizer's existing assumption.

**One empirical bring-up item — front-face winding.** Unity's left-handed clip space vs. the rasterizer's
`IsFrontFace` sign may disagree once we feed Unity data. Resolve at M1 by either flipping the comparison in
`IsFrontFace` or honoring the material `cull` mode (we implement proper cull anyway). This is a one-line
sign decision, not a structural risk.

**Known limitation — odd/negative scale.** `GetOddNegativeScale()` returns 1 always
([ShaderFunction.hpp:212](../src/gpu/ShaderFunction.hpp)); mirrored objects (negative scale) would get a
wrong tangent sign. Detect `det(M) < 0` in the exporter and warn; full support is post-v1.

---

## 6. Asset formats

Authoritative spec: [MyRender_AssetFormat.md](MyRender_AssetFormat.md). Summary:

- **scene.json** — camera (now also carries `worldToCameraMatrix` + `projectionMatrix`), main directional
  light, ambient, and an `objects[]` list (per object: `mesh`, `materials[]` per submesh, TRS + `localToWorld`
  + `worldToLocal`, `skinned`).
- **.mesh** (binary `MRSH`) — interleaved `pos/normal/tangent/uv0(+uv1/color)`, submesh index ranges,
  u32 indices; optional skin block (`boneIndex[4]/boneWeight[4]` + bindposes) for v2.
- **.mat.json** — URP Lit property mapping (see §10).
- **textures** — TGA; color = sRGB, data (normal/mask/occlusion) = linear.

> Exporter change needed (M1): add `worldToCameraMatrix`/`projectionMatrix` to the camera block and
> `worldToLocal` to each object, per §5. The current exporter emits `localToWorld` + fov only.

---

## 7. Exporter design (Unity side)

Location: `urp_sample/Assets/Editor/MyRenderExport/`.

- `MyRenderExporter.cs` — `[MenuItem("MyRender/Export Active Scene")]`. Walks root GOs →
  `MeshRenderer`/`SkinnedMeshRenderer`; dedups meshes/materials/textures via dictionaries; writes scene.json.
  Reads URP props via `Material.GetTexture/GetColor/GetFloat` (no YAML parsing). Shader name → `shaderModel`
  (`Lit/SimpleLit/Unlit/Fallback`).
- `MeshWriter.cs` — Unity `Mesh` → `.mesh`. v1 static (no skin). Verbatim values.
- `TextureExporter.cs` — Blit→ReadPixels→`ImageConversion.EncodeToTGA`. **TODO for real scenes**: normal/mask
  maps that are BC5/DXT5nm-compressed are GPU-swizzled; add a source-asset reimport path (temporarily set
  `TextureImporter` uncompressed + `sRGBTexture=false` + readable, read, restore). For the validation scene,
  author those maps uncompressed/readable so the simple path is correct.
- `ValidationSceneBuilder.cs` — `[MenuItem("MyRender/Build Validation Scene")]`. Programmatically builds a
  minimal URP/Lit scene (ground + cube/sphere/capsule with varied metallic/smoothness + 1 directional light
  + 1 static camera).

Pending (M4): skinned-mesh + animation export — bake per-frame skinning matrices via `AnimationMode`
(see §9).

---

## 8. Renderer refactor (C++ side)

Strategy: **keep the rasterizer, clip, and URP shader layer; replace the front-end (loaders, scene, matrix
upload) and add ShadowPass + skinning.** Minimal disruption to proven code.

New / changed modules:

1. **Binary mesh loader** — `Mesh` gains a `.mesh` path branch (alongside or replacing `.obj`).
   Reads interleaved vertices into the existing per-triangle `Vertex` arrays. No `x` flip.
   Tangents come from the file (Unity-provided), so MikkTSpace recompute becomes optional.
2. **Scene/asset model** — new `SceneData`/loaders for the §6 JSON. `scene.json` carries matrices;
   `GameObject` stores `localToWorld`/`worldToLocal` directly instead of TRS+euler.
3. **Matrix upload** — `UpdateModelMatrix` sets `UNITY_MATRIX_M = localToWorld`,
   `UNITY_MATRIX_I_M = worldToLocal`. `FrameStart`/camera sets `V`,`P`,`VP` from the exported matrices,
   `_MainLightPosition = -lightDir`. Delete the euler/orbit code paths.
4. **Cull modes** — honor material `cull` (back/front/off) in rasterization (replaces the single hard-coded
   `IsFrontFace`). Resolve the winding sign here (§5).
5. **ShadowPass** (§9). 6. **AnimationPlayer + skinning** (§9).

What stays untouched: `RasterizeTri`, clipping, barycentric/perspective interpolation, `UniversalFragmentPBR`
and the BRDF/lighting files, TGA loader, threading model.

---

## 9. Shadows & animation (the two big new systems)

### 9.1 Directional shadow map

Add a depth-only pass before the main render:

1. **Light camera**: orthographic. Compute scene AABB from all opaque object world bounds; build
   `lightView = worldToLight` (look along light dir, up = world-up-ish) and `lightProj` = ortho fit to the AABB.
   `lightVP = lightProj · lightView`. (v1 = single map fit to whole scene; no CSM cascades.)
2. **Render shadow map**: reuse the rasterizer in a **depth-only mode** (no fragment shader, write nearest
   depth) into a `shadowDepth[shadowRes²]` buffer (e.g. 2048²). Only opaque, shadow-casting objects.
3. **Globals**: upload `shadowDepth` + `lightVP` + bias/PCF params to `gpu::`.
4. **Sample in fragment**: implement the stubbed path —
   `shadowCoord = lightVP · float4(positionWS,1)`; to shadow-map UV+depth; depth compare with slope/normal
   bias; 3×3 PCF. `MainLightRealtimeShadow` returns the average; it already flows into
   `light.shadowAttenuation` and through `UniversalFragmentPBR`.

Cost: one extra full rasterization pass at shadow resolution. Fully CPU-feasible.

### 9.2 Skeletal animation (bake + LBS)

**Decision (confirmed): bake per-frame skinning matrices; C++ does linear blend skinning.** No Animator
state machine, no curve evaluation in C++.

Skinning math: mesh vertices are in mesh-local space; `bindpose[b]` maps mesh-local→bone-local;
`bone.localToWorld[b]` maps bone-local→world. So per bone the **skinning matrix**
`S[b] = bone.localToWorld[b] · bindpose[b]` maps mesh-local→world directly, and
`positionWS = Σ_b w_b · (S[b] · posOS)` (no separate `M` needed for skinned objects).

- **Export** (M4): for a clip at `fps`, for each frame `f`: `AnimationMode.BeginSampling()` →
  `AnimationMode.SampleAnimationClip(go, clip, t)` → read each bone transform → write `S[b]`.
  `.anim` = `{fps, frameCount, boneCount, S[frame][bone] (3×4 or 4×4)}`. Mesh stores `boneIndex/weight` + bindposes.
- **Runtime**: `AnimationPlayer` advances time by wall clock → current frame; uploads that frame's `S[]`
  to a `gpu::` bone-matrix array. A skinned vertex path computes `positionWS`/`normalWS` via the weighted
  sum above, then `positionCS = VP · positionWS`. `Attributes` gains `boneIndex[4]`/`boneWeight[4]`;
  add a `SkinnedLitMat` (or a skin flag in the vertex stage).
- v1 plays a single clip on loop; blending/retargeting are out of scope.

---

## 10. Material mapping

| URP Lit property | .mat.json | MyRender global / behavior |
|---|---|---|
| `_BaseColor` / `_Color` | `baseColor` | `_BaseColor` |
| `_BaseMap` / `_MainTex` | `baseMap` (sRGB) | `_BaseMap` (sRGB→linear on load) |
| `_BaseMap_ST` scale/offset | `tiling`/`offset` | `_Tiling`/`_Offset` via `TRANSFORM_TEX` |
| `_BumpMap`,`_BumpScale` | `normalMap` (linear),`normalScale` | `_BumpMap`, `SampleNormal` |
| `_MetallicGlossMap` (R=metal, A=smooth) | `metallicGlossMap` | `_SpecGlossMap` + `_SPECGLOSSMAP=true`; `SampleMetallicSpecGloss` reads `.r`/`.a` |
| `_Metallic`,`_Smoothness` | `metallic`,`smoothness` | scalar fallback when no mask |
| `_SmoothnessTextureChannel` | `smoothnessChannel` | metallicAlpha vs albedoAlpha |
| `_OcclusionMap`,`_OcclusionStrength` | `occlusionMap`,`occlusionStrength` | `SampleOcclusion` (needs `_OCCLUSIONMAP` wired) |
| `_EmissionColor`,`_EmissionMap` | `emissionColor`,`emissionMap` | `_EmissionColor`/`_EmissionMap`, set `_EMISSION` |
| `_Cull` (0/1/2) | `cull` (off/front/back) | rasterizer cull mode |
| `_Surface`,`_Cutoff`,`_AlphaClip` | `surfaceType`,`cutoff`,`alphaClip` | `_Surface`, alpha test/blend |

**Shader Graph → Fallback**: `shaderModel="Fallback"`; exporter probes `_BaseMap`/`_MainTex` and
`_BaseColor`/`_Color`; rendered as Lit with whatever exists (often just base color). Visual mismatch accepted.

**LitMat rework**: current `LitMat`/`MaterialData` ([LitMat.hpp](../src/core/LitMat.hpp),
[MetaData.hpp](../src/core/MetaData.hpp)) extend to carry the mask/occlusion maps, smoothness channel,
cull mode, and to set `_SPECGLOSSMAP`/`_OCCLUSIONMAP`/`_EMISSION` toggles.

---

## 11. Lighting parity & known visual gaps

MyRender already ships URP-equivalent `UniversalFragmentPBR`/BRDF, so direct lighting from the main
directional light should be close to Unity. Expected differences in v1 (documented, not bugs):

- **Indirect/GI**: Unity uses lightmaps + light probes + reflection probes; MyRender has only flat ambient
  + simple IBL. Shadowed/indirect areas will differ most.
- **Texture filtering**: point sampling only (no bilinear/mip) → aliasing/shimmer.
- **Shadows**: single map + PCF, no cascades → coarser than URP soft shadows.
- **Transparency**: per-object draw order, no per-pixel sort → possible ordering artifacts.
- **Shader Graph** surfaces: approximated by Lit fallback.
- **Missing entirely**: particles, VFX Graph, post-processing (tonemap/bloom/SSAO), decals.

---

## 12. Milestones

| # | Deliverable | Gate |
|---|---|---|
| M0 | Format contract + Unity exporter (static path) + validation scene builder | **done** (this session) |
| M1 | MyRender loaders (.mesh/scene/.mat) + Unity V/P/M upload + cull/winding | validation scene renders a correct static frame |
| M2 | Full PBR material mapping (mask/normal/emission/occlusion), Fallback shaders | materials visually match Unity for Lit objects |
| M3 | Directional shadow map (depth pass + PCF) | objects cast/receive shadows |
| M4 | Skinning + baked animation playback | an animated skinned character plays |
| M5 | GardenScene end-to-end (SG fallback, scale/perf) | Garden renders recognizably |

M1–M3 are the "static scene looks right" core; M4 adds motion; M5 is the real-scene stress test.

---

## 13. Open questions / risks

1. **Front-face winding sign** (§5) — resolve empirically at M1.
2. **Normal/mask texture read** for real scenes — needs the source-asset reimport path (§7); validation
   scene sidesteps it.
3. **Animated camera** in Garden/Cockpit is Timeline-driven — v1 exports a static snapshot; sampling a
   Timeline-driven camera per frame is a later add (same `AnimationMode`-style bake, different source).
4. **CPU performance** — Garden/Oasis have large vertex counts; the single-thread vertex/clip pass may
   bottleneck. Measure at M5; possible mitigation: parallelize Pass 1 per object, or cap scene.
5. **Negative-scale objects** — wrong tangent sign until `GetOddNegativeScale` is implemented (§5).
6. **Transparency ordering** — no global sort; acceptable for v1.
