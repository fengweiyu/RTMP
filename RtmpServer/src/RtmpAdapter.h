/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpAdapter.h
* Description       :   RtmpAdapter operation center
                        适配外部底层依赖接口
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_ADAPTER_H
#define RTMP_ADAPTER_H


#define  RTMP_LOGW2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMP_LOGE2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMP_LOGD2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMP_LOGW(...)     printf(__VA_ARGS__)
#define  RTMP_LOGE(...)     printf(__VA_ARGS__)
#define  RTMP_LOGD(...)     printf(__VA_ARGS__)
#define  RTMP_LOGI(...)     printf(__VA_ARGS__)

#if 0
#include "XLog.h"
#include "XBaseTypes.h"
#define  RTMP_LOGW2(val,...)     logi(RTMP_SESSION) << lkv(ClientPort, val) << lformat(RTMP,__VA_ARGS__) << lend 
#define  RTMP_LOGE2(val,...)     loge(RTMP_SESSION) << lkv(ClientPort, val) << lformat(RTMP,__VA_ARGS__) << lend 
#define  RTMP_LOGD2(val,...)     logd(RTMP_SESSION) << lkv(ClientPort, val) << lformat(RTMP,__VA_ARGS__) << lend 
#define  RTMP_LOGW(...)     logi(RTMP_SESSION) << lformat(RTMP,__VA_ARGS__) << lend 
#define  RTMP_LOGE(...)     loge(RTMP_SESSION) << lformat(RTMP,__VA_ARGS__) << lend
#define  RTMP_LOGD(...)     logd(RTMP_SESSION) << lformat(RTMP,__VA_ARGS__) << lend
#define  RTMP_LOGI(...)     logi(RTMP_SESSION) << lformat(RTMP,__VA_ARGS__) << lend
#define  TCP_LOG(...)     LOGD(__VA_ARGS__)  
#endif

/*#ifndef uint64
#ifdef OS_WINDOWS
typedef unsigned __int64    uint64;
#define FORMAT_INT64 "%I64d"
#else
typedef unsigned long long  uint64;
#define FORMAT_INT64 "%lld"
#endif
#endif*/
#ifdef _WIN32
#include <Windows.h>
#define SleepMs(val) Sleep(val)
#define GetSecCnt() (GetTickCount64()/1000) // 返回自系统启动以来的秒数
#ifndef uint64
typedef unsigned __int64   uint64;
#endif
#else
#include <unistd.h>
#include <stdint.h>
#define SleepMs(val) usleep(val*1000)
#define GetSecCnt() (time(NULL)) //返回的是从Epoch开始的秒数
#ifndef uint64
typedef uint64_t uint64;
#endif
#endif

#define  Read16BE(ptr,val)     *val = ((unsigned char)ptr[0] << 8) | (unsigned char)ptr[1]
#define  Read24BE(ptr,val)     *val = ((unsigned char)ptr[0] << 16) | ((unsigned char)ptr[1] << 8) | (unsigned char)ptr[2]
#define  Read32BE(ptr,val)     *val = ((unsigned char)ptr[0] << 24) | ((unsigned char)ptr[1] << 16) | ((unsigned char)ptr[2] << 8) | (unsigned char)ptr[3]
#define  Read32LE(ptr,val)     *val = (unsigned char)ptr[0] | ((unsigned char)ptr[1] << 8) | ((unsigned char)ptr[2] << 16) | ((unsigned char)ptr[3] << 24)

#define Write16BE(p,value) \
do{ \
    p[0] = (unsigned char)((value >> 8) & 0xFF);  \
    p[1] = (unsigned char)((value) & 0xFF);    \
}while(0)

#define Write24BE(ptr,val) \
do{ \
    ptr[0] = (unsigned char)((val >> 16) & 0xFF); \
    ptr[1] = (unsigned char)((val >> 8) & 0xFF); \
    ptr[2] = (unsigned char)((val) & 0xFF);  \
}while(0)

#define Write32BE(p,value) \
do{ \
    p[0] = (unsigned char)((value >> 24) & 0xFF); \
    p[1] = (unsigned char)((value >> 16) & 0xFF); \
    p[2] = (unsigned char)((value >> 8) & 0xFF);  \
    p[3] = (unsigned char)((value) & 0xFF);    \
}while(0)

#define Write32LE(ptr,val) \
do{ \
    ptr[3] = (unsigned char)((val >> 24) & 0xFF);\
    ptr[2] = (unsigned char)((val >> 16) & 0xFF);\
    ptr[1] = (unsigned char)((val >> 8) & 0xFF);\
    ptr[0] = (unsigned char)((val) & 0xFF);\
}while(0)



#endif

