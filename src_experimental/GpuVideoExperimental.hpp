//
//  GpuVideoExperimental.hpp
//  example-player-osx
//
//  Created by Nakazi_w0w on 5/27/16.
//
//

#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <memory>

#include "GpuVideo.hpp"
#include "GpuVideoIO.hpp"
#include "FileIO.hpp"

namespace gv {
    struct Header {
        uint32_t width;
        uint32_t height;
        uint32_t frame_count;
        float frame_per_second;
        uint32_t format;
        uint32_t frame_bytes;
    };
    
    class DataSourceStreamPolicy {
    public:
        enum {
            thread_safe = false
        };
        DataSourceStreamPolicy(std::shared_ptr<FileIO> io):_io(io) {
            io->seek(0, SEEK_END);
            _size = io->tellg();
        }
        template <class F>
        void touch(uint64_t at, uint64_t bytes, F f) {
            _buffer.reserve(bytes * 2);
            _buffer.resize(bytes);
            _io->seek(at, SEEK_SET);
            _io->read(_buffer.data(), bytes);
            f(_buffer.data());
        }
        uint64_t size() const {
            return _size;
        }
    private:
        uint64_t _size = 0;
        std::vector<uint8_t> _buffer;
        std::shared_ptr<FileIO> _io;
    };
    class DataSourceOnMemoryPolicy {
    public:
        enum {
            thread_safe = true
        };
        DataSourceOnMemoryPolicy(std::shared_ptr<FileIO> io) {
            io->seek(0, SEEK_END);
            uint64_t filesize = io->tellg();
            io->seek(0, SEEK_SET);
            
            _data.resize(filesize);
            io->read(_data.data(), filesize);
        }
        template <class F>
        void touch(uint64_t at, uint64_t bytes, F f) {
            f(_data.data() + at);
        }
        uint64_t size() const {
            return _data.size();
        }
    private:
        std::vector<uint8_t> _data;
    };
    
    template <class DataSourcePolicy>
    class Reader {
    public:
        enum {
            thread_safe = DataSourcePolicy::thread_safe
        };
        Reader(const std::string &path) {
            auto io = std::shared_ptr<FileIO>(new FileIO(path.c_str(), "rb"));
            io->read(&_header, sizeof(Header));
            _dataSource = std::unique_ptr<DataSourcePolicy>(new DataSourcePolicy(io));
            
            _lz4Blocks.resize(_header.frame_count);
            uint64_t lz4block_size = sizeof(Lz4Block) * _header.frame_count;
            _dataSource->touch(_dataSource->size() - lz4block_size, lz4block_size, [this, lz4block_size](const uint8_t *p) {
                memcpy(_lz4Blocks.data(), p, lz4block_size);
            });
        }
        uint32_t width() const {
            return _header.width;
        }
        uint32_t height() const {
            return _header.height;
        }
        uint32_t frame_count() const {
            return _header.frame_count;
        }
        float frame_per_second() const {
            return _header.frame_per_second;
        }
        GPU_COMPRESS format() const {
            return GPU_COMPRESS(_header.format);
        }
        uint32_t frame_bytes() const {
            return _header.frame_bytes;
        }
        void read(uint8_t *dst, int frame) const {
            assert(0 <= frame && frame < _lz4Blocks.size());
            Lz4Block lz4block = _lz4Blocks[frame];
            _dataSource->touch(lz4block.address, lz4block.size, [dst, lz4block, this](const uint8_t *p) {
                int r = LZ4_decompress_safe((const char *)p, (char *)dst, static_cast<int>(lz4block.size), _header.frame_bytes);
                assert(r == _header.frame_bytes);
            });
        }
    private:
        std::unique_ptr<DataSourcePolicy> _dataSource;
        Header _header;
        std::vector<Lz4Block> _lz4Blocks;
    };
    
    // Set Reader ができれば良い？うーん、結局のところ、再生状態を保持したほうがいい・・・
    class StreamingTexture {
    public:
        
    };
//    
//    template <class
//    class Reader
    
//    template <class T>
//    class Reader {
//    public:
//        uint32_t width() const {
//            return static_cast<T*>(this)->width();
//        }
//        uint32_t height() const {
//            return static_cast<T*>(this)->height();
//        }
//        uint32_t frame_count() const {
//            return static_cast<T*>(this)->frame_count();
//        }
//        float frame_per_second() const {
//            return static_cast<T*>(this)->frame_per_second();
//        }
//        int format() const {
//            return static_cast<T*>(this)->format();
//        }
//        uint32_t frame_bytes() const {
//            return static_cast<T*>(this)->frame_bytes();
//        }
//
//        bool thread_safe() const {
//            return static_cast<T*>(this)->thread_safe();
//        }
//        void read(uint8_t *dst, int frame) const {
//            return static_cast<T*>(this)->read(dst, frame);
//        }
//    };
//    
//    class StorageReader : public Reader<StorageReader> {
//    public:
//        StorageReader(const char *path) {
//            
//        }
//        bool thread_safe() const {
//            return false;
//        }
//    };
//    class OnMemoryReader : public Reader<OnMemoryReader> {
//    public:
//        bool thread_safe() const {
//            return true;
//        }
//    };
//    
//    class StreamingTexture {
//    public:
//        template <class T>
//        void update_cpu(const Reader<T> &reader) {
//            
//        }
//        std::vector<uint8_t> _decompressed;
//    };
    
}

