/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecAAC.cpp
* Description		: 	CodecAAC operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>
#include "CodecAAC.h"


#define CODEC_AAC_ENCODE_BUF_MAX_LEN 10240

/*****************************************************************************
-Fuction		: CodecAAC
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecAAC::CodecAAC()
{
    //m_pbEncodeBuf=new unsigned char [CODEC_AAC_ENCODE_BUF_MAX_LEN];
    m_pbEncodeBuf=NULL;
    m_iEncodeBufLen=0;
    m_ptEncoder=NULL;
    m_ptDecoder=NULL;
    memset(&m_tEncCodecParam,0,sizeof(T_AudioCodecParam));
}
/*****************************************************************************
-Fuction		: ~CodecAAC
-Description	: ~CodecAAC
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecAAC::~CodecAAC()
{
    AC_LOGD("~CodecAAC %p %p\n", m_ptEncoder,m_ptDecoder);
    if(NULL != m_ptEncoder)
        aacEncClose(&m_ptEncoder);
    if(NULL != m_ptEncoder)
        aacDecoder_Close(m_ptDecoder);
    if(NULL != m_pbEncodeBuf)
        delete m_pbEncodeBuf;
}

/*****************************************************************************
-Fuction		: CodecAAC::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecAAC::Init()
{
    int iRet=-1;

	return iRet;
}

/*****************************************************************************
-Fuction		: CodecAAC::Init
-Description	: 
-Input			: i_tSrcCodecParam 输入数据的编码参数
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecAAC::InitEncode(T_AudioCodecParam i_tSrcCodecParam)
{
    int iRet=-1;
    CHANNEL_MODE eChannelMode;
    int aot=2,afterburner=1,eld_sbr=0,vbr=0,bitrate=64000,adts=1;// 2, 1, 0, 0, 64000, 1); // AAC-LC
    
    if (i_tSrcCodecParam.dwBitsPerSample/8 == 0) //输入数据编码参数要求
    {
        AC_LOGE("Unsupported AudioCodecParam %d %d %d \n", i_tSrcCodecParam.dwBitsPerSample, i_tSrcCodecParam.dwChannels, i_tSrcCodecParam.dwSampleRate);
        return iRet;
    }
    memcpy(&m_tEncCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    AC_LOGD("InitEncode %d %d %d \n", i_tSrcCodecParam.dwBitsPerSample, i_tSrcCodecParam.dwChannels, i_tSrcCodecParam.dwSampleRate);
    switch (i_tSrcCodecParam.dwChannels) 
    {
        case 1: eChannelMode = MODE_1;       break;
        case 2: eChannelMode = MODE_2;       break;
        case 3: eChannelMode = MODE_1_2;     break;
        case 4: eChannelMode = MODE_1_2_1;   break;
        case 5: eChannelMode = MODE_1_2_2;   break;
        case 6: eChannelMode = MODE_1_2_2_1; break;
        default:
            AC_LOGE("Unsupported WAV channels %d\n", i_tSrcCodecParam.dwChannels);
            return iRet;
    }
    if (aacEncOpen(&m_ptEncoder, 0, i_tSrcCodecParam.dwChannels) != AACENC_OK) 
    {
        AC_LOGE("Unable to open encoder\n");
        return iRet;
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_AOT, aot) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the AOT\n");
        return iRet;
    }
    if (aot == 39 && eld_sbr) 
    {
        if (aacEncoder_SetParam(m_ptEncoder, AACENC_SBR_MODE, 1) != AACENC_OK) 
        {
            AC_LOGE("Unable to set SBR mode for ELD\n");
            return iRet;
        }
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_SAMPLERATE, i_tSrcCodecParam.dwSampleRate) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the AACENC_SAMPLERATE %d\n",i_tSrcCodecParam.dwSampleRate);
        return iRet;
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_CHANNELMODE, eChannelMode) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the channel mode\n");
        return iRet;
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_CHANNELORDER, 1) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the wav channel order\n");
        return iRet;
    }
    if (vbr) 
    {
        if (aacEncoder_SetParam(m_ptEncoder, AACENC_BITRATEMODE, vbr) != AACENC_OK) 
        {
            AC_LOGE("Unable to set the VBR bitrate mode\n");
            return iRet;
        }
    } 
    else 
    {
        if (aacEncoder_SetParam(m_ptEncoder, AACENC_BITRATE, bitrate) != AACENC_OK) 
        {
            AC_LOGE("Unable to set the bitrate\n");
            return iRet;
        }
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_TRANSMUX, adts ? 2 : 0) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the ADTS transmux\n");
        return iRet;
    }
    if (aacEncoder_SetParam(m_ptEncoder, AACENC_AFTERBURNER, afterburner) != AACENC_OK) 
    {
        AC_LOGE("Unable to set the afterburner mode\n");
        return iRet;
    }
    if (aacEncEncode(m_ptEncoder, NULL, NULL, NULL, NULL) != AACENC_OK) 
    {
        AC_LOGE("Unable to initialize the encoder\n");
        return iRet;
    }
    //if (aacEncInfo(tEncoder, &info) != AACENC_OK) 
    {
        //fprintf(stderr, "Unable to get the encoder info\n");
        //return 1;
    }

	return 0;
}

/*****************************************************************************
-Fuction		: CodecAAC::Encode
-Description	: 编码不改变采样率，采样位数，声道(通道)
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecAAC::Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet=-1;
    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
    AACENC_InArgs in_args = { 0 };
    AACENC_OutArgs out_args = { 0 };
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    void *in_ptr, *out_ptr;
    AACENC_ERROR err;
    
    if(NULL == o_abDstBuf || (NULL == i_abSrcBuf && (NULL == m_pbEncodeBuf ||m_pbEncodeBuf->iBufLen <= 0)))
    {
        AC_LOGE("CodecAAC Encode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    if(NULL != i_abSrcBuf)
    {
        if(NULL == m_pbEncodeBuf)
        {
            m_pbEncodeBuf = new Buffer(i_iSrcBufLen);
            m_pbEncodeBuf->Copy(i_abSrcBuf,i_iSrcBufLen);
        }
        else
        {
            m_pbEncodeBuf->Copy(i_abSrcBuf,i_iSrcBufLen);
        }
    }

    in_ptr = m_pbEncodeBuf->pbBuf;
    in_size = m_pbEncodeBuf->iBufLen;
    in_elem_size = m_tEncCodecParam.dwBitsPerSample/8;
    //in_elem_size = 1;//in_elem_size = 2; 编码单位字节数(采样位数1 8位,2 16位)
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;
    
    in_args.numInSamples = m_pbEncodeBuf->iBufLen/in_elem_size;
    
    out_ptr = o_abDstBuf;
    out_size = i_iDstBufMaxLen;
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    if ((err = aacEncEncode(m_ptEncoder, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) 
    {
        if (err == AACENC_ENCODE_EOF)
        {
            AC_LOGD("AACENC_ENCODE_EOF %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return 0;
        }
        AC_LOGE("Encoding failed\n");
        return iRet;
    }
    m_pbEncodeBuf->Delete(m_pbEncodeBuf->iBufLen);//里面会存储输入缓存数据(这种使用方式可能会带来噪声，放大原噪声)
    if (out_args.numOutBytes == 0)
    {
        AC_LOGD("out_args.numOutBytes == 0 %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return 0;
    }
    //m_pbEncodeBuf->Delete(out_args.numInSamples*in_elem_size);
    
    iRet=out_args.numOutBytes;
	return iRet;
}

/*****************************************************************************
-Fuction		: CodecAAC::InitDecoder
-Description	: 解码数据必须带adts头
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecAAC::InitDecode()
{
    int iRet=-1;
    
    /*m_ptDecoder = aacDecoder_Open(adts ? TT_MP4_ADTS : TT_MP4_RAW, 1);
    if (!adts) 
    {
        UCHAR *bufArray[] = { info.confBuf };
        if (aacDecoder_ConfigRaw(decoder, (UCHAR**) bufArray, &info.confSize) != AAC_DEC_OK) 
        {
            fprintf(stderr, "Unable to set ASC\n");
            ret = 1;
            goto end;
        }
    }*/
    m_ptDecoder = aacDecoder_Open(TT_MP4_ADTS, 1);
    aacDecoder_SetParam(m_ptDecoder, AAC_CONCEAL_METHOD, 1);
    aacDecoder_SetParam(m_ptDecoder, AAC_PCM_LIMITER_ENABLE, 0);

	return 0;
}

