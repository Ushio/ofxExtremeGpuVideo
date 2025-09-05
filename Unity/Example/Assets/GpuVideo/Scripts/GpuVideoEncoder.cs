using System;
using System.IO;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif

#if UNITY_EDITOR
namespace ExtremeGpuVideo.Encoder
{
    public partial class GpuVideoEncoder : EditorWindow
    {
        public class EncodeQueue
        {
            public enum ResultType
            {
                None = 0,
                Success = 1,
                Error = 2,
            }

            public string Path { private set; get; }
            public float FrameRate = 30f;
            public bool IsCompleted { private set; get; }
            public ResultType Result { private set; get; }
            public string ExportPath { private set; get; }

            public EncodeQueue(string path)
            {
                Path = path;
            }

            public void Complete(ResultType result)
            {
                this.IsCompleted = true;
                this.Result = result;
            }

            public void LogError(string message)
            {
                Debug.LogError($"[GpuVideoEncoder] {Path}\n{message}");
            }

            // :)
            public void SetExportPath(string path)
            {
                if (System.IO.Path.IsPathRooted(path) && path.EndsWith(".gv"))
                {
                    ExportPath = path;
                }
            }

            public string GetDefaultExportPath(string basePath)
            {
                var tempPath = System.IO.Path.Combine(Path, "video.gv");
                var directoryName = System.IO.Path.GetDirectoryName(tempPath);
                var fileName = System.IO.Path.GetFileName(directoryName);
                return System.IO.Path.Combine(basePath, $"{fileName}.gv");
            }

            public static EncodeQueue Create(string path)
            {
                return new EncodeQueue(path);
            }
        }

        private static string defaultExportDir { get => System.IO.Path.Combine(System.IO.Path.GetDirectoryName(Application.dataPath), "ExportedGpuVideo/"); }
        private static GpuVideoEncoder encorder = null;
        //private List<string> targetPaths = new List<string>();
        private List<EncodeQueue> encodeQueues = new List<EncodeQueue>();
        private Vector2 targetFoldersScrollPosition = Vector2.zero;
        private Vector2 messagesScrollPosition = Vector2.zero;

        [MenuItem("ExtremeGpuVideo/Encorder")]
        private static void ShowEncorderMenu()
        {
            ShowEncorder();
        }

        public static void ShowEncorder()
        {
            if (encorder != null)
            {
                Debug.Log("[GpuVideoEncorder] Already opened.");
                encorder.Show();
                return;
            }

            var window = EditorWindow.CreateWindow<GpuVideoEncoder>();
            window.Show();

            encorder = window;
            encorder.Init();
        }

        private void Init()
        {
            encodeQueues.Clear();
        }

        private void OnDroppedDirectories(IEnumerable<string> directories)
        {
            foreach (var directory in directories)
            {
                if (System.IO.Path.IsPathRooted(directory))
                {
                    encodeQueues.Add(EncodeQueue.Create(directory));
                }
            }
        }

        private void OnGUI()
        {
            bool containsMouse = false;

            // Select button
            if (GUILayout.Button("Select or Drop folder contains target files", GUILayout.ExpandWidth(true), GUILayout.Height(50f)))
            {
                var path = EditorUtility.OpenFolderPanel("Select folder contains target files.", null, "");
                if (System.IO.Directory.Exists(path))
                {
                    encodeQueues.Add(EncodeQueue.Create(path));
                }
            }

            containsMouse |= GUILayoutUtility.GetLastRect().Contains(Event.current.mousePosition);

            // List
            EditorGUILayout.LabelField("Target folders: ");
            using (var scrollView = new EditorGUILayout.ScrollViewScope(targetFoldersScrollPosition, GUI.skin.box, GUILayout.ExpandWidth(true), GUILayout.Height(250)))
            {
                targetFoldersScrollPosition = scrollView.scrollPosition;

                for (int i = 0; i < encodeQueues.Count; i++)
                {
                    var queue = encodeQueues[i];
                    using (var horizontalScope = new EditorGUILayout.HorizontalScope())
                    {
                        if (GUILayout.Button("x", GUILayout.Width(20), GUILayout.Height(20)))
                        {
                            encodeQueues.RemoveAt(i);
                            continue;
                        }
                        // Icon
                        GUILayout.Label(EditorGUIUtility.IconContent("d_Project"), GUILayout.Width(20), GUILayout.Height(20));
                        // DirName
                        EditorGUILayout.LabelField(System.IO.Path.GetFileName(queue.Path), GUILayout.Width(150));
                        // Fps
                        GUILayout.Label("FPS:", GUILayout.Width(30));
                        queue.FrameRate = EditorGUILayout.FloatField(queue.FrameRate, GUILayout.Width(40));


                        string exportLabel = string.IsNullOrEmpty(queue.ExportPath) ? "Select Export" : queue.ExportPath;
                        if (GUILayout.Button(exportLabel, GUILayout.ExpandWidth(true), GUILayout.MinWidth(200), GUILayout.Height(20)))
                        {
                            var exportPath = EditorUtility.SaveFilePanel("Select Export", null, "video.gv", "gv");
                            if (string.IsNullOrEmpty(exportPath) == false)
                            {
                                queue.SetExportPath(exportPath);
                            }
                        }
                    }
                }
            }

            containsMouse |= GUILayoutUtility.GetLastRect().Contains(Event.current.mousePosition);

            // dragdrop
            int id = GUIUtility.GetControlID(FocusType.Passive);

            switch (Event.current.type)
            {
                case EventType.DragUpdated:
                case EventType.DragPerform:
                    if (containsMouse == false)
                    {
                        return;
                    }

                    DragAndDrop.visualMode = DragAndDropVisualMode.Copy;
                    DragAndDrop.activeControlID = id;

                    if (Event.current.type == EventType.DragPerform)
                    {
                        DragAndDrop.AcceptDrag();
                        OnDroppedDirectories(DragAndDrop.paths);
                    }
                    Event.current.Use();
                    break;
            }

            if (GUILayout.Button("Start Encode!", GUILayout.ExpandWidth(true), GUILayout.Height(30)))
            {
                StartEncode();
            }

            // Show results
            EditorGUILayout.LabelField("Messages: ");
            using (var scrollView = new EditorGUILayout.ScrollViewScope(messagesScrollPosition, GUI.skin.box, GUILayout.ExpandWidth(true), GUILayout.Height(250)))
            {
                messagesScrollPosition = scrollView.scrollPosition;

                foreach (var queue in encodeQueues)
                {
                    if (queue.IsCompleted)
                    {
                        using (var horizontalScope = new EditorGUILayout.HorizontalScope())
                        {
                            string iconName = "";
                            string message = "";

                            switch (queue.Result)
                            {
                                case EncodeQueue.ResultType.None:
                                    iconName = "";
                                    message = "???";
                                    break;
                                case EncodeQueue.ResultType.Success:
                                    iconName = "d_Progress";
                                    string exportPath = string.IsNullOrEmpty(queue.ExportPath) ?
                                        queue.GetDefaultExportPath(defaultExportDir) :
                                        queue.ExportPath;
                                    message = $"Success! {exportPath}";
                                    break;
                                case EncodeQueue.ResultType.Error:
                                    iconName = "Error";
                                    message = "Failed. Please see the editor console.";
                                    break;
                            }

                            GUILayout.Label(EditorGUIUtility.IconContent(iconName), GUILayout.Width(20), GUILayout.Height(20));
                            EditorGUILayout.LabelField(message);
                        }
                    }
                }
            }
        }
    }
}

#endif