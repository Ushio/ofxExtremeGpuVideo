//
//  GpuVideoTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once

#ifndef _MSC_VER
#define _FILE_OFFSET_BITS 64
#else
#endif

#include <cstdio>
#include <cstdint>

class GpuVideoIO {
public:
	GpuVideoIO(const char *filename, const char *mode) {
		_fp = fopen(filename, mode);
		if (_fp == nullptr) {
			throw std::runtime_error("file not found");
		}
	}
	~GpuVideoIO() {
		fclose(_fp);
	}
	GpuVideoIO(const GpuVideoIO &) = delete;
	void operator=(const GpuVideoIO &) = delete;

	int seek(int64_t offset, int origin) {
#ifdef _MSC_VER
		return _fseeki64(_fp, offset, origin);
#else
		return fseeko(_fp, offset, origin);
#endif
	}
	int64_t tellg() {
#ifdef _MSC_VER
		return _ftelli64(_fp);
#else
		return ftello(_fp);
#endif
	}
	std::size_t read(void *dst, std::size_t size) {
		return fread(dst, 1, size, _fp);
	}
	std::size_t write(const void *src, size_t size) {
		return fwrite(src, 1, size, _fp);
	}
private:
	FILE *_fp = nullptr;
};