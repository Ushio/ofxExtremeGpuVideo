using UnityEngine;

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
