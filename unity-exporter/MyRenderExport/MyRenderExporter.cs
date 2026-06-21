using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
#if UNITY_2019_1_OR_NEWER
using UnityEngine.Rendering.Universal;
#endif

namespace MyRenderExport
{
    /// One-click exporter: walks the active scene and writes scene.json + meshes +
    /// materials + textures into <ExportRoot>/<SceneName>/.
    /// ExportRoot is chosen via MyRender/Settings - Change Export Folder and stored
    /// in EditorPrefs (no hardcoded paths).
    public static class MyRenderExporter
    {
        [MenuItem("MyRender/Export Active Scene")]
        public static void ExportActiveScene()
        {
            string exportRoot = ExportSettings.GetExportRoot();
            if (string.IsNullOrEmpty(exportRoot))
            {
                Debug.LogError("[MyRender] Export cancelled: no export folder selected.");
                return;
            }

            Scene scene = SceneManager.GetActiveScene();
            string outRoot = Path.Combine(exportRoot, Sanitize(scene.name));
            Directory.CreateDirectory(outRoot);

            var meshPaths = new Dictionary<Mesh, string>();
            var matPaths  = new Dictionary<Material, string>();
            var texPaths  = new Dictionary<Texture, string>();

            var objects = new List<string>();
            int exported = 0, skipped = 0;

            foreach (var go in scene.GetRootGameObjects())
            foreach (var r in go.GetComponentsInChildren<Renderer>(false))
            {
                Mesh mesh = GetMesh(r);
                if (mesh == null) { skipped++; continue; }
                if (!(r is MeshRenderer) && !(r is SkinnedMeshRenderer)) { skipped++; continue; }

                string meshRel = ExportMesh(mesh, outRoot, meshPaths);
                var mats = new List<string>();
                foreach (var m in r.sharedMaterials)
                    mats.Add(m == null ? "" : ExportMaterial(m, outRoot, matPaths, texPaths));

                Transform t = r.transform;
                var jb = new Jb();
                jb.Begin();
                jb.Str("name", r.gameObject.name);
                jb.Str("mesh", meshRel);
                jb.StrArray("materials", mats);
                jb.Vec3("position", t.position);
                jb.Quat("rotation", t.rotation);
                jb.Vec3("scale", t.lossyScale);
                jb.Matrix("matrix", t.localToWorldMatrix);
                jb.Matrix("worldToLocal", t.worldToLocalMatrix);
                jb.Bool("skinned", r is SkinnedMeshRenderer);

                string animRel = "";
                if (r is SkinnedMeshRenderer smr)
                {
                    AnimationClip clip = FindClip(smr);
                    if (clip != null) animRel = AnimationExporter.Export(smr, clip, outRoot, 30);
                }
                jb.Str("anim", animRel);
                jb.Int("lightmapIndex", r.lightmapIndex);
                jb.Vec4("lightmapScaleOffset", r.lightmapScaleOffset, last: true);
                jb.End();
                objects.Add(jb.ToString());
                exported++;
            }

            // Export baked lightmap textures (if any).
            // Unity stores HDR lightmaps in RGBM encoding (rgb * alpha * ~8). The
            // default ExportTexture path blits the raw encoded bytes, which would
            // feed the shader under-decoded (far too bright) values. Use a dedicated
            // path that reads via GetPixels() — which decodes RGBM → linear float —
            // and writes the true linear radiance, clamped to [0,1] for RGBA32.
            var lightmapPaths = new List<string>();
            var lmData = LightmapSettings.lightmaps;
            for (int i = 0; i < lmData.Length; i++)
            {
                Texture2D lmTex = lmData[i].lightmapColor;
                string lmRel = lmTex != null ? ExportLightmap(lmTex, outRoot, texPaths) : "";
                lightmapPaths.Add(lmRel);
            }

            string sceneJson = BuildSceneJson(scene.name, objects, lightmapPaths);
            File.WriteAllText(Path.Combine(outRoot, "scene.json"), sceneJson);
            AssetDatabase.Refresh();

            Debug.Log($"[MyRender] Exported '{scene.name}' -> {outRoot}\n" +
                      $"objects: {exported}, skipped: {skipped}, meshes: {meshPaths.Count}, " +
                      $"materials: {matPaths.Count}, textures: {texPaths.Count}");
        }

