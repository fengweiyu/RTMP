/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpServerManager.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef RTMP_SERVER_MANAGER_H
#define RTMP_SERVER_MANAGER_H

#include <mutex>
#include <string>
#include <list>
#include <map>
#include "RtmpServerIO.h"

using std::map;
using std::string;
using std::list;
using std::mutex;

/*****************************************************************************
-Class			: RtmpServerManager
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpServerManager : public TcpServer
{
public:
	RtmpServerManager(int i_iServerPort);
	virtual ~RtmpServerManager();
    int Proc();
    
private:
    int CheckMapServerIO();
    int AddMapServerIO(RtmpServerIO * i_pRtmpServerIO,int i_iClientSocketFd);
    
    map<int, RtmpServerIO *>  m_RtmpServerIOMap;
    mutex m_MapMtx;
};

#endif
