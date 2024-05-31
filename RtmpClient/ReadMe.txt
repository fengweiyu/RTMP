RtmpClient支持windows和linux两个平台，并都支持推拉流
1.windows
    1.1.编译：
        打开RTMP/RtmpClient/prj/win/RtmpClient/RtmpClient.sln,
        即可编译运行
    1.2.使用：
        1.2.1.先启动RtmpServer服务，再执行RtmpClient程序，注意要带参数
        1.2.2.注意要带参数使用，(假设服务地址10.10.22.121:9213)
              使用rtmp://10.10.22.121:9213/play/h264aac这个URL作为参数，会在当前目录下生成h264aac.mp4文件
              使用rtmp://10.10.22.121:9213/push/h264aac.flv这个URL作为参数 即可向服务器推流(需将RTMP/RtmpClient/build/linux/x86/h264aac.flv拷贝到RtmpClient程序同目录)

2.linux
    2.1.编译：
        使用cmake进行编译，必须先安装cmake，安装后(执行./build.sh x86)：
        ywf@ywf-pc:/work/workspace/RTMP/RtmpClient$ ./build.sh x86 
        编译成功后，会在如下路径生成RtmpServer应用程序：
        ywf@ywf-pc:/work/workspace/RTMP/RtmpClient/build/linux/x86$ ls RtmpClient

    2.2.使用：
        2.2.1.先启动RtmpServer服务，再执行RtmpClient程序，注意要带参数
        2.2.2.注意要带参数使用，(假设服务地址10.10.22.121:9213)
              ./RtmpClient rtmp://10.10.22.121:9213/play/h264aac
                使用rtmp://10.10.22.121:9213/play/h264aac这个URL作为参数，会在当前目录下生成h264aac.mp4文件
              ./RtmpClient rtmp://10.10.22.121:9213/push/h264aac.flv
                使用rtmp://10.10.22.121:9213/push/h264aac.flv这个URL作为参数 即可向服务器推流(需将RTMP/RtmpClient/build/linux/x86/h264aac.flv拷贝到RtmpClient程序同目录)