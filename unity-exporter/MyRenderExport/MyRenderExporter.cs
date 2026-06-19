using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

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
                jb.Str("anim", animRel, last: true);
                jb.End();
                objects.Add(jb.ToString());
                exported++;
            }

            string sceneJson = BuildSceneJson(scene.name, objects);
            File.WriteAllText(Path.Combine(outRoot, "scene.json"), sceneJson);
            AssetDatabase.Refresh();

            Debug.Log($"[MyRender] Exported '{scene.name}' -> {outRoot}\n" +
                      $"objects: {exported}, skipped: {skipped}, meshes: {meshPaths.Count}, " +
                      $"materials: {matPaths.Count}, textures: {texPaths.Count}");
        }

        // ---- scene.json assembly ----

        static string BuildSceneJson(string name, List<string> objects)
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

            sb.Append("  \"objects\": [\n");
            for (int i = 0; i < objects.Count; i++)
            {
                sb.Append("    ");
                sb.Append(objects[i].Replace("\n", "\n    "));
                sb.Append(i + 1 < objects.Count ? ",\n" : "\n");
            }
            sb.Append("  ]\n}\n");
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
            string model = shaderName switch
            {
                "Universal Render Pipeline/Lit"        => "Lit",
                "Universal Render Pipeline/Simple Lit" => "SimpleLit",
                "Universal Render Pipeline/Unlit"      => "Unlit",
                _                                       => "Fallback",
            };

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

        // ---- helpers ----

        static string Sanitize(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            foreach (char c in Path.GetInvalidFileNameChars()) s = s.Replace(c, '_');
            return s.Replace(' ', '_');
        }

        static string F(float v) => v.ToString("R", CultureInfo.InvariantCulture);

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
            public void Bool(string k, bool v, bool last = false) { Key(k); _sb.Append(v ? "true" : "false"); Tail(last); }
            public void Vec2(string k, Vector2 v, bool last = false) { Key(k); _sb.Append($"[{F(v.x)}, {F(v.y)}]"); Tail(last); }
            public void Vec3(string k, Vector3 v, bool last = false) { Key(k); _sb.Append($"[{F(v.x)}, {F(v.y)}, {F(v.z)}]"); Tail(last); }
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

            static string Esc(string s) => string.IsNullOrEmpty(s) ? "" : s.Replace("\\", "\\\\").Replace("\"", "\\\"");
        }
    }
}
