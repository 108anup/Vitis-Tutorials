#!/bin/bash
g++ -std=c++14 -O2 -o usb_input_multi_threads_refinedet_drm test/usb_input_multi_threads_refinedet.cpp -Iinclude src/*.cpp -lglog -lmy_v4l2s -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lvitis_ai_library-refinedet -lvitis_ai_library-model_config -ljson-c -lglog -lpthread -ldrm -lvart-runner -I/usr/include/drm -DUSE_DRM=1
