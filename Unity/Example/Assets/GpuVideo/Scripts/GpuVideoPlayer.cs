#define GPUVIDEO_SUPPORT_TIMELINE
using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
using UnityEngine.Playables;

#endif

#if GPUVIDEO_SUPPORT_TIMELINE
using UnityEngine.Timeline;
#endif


namespace ExtremeGpuVideo
{
#if GPUVIDEO_SUPPORT_TIMELINE
    [ExecuteInEditMode]
    public class GpuVideoPlayer : MonoBehaviour, ITimeControl, IPropertyPreview
#else
    public class GpuVideoPlayer : MonoBehaviour
#endif
    {
        public enum RenderTarget
        {
            MainTexture,
            RenderTexture,
        }

        public string PathForStreamingAssets
        {
            get => pathForStreamingAssets;
        }

        public RenderTarget Target
        {
            get => renderTarget;
        }

        public float TimeAt
        {
            get => timeAt;
            set
            {
                if(video != null)
                {
                    timeAt = Mathf.Clamp(value, 0.0f, video.Duration);
                    video.setTime(timeAt);
                }
            }
        }

        public bool IsPause
        {
            get => isPause;
            set => isPause = value;
        }

        public bool IsLoop
        {
            get => isLoop;
            set => isLoop = value;
        }

        [SerializeField]
        private string pathForStreamingAssets = string.Empty;
        [SerializeField]
        private RenderTarget renderTarget = RenderTarget.MainTexture;
        [SerializeField]
        private Renderer rendererComponent = null;
        [SerializeField]
        private RenderTexture renderTexture = null;
        [SerializeField, Range(0.0f, 1.0f)]
        private float timeAt = 0.0f;
        [SerializeField]
        private bool isPause = false;
        [SerializeField]
        private bool isLoop = false;

        private GpuVideo video = null;
        private bool controlExternal = false;
        private Shader flipyShader = null;
        private Material flipyMaterial = null;

        private Material renderMaterial
        {
            get
            {
                if (flipyMaterial == null)
                {
                    if (flipyShader == null)
                    {
                        flipyShader = Resources.Load<Shader>("FlipY");
                    }
                    flipyMaterial = new Material(flipyShader);
                }
                return flipyMaterial;
            }
        }

        private void RenderInternal()
        {
            switch (renderTarget)
            {
                case RenderTarget.MainTexture:
                    if (rendererComponent != null && Application.isPlaying)
                    {
                        rendererComponent.material.mainTexture = video.Texture;
                    }
                    break;
                case RenderTarget.RenderTexture:
                    if (renderTexture != null)
                    {
                        if(video.FlipY)
                        {
                            Graphics.Blit(video.Texture, renderTexture, renderMaterial);
                        }
                        else
                        {
                            Graphics.Blit(video.Texture, renderTexture);
                        }
                    }
                    break;
            }
        }

        private void OnEnable()
        {
            video = new GpuVideo(pathForStreamingAssets);
            RenderInternal();
        }

        private void OnDisable()
        {
            if (rendererComponent != null)
            {
                rendererComponent.material.mainTexture = null;
            }
            
            if(flipyMaterial != null)
            {
                if (Application.isEditor)
                {
                    DestroyImmediate(flipyMaterial);
                }
                else
                {
                    Destroy(flipyMaterial);
                }
                flipyMaterial = null;
            }

            video?.Dispose();
            video = null;
        }

        private void Update()
        {
            if(video == null)
            {
                return;
            }

            if (Application.isPlaying && isPause == false && controlExternal == false)
            {
                timeAt += Time.deltaTime;
                
                if (isLoop && video.Duration < timeAt)
                {
                    timeAt = 0.0f;
                }    
            }

            video.setTime(timeAt);
            RenderInternal();
        }

        public void SetTime(double time)
        {
            timeAt = (float)time % video.Duration;
        }

        public void OnControlTimeStart()
        {
            controlExternal = true;
        }

        public void OnControlTimeStop()
        {
            controlExternal = false;
        }

#if GPUVIDEO_SUPPORT_TIMELINE
        public void GatherProperties(PlayableDirector director, IPropertyCollector driver)
        {
            driver.AddFromName<GpuVideoPlayer>("timeAt");
        }
#endif

#if UNITY_EDITOR
        [CustomEditor(typeof(GpuVideoPlayer))]
        public class GpuVideoPlayerEditor : Editor
        {
            float _position = 0.0f;
            GpuVideo _video;


            void OnEnable()
            {

            }

            void OnDisable()
            {
                if (_video != null)
                {
                    _video.Dispose();
                    _video = null;
                }
            }

            public override void OnInspectorGUI()
            {
                //DrawDefaultInspector();

                try
                {
                    GpuVideoPlayer myScript = (GpuVideoPlayer)target;

                    // DrawProperty
                    myScript.pathForStreamingAssets = EditorGUILayout.TextField("PathForStreamingAssets", myScript.PathForStreamingAssets);
                    myScript.renderTarget = (RenderTarget)EditorGUILayout.EnumPopup("RenderTarget", myScript.renderTarget);

                    switch(myScript.renderTarget)
                    {
                        case RenderTarget.MainTexture:
                            myScript.rendererComponent = (Renderer)EditorGUILayout.ObjectField("RendererComponent", myScript.rendererComponent, typeof(Renderer), true);
                            break;
                        case RenderTarget.RenderTexture:
                            myScript.renderTexture = (RenderTexture)EditorGUILayout.ObjectField("RenderTexture", myScript.renderTexture, typeof(RenderTexture), true);
                            break;
                    }

                    myScript.timeAt = EditorGUILayout.FloatField("TimeAt", myScript.timeAt);
                    myScript.isPause = EditorGUILayout.Toggle("IsPause", myScript.isPause);
                    myScript.isLoop = EditorGUILayout.Toggle("IsLoop", myScript.isLoop);

                    if (_video == null || _video.PathForStreamingAssets != myScript.PathForStreamingAssets)
                    {
                        if (_video != null)
                        {
                            _video.Dispose();
                        }
                        _video = new GpuVideo(myScript.PathForStreamingAssets);
                    }

                    _position = GUILayout.HorizontalSlider(_position, 0.0f, 1.0f);
                    _video.setTime(_video.Duration * _position);
                    GUILayout.Label(_video.Texture, GUILayout.Width(300), GUILayout.Height(200));
                }
                catch (System.Exception)
                {
                    // :(
                }
            }
        }
#endif
    }
}