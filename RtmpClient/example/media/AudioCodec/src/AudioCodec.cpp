/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	AudioCodec.cpp
* Description		: 	AudioCodec operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>
#include "AudioCodec.h"
#include "CodecAAC.h"
#include "CodecWAV.h"
#include "CodecG711A.h"
#include "CodecG711U.h"
#include "CodecPCM.h"

#define CODEC_DECODE_BUF_MAX_LEN 10240

/*****************************************************************************
-Fuction		: AudioCodec
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
AudioCodec::AudioCodec()
{
    //m_pbDecodeBuf=new unsigned char [CODEC_DECODE_BUF_MAX_LEN];
    m_pbDecodeBuf=NULL;
    m_ptEncoder=NULL;
    m_ptDecoder=NULL;
    m_pEncoderHeader=NULL;
    m_pSubDecoder=NULL;
    m_pbEncHeaderBuf=NULL;
    m_pbSubDecBuf=NULL;
    m_pRawDataHandle=NULL;
}
/*****************************************************************************
-Fuction		: ~AudioCodec
-Description	: ~AudioCodec
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
AudioCodec::~AudioCodec()
{
    if(NULL != m_ptEncoder)
    {
        delete m_ptEncoder;
        m_ptEncoder=NULL;
    }
    if(NULL != m_ptDecoder)
    {
        delete m_ptDecoder;
        m_ptDecoder=NULL;
    }
    if(NULL != m_pbDecodeBuf)
    {
        delete [] m_pbDecodeBuf;
        m_pbDecodeBuf=NULL;
    }
    if(NULL != m_pEncoderHeader)
    {
        delete m_pEncoderHeader;
        m_pEncoderHeader=NULL;
    }
    if(NULL != m_pSubDecoder)
    {
        delete m_pSubDecoder;
        m_pSubDecoder=NULL;
    }
    if(NULL != m_pbEncHeaderBuf)
    {
        delete m_pbEncHeaderBuf;
        m_pbEncHeaderBuf=NULL;
    }
    if(NULL != m_pbSubDecBuf)
    {
        delete m_pbSubDecBuf;
        m_pbSubDecBuf=NULL;
    }
    if(NULL != m_pRawDataHandle)
    {
        delete ((CodecPCM *)m_pRawDataHandle);
        m_pRawDataHandle=NULL;
    }
}

/*****************************************************************************
-Fuction		: AudioCodec::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::Init()
{

	return 0;
}

/*****************************************************************************
-Fuction		: AudioCodec::Transcode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::Transcode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,T_AudioCodecParam i_tSrcCodecParam,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam i_tDstCodecParam)
{
    int iRet = -1;
    int iDecodeLen = 0;
    T_AudioCodecParam tCodecParam;
    
    if(NULL == o_abDstBuf)
    {
        AC_LOGE("Transcode err NULL%d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    memset(&tCodecParam,0,sizeof(T_AudioCodecParam));
    if(NULL == m_ptDecoder)
    {
        iRet = InitDecode(i_tSrcCodecParam.eAudioCodecType);
        if(iRet < 0)
        {
            AC_LOGE("Transcode InitDecode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
    }
    if(NULL != i_abSrcBuf)
    {
        if(NULL == m_pbDecodeBuf)
            m_pbDecodeBuf=new unsigned char [i_iSrcBufLen*12];
        iRet = Decode(i_abSrcBuf,i_iSrcBufLen,m_pbDecodeBuf,i_iSrcBufLen*12,&tCodecParam);
        if(iRet <= 0)
        {
            AC_LOGE("Transcode Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
        iDecodeLen = iRet;//输入的全部解码出来
        if(AUDIO_CODEC_TYPE_UNKNOW==tCodecParam.eAudioCodecType)
        {
            memcpy(&tCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
        }
        if(tCodecParam.dwSampleRate==0)
        {
            tCodecParam.dwSampleRate=i_tSrcCodecParam.dwSampleRate;
        }
        if(tCodecParam.dwBitsPerSample==0)
        {
            tCodecParam.dwBitsPerSample=i_tSrcCodecParam.dwBitsPerSample;
        }
        if(tCodecParam.dwChannels==0)
        {
            tCodecParam.dwChannels=i_tSrcCodecParam.dwChannels;
        }
    }
    if(NULL == m_pRawDataHandle)
    {
        if(i_tDstCodecParam.dwSampleRate==0)
        {
            i_tDstCodecParam.dwSampleRate=tCodecParam.dwSampleRate;
        }
        if(i_tDstCodecParam.dwBitsPerSample==0)
        {
            i_tDstCodecParam.dwBitsPerSample=tCodecParam.dwBitsPerSample;
        }
        if(i_tDstCodecParam.dwChannels==0)
        {
            i_tDstCodecParam.dwChannels=tCodecParam.dwChannels;
        }
        m_pRawDataHandle = new CodecPCM();
        ((CodecPCM *)m_pRawDataHandle)->Init(tCodecParam,i_tDstCodecParam);
    }

    if(NULL == m_ptEncoder)
    {
        iRet = InitEncode(i_tDstCodecParam);//解码出来的数据认为都是pcm裸数据，所以直接编码
        if(iRet < 0)
        {
            AC_LOGE("Transcode InitEncode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
    }
    if(NULL == i_abSrcBuf)
    {
        iRet = Encode(NULL,0,o_abDstBuf,i_iDstBufMaxLen);
        if(iRet <= 0)
        {
            AC_LOGE("Transcode NULL Encode %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
        return iRet;
    }
    iDecodeLen = ((CodecPCM *)m_pRawDataHandle)->TransSampleRate(m_pbDecodeBuf,iDecodeLen,i_iSrcBufLen*12);
    iRet = Encode(m_pbDecodeBuf,iDecodeLen,o_abDstBuf,i_iDstBufMaxLen);
    if(iRet <= 0)
    {
        AC_LOGE("Transcode Encode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
	return iRet;
}

/*****************************************************************************
-Fuction		: AudioCodec::InitEncode
-Description	: 
-Input			: i_tCodecParam 输入数据的编码参数
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::InitEncode(T_AudioCodecParam i_tCodecParam)
{
    switch(i_tCodecParam.eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_WAV_PCM :
        {
            //i_tCodecParam.eAudioCodecType=AUDIO_CODEC_TYPE_PCM;//传入wav被封装的数据
            m_ptEncoder=new CodecWAV();
            break;
        }
        case AUDIO_CODEC_TYPE_WAV_PCMA :
        {
            m_pEncoderHeader=new CodecWAV();
            m_ptEncoder=new CodecG711A();
            break;
        }
        case AUDIO_CODEC_TYPE_WAV_PCMU :
        {
            m_pEncoderHeader=new CodecWAV();
            m_ptEncoder=new CodecG711U();
            break;
        }
        case AUDIO_CODEC_TYPE_PCMA :
        {
            m_ptEncoder=new CodecG711A();
            break;
        }
        case AUDIO_CODEC_TYPE_PCMU :
        {
            m_ptEncoder=new CodecG711A();
            break;
        }
        case AUDIO_CODEC_TYPE_AAC :
        {
            m_ptEncoder=new CodecAAC();
            break;
        }
        default :
        {
            AC_LOGE("InitEncode.eAudioCodecType err %d\r\n",i_tCodecParam.eAudioCodecType);
            return -1;
        }
    }
    if(NULL != m_pEncoderHeader)
        return m_pEncoderHeader->InitEncode(i_tCodecParam);
    if(NULL != m_ptEncoder)
        return m_ptEncoder->InitEncode(i_tCodecParam);
	return 0;
}

/*****************************************************************************
-Fuction		: AudioCodec::Encode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: <0 err,=0 need more data,>0 DstBufLen
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet=-1;
    int iLen=0;
    
    if(NULL == o_abDstBuf || NULL == m_ptEncoder)
    {
        AC_LOGE("AudioCodec Encode NULL %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    iRet=m_ptEncoder->Encode(i_abSrcBuf,i_iSrcBufLen,o_abDstBuf,i_iDstBufMaxLen);
    if(iRet<=0)
    {
        AC_LOGE("AudioCodec Encode NULL err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    
    if(NULL != m_pEncoderHeader)
    {
        iLen=iRet;
        if(NULL == m_pbEncHeaderBuf)
        {
            m_pbEncHeaderBuf = new Buffer(i_iSrcBufLen);
            m_pbEncHeaderBuf->Copy(o_abDstBuf,iLen);
        }
        else
        {
            m_pbEncHeaderBuf->Copy(o_abDstBuf,iLen);
        }
        iRet=m_pEncoderHeader->Encode(m_pbEncHeaderBuf->pbBuf,m_pbEncHeaderBuf->iBufLen,o_abDstBuf,i_iDstBufMaxLen);

        m_pbEncHeaderBuf->Delete(m_pbEncHeaderBuf->iBufLen);
    }
    
    return iRet;
}

/*****************************************************************************
-Fuction		: AudioCodec::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::InitDecode(E_AudioCodecType i_eAudioCodecType)
{
    switch(i_eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_WAV_PCM :
        case AUDIO_CODEC_TYPE_WAV_PCMA :
        case AUDIO_CODEC_TYPE_WAV_PCMU :
        {
            m_ptDecoder=new CodecWAV();
            break;
        }
        case AUDIO_CODEC_TYPE_PCMA :
        {
            m_ptDecoder=new CodecG711A();
            break;
        }
        case AUDIO_CODEC_TYPE_PCMU :
        {
            m_ptDecoder=new CodecG711U();
            break;
        }
        case AUDIO_CODEC_TYPE_AAC :
        {
            m_ptDecoder=new CodecAAC();
            break;
        }
        default:
        {
            AC_LOGE("InitDecode.eAudioCodecType err %d\r\n",i_eAudioCodecType);
            return -1;
        }
    }
    if(NULL != m_ptDecoder)
        return m_ptDecoder->InitDecode();
	return 0;
}

/*****************************************************************************
-Fuction		: AudioCodec::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: <0 err,=0 need more data,>0 DstBufLen
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int AudioCodec::Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)
{
    int iRet=-1;
    int iLen=0;

    if(NULL == i_abSrcBuf || NULL == o_abDstBuf ||NULL == m_ptDecoder)
    {
        AC_LOGE("AudioCodec Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    iRet=m_ptDecoder->Decode(i_abSrcBuf,i_iSrcBufLen,o_abDstBuf,i_iDstBufMaxLen,o_ptCodecParam);
    
    if(AUDIO_CODEC_TYPE_PCMA==o_ptCodecParam->eAudioCodecType)
    {
        m_pSubDecoder = new CodecG711A();
    }
    else if(AUDIO_CODEC_TYPE_PCMU==o_ptCodecParam->eAudioCodecType)
    {
        m_pSubDecoder = new CodecG711U();
    }
    if(NULL!=m_pSubDecoder)
    {
        iLen=iRet;
        if(NULL == m_pbSubDecBuf)
        {
            m_pbSubDecBuf = new Buffer(i_iDstBufMaxLen);
            m_pbSubDecBuf->Copy(o_abDstBuf,iLen);
        }
        else
        {
            m_pbSubDecBuf->Copy(o_abDstBuf,iLen);
        }
        iRet=m_pSubDecoder->Decode(m_pbSubDecBuf->pbBuf,m_pbSubDecBuf->iBufLen,o_abDstBuf,i_iDstBufMaxLen,o_ptCodecParam);
        m_pbSubDecBuf->Delete(m_pbSubDecBuf->iBufLen);
    }
    return iRet;
}

