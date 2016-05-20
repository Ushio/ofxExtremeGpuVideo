//
//  main.cpp
//  converter-cmd-osx
//
//  Created by Nakazi_w0w on 5/10/16.
//  Copyright © 2016 wow. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <tbb/tbb.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "squish.h"
#include "lz4.h"
#include "lz4frame.h"
#include "lz4hc.h"

#include "cmdline.h"

#include "GpuVideo.hpp"
#include "GpuVideoIO.hpp"

/*
 TODO: 16bit png
 TODO: 関数化
 TODO: GUI front end
 TODO: format
 TODO: convert command
 */

int main(int argc, const char * argv[]) {
    cmdline::parser p;
    p.add("help", 0, "print help");
    p.add<std::string>("input", 'i', "input video (required)", true, "");
    p.add("deep", 'd', "deep mode");
    
    if (!p.parse(argc, argv) || p.exist("help")){
        std::cout << p.error_full() << p.usage();
        return 0;
    }
    
    auto input_path = boost::filesystem::absolute(p.get<std::string>("input"));
    
    auto output_path = input_path;
    output_path.replace_extension("gv");
    
    auto intermediate_path = output_path;
    intermediate_path.replace_extension("gvintermediate");
    
    if(p.exist("deep")) {
        std::cout << "deep mode." << std::endl;
    }
    
    std::cout << "input: " << input_path << std::endl;
    std::cout << "output: " << output_path << std::endl;
    std::cout << "intermediate: " << intermediate_path << std::endl;
    
    // パス
    const char *BREW_PATH = "/usr/local/bin/";
    const char *PATH = getenv("PATH");
    if(strstr(PATH, BREW_PATH) == nullptr) {
        std::string newPath = std::string(PATH) + ":" + BREW_PATH;
        setenv("PATH", newPath.c_str(), 1);
    }
    
    boost::filesystem::remove_all(intermediate_path);
    boost::filesystem::create_directory(intermediate_path);
    
    char command[1024];
    sprintf(command, "ffmpeg -i %s -an -vcodec png %s/%%05d.png", input_path.c_str(), intermediate_path.c_str());
    std::system(command);
    
    // memory
    uint32_t _format = 0;
    std::vector<std::string> _imagePaths;
    uint32_t _width = 0;
    uint32_t _height = 0;
    float _fps = 30.0f;
    uint32_t _bufferSize = 0;
    uint32_t _squishFlag = 0;
    
    std::vector<uint8_t> _gpuCompressBuffer;
    std::vector<uint8_t> _lz4CompressBuffer;
    std::vector<Lz4Block> _lz4blocks;
    std::unique_ptr<GpuVideoIO> _io;
    
    int _index = 0;
    
    // char command[1024];
    sprintf(command, "ffmpeg -i %s 2>&1", input_path.c_str());
    
    std::regex regex("Video:.*, ?([\\d\\.]+)\\s?fps");
    std::smatch match;
    
    char line_buffer[1024 * 1024];
    FILE *fp = popen(command, "r");
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        if(std::regex_search(std::string(line_buffer), match, regex)) {
            if(match.size() == 2) {
                try {
                    _fps = boost::lexical_cast<float>(match[1]);
                    break;
                } catch (std::exception &e) {
                    
                }
            }
        }
    }
    pclose(fp);
    
    for(int i = 0; true ; ++i) {
        char path[1024];
        sprintf(path, "%s/%05d.png", intermediate_path.c_str(), i + 1);
        if(boost::filesystem::exists(path) == false) {
            break;
        }
        _imagePaths.push_back(path);
    }
    
    if(_imagePaths.size() > 0) {
        int width;
        int height;
        int bpp;
        unsigned char* pixels = stbi_load (_imagePaths[0].c_str(), &width, &height, &bpp, 4);
        stbi_image_free (pixels);
        
        _width = width;
        _height = height;
    } else {
        std::cout << "intermediate is invalid" << std::endl;
        return -1;
    }
    
    uint32_t flagQuality = p.exist("deep") ? squish::kColourIterativeClusterFit : (squish::kColourRangeFit | squish::kColourMetricUniform);
    
    const char* kFormatStrings[] = {"DXT 1", "DXT 3", "DXT 5"};
    static const int kGpuFmts[] = {squish::kDxt1, squish::kDxt3, squish::kDxt5};
    static const int kGpuVideoFmts[] = {GPU_COMPRESS_DXT1, GPU_COMPRESS_DXT3, GPU_COMPRESS_DXT5};
    
    _squishFlag = flagQuality | kGpuFmts[_format];
    _bufferSize = squish::GetStorageRequirements(_width, _height, _squishFlag);
    
    std::cout << "begin encode..." << std::endl;
    
    // 書き出し開始
    _io = std::unique_ptr<GpuVideoIO>(new GpuVideoIO(output_path.c_str(), "wb"));
    
    // ヘッダー情報書き出し
