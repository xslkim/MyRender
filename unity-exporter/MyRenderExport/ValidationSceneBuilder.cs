using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace MyRenderExport
{
    /// Builds a minimal URP/Lit scene to validate the export -> render pipeline end to end:
    /// ground + a few colored primitives, one directional light, one static camera.
    public static class ValidationSceneBuilder
    {
        const string ScenePath = "Assets/Scenes/MyRenderValidation/ValidationScene.unity";

        [MenuItem("MyRender/Build Validation Scene")]
        public static void Build()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            // Directional light (angled so normal mapping / shading is visible).
            var lightGo = new GameObject("Directional Light");
            var light = lightGo.AddComponent<Light>();
            light.type = LightType.Directional;
            light.color = Color.white;
            light.intensity = 1f;
            light.shadows = LightShadows.Soft;
            lightGo.transform.rotation = Quaternion.Euler(50f, -30f, 0f);

            // Ground.
            var ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
            ground.name = "Ground";
            ground.transform.localScale = new Vector3(2, 1, 2);
            ground.GetComponent<MeshRenderer>().sharedMaterial = MakeLit("Ground_Mat", new Color(0.6f, 0.6f, 0.6f), 0f, 0.3f);

            // A row of primitives with different metallic/smoothness.
            MakePrimitive(PrimitiveType.Cube,    "Cube",    new Vector3(-2.5f, 0.5f, 0), new Color(0.8f, 0.2f, 0.2f), 0f, 0.5f);
            MakePrimitive(PrimitiveType.Sphere,  "Sphere",  new Vector3( 0f,   0.5f, 0), new Color(0.2f, 0.6f, 0.9f), 1f, 0.8f);
            MakePrimitive(PrimitiveType.Capsule, "Capsule", new Vector3( 2.5f, 1.0f, 0), new Color(0.3f, 0.8f, 0.3f), 0f, 0.9f);

            // Fully-textured sphere floating above the row — exercises every M2 map:
            // albedo + normal + metallic/smoothness mask + occlusion + emission.
            MakeTexturedSphere("Textured", new Vector3(0f, 2.4f, 0f), 1.5f);

            // M4: a 2-bone skinned bar that bends — validates skin export + LBS.
            MakeBendingBar("Bar", new Vector3(4.5f, 0f, 1.5f));

            // Camera looking at the row.
            var camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            var cam = camGo.AddComponent<Camera>();
            cam.backgroundColor = new Color(0.12f, 0.14f, 0.18f, 1f);
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.fieldOfView = 60f;
            camGo.transform.position = new Vector3(0f, 3f, -7f);
            camGo.transform.rotation = Quaternion.LookRotation(new Vector3(0f, 0.5f, 0f) - camGo.transform.position, Vector3.up);

            Directory.CreateDirectory(Path.GetDirectoryName(ScenePath));
            EditorSceneManager.SaveScene(scene, ScenePath);
            Debug.Log($"[MyRender] Built validation scene at {ScenePath}. Now run MyRender/Export Active Scene.");
        }

        static void MakePrimitive(PrimitiveType type, string name, Vector3 pos, Color color, float metallic, float smoothness)
        {
            var go = GameObject.CreatePrimitive(type);
            go.name = name;
            go.transform.position = pos;
            go.GetComponent<MeshRenderer>().sharedMaterial = MakeLit(name + "_Mat", color, metallic, smoothness);
        }

        static Material MakeLit(string name, Color color, float metallic, float smoothness)
        {
            var shader = Shader.Find("Universal Render Pipeline/Lit");
            var m = new Material(shader) { name = name };
            m.SetColor("_BaseColor", color);
            m.SetFloat("_Metallic", metallic);
            m.SetFloat("_Smoothness", smoothness);
            return m;
        }

        // ---- M4: procedural 2-bone skinned bar ----

        const string MeshDir = "Assets/Scenes/MyRenderValidation/Meshes";
        const float  BarHeight  = 4f;     // bar runs from y=0 to y=BarHeight
        const float  BarRadius  = 0.3f;   // square cross-section half-width
        const float  JointY     = 2f;     // bone1 pivot; blend region [JointY-1, JointY+1]
        const int    BarSegments = 16;    // rings along Y (more = smoother bend)

        // Builds a skinned bar: bone0 (root) + bone1 (child at the joint). Bone1
        // bends around Z, sweeping the upper half. Mesh, bindposes and weights are
        // authored so the bind pose maps mesh-local -> world via the root transform.
        static void MakeBendingBar(string name, Vector3 pos)
        {
            var root = new GameObject(name);
            root.transform.position = pos;

            var bone0 = new GameObject("Bone0").transform;
            bone0.SetParent(root.transform, false);
            bone0.localPosition = Vector3.zero;

            var bone1 = new GameObject("Bone1").transform;
            bone1.SetParent(bone0, false);
            bone1.localPosition = new Vector3(0f, JointY, 0f); // pivot at the joint
            Transform[] bones = { bone0, bone1 };

            Mesh mesh = BuildBarMesh(bones, root.transform);
            Directory.CreateDirectory(MeshDir);
            string meshPath = MeshDir + "/" + name + ".asset";
            AssetDatabase.CreateAsset(mesh, meshPath);

            var smrGo = new GameObject("BarMesh");
            smrGo.transform.SetParent(root.transform, false);
            var smr = smrGo.AddComponent<SkinnedMeshRenderer>();
            smr.sharedMesh   = mesh;
            smr.bones        = bones;
            smr.rootBone     = bone0;
            smr.sharedMaterial = MakeLit(name + "_Mat", new Color(0.9f, 0.7f, 0.2f), 0f, 0.4f);
            smr.updateWhenOffscreen = true;

            AnimationClip clip = BuildBendClip(name);
            var anim = root.AddComponent<Animation>();
            anim.AddClip(clip, clip.name);
            anim.clip = clip;
        }

        // Vertical bar as a stack of square rings; bone1 weight ramps across the joint.
        static Mesh BuildBarMesh(Transform[] bones, Transform root)
        {
            int rings = BarSegments + 1;
            var verts   = new Vector3[rings * 4];
            var normals = new Vector3[rings * 4];
            var uvs      = new Vector2[rings * 4];
            var weights = new BoneWeight[rings * 4];

            // 4 corners of the square cross-section (XZ), with outward normals.
            Vector2[] corner = { new Vector2(-1,-1), new Vector2(1,-1), new Vector2(1,1), new Vector2(-1,1) };

            for (int r = 0; r < rings; r++)
            {
                float h = r / (float)BarSegments * BarHeight;
                float w1 = Mathf.Clamp01((h - (JointY - 1f)) / 2f); // 0 below, 1 above joint
                for (int c = 0; c < 4; c++)
                {
                    int i = r * 4 + c;
                    verts[i]   = new Vector3(corner[c].x * BarRadius, h, corner[c].y * BarRadius);
                    normals[i] = new Vector3(corner[c].x, 0f, corner[c].y).normalized;
                    uvs[i]     = new Vector2(c / 4f, h / BarHeight);
                    weights[i] = new BoneWeight {
                        boneIndex0 = 0, weight0 = 1f - w1,
                        boneIndex1 = 1, weight1 = w1,
                    };
                }
            }

            // Side quads between consecutive rings.
            var tris = new System.Collections.Generic.List<int>();
            for (int r = 0; r < BarSegments; r++)
            for (int c = 0; c < 4; c++)
            {
                int a = r * 4 + c;
                int b = r * 4 + (c + 1) % 4;
                int d = (r + 1) * 4 + c;
                int e = (r + 1) * 4 + (c + 1) % 4;
                tris.Add(a); tris.Add(d); tris.Add(b);
                tris.Add(b); tris.Add(d); tris.Add(e);
            }

            var mesh = new Mesh { name = "Bar" };
            mesh.vertices    = verts;
            mesh.normals     = normals;
            mesh.uv          = uvs;
            mesh.boneWeights = weights;
            mesh.triangles   = tris.ToArray();

            // bindpose[b] = bone.worldToLocal · root.localToWorld, so at rest the
            // weighted sum maps a root-local vertex back to world (design §9.2).
            var bind = new Matrix4x4[bones.Length];
            for (int b = 0; b < bones.Length; b++)
                bind[b] = bones[b].worldToLocalMatrix * root.localToWorldMatrix;
            mesh.bindposes = bind;
            mesh.RecalculateTangents();
            return mesh;
        }

        // A looping clip that bends bone1 around Z: 0 -> 45 -> 0 -> -45 -> 0 over 2.4s.
        static AnimationClip BuildBendClip(string name)
        {
            float[] times  = { 0f, 0.6f, 1.2f, 1.8f, 2.4f };
            float[] anglesDeg = { 0f, 45f, 0f, -45f, 0f };

            var kx = new Keyframe[times.Length];
            var ky = new Keyframe[times.Length];
            var kz = new Keyframe[times.Length];
            var kw = new Keyframe[times.Length];
            for (int i = 0; i < times.Length; i++)
            {
                Quaternion q = Quaternion.AngleAxis(anglesDeg[i], Vector3.forward);
                kx[i] = new Keyframe(times[i], q.x);
                ky[i] = new Keyframe(times[i], q.y);
                kz[i] = new Keyframe(times[i], q.z);
                kw[i] = new Keyframe(times[i], q.w);
            }

            var clip = new AnimationClip { name = name + "_Bend", legacy = true, wrapMode = WrapMode.Loop };
            const string path = "Bone0/Bone1";
            clip.SetCurve(path, typeof(Transform), "m_LocalRotation.x", new AnimationCurve(kx));
            clip.SetCurve(path, typeof(Transform), "m_LocalRotation.y", new AnimationCurve(ky));
            clip.SetCurve(path, typeof(Transform), "m_LocalRotation.z", new AnimationCurve(kz));
            clip.SetCurve(path, typeof(Transform), "m_LocalRotation.w", new AnimationCurve(kw));
            clip.EnsureQuaternionContinuity();

            Directory.CreateDirectory(MeshDir);
            string clipPath = MeshDir + "/" + name + "_Bend.anim";
            AssetDatabase.CreateAsset(clip, clipPath);
            return clip;
        }

        // ---- textured M2 validation object ----

        const string TexDir = "Assets/Scenes/MyRenderValidation/Textures";

        static void MakeTexturedSphere(string name, Vector3 pos, float scale)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            go.name = name;
            go.transform.position = pos;
            go.transform.localScale = new Vector3(scale, scale, scale);

            var albedo   = SaveTex(name + "_Albedo",   GenAlbedo(),       sRGB: true,  normal: false);
            var normal   = SaveTex(name + "_Normal",   GenNormal(),       sRGB: false, normal: true);
            var mask     = SaveTex(name + "_Mask",     GenMetallicGloss(),sRGB: false, normal: false);
            var occ      = SaveTex(name + "_AO",       GenOcclusion(),    sRGB: false, normal: false);
            var emis     = SaveTex(name + "_Emission", GenEmission(),     sRGB: true,  normal: false);

            var shader = Shader.Find("Universal Render Pipeline/Lit");
            var m = new Material(shader) { name = name + "_Mat" };
            m.SetColor("_BaseColor", Color.white);
            m.SetTexture("_BaseMap", albedo);

            m.SetTexture("_BumpMap", normal);
            m.SetFloat("_BumpScale", 1f);
            m.EnableKeyword("_NORMALMAP");

            m.SetTexture("_MetallicGlossMap", mask);
            m.SetFloat("_Metallic", 1f);
            m.SetFloat("_Smoothness", 1f);
            m.EnableKeyword("_METALLICSPECGLOSSMAP");

            m.SetTexture("_OcclusionMap", occ);
            m.SetFloat("_OcclusionStrength", 1f);
            m.EnableKeyword("_OCCLUSIONMAP");

            m.SetTexture("_EmissionMap", emis);
            m.SetColor("_EmissionColor", Color.white);
            m.EnableKeyword("_EMISSION");
            m.globalIlluminationFlags = MaterialGlobalIlluminationFlags.RealtimeEmissive;

            go.GetComponent<MeshRenderer>().sharedMaterial = m;
        }

        // Write a generated texture to a PNG asset and reimport uncompressed+readable
        // so the exporter's simple Blit+ReadPixels path is byte-correct.
        static Texture2D SaveTex(string name, Texture2D tex, bool sRGB, bool normal)
        {
            Directory.CreateDirectory(TexDir);
            string path = TexDir + "/" + name + ".png";
            File.WriteAllBytes(path, ImageConversion.EncodeToPNG(tex));
            Object.DestroyImmediate(tex);
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceSynchronousImport);

            var ti = (TextureImporter)AssetImporter.GetAtPath(path);
            ti.textureType        = normal ? TextureImporterType.NormalMap : TextureImporterType.Default;
            ti.sRGBTexture        = sRGB;
            ti.isReadable         = true;
            ti.textureCompression = TextureImporterCompression.Uncompressed;
            ti.wrapMode           = TextureWrapMode.Repeat;
            ti.mipmapEnabled      = false;
            ti.SaveAndReimport();

            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        const int TexSize = 256;

        static Texture2D NewTex()
        {
            return new Texture2D(TexSize, TexSize, TextureFormat.RGBA32, false);
        }

        // Albedo: 8x8 checker (orange / off-white) so UV + tiling are obvious.
        static Texture2D GenAlbedo()
        {
            var t = NewTex();
            for (int y = 0; y < TexSize; y++)
            for (int x = 0; x < TexSize; x++)
            {
                bool a = ((x / 32) + (y / 32)) % 2 == 0;
                t.SetPixel(x, y, a ? new Color(0.85f, 0.55f, 0.25f) : new Color(0.92f, 0.92f, 0.90f));
            }
            t.Apply();
            return t;
        }

        // Tangent-space normal map: a grid of sinusoidal bumps.
        static Texture2D GenNormal()
        {
            var t = NewTex();
            for (int y = 0; y < TexSize; y++)
            for (int x = 0; x < TexSize; x++)
            {
                float u = (float)x / TexSize, v = (float)y / TexSize;
                float nx = Mathf.Sin(u * Mathf.PI * 2f * 6f) * 0.5f;
                float ny = Mathf.Sin(v * Mathf.PI * 2f * 6f) * 0.5f;
                float nz = Mathf.Sqrt(Mathf.Max(0.0001f, 1f - nx * nx - ny * ny));
                t.SetPixel(x, y, new Color(nx * 0.5f + 0.5f, ny * 0.5f + 0.5f, nz * 0.5f + 0.5f, 1f));
            }
            t.Apply();
            return t;
        }

        // URP mask: R = metallic (left->right), A = smoothness (bottom->top).
        static Texture2D GenMetallicGloss()
        {
            var t = NewTex();
            for (int y = 0; y < TexSize; y++)
            for (int x = 0; x < TexSize; x++)
            {
                float metallic   = (float)x / (TexSize - 1);
                float smoothness = (float)y / (TexSize - 1);
                t.SetPixel(x, y, new Color(metallic, 0f, 0f, smoothness));
            }
            t.Apply();
            return t;
        }

        // Occlusion: vertical bands (grayscale) so AO darkening is visible.
        static Texture2D GenOcclusion()
        {
            var t = NewTex();
            for (int y = 0; y < TexSize; y++)
            for (int x = 0; x < TexSize; x++)
            {
                float ao = ((x / 16) % 2 == 0) ? 1.0f : 0.45f;
                t.SetPixel(x, y, new Color(ao, ao, ao, 1f));
            }
            t.Apply();
            return t;
        }

        // Emission: black with a bright cyan cross.
        static Texture2D GenEmission()
        {
            var t = NewTex();
            for (int y = 0; y < TexSize; y++)
            for (int x = 0; x < TexSize; x++)
            {
                float u = (float)x / TexSize, v = (float)y / TexSize;
                bool cross = Mathf.Abs(u - 0.5f) < 0.06f || Mathf.Abs(v - 0.5f) < 0.06f;
                t.SetPixel(x, y, cross ? new Color(0f, 1f, 1f) : Color.black);
            }
            t.Apply();
            return t;
        }
    }
}
