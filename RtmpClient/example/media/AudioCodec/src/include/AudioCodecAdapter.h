/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	AudioCodecAdapter.h
* Description		: 	AudioCodecAdapter operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef AUDIO_CODEC_ADAPTER_H
#define AUDIO_CODEC_ADAPTER_H

#include <stdlib.h>
#include <stdio.h>
#include <string>


#define  AC_LOGW(...)     printf(__VA_ARGS__)
#define  AC_LOGE(...)     printf(__VA_ARGS__)
#define  AC_LOGD(...)     printf(__VA_ARGS__)
#define  AC_LOGI(...)     printf(__VA_ARGS__)


typedef enum AudioCodecType
{
    AUDIO_CODEC_TYPE_UNKNOW=0,
    AUDIO_CODEC_TYPE_WAV_PCM,//wav data 为 pcm
    AUDIO_CODEC_TYPE_WAV_PCMA,//wav data 为 pcma
    AUDIO_CODEC_TYPE_WAV_PCMU,//wav data 为 pcmu
    AUDIO_CODEC_TYPE_PCM,
    AUDIO_CODEC_TYPE_PCMA, // G711A
    AUDIO_CODEC_TYPE_PCMU, // G711U
    AUDIO_CODEC_TYPE_AAC,
} E_AudioCodecType;//

typedef struct AudioCodecParam
{
	//unsigned char * pbSrcBuf;        // 
	//unsigned int dwSrcBufLen;         // 
	//unsigned char * pbDstBuf;        // 
	//unsigned int dwDstBufLen;         // 
	//unsigned int dwDstBufMaxLen;         // 
	
	E_AudioCodecType eAudioCodecType;         // 
	unsigned int dwSampleRate;         // 
	unsigned int dwChannels;         // 
	unsigned int dwBitsPerSample;         // 
} T_AudioCodecParam;//


/*****************************************************************************
-Class			: Buffer
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class Buffer
{
public:
    Buffer(int i_iBufMaxLen)
    {
        pbBuf = new unsigned char [i_iBufMaxLen];
        iBufLen = 0;
        iBufMaxLen = i_iBufMaxLen;
    }
    virtual ~Buffer()
    {
        delete [] pbBuf;
    }
    void Copy(unsigned char * i_pbSrcData,int i_iSrcDataLen)
    {
        if(i_iSrcDataLen+iBufLen<=iBufMaxLen)
        {
            memcpy(pbBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
            iBufLen += i_iSrcDataLen;
            return;
        }
        unsigned char * pBuf = new unsigned char [i_iSrcDataLen+iBufLen];
        memcpy(pBuf,pbBuf,iBufLen);
        memcpy(pBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
        delete [] pbBuf;
        pbBuf=pBuf;
        iBufLen = i_iSrcDataLen+iBufLen;
        iBufMaxLen = i_iSrcDataLen+iBufLen;
    }
    void Delete(int i_iProcessedLen)
    {
        if(i_iProcessedLen>=iBufLen)
        {
            iBufLen=0;
            return;
        }
        memmove(pbBuf,pbBuf+i_iProcessedLen,iBufLen-i_iProcessedLen);
        iBufLen = iBufLen-i_iProcessedLen;
    }
    unsigned char * pbBuf;//只允许只读操作
    int iBufLen;//只允许只读操作
    int iBufMaxLen;
};

/*****************************************************************************
-Class			: AudioCodecInterface
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class AudioCodecInterface
{
public:
    virtual int Init()=0;
    virtual int InitEncode(T_AudioCodecParam i_tSrcCodecParam)=0;
    virtual int Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)=0;//输入为未压缩的裸音频数据
    virtual int InitDecode()=0;
    virtual int Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)=0;//输出为未压缩的裸音频数据
};





#endif