#define W(v) if(_io->write(&v, sizeof(v)) != sizeof(v)) { assert(0); }
    W(_width);
    W(_height);
    
    uint32_t frameCount = (uint32_t)_imagePaths.size();
    W(frameCount);
    W(_fps);
    uint32_t videoFmt = kGpuVideoFmts[_format];
    W(videoFmt);
    W(_bufferSize);
#undef W
    
    // sprintf(command, "ffmpeg -i ", )
    // std::system("ffmpeg ");
    
    for(;;) {
        if(_index < _imagePaths.size()) {
            auto compress = [_imagePaths, _width, _height, _squishFlag](int index, uint8_t *dst) {
                std::string src = _imagePaths[index];
                
                int width;
                int height;
                int bpp;
                unsigned char* pixels = stbi_load (src.c_str(), &width, &height, &bpp, STBI_rgb_alpha);
                if(width != _width || height != _height) {
                    abort();
                }
                
                squish::CompressImage(pixels, _width, _height, dst, _squishFlag);
                
                stbi_image_free (pixels);
            };
            
            const int kBatchCount = 32;
            int workCount = std::min((int)_imagePaths.size() - _index, kBatchCount);
            
            uint32_t lz4sizes[kBatchCount];
            int compressBound = LZ4_compressBound(_bufferSize);
            
            _gpuCompressBuffer.resize(workCount * _bufferSize);
            _lz4CompressBuffer.resize(workCount * compressBound);
            
            tbb::parallel_for(tbb::blocked_range<int>( 0, workCount, 1 ), [compress, _index, _bufferSize, compressBound, &lz4sizes, &_gpuCompressBuffer, &_lz4CompressBuffer](const tbb::blocked_range< int >& range) {
                for (int i = range.begin(); i != range.end(); i++) {
                    compress(_index + i, _gpuCompressBuffer.data() + i * _bufferSize);
                    lz4sizes[i] = LZ4_compress_HC((char *)_gpuCompressBuffer.data() + i * _bufferSize,
                                                  (char *)_lz4CompressBuffer.data() + i * compressBound,
                                                  _bufferSize, compressBound, 16);
                    // printf("%d -> %d, %.2f%%\n", (int)_bufferSize, lz4sizes[i], (float)lz4sizes[i] / (float)_bufferSize);
                }
            });
            
            uint64_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
            for (int i = 0; i < workCount; i++) {
                // 住所を記録しつつ
                Lz4Block lz4block;
                lz4block.address = head;
                lz4block.size = lz4sizes[i];
                head += lz4block.size;
                _lz4blocks.push_back(lz4block);
                
                // 書き込み
                if(_io->write(_lz4CompressBuffer.data() + i * compressBound, lz4sizes[i]) != lz4sizes[i]) {
                    assert(0);
                }
            }
            
            _index += workCount;
            
            double progress = 100.0 * (double)_index / _imagePaths.size();
            printf("%d / %d (%.1f %%)\n", _index, (int)_imagePaths.size(), progress);
        } else {
            // 最後に住所を記録
            uint64_t size = _lz4blocks.size() * sizeof(Lz4Block);
            if(_io->write(_lz4blocks.data(), size) != size) {
                assert(0);
            }
            
            // ファイルをクローズ
            _io.reset();
            
            // 終了
            break;
        }
    }
    
    boost::filesystem::remove_all(intermediate_path);
    
    return 0;
}
