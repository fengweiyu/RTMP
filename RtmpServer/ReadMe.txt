RtmpServer支持windows和linux两个平台，并都支持推拉流
1.windows
    1.1.编译：
        打开RTMP/RtmpServer/prj/win/RtmpServer/RtmpServer.sln,
        即可编译运行
    1.2.使用：
        1.2.1.启动RtmpServer服务(将RTMP/RtmpServer/build/linux/x86/h264aac.flv拷贝到RtmpServer程序同目录)
        1.2.2.使用rtmp://10.10.22.121:9213/play/h264aac.flv这个URL即可播放，默认监听9213端口
        1.2.3.使用rtmp://10.10.22.121:9213/push/h264aac.flv 这个URL即可向服务器推流
              ffmpeg推流命令：
              ffmpeg -re -i D:\test\2024h264aac.flv -c copy -f flv rtmp://10.10.22.121:9213/push/h264aac.flv
2.linux
    2.1.编译：
        使用cmake进行编译，必须先安装cmake，安装后(执行./build.sh x86)：
        ywf@ywf-pc:/work/workspace/RTMP/RtmpServer$ ./build.sh x86 
        编译成功后，会在如下路径生成RtmpServer应用程序：
        ywf@ywf-pc:/work/workspace/RTMP/RtmpServer/build/linux/x86$ ls RtmpServer
        ./RtmpServer
    2.2.使用：
        2.2.1.启动RtmpServer服务(将RTMP/RtmpServer/build/linux/x86/h264aac.flv拷贝到RtmpServer程序同目录)
          ./RtmpServer
        2.2.2.使用rtmp://10.10.22.121:9213/play/h264aac.flv这个URL即可播放，默认监听9213端口
        2.2.3.使用rtmp://10.10.22.121:9213/push/h264aac.flv 这个URL即可向服务器推流
              ffmpeg推流命令：
              ffmpeg -re -i D:\test\2024h264aac.flv -c copy -f flv rtmp://10.10.22.121:9213/push/h264aac.flv