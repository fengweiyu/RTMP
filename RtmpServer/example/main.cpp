/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       main.cpp
* Description           : 	    
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#include "RtmpServerManager.h"

static void PrintUsage(char *i_strProcName);

/*****************************************************************************
-Fuction        : main
-Description    : main
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int main(int argc, char* argv[]) 
{
    int iRet = -1;
    
    int dwServerPort=9216;
    
    if(argc !=2)
    {
        PrintUsage(argv[0]);
    }
    else
    {
        dwServerPort=atoi(argv[1]);
    }
    RtmpServerManager *pRtmpServerManager = new RtmpServerManager(dwServerPort);
    iRet=pRtmpServerManager->Proc();//阻塞
    
    return iRet;
}
/*****************************************************************************
-Fuction        : PrintUsage
-Description    : 
使用rtmp://10.10.22.121:9216/play/h264aac.flv 从服务器推流或
rtmp://10.10.22.121:9216/play_enhanced/h265aac.flv
rtmp://10.10.22.121:9216/push/h264aac.flv 拉流
如果推流则会在程序目录下生成2024h264aac.mp4
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
static void PrintUsage(char *i_strProcName)
{
    printf("Usage: %s ServerPort \r\n",i_strProcName);
    printf("run default args: %s 9216 \r\n",i_strProcName);
    printf("play url eg: %s\r\n","rtmp://localhost:9216/play/h264aac.flv");
    printf("play url eg: %s\r\n","rtmp://localhost:9216/play_enhanced/h265aac.flv");
    printf("push url eg: %s\r\n","rtmp://localhost:9216/push/h264aac");
}

