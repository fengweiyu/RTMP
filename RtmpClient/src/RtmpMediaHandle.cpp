/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpMediaHandle.cpp
* Description       :   RtmpMediaHandle operation center
                        RTMP媒体数据处理，包括音视频数据打包
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "RtmpMediaHandle.h"


#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define BIT(ptr, off) (((ptr)[(off) / 8] >> (7 - ((off) % 8))) & 0x01)






/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpMediaHandle::RtmpMediaHandle()
{




}

/*****************************************************************************
-Fuction        : ~RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpMediaHandle::~RtmpMediaHandle()
{



}

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
int RtmpMediaHandle::ParseNaluFromFrame(E_RTMP_ENC_TYPE i_eEncType,unsigned char *i_pbVideoData,int i_iVideoDataLen,T_RtmpFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    
    if(NULL == i_pbVideoData || NULL == o_ptFrameInfo || RTMP_UNKNOW_ENC_TYPE == i_eEncType || NULL == o_ptFrameInfo->pbNaluData ||i_iVideoDataLen <= 4)
    {
        RTMP_LOGE("ParseNaluFromFrame NULL %d\r\n", i_iVideoDataLen);
        return iRet;
    }
    o_ptFrameInfo->eEncType = i_eEncType;
    if(RTMP_ENC_H264 == i_eEncType)
        iRet = ParseH264NaluFromFrame(i_pbVideoData,i_iVideoDataLen,o_ptFrameInfo);
    else if(RTMP_ENC_H265 == i_eEncType)
        iRet = ParseH265NaluFromFrame(i_pbVideoData,i_iVideoDataLen,o_ptFrameInfo);

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
int RtmpMediaHandle::ParseH264NaluFromFrame(unsigned char *i_pbVideoData,int i_iVideoDataLen ,T_RtmpFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    unsigned char *pcFrameStartPos = NULL;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    unsigned char bStartCodeLen = 0;
    
    if(NULL == i_pbVideoData || NULL == o_ptFrameInfo || NULL == o_ptFrameInfo->pbNaluData ||i_iVideoDataLen <= 4)
    {
        RTMP_LOGE("ParseH264NaluFromFrame NULL %d\r\n", i_iVideoDataLen);
        return iRet;
    }
    pcFrameData = i_pbVideoData;
    iRemainDataLen = i_iVideoDataLen;
    
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
                    iRet=SetH264NaluData(bNaluType,pcNaluStartPos+bStartCodeLen,pcNaluEndPos - pcNaluStartPos - 3,o_ptFrameInfo);//包括类型减3开始码
                    if(iRet < 0)
                    {
                        RTMP_LOGE("SetH264NaluData err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
                        return iRet;
                    }
                }
                pcNaluStartPos = pcNaluEndPos;//上一个nalu的结束为下一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
                pcNaluEndPos = NULL;
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
                    iRet=SetH264NaluData(bNaluType,pcNaluStartPos+bStartCodeLen,pcNaluEndPos - pcNaluStartPos - 4,o_ptFrameInfo);//包括类型减4开始码
                    if(iRet < 0)
                    {
                        RTMP_LOGE("SetH264NaluData err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
                        return iRet;
                    }
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = pcNaluStartPos[4] & 0x1f;
                pcNaluEndPos = NULL;
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
    if(pcNaluStartPos != NULL)
    {
        iRet=SetH264NaluData(bNaluType,pcNaluStartPos+bStartCodeLen,pcFrameData - pcNaluStartPos - bStartCodeLen,o_ptFrameInfo);//包括类型减4开始码
        if(iRet < 0)
        {
            RTMP_LOGE("SetH264NaluData err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
            return iRet;
        }
    }

    return 0;
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
int RtmpMediaHandle::ParseH265NaluFromFrame(unsigned char *i_pbVideoData,int i_iVideoDataLen,T_RtmpFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    unsigned char *pcFrameStartPos = NULL;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    
    if(NULL == i_pbVideoData || NULL == o_ptFrameInfo  || NULL == o_ptFrameInfo->pbNaluData ||i_iVideoDataLen <= 4)
    {
        RTMP_LOGE("ParseH265NaluFromFrame NULL %d\r\n", i_iVideoDataLen);
        return iRet;
    }
    pcFrameData = i_pbVideoData;
    iRemainDataLen = i_iVideoDataLen;
    
    while(iRemainDataLen > 0)
    {
        if (iRemainDataLen >= 4 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 0 && pcFrameData[3] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;
            }
            else
            {
                pcNaluStartPos = pcFrameData;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet = SetH265NaluData(bNaluType,pcNaluStartPos+4,pcNaluEndPos - pcNaluStartPos - 4,o_ptFrameInfo);//包括类型减4//去掉00 00 00 01
                    if(iRet < 0)
                    {
                        RTMP_LOGE("SetH265NaluData err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
                        return iRet;
                    }
                }
                pcNaluStartPos = pcNaluEndPos;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
                pcNaluEndPos = NULL;
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
    if(pcNaluStartPos != NULL)
    {
        iRet=SetH265NaluData(bNaluType,pcNaluStartPos+4,pcFrameData - pcNaluStartPos - 4,o_ptFrameInfo);//包括类型减4开始码
        if(iRet < 0)
        {
            RTMP_LOGE("SetH265NaluData err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
            return iRet;
        }
    }
    return 0;
}


/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::SetH264NaluData(unsigned char i_bNaluType,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_RtmpFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    if(NULL == i_pbNaluData || NULL == o_ptFrameInfo)
    {
        RTMP_LOGE("SetNaluData NULL %d \r\n", iRet);
        return iRet;
    }

    switch(i_bNaluType)//取nalu类型
    {
        case 0x7:
        {
            memset(o_ptFrameInfo->abSPS,0,sizeof(o_ptFrameInfo->abSPS));
            o_ptFrameInfo->wSpsLen= i_iNaluDataLen;//包括类型(减3开始码)
            memcpy(o_ptFrameInfo->abSPS,i_pbNaluData,o_ptFrameInfo->wSpsLen);
            break;
        }
        case 0x8:
        {
            memset(o_ptFrameInfo->abPPS,0,sizeof(o_ptFrameInfo->abPPS));
            o_ptFrameInfo->wPpsLen= i_iNaluDataLen;//包括类型减3开始码
            memcpy(o_ptFrameInfo->abPPS,i_pbNaluData,o_ptFrameInfo->wPpsLen);
            break;
        }
        case 0x1:
        case 0x5:
        {
            if(o_ptFrameInfo->dwNaluDataLen+i_iNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen)//去掉00 00 00 01
            {
                RTMP_LOGE("o_ptFrameInfo->dwNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
                return iRet;
            }
            o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData = o_ptFrameInfo->pbNaluData+o_ptFrameInfo->dwNaluDataLen;
            memcpy(o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData,i_pbNaluData,i_iNaluDataLen);
            o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].dwDataLen = i_iNaluDataLen;
            o_ptFrameInfo->dwNaluCnt++;
            o_ptFrameInfo->dwNaluDataLen += i_iNaluDataLen;
            break;
        }
        default:
        {
            break;
        }
    }
    
    return 0;
}


/*****************************************************************************
-Fuction        : SetH265NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::SetH265NaluData(unsigned char i_bNaluType,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_RtmpFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    if(NULL == i_pbNaluData || NULL == o_ptFrameInfo)
    {
        RTMP_LOGE("SetH265NaluData NULL %d \r\n", iRet);
        return iRet;
    }

    if(i_bNaluType >= 0 && i_bNaluType <= 9)// p slice 片
    {
        if(o_ptFrameInfo->dwNaluDataLen+i_iNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen)//去掉00 00 00 01
        {
            RTMP_LOGE("o_ptFrameInfo->dwNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
            return iRet;
        }
        o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData = o_ptFrameInfo->pbNaluData+o_ptFrameInfo->dwNaluDataLen;
        memcpy(o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData,i_pbNaluData,i_iNaluDataLen);
        o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].dwDataLen = i_iNaluDataLen;//去掉00 00 00 01
        o_ptFrameInfo->dwNaluCnt++;
        o_ptFrameInfo->dwNaluDataLen += i_iNaluDataLen;
    }
    else if(i_bNaluType >= 16 && i_bNaluType <= 21)// IRAP 等同于i帧
    {
        if(o_ptFrameInfo->dwNaluDataLen+i_iNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen)//去掉00 00 00 01
        {
            RTMP_LOGE("o_ptFrameInfo->dwNaluDataLen > o_ptFrameInfo->dwNaluDataMaxLen err %d %d\r\n", o_ptFrameInfo->dwNaluDataLen,o_ptFrameInfo->dwNaluDataMaxLen);
            return iRet;
        }
        o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData = o_ptFrameInfo->pbNaluData+o_ptFrameInfo->dwNaluDataLen;
        memcpy(o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].pbData,i_pbNaluData,i_iNaluDataLen);
        o_ptFrameInfo->atNaluInfo[o_ptFrameInfo->dwNaluCnt].dwDataLen = i_iNaluDataLen;//去掉00 00 00 01
        o_ptFrameInfo->dwNaluCnt++;
        o_ptFrameInfo->dwNaluDataLen += i_iNaluDataLen;
    }
    else if(i_bNaluType == 32)//VPS
    {
        memset(o_ptFrameInfo->abVPS,0,sizeof(o_ptFrameInfo->abVPS));
        o_ptFrameInfo->wVpsLen= i_iNaluDataLen;//包括类型减4
        memcpy(o_ptFrameInfo->abVPS,i_pbNaluData,o_ptFrameInfo->wVpsLen);
    }
    else if(i_bNaluType == 33)//SPS
    {
        memset(o_ptFrameInfo->abSPS,0,sizeof(o_ptFrameInfo->abSPS));
        o_ptFrameInfo->wSpsLen= i_iNaluDataLen;//包括类型减4
        memcpy(o_ptFrameInfo->abSPS,i_pbNaluData,o_ptFrameInfo->wSpsLen);
    }
    else if(i_bNaluType == 34)//PPS
    {
        memset(o_ptFrameInfo->abPPS,0,sizeof(o_ptFrameInfo->abPPS));
        o_ptFrameInfo->wPpsLen= i_iNaluDataLen;//包括类型减4
        memcpy(o_ptFrameInfo->abPPS,i_pbNaluData,o_ptFrameInfo->wPpsLen);
    }
    
    return 0;
}

/*****************************************************************************
-Fuction        : GenerateVideoData
-Description    : GenerateVideoData
-Input          : 
-Output         : 
-Return         : iVideoDataLen must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GenerateVideoData(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData)
{
    int iVideoDataLen = 0;

    if(NULL == i_ptFrameInfo || NULL == o_pbVideoData)
    {
        RTMP_LOGE("GenerateVideoData NULL %d\r\n", i_iMaxVideoData);
        return iVideoDataLen;
    }
    if(RTMP_UNKNOW_ENC_TYPE == i_ptFrameInfo->eEncType ||RTMP_UNKNOW_FRAME == i_ptFrameInfo->eFrameType||RTMP_AUDIO_FRAME == i_ptFrameInfo->eFrameType)
    {
        RTMP_LOGE("GenerateVideoData RTMP_UNKNOW_FRAME %d\r\n", i_ptFrameInfo->eFrameType);
        return iVideoDataLen;
    }
    if(RTMP_ENC_H264 == i_ptFrameInfo->eEncType)
    {
        iVideoDataLen = GenerateVideoDataH264(i_ptFrameInfo,i_iIsAvcSeqHeader,o_pbVideoData,i_iMaxVideoData);
    }
    if(RTMP_ENC_H265 == i_ptFrameInfo->eEncType)
    {
        iVideoDataLen = GenerateVideoDataH265(i_ptFrameInfo,i_iIsAvcSeqHeader,o_pbVideoData,i_iMaxVideoData);
    }

    return iVideoDataLen;
}

/*****************************************************************************
-Fuction        : GenerateVideoData
-Description    : GenerateVideoData
-Input          : 
-Output         : 
-Return         : iVideoDataLen must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GenerateAudioData(T_RtmpAudioInfo * i_ptRtmpAudioInfo,int i_iIsAACSeqHeader,unsigned char *o_pbAudioData,int i_iMaxAudioData)
{
    int iAudioDataLen = 0;
    unsigned char bAudioTagHeader = 0;

    
    if(NULL == i_ptRtmpAudioInfo || NULL == o_pbAudioData)
    {
        RTMP_LOGE("GenerateAudioData NULL %d\r\n", i_iMaxAudioData);
        return iAudioDataLen;
    }
    if(RTMP_UNKNOW_ENC_TYPE == i_ptRtmpAudioInfo->tParam.eEncType)
    {
        RTMP_LOGE("RTMP_UNKNOW_FRAME %d\r\n", i_ptRtmpAudioInfo->tParam.eEncType);
        return iAudioDataLen;
    }
    //tag Header 1 byte
    bAudioTagHeader = CreateAudioDataTagHeader(&i_ptRtmpAudioInfo->tParam);
    o_pbAudioData[iAudioDataLen] = bAudioTagHeader;
    iAudioDataLen += 1;

    if(RTMP_ENC_AAC == i_ptRtmpAudioInfo->tParam.eEncType)
    {
        //tag Body(AAC packet type) 1 byte
        if(0 == i_iIsAACSeqHeader)
        {
            o_pbAudioData[iAudioDataLen] = 0x1;
        }
        else
        {
            o_pbAudioData[iAudioDataLen] = 0x0;//AvcSeqHeader
        }
        iAudioDataLen += 1;
        //tag Body(AAC SeqHeader or AAC Raw)
        if(0 == i_iIsAACSeqHeader)
        {
            memcpy(&o_pbAudioData[iAudioDataLen], i_ptRtmpAudioInfo->pbAudioData+7,i_ptRtmpAudioInfo->dwAudioDataLen-7);
            iAudioDataLen += i_ptRtmpAudioInfo->dwAudioDataLen-7;
        }
        else
        {
            iAudioDataLen += CreateAudioSpecCfgAAC(i_ptRtmpAudioInfo->tParam.dwSamplesPerSecond,i_ptRtmpAudioInfo->tParam.dwChannels,&o_pbAudioData[iAudioDataLen]);
        }
    }
    else
    {
        memcpy(&o_pbAudioData[iAudioDataLen], i_ptRtmpAudioInfo->pbAudioData,i_ptRtmpAudioInfo->dwAudioDataLen);
        iAudioDataLen += i_ptRtmpAudioInfo->dwAudioDataLen;
    }
    return iAudioDataLen;
}

/*****************************************************************************
-Fuction        : GenerateVideoData
-Description    : GenerateVideoMsgBody
-Input          : i_iIsAvcSeqHeader 0 nalu ,1 AvcSeqHeader
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GenerateVideoDataH264(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData)
{
    int iVideoDataLen = 0;
    unsigned char* pbVideoData = NULL;

    if(NULL == i_ptFrameInfo || NULL == o_pbVideoData ||
    i_iMaxVideoData < (int)(5+11 +(i_iIsAvcSeqHeader?i_ptFrameInfo->wSpsLen+i_ptFrameInfo->wPpsLen : i_ptFrameInfo->dwNaluDataLen)))
    {
        RTMP_LOGE("GenerateVideoData NULL %d\r\n", i_iMaxVideoData);
        return iVideoDataLen;
    }
    if(RTMP_ENC_H264 != i_ptFrameInfo->eEncType ||RTMP_UNKNOW_FRAME == i_ptFrameInfo->eFrameType || 
    (RTMP_VIDEO_KEY_FRAME == i_ptFrameInfo->eFrameType && 0 != i_iIsAvcSeqHeader && (i_ptFrameInfo->wSpsLen <= 0 ||i_ptFrameInfo->wPpsLen <= 0)))
    {
        RTMP_LOGE("GenerateVideoData RTMP_UNKNOW_FRAME %d\r\n", i_ptFrameInfo->eFrameType);
        return iVideoDataLen;
    }
    //tag Header 1 byte
    if(RTMP_VIDEO_INNER_FRAME == i_ptFrameInfo->eFrameType)
    {
        o_pbVideoData[iVideoDataLen] = 0x27;
    }
    if(RTMP_VIDEO_KEY_FRAME == i_ptFrameInfo->eFrameType)
    {
        o_pbVideoData[iVideoDataLen] = 0x17;
    }
    iVideoDataLen += 1;
    //tag Body(AVCC Header) 4 byte
    if(0 == i_iIsAvcSeqHeader)
    {
        o_pbVideoData[iVideoDataLen] = 0x1;
    }
    else
    {
        o_pbVideoData[iVideoDataLen] = 0x0;//AvcSeqHeader
    }
    iVideoDataLen += 1;
    memset(&o_pbVideoData[iVideoDataLen],0,3);
    iVideoDataLen += 3;
    
    //tag Body(AVCC Body)
    if(0 == i_iIsAvcSeqHeader)
    {
        int i=0;
#if 0        
        const char *test = "{\"test\":123}";
        char seih[] = {6,5};
        unsigned char pInfoUUID[16] = {0x4A, 0x46, 0x55, 0x55, 0x49, 0x44, 0x49, 0x4E, 0x46, 0x4F, 0x46, 0x52, 0x41, 0x4D, 0x00, 0x0F}; 
        char sei[1024];
        int len = 0;
        memcpy(&sei[len],seih,sizeof(seih));
        len+=sizeof(seih);
        sei[len] = sizeof(pInfoUUID)+strlen(test);
        len++;
        memcpy(&sei[len],pInfoUUID,sizeof(pInfoUUID));
        len+=sizeof(pInfoUUID);
        memcpy(&sei[len],test,strlen(test));
        len+=strlen(test);
        if(len%2 == 0)
        {
            sei[len] = 0x00;
            sei[len+1] = 0x80;
            len+=2;
        }
        else
        {
            sei[len] = 0x80;
            len+=1;
        }
        pbVideoData = &o_pbVideoData[iVideoDataLen];
        Write32BE(pbVideoData,len);
        iVideoDataLen += sizeof(len);
        memcpy(&o_pbVideoData[iVideoDataLen],sei,len);
        iVideoDataLen +=len;
#endif
        for(i=0;i<(int)i_ptFrameInfo->dwNaluCnt;i++)
        {
            pbVideoData = &o_pbVideoData[iVideoDataLen];
            Write32BE(pbVideoData,i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            iVideoDataLen += sizeof(i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            memcpy(&o_pbVideoData[iVideoDataLen], i_ptFrameInfo->atNaluInfo[i].pbData,i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            iVideoDataLen += i_ptFrameInfo->atNaluInfo[i].dwDataLen;
        }
    }
    else
    {
        o_pbVideoData[iVideoDataLen] = 0x1;////AVC sequence header or extradata版本号 1
        iVideoDataLen += 1;
        memcpy(&o_pbVideoData[iVideoDataLen], &i_ptFrameInfo->abSPS[1], 3);
        iVideoDataLen += 3;
        o_pbVideoData[iVideoDataLen] = 0xff;//nalu size 4 字节
        iVideoDataLen += 1;
        o_pbVideoData[iVideoDataLen] = 0xe1;//SPS 个数 =1
        iVideoDataLen += 1;
        pbVideoData = &o_pbVideoData[iVideoDataLen];
        Write16BE(pbVideoData, i_ptFrameInfo->wSpsLen);
        iVideoDataLen += sizeof(i_ptFrameInfo->wSpsLen);
        memcpy(&o_pbVideoData[iVideoDataLen], i_ptFrameInfo->abSPS,i_ptFrameInfo->wSpsLen);
        iVideoDataLen += i_ptFrameInfo->wSpsLen;
        o_pbVideoData[iVideoDataLen] = 0x1;//PPS 个数 =1
        iVideoDataLen += 1;
        pbVideoData = &o_pbVideoData[iVideoDataLen];
        Write16BE(pbVideoData, i_ptFrameInfo->wPpsLen);
        iVideoDataLen += sizeof(i_ptFrameInfo->wPpsLen);
        memcpy(&o_pbVideoData[iVideoDataLen], i_ptFrameInfo->abPPS,i_ptFrameInfo->wPpsLen);
        iVideoDataLen += i_ptFrameInfo->wPpsLen;
    }
    
    return iVideoDataLen;
}

/*****************************************************************************
-Fuction        : GenerateVideoData
-Description    : GenerateVideoMsgBody
-Input          : i_iIsAvcSeqHeader 0 nalu ,1 AvcSeqHeader
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GenerateVideoDataH265(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData)
{
    int iVideoDataLen = 0;
    T_RtmpH265Extradata tRtmpH265Extradata;
    unsigned char* pbVideoData = NULL;

    if(NULL == i_ptFrameInfo || NULL == o_pbVideoData ||
    i_iMaxVideoData < (int)(5+23 +(i_iIsAvcSeqHeader?i_ptFrameInfo->wSpsLen+i_ptFrameInfo->wPpsLen+i_ptFrameInfo->wVpsLen : i_ptFrameInfo->dwNaluDataLen)))
    {
        RTMP_LOGE("GenerateVideoData NULL %d\r\n", i_iMaxVideoData);
        return iVideoDataLen;
    }
    if(RTMP_ENC_H265 != i_ptFrameInfo->eEncType ||RTMP_UNKNOW_FRAME == i_ptFrameInfo->eFrameType || 
    (RTMP_VIDEO_KEY_FRAME == i_ptFrameInfo->eFrameType && 0 != i_iIsAvcSeqHeader && (i_ptFrameInfo->wSpsLen <= 0 ||i_ptFrameInfo->wPpsLen <= 0)))
    {
        RTMP_LOGE("GenerateVideoData RTMP_UNKNOW_FRAME %d\r\n", i_ptFrameInfo->eFrameType);
        return iVideoDataLen;
    }
    pbVideoData = o_pbVideoData;
    //tag Header 1 byte
    if(RTMP_VIDEO_INNER_FRAME == i_ptFrameInfo->eFrameType)
    {
        *pbVideoData = 0x2c;//微信小程序验证h265这种定义是可以的
    }
    if(RTMP_VIDEO_KEY_FRAME == i_ptFrameInfo->eFrameType)
    {
        *pbVideoData = 0x1c;
    }
    pbVideoData += 1;
    //tag Body(AVCC Header) 4 byte
    if(0 == i_iIsAvcSeqHeader)
    {
        *pbVideoData = 0x1;
    }
    else
    {
        *pbVideoData = 0x0;//AvcSeqHeader
    }
    pbVideoData += 1;
    memset(pbVideoData,0,3);
    pbVideoData += 3;
    
    //tag Body(AVCC Body)
    if(0 == i_iIsAvcSeqHeader)
    {
        int i=0;
        for(i=0;i<(int)i_ptFrameInfo->dwNaluCnt;i++)
        {
            Write32BE(pbVideoData,i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            pbVideoData += sizeof(i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            memcpy(pbVideoData, i_ptFrameInfo->atNaluInfo[i].pbData,i_ptFrameInfo->atNaluInfo[i].dwDataLen);
            pbVideoData += i_ptFrameInfo->atNaluInfo[i].dwDataLen;
        }
    }
    else//23字节解析可参考SrsRawHEVCStream::mux_sequence_header,需要通过解析vps,sps才能得出这个HEVC extradata
    {//或者参考media server中的hevc_profile_tier_level调用
        memset(&tRtmpH265Extradata,0,sizeof(T_RtmpH265Extradata));
        if(0 != AnnexbToH265Extradata(i_ptFrameInfo,&tRtmpH265Extradata))
        {
            RTMP_LOGE("AnnexbToH265Extradata err %d\r\n", iVideoDataLen);
            iVideoDataLen = 0;
            return -1;
        }
        // HEVCDecoderConfigurationRecord
        // ISO/IEC 14496-15:2017
        // 8.3.3.1.2 Syntax
        *pbVideoData = tRtmpH265Extradata.configurationVersion;
        pbVideoData++;
        // general_profile_space + general_tier_flag + general_profile_idc
        *pbVideoData = ((tRtmpH265Extradata.general_profile_space & 0x03) << 6) | ((tRtmpH265Extradata.general_tier_flag & 0x01) << 5) | (tRtmpH265Extradata.general_profile_idc & 0x1F);
        pbVideoData++;

        // general_profile_compatibility_flags
        Write32BE(pbVideoData, tRtmpH265Extradata.general_profile_compatibility_flags);
        pbVideoData += sizeof(tRtmpH265Extradata.general_profile_compatibility_flags);
        // general_constraint_indicator_flags
        Write32BE(pbVideoData, (unsigned int)(tRtmpH265Extradata.general_constraint_indicator_flags >> 16));
        pbVideoData += sizeof(unsigned int);
        Write16BE(pbVideoData, (unsigned short)tRtmpH265Extradata.general_constraint_indicator_flags);
        pbVideoData += sizeof(unsigned short);
        // general_level_idc
        *pbVideoData = tRtmpH265Extradata.general_level_idc;
        pbVideoData++;
        // min_spatial_segmentation_idc
        Write16BE(pbVideoData, 0xF000 | tRtmpH265Extradata.min_spatial_segmentation_idc);
        pbVideoData += sizeof(unsigned short);
        *pbVideoData = 0xFC | tRtmpH265Extradata.parallelismType;
        pbVideoData++;
        *pbVideoData = 0xFC | tRtmpH265Extradata.chromaFormat;
        pbVideoData++;
        *pbVideoData = 0xF8 | tRtmpH265Extradata.bitDepthLumaMinus8;
        pbVideoData++;
        *pbVideoData = 0xF8 | tRtmpH265Extradata.bitDepthChromaMinus8;
        pbVideoData++;
        Write16BE(pbVideoData,tRtmpH265Extradata.avgFrameRate);
        pbVideoData += sizeof(unsigned short);
        *pbVideoData = (tRtmpH265Extradata.constantFrameRate << 6) | ((tRtmpH265Extradata.numTemporalLayers & 0x07) << 3) | ((tRtmpH265Extradata.temporalIdNested & 0x01) << 2) | (tRtmpH265Extradata.lengthSizeMinusOne & 0x03);
        pbVideoData++;
        *pbVideoData = tRtmpH265Extradata.numOfArrays;
        pbVideoData++;
        
        // numOfVideoParameterSets, always 1
        char numOfVideoParameterSets[2] = { 0x00,0x01 };
     // vps
        // nal_type
        *pbVideoData = (i_ptFrameInfo->abVPS[0] >> 1) & 0x3f;
        pbVideoData++;
        // numOfVideoParameterSets, always 1
        memcpy(pbVideoData, numOfVideoParameterSets, sizeof(numOfVideoParameterSets));
        pbVideoData += sizeof(numOfVideoParameterSets);
        // videoParameterSetLength
        Write16BE(pbVideoData,i_ptFrameInfo->wVpsLen);
        pbVideoData += sizeof(unsigned short);
        // videoParameterSetNALUnit
        memcpy(pbVideoData,i_ptFrameInfo->abVPS,i_ptFrameInfo->wVpsLen);
        pbVideoData += i_ptFrameInfo->wVpsLen;
    // sps
       // nal_type
        *pbVideoData = (i_ptFrameInfo->abSPS[0] >> 1) & 0x3f;
       pbVideoData++;
       // numOfVideoParameterSets, always 1
       memcpy(pbVideoData, numOfVideoParameterSets, sizeof(numOfVideoParameterSets));
       pbVideoData += sizeof(numOfVideoParameterSets);
       // videoParameterSetLength
       Write16BE(pbVideoData,i_ptFrameInfo->wSpsLen);
       pbVideoData += sizeof(unsigned short);
       // videoParameterSetNALUnit
       memcpy(pbVideoData,i_ptFrameInfo->abSPS,i_ptFrameInfo->wSpsLen);
       pbVideoData += i_ptFrameInfo->wSpsLen;
   // pps
      // nal_type
       *pbVideoData = (i_ptFrameInfo->abPPS[0] >> 1) & 0x3f;
       pbVideoData++;
      // numOfVideoParameterSets, always 1
      memcpy(pbVideoData, numOfVideoParameterSets, sizeof(numOfVideoParameterSets));
      pbVideoData += sizeof(numOfVideoParameterSets);
      // videoParameterSetLength
      Write16BE(pbVideoData,i_ptFrameInfo->wPpsLen);
      pbVideoData += sizeof(unsigned short);
      // videoParameterSetNALUnit
      memcpy(pbVideoData,i_ptFrameInfo->abPPS,i_ptFrameInfo->wPpsLen);
      pbVideoData += i_ptFrameInfo->wPpsLen;
    }
    iVideoDataLen = pbVideoData - o_pbVideoData;
    return iVideoDataLen;
}




/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::AnnexbToH265Extradata(T_RtmpFrameInfo * i_ptFrameInfo,T_RtmpH265Extradata *o_ptRtmpH265Extradata)
{
    int iRet = -1;
    if(NULL == i_ptFrameInfo || NULL == o_ptRtmpH265Extradata)
    {
        RTMP_LOGE("AnnexbToH265Extradata NULL %d \r\n", iRet);
        return iRet;
    }
    o_ptRtmpH265Extradata->configurationVersion = 1;
    o_ptRtmpH265Extradata->lengthSizeMinusOne = 3; // 4 bytes
    o_ptRtmpH265Extradata->numOfArrays = 3; // numOfArrays, default 3
    iRet = VpsToH265Extradata(i_ptFrameInfo->abVPS,i_ptFrameInfo->wVpsLen,o_ptRtmpH265Extradata);
    iRet |= SpsToH265Extradata(i_ptFrameInfo->abSPS,i_ptFrameInfo->wSpsLen,o_ptRtmpH265Extradata);
    return iRet;
}




/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::VpsToH265Extradata(unsigned char *i_pbVpsData,unsigned short i_wVpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata)
{
    int iRet = -1;
    unsigned char abSodbVPS[RTMP_VPS_MAX_SIZE];
    int iSodbLen = 0;
    unsigned char vps_max_sub_layers_minus1;
    unsigned char vps_temporal_id_nesting_flag;
    
    if(NULL == i_pbVpsData || NULL == o_ptRtmpH265Extradata)
    {
        RTMP_LOGE("VpsToH265Extradata NULL %d \r\n", i_wVpsLen);
        return iRet;
    }
    memset(abSodbVPS,0,sizeof(abSodbVPS));
    iSodbLen = DecodeEBSP(i_pbVpsData, i_wVpsLen, abSodbVPS);
    if (iSodbLen < 16 + 2)
        return iRet;
    vps_max_sub_layers_minus1 = (abSodbVPS[3] >> 1) & 0x07;
    vps_temporal_id_nesting_flag = abSodbVPS[3] & 0x01;
    o_ptRtmpH265Extradata->numTemporalLayers = MAX(o_ptRtmpH265Extradata->numTemporalLayers, vps_max_sub_layers_minus1 + 1);
    o_ptRtmpH265Extradata->temporalIdNested = (o_ptRtmpH265Extradata->temporalIdNested || vps_temporal_id_nesting_flag) ? 1 : 0;
    iRet = HevcProfileTierLevel(abSodbVPS + 6, iSodbLen - 6, vps_max_sub_layers_minus1, o_ptRtmpH265Extradata);
    if(iRet < 0)
    {
        RTMP_LOGE("HevcProfileTierLevel err %d \r\n", i_wVpsLen);
        return iRet;
    }
    return 0;
}




/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::SpsToH265Extradata(unsigned char *i_pbSpsData,unsigned short i_wSpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata)
{
    int iRet = -1;
    unsigned char abSodbSPS[RTMP_SPS_MAX_SIZE];
    int iSodbLen = 0;
    unsigned char sps;
    unsigned char sps_max_sub_layers_minus1;
    unsigned char sps_temporal_id_nesting_flag;
    unsigned char conformance_window_flag;
    int n;
    unsigned int pic_width_in_luma_samples;
    unsigned int pic_height_in_luma_samples;
    unsigned int conf_win_left_offset;
    unsigned int conf_win_right_offset;
    unsigned int conf_win_top_offset;
    unsigned int conf_win_bottom_offset;
    unsigned int sub_width,sub_height;
    unsigned char separate_colour_plane_flag = 0;
    
    if(NULL == i_pbSpsData || NULL == o_ptRtmpH265Extradata || 0 >= i_wSpsLen)
    {
        RTMP_LOGE("SpsToH265Extradata NULL %d \r\n", i_wSpsLen);
        return iRet;
    }
    memset(abSodbSPS,0,sizeof(abSodbSPS));
    iSodbLen = DecodeEBSP(i_pbSpsData, i_wSpsLen, abSodbSPS);
    if (iSodbLen < 12+3)
        return iRet;
    sps_max_sub_layers_minus1 = (abSodbSPS[2] >> 1) & 0x07;
    sps_temporal_id_nesting_flag = abSodbSPS[2] & 0x01;
    n = HevcProfileTierLevel(abSodbSPS + 3, iSodbLen - 3, sps_max_sub_layers_minus1, o_ptRtmpH265Extradata);
    if (n <= 0)
        return iRet;
    n = (n + 3) * 8;
    
    sps = (unsigned char)H264ReadBitByUE(abSodbSPS, iSodbLen, &n);
    o_ptRtmpH265Extradata->chromaFormat = (unsigned char)H264ReadBitByUE(abSodbSPS, iSodbLen, &n);
    if (3 == o_ptRtmpH265Extradata->chromaFormat)
    {
        separate_colour_plane_flag=BIT(abSodbSPS, n);
        n++;
    }
    pic_width_in_luma_samples=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // pic_width_in_luma_samples
    pic_height_in_luma_samples=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // pic_height_in_luma_samples
    conformance_window_flag = BIT(abSodbSPS, n); 
    n++; // conformance_window_flag
    if (conformance_window_flag)
    {
        conf_win_left_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // conf_win_left_offset
        conf_win_right_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // conf_win_right_offset
        conf_win_top_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // conf_win_top_offset
        conf_win_bottom_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &n); // conf_win_bottom_offset
    }
    o_ptRtmpH265Extradata->bitDepthLumaMinus8 = (unsigned char)H264ReadBitByUE(abSodbSPS, iSodbLen, &n);
    o_ptRtmpH265Extradata->bitDepthChromaMinus8 = (unsigned char)H264ReadBitByUE(abSodbSPS, iSodbLen, &n);
    
    o_ptRtmpH265Extradata->pic_width = pic_width_in_luma_samples;
    o_ptRtmpH265Extradata->pic_height = pic_height_in_luma_samples;
    if (conformance_window_flag)
    {
        sub_width=((1==o_ptRtmpH265Extradata->chromaFormat)||(2 == o_ptRtmpH265Extradata->chromaFormat))&&(0==separate_colour_plane_flag)?2:1;
        sub_height=(1==o_ptRtmpH265Extradata->chromaFormat)&& (0 == separate_colour_plane_flag)?2:1;
        o_ptRtmpH265Extradata->pic_width -= (sub_width*conf_win_right_offset + sub_width*conf_win_left_offset);
        o_ptRtmpH265Extradata->pic_height -= (sub_height*conf_win_bottom_offset + sub_height*conf_win_top_offset);
    }
    return 0;
}

/*****************************************************************************
-Fuction        : RtmpMediaHandle
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::SpsToH264Resolution(unsigned char *i_pbSpsData,unsigned short i_wSpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata)
{
    int iRet = -1;
    unsigned char abSodbSPS[RTMP_SPS_MAX_SIZE];
    int iSodbLen = 0;
    
    unsigned char id;
    unsigned char profile_idc;
    unsigned char level_idc;
    unsigned char constraint_set_flags = 0;
    unsigned char chroma_format_idc;
    unsigned char bit_depth_luma;
    unsigned char frame_mbs_only_flag;
    unsigned char seq_scaling_matrix_present_flag,seq_scaling_list_present_flag;
    unsigned char frame_cropping_flag;
    int delta_scale, lastScale = 8, nextScale = 8;
    int sizeOfScalingList;
    int iBit;//偏移第几位
    int i ,j,num_ref_frames_in_pic_order_cnt_cycle;
    unsigned int pic_order_cnt_type;
    unsigned int pic_width_in_mbs_minus1,pic_height_in_map_units_minus1;
    unsigned int frame_crop_left_offset,frame_crop_right_offset,frame_crop_top_offset,frame_crop_bottom_offset;

    
    if(NULL == i_pbSpsData || NULL == o_ptRtmpH265Extradata || 0 >= i_wSpsLen)
    {
        RTMP_LOGE("SpsToH265Extradata NULL %d \r\n", i_wSpsLen);
        return iRet;
    }
    memset(abSodbSPS,0,sizeof(abSodbSPS));
    iSodbLen = DecodeEBSP(i_pbSpsData, i_wSpsLen, abSodbSPS);
    if (iSodbLen < 12+3)
        return iRet;
    profile_idc = abSodbSPS[1] ;
    iBit = 2*8;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 0; // constraint_set0_flag
    iBit++;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 1; // constraint_set1_flag
    iBit++;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 2; // constraint_set2_flag
    iBit++;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 3; // constraint_set3_flag
    iBit++;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 4; // constraint_set4_flag
    iBit++;
    constraint_set_flags |= BIT(abSodbSPS, iBit) << 5; // constraint_set5_flag
    iBit++;
    iBit+=2;
    level_idc = abSodbSPS[3] ;
    iBit+=8;
    id=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit);
    if (profile_idc == 100 || profile_idc == 110 ||profile_idc == 122 || profile_idc == 244 || profile_idc ==  44 ||profile_idc == 83 || 
    profile_idc == 86 || profile_idc == 118 ||profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||profile_idc == 134) 
    {
        chroma_format_idc=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit);
        if (chroma_format_idc == 3) 
        {
            iBit++; // separate_colour_plane_flag
        }
        bit_depth_luma = H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit) + 8;
        H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // bit_depth_chroma_minus8
        iBit++; // qpprime_y_zero_transform_bypass_flag
        seq_scaling_matrix_present_flag = BIT(abSodbSPS, iBit);// seq_scaling_matrix_present_flag
        iBit++;
        if (seq_scaling_matrix_present_flag) 
        { 
            for (i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) 
            {
                seq_scaling_list_present_flag = BIT(abSodbSPS, iBit); // seq_scaling_list_present_flag
                iBit++;
                if (!seq_scaling_list_present_flag)
                    continue;
                lastScale = 8;
                nextScale = 8;
                sizeOfScalingList = i < 6 ? 16 : 64;
                for (j = 0; j < sizeOfScalingList; j++) 
                {
                    if (nextScale != 0) 
                    {
                        delta_scale = H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit);
                        nextScale = (lastScale + delta_scale) & 0xff;
                    }
                    lastScale = nextScale == 0 ? lastScale : nextScale;
                }
            }
        }
    }
    else
    {
        chroma_format_idc = 1;
        bit_depth_luma = 8;
    }
    H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // log2_max_frame_num_minus4
    pic_order_cnt_type = H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit);
    if (pic_order_cnt_type == 0) 
    {
        H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // log2_max_pic_order_cnt_lsb_minus4
    } 
    else if (pic_order_cnt_type == 1) 
    {
        iBit++;    // delta_pic_order_always_zero
        H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // offset_for_non_ref_pic
        H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // offset_for_top_to_bottom_field
        num_ref_frames_in_pic_order_cnt_cycle = (int)H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit);
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
            H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // offset_for_ref_frame
    }
    H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // max_num_ref_frames
    iBit++; // gaps_in_frame_num_value_allowed_flag
    pic_width_in_mbs_minus1=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // pic_width_in_mbs_minus1
    pic_height_in_map_units_minus1=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // pic_height_in_map_units_minus1
    frame_mbs_only_flag = BIT(abSodbSPS, iBit);
    iBit++;
    if (!frame_mbs_only_flag)
        iBit++; // mb_adaptive_frame_field_flag
    iBit++; // direct_8x8_inference_flag
    frame_cropping_flag=BIT(abSodbSPS, iBit);// frame_cropping_flag
    iBit++;
    if (frame_cropping_flag) 
    { 
        frame_crop_left_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // frame_crop_left_offset
        frame_crop_right_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // frame_crop_right_offset
        frame_crop_top_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // frame_crop_top_offset
        frame_crop_bottom_offset=H264ReadBitByUE(abSodbSPS, iSodbLen, &iBit); // frame_crop_bottom_offset
    }
    
    // 宽高计算公式
    o_ptRtmpH265Extradata->pic_width = (pic_width_in_mbs_minus1+1) * 16;
    o_ptRtmpH265Extradata->pic_height = (2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16;
    if(frame_cropping_flag)
    {
        unsigned int crop_unit_x;
        unsigned int crop_unit_y;
        if (0 == chroma_format_idc) // monochrome
        {
            crop_unit_x = 1;
            crop_unit_y = 2 - frame_mbs_only_flag;
        }
        else if (1 == chroma_format_idc) // 4:2:0
        {
            crop_unit_x = 2;
            crop_unit_y = 2 * (2 - frame_mbs_only_flag);
        }
        else if (2 == chroma_format_idc) // 4:2:2
        {
            crop_unit_x = 2;
            crop_unit_y = 2 - frame_mbs_only_flag;
        }
        else // 3 == sps.chroma_format_idc   // 4:4:4
        {
            crop_unit_x = 1;
            crop_unit_y = 2 -frame_mbs_only_flag;
        }
        
        o_ptRtmpH265Extradata->pic_width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
        o_ptRtmpH265Extradata->pic_height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
    }

    return 0;
}

/*****************************************************************************
-Fuction        : DecodeEBSP
-Description    : 脱壳操作
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::DecodeEBSP(unsigned char* nalu, int bytes, unsigned char* sodb)
{
    int i, j;
    for (j = i = 0; i < bytes; i++)
    {
        if (i + 2 < bytes && 0 == nalu[i] && 0 == nalu[i + 1] && 0x03 == nalu[i + 2])
        {
            sodb[j++] = nalu[i];
            sodb[j++] = nalu[i + 1];
            i += 2;
        }
        else
        {
            sodb[j++] = nalu[i];
        }
    }
    return j;
}

/*****************************************************************************
-Fuction        : hevc_profile_tier_level
-Description    : 脱壳操作
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::HevcProfileTierLevel(unsigned char* nalu, int bytes, unsigned char maxNumSubLayersMinus1,T_RtmpH265Extradata* hevc)
{
    int n;
    unsigned char i;
    unsigned char sub_layer_profile_present_flag[8];
    unsigned char sub_layer_level_present_flag[8];

    if (bytes < 12)
        return -1;

    hevc->general_profile_space = (nalu[0] >> 6) & 0x03;
    hevc->general_tier_flag = (nalu[0] >> 5) & 0x01;
    hevc->general_profile_idc = nalu[0] & 0x1f;

    hevc->general_profile_compatibility_flags = 0;
    hevc->general_profile_compatibility_flags |= nalu[1] << 24;
    hevc->general_profile_compatibility_flags |= nalu[2] << 16;
    hevc->general_profile_compatibility_flags |= nalu[3] << 8;
    hevc->general_profile_compatibility_flags |= nalu[4];

    hevc->general_constraint_indicator_flags = 0;
    hevc->general_constraint_indicator_flags |= ((uint64_t)nalu[5]) << 40;
    hevc->general_constraint_indicator_flags |= ((uint64_t)nalu[6]) << 32;
    hevc->general_constraint_indicator_flags |= ((uint64_t)nalu[7]) << 24;
    hevc->general_constraint_indicator_flags |= ((uint64_t)nalu[8]) << 16;
    hevc->general_constraint_indicator_flags |= ((uint64_t)nalu[9]) << 8;
    hevc->general_constraint_indicator_flags |= nalu[10];

    hevc->general_level_idc = nalu[11];
    if (maxNumSubLayersMinus1 < 1)
        return 12;

    if (bytes < 14)
        return -1; // error

    for (i = 0; i < maxNumSubLayersMinus1; i++)
    {
        sub_layer_profile_present_flag[i] = BIT(nalu, 12 * 8 + i * 2);
        sub_layer_level_present_flag[i] = BIT(nalu, 12 * 8 + i * 2 + 1);
    }

    n = 12 + 2;
    for (i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if(sub_layer_profile_present_flag[i])
            n += 11;
        if (sub_layer_level_present_flag[i])
            n += 1;
    }

    return bytes < n ? n : -1;
}

/*****************************************************************************
-Fuction        : H264ReadBitByUE 读字节数据
-Description    : 指数哥伦布编码，ue(v)的解码
第一步：每次读取一个比特，如果是0就继续读，直至读到1为止，
此时读取比特的个数即leadingZeroBits
第二步：从第一步读到的比特1后，再顺序读leadingZeroBits       
个比特
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
unsigned int RtmpMediaHandle::H264ReadBitByUE(unsigned char* data, int bytes, int* offset)
{
    int bit, i;
    int leadingZeroBits = -1;

    for (bit = 0; !bit && *offset / 8 < bytes; ++leadingZeroBits)
    {
        bit = (data[*offset / 8] >> (7 - (*offset % 8))) & 0x01;
        ++*offset;
    }

    bit = 0;
    //assert(leadingZeroBits < 32);

    for (i = 0; i < leadingZeroBits && *offset / 8 < bytes; i++)
    {
        bit = (bit << 1) | ((data[*offset / 8] >> (7 - (*offset % 8))) & 0x01);
        ++*offset;
    }

    return (unsigned int)((1 << leadingZeroBits) - 1 + bit);
}

/*****************************************************************************
-Fuction        : CreateAudioDataTagHeader
-Description    : CreateAudioDataTagHeader
-Input          : 
-Output         : 
-Return         : iVideoDataLen must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
unsigned char RtmpMediaHandle::CreateAudioDataTagHeader(T_RtmpAudioParam * i_ptRtmpAudioParam)
{
    unsigned char bAudioTagHeader = 0;
    unsigned char bEncType = 0;
    unsigned char bSampleRate = 3;// 0-5.5kHz;1-11kHz;2-22kHz;3-44kHz(AAC都是3)
    unsigned char bSendBit = 0b00;
    unsigned char bChannels = 1;
    
    switch(i_ptRtmpAudioParam->eEncType)
    {
        case RTMP_ENC_MP3:
        {
            bEncType = 2;
            break;
        }
        case RTMP_ENC_G711A:
        {
            bEncType = 7;
            break;
        }
        case RTMP_ENC_G711U:
        {
            bEncType = 8;
            break;
        }
        case RTMP_ENC_AAC:
        default:
        {
            bEncType = 10;
            break;
        }
    }
    switch (i_ptRtmpAudioParam->dwSamplesPerSecond)
    {
        case 5500:
        {
            bSampleRate = 0;
            break;
        }
        case 11000:
        {
            bSampleRate = 1;
            break;
        }
        case 22000:
        {
            bSampleRate = 2;
            break;
        }
        case 44000:
        default:
        {
            bSampleRate = 3;
            break;
        }
    }
    bSendBit = i_ptRtmpAudioParam->dwBitsPerSample == 16 ? 0b01 : 0b00;

    if (i_ptRtmpAudioParam->eEncType != RTMP_ENC_AAC)
    {
        bChannels = i_ptRtmpAudioParam->dwChannels > 1 ? 1 : 0;
    }
    else
    {
        bChannels = 1;
    }
    
    bAudioTagHeader = bEncType << 4;
    bAudioTagHeader |= (bSampleRate << 2);
    bAudioTagHeader |= (bSendBit << 1);
    bAudioTagHeader |= bChannels;

    return bAudioTagHeader;
}


/*****************************************************************************
-Fuction        : CreateAudioDataTagHeader
-Description    : CreateAudioDataTagHeader
-Input          : 
-Output         : 
-Return         : iVideoDataLen must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::CreateAudioSpecCfgAAC(unsigned int i_dwFrequency,unsigned int i_dwChannels,unsigned char *o_pbAudioData)
{
    int iAudioSpecCfgLen = 0;
    unsigned char bProfile = 1;
    unsigned char bAudioObjectType = 0;
    unsigned char bChannelConfiguration = 0;
    unsigned char bSamplingFrequencyIndex = 0;
    int i = 0;
    ///索引表  
    static unsigned int const s_adwSamplingFrequencyTable[16] = {
      96000, 88200, 64000, 48000,
      44100, 32000, 24000, 22050,
      16000, 12000, 11025, 8000,
      7350,  0,     0,      0
    };

    bProfile = 1;
    bAudioObjectType = bProfile + 1;  ///其中profile=1;  
    for (i = 0; i < 16; i++)
    {
        if (s_adwSamplingFrequencyTable[i] == i_dwFrequency)
        {
            bSamplingFrequencyIndex = (unsigned char)i;
            break;
        }
    }
    bChannelConfiguration = (unsigned char)i_dwChannels;
    
    o_pbAudioData[0] = (bAudioObjectType << 3) | (bSamplingFrequencyIndex >> 1);
    o_pbAudioData[1] = (bSamplingFrequencyIndex << 7) | (bChannelConfiguration << 3);
    iAudioSpecCfgLen = 2;

    return iAudioSpecCfgLen;
}


/*****************************************************************************
-Fuction        : GetVideoData
-Description    : FLV格式视频数据解析
-Input          : 
-Output         : 
-Return         : 不支持的编码格式返回-1，只有序列头返回0，完整数据则返回长度
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GetVideoData(unsigned char *i_pbVideoTag,int i_iTagLen,T_RtmpFrameInfo * m_ptRtmpFrameInfo,unsigned char *o_pbVideoData,int i_iVideoDataMaxLen)
{
    int iVideoDataLen = 0;
    unsigned char bIsAvcSeqHeader = 0;
    int iProcessedLen = 0;
    int i = 0,j = 0;
    unsigned int dwNaluLen = 0;
    unsigned char bNaluType = 0;
    
    if(NULL == i_pbVideoTag || NULL == m_ptRtmpFrameInfo || NULL == o_pbVideoData)
    {
        RTMP_LOGE("GetVideoData NULL %d\r\n", i_iTagLen);
        return -1;
    }
    //tag Header 1 byte
    switch(i_pbVideoTag[iProcessedLen])
    {
        case 0x17:
        {
            m_ptRtmpFrameInfo->eFrameType = RTMP_VIDEO_KEY_FRAME;
            m_ptRtmpFrameInfo->eEncType = RTMP_ENC_H264;
            break;
        }
        case 0x27:
        {
            m_ptRtmpFrameInfo->eFrameType = RTMP_VIDEO_INNER_FRAME;
            m_ptRtmpFrameInfo->eEncType = RTMP_ENC_H264;
            break;
        }
        case 0x1c:
        {
            m_ptRtmpFrameInfo->eFrameType = RTMP_VIDEO_KEY_FRAME;
            m_ptRtmpFrameInfo->eEncType = RTMP_ENC_H265;
            break;
        }
        case 0x2c:
        {
            m_ptRtmpFrameInfo->eFrameType = RTMP_VIDEO_INNER_FRAME;
            m_ptRtmpFrameInfo->eEncType = RTMP_ENC_H265;
            break;
        }
        default:
        {
            m_ptRtmpFrameInfo->eFrameType = RTMP_UNKNOW_FRAME;
            m_ptRtmpFrameInfo->eEncType = RTMP_UNKNOW_ENC_TYPE;
            break;
        }
    }
    if(RTMP_UNKNOW_ENC_TYPE == m_ptRtmpFrameInfo->eEncType)
    {
        RTMP_LOGE("ptRtmpFrameInfo->eEncType err %d\r\n",i_pbVideoTag[iProcessedLen]);//H265暂不支持,后续再做
        return -1;
    }
    iProcessedLen++;

    if(RTMP_ENC_H264 == m_ptRtmpFrameInfo->eEncType)
    {
        bIsAvcSeqHeader = i_pbVideoTag[iProcessedLen] == 0 ? 1:0;//tag Body(AVC packet type) 1 byte
        iProcessedLen++;
        //tag Body(AVC SeqHeader or AVC Raw)
        int cts;        /// video composition time(PTS - DTS), AVC/HEVC/AV1 only
        cts = (i_pbVideoTag[iProcessedLen] << 16) | (i_pbVideoTag[iProcessedLen+1] << 8) | i_pbVideoTag[iProcessedLen+2];
        cts = (cts + 0xFF800000) ^ 0xFF800000; // signed 24-integer
        iProcessedLen+=3;
        if(0 == bIsAvcSeqHeader)
        {
            T_RtmpH265Extradata tRtmpH265Extradata;
            memset(&tRtmpH265Extradata,0,sizeof(T_RtmpH265Extradata));
            SpsToH264Resolution(m_ptRtmpFrameInfo->abSPS,m_ptRtmpFrameInfo->wSpsLen,&tRtmpH265Extradata);
            m_ptRtmpFrameInfo->dwWidth = tRtmpH265Extradata.pic_width;
            m_ptRtmpFrameInfo->dwHeight = tRtmpH265Extradata.pic_height;
        
            iVideoDataLen=GetVideoDataNalu(&i_pbVideoTag[iProcessedLen],i_iTagLen-iProcessedLen,m_ptRtmpFrameInfo,o_pbVideoData,i_iVideoDataMaxLen);
        }
        else
        {
            if(1 != i_pbVideoTag[iProcessedLen])
            {
                RTMP_LOGE("i_pbVideoTag->ver err %d\r\n",i_pbVideoTag[iProcessedLen]);
                return -1;
            }
            iProcessedLen++;
            unsigned char bProfile;
            unsigned char bCompatibility; // constraint_set[0-5]_flag
            unsigned char bLevel;
            unsigned char bNaluLenSize; // NALUnitLength = (lengthSizeMinusOne + 1), default 4(0x03+1)
            unsigned char bSpsNum;
            unsigned char bPpsNum;
            bProfile = i_pbVideoTag[iProcessedLen];
            iProcessedLen++;
            bCompatibility = i_pbVideoTag[iProcessedLen];
            iProcessedLen++;
            bLevel = i_pbVideoTag[iProcessedLen];
            iProcessedLen++;
            bNaluLenSize = (i_pbVideoTag[iProcessedLen] & 0x03) + 1;
            iProcessedLen++;
            bSpsNum = i_pbVideoTag[iProcessedLen] & 0x1F;
            iProcessedLen++;
            for(i = 0;i<bSpsNum;i++)
            {
                Read16BE((i_pbVideoTag+iProcessedLen),&m_ptRtmpFrameInfo->wSpsLen);
                iProcessedLen+=2;
                if(m_ptRtmpFrameInfo->wSpsLen <= sizeof(m_ptRtmpFrameInfo->abSPS))
                {
                    memcpy(m_ptRtmpFrameInfo->abSPS,&i_pbVideoTag[iProcessedLen],m_ptRtmpFrameInfo->wSpsLen);
                }
                iProcessedLen+=m_ptRtmpFrameInfo->wSpsLen;
            }
            bPpsNum = i_pbVideoTag[iProcessedLen];
            iProcessedLen++;
            for(i = 0;i<bPpsNum;i++)
            {
                Read16BE((i_pbVideoTag+iProcessedLen),&m_ptRtmpFrameInfo->wPpsLen);
                iProcessedLen+=2;
                if(m_ptRtmpFrameInfo->wPpsLen <= sizeof(m_ptRtmpFrameInfo->abPPS))
                {
                    memcpy(m_ptRtmpFrameInfo->abPPS,&i_pbVideoTag[iProcessedLen],m_ptRtmpFrameInfo->wPpsLen);
                }
                iProcessedLen+=m_ptRtmpFrameInfo->wPpsLen;
            }
            m_ptRtmpFrameInfo->bNaluLenSize = bNaluLenSize;
        }
    }
    else if(RTMP_ENC_H265 == m_ptRtmpFrameInfo->eEncType)
    {        
        bIsAvcSeqHeader = i_pbVideoTag[iProcessedLen] == 0 ? 1:0;//tag Body(AVC packet type) 1 byte
        iProcessedLen++;
        //tag Body(AVC SeqHeader or AVC Raw)
        int cts;        /// video composition time(PTS - DTS), AVC/HEVC/AV1 only
        cts = (i_pbVideoTag[iProcessedLen] << 16) | (i_pbVideoTag[iProcessedLen+1] << 8) | i_pbVideoTag[iProcessedLen+2];
        cts = (cts + 0xFF800000) ^ 0xFF800000; // signed 24-integer
        iProcessedLen+=3;
        
        T_RtmpH265Extradata tRtmpH265Extradata;
        if(0 == bIsAvcSeqHeader)
        {
            memset(&tRtmpH265Extradata,0,sizeof(T_RtmpH265Extradata));
            SpsToH265Extradata(m_ptRtmpFrameInfo->abSPS,m_ptRtmpFrameInfo->wSpsLen,&tRtmpH265Extradata);
            m_ptRtmpFrameInfo->dwWidth = tRtmpH265Extradata.pic_width;
            m_ptRtmpFrameInfo->dwHeight = tRtmpH265Extradata.pic_height;
            iVideoDataLen=GetVideoDataNalu(&i_pbVideoTag[iProcessedLen],i_iTagLen-iProcessedLen,m_ptRtmpFrameInfo,o_pbVideoData,i_iVideoDataMaxLen);
        }
        else
        {
            unsigned char *pbVideoData = &i_pbVideoTag[iProcessedLen];
            unsigned char numOfArrays;
            unsigned char nalutype;
            unsigned char *pbVideoParams;
            unsigned short numOfVideoParameterSets,lenOfVideoParameterSets;
            
            memset(&tRtmpH265Extradata,0,sizeof(T_RtmpH265Extradata));
            tRtmpH265Extradata.configurationVersion = pbVideoData[0];
            if(1 != tRtmpH265Extradata.configurationVersion)
            {
                RTMP_LOGE("i_pbVideoTag->ver err %d\r\n",tRtmpH265Extradata.configurationVersion);
                return -1;
            }
            
            tRtmpH265Extradata.general_profile_space = (pbVideoData[1] >> 6) & 0x03;
            tRtmpH265Extradata.general_tier_flag = (pbVideoData[1] >> 5) & 0x01;
            tRtmpH265Extradata.general_profile_idc = pbVideoData[1] & 0x1F;
            tRtmpH265Extradata.general_profile_compatibility_flags = (pbVideoData[2] << 24) | (pbVideoData[3] << 16) | (pbVideoData[4] << 8) | pbVideoData[5];
            tRtmpH265Extradata.general_constraint_indicator_flags = ((unsigned int)pbVideoData[6] << 24) | ((unsigned int)pbVideoData[7] << 16) | ((unsigned int)pbVideoData[8] << 8) | (unsigned int)pbVideoData[9];
            tRtmpH265Extradata.general_constraint_indicator_flags = (tRtmpH265Extradata.general_constraint_indicator_flags << 16) | (((uint64)pbVideoData[10]) << 8) | pbVideoData[11];
            tRtmpH265Extradata.general_level_idc = pbVideoData[12];
            tRtmpH265Extradata.min_spatial_segmentation_idc = ((pbVideoData[13] & 0x0F) << 8) | pbVideoData[14];
            tRtmpH265Extradata.parallelismType = pbVideoData[15] & 0x03;
            tRtmpH265Extradata.chromaFormat = pbVideoData[16] & 0x03;
            tRtmpH265Extradata.bitDepthLumaMinus8 = pbVideoData[17] & 0x07;
            tRtmpH265Extradata.bitDepthChromaMinus8 = pbVideoData[18] & 0x07;
            tRtmpH265Extradata.avgFrameRate = (pbVideoData[19] << 8) | pbVideoData[20];
            tRtmpH265Extradata.constantFrameRate = (pbVideoData[21] >> 6) & 0x03;
            tRtmpH265Extradata.numTemporalLayers = (pbVideoData[21] >> 3) & 0x07;
            tRtmpH265Extradata.temporalIdNested = (pbVideoData[21] >> 2) & 0x01;
            tRtmpH265Extradata.lengthSizeMinusOne = pbVideoData[21] & 0x03;
            numOfArrays = pbVideoData[22];

            pbVideoParams = &pbVideoData[23];
            for (i = 0; i < numOfArrays; i++)
            {
                nalutype = pbVideoParams[0];
                numOfVideoParameterSets = (pbVideoParams[1] << 8) | pbVideoParams[2];
                pbVideoParams += 3;
                for (j = 0; j < numOfVideoParameterSets; j++)
                {
                    lenOfVideoParameterSets = (pbVideoParams[0] << 8) | pbVideoParams[1];
                    switch(nalutype & 0x3F)
                    {
                        case 32:
                        {//VPS
                            m_ptRtmpFrameInfo->wVpsLen= lenOfVideoParameterSets;
                            memcpy(m_ptRtmpFrameInfo->abVPS,&pbVideoParams[2],m_ptRtmpFrameInfo->wVpsLen);
                            break;
                        }
                        case 33:
                        {//SPS
                            m_ptRtmpFrameInfo->wSpsLen= lenOfVideoParameterSets;
                            memcpy(m_ptRtmpFrameInfo->abSPS,&pbVideoParams[2],m_ptRtmpFrameInfo->wSpsLen);
                            break;
                        }
                        case 34:
                        {//PPS
                            m_ptRtmpFrameInfo->wPpsLen = lenOfVideoParameterSets;
                            memcpy(m_ptRtmpFrameInfo->abPPS,&pbVideoParams[2],m_ptRtmpFrameInfo->wPpsLen);
                            break;
                        }
                        default:
                        {
                            RTMP_LOGE("nalutype & 0x3F err %d\r\n",nalutype & 0x3F);
                            break;
                        }

                    }
                    tRtmpH265Extradata.numOfArrays++;
                    pbVideoParams += 2 + lenOfVideoParameterSets;
                }
            }
        }
    }
    else
    {
        iVideoDataLen = -1;
    }
    return iVideoDataLen;
}


/*****************************************************************************
-Fuction        : GetVideoDataNalu
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GetVideoDataNalu(unsigned char *i_pbVideoData,int i_iDataLen,T_RtmpFrameInfo * m_ptRtmpFrameInfo,unsigned char *o_pbVideoData,int i_iVideoDataMaxLen)
{
    int iVideoDataLen = 0;
    int iProcessedLen = 0;
    int i = 0,j = 0;
    unsigned int dwNaluLen = 0;
    //unsigned char bNaluType = 0;
    
    m_ptRtmpFrameInfo->dwNaluCnt=0;
    for(i=0;i<(int)(sizeof(m_ptRtmpFrameInfo->atNaluInfo)/sizeof(T_RtmpNaluInfo))&& iProcessedLen < i_iDataLen;i++)//flv tag data中有多个nalu size+nalu data
    {
        m_ptRtmpFrameInfo->atNaluInfo[i].pbData = &o_pbVideoData[iVideoDataLen];
        if(m_ptRtmpFrameInfo->eFrameType == RTMP_VIDEO_KEY_FRAME && i == 0)//与ffmpeg h264_mp4toannexb一致
        {//只是第一个nalu前加,一个tag data 中nalu类型是一样的
            memset(&o_pbVideoData[iVideoDataLen],0x00,3);
            iVideoDataLen+=3;
            o_pbVideoData[iVideoDataLen]=1;
            iVideoDataLen++;
            memcpy(&o_pbVideoData[iVideoDataLen], m_ptRtmpFrameInfo->abSPS,m_ptRtmpFrameInfo->wSpsLen);
            iVideoDataLen +=m_ptRtmpFrameInfo->wSpsLen;
            
            memset(&o_pbVideoData[iVideoDataLen],0x00,3);
            iVideoDataLen+=3;
            o_pbVideoData[iVideoDataLen]=1;
            iVideoDataLen++;
            memcpy(&o_pbVideoData[iVideoDataLen], m_ptRtmpFrameInfo->abPPS,m_ptRtmpFrameInfo->wPpsLen);
            iVideoDataLen +=m_ptRtmpFrameInfo->wPpsLen;
        }
        memset(&o_pbVideoData[iVideoDataLen],0x00,3);
        iVideoDataLen+=3;
        o_pbVideoData[iVideoDataLen]=1;
        iVideoDataLen++;
        
        //while(iProcessedLen < i_iTagLen)//现在只支持data中只有一个nalu的情况
        {
            dwNaluLen = 0;
            for (j = 0; j < m_ptRtmpFrameInfo->bNaluLenSize; j++)
                dwNaluLen = (dwNaluLen << 8) + i_pbVideoData[iProcessedLen+j];
            iProcessedLen+= m_ptRtmpFrameInfo->bNaluLenSize;
            
            //if(bNaluType != i_pbVideoTag[iProcessedLen])//兼容只有一个nalu的情况
            {
                //bNaluType = i_pbVideoTag[iProcessedLen];//
            }
            //else//这样处理无法被解码
            {
                //iProcessedLen+=1;//偏移nalu type NaluSliceHeader
                //dwNaluLen-=1;//
            }
            
            memcpy(&o_pbVideoData[iVideoDataLen], &i_pbVideoData[iProcessedLen],dwNaluLen);
            iVideoDataLen += dwNaluLen;
            iProcessedLen+=dwNaluLen;
        }
        
        m_ptRtmpFrameInfo->atNaluInfo[i].dwDataLen = &o_pbVideoData[iVideoDataLen]-m_ptRtmpFrameInfo->atNaluInfo[i].pbData;
        m_ptRtmpFrameInfo->dwNaluCnt++;
    }
    return iVideoDataLen;
}

/*****************************************************************************
-Fuction        : GetAudioData
-Description    : FLV格式音频数据解析
-Input          : 
-Output         : 
-Return         : 不支持的编码格式返回-1，只有序列头返回0，完整数据则返回长度
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::GetAudioData(unsigned char *i_pbAudioTag,int i_iTagLen,T_RtmpAudioParam * m_ptRtmpAudioParam,unsigned char *o_pbAudioData,int i_iAudioDataMaxLen)
{
    int iAudioDataLen = 0;
    unsigned char bIsAACSeqHeader = 0;
    T_RtmpAudioParam tRtmpAudioParam;
    int iProcessedLen = 0;
    
    if(NULL == i_pbAudioTag || NULL == m_ptRtmpAudioParam || NULL == o_pbAudioData)
    {
        RTMP_LOGE("GetAudioData NULL %d\r\n", i_iTagLen);
        return -1;
    }
    //tag Header 1 byte
    memset(&tRtmpAudioParam,0,sizeof(T_RtmpAudioParam));
    ParseAudioDataTagHeader(i_pbAudioTag[iProcessedLen],&tRtmpAudioParam);
    if(RTMP_UNKNOW_ENC_TYPE == tRtmpAudioParam.eEncType ||RTMP_ENC_OPUS == tRtmpAudioParam.eEncType)
    {
        RTMP_LOGE("RTMP_UNKNOW_ENC_TYPE %d\r\n", tRtmpAudioParam.eEncType);//OPUS暂不支持,后续再做
        return -1;
    }
    iProcessedLen++;

    if(RTMP_ENC_AAC == tRtmpAudioParam.eEncType)
    {
        bIsAACSeqHeader = i_pbAudioTag[iProcessedLen] == 0 ? 1:0;//tag Body(AAC packet type) 1 byte
        iProcessedLen++;
        //tag Body(AAC SeqHeader or AAC Raw)
        if(0 == bIsAACSeqHeader)
        {
            iAudioDataLen += AddAdtsHeader(m_ptRtmpAudioParam->dwSamplesPerSecond,m_ptRtmpAudioParam->dwChannels,
            i_iTagLen-iProcessedLen,o_pbAudioData,i_iAudioDataMaxLen);
            memcpy(o_pbAudioData+iAudioDataLen, &i_pbAudioTag[iProcessedLen],i_iTagLen-iProcessedLen);
            iAudioDataLen +=i_iTagLen-iProcessedLen;
        }
        else
        {
            unsigned char bProfile = 0; // 0-NULL, 1-AAC Main, 2-AAC LC, 2-AAC SSR, 3-AAC LTP
            unsigned char bSamplingFreqIndex = 0; // 0-96000, 1-88200, 2-64000, 3-48000, 4-44100, 5-32000, 6-24000, 7-22050, 8-16000, 9-12000, 10-11025, 11-8000, 12-7350, 13/14-reserved, 15-frequency is written explictly
            unsigned char bChannelConf = 0; // 0-AOT, 1-1channel,front-center, 2-2channels, front-left/right, 3-3channels: front center/left/right, 4-4channels: front-center/left/right, back-center, 5-5channels: front center/left/right, back-left/right, 6-6channels: front center/left/right, back left/right LFE-channel, 7-8channels
            unsigned int dwSamplingFrequency = 0;  // codec frequency, valid only in decode
            unsigned char bChannel = 0; 
            static const unsigned int s_adwSamplingFreq[13] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };
            static const unsigned char s_abChannels[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };
            
            bProfile = (i_pbAudioTag[iProcessedLen] >> 3) & 0x1F;
            bSamplingFreqIndex = ((i_pbAudioTag[iProcessedLen] & 0x7) << 1) | ((i_pbAudioTag[iProcessedLen+1] >> 7) & 0x01);
            bChannelConf = (i_pbAudioTag[iProcessedLen+1] >> 3) & 0x0F;
            if(bSamplingFreqIndex < 13)
                dwSamplingFrequency = s_adwSamplingFreq[bSamplingFreqIndex];
            if(bChannelConf < 8)
                bChannel = s_abChannels[bChannelConf];
            m_ptRtmpAudioParam->eEncType = tRtmpAudioParam.eEncType;
            m_ptRtmpAudioParam->dwBitsPerSample = tRtmpAudioParam.dwBitsPerSample;
            m_ptRtmpAudioParam->dwSamplesPerSecond = dwSamplingFrequency;
            m_ptRtmpAudioParam->dwChannels = bChannel;
        }
        
    }
    else
    {
        memcpy(m_ptRtmpAudioParam,&tRtmpAudioParam,sizeof(T_RtmpAudioParam));
        memcpy(o_pbAudioData,&i_pbAudioTag[iProcessedLen],i_iTagLen-iProcessedLen);
        iAudioDataLen += i_iTagLen-iProcessedLen;
    }
    return iAudioDataLen;
}

/*****************************************************************************
-Fuction        : ParseAudioDataTagHeader
-Description    : ParseAudioDataTagHeader
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
unsigned char RtmpMediaHandle::ParseAudioDataTagHeader(unsigned char i_bAudioTagHeader,T_RtmpAudioParam * o_ptRtmpAudioParam)
{
    unsigned char bAudioTagHeader = 0;
    unsigned char bEncType = 0;
    unsigned char bSampleRateIndex = 3;// 0-5.5kHz;1-11kHz;2-22kHz;3-44kHz(AAC都是3)
    unsigned char bSampleBits = 0b00;// audio sample bits: 0-8 bit samples, 1-16-bit samples
    unsigned char bChannels = 1;// audio channel count: 0-Mono sound, 1-Stereo sound
    ///索引表  
    static unsigned int const s_adwSampleRateTable[16] = {
      5500, 11000, 22000, 44000,
    };

	bEncType = (i_bAudioTagHeader & 0xF0) >> 4;
	bSampleRateIndex = (i_bAudioTagHeader & 0x0C) >> 2;
	bSampleBits = (i_bAudioTagHeader & 0x02) >> 1;
	bChannels = i_bAudioTagHeader & 0x01;
	
    o_ptRtmpAudioParam->dwChannels = bChannels;
    o_ptRtmpAudioParam->dwBitsPerSample = bSampleBits == 0b01 ? 16 : 8;
    o_ptRtmpAudioParam->dwSamplesPerSecond = 44000;
    if(bSampleRateIndex < 4)
        o_ptRtmpAudioParam->dwSamplesPerSecond = s_adwSampleRateTable[bSampleRateIndex];
    switch(bEncType)
    {
        case 2:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_MP3;
            break;
        }
        case 7:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_G711A;
            break;
        }
        case 8:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_G711U;
            break;
        }
        case 13:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_OPUS;
            break;
        }
        case 0:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_LPCM;
            break;
        }
        case 1:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_ADPCM;
            break;
        }
        case 3:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_LLPCM;
            break;
        }
        case 10:
        default:
        {
            o_ptRtmpAudioParam->eEncType = RTMP_ENC_AAC;
            break;
        }
    }
    return 0;
}

/*****************************************************************************
-Fuction        : AddAdtsHeader
-Description    : AddAdtsHeader
-Input          : 
-Output         : 
-Return         : iVideoDataLen must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::AddAdtsHeader(unsigned int i_dwSampleRate,unsigned int i_dwChannels,int i_iAudioRawDataLen,unsigned char *o_pbAudioData,int i_iDataMaxLen)
{
    int iLen = 0;
    int iAllLen = i_iAudioRawDataLen + 7;
    int iSampleIndex = 0x04;
    
    if (NULL == o_pbAudioData || i_iDataMaxLen < 7) 
    {
        RTMP_LOGE("AddAdtsHeader NULL %d \r\n", i_iDataMaxLen);
        return iLen;
    }

    switch (i_dwSampleRate) 
    {
        case 96000:
            iSampleIndex = 0x00;
            break;
        case 88200:
            iSampleIndex = 0x01;
            break;
        case 64000:
            iSampleIndex = 0x02;
            break;
        case 48000:
            iSampleIndex = 0x03;
            break;
        case 44100:
            iSampleIndex = 0x04;
            break;
        case 32000:
            iSampleIndex = 0x05;
            break;
        case 24000:
            iSampleIndex = 0x06;
            break;
        case 22050:
            iSampleIndex = 0x07;
            break;
        case 16000:
            iSampleIndex = 0x08;
            break;
        case 12000:
            iSampleIndex = 0x09;
            break;
        case 11025:
            iSampleIndex = 0x0a;
            break;
        case 8000:
            iSampleIndex = 0x0b;
            break;
        case 7350:
            iSampleIndex = 0x0c;
            break;
        default:
            break;
    }

    o_pbAudioData[0] = 0xFF;
    o_pbAudioData[1] = 0xF1;
    o_pbAudioData[2] = 0x40 | (iSampleIndex << 2) | (i_dwChannels >> 2);
    o_pbAudioData[3] = ((i_dwChannels & 0x03) << 6) | (iAllLen >> 11);
    o_pbAudioData[4] = (iAllLen >> 3) & 0xFF;
    o_pbAudioData[5] = ((iAllLen << 5) & 0xFF) | 0x1F;
    o_pbAudioData[6] = 0xFC;

    iLen = 7;
    return iLen;
}


/****************************************************************************/

