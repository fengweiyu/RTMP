/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpClientManager.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef RTMP_CLIENT_MANAGER_H
#define RTMP_CLIENT_MANAGER_H

#include <mutex>
#include <string>
#include <list>
#include <map>
#include "RtmpClientIO.h"

using std::map;
using std::string;
using std::list;
using std::mutex;

/*****************************************************************************
-Class			: Pushing
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpClientManager
{
public:
	RtmpClientManager();
	virtual ~RtmpClientManager();
    int Proc(char *i_strURL,char *i_strOutName=NULL);//����
    int MediaProc();
    
private:
    static int HandlePlayData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);
    
    int Pushing(T_MediaFrameInfo * i_pFrame);
    int HandlePlayMediaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen);


	int m_iProcFlag;
	int m_iMediaProcFlag;
    MediaHandle *m_pMediaHandle;
    MediaHandle *m_pHandlePushMediaHandle;
    string * m_pFileName;
    
    FILE  *m_pMediaFile;
    FILE  *m_pMediaFile2;
    unsigned char *m_pbFileBuf;
    RtmpClientIO * m_pRtmpClientIO;
    string * m_pOutFileName;
};

#endif
