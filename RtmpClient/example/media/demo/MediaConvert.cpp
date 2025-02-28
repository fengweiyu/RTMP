/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       MediaConvert.cpp
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
#include <string>
#include "MediaConvert.h"

#define MEDIA_FORMAT_MAX_LEN	(10*1024) 
#define MEDIA_OUTPUT_BUF_MAX_LEN	(2*1024*1024) 
#define MEDIA_INPUT_BUF_MAX_LEN	(6*1024*1024) 

MediaConvert * MediaConvert::m_pInstance = new MediaConvert();//一般使用饿汉模式,懒汉模式线程不安全
/*****************************************************************************
-Fuction        : MediaConvert
-Description    : MediaConvert
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
MediaConvert:: MediaConvert()
{
    m_pDataBufList.clear();
    m_eDstVideoEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_eDstAudioEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_pbInputBuf = new DataBuf(MEDIA_INPUT_BUF_MAX_LEN);
    m_pAudioCodec = NULL;
    m_pAudioCodec = new AudioCodec();
    m_pAudioTranscodeBuf = new unsigned char [MEDIA_FORMAT_MAX_LEN];
}
/*****************************************************************************
-Fuction        : MediaConvert
-Description    : MediaConvert
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
MediaConvert:: ~MediaConvert()
{
    if(!m_pDataBufList.empty())
    {
        DataBuf * it = m_pDataBufList.front();
        delete it;
        m_pDataBufList.pop_front();
    }
    delete m_pbInputBuf;
    if(NULL != m_pAudioCodec)
    {
        delete m_pAudioCodec;
    }
    if(NULL != m_pAudioTranscodeBuf)
    {
        delete [] m_pAudioTranscodeBuf;
    }
}
/*****************************************************************************
-Fuction		: Instance
-Description	: Instance
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
MediaConvert * MediaConvert::Instance()
{
	return m_pInstance;
}

/*****************************************************************************
-Fuction        : Convert
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ConvertFromPri(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_StreamType i_eDstStreamType)
{
    T_MediaFrameInfo tFileFrameInfo;
	DataBuf * pbOutBuf =NULL;
	int iRet = -1,iWriteLen=0;
	int iHeaderLen=0;
	int iPutFrameLen=0;

#ifdef SUPPORT_PRI
    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    m_streamer.Inputdata(i_pbSrcData,i_iSrcDataLen);
    while(1)
    {
        NSXPTL::FRAME_INFO* pFrame = m_streamer.GetNextFrameNoSafe();
        if(pFrame == NULL)
        {
            printf("pFrame == NULL [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,pFrame->nLength);
            break;
        } 
        memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
        iRet=PriToMedia(pFrame,&tFileFrameInfo);
        if(iRet<0)
        {
            printf("PriToMedia err%d\r\n",pFrame->nEncodeType);
            continue;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_eDstVideoEncType = tFileFrameInfo.eEncType;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
        {
            if(i_eDstStreamType == STREAM_TYPE_FMP4_STREAM)
            {//mse(mp4)才需要转换
                if(AudioTranscode(&tFileFrameInfo)<=0)
                {
                    continue;//转换失败或数据不够，则跳过这一帧
                }
            }
            m_eDstAudioEncType = tFileFrameInfo.eEncType;
        }

        iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {
            pbOutBuf = new DataBuf(iPutFrameLen+MEDIA_FORMAT_MAX_LEN);
        }
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            iPutFrameLen=0;
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//没有获取到封装数据(输入数据不够)
                delete pbOutBuf;//但是下次输出数据会更大，获取封装数据需要更大的缓存
                pbOutBuf = NULL;//所以需要释放，重新分配
            }//如果每次都传入一个足够大的缓存，则不需要这么麻烦
            continue;
        }
        printf("FrameToContainer iPutFrameLen %d iWriteLen %d iFrameProcessedLen[%d]\r\n",iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//没用到的封装缓存长度
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//总数据-被打包的数据长度和封装长度=未打包数据长度+未用封装缓存长度再减iRemain=未打包数据长度
        iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLen要等于封装对象中缓存的未打包的数据长度，这样内存才够。
    }
    m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);//
    if(NULL != pbOutBuf)
    {
        delete pbOutBuf;
    }
#endif    
    return iWriteLen;
}

/*****************************************************************************
-Fuction        : Convert
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert::Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType)
{
    T_MediaFrameInfo tFileFrameInfo;
	DataBuf * pbOutBuf =NULL;
	int iRet = -1,iWriteLen=0;
	int iHeaderLen=0;
	int iPutFrameLen=0;

    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    m_pbInputBuf->Copy(i_pbSrcData,i_iSrcDataLen);
    memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf;
    tFileFrameInfo.eStreamType = i_eSrcStreamType;
    tFileFrameInfo.eEncType = i_eSrcEncType;
    tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen;
    tFileFrameInfo.iFrameBufMaxLen = m_pbInputBuf->iBufMaxLen;
    while(1)
    {
        tFileFrameInfo.iFrameLen = 0;
        tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf+tFileFrameInfo.iFrameProcessedLen;
        tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen-tFileFrameInfo.iFrameProcessedLen;
        tFileFrameInfo.iFrameBufMaxLen = m_pbInputBuf->iBufMaxLen-tFileFrameInfo.iFrameProcessedLen;
        if(STREAM_TYPE_VIDEO_STREAM==i_eSrcStreamType)
        {//非文件流使用m_oMediaHandle则要保证数据只有一帧
            tFileFrameInfo.pbFrameStartPos = NULL;//因此干脆直接使用ParseNaluFromFrame
            tFileFrameInfo.eStreamType = STREAM_TYPE_UNKNOW;//ParseNaluFromFrame 赋值需要先设置为MEDIA_ENCODE_TYPE_UNKNOW
            ParseNaluFromFrame(&tFileFrameInfo);
            tFileFrameInfo.eStreamType = i_eSrcStreamType;
        }
        else//GetFrame 内aac已经改为使用eFrameType判断外部是否已经完善了一帧的信息
        {//aac的eFrameType为UNKNOW，则可以使用GetFrame解析,内部会进行赋值
            m_oMediaHandle.GetFrame(&tFileFrameInfo);//后续视频裸流也改为使用eFrameType判断，而不是eStreamType，则也可以按照aac的方式使用GetFrame
        }
        if(tFileFrameInfo.iFrameLen <= 0)
        {
            printf("tFileFrameInfo.iFrameLen <= 0 [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,tFileFrameInfo.iFrameProcessedLen);
            break;
        } 
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_eDstVideoEncType = tFileFrameInfo.eEncType;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
        {
            if(i_eDstStreamType == STREAM_TYPE_FMP4_STREAM)
            {//mse(mp4)才需要转换
                if(AudioTranscode(&tFileFrameInfo)<=0)
                {
                    continue;//转换失败或数据不够，则跳过这一帧
                }
            }
            m_eDstAudioEncType = tFileFrameInfo.eEncType;
        }

#ifdef SUPPORT_PRI
        if(STREAM_TYPE_UNKNOW == i_eDstStreamType)
        {
            FRAME_INFO* ptFrameInfo = MediaToPri(&tFileFrameInfo);
            if(NULL == ptFrameInfo || ptFrameInfo->nLength <= 0)
            {
                printf("MediaToPri err %d %d\r\n",tFileFrameInfo.iFrameLen,ptFrameInfo->nLength);
                continue;
            }
            if(NULL == pbOutBuf)
            {
                pbOutBuf = new DataBuf(ptFrameInfo->nLength);
            }
            pbOutBuf->Copy(ptFrameInfo->pHeader,ptFrameInfo->nLength);
            m_pDataBufList.push_back(pbOutBuf);
            pbOutBuf = NULL;
            ptFrameInfo->Release();
            continue;
        }
#endif
        iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {//如果每次都传入一个足够大的缓存，则不需要统计 iPutFrameLen这么麻烦
            pbOutBuf = new DataBuf(iPutFrameLen+MEDIA_FORMAT_MAX_LEN);//但是每次(帧)都这么大的缓存，如果不能及时释放则内存容易耗尽
        }
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            iPutFrameLen=0;
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//没有获取到封装数据(输入数据不够)
                delete pbOutBuf;//但是下次输出数据会更大，获取封装数据需要更大的缓存
                pbOutBuf = NULL;//所以需要释放，重新分配
            }//如果每次都传入一个足够大的缓存，则不需要这么麻烦
            continue;
        }
        printf("FrameToContainer iPutFrameLen %d iWriteLen %d iFrameProcessedLen[%d]\r\n",iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//没用到的封装缓存长度
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//总数据-被打包的数据长度和封装长度=未打包数据长度+未用封装缓存长度再减iRemain=未打包数据长度
        iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLen要等于封装对象中缓存的未打包的数据长度，这样内存才够。
    }//gop类打包(mp4)，最新的i帧不会被打包是下次打包，最新的帧长度= 封装对象中缓存的未打包的数据长度
    m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);
    if(NULL != pbOutBuf)
    {
        delete pbOutBuf;
    }
    
    return iWriteLen;
}

/*****************************************************************************
-Fuction		: GetData
-Description	: GetData
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::GetData(unsigned char * o_pbData,int i_iMaxDataLen)
{
    int iRet = -1;
    
    if(NULL == o_pbData || i_iMaxDataLen<= 0)
    {
        printf("GetData NULL %d\r\n",i_iMaxDataLen);
        return iRet;
    }
    
    if(!m_pDataBufList.empty())
    {
        DataBuf * it = m_pDataBufList.front();
        if(it->iBufLen > i_iMaxDataLen)
        {
            printf("GetData err %d,%d\r\n",it->iBufLen, i_iMaxDataLen);
        }
        else
        {
            memcpy(o_pbData,it->pbBuf,it->iBufLen);
            iRet =it->iBufLen;
            delete it;
            m_pDataBufList.pop_front();
        }
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: GetData
-Description	: GetData
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen)
{
    int iRet = -1;
    
    if(NULL != o_pbVideoEncBuf && i_iMaxVideoEncBufLen>4)
    {
        switch(m_eDstVideoEncType)
        {
            case MEDIA_ENCODE_TYPE_H264:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","h264");
                iRet=0;
                break;
            }
            case MEDIA_ENCODE_TYPE_H265:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","h265");
                iRet=0;
                break;
            }
            default:
            {
                printf("m_eDstVideoEncType err %d %d\r\n",i_iMaxVideoEncBufLen,m_eDstVideoEncType);
                iRet=-1;
                break;
            }
        }
    }

    if(NULL != o_pbAudioEncBuf && i_iMaxAudioEncBufLen>4)
    {
        switch(m_eDstAudioEncType)
        {
            case MEDIA_ENCODE_TYPE_AAC:
            {
                snprintf((char *)o_pbAudioEncBuf,i_iMaxAudioEncBufLen,"%s","aac");
                iRet=0;
                break;
            }
            case MEDIA_ENCODE_TYPE_G711A:
            {
                snprintf((char *)o_pbAudioEncBuf,i_iMaxAudioEncBufLen,"%s","g711a");
                iRet=0;
                break;
            }
            default:
            {
                printf("m_eDstAudioEncType err %d %d\r\n",i_iMaxAudioEncBufLen,m_eDstAudioEncType);
                iRet=-1;
                break;
            }
        }
    }
    if(iRet<0)
    {
        printf("GetEncodeType err %d %d\r\n",i_iMaxVideoEncBufLen,i_iMaxAudioEncBufLen);
        return iRet;
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: AudioTranscode
-Description	: g711转aac(mse只支持aac)
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::AudioTranscode(T_MediaFrameInfo * m_pbAudioFrame)
{
    int iRet = -1;
    T_AudioCodecParam i_tSrcCodecParam;
    T_AudioCodecParam i_tDstCodecParam;
    E_AudioCodecType eSrcAudioCodecType=AUDIO_CODEC_TYPE_UNKNOW;
    int iWriteLen = 0;


    if(NULL == m_pbAudioFrame || m_pbAudioFrame->iFrameLen<= 0)
    {
        printf("AudioTranscode NULL %p\r\n",m_pbAudioFrame);
        return iRet;
    }
    switch(m_pbAudioFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_G711A:
        {
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_PCMA;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_PCMU;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            printf("already acc,no need convert %d\r\n",m_pbAudioFrame->iFrameLen);
            return m_pbAudioFrame->iFrameLen;
        }
        default :
        {
            printf("AudioTranscode eEncType err %d\r\n",m_pbAudioFrame->eEncType);
            return iRet;
        }
    }
    memset(&i_tSrcCodecParam,0,sizeof(T_AudioCodecParam));
    i_tSrcCodecParam.dwBitsPerSample=m_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample;
    i_tSrcCodecParam.dwChannels=m_pbAudioFrame->tAudioEncodeParam.dwChannels;
    i_tSrcCodecParam.dwSampleRate=m_pbAudioFrame->dwSampleRate;
    i_tSrcCodecParam.eAudioCodecType=eSrcAudioCodecType;
    memcpy(&i_tDstCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    i_tDstCodecParam.eAudioCodecType=AUDIO_CODEC_TYPE_AAC;
    //i_tDstCodecParam.dwSampleRate=44100;

    iRet = m_pAudioCodec->Transcode(m_pbAudioFrame->pbFrameStartPos,m_pbAudioFrame->iFrameLen,i_tSrcCodecParam,m_pAudioTranscodeBuf,MEDIA_FORMAT_MAX_LEN,i_tDstCodecParam);
    if(iRet <= 0)
    {
        printf("Transcode iRet %d iFrameLen%d\r\n",iRet,m_pbAudioFrame->iFrameLen);
        return iRet;
    } 
    iWriteLen=iRet;
    iRet=0;
    do
    {
        iWriteLen+=iRet;
        iRet=m_pAudioCodec->Transcode(NULL,0,i_tSrcCodecParam,m_pAudioTranscodeBuf+iWriteLen,MEDIA_FORMAT_MAX_LEN-iWriteLen,i_tDstCodecParam);
    } while(iRet>0);


    switch(i_tDstCodecParam.eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_AAC:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMU:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711U;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMA:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        default :
        {
            printf("AudioTranscode dst eEncType err %d\r\n",i_tDstCodecParam.eAudioCodecType);
            return iRet;
        }
    }
    m_pbAudioFrame->dwSampleRate=i_tDstCodecParam.dwSampleRate;
    m_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample=i_tDstCodecParam.dwBitsPerSample;
    m_pbAudioFrame->tAudioEncodeParam.dwChannels=i_tDstCodecParam.dwChannels;
    m_pbAudioFrame->pbFrameBuf=m_pAudioTranscodeBuf;//
    m_pbAudioFrame->iFrameBufLen=iWriteLen;//
    m_pbAudioFrame->iFrameBufMaxLen=MEDIA_FORMAT_MAX_LEN;//
    m_pbAudioFrame->iFrameLen=iWriteLen;
    m_pbAudioFrame->pbFrameStartPos=m_pAudioTranscodeBuf;
    
    return iWriteLen;
}

#ifdef SUPPORT_PRI
/*****************************************************************************
-Fuction        : WebRtcFrameToFrameInfo
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
FRAME_INFO* MediaConvert::MediaToPri(T_MediaFrameInfo * i_ptMediaFrame) 
{
	FRAME_INFO* ptFrameInfo = new FRAME_INFO();

    switch (i_ptMediaFrame->eFrameType)
    {
        case MEDIA_FRAME_TYPE_VIDEO_I_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_P_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_B_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_B_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_AUDIO_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_AUDIO;
            break;
        }
        default:
            break;
    }
    switch (i_ptMediaFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_H264:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H264;
            break;
        }
        case MEDIA_ENCODE_TYPE_H265:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H265;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_AAC;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711A:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711A;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711U;
            break;
        }
        default:
        {
            printf("ptFrameInfo->eEncType err%d\r\n",i_ptMediaFrame->eEncType);
            delete ptFrameInfo;
            return NULL;
        }
    }
	ptFrameInfo->nSamplesPerSecond = i_ptMediaFrame->dwSampleRate;
	ptFrameInfo->nTimeStamp = i_ptMediaFrame->dwTimeStamp;
	ptFrameInfo->nCumulativeTimeMS = i_ptMediaFrame->dwTimeStamp;
	ptFrameInfo->nFrameLength = i_ptMediaFrame->iFrameLen;
	ptFrameInfo->nChannels = i_ptMediaFrame->tAudioEncodeParam.dwChannels;
	//if(0 != m_iPullVideoFrameRate ||0!=m_dwTalkVideoFPS)
    	//ptFrameInfo->nFrameRate = m_iPullVideoFrameRate==0?m_dwTalkVideoFPS:m_iPullVideoFrameRate;//如果前端没有传递帧率则使用json里面的告诉转码库要转成多少帧率
	ptFrameInfo->nWidth = i_ptMediaFrame->dwWidth;//宽高不赋值，宽高不一致就会触发转码
	ptFrameInfo->nHeight = i_ptMediaFrame->dwHeight;//h265 pc浏览器 b帧；手机浏览器帧切片，都需要转码

    FRAME_INFO* pFrame = CSTDStream::NewFrame(ptFrameInfo, (const char*)i_ptMediaFrame->pbFrameStartPos, i_ptMediaFrame->iFrameLen);
    pFrame->AddRef();
    delete ptFrameInfo;
	return pFrame;
	return NULL;
}

/*****************************************************************************
-Fuction        : GenerateFlvData
-Description    : 
-Input          : 
-Output         : 
-Return         : len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::PriToMedia(FRAME_INFO * i_pFrame,T_MediaFrameInfo *o_ptMediaFrameInfo)
{
    int iRet=-1;
    E_MediaEncodeType eEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    E_MediaFrameType eFrameType=MEDIA_FRAME_TYPE_UNKNOW;
    E_StreamType eStreamType=STREAM_TYPE_FLV_STREAM;
    
    if(NULL == i_pFrame)
    {
        printf("PriToMedia err NULL\r\n");
        return iRet;
    }
    switch(i_pFrame->nEncodeType)
    {
        case ENCODE_VIDEO_H264 :
        {
            eEncType = MEDIA_ENCODE_TYPE_H264;
            break;
        }
        case ENCODE_VIDEO_H265 :
        {
            eEncType = MEDIA_ENCODE_TYPE_H265;
            break;
        }
        case ENCODE_AUDIO_AAC :
        {
            eEncType = MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case ENCODE_AUDIO_G711A :
        {
            eEncType = MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        default :
        {
            printf("PriToMedia i_ptFrame->eEncType err %d\r\n",i_pFrame->nEncodeType);
            return iRet;
        }
    }
    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        switch (i_pFrame->nSubType)
        {
            case FRAME_TYPE_VIDEO_I_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
                break;
            }
            case FRAME_TYPE_VIDEO_P_FRAME:
            case FRAME_TYPE_VIDEO_S_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
                break;
            }
            case FRAME_TYPE_VIDEO_B_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_B_FRAME;
                break;
            }
            default:
            {
                printf("PriToMedia i_ptFrame->eFrameType err %d\r\n",i_pFrame->nSubType);
                return iRet;
            }
        }
    }
    else if (i_pFrame->nType == FRAME_TYPE_AUDIO)
    {
        eFrameType = MEDIA_FRAME_TYPE_AUDIO_FRAME;
    }
    else
    {
        printf("PriToMedia i_ptFrame->nType err%d\r\n",i_pFrame->nType);
        return iRet;
    }

    o_ptMediaFrameInfo->pbFrameBuf = i_pFrame->pContent;
    o_ptMediaFrameInfo->iFrameBufMaxLen= i_pFrame->nFrameLength;
    o_ptMediaFrameInfo->iFrameBufLen = i_pFrame->nFrameLength;
    o_ptMediaFrameInfo->eStreamType= STREAM_TYPE_MUX_STREAM;//视频流需要解析成多个nalu，音频数据可直接外部输入

    o_ptMediaFrameInfo->eEncType = eEncType;
    o_ptMediaFrameInfo->eFrameType = eFrameType;

    o_ptMediaFrameInfo->dwTimeStamp=i_pFrame->nTimeStamp;
    o_ptMediaFrameInfo->dwWidth= i_pFrame->nWidth;
    o_ptMediaFrameInfo->dwHeight=i_pFrame->nHeight;

    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        o_ptMediaFrameInfo->dwSampleRate= 90000;//h264频率不会传过来，h264频率可以固定
        iRet=ParseNaluFromFrame(o_ptMediaFrameInfo);
        if(iRet<0)
        {
            printf("PriToMedia ParseNaluFromFrame err %d\r\n",i_pFrame->nEncodeType);
            return iRet;
        }
    }
    else if (i_pFrame->nType == FRAME_TYPE_AUDIO)
    {
        o_ptMediaFrameInfo->dwSampleRate= i_pFrame->nSamplesPerSecond;
        o_ptMediaFrameInfo->tAudioEncodeParam.dwChannels= i_pFrame->nChannels;
        o_ptMediaFrameInfo->tAudioEncodeParam.dwBitsPerSample= i_pFrame->nBitsPerSample;
        o_ptMediaFrameInfo->pbFrameStartPos = i_pFrame->pContent;
        o_ptMediaFrameInfo->iFrameLen= i_pFrame->nFrameLength;
    }
    return 0;
}
#endif
/*****************************************************************************
-Fuction        : ParseNaluFromFrame
-Description    : ParseNaluFromFrame
-Input          : 
-Output         : 
-Return         : iRet must be 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseNaluFromFrame(T_MediaFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    
    if(NULL == o_ptFrameInfo->pbFrameBuf || MEDIA_ENCODE_TYPE_UNKNOW == o_ptFrameInfo->eEncType  ||o_ptFrameInfo->iFrameBufLen <= 4)
    {
        printf("ParseNaluFromFrame NULL %d\r\n", o_ptFrameInfo->iFrameBufLen);
        return iRet;
    }
    if(MEDIA_ENCODE_TYPE_H264 == o_ptFrameInfo->eEncType)
        iRet = ParseH264NaluFromFrame(o_ptFrameInfo);
    else if(MEDIA_ENCODE_TYPE_H265 == o_ptFrameInfo->eEncType)
        iRet = ParseH265NaluFromFrame(o_ptFrameInfo);

    return iRet;
}

/*****************************************************************************
-Fuction        : ParseNaluFromFrame
-Description    : ParseNaluFromFrame
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseH264NaluFromFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    unsigned char bStartCodeLen = 0;
    int iFrameType = MEDIA_FRAME_TYPE_UNKNOW;


    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf ||m_ptFrame->iFrameBufLen <= 4)
    {
        printf("GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
        return iRet;
    }
	
	pcFrameData = m_ptFrame->pbFrameBuf;
	iRemainDataLen = m_ptFrame->iFrameBufLen;
    m_ptFrame->dwNaluCount = 0;
    m_ptFrame->iFrameLen = 0;
    while(iRemainDataLen > 0)
    {
        if (iRemainDataLen >= 3 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;//此时是一个nalu的结束
            }
            else
            {
                pcNaluStartPos = pcFrameData;//此时是一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//上一个nalu的结束为下一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 3;
            iRemainDataLen -= 3;
        }
        else if (iRemainDataLen >= 4 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 0 && pcFrameData[3] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;
            }
            else
            {
                pcNaluStartPos = pcFrameData;
                bStartCodeLen = 4;
                bNaluType = pcNaluStartPos[4] & 0x1f;
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = pcNaluStartPos[4] & 0x1f;
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 4;
            iRemainDataLen -= 4;
        }
        else
        {
            pcFrameData ++;
            iRemainDataLen --;
        }
    }
    if(pcNaluStartPos != NULL && (iFrameType<=0 ||STREAM_TYPE_UNKNOW != m_ptFrame->eStreamType))
    {//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
        iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcFrameData - pcNaluStartPos,m_ptFrame);
        if(iRet <= 0 && iFrameType<=0)
        {
            printf("SetH264NaluData err %d %d\r\n", m_ptFrame->dwNaluCount,m_ptFrame->iFrameLen);
            return iRet;
        }
        if(iRet>0)
            iFrameType = iRet;
    }
    if(iFrameType>0)
        iRet=0;
	if(NULL != m_ptFrame->pbFrameStartPos)
	{
        m_ptFrame->iFrameProcessedLen += m_ptFrame->pbFrameStartPos - m_ptFrame->pbFrameBuf + m_ptFrame->iFrameLen;
	}
    return iRet;
}

/*****************************************************************************
-Fuction        : GetFrame
-Description    : GetFrame
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseH265NaluFromFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    unsigned char bStartCodeLen = 0;
    int iFrameType = MEDIA_FRAME_TYPE_UNKNOW;


    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf ||m_ptFrame->iFrameBufLen <= 4)
    {
        printf("GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
        return iRet;
    }
	
	pcFrameData = m_ptFrame->pbFrameBuf;
	iRemainDataLen = m_ptFrame->iFrameBufLen;
    m_ptFrame->dwNaluCount = 0;
    m_ptFrame->iFrameLen = 0;
    while(iRemainDataLen > 0)
    {
        if (iRemainDataLen >= 3 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;//此时是一个nalu的结束
            }
            else
            {
                pcNaluStartPos = pcFrameData;//此时是一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//取nalu类型
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//上一个nalu的结束为下一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//取nalu类型
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 3;
            iRemainDataLen -= 3;
        }
        else if (iRemainDataLen >= 4 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 0 && pcFrameData[3] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;
            }
            else
            {
                pcNaluStartPos = pcFrameData;
                bStartCodeLen = 4;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet = SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);//包括类型减4//去掉00 00 00 01
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 4;
            iRemainDataLen -= 4;
        }
        else
        {
            pcFrameData ++;
            iRemainDataLen --;
        }
    }
    if(pcNaluStartPos != NULL && (iFrameType<=0 ||STREAM_TYPE_UNKNOW != m_ptFrame->eStreamType))
    {//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
        iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcFrameData - pcNaluStartPos,m_ptFrame);//包括类型减4开始码
        if(iRet <= 0 && iFrameType<=0)
        {
            printf("SetH265NaluData err bNaluType%d bStartCodeLen%d dwNaluCount%d iFrameLen%d\r\n", bNaluType,bStartCodeLen,m_ptFrame->dwNaluCount,m_ptFrame->iFrameLen);
            return iRet;
        }
        if(iRet>0)
            iFrameType = iRet;
    }
    if(iFrameType>0)
        iRet=0;
	if(NULL != m_ptFrame->pbFrameStartPos)
	{
        m_ptFrame->iFrameProcessedLen += m_ptFrame->pbFrameStartPos - m_ptFrame->pbFrameBuf + m_ptFrame->iFrameLen;
	}
    return iRet;
}


/*****************************************************************************
-Fuction        : SetH264NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : eFrameType
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::SetH264NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char * pbNaluData = NULL;//去掉00 00 00 01
    int iNaluDataLen=0;
    E_MediaFrameType eFrameType = MEDIA_FRAME_TYPE_UNKNOW;
    
    if(NULL == i_pbNaluData || NULL == m_ptFrame)
    {
        printf("SetH264NaluData NULL %d \r\n", i_iNaluDataLen);
        return iRet;
    }
    
    if(m_ptFrame->pbFrameStartPos == NULL)
    {
        m_ptFrame->pbFrameStartPos = i_pbNaluData;
    }
    m_ptFrame->iFrameLen += i_iNaluDataLen;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].pbData= i_pbNaluData;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].dwDataLen= i_iNaluDataLen;
    m_ptFrame->dwNaluCount++;

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//包括类型减开始码
    pbNaluData = i_pbNaluData+i_bStartCodeLen;
    switch(i_bNaluType)//取nalu类型
    {
        case 0x7:
        {
            memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//包括类型(减3开始码)
            memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
            break;
        }
        case 0x8:
        {
            memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfPPS = iNaluDataLen;//包括类型减3开始码
            memcpy(m_ptFrame->tVideoEncodeParam.abPPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfPPS);
            break;
        }
        case 0x1:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case 0x5:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        default:
        {
            break;
        }
    }

    if(MEDIA_FRAME_TYPE_UNKNOW != eFrameType)
    {
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//文件的时候才需要赋值，数据流的时候外部会赋值以外部为准
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H264;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//解析出一帧则退出
    }
    return eFrameType;
}
/*****************************************************************************
-Fuction        : SetH265NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : eFrameType
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::SetH265NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char * pbNaluData = NULL;//去掉00 00 00 01
    int iNaluDataLen=0;
    E_MediaFrameType eFrameType = MEDIA_FRAME_TYPE_UNKNOW;
    
    if(NULL == i_pbNaluData || NULL == m_ptFrame)
    {
        printf("SetH265NaluData NULL %d \r\n", iRet);
        return iRet;
    }
    
    if(m_ptFrame->pbFrameStartPos == NULL)
    {
        m_ptFrame->pbFrameStartPos = i_pbNaluData;
    }
    m_ptFrame->iFrameLen += i_iNaluDataLen;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].pbData= i_pbNaluData;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].dwDataLen= i_iNaluDataLen;
    m_ptFrame->dwNaluCount++;

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//包括类型减开始码
    pbNaluData = i_pbNaluData+i_bStartCodeLen;

    if(i_bNaluType >= 0 && i_bNaluType <= 9)// p slice 片
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
    }
    else if(i_bNaluType >= 16 && i_bNaluType <= 21)// IRAP 等同于i帧
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
    }
    else if(i_bNaluType == 32)//VPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abVPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abVPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfVPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abVPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfVPS);
    }
    else if(i_bNaluType == 33)//SPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
    }
    else if(i_bNaluType == 34)//PPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfPPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abPPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfPPS);
    }
    
    if(MEDIA_FRAME_TYPE_UNKNOW != eFrameType)
    {
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//文件的时候才需要赋值，数据流的时候外部会赋值以外部为准
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H265;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//解析出一帧则退出
    }
    return eFrameType;
}


/*****************************************************************************
-Fuction        : InputData
-Description    : InputData
-Input          : 输入的数据类型 ,输出的数据类型
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int InputData(unsigned char * i_pbSrcData,int i_iSrcDataLen,const char *i_strSrcName,const char *i_strDstName)
{
    E_StreamType eSrcStreamType=STREAM_TYPE_UNKNOW;
    E_StreamType eDstStreamType=STREAM_TYPE_UNKNOW;
    E_MediaEncodeType eSrcEncType=MEDIA_ENCODE_TYPE_UNKNOW;

    if(NULL != strstr(i_strDstName,".ts"))
    {
        eDstStreamType=STREAM_TYPE_TS_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".mp4"))
    {
        eDstStreamType=STREAM_TYPE_FMP4_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".flv"))
    {
        eDstStreamType=STREAM_TYPE_ENHANCED_FLV_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".VideoRaw"))
    {
        eDstStreamType=STREAM_TYPE_VIDEO_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".AudioRaw"))
    {
        eDstStreamType=STREAM_TYPE_AUDIO_STREAM;
    }
#ifdef SUPPORT_PRI
    else if(NULL != strstr(i_strDstName,".pri"))
    {
        eDstStreamType=STREAM_TYPE_UNKNOW;
    }
#endif    
    else
    {
        printf("i_strDstName %s err\r\n",i_strDstName);
        return -1;
    }
    if(NULL != strstr(i_strSrcName,".pri"))
    {
#ifdef SUPPORT_PRI
        printf("Convert %s to %s\r\n",i_strSrcName,i_strDstName);
        return MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#else
        printf("Convert %s to %s unsupport\r\n",i_strSrcName,i_strDstName);
        return -1;//MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#endif    
    }

    if(NULL != strstr(i_strSrcName,".flv"))
    {
        eSrcStreamType=STREAM_TYPE_FLV_STREAM;
    }
    else if(NULL != strstr(i_strSrcName,".h264"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H264;
    }
    else if(NULL != strstr(i_strSrcName,".h265"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H265;
    }
    else if(NULL != strstr(i_strSrcName,".aac"))
    {
        eSrcStreamType=STREAM_TYPE_AUDIO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_AAC;
    }
    else
    {
        printf("i_strSrcName %s err\r\n",i_strSrcName);
        return -1;
    }
    printf("Convert %s to %s\r\n",i_strSrcName,i_strDstName);
    return MediaConvert::Instance()->Convert(i_pbSrcData,i_iSrcDataLen,eSrcEncType,eSrcStreamType,eDstStreamType);
}


/*****************************************************************************
-Fuction        : GetData
-Description    : 一次返回输入数据的最小单位，
比如裸流就返回一帧
flv返回一帧对应的tag
mp4返回一个gop
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int GetData(unsigned char * o_pbData,int i_iMaxDataLen)
{
    return MediaConvert::Instance()->GetData(o_pbData,i_iMaxDataLen);
}


/*****************************************************************************
-Fuction        : GetData
-Description    : 一次返回输入数据的最小单位，
比如裸流就返回一帧
flv返回一帧对应的tag
mp4返回一个gop
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen)
{
    return MediaConvert::Instance()->GetEncodeType(o_pbVideoEncBuf,i_iMaxVideoEncBufLen,o_pbAudioEncBuf,i_iMaxAudioEncBufLen);
}