/*****************************************************************************
-Fuction        : ParseFlvHeader
-Description    : ParseFlvHeader
-Input          : 
-Output         : 
-Return         :  must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::ParseFlvHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvHeader * o_ptFlvHeader)
{
	if (i_dwLen < FLV_HEADER_LEN || 'F' != i_pbBuf[0] || 'L' != i_pbBuf[1] || 'V' != i_pbBuf[2])
	{
        RTMP_LOGE("ParseFlvHeader NULL %d \r\n", i_dwLen);
		return -1;
	}
	if (0x00 != (i_pbBuf[4] & 0xF8) || 0x00 != (i_pbBuf[4] & 0x20))
	{
        RTMP_LOGE("ParseFlvHeader err %d \r\n", i_pbBuf[4]);//预留为0
		return -1;
	}
	
	o_ptFlvHeader->FLV[0] = i_pbBuf[0];
	o_ptFlvHeader->FLV[1] = i_pbBuf[1];
	o_ptFlvHeader->FLV[2] = i_pbBuf[2];
	o_ptFlvHeader->bVersion = i_pbBuf[3];
	o_ptFlvHeader->bAudio = (i_pbBuf[4] >> 2) & 0x01;
	o_ptFlvHeader->bVideo = i_pbBuf[4] & 0x01;
	Read32BE((i_pbBuf + 5),&o_ptFlvHeader->dwOffset);

	return FLV_HEADER_LEN;
}

/*****************************************************************************
-Fuction        : FlvReadHeader
-Description    : FlvReadHeader
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::FlvReadHeader(unsigned char* i_pbBuf,unsigned int i_dwLen)
{
    unsigned int dwPreviousTagSize0;
    T_FlvHeader tFlvHeader;

	if (i_dwLen < FLV_HEADER_LEN ||NULL == i_pbBuf)
	{
        RTMP_LOGE("FlvReadHeader NULL %d \r\n", i_dwLen);
		return -1;
	}
	memset(&tFlvHeader,0,sizeof(T_FlvHeader));
	if(FLV_HEADER_LEN != ParseFlvHeader(i_pbBuf,i_dwLen,&tFlvHeader))
    {
        RTMP_LOGE("FlvReadHeader err %d \r\n", i_dwLen);
        return -1;
    }
	if (tFlvHeader.dwOffset < FLV_HEADER_LEN ||tFlvHeader.dwOffset > FLV_HEADER_LEN + 4096)
	{
        RTMP_LOGE("FlvReadHeader bOffset err %d \r\n", tFlvHeader.dwOffset);
		return -1;
	}

	// PreviousTagSize0
	Read32BE((i_pbBuf + tFlvHeader.dwOffset),&dwPreviousTagSize0);
	if(0 != dwPreviousTagSize0)
    {
        RTMP_LOGE("FlvReadHeader dwPreviousTagSize0 err %d \r\n", dwPreviousTagSize0);
        return -1;
    }
	return tFlvHeader.dwOffset+FLV_PRE_TAG_LEN;
}


/*****************************************************************************
-Fuction        : ParseFlvHeader
-Description    : ParseFlvHeader
-Input          : 
-Output         : 
-Return         :  must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::ParseFlvTagHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvTagHeader * o_ptFlvTagHeader)
{
    if (i_dwLen < FLV_TAG_HEADER_LEN ||NULL == i_pbBuf ||NULL == o_ptFlvTagHeader)
    {
        RTMP_LOGE("ParseFlvTagHeader NULL %d \r\n", i_dwLen);
        return -1;
    }

	// TagType
	o_ptFlvTagHeader->bType = i_pbBuf[0] & 0x1F;
	o_ptFlvTagHeader->bFilter = (i_pbBuf[0] >> 5) & 0x01;
    if (FLV_TAG_AUDIO_TYPE != o_ptFlvTagHeader->bType && FLV_TAG_VIDEO_TYPE != o_ptFlvTagHeader->bType && 
    FLV_TAG_SCRIPT_TYPE != o_ptFlvTagHeader->bType)
    {
        RTMP_LOGE("ParseFlvTagHeader err %d \r\n", o_ptFlvTagHeader->bType);
        return -1;
    }

	// DataSize
	o_ptFlvTagHeader->dwSize= ((unsigned int)i_pbBuf[1] << 16) | ((unsigned int)i_pbBuf[2] << 8) | i_pbBuf[3];

	// TimestampExtended | Timestamp
	o_ptFlvTagHeader->dwTimestamp= ((unsigned int)i_pbBuf[4] << 16) | ((unsigned int)i_pbBuf[5] << 8) | i_pbBuf[6] | ((unsigned int)i_pbBuf[7] << 24);

	// StreamID Always 0
	o_ptFlvTagHeader->dwStreamId= ((unsigned int)i_pbBuf[8] << 16) | ((unsigned int)i_pbBuf[9] << 8) | i_pbBuf[10];
	//assert(0 == tag->streamId);

	return FLV_TAG_HEADER_LEN;
}

/*****************************************************************************
-Fuction        : ParseFlvHeader
-Description    : ParseFlvHeader
-Input          : 
-Output         : 
-Return         :  must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpMediaHandle::FlvReadTagHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvTag * o_ptFlvTag)
{
	unsigned int dwProcessedLen = 0;
	T_FlvTagHeader tFlvTagHeader;
    unsigned int dwPreviousTagSize;

    if (i_dwLen < FLV_TAG_HEADER_LEN ||NULL == i_pbBuf ||NULL == o_ptFlvTag||NULL == o_ptFlvTag->pbTagData)
    {
        RTMP_LOGE("FlvReadTagHeader NULL %d \r\n", i_dwLen);
        return -1;
    }
    memset(&tFlvTagHeader,0,sizeof(T_FlvTagHeader));
	if (FLV_TAG_HEADER_LEN != ParseFlvTagHeader(i_pbBuf, i_dwLen, &tFlvTagHeader))
    {
        RTMP_LOGE("FlvReadTagHeader err %d \r\n", i_dwLen);
        return -1;
    }

	if (i_dwLen < tFlvTagHeader.dwSize+sizeof(dwPreviousTagSize) || o_ptFlvTag->dwDataMaxLen < tFlvTagHeader.dwSize)
    {
        RTMP_LOGE("FlvReadTagHeader err i_dwLen%d,dwSize%d,dwDataMaxLen%d \r\n", i_dwLen,tFlvTagHeader.dwSize,o_ptFlvTag->dwDataMaxLen);
        return -1;
    }
	if(0 != tFlvTagHeader.dwStreamId)
    {
        RTMP_LOGE("FlvReadTagHeader dwStreamId err %d \r\n", tFlvTagHeader.dwStreamId);// StreamID Always 0
        return -1;
    }
	// PreviousTagSizeN
	Read32BE((i_pbBuf + FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize),&dwPreviousTagSize);
	if(FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize != dwPreviousTagSize)
    {
        RTMP_LOGE("FlvReadTagHeader dwPreviousTagSize err %d \r\n", dwPreviousTagSize);
        return -1;
    }
    
    memcpy(&o_ptFlvTag->tTagHeader,&tFlvTagHeader,sizeof(T_FlvTagHeader));
    memcpy(o_ptFlvTag->pbTagData,i_pbBuf+FLV_TAG_HEADER_LEN,tFlvTagHeader.dwSize);
    o_ptFlvTag->dwCurrentSize = dwPreviousTagSize;
    
    dwProcessedLen = FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize+sizeof(dwPreviousTagSize);
    return dwProcessedLen;
}

