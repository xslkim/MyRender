// MyRenderVideoCapture — 一键导出"从 Unity 导出到 CPU 软渲染"教学视频用的真实 URP 渲染素材。
//
// 用法：在 Unity 里打开要拍的场景（工地 SampleScene），菜单
//   MyRender ▸ Capture Video Assets
// 产物写到 <项目根>/MyRenderVideoAssets/：
//   unity_scene_game.png   当前主相机机位的静帧（视频里的"标准答案"/Hero 静图）
//   unity_fog_off.png      临时关掉雾再渲一张（和 EP5 的雾对比呼应）
//   orbit/orbit_####.png   绕场景一圈的序列帧（拼成环绕 Hero 视频）
//
// 渲染走的是工程当前的 URP 管线，所见即所得。跑完把路径告诉对方即可（PNG 序列可再转 mp4）。
// 兼容 Unity 2019.4 与 2022.3。

using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public static class MyRenderVideoCapture
{
    const int   kWidth       = 1920;   // 视频分辨率 16:9
    const int   kHeight      = 1080;
    const int   kOrbitFrames = 120;    // 环绕序列帧数（120 帧 @ 30fps = 4s）
    const float kOrbitElevDeg = 15f;   // 环绕相机俯仰（度）
    const int   kMSAA        = 4;      // RT 抗锯齿

    // 路径写死到仓库的视频素材目录：点一下菜单，素材就直接落位，无需手动拷贝。
    const string kOutDir = @"G:/MyRender/docs/video/unity-urp-cpu/assets";

    static string OutDir
    {
        get { Directory.CreateDirectory(kOutDir); return kOutDir; }
    }

    [MenuItem("MyRender/Capture Video Assets")]
    static void CaptureAll()
    {
        Camera cam = Camera.main;
        if (cam == null) cam = Object.FindObjectOfType<Camera>();
        if (cam == null) { Debug.LogError("[MyRenderVideoCapture] 场景里没有相机。"); return; }

        string outDir = OutDir;

        // 保存现场，结束后还原
        Vector3    savedPos = cam.transform.position;
        Quaternion savedRot = cam.transform.rotation;
        RenderTexture savedRT = cam.targetTexture;
        bool       savedFog = RenderSettings.fog;

        RenderTexture rt = new RenderTexture(kWidth, kHeight, 24)
        {
            antiAliasing = kMSAA,
            name = "__MyRenderCaptureRT"
        };
        cam.targetTexture = rt;

        try
        {
            // 1) 当前机位静帧（标准答案）
            RenderToPng(cam, rt, Path.Combine(outDir, "unity_scene_game.png"));
            Debug.Log("[MyRenderVideoCapture] saved unity_scene_game.png");

            // 2) 关掉雾再来一张（呼应 EP5 的雾对比）
            RenderSettings.fog = false;
            RenderToPng(cam, rt, Path.Combine(outDir, "unity_fog_off.png"));
            RenderSettings.fog = savedFog;
            Debug.Log("[MyRenderVideoCapture] saved unity_fog_off.png");

            // 3) 环绕序列
            Bounds b = ComputeSceneBounds();
            float radius = Mathf.Max(b.extents.magnitude, 0.01f);
            float dist   = radius / Mathf.Sin(cam.fieldOfView * 0.5f * Mathf.Deg2Rad) * 1.15f;
            float elev   = kOrbitElevDeg * Mathf.Deg2Rad;

            string orbitDir = Path.Combine(outDir, "_orbit_frames"); // 临时序列，转成 orbit.mp4 后清理
            Directory.CreateDirectory(orbitDir);

            for (int i = 0; i < kOrbitFrames; i++)
            {
                if (EditorUtility.DisplayCancelableProgressBar(
                        "Capturing orbit", $"frame {i + 1}/{kOrbitFrames}",
                        (float)i / kOrbitFrames))
                    break;

                float a = 2f * Mathf.PI * i / kOrbitFrames;
                Vector3 dir = new Vector3(
                    Mathf.Cos(a) * Mathf.Cos(elev),
                    Mathf.Sin(elev),
                    Mathf.Sin(a) * Mathf.Cos(elev));
                cam.transform.position = b.center + dir * dist;
                cam.transform.LookAt(b.center);

                RenderToPng(cam, rt, Path.Combine(orbitDir, $"orbit_{i:0000}.png"));
            }
        }
        finally
        {
            EditorUtility.ClearProgressBar();
            cam.targetTexture        = savedRT;
            cam.transform.position   = savedPos;
            cam.transform.rotation   = savedRot;
            RenderSettings.fog       = savedFog;
            RenderTexture.active     = null;
            Object.DestroyImmediate(rt);
        }

        Debug.Log($"[MyRenderVideoCapture] 完成 → {outDir}\n" +
                  "把 unity_scene_game.png / unity_fog_off.png 和 orbit/ 序列交给对方即可。");
        EditorUtility.RevealInFinder(outDir);
    }

    // 渲染当前相机到 RT 并存成 PNG
    static void RenderToPng(Camera cam, RenderTexture rt, string path)
    {
        cam.Render();
        RenderTexture prev = RenderTexture.active;
        RenderTexture.active = rt;
        Texture2D tex = new Texture2D(rt.width, rt.height, TextureFormat.RGB24, false);
        tex.ReadPixels(new Rect(0, 0, rt.width, rt.height), 0, 0);
        tex.Apply();
        File.WriteAllBytes(path, tex.EncodeToPNG());
        RenderTexture.active = prev;
        Object.DestroyImmediate(tex);
    }

    // 场景世界包围盒：合并所有可见 Renderer
    static Bounds ComputeSceneBounds()
    {
        var renderers = Object.FindObjectsOfType<Renderer>();
        bool any = false;
        Bounds b = new Bounds(Vector3.zero, Vector3.one);
        foreach (var r in renderers)
        {
            if (!r.enabled || !r.gameObject.activeInHierarchy) continue;
            if (r is ParticleSystemRenderer) continue;
            if (!any) { b = r.bounds; any = true; }
            else b.Encapsulate(r.bounds);
        }
        if (!any) b = new Bounds(Vector3.zero, Vector3.one * 5f);
        return b;
    }
}
