using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace MyRenderExport
{
    /// Renders the active scene's main camera to a 960x540 PNG so it can be
    /// compared 1:1 against MyRender's output.
    ///
    /// Output: <ExportRoot>/<SceneName>/unity_ref.png — co-located with scene.json
    /// so MyRender can find it alongside the exported assets.
    public static class ReferenceCapture
    {
        const int Width  = 960;
        const int Height = 540;

        [MenuItem("MyRender/Capture Reference PNG")]
        public static void Capture()
        {
            string exportRoot = ExportSettings.GetExportRoot();
            if (string.IsNullOrEmpty(exportRoot))
            {
                Debug.LogError("[MyRender] Capture cancelled: no export folder selected.");
                return;
            }

            Camera cam = Camera.main;
            if (cam == null) cam = Object.FindObjectOfType<Camera>();
            if (cam == null) { Debug.LogError("[MyRender] No camera in scene."); return; }

            string sceneName = SceneManager.GetActiveScene().name;
            string outPath = Path.Combine(exportRoot, Sanitize(sceneName), "unity_ref.png");
            Directory.CreateDirectory(Path.GetDirectoryName(outPath));

            RenderTexture prevTarget = cam.targetTexture;
            RenderTexture prevActive = RenderTexture.active;

            // sRGB RT so the PNG matches what you see on screen (and what
            // MyRender writes after its linear->sRGB conversion step).
            var rt = new RenderTexture(Width, Height, 24, RenderTextureFormat.ARGB32,
                                       RenderTextureReadWrite.sRGB);
            rt.antiAliasing = 1;

            cam.targetTexture = rt;
            cam.aspect        = (float)Width / Height;
            cam.Render();

            RenderTexture.active = rt;
            var tex = new Texture2D(Width, Height, TextureFormat.RGBA32, false);
            tex.ReadPixels(new Rect(0, 0, Width, Height), 0, 0);
            tex.Apply();

            File.WriteAllBytes(outPath, tex.EncodeToPNG());

            cam.targetTexture    = prevTarget;
            cam.ResetAspect();
            RenderTexture.active = prevActive;
            Object.DestroyImmediate(tex);
            rt.Release();
            Object.DestroyImmediate(rt);

            Debug.Log($"[MyRender] Reference PNG ({Width}x{Height}) -> {outPath}\n" +
                      "Tip: re-run 'Export Active Scene' first so scene.json's projection " +
                      "matches this 960x540 aspect.");
        }

        static string Sanitize(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            foreach (char c in Path.GetInvalidFileNameChars()) s = s.Replace(c, '_');
            return s.Replace(' ', '_');
        }
    }
}
