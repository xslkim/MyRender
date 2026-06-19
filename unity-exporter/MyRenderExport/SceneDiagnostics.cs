using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace MyRenderExport
{
    /// Menu item "MyRender/Diagnose Scene" — writes a detailed diagnostic report
    /// to <ExportRoot>/diagnostic_report.txt. Copy this file into your Unity
    /// project's Editor folder (alongside the other MyRenderExport/*.cs files),
    /// then run it from the Unity menu bar.
    public static class SceneDiagnostics
    {
        const int TargetW = 960;
        const int TargetH = 540;

        [MenuItem("MyRender/Diagnose Scene")]
        public static void Run()
        {
            string path = "G:/MyRender/out/unity_diag.txt";
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            using (var w = new StreamWriter(path))
            {
                w.WriteLine("=== Scene Diagnostics ===");
                w.WriteLine($"Scene: {SceneManager.GetActiveScene().name}");
                w.WriteLine($"Unity: {Application.unityVersion}");
                w.WriteLine($"Time:  {System.DateTime.Now}");
                w.WriteLine();

                // --- Camera ---
                Camera cam = Camera.main;
                if (cam == null) cam = Object.FindObjectOfType<Camera>();
                if (cam != null)
                    DumpCamera(w, cam);
                else
                    w.WriteLine("CAMERA: NONE FOUND");

                // --- Objects ---
                DumpAllRenderers(w);

                // --- Project a known vertex through Unity's matrices ---
                if (cam != null)
                    ProjectTestVertex(w, cam);

                // --- Mesh sampling ---
                SampleFirstMesh(w, cam);
            }
            Debug.Log($"[Diag] Report written to {path}");
        }

        static void DumpCamera(StreamWriter w, Camera cam)
        {
            Transform t = cam.transform;
            float prevAspect = cam.aspect;
            cam.aspect = (float)TargetW / TargetH;
            Matrix4x4 proj960 = cam.projectionMatrix;
            cam.ResetAspect();
            cam.aspect = prevAspect;

            w.WriteLine("--- Camera ---");
            w.WriteLine($"  name:          {cam.name}");
            w.WriteLine($"  position:      {F3(t.position)}");
            w.WriteLine($"  rotation euler:{F3(t.rotation.eulerAngles)}");
            w.WriteLine($"  forward:       {F3(t.forward)}");
            w.WriteLine($"  up:            {F3(t.up)}");
            w.WriteLine($"  FOV:           {cam.fieldOfView}");
            w.WriteLine($"  near/far:      {cam.nearClipPlane} / {cam.farClipPlane}");
            w.WriteLine($"  aspect (orig): {cam.aspect}");
            w.WriteLine($"  aspect (set):  {(float)TargetW / TargetH}");
            w.WriteLine($"  orthographic:  {cam.orthographic}");

            Matrix4x4 V = cam.worldToCameraMatrix;
            w.WriteLine($"  worldToCameraMatrix:");
            w.WriteLine($"    row0: {F4(V.GetRow(0))}");
            w.WriteLine($"    row1: {F4(V.GetRow(1))}");
            w.WriteLine($"    row2: {F4(V.GetRow(2))}");
            w.WriteLine($"    row3: {F4(V.GetRow(3))}");

            w.WriteLine($"  projectionMatrix (@{TargetW}x{TargetH}):");
            w.WriteLine($"    row0: {F4(proj960.GetRow(0))}");
            w.WriteLine($"    row1: {F4(proj960.GetRow(1))}");
            w.WriteLine($"    row2: {F4(proj960.GetRow(2))}");
            w.WriteLine($"    row3: {F4(proj960.GetRow(3))}");

            Matrix4x4 VP = proj960 * V;
            w.WriteLine($"  VP = proj * view:");
            w.WriteLine($"    row0: {F4(VP.GetRow(0))}");
            w.WriteLine($"    row1: {F4(VP.GetRow(1))}");
            w.WriteLine($"    row2: {F4(VP.GetRow(2))}");
            w.WriteLine($"    row3: {F4(VP.GetRow(3))}");
            w.WriteLine();
        }

        static void DumpAllRenderers(StreamWriter w)
        {
            w.WriteLine("--- All Renderers ---");
            int total = 0, meshRend = 0, skinned = 0, skipped = 0;
            var roots = SceneManager.GetActiveScene().GetRootGameObjects();
            w.WriteLine($"  Root GameObjects: {roots.Length}");
            foreach (var go in roots)
                w.WriteLine($"    root: '{go.name}' active={go.activeSelf} children={go.transform.childCount}");

            foreach (var go in roots)
            foreach (var r in go.GetComponentsInChildren<Renderer>(true))
            {
                total++;
                bool active = r.gameObject.activeInHierarchy;
                string type = r.GetType().Name;
                Mesh mesh = r is SkinnedMeshRenderer smr ? smr.sharedMesh
                          : r.GetComponent<MeshFilter>()?.sharedMesh;

                if (!(r is MeshRenderer) && !(r is SkinnedMeshRenderer))
                {
                    skipped++;
                    w.WriteLine($"  [{total}] SKIP type={type} name='{r.gameObject.name}' active={active}");
                    continue;
                }

                if (r is MeshRenderer) meshRend++; else skinned++;

                var t = r.transform;
                var M = t.localToWorldMatrix;
                var bounds = mesh != null ? mesh.bounds : new Bounds();

                // Transform the 8 AABB corners to world space
                Vector3 lo = bounds.min, hi = bounds.max;
                Vector3 wMin = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);
                Vector3 wMax = new Vector3(float.MinValue, float.MinValue, float.MinValue);
                for (int k = 0; k < 8; k++)
                {
                    Vector3 c = new Vector3(
                        (k & 1) != 0 ? hi.x : lo.x,
                        (k & 2) != 0 ? hi.y : lo.y,
                        (k & 4) != 0 ? hi.z : lo.z);
                    Vector3 wc = M.MultiplyPoint(c);
                    wMin = Vector3.Min(wMin, wc);
                    wMax = Vector3.Max(wMax, wc);
                }

                int matCount = r.sharedMaterials.Length;
                int vertCount = mesh != null ? mesh.vertexCount : 0;
                int triCount = mesh != null ? mesh.triangles.Length / 3 : 0;
                int subCount = mesh != null ? mesh.subMeshCount : 0;

                w.WriteLine($"  [{total}] {type} name='{r.gameObject.name}' active={active}");
                w.WriteLine($"         verts={vertCount} tris={triCount} submeshes={subCount} materials={matCount}");
                w.WriteLine($"         pos={F3(t.position)} scale={F3(t.lossyScale)}");
                w.WriteLine($"         obj AABB: lo={F3(lo)} hi={F3(hi)}");
                w.WriteLine($"         world AABB: lo={F3(wMin)} hi={F3(wMax)}");

                // Project world AABB corners to NDC
                Camera cam = Camera.main;
                if (cam == null) cam = Object.FindObjectOfType<Camera>();
                if (cam != null)
                {
                    float prevAspect = cam.aspect;
                    cam.aspect = (float)TargetW / TargetH;
                    Matrix4x4 VP = cam.projectionMatrix * cam.worldToCameraMatrix;
                    cam.ResetAspect();
                    cam.aspect = prevAspect;

                    float ndcXmin = float.MaxValue, ndcXmax = float.MinValue;
                    float ndcYmin = float.MaxValue, ndcYmax = float.MinValue;
                    float ndcZmin = float.MaxValue, ndcZmax = float.MinValue;
                    float wMinW = float.MaxValue, wMaxW = float.MinValue;
                    for (int k = 0; k < 8; k++)
                    {
                        Vector3 c = new Vector3(
                            (k & 1) != 0 ? hi.x : lo.x,
                            (k & 2) != 0 ? hi.y : lo.y,
                            (k & 4) != 0 ? hi.z : lo.z);
                        Vector3 wc = M.MultiplyPoint(c);
                        Vector4 cs = VP * new Vector4(wc.x, wc.y, wc.z, 1);
                        float iw = 1.0f / cs.w;
                        ndcXmin = Mathf.Min(ndcXmin, cs.x * iw);
                        ndcXmax = Mathf.Max(ndcXmax, cs.x * iw);
                        ndcYmin = Mathf.Min(ndcYmin, cs.y * iw);
                        ndcYmax = Mathf.Max(ndcYmax, cs.y * iw);
                        ndcZmin = Mathf.Min(ndcZmin, cs.z * iw);
                        ndcZmax = Mathf.Max(ndcZmax, cs.z * iw);
                        wMinW = Mathf.Min(wMinW, cs.w);
                        wMaxW = Mathf.Max(wMaxW, cs.w);
                    }
                    w.WriteLine($"         NDC X:[{ndcXmin:F3},{ndcXmax:F3}] Y:[{ndcYmin:F3},{ndcYmax:F3}] Z:[{ndcZmin:F3},{ndcZmax:F3}]");
                    w.WriteLine($"         W:[{wMinW:F3},{wMaxW:F3}]");
                    // Convert to MyRender screen coordinates (using same formula)
                    float scrXmin = (ndcXmin + 1) * 0.5f * (TargetW - 1);
                    float scrXmax = (ndcXmax + 1) * 0.5f * (TargetW - 1);
                    float scrYmin = (ndcYmin + 1) * 0.5f * (TargetH - 1);
                    float scrYmax = (ndcYmax + 1) * 0.5f * (TargetH - 1);
                    w.WriteLine($"         MyRender screen Y: [{scrYmin:F0},{scrYmax:F0}] (before Y-flip)");
                    w.WriteLine($"         MyRender output rows: [{TargetH-1-scrYmax:F0},{TargetH-1-scrYmin:F0}] (after Y-flip)");
                }
                w.WriteLine();
            }

            w.WriteLine($"  Summary: total={total}, MeshRenderer={meshRend}, Skinned={skinned}, skipped={skipped}");
            w.WriteLine();
        }

        static void ProjectTestVertex(StreamWriter w, Camera cam)
        {
            w.WriteLine("--- Vertex Projection Test ---");
            // Project several points to verify the matrix chain
            float prevAspect = cam.aspect;
            cam.aspect = (float)TargetW / TargetH;
            Matrix4x4 VP = cam.projectionMatrix * cam.worldToCameraMatrix;
            cam.ResetAspect();
            cam.aspect = prevAspect;

            // Test the origin and a few known points
            Vector4[] testPts = {
                new Vector4(0, 0, 0, 1),
                new Vector4(-1, 0, 2.5f, 1),
                new Vector4(1, 0, 2.5f, 1),
                new Vector4(0, 0, 5, 1),
                new Vector4(0, -0.15f, 0, 1),
                new Vector4(3.17f, 0, 1.34f, 1),
                new Vector4(-1.83f, -0.15f, -3.66f, 1),
            };

            foreach (var pt in testPts)
            {
                Vector4 cs = VP * pt;
                float iw = 1.0f / cs.w;
                float ndcX = cs.x * iw, ndcY = cs.y * iw, ndcZ = cs.z * iw;
                float scrY = (ndcY + 1) * 0.5f * (TargetH - 1);
                float outRow = TargetH - 1 - scrY;
                w.WriteLine($"  world{pt} -> NDC=({ndcX:F3},{ndcY:F3},{ndcZ:F3}) W={cs.w:F3} scrY={scrY:F0} outRow={outRow:F0}");
            }
            w.WriteLine();
        }

        static void SampleFirstMesh(StreamWriter w, Camera cam)
        {
            w.WriteLine("--- First Mesh Vertex Sample ---");
            var roots = SceneManager.GetActiveScene().GetRootGameObjects();
            foreach (var go in roots)
            foreach (var r in go.GetComponentsInChildren<MeshRenderer>(false))
            {
                Mesh mesh = r.GetComponent<MeshFilter>()?.sharedMesh;
                if (mesh == null) continue;

                Vector3[] verts = mesh.vertices;
                w.WriteLine($"  Mesh: '{mesh.name}' verts={verts.Length} tris={mesh.triangles.Length/3} sub={mesh.subMeshCount}");
                w.WriteLine($"  Object: '{r.gameObject.name}' pos={F3(r.transform.position)}");

                // Show first 5 vertex positions
                w.WriteLine("  First 5 object-space vertices:");
                for (int i = 0; i < Mathf.Min(5, verts.Length); i++)
                    w.WriteLine($"    v[{i}]: {F3(verts[i])}");

                // Compute object-space bounds
                Vector3 lo = verts[0], hi = verts[0];
                foreach (var v in verts) { lo = Vector3.Min(lo, v); hi = Vector3.Max(hi, v); }
                w.WriteLine($"  Object-space bounds: lo={F3(lo)} hi={F3(hi)}");

                // Project the bounds to NDC using the same pipeline as MyRender
                Matrix4x4 M = r.transform.localToWorldMatrix;
                float prevAspect = cam.aspect;
                cam.aspect = (float)TargetW / TargetH;
                Matrix4x4 VP = cam.projectionMatrix * cam.worldToCameraMatrix;
                cam.ResetAspect();
                cam.aspect = prevAspect;

                float ndcYmin = float.MaxValue, ndcYmax = float.MinValue;
                float ndcXmin = float.MaxValue, ndcXmax = float.MinValue;
                foreach (var v in verts)
                {
                    Vector3 wv = M.MultiplyPoint(v);
                    Vector4 cs = VP * new Vector4(wv.x, wv.y, wv.z, 1);
                    float iw = 1.0f / cs.w;
                    ndcXmin = Mathf.Min(ndcXmin, cs.x * iw);
                    ndcXmax = Mathf.Max(ndcXmax, cs.x * iw);
                    ndcYmin = Mathf.Min(ndcYmin, cs.y * iw);
                    ndcYmax = Mathf.Max(ndcYmax, cs.y * iw);
                }

                float scrYlo = (ndcYmin + 1) * 0.5f * (TargetH - 1);
                float scrYhi = (ndcYmax + 1) * 0.5f * (TargetH - 1);
                w.WriteLine($"  ALL-VERTEX NDC: X[{ndcXmin:F3},{ndcXmax:F3}] Y[{ndcYmin:F3},{ndcYmax:F3}]");
                w.WriteLine($"  ALL-VERTEX MyRender screen Y: [{scrYlo:F0},{scrYhi:F0}]");
                w.WriteLine($"  ALL-VERTEX MyRender output rows: [{TargetH-1-scrYhi:F0},{TargetH-1-scrYlo:F0}]");
                w.WriteLine();

                break; // only first mesh
            }
        }

        static string F3(Vector3 v) => $"({v.x:R}, {v.y:R}, {v.z:R})";
        static string F4(Vector4 v) => $"({v.x:R}, {v.y:R}, {v.z:R}, {v.w:R})";
    }
}
