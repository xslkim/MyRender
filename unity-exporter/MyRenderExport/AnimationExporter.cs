using System.IO;
using UnityEditor;
using UnityEngine;

namespace MyRenderExport
{
    /// Bakes a SkinnedMeshRenderer + AnimationClip into a MyRender .anim file
    /// (see docs/MyRender_AssetFormat.md). For each frame we sample the clip and
    /// write, per bone, the skinning matrix S[b] = bone.localToWorld · bindpose[b]
    /// which maps a vertex from mesh-local straight to world space. The runtime
    /// then does linear blend skinning; no curve evaluation happens in C++.
    public static class AnimationExporter
    {
        const ushort Version = 1;

        /// Returns the relative path written under outRoot, or "" on failure.
        public static string Export(SkinnedMeshRenderer smr, AnimationClip clip,
                                    string outRoot, int fps = 30)
        {
            if (smr == null || clip == null) return "";
            Transform[] bones = smr.bones;
            Matrix4x4[] bind  = smr.sharedMesh != null ? smr.sharedMesh.bindposes : null;
            if (bones == null || bind == null || bones.Length == 0 || bind.Length != bones.Length)
            {
                Debug.LogWarning($"[MyRender] skin/bindpose mismatch on {smr.name}; skipping anim.");
                return "";
            }

            int boneCount  = bones.Length;
            int frameCount = Mathf.Max(2, Mathf.CeilToInt(clip.length * fps) + 1);

            GameObject root = smr.transform.root.gameObject;

            string name = Sanitize(clip.name);
            if (string.IsNullOrEmpty(name)) name = "clip";
            string rel = "anims/" + name + ".anim";
            string abs = Path.Combine(outRoot, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(abs));

            using var bw = new BinaryWriter(File.Open(abs, FileMode.Create));
            bw.Write(new[] { 'M', 'R', 'A', 'N' });
            bw.Write(Version);
            bw.Write((ushort)0);          // flags
            bw.Write((float)fps);
            bw.Write((uint)frameCount);
            bw.Write((uint)boneCount);

            AnimationMode.StartAnimationMode();
            try
            {
                for (int f = 0; f < frameCount; f++)
                {
                    float t = (frameCount > 1) ? (f / (float)fps) : 0f;
                    t = Mathf.Min(t, clip.length);

                    AnimationMode.BeginSampling();
                    AnimationMode.SampleAnimationClip(root, clip, t);
                    AnimationMode.EndSampling();

                    for (int b = 0; b < boneCount; b++)
                    {
                        // S = bone.localToWorld · bindpose  (mesh-local -> world)
                        Matrix4x4 s = bones[b].localToWorldMatrix * bind[b];
                        for (int r = 0; r < 4; r++)
                            for (int c = 0; c < 4; c++)
                                bw.Write(s[r, c]);
                    }
                }
            }
            finally
            {
                AnimationMode.StopAnimationMode();
            }

            Debug.Log($"[MyRender] Baked anim '{clip.name}' -> {rel} " +
                      $"({frameCount} frames, {boneCount} bones, {fps} fps)");
            return rel;
        }

        static string Sanitize(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            foreach (char c in Path.GetInvalidFileNameChars()) s = s.Replace(c, '_');
            return s.Replace(' ', '_');
        }
    }
}