        // ---- scene.json assembly ----

        static string BuildSceneJson(string name, List<string> objects, List<string> lightmapPaths = null)
        {
            var sb = new StringBuilder();
            sb.Append("{\n");
            sb.Append($"  \"name\": \"{name}\",\n");
            sb.Append($"  \"unityVersion\": \"{Application.unityVersion}\",\n");
            sb.Append("  \"coordinateSystem\": \"unity-lh-yup-zforward-meters\",\n");

            Camera cam = Camera.main;
            if (cam == null) cam = Object.FindObjectOfType<Camera>();
            sb.Append("  \"camera\": ");
            sb.Append(cam != null ? CameraJson(cam) : "null");
            sb.Append(",\n");

            Light light = FindMainLight();
            sb.Append("  \"mainLight\": ");
            sb.Append(light != null ? LightJson(light) : "null");
            sb.Append(",\n");

            Color amb = ComputeFlatAmbient();
            sb.Append($"  \"ambient\": {{ \"color\": [{F(amb.r)}, {F(amb.g)}, {F(amb.b)}], \"intensity\": 1.0 }},\n");

            sb.Append("  \"environment\": ");
            sb.Append(EnvironmentJson());
            sb.Append(",\n");

            sb.Append("  \"fog\": ");
            sb.Append(FogJson());
            sb.Append(",\n");

            sb.Append("  \"postProcessing\": ");
            sb.Append(PostProcessingJson());
            sb.Append(",\n");

            sb.Append("  \"additionalLights\": ");
            sb.Append(AdditionalLightsJson());
            sb.Append(",\n");

            sb.Append("  \"objects\": [\n");
            for (int i = 0; i < objects.Count; i++)
            {
                sb.Append("    ");
                sb.Append(objects[i].Replace("\n", "\n    "));
                sb.Append(i + 1 < objects.Count ? ",\n" : "\n");
            }
            sb.Append("  ]");

            // Lightmap texture paths (may be empty if scene has no baked lightmaps).
            if (lightmapPaths != null && lightmapPaths.Count > 0)
            {
                sb.Append(",\n  \"lightmaps\": [");
                for (int i = 0; i < lightmapPaths.Count; i++)
                {
                    sb.Append($"\"{Esc(lightmapPaths[i])}\"");
                    if (i + 1 < lightmapPaths.Count) sb.Append(", ");
                }
                sb.Append("]");
            }

            sb.Append("\n}\n");
            return sb.ToString();
        }

        // MyRender renders at a fixed 960x540 framebuffer.
        const int TargetWidth  = 960;
        const int TargetHeight = 540;

        static string CameraJson(Camera cam)
        {
            Transform t = cam.transform;

            float prevAspect = cam.aspect;
            cam.aspect = (float)TargetWidth / TargetHeight;
            Matrix4x4 proj = cam.projectionMatrix;
            float usedAspect = cam.aspect;
            cam.ResetAspect();
            cam.aspect = prevAspect;

            var jb = new Jb();
            jb.Begin();
            jb.Vec3("position", t.position);
            jb.Quat("rotation", t.rotation);
            // Verbatim Unity matrices: worldToCamera has the -Z view flip baked in (do NOT invert
            // localToWorld), projectionMatrix is the OpenGL-convention form (NDC z in [-1,1]) the
            // rasterizer expects (do NOT run it through GL.GetGPUProjectionMatrix).
            jb.Matrix("worldToCameraMatrix", cam.worldToCameraMatrix);
            jb.Matrix("projectionMatrix", proj);
            jb.Num("fovVertical", cam.fieldOfView);
            jb.Num("near", cam.nearClipPlane);
            jb.Num("far", cam.farClipPlane);
            jb.Num("aspect", usedAspect);
            jb.Bool("orthographic", cam.orthographic);
            jb.Num("orthoSize", cam.orthographicSize);
            jb.Color4("backgroundColor", cam.backgroundColor.linear, last: true);
            jb.End();
            return jb.ToString();
        }

