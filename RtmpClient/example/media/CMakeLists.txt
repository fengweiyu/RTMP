cmake_minimum_required(VERSION 3.10) 


project (media)
 
#set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ThirdLib/media) #CMAKE_INSTALL_PREFIX 外面设置，内部设置无效
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/wasm/x64)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/wasm/fdk-aac/lib/libfdk-aac.a)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows") #WIN32
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/win/x64)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/win/fdk-aac/lib/libfdk-aac.lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #子目录也会设置CMAKE_INSTALL_PREFIX，但是子目录退出后不生效，所以这里设置全局的，
    #这样这里生成的Makefile可以执行install到下面这个目录，缺点是子目录相关的库文件头文件也会拷贝过来
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build/linux/x64)
    set(AAC_LIB_PATH ${PROJECT_SOURCE_DIR}/AudioCodec/third/lib/linux/fdk-aac/lib/libfdk-aac.a)
else()  
    message("CMAKE_SYSTEM_NAME err is: ${CMAKE_SYSTEM_NAME}")
endif() 
# 添加 lib 子目录  
add_subdirectory(AudioCodec) 



# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("PROJECT_SOURCE_DIR is: ${PROJECT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}")

# 收集所有源文件  
file(GLOB src_list 
    src/MediaHandle.cpp
    src/TS/TsInterface.cpp
    src/TS/TsPack.cpp
    src/FMP4/FMP4HandleInterface.cpp
    src/FMP4/FMP4Handle.cpp
    src/FMP4/FMP4.cpp
    src/FLV/FlvHandleInterface.cpp
    src/FLV/FlvParseHandle.cpp
    src/FLV/FlvPackHandle.cpp
    src/FLV/FlvHandle.cpp
    src/RawVideoHandle.cpp
    src/RawAudioHandle.cpp
    )  # 可以根据你的目录结构调整 

#set(src_list
#    src/xxx.cpp
#)

# 定义库  
add_library(media ${src_list}) 

target_include_directories(media PUBLIC
    ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/src/FLV/include
    ${PROJECT_SOURCE_DIR}/src/FLV
    ${PROJECT_SOURCE_DIR}/src/TS/include
    ${PROJECT_SOURCE_DIR}/src/TS
    ${PROJECT_SOURCE_DIR}/src/FMP4/include
    ${PROJECT_SOURCE_DIR}/src/FMP4
)





# 安装库文件  
install(TARGETS media  
        DESTINATION ThirdLib/media/lib)  # 库文件的安装目录  

# 安装头文件  
file(GLOB PUBLIC_HEADERS "${PROJECT_SOURCE_DIR}/src/include/*.h")
install(FILES ${PUBLIC_HEADERS}  
        DESTINATION ThirdLib/media/include)  # 头文件的安装目录

#-s WASM=1 -s USE_SDL=2 -s TOTAL_MEMORY=268435456 通过设置 WASM=1，编译器会生成适用于 WebAssembly 的输出 定义 WebAssembly 模块的总内存大小256 MB 用于_mallo等内存拷贝
#-s EXPORTED_FUNCTIONS="['_fnInitDecoder', '_fnUninitDecoder', '_main', '_malloc', '_free']" js中无用定义和使用数组以及指针，必须要_malloc
#-s EXTRA_EXPORTED_RUNTIME_METHODS="['addFunction']" 这些都是链接选项，不是编译选项
#-s RESERVED_FUNCTION_POINTERS=15 指定要保留的函数指针（Function Pointers）数量 Emscripten 将为最多 15 个函数指针分配存储空间
#-s FORCE_FILESYSTEM=1 参数强制 Emscripten 在 WebAssembly 模块中启用文件系统支持
# 为目标添加编译选项  
#target_compile_options(MyExecutable PRIVATE -Wall -Wextra)  # 例如，添加警告选项  
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s TOTAL_MEMORY=268435456 -s RESERVED_FUNCTION_POINTERS=15 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free]")  
#target_link_options(MyExecutable PRIVATE ) 
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")  
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s TOTAL_MEMORY=268435456 -s RESERVED_FUNCTION_POINTERS=15 -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS=[_malloc,_free]") 
endif() 

# 收集源文件  
file(GLOB demo_list demo/MediaConvert.cpp demo/MediaConvertMain.cpp)
add_executable(MediaConvert ${demo_list})
 #set(CMAKE_EXECUTABLE_SUFFIX ".html") # 编译生成.html
set(MC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src/include ${PROJECT_SOURCE_DIR}/demo ${PROJECT_SOURCE_DIR}/AudioCodec/src/include)
set(MC_LINK_LIBS media AudioCodec ${AAC_LIB_PATH})
if(DEFINED PRI)  
    add_definitions(-DSUPPORT_PRI)
    set(MC_INCLUDE_DIRS ${MC_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/third/XBasic/include ${PROJECT_SOURCE_DIR}/third/XBasic/include/XBasic
    ${PROJECT_SOURCE_DIR}/third/XProtocol/include ${PROJECT_SOURCE_DIR}/third/XProtocol/include/XProtocol/StreamParser)
    set(MC_LINK_LIBS ${MC_LINK_LIBS} ${PROJECT_SOURCE_DIR}/third/XBasic/lib/wasm/libXBasic.a ${PROJECT_SOURCE_DIR}/third/XProtocol/lib/wasm/libXProtocol.a ) 
else()  
endif()  
target_include_directories(MediaConvert PUBLIC
    ${MC_INCLUDE_DIRS}
)
target_link_libraries(MediaConvert PUBLIC ${MC_LINK_LIBS}) 
install(TARGETS MediaConvert DESTINATION .) #安装目录 目录MediaConvert换成目录. 也可以，这样就在当前目录生成不会生成在MediaConvert目录下

#SET_TARGET_PROPERTIES(base64_static PROPERTIES OUTPUT_NAME base64)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)

#TARGET_LINK_LIBRARIES(webrtc pthread -lm)

#TARGET_LINK_LIBRARIES(crnode log4cpp thrift)










