/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpServerIO.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef RTMP_SERVER_IO_H
#define RTMP_SERVER_IO_H

#include "TcpSocket.h"
#include "RtmpServer.h"
#include "MediaHandle.h"
#include <thread>
#include <mutex>

using std::thread;
using std::mutex;



#define  RTMPS_LOGW2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPS_LOGE2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPS_LOGD2(val,fmt,...)      printf("<%d>:"fmt,val,##__VA_ARGS__)
#define  RTMPS_LOGW(...)     printf(__VA_ARGS__)
#define  RTMPS_LOGE(...)     printf(__VA_ARGS__)
#define  RTMPS_LOGD(...)     printf(__VA_ARGS__)
#define  RTMPS_LOGI(...)     printf(__VA_ARGS__)


/*****************************************************************************
-Class			: RtmpServerIO
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpServerIO : public TcpServer
{
public:
	RtmpServerIO(int i_iClientSocketFd);
	virtual ~RtmpServerIO();
    int Proc();
    int SendDatas(char * i_acSendBuf,int i_iSendLen);
    int GetProcFlag();
    int StopAllProc();
    int HandlePlayURL(const char * url);
    int HandlePushURL(const char * url);
    int HandlePushMediaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen);
        
private:
    int MediaProc();
    int Playing(T_MediaFrameInfo * i_pFrame);

    static int StartPlay(char *i_strPalySrc,void *i_pIoHandle);
    static int StopPlay(char *i_strPalySrc);//,void *i_pIoHandle
    static int SendData(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen);
    static int StartHandlePushStream(char *i_strStreamSrc,void *i_pIoHandle);
    static int StopHandlePushStream(char *i_strStreamSrc,void *i_pIoHandle);
    static int HandlePushVideoData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);
    static int HandlePushAudioData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);
    static int HandlePushScriptData(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen);
    static long GetRandom();

                    

    RtmpServer * m_pRtmpServer;
	int m_iClientSocketFd;

    thread * m_pRtmpServerIOProc;
	int m_iRtmpServerIOFlag;
    MediaHandle *m_pMediaHandle;
    MediaHandle *m_pHandlePushMediaHandle;
    thread * m_pMediaProc;
	int m_iMediaProcFlag;

    string * m_pFileName;
    string * m_pPushFileName;
    
    FILE  *m_pMediaFile;
    unsigned char *m_pbFileBuf;
};

#endif
