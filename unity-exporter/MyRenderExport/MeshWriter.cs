using System.IO;
using UnityEngine;

namespace MyRenderExport
{
    /// Writes a Unity Mesh to the MyRender .mesh binary format (see docs/MyRender_AssetFormat.md).
    /// v1: static path only (no skin). Values are written verbatim in Unity space.
    public static class MeshWriter
    {
        const ushort Version    = 1;
        const ushort FlagHasSkin = 1 << 0;
        const ushort FlagHasUV1  = 1 << 1;
        const ushort FlagHasColor = 1 << 2;

        public static void Write(Mesh mesh, string path)
        {
            Vector3[] positions = mesh.vertices;
            Vector3[] normals   = mesh.normals;
            Vector4[] tangents  = mesh.tangents;
            Vector2[] uv0       = mesh.uv;
            Vector2[] uv1       = mesh.uv2;
            Color[]   colors    = mesh.colors;

            int vc = positions.Length;
            bool hasUV1   = uv1 != null && uv1.Length == vc;
            bool hasColor = colors != null && colors.Length == vc;

            // Skin: legacy 4-influence BoneWeight[] + bindposes. We only support the
            // 4-bone path (matches the runtime's boneIndex[4]/boneWeight[4]).
            BoneWeight[] boneWeights = mesh.boneWeights;
            Matrix4x4[]  bindposes   = mesh.bindposes;
            bool hasSkin = bindposes != null && bindposes.Length > 0 &&
                           boneWeights != null && boneWeights.Length == vc;
            int boneCount = hasSkin ? bindposes.Length : 0;

            // Fall back to sane defaults when a stream is missing.
            if (normals  == null || normals.Length  != vc) normals  = new Vector3[vc];
            if (tangents == null || tangents.Length != vc) tangents = Filled(vc, new Vector4(1, 0, 0, 1));
            if (uv0      == null || uv0.Length      != vc) uv0      = new Vector2[vc];

            // Combined index buffer + submesh ranges.
            int sub = mesh.subMeshCount;
            var ranges = new (int start, int count)[sub];
            int total = 0;
            var allIndices = new System.Collections.Generic.List<int>(mesh.triangles.Length);
            for (int s = 0; s < sub; s++)
            {
                int[] tris = mesh.GetTriangles(s);
                ranges[s] = (total, tris.Length);
                allIndices.AddRange(tris);
                total += tris.Length;
            }

            ushort flags = 0;
            if (hasSkin)  flags |= FlagHasSkin;
            if (hasUV1)   flags |= FlagHasUV1;
            if (hasColor) flags |= FlagHasColor;

            Directory.CreateDirectory(Path.GetDirectoryName(path));
            using (var bw = new BinaryWriter(File.Open(path, FileMode.Create)))
            {
                bw.Write(new[] { 'M', 'R', 'S', 'H' });
                bw.Write(Version);
                bw.Write(flags);
                bw.Write((uint)vc);
                bw.Write((uint)allIndices.Count);
                bw.Write((uint)sub);
                bw.Write((uint)boneCount);

                foreach (var r in ranges) { bw.Write((uint)r.start); bw.Write((uint)r.count); }

                for (int i = 0; i < vc; i++)
                {
                    Write3(bw, positions[i]);
                    Write3(bw, normals[i]);
                    Write4(bw, tangents[i]);
                    Write2(bw, uv0[i]);
                    if (hasUV1)   Write2(bw, uv1[i]);
                    if (hasColor) WriteColor(bw, colors[i]);
                    if (hasSkin)
                    {
                        BoneWeight w = boneWeights[i];
                        bw.Write((ushort)w.boneIndex0); bw.Write((ushort)w.boneIndex1);
                        bw.Write((ushort)w.boneIndex2); bw.Write((ushort)w.boneIndex3);
                        bw.Write(w.weight0); bw.Write(w.weight1);
                        bw.Write(w.weight2); bw.Write(w.weight3);
                    }
                }

                foreach (int idx in allIndices) bw.Write((uint)idx);

                // Bindposes (row-major), one 4x4 per bone. The runtime bakes these into
                // the .anim skinning matrices and skips them, but the format carries them.
                if (hasSkin)
                    foreach (var m in bindposes)
                        for (int r = 0; r < 4; r++)
                            for (int c = 0; c < 4; c++)
                                bw.Write(m[r, c]);
            }
        }

        static Vector4[] Filled(int n, Vector4 v) { var a = new Vector4[n]; for (int i = 0; i < n; i++) a[i] = v; return a; }
        static void Write2(BinaryWriter bw, Vector2 v) { bw.Write(v.x); bw.Write(v.y); }
        static void Write3(BinaryWriter bw, Vector3 v) { bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); }
        static void Write4(BinaryWriter bw, Vector4 v) { bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); bw.Write(v.w); }
        static void WriteColor(BinaryWriter bw, Color c) { bw.Write(c.r); bw.Write(c.g); bw.Write(c.b); bw.Write(c.a); }
    }
}
