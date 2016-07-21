#pragma once

#include "GpuVideoTexture.hpp"
#include "GpuVideoReader.hpp"
#include "GpuVideoReaderDecompressed.hpp"
#include "GpuVideoStreamingTexture.hpp"
#include "GpuVideoOnGpuMemoryTexture.hpp"

class ofxGvTexture {
public:
	enum Mode {
		/* streaming from storage */
		GPU_VIDEO_STREAMING_FROM_STORAGE,

		/* on memory streaming */
		GPU_VIDEO_STREAMING_FROM_CPU_MEMORY,

		/* on memory streaming with pre decompressed */
		GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED,

		/* all gpu texture */
		GPU_VIDEO_ON_GPU_MEMORY
	};

	ofxGvTexture() {

	}
	~ofxGvTexture() {
		ofTextureData &data = _texture.texData;
		data.textureID = 0;
	}
	ofxGvTexture(const ofxGvTexture &) = delete;
	void operator=(const ofxGvTexture &) = delete;

	std::shared_ptr<IGpuVideoReader> load(const std::string &name, Mode mode, GLenum interpolation = GL_LINEAR, GLenum wrap = GL_CLAMP_TO_EDGE) {
		std::shared_ptr<IGpuVideoReader> reader;
		switch (mode) {
		case GPU_VIDEO_STREAMING_FROM_STORAGE: {
			reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false);
			_videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
			break;
		}
		case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY: {
			reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), true);
			_videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
			break;
		}
		case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED: {
			reader = std::make_shared<GpuVideoReaderDecompressed>(std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false));
			_videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
			break;
		}
		case GPU_VIDEO_ON_GPU_MEMORY:
			reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false);
			_videoTexture = std::make_shared<GpuVideoOnGpuMemoryTexture>(reader, interpolation, wrap);
			break;
		}

		_width = reader->getWidth();
		_height = reader->getHeight();
		_frameCount = reader->getFrameCount();
		_framePerSecond = reader->getFramePerSecond();

		_isLoaded = true;
		return reader;
	}
	std::shared_ptr<IGpuVideoReader> load(std::shared_ptr<IGpuVideoReader> reader, Mode mode, GLenum interpolation = GL_LINEAR, GLenum wrap = GL_CLAMP_TO_EDGE) {
		switch (mode) {
		case GPU_VIDEO_STREAMING_FROM_STORAGE:
		case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY:
		case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED: {
			_videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
			break;
		}
		case GPU_VIDEO_ON_GPU_MEMORY:
			_videoTexture = std::make_shared<GpuVideoOnGpuMemoryTexture>(reader, interpolation, wrap);
			break;
		}

		_width = reader->getWidth();
		_height = reader->getHeight();
		_frameCount = reader->getFrameCount();
		_framePerSecond = reader->getFramePerSecond();

		_isLoaded = true;
		return reader;
	}
	void unload() {
		_videoTexture = std::shared_ptr<IGpuVideoTexture>();
		_width = 0;
		_height = 0;
		_frameCount = 0;
		_framePerSecond = 0;

		_frameAt = 0;

		ofTextureData &data = _texture.texData;
		data.textureID = 0;
		_texture.clear();

		_isLoaded = false;
	}
	bool isLoaded() const {
		return _isLoaded;
	}

	void update() {
		if (_isLoaded == false) {
			return;
		}
		updateCPU();
		uploadGPU();
	}

	// separate update (advanced use)
	void updateCPU() {
		_videoTexture->updateCPU(_frameAt);
	}
	void uploadGPU() {
		_videoTexture->uploadGPU();

		ofTextureData &data = _texture.texData;
		data.textureID = 0;
		_texture.setUseExternalTextureID(_videoTexture->getTexture());

		data.width = _width;
		data.height = _height;
		data.textureTarget = GL_TEXTURE_2D;
		data.tex_w = _width;
		data.tex_h = _height;
		data.tex_t = 1.0f;
		data.tex_u = 1.0f;
	}

	float getDuration() const {
		return _frameCount * (1.0f / _framePerSecond);
	}

	void setTime(float atTime) {
		float framef = atTime * _framePerSecond;
		this->setFrame(static_cast<int>(framef));
	}
	void setFrame(int frameAt) {
		frameAt = std::max(frameAt, 0);
		frameAt = std::min(frameAt, (int)_frameCount - 1);
		_frameAt = frameAt;
	}

	ofTexture &getTexture() {
		return _texture;
	}
private:
	bool _isLoaded = false;
	int _frameAt = 0;

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _frameCount = 0;
	float _framePerSecond = 0;

	std::shared_ptr<IGpuVideoTexture> _videoTexture;

	ofTexture _texture;
};