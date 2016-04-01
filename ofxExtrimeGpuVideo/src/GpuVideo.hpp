//
//  GpuVideo.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/29/16.
//
//

#pragma once

/*
 0: uint32_t width
 4: uint32_t height
 8: uint32_t frame count
 12: float fps
 16: uint32_t fmt (DXT1 = 1, DXT5 = 5)
 20: uint32_t frame bytes
 24: raw memory storage
 eof: [(uint64_t, uint64_t)..<frame count] (address, size) of lz4, address is zero based from file head
 */

enum GPU_COMPRESS : uint32_t {
    GPU_COMPRESS_DXT1 = 1,
    GPU_COMPRESS_DXT3 = 3,
    GPU_COMPRESS_DXT5 = 5,
};

static const uint32_t kRawMemoryAt = 24;

struct Lz4Block {
    uint64_t address = 0;
    uint64_t size = 0;
};
