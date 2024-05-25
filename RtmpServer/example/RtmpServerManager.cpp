/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpServerManager.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "RtmpServerManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <utility>

using std::make_pair;

/*****************************************************************************
-Fuction		: RtmpServerManager
-Description	: RtmpServerManager
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpServerManager :: RtmpServerManager(int i_iServerPort)
{
    TcpServer::Init(NULL,i_iServerPort);
}

/*****************************************************************************
-Fuction		: ~RtmpServerManager
-Description	: ~RtmpServerManager
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpServerManager :: ~RtmpServerManager()
{
}

/*****************************************************************************
-Fuction		: Proc
-Description	: 阻塞
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerManager :: Proc()
{
    int iClientSocketFd=-1;
    RtmpServerIO *pRtmpServerIO = NULL;
    while(1)
    {
        iClientSocketFd=TcpServer::Accept();
        if(iClientSocketFd<0)  
        {  
            SleepMs(10);
            CheckMapServerIO();
            continue;
        } 
        pRtmpServerIO = new RtmpServerIO(iClientSocketFd);
        AddMapServerIO(pRtmpServerIO,iClientSocketFd);
    }
    return 0;
}

/*****************************************************************************
-Fuction        : CheckMapServerIO
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerManager::CheckMapServerIO()
{
    int iRet = -1;
    RtmpServerIO *pRtmpServerIO=NULL;

    std::lock_guard<std::mutex> lock(m_MapMtx);//std::lock_guard对象会在其作用域结束时自动释放互斥量
    for (map<int, RtmpServerIO *>::iterator iter = m_RtmpServerIOMap.begin(); iter != m_RtmpServerIOMap.end(); )
    {
        pRtmpServerIO=iter->second;
        if(0 == pRtmpServerIO->GetProcFlag())
        {
            delete pRtmpServerIO;
            iter=m_RtmpServerIOMap.erase(iter);// 擦除元素并返回下一个元素的迭代器
        }
        else
        {
            iter++;// 继续遍历下一个元素
        }
    }
    return 0;
}

/*****************************************************************************
-Fuction        : AddMapHttpSession
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerManager::AddMapServerIO(RtmpServerIO * i_pRtmpServerIO,int i_iClientSocketFd)
{
    int iRet = -1;

    if(NULL == i_pRtmpServerIO)
    {
        RTMPS_LOGE("AddMapServerIO NULL!!!%p\r\n",i_pRtmpServerIO);
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_MapMtx);//std::lock_guard对象会在其作用域结束时自动释放互斥量
    m_RtmpServerIOMap.insert(make_pair(i_iClientSocketFd,i_pRtmpServerIO));
    return 0;
}





