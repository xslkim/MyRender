# MyRender Asset Format (v1)

The contract between the Unity exporter (`urp_sample/Assets/Editor/MyRenderExport`)
and the MyRender runtime. Both sides build against this document.

## Conventions

- **Coordinate system**: Unity world space, left-handed, **+X right, +Y up, +Z forward**, units = **meters**.
  Values are exported **verbatim** — no axis flip, no winding reversal. The renderer adopts this system.
- **Endianness**: little-endian.
- **Matrices**: stored **row-major**, 16 `f32` (`m00 m01 m02 m03 m10 ...`). A transform matrix is `localToWorld`.
- **Quaternions**: `[x, y, z, w]`.
- **Colors**: linear-ish as authored in Unity (`[r, g, b, a]`, floats). Textures carry their own color space (see below).
- **Front face**: clockwise in screen space (Unity default). Renderer culls per material `cull`.
- **Paths** in JSON are relative to the exported scene root, forward slashes.

Output layout (one folder per exported scene), default root `G:/MyRender/assets/unity_export/<SceneName>/`:

```
<SceneName>/
  scene.json
  meshes/<guid_or_name>.mesh
  materials/<name>.mat.json
  textures/<name>.tga
```

Meshes/materials/textures are de-duplicated by source asset so multiple objects share one file.

---

## scene.json

```jsonc
{
  "name": "ValidationScene",
  "coordinateSystem": "unity-lh-yup-zforward-meters",
  "camera": {
    "position": [x, y, z],
    "rotation": [qx, qy, qz, qw],
    "matrix":   [16 floats],          // worldToCamera convenience (== view matrix)
    "fovVertical": 60.0,              // degrees
    "near": 0.3, "far": 1000.0,
    "aspect": 1.7777778,
    "orthographic": false,
    "orthoSize": 5.0,                 // only when orthographic
    "backgroundColor": [r, g, b, a]
  },
  "mainLight": {                       // single directional light (brightest in scene)
    "direction": [x, y, z],           // world-space direction the light travels
    "rotation":  [qx, qy, qz, qw],
    "color": [r, g, b],
    "intensity": 1.0
  },
  "ambient": { "color": [r, g, b], "intensity": 1.0 },
  "objects": [
    {
      "name": "Cube",
      "mesh": "meshes/abc123.mesh",
      "materials": ["materials/Foo.mat.json"],  // one per submesh, in submesh order
      "position": [x, y, z],
      "rotation": [qx, qy, qz, qw],
      "scale":    [x, y, z],
      "matrix":   [16 floats],        // localToWorld; renderer uses this directly
      "skinned": false                // true => mesh has skin block, uses anim (v2)
    }
  ]
}
```

---

## .mesh (binary)

```
offset  type      field
0       char[4]   magic = "MRSH"
4       u16       version = 1
6       u16       flags          (bit0 hasSkin, bit1 hasUV1, bit2 hasColor)
8       u32       vertexCount
12      u32       indexCount     (always triangles, 3 per face)
16      u32       submeshCount
20      u32       boneCount      (0 unless hasSkin)

submeshCount x { u32 indexStart; u32 indexCount; }   // submesh -> index range

vertexCount x {                  // interleaved
  f32 position[3]
  f32 normal[3]
  f32 tangent[4]                 // xyz + w (bitangent sign)
  f32 uv0[2]
  f32 uv1[2]                     // only if flags.hasUV1
  f32 color[4]                   // only if flags.hasColor
  u16 boneIndex[4]               // only if flags.hasSkin
  f32 boneWeight[4]              // only if flags.hasSkin (normalized, sums to 1)
}

indexCount x u32                 // triangle indices, front face = clockwise

// only if flags.hasSkin:
boneCount x f32[16]              // bindposes (row-major), one per bone
```

Skin path (`hasSkin`) is **v2**; v1 exporter writes `flags = hasUV1|hasColor` subset only, `boneCount = 0`.

---

## .mat.json

```jsonc
{
  "name": "BambooStructure_01_Mat",
  "shaderModel": "Lit",            // Lit | SimpleLit | Unlit | Fallback
  "sourceShader": "Universal Render Pipeline/Lit",
  "surfaceType": "opaque",         // opaque | transparent
  "cull": "back",                  // back | front | off
  "alphaClip": false, "cutoff": 0.5,

  "baseColor": [1, 1, 1, 1],
  "baseMap": "textures/Bamboo_Albedo.tga",   // sRGB
  "tiling": [1, 1], "offset": [0, 0],

  "normalMap": "textures/Bamboo_Normal.tga", // linear
  "normalScale": 1.0,

  // URP packs metallic in R, smoothness in A of _MetallicGlossMap.
  // smoothnessChannel: "metallicAlpha" (default) or "albedoAlpha".
  "metallic": 0.0, "smoothness": 1.0,
  "metallicGlossMap": "textures/Bamboo_Mask.tga",  // linear
  "smoothnessChannel": "metallicAlpha",

  "occlusionMap": "textures/Bamboo_Mask.tga",      // linear
  "occlusionStrength": 1.0,

  "emissionColor": [0, 0, 0],
  "emissionMap": ""                                 // sRGB
}
```

`shaderModel` mapping:
- `Universal Render Pipeline/Lit` -> `Lit`
- `Universal Render Pipeline/Simple Lit` -> `SimpleLit`
- `Universal Render Pipeline/Unlit` -> `Unlit`
- anything else (Shader Graph, custom) -> `Fallback`: exporter probes for `_BaseMap/_MainTex`
  and `_BaseColor/_Color`, renders as Lit with whatever it finds.

---

## Textures

- Format: **TGA** (24/32-bit), via `ImageConversion.EncodeToTGA`.
- Color textures (baseMap, emissionMap) keep sRGB; the renderer does sRGB->linear on load.
- Data textures (normalMap, metallicGlossMap, occlusionMap) are linear; renderer skips sRGB conversion.
- Normal / mask maps are read from the **source asset** with compression+sRGB temporarily disabled
  to avoid BC5 swizzle, then restored.
