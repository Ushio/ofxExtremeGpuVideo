using UnityEngine;

public class Lz4Object : MonoBehaviour {
	public int frameAt = 0;

	GpuVideo _video;
	Material _material;

	// Use this for initialization
	void Start () {
	}
	
	// Update is called once per frame
	void Update () {
		_video.setFrame (frameAt);
	}

	void OnEnable() {
		_video = new GpuVideo ("/footage.gv");
		_material = this.gameObject.GetComponent<Renderer>().material;
		_material.mainTexture = _video.Texture;
	}
	void OnDisable() {
		_material.mainTexture = null;
		_video.Dispose ();
		_video = null;
	}
}
