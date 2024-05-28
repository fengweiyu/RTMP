/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpClientIO.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef RTMP_CLIENT_IO_H
#define RTMP_CLIENT_IO_H

#include "TcpSocket.h"
#include "RtmpClient.h"
#include "MediaHandle.h"
#include <thread>
#include <mutex>

using std::thread;
using std::mutex;



#define  RTMPC_LOGW2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPC_LOGE2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPC_LOGD2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPC_LOGW(...)     printf(__VA_ARGS__)
#define  RTMPC_LOGE(...)     printf(__VA_ARGS__)
#define  RTMPC_LOGD(...)     printf(__VA_ARGS__)
#define  RTMPC_LOGI(...)     printf(__VA_ARGS__)

typedef struct RtmpClientCb
{
    int (*PlayVideoData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//Annex-B格式裸流带00 00 00 01
    int (*PlayAudioData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//aac带7字节头
    int (*PlayScriptData)(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen);
}T_RtmpClientCb;


/*****************************************************************************
-Class			: RtmpClientIO
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpClientIO : TcpClient
{
public:
	RtmpClientIO();
    RtmpClientIO(char *i_strURL,T_RtmpClientCb i_tRtmpClientCb);
	virtual ~RtmpClientIO();
	int Start(char *i_strURL,T_RtmpClientCb i_tRtmpClientCb);
    int Stop(int err);
    int Proc();
    int StopAllProc();
    int GetProcFlag();
    int Pushing(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath);
        
private:
    static int ConnectServer(void *i_pIoHandle,char * i_strIP,unsigned short i_wPort);
    static int SendData(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen);
    static long GetRandom();

                    

    RtmpClient m_RtmpClient;
	int m_iClientSocketFd;

    thread * m_pRtmpClientIOProc;
	int m_iRtmpClientIOFlag;
};

#endif
