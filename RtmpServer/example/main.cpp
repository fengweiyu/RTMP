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
    iRet=pRtmpServerManager->Proc();//����
    
    return iRet;
}
/*****************************************************************************
-Fuction        : PrintUsage
-Description    : 
ʹ��rtmp://10.10.22.121:9216/play/h264aac.flv �ӷ�����������
rtmp://10.10.22.121:9216/play_enhanced/h265aac.flv
rtmp://10.10.22.121:9216/push/h264aac.flv ����
�����������ڳ���Ŀ¼������2024h264aac.mp4
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

