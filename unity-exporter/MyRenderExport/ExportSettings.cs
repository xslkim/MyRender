using System.IO;
using UnityEditor;
using UnityEngine;

namespace MyRenderExport
{
    /// Manages the configurable export root folder, stored in EditorPrefs so it
    /// persists per machine without being hardcoded or committed to source control.
    ///
    /// Version-specific API differences between URP 7.x (2019) and URP 14 (2022)
    /// are isolated with #if UNITY_2021_2_OR_NEWER blocks. Add them here or in
    /// the relevant exporter file when a concrete API divergence arises (e.g.,
    /// post-process Volume API in M7).
    public static class ExportSettings
    {
        const string PrefKey = "MyRender.ExportRoot";

        /// Returns the saved export root, prompting the user to pick one if unset.
        /// Returns "" if the user cancels — callers must handle that and abort.
        public static string GetExportRoot()
        {
            string saved = EditorPrefs.GetString(PrefKey, "");
            if (!string.IsNullOrEmpty(saved) && Directory.Exists(saved))
                return saved;
            return PromptPickFolderInternal();
        }

        [MenuItem("MyRender/Settings - Change Export Folder")]
        public static void ChangeExportFolderMenu()
        {
            string folder = PromptPickFolderInternal();
            if (!string.IsNullOrEmpty(folder))
                Debug.Log($"[MyRender] Export root set to: {folder}");
            else
                Debug.LogWarning("[MyRender] Export folder not changed (cancelled).");
        }

        static string PromptPickFolderInternal()
        {
            string current = EditorPrefs.GetString(PrefKey, "");
            string folder = EditorUtility.OpenFolderPanel(
                "Select MyRender export root (scene folders will be created inside)",
                current, "");
            if (string.IsNullOrEmpty(folder)) return "";
            EditorPrefs.SetString(PrefKey, folder);
            return folder;
        }
    }
}
