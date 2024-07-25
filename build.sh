#!/bin/sh

cd ${PWD}/../build/ffmpeg/

make clean
make -j8

llvm-dlltool -m i386 -d libavcodec/avcodec-61.def      -l libavcodec/avcodec.lib       -D avcodec-61.dll
llvm-dlltool -m i386 -d libavdevice/avdevice-61.def    -l libavdevice/avdevice.lib     -D avdevice-61.dll
llvm-dlltool -m i386 -d libavfilter/avfilter-10.def    -l libavfilter/avfilter.lib     -D avfilter-10.dll
llvm-dlltool -m i386 -d libavformat/avformat-61.def    -l libavformat/avformat.lib     -D avformat-61.dll
llvm-dlltool -m i386 -d libavutil/avutil-59.def        -l libavutil/avutil.lib         -D avutil-59.dll
llvm-dlltool -m i386 -d libswresample/swresample-5.def -l libswresample/swresample.lib -D swresample-5.dll
llvm-dlltool -m i386 -d libswscale/swscale-8.def       -l libswscale/swscale.lib       -D swscale-8.dll

make install
