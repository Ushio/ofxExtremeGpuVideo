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

#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

#include "cmdline.h"

int main(int argc, const char * argv[]) {
    cmdline::parser p;
    p.add("help", 0, "print help");
    p.add<std::string>("input", 'i', "input video (required)", true, "");
    p.add<std::string>("output", 'o', "output video (required)", true, "");
    
    if (!p.parse(argc, argv) || p.exist("help")){
        std::cout << p.error_full() << p.usage();
        return 0;
    }
    
    auto input_path = boost::filesystem::absolute(p.get<std::string>("input"));
    auto output_path = boost::filesystem::absolute(p.get<std::string>("output"));
    
    std::cout << "input: " << input_path << std::endl;
    std::cout << "output: " << output_path << std::endl;
    
    cv::VideoCapture capture(input_path.string());
    
    if(capture.isOpened() == false) {
        std::cout << "can't open input" << std::endl;
        return -1;
    }
    
    std::cout << "-- image property --" << std::endl;
    
    uint32_t width = (uint32_t)capture.get(CV_CAP_PROP_FRAME_WIDTH);
    uint32_t height = (uint32_t)capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    float fps = (float)capture.get(CV_CAP_PROP_FPS);
    uint32_t frameCount = (uint32_t)capture.get(CV_CAP_PROP_FRAME_COUNT);
    
    std::cout << "width: " << width << std::endl;
    std::cout << "height: " << height << std::endl;
    std::cout << "fps: " << fps << std::endl;
    std::cout << "frame count: " << frameCount << std::endl;
    
    std::cout << "-- begin encode --" << std::endl;
    
    std::vector<uint8_t> raw_buffer(width * height * 4);
    
    for(int i = 0 ; i < frameCount ; ++i) {
        cv::Mat frame;
        if(capture.read(frame)) {
            int c = frame.channels();
            std::cout << c;
        }
        
        // アルファが読めない問題がある
        // cv::cvtColor(frame, raw_buffer.data(), CV_BGR2RGBA, 4);
    }
    
    
    return 0;
}
