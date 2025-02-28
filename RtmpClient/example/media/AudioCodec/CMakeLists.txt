cmake_minimum_required(VERSION 3.10) 


project (AudioCodec)
 
#set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ThirdLib/AudioCodec) #CMAKE_INSTALL_PREFIX 外面设置，内部设置无效

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("PROJECT_SOURCE_DIR is: ${PROJECT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}")

# 收集所有源文件  
file(GLOB src_list 
    src/AudioCodec.cpp
    src/CodecAAC.cpp
    src/CodecWAV.cpp
    src/CodecG711A.cpp
    src/CodecG711U.cpp
    src/CodecPCM.cpp
    )  # 可以根据你的目录结构调整 

#set(src_list
#    src/xxx.cpp
#)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(THIRD_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/third/lib/wasm/fdk-aac/include/fdk-aac)
    set(THIRD_LINK_LIBS ${PROJECT_SOURCE_DIR}/third/lib/wasm/fdk-aac/lib/libfdk-aac.a ) 
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    #cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -A x64 ..  
    #如果报错does not support platform specification   CMake Error: CMAKE_C_COMPILER not set 等，则因为当前(sh)编译环境没有导入编译工具，
    #解决方法："C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    #cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=../package_static -DCMAKE_DEBUG_POSTFIX=_staticd -DCMAKE_RELEASE_POSTFIX=_static
    #ninja 
    #ninja install
    set(THIRD_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/third/lib/win/fdk-aac/include/fdk-aac)
    set(THIRD_LINK_LIBS ${PROJECT_SOURCE_DIR}/third/lib/win/fdk-aac/lib/libfdk-aac.lib ) #cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -A x64 ..  ninja
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THIRD_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/third/lib/linux/fdk-aac/include/fdk-aac)
    set(THIRD_LINK_LIBS ${PROJECT_SOURCE_DIR}/third/lib/linux/fdk-aac/lib/libfdk-aac.a ) 
else()  
    message("CMAKE_SYSTEM_NAME err is: ${CMAKE_SYSTEM_NAME}")
endif() 

# 定义库  
add_library(AudioCodec ${src_list}) 

target_include_directories(AudioCodec PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/include
    ${THIRD_INCLUDE_DIRS}
)



# 安装库文件  
install(TARGETS AudioCodec  
        DESTINATION lib)  # 库文件的安装目录  

# 安装头文件  
file(GLOB PUBLIC_HEADERS "${PROJECT_SOURCE_DIR}/src/include/*.h")
install(FILES ${PUBLIC_HEADERS}  
        DESTINATION include)  # 头文件的安装目录



# 收集源文件  
file(GLOB demo_list demo/main.cpp)
add_executable(AudioTranscode ${demo_list})
 #set(CMAKE_EXECUTABLE_SUFFIX ".html") # 编译生成.html
set(EXE_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src/include ${THIRD_INCLUDE_DIRS})
set(EXE_LINK_LIBS AudioCodec ${THIRD_LINK_LIBS})

target_include_directories(AudioTranscode PUBLIC
    ${EXE_INCLUDE_DIRS}
)
target_link_libraries(AudioTranscode PUBLIC ${EXE_LINK_LIBS}) 
install(TARGETS AudioTranscode DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下

#SET_TARGET_PROPERTIES(base64_static PROPERTIES OUTPUT_NAME base64)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)

#TARGET_LINK_LIBRARIES(webrtc pthread -lm)

#TARGET_LINK_LIBRARIES(crnode log4cpp thrift)