        static string LightJson(Light l)
        {
            var jb = new Jb();
            jb.Begin();
            jb.Vec3("direction", l.transform.forward);
            jb.Quat("rotation", l.transform.rotation);
            jb.Color3("color", l.color.linear);
            jb.Num("intensity", l.intensity, last: true);
            jb.End();
            return jb.ToString();
        }

        // Export all non-directional lights: Point, Spot (Area not supported in URP real-time).
        static string AdditionalLightsJson()
        {
            var sb2 = new StringBuilder();
            sb2.Append("[");
            bool first = true;
            foreach (var l in Object.FindObjectsOfType<Light>())
            {
                if (!l.isActiveAndEnabled) continue;
                if (l.type == LightType.Directional) continue; // already mainLight
                if (l.type == LightType.Area) continue;        // baked only in URP

                string typeStr = l.type == LightType.Spot ? "spot" : "point";
                Vector3 pos = l.transform.position;
                Color col = l.color.linear;
                float range = l.range;
                float intensity = l.intensity;
                Vector3 dir = l.transform.forward;
                float spotOuter = l.type == LightType.Spot ? l.spotAngle       : 0f;
                float spotInner = l.type == LightType.Spot ? l.innerSpotAngle  : 0f;

                if (!first) sb2.Append(",\n");
                first = false;
                sb2.Append($"{{\"type\":\"{typeStr}\"," +
                           $"\"position\":[{F(pos.x)},{F(pos.y)},{F(pos.z)}]," +
                           $"\"color\":[{F(col.r)},{F(col.g)},{F(col.b)}]," +
                           $"\"intensity\":{F(intensity)}," +
                           $"\"range\":{F(range)}," +
                           $"\"direction\":[{F(dir.x)},{F(dir.y)},{F(dir.z)}]," +
                           $"\"spotAngleOuter\":{F(spotOuter)}," +
                           $"\"spotAngleInner\":{F(spotInner)}}}");
            }
            sb2.Append("]");
            return sb2.ToString();
        }

        // Export sky/equator/ground gradient colors + SH9 for A1/A2.
        // sky/equator/groundColor: from RenderSettings tricolor — these are ambient IRRADIANCE
        //   colors used for lighting, NOT the visual skybox appearance.
        // skyboxVisual*: the actual visual colours you see in the skybox background, read from
        //   the skybox material. Used by SkyboxPass for background pixel colouring.
        // sh[27]: raw SphericalHarmonicsL2 coefficients, channel-major (r*9, g*9, b*9).
        static string EnvironmentJson()
        {
            string modeStr;
            switch (RenderSettings.ambientMode)
            {
                case UnityEngine.Rendering.AmbientMode.Skybox:   modeStr = "skybox";   break;
                case UnityEngine.Rendering.AmbientMode.Trilight: modeStr = "trilight"; break;
                default:                                          modeStr = "color";    break;
            }

            Color sky     = RenderSettings.ambientSkyColor.linear;
            Color equator = RenderSettings.ambientEquatorColor.linear;
            Color ground  = RenderSettings.ambientGroundColor.linear;

            // ---- Visual skybox colours (for background rendering) ----
            // Render the skybox to a tiny cubemap and sample three directions:
            //   +Y = zenith (sky top), +Z = horizon (sky mid), -Y = nadir (sky bot).
            // This captures the actual rendered sky colour (atmospheric scattering, exposure,
            // etc.) rather than just _SkyTint * exposure, which is far too bright.
            Color visTop = new Color(0.5f, 0.5f, 0.5f);
            Color visMid = new Color(0.4f, 0.4f, 0.4f);
            Color visBot = new Color(0.1f, 0.1f, 0.1f);
            float skyboxExposure = 1.0f;

            Material skyMat = RenderSettings.skybox;
            if (skyMat != null && skyMat.HasProperty("_Exposure"))
                skyboxExposure = skyMat.GetFloat("_Exposure");

            if (skyMat != null)
            {
                bool captured = false;
                Cubemap cube = null;
                GameObject camGo = null;
                try
                {
                    cube  = new Cubemap(8, TextureFormat.RGBAFloat, false);
                    camGo = new GameObject("__MyRender_SkyCap__");
                    Camera skyCam = camGo.AddComponent<Camera>();
                    skyCam.clearFlags  = CameraClearFlags.Skybox;
                    skyCam.cullingMask = 0; // sky only — no geometry
                    skyCam.nearClipPlane = 0.01f;
                    skyCam.farClipPlane  = 10000f;
                    captured = skyCam.RenderToCubemap(cube);
                }
                catch (System.Exception ex)
                {
                    UnityEngine.Debug.LogWarning("[MyRenderExporter] RenderToCubemap failed: " + ex.Message);
                }
                finally
                {
                    if (camGo != null) Object.DestroyImmediate(camGo);
                }

                if (captured && cube != null)
                {
                    // Average the 4 centre pixels of each face for stability.
                    Color FaceCenter(Cubemap c, CubemapFace face)
                    {
                        Color[] px = c.GetPixels(face);
                        int n = px.Length, h = (int)Mathf.Sqrt(n);
                        int cy = h / 2, cx = h / 2;
                        // average 2×2 block near centre
                        Color avg = Color.black;
                        int cnt = 0;
                        for (int dy = -1; dy <= 0; dy++) for (int dx = -1; dx <= 0; dx++)
                        {
                            int r = cy+dy, cc = cx+dx;
                            if (r >= 0 && r < h && cc >= 0 && cc < h)
                            { avg += px[r*h + cc]; cnt++; }
                        }
                        return cnt > 0 ? avg / cnt : px[n/2];
                    }
                    visTop = FaceCenter(cube, CubemapFace.PositiveY);
                    visMid = FaceCenter(cube, CubemapFace.PositiveZ);
                    visBot = FaceCenter(cube, CubemapFace.NegativeY);
                    Object.DestroyImmediate(cube);
                }
                else
                {
                    // Fallback: material property reading (less accurate for Procedural sky)
                    if (cube != null) Object.DestroyImmediate(cube);
                    if (skyMat.HasProperty("_SkyTint"))
                    {
                        Color t = skyMat.GetColor("_SkyTint").linear * skyboxExposure;
                        visTop = t;
                        visMid = Color.Lerp(t, visBot, 0.4f);
                    }
                    else if (skyMat.HasProperty("_Tint"))
                    {
                        Color t = skyMat.GetColor("_Tint").linear * skyboxExposure;
                        visTop = t; visMid = t;
                    }
                    else
                    {
                        const float kPi = 3.14159265f;
                        visTop = sky * kPi * skyboxExposure;
                        visMid = equator * kPi * skyboxExposure;
                        visBot = ground * kPi * skyboxExposure;
                    }
                    if (skyMat.HasProperty("_GroundColor"))
                        visBot = skyMat.GetColor("_GroundColor").linear * skyboxExposure;
                }
            }

            // SH9 coefficients for A2: raw SphericalHarmonicsL2 values.
            // Layout: sh[channel*9 + basis], channel: 0=R 1=G 2=B, basis: 0..8 (L0+L1+L2).
            var probe = RenderSettings.ambientProbe;
            var sb2 = new System.Text.StringBuilder();
            sb2.Append("[");
            for (int c = 0; c < 3; c++)
                for (int i = 0; i < 9; i++)
                {
                    sb2.Append(F(probe[c, i]));
                    if (c != 2 || i != 8) sb2.Append(", ");
                }
            sb2.Append("]");

            return $"{{\n  \"ambientMode\": \"{modeStr}\",\n" +
                   $"  \"skyColor\":     [{F(sky.r)}, {F(sky.g)}, {F(sky.b)}],\n" +
                   $"  \"equatorColor\": [{F(equator.r)}, {F(equator.g)}, {F(equator.b)}],\n" +
                   $"  \"groundColor\":  [{F(ground.r)}, {F(ground.g)}, {F(ground.b)}],\n" +
                   $"  \"skyboxVisualTop\": [{F(visTop.r)}, {F(visTop.g)}, {F(visTop.b)}],\n" +
                   $"  \"skyboxVisualMid\": [{F(visMid.r)}, {F(visMid.g)}, {F(visMid.b)}],\n" +
                   $"  \"skyboxVisualBot\": [{F(visBot.r)}, {F(visBot.g)}, {F(visBot.b)}],\n" +
                   $"  \"skyboxExposure\": {F(skyboxExposure)},\n" +
                   $"  \"sh\": {sb2}\n}}";
        }

        // Export realtime fog (RenderSettings.fog). Color is written in linear space to
        // match the renderer's lighting pipeline. density is for exp/exp2; start/end for linear.
        static string FogJson()
        {
            bool enabled = RenderSettings.fog;
            string mode;
            switch (RenderSettings.fogMode)
            {
                case FogMode.Linear:               mode = "linear"; break;
                case FogMode.Exponential:          mode = "exp";    break;
                default:                           mode = "exp2";   break; // ExponentialSquared (Unity default)
            }
            Color c = RenderSettings.fogColor.linear;
            return $"{{ \"enabled\": {(enabled ? "true" : "false")}, \"mode\": \"{mode}\", " +
                   $"\"color\": [{F(c.r)}, {F(c.g)}, {F(c.b)}], " +
                   $"\"density\": {F(RenderSettings.fogDensity)}, " +
                   $"\"start\": {F(RenderSettings.fogStartDistance)}, " +
                   $"\"end\": {F(RenderSettings.fogEndDistance)} }}";
        }

        // Export post-processing Volume settings (tonemapping, bloom, color adjustments).
        // Iterates all global Volumes; takes settings from the first profile that has each component.
        static string PostProcessingJson()
        {
            string tonemapping = "none";
            float  exposure    = 0.0f;   // EV100 post-exposure (ColorAdjustments.postExposure)
            float  contrast    = 0.0f;   // ColorAdjustments.contrast  (-100..100)
            float  saturation  = 0.0f;   // ColorAdjustments.saturation (-100..100)
            float  bloomThresh = 1.0f;
            float  bloomInt    = 0.0f;
            bool   bloomOn     = false;

#if UNITY_2019_1_OR_NEWER
            foreach (var v in Object.FindObjectsOfType<Volume>())
            {
                if (v.profile == null) continue;

                Tonemapping tm;
                if (v.profile.TryGet(out tm) && tm.active)
                {
                    switch (tm.mode.value)
                    {
                        case TonemappingMode.ACES:    tonemapping = "aces";    break;
                        case TonemappingMode.Neutral: tonemapping = "neutral"; break;
                        default:                      tonemapping = "none";    break;
                    }
                }

                ColorAdjustments ca;
                if (v.profile.TryGet(out ca) && ca.active)
                {
                    exposure   = ca.postExposure.value;
                    contrast   = ca.contrast.value;
                    saturation = ca.saturation.value;
                }

                Bloom bl;
                if (v.profile.TryGet(out bl) && bl.active)
                {
                    bloomOn     = true;
                    bloomThresh = bl.threshold.value;
                    bloomInt    = bl.intensity.value;
                }
            }
#endif

            return $"{{\n" +
                   $"  \"tonemapping\": \"{tonemapping}\",\n" +
                   $"  \"postExposure\": {F(exposure)},\n" +
                   $"  \"contrast\": {F(contrast)},\n" +
                   $"  \"saturation\": {F(saturation)},\n" +
                   $"  \"bloomEnabled\": {(bloomOn ? "true" : "false")},\n" +
                   $"  \"bloomThreshold\": {F(bloomThresh)},\n" +
                   $"  \"bloomIntensity\": {F(bloomInt)}\n}}";
        }

        // Flat indirect ambient: in Skybox mode we average the ambient probe over
        // 6 axes; other modes use the plain ambient light color.
        static Color ComputeFlatAmbient()
        {
            if (RenderSettings.ambientMode == UnityEngine.Rendering.AmbientMode.Skybox)
            {
                var probe = RenderSettings.ambientProbe;
                var dirs = new[] { Vector3.up, Vector3.down, Vector3.left,
                                   Vector3.right, Vector3.forward, Vector3.back };
                var res = new Color[6];
                probe.Evaluate(dirs, res);
                Color sum = Color.black;
                foreach (var c in res) sum += c;
                return sum / 6f;
            }
            return RenderSettings.ambientLight.linear;
        }

        static Light FindMainLight()
        {
            Light best = null;
            foreach (var l in Object.FindObjectsOfType<Light>())
            {
                if (l.type != LightType.Directional || !l.enabled) continue;
                if (best == null || l.intensity > best.intensity) best = l;
            }
            return best;
        }

        // ---- mesh ----

        static string ExportMesh(Mesh mesh, string outRoot, Dictionary<Mesh, string> cache)
        {
            if (cache.TryGetValue(mesh, out var rel)) return rel;
            string name = Sanitize(mesh.name);
            if (string.IsNullOrEmpty(name)) name = "mesh";
            rel = "meshes/" + name + "_" + mesh.GetInstanceID() + ".mesh";
            MeshWriter.Write(mesh, Path.Combine(outRoot, rel));
            cache[mesh] = rel;
            return rel;
        }

        static Mesh GetMesh(Renderer r)
        {
            if (r is SkinnedMeshRenderer smr) return smr.sharedMesh;
            var mf = r.GetComponent<MeshFilter>();
            return mf != null ? mf.sharedMesh : null;
        }

        static AnimationClip FindClip(SkinnedMeshRenderer smr)
        {
            var legacy = smr.GetComponentInParent<Animation>();
            if (legacy != null && legacy.clip != null) return legacy.clip;
            var animator = smr.GetComponentInParent<Animator>();
            if (animator != null && animator.runtimeAnimatorController != null)
            {
                var clips = animator.runtimeAnimatorController.animationClips;
                if (clips != null && clips.Length > 0) return clips[0];
            }
            return null;
        }

        // ---- material ----

        static string ExportMaterial(Material m, string outRoot, Dictionary<Material, string> cache,
                                     Dictionary<Texture, string> texCache)
        {
            if (cache.TryGetValue(m, out var rel)) return rel;

            string shaderName = m.shader != null ? m.shader.name : "";
            string model;
            if      (shaderName == "Universal Render Pipeline/Lit")        model = "Lit";
            else if (shaderName == "Universal Render Pipeline/Simple Lit") model = "SimpleLit";
            else if (shaderName == "Universal Render Pipeline/Unlit")      model = "Unlit";
            else                                                            model = "Fallback";

            string Tex(string prop, bool linear, bool isNormal = false)
            {
                if (!m.HasProperty(prop)) return "";
                var tex = m.GetTexture(prop);
                return ExportTexture(tex, outRoot, texCache, linear, isNormal);
            }
            Color Col(string prop, Color def) => m.HasProperty(prop) ? m.GetColor(prop) : def;
            float Flt(string prop, float def) => m.HasProperty(prop) ? m.GetFloat(prop) : def;
            Vector2 Scale(string prop) => m.HasProperty(prop) ? m.GetTextureScale(prop) : Vector2.one;
            Vector2 Off(string prop)   => m.HasProperty(prop) ? m.GetTextureOffset(prop) : Vector2.zero;

            string baseMap = Tex("_BaseMap", false);
            if (string.IsNullOrEmpty(baseMap)) baseMap = Tex("_MainTex", false);
            Color baseColor = m.HasProperty("_BaseColor") ? m.GetColor("_BaseColor") : Col("_Color", Color.white);

            int cull = (int)Flt("_Cull", 2);
            string cullStr = cull == 0 ? "off" : cull == 1 ? "front" : "back";
            int surface = (int)Flt("_Surface", 0);

            var jb = new Jb();
            jb.Begin();
            jb.Str("name", m.name);
            jb.Str("shaderModel", model);
            jb.Str("sourceShader", shaderName);
            jb.Str("surfaceType", surface == 1 ? "transparent" : "opaque");
            jb.Str("cull", cullStr);
            jb.Bool("alphaClip", Flt("_AlphaClip", 0) > 0.5f);
            jb.Num("cutoff", Flt("_Cutoff", 0.5f));
            jb.Color4("baseColor", baseColor);
            jb.Str("baseMap", baseMap);
            jb.Vec2("tiling", Scale("_BaseMap"));
            jb.Vec2("offset", Off("_BaseMap"));
            jb.Str("normalMap", Tex("_BumpMap", true, isNormal: true));
            jb.Num("normalScale", Flt("_BumpScale", 1f));
            jb.Num("metallic", Flt("_Metallic", 0f));
            jb.Num("smoothness", Flt("_Smoothness", 0.5f));
            jb.Str("metallicGlossMap", Tex("_MetallicGlossMap", true));
            jb.Str("smoothnessChannel", Flt("_SmoothnessTextureChannel", 0) > 0.5f ? "albedoAlpha" : "metallicAlpha");
            jb.Str("occlusionMap", Tex("_OcclusionMap", true));
            jb.Num("occlusionStrength", Flt("_OcclusionStrength", 1f));
            jb.Color3("emissionColor", Col("_EmissionColor", Color.black));
            jb.Str("emissionMap", Tex("_EmissionMap", false), last: true);
            jb.End();

            string name = Sanitize(m.name);
            if (string.IsNullOrEmpty(name)) name = "mat_" + m.GetInstanceID();
            rel = "materials/" + name + ".mat.json";
            string abs = Path.Combine(outRoot, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(abs));
            File.WriteAllText(abs, jb.ToString());
            cache[m] = rel;
            return rel;
        }

        static string ExportTexture(Texture tex, string outRoot, Dictionary<Texture, string> cache, bool linear, bool isNormal = false)
        {
            if (tex == null) return "";
            if (cache.TryGetValue(tex, out var rel)) return rel;
            rel = TextureExporter.Export(tex, outRoot, linear, isNormal);
            if (!string.IsNullOrEmpty(rel)) cache[tex] = rel;
            return rel;
        }

        // Export a baked HDR lightmap, decoding Unity's RGBM encoding to true linear
        // radiance. GetPixels() on a lightmap Texture2D already decodes RGBM → float
        // (this is the API contract for lightmapColor), so we just need a readable
        // copy of the source asset and write its float pixels clamped to [0,1].
        // The runtime shader then samples plain linear values — no RGBM decode needed.
        static string ExportLightmap(Texture tex, string outRoot, Dictionary<Texture, string> cache)
        {
            if (tex == null) return "";
            if (cache.TryGetValue(tex, out var rel)) return rel;

            var tex2d = tex as Texture2D;
            if (tex2d == null) return "";

            string name = Sanitize(tex2d.name);
            if (string.IsNullOrEmpty(name)) name = "tex_" + tex2d.GetInstanceID();
            rel = "textures/" + name + ".tga";
            string abs = Path.Combine(outRoot, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(abs));

            // Make the source asset readable+uncompressed so GetPixels works and the
            // RGBM decode runs. Import as a Default (non-lightmap) linear texture so
            // GetPixels returns decoded linear float, then restore settings.
            string assetPath = AssetDatabase.GetAssetPath(tex2d);
            var importer = string.IsNullOrEmpty(assetPath)
                ? null : AssetImporter.GetAtPath(assetPath) as TextureImporter;

            Texture2D readable;
            bool reimported = false;
            if (importer != null && (!importer.isReadable ||
                                      importer.textureCompression != TextureImporterCompression.Uncompressed))
            {
                var oType = importer.textureType;
                var oComp = importer.textureCompression;
                var oRead = importer.isReadable;
                var oSRGB = importer.sRGBTexture;
                importer.textureType        = TextureImporterType.Default;
                importer.textureCompression = TextureImporterCompression.Uncompressed;
                importer.isReadable         = true;
                importer.sRGBTexture        = false; // lightmaps are linear data
                importer.SaveAndReimport();
                reimported = true;
                var src = AssetDatabase.LoadAssetAtPath<Texture2D>(assetPath);
                readable = new Texture2D(src.width, src.height, TextureFormat.RGBAFloat, false, true);
                readable.SetPixels(src.GetPixels()); // GetPixels decodes RGBM → linear float
                readable.Apply();
                if (reimported)
                {
                    importer.textureType        = oType;
                    importer.textureCompression = oComp;
                    importer.isReadable         = oRead;
                    importer.sRGBTexture        = oSRGB;
                    importer.SaveAndReimport();
                }
            }
            else
            {
                // Already readable: GetPixels decodes RGBM directly.
                readable = new Texture2D(tex2d.width, tex2d.height, TextureFormat.RGBAFloat, false, true);
                readable.SetPixels(tex2d.GetPixels());
                readable.Apply();
            }

            // Write as TGA in linear space. EncodeToTGA on an RGBAFloat texture keeps
            // the float values, clamped to [0,1] for the 8-bit file.
            File.WriteAllBytes(abs, ImageConversion.EncodeToTGA(readable));
            Object.DestroyImmediate(readable);
            cache[tex] = rel;
            return rel;
        }

