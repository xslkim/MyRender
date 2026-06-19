# MyRender Unity Exporter

Editor scripts that export a Unity URP scene to the MyRender binary asset format
(`.mesh` + `.mat.json` + `.tga` textures + `scene.json`).

## Supported Unity versions

| Unity | URP | Status |
|-------|-----|--------|
| 2019.4 LTS | 7.x | Supported |
| 2022.3 LTS | 14.x | Supported |

Version-specific API differences are isolated with `#if UNITY_2021_2_OR_NEWER`
(and similar) blocks inside the relevant `.cs` files. The common path (~95% of
the code) is shared.

## Installation

Copy the `MyRenderExport/` folder into your Unity project's `Assets/Editor/`
directory:

```
<YourProject>/Assets/Editor/MyRenderExport/
    ExportSettings.cs
    MyRenderExporter.cs
    MeshWriter.cs
    TextureExporter.cs
    AnimationExporter.cs
    ReferenceCapture.cs
    ValidationSceneBuilder.cs   (optional — only needed for the built-in validation scene)
```

No `.asmdef` or Package Manager setup is required. Unity will compile the
scripts automatically on the next domain reload.

## First-time setup

1. Open your Unity project and wait for compilation.
2. In the menu bar: **MyRender → Settings - Change Export Folder**
3. Pick the folder where scene exports should land, e.g. `G:/MyRender/assets/unity_export`.
   This is saved in `EditorPrefs` and remembered across sessions. The exporter
   creates one sub-folder per scene inside it.

## Usage

| Menu item | What it does |
|-----------|-------------|
| **MyRender → Export Active Scene** | Exports meshes / materials / textures + `scene.json` for the currently open scene |
| **MyRender → Capture Reference PNG** | Renders the scene camera at 960×540 to `<export_root>/<SceneName>/unity_ref.png` — use this as the ground-truth reference image |
| **MyRender → Settings - Change Export Folder** | Re-pick the export root folder |
| **MyRender → Build Validation Scene** | Creates the built-in validation scene (URP/Lit primitives + textured sphere + skinned bar) |

## Output layout

```
<export_root>/
  <SceneName>/
    scene.json          ← camera, light, ambient, object list
    unity_ref.png       ← reference capture (960x540 PNG)
    meshes/
      *.mesh            ← binary mesh (MRSH format, see docs/MyRender_AssetFormat.md)
    materials/
      *.mat.json        ← material parameters
    textures/
      *.tga             ← exported textures
    anims/
      *.anim            ← baked skeletal animations (MRAN format)
```

## scene.json fields

New in this version:

- `unityVersion` — e.g. `"2019.4.40f1"` — informational, not parsed by the renderer
- Output path is no longer hardcoded; `ReferenceCapture` saves `unity_ref.png`
  into the scene export folder instead of a fixed `G:/MyRender/out/` path

## Running MyRender

```
MyRender.exe --unity <scene_dir>
MyRender.exe --capture-unity <scene_dir> <out.png>
```

where `<scene_dir>` is the `<SceneName>` folder produced by the exporter.