/*****************************************************************************
-Fuction		: CodecAAC::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecAAC::Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)
{
    int iRet=-1;
    int iLen=0;
    if(NULL == i_abSrcBuf || NULL == o_abDstBuf)
    {
        AC_LOGE("CodecAAC Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    AAC_DECODER_ERROR err;
    CStreamInfo *info;
    UINT valid, buffer_size;
    unsigned char *ptr=i_abSrcBuf;
    int size=i_iSrcBufLen;
    unsigned char *decoder_buffer=o_abDstBuf;
    int decoder_buffer_size=i_iDstBufMaxLen;

    do {
        valid = buffer_size = size;
        err = aacDecoder_Fill(m_ptDecoder, (UCHAR**) &ptr, &buffer_size, &valid);
        ptr += buffer_size - valid;
        size -= buffer_size - valid;
        if (err == AAC_DEC_NOT_ENOUGH_BITS)
            continue;
        if (err != AAC_DEC_OK)
            break;
        err = aacDecoder_DecodeFrame(m_ptDecoder, (INT_PCM *) (decoder_buffer+iLen), decoder_buffer_size / sizeof(INT_PCM), 0);
        if (!ptr && err != AAC_DEC_OK)
            break;
        if (err == AAC_DEC_NOT_ENOUGH_BITS)
            continue;
        if (err != AAC_DEC_OK) 
        {
            AC_LOGE("Decoding failed\n");
            return iRet;
        }
        info = aacDecoder_GetStreamInfo(m_ptDecoder);
        //decoder_output(decoder_buffer, info->numChannels * info->frameSize*sizeof(INT_PCM));
        iLen+=info->numChannels * info->frameSize*sizeof(INT_PCM);
        o_ptCodecParam->dwChannels=info->numChannels;
        o_ptCodecParam->dwBitsPerSample=sizeof(INT_PCM)*8;
        o_ptCodecParam->dwSampleRate=info->sampleRate;
        o_ptCodecParam->eAudioCodecType=AUDIO_CODEC_TYPE_PCM;
    } while (size > 0);
    iRet=iLen;
	return iRet;
}

