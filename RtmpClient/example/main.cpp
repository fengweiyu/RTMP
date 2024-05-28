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

#include "RtmpClientManager.h"

static void PrintUsage(char *i_strProcName);

const char * g_strPushURL="rtmp://10.10.22.121:9213/push/2024h264aac.flv";
const char * g_strPlayURL="rtmp://10.10.22.121:9213/pull/2024h264aac.flv";

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
    
    if(argc !=2)
    {
        PrintUsage(argv[0]);
        return iRet;
    }
    RtmpClientManager *pRtmpClientManager = new RtmpClientManager(argv[1]);
    iRet=pRtmpClientManager->Proc();//×èÈû
    delete pRtmpClientManager;
    return iRet;
}
/*****************************************************************************
-Fuction        : PrintUsage
-Description    : PrintUsage
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
static void PrintUsage(char *i_strProcName)
{
    printf("Usage: %s url \r\n",i_strProcName);
    printf("eg: %s %s \r\n eg: %s %s \r\n",i_strProcName,g_strPushURL,i_strProcName,g_strPlayURL);
    printf("run default args: %s %s \r\n",i_strProcName,g_strPushURL);
}