        // ---- helpers ----

        static string Sanitize(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            foreach (char c in Path.GetInvalidFileNameChars()) s = s.Replace(c, '_');
            return s.Replace(' ', '_');
        }

        static string F(float v) => v.ToString("R", CultureInfo.InvariantCulture);

        // JSON string escaping (shared by BuildSceneJson and the Jb builder below).
        static string Esc(string s) => string.IsNullOrEmpty(s) ? "" : s.Replace("\\", "\\\\").Replace("\"", "\\\"");

        /// Tiny JSON object builder with fixed-order fields.
        class Jb
        {
            readonly StringBuilder _sb = new StringBuilder();
            public void Begin() => _sb.Append("{\n");
            public void End()   => _sb.Append("}");
            public override string ToString() => _sb.ToString();

            void Key(string k) => _sb.Append($"  \"{k}\": ");
            void Tail(bool last) => _sb.Append(last ? "\n" : ",\n");

            public void Str(string k, string v, bool last = false) { Key(k); _sb.Append($"\"{Esc(v)}\""); Tail(last); }
            public void Num(string k, float v, bool last = false) { Key(k); _sb.Append(F(v)); Tail(last); }
            public void Int(string k, int v, bool last = false) { Key(k); _sb.Append(v); Tail(last); }
            public void Bool(string k, bool v, bool last = false) { Key(k); _sb.Append(v ? "true" : "false"); Tail(last); }
            public void Vec2(string k, Vector2 v, bool last = false) { Key(k); _sb.Append($"[{F(v.x)}, {F(v.y)}]"); Tail(last); }
            public void Vec3(string k, Vector3 v, bool last = false) { Key(k); _sb.Append($"[{F(v.x)}, {F(v.y)}, {F(v.z)}]"); Tail(last); }
            public void Vec4(string k, Vector4 v, bool last = false) { Key(k); _sb.Append($"[{F(v.x)}, {F(v.y)}, {F(v.z)}, {F(v.w)}]"); Tail(last); }
            public void Quat(string k, Quaternion q, bool last = false) { Key(k); _sb.Append($"[{F(q.x)}, {F(q.y)}, {F(q.z)}, {F(q.w)}]"); Tail(last); }
            public void Color3(string k, Color c, bool last = false) { Key(k); _sb.Append($"[{F(c.r)}, {F(c.g)}, {F(c.b)}]"); Tail(last); }
            public void Color4(string k, Color c, bool last = false) { Key(k); _sb.Append($"[{F(c.r)}, {F(c.g)}, {F(c.b)}, {F(c.a)}]"); Tail(last); }

            public void Matrix(string k, Matrix4x4 m, bool last = false)
            {
                Key(k);
                _sb.Append("[");
                for (int row = 0; row < 4; row++)
                    for (int col = 0; col < 4; col++)
                    {
                        _sb.Append(F(m[row, col]));
                        if (row != 3 || col != 3) _sb.Append(", ");
                    }
                _sb.Append("]");
                Tail(last);
            }

            public void StrArray(string k, List<string> items, bool last = false)
            {
                Key(k);
                _sb.Append("[");
                for (int i = 0; i < items.Count; i++)
                {
                    _sb.Append($"\"{Esc(items[i])}\"");
                    if (i + 1 < items.Count) _sb.Append(", ");
                }
                _sb.Append("]");
                Tail(last);
            }

            // Esc lives on the outer class MyRenderExporter; visible to this nested type.
        }
    }
}
