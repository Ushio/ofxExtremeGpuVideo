using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif

public class GpuVideoPlayer : MonoBehaviour {
    public string PathForStreamingAssets = "";
    public Renderer RendererCompornent;
    public float TimeAt = 0.0f;
    public bool IsPause = false;
    public bool IsLoop = false;

	GpuVideo _video;
	Material _material;

	// Use this for initialization
	void Start () {
        
    }
	
	// Update is called once per frame
	void Update () {
        if(_material == null)
        {
            _material = RendererCompornent.material;
            _material.mainTexture = _video.Texture;
        }
        if(IsPause == false)
        {
            TimeAt += Time.deltaTime;

            if(IsLoop && _video.Duration < TimeAt)
            {
                TimeAt = 0.0f;
            }
        }

        _video.setTime(TimeAt);
	}

	void OnEnable() {
		_video = new GpuVideo (PathForStreamingAssets);
    }
	void OnDisable() {
		_material.mainTexture = null;
		_video.Dispose ();
		_video = null;
	}
}

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
        if(_video != null)
        {
            _video.Dispose();
            _video = null;
        }
    }

    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        try
        {
            GpuVideoPlayer myScript = (GpuVideoPlayer)target;

            if (_video == null || _video.PathForStreamingAssets != myScript.PathForStreamingAssets)
            {
                if(_video != null)
                {
                    _video.Dispose();
                }
                _video = new GpuVideo(myScript.PathForStreamingAssets);
            }

            _video.setTime(_video.Duration * _position);
            GUILayout.Label(_video.Texture, GUILayout.Width(300), GUILayout.Height(200));
            _position = GUILayout.HorizontalSlider(_position, 0.0f, 1.0f);
        } catch (System.Exception ) {
            // :(
        }
    }
}
#endif