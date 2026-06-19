using System.IO;
using UnityEditor;
using UnityEngine;

namespace MyRenderExport
{
    /// Exports Unity textures to TGA for MyRender.
    ///
    /// Two paths:
    ///  - Real source assets that are compressed or non-readable (the URP sample's
    ///    BC5/DXT5nm normal & mask maps) -> temporarily reimport the SOURCE asset
    ///    uncompressed + readable and read raw pixels. Normals are read as a plain
    ///    Default texture so we get the authored RGB normal directly (no swizzle).
    ///  - Already uncompressed + readable textures (e.g. the validation scene's
    ///    procedural maps) -> the simple Blit + ReadPixels path, with the Unity
    ///    normal swizzle reconstructed (X in alpha, Y in green).
    public static class TextureExporter
    {
        public static string Export(Texture texture, string outRoot, bool linear, bool isNormal = false)
        {
            if (texture == null) return "";
            var tex2d = texture as Texture2D;
            if (tex2d == null) return ""; // skip cubemaps / render textures for v1

            string name = Sanitize(texture.name);
            if (string.IsNullOrEmpty(name)) name = "tex_" + texture.GetInstanceID();
            string rel = "textures/" + name + ".tga";
            string abs = Path.Combine(outRoot, rel);
            if (File.Exists(abs)) return rel; // dedup within this export run
            Directory.CreateDirectory(Path.GetDirectoryName(abs));

            string assetPath = AssetDatabase.GetAssetPath(tex2d);
            var importer = string.IsNullOrEmpty(assetPath)
                ? null : AssetImporter.GetAtPath(assetPath) as TextureImporter;

            bool needsReimport = importer != null &&
                (importer.textureCompression != TextureImporterCompression.Uncompressed ||
                 !importer.isReadable);

            Texture2D pixels;
            bool destroy;
            if (needsReimport)
            {
                pixels  = ReadSourceUncompressed(importer, assetPath, isNormal);
                destroy = true; // a fresh copy we own
            }
            else
            {
                pixels  = BlitReadable(tex2d, linear);
                if (isNormal) UnswizzleNormal(pixels);
                destroy = true;
            }

            File.WriteAllBytes(abs, ImageConversion.EncodeToTGA(pixels));
            Object.DestroyImmediate(pixels);
            return rel;
        }

        // Temporarily reimport the source asset uncompressed + readable, copy its
        // pixels, then restore the original import settings. Normals are read as a
        // Default (non-normal) texture so GetPixels returns the authored RGB normal.
        static Texture2D ReadSourceUncompressed(TextureImporter imp, string path, bool isNormal)
        {
            var oType = imp.textureType;
            var oComp = imp.textureCompression;
            var oRead = imp.isReadable;
            var oSRGB = imp.sRGBTexture;

            imp.textureCompression = TextureImporterCompression.Uncompressed;
            imp.isReadable = true;
            if (isNormal) { imp.textureType = TextureImporterType.Default; imp.sRGBTexture = false; }
            imp.SaveAndReimport();

            var src = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            var copy = new Texture2D(src.width, src.height, TextureFormat.RGBA32, false, linear: isNormal || !oSRGB);
            copy.SetPixels(src.GetPixels());
            copy.Apply();

            imp.textureType        = oType;
            imp.textureCompression = oComp;
            imp.isReadable         = oRead;
            imp.sRGBTexture        = oSRGB;
            imp.SaveAndReimport();

            return copy;
        }

        // Blit through a temp RenderTexture so non-readable textures can be read.
        static Texture2D BlitReadable(Texture2D tex2d, bool linear)
        {
            var rw = linear ? RenderTextureReadWrite.Linear : RenderTextureReadWrite.sRGB;
            var rt = RenderTexture.GetTemporary(tex2d.width, tex2d.height, 0, RenderTextureFormat.ARGB32, rw);
            var prev = RenderTexture.active;
            Graphics.Blit(tex2d, rt);
            RenderTexture.active = rt;

            var readable = new Texture2D(tex2d.width, tex2d.height, TextureFormat.RGBA32, false, linear);
            readable.ReadPixels(new Rect(0, 0, tex2d.width, tex2d.height), 0, 0);
            readable.Apply();

            RenderTexture.active = prev;
            RenderTexture.ReleaseTemporary(rt);
            return readable;
        }

        static void UnswizzleNormal(Texture2D tex)
        {
            var px = tex.GetPixels();
            for (int i = 0; i < px.Length; i++)
            {
                float x = px[i].a * 2f - 1f;
                float y = px[i].g * 2f - 1f;
                float z = Mathf.Sqrt(Mathf.Max(0f, 1f - x * x - y * y));
                px[i] = new Color(x * 0.5f + 0.5f, y * 0.5f + 0.5f, z * 0.5f + 0.5f, 1f);
            }
            tex.SetPixels(px);
            tex.Apply();
        }

        static string Sanitize(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            foreach (char c in Path.GetInvalidFileNameChars()) s = s.Replace(c, '_');
            return s.Replace(' ', '_');
        }
    }
}
