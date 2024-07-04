/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpMediaHandle.h
* Description       :   RtmpMediaHandle operation center
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_MEDIA_HANDLE_H
#define RTMP_MEDIA_HANDLE_H 

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "RtmpAdapter.h"

#define RTMP_SPS_MAX_SIZE 128
#define RTMP_PPS_MAX_SIZE 64
#define RTMP_VPS_MAX_SIZE 256

typedef enum
{
    RTMP_UNKNOW_FRAME = 0,
    RTMP_VIDEO_KEY_FRAME,
    RTMP_VIDEO_INNER_FRAME,
    RTMP_AUDIO_FRAME,
}E_RTMP_FRAME_TYPE;
typedef enum
{
    RTMP_UNKNOW_ENC_TYPE = 0,
    RTMP_ENC_H264,
    RTMP_ENC_H265,
    RTMP_ENC_G711A,// G711 A-law
    RTMP_ENC_G711U,// G711 mu-law
    RTMP_ENC_AAC,
    RTMP_ENC_OPUS,
    RTMP_ENC_LPCM,// Linear PCM, platform endian
    RTMP_ENC_ADPCM,
    RTMP_ENC_MP3,
    RTMP_ENC_LLPCM,// Linear PCM, little endian
}E_RTMP_ENC_TYPE;

typedef struct RtmpHevcDecoderConfigurationRecord
{
    unsigned char  configurationVersion;    // 1-only
    unsigned char  general_profile_space;   // 2bit,[0,3]
    unsigned char  general_tier_flag;       // 1bit,[0,1]
    unsigned char  general_profile_idc; // 5bit,[0,31]
    unsigned int general_profile_compatibility_flags;//uint32_t
    uint64 general_constraint_indicator_flags;//uint64_t
    unsigned char  general_level_idc;
    unsigned short min_spatial_segmentation_idc;
    unsigned char  parallelismType;     // 2bit,[0,3]
    unsigned char  chromaFormat;            // 2bit,[0,3]
    unsigned char  bitDepthLumaMinus8;  // 3bit,[0,7]
    unsigned char  bitDepthChromaMinus8;    // 3bit,[0,7]
    unsigned short avgFrameRate;
    unsigned char  constantFrameRate;       // 2bit,[0,3]
    unsigned char  numTemporalLayers;       // 3bit,[0,7]
    unsigned char  temporalIdNested;        // 1bit,[0,1]
    unsigned char  lengthSizeMinusOne;  // 2bit,[0,3]

    unsigned char  numOfArrays;

    unsigned int pic_width;
    unsigned int pic_height;
}T_RtmpH265Extradata;

typedef struct RtmpAudioParam
{
    E_RTMP_ENC_TYPE eEncType;
    unsigned int dwChannels;
    unsigned int dwBitsPerSample;
    unsigned int dwSamplesPerSecond;
}T_RtmpAudioParam;

typedef struct RtmpAudioInfo
{
    T_RtmpAudioParam tParam;
    unsigned char *pbAudioData;//包含aac 7字节头，ADTS头
    unsigned int dwAudioDataLen;
    unsigned int dwAudioDataMaxLen;
}T_RtmpAudioInfo;

typedef struct RtmpNaluInfo
{
    unsigned char *pbData;//去掉了00 00 00 01，
    unsigned int dwDataLen;
}T_RtmpNaluInfo;

typedef struct RtmpFrameInfo
{
    E_RTMP_FRAME_TYPE eFrameType;
    E_RTMP_ENC_TYPE eEncType;
    unsigned int dwWidth;//
    unsigned int dwHeight;//
    unsigned short wSpsLen;
    unsigned char abSPS[RTMP_SPS_MAX_SIZE];//包含nalu type ，去掉了00 00 00 01，
    unsigned short wPpsLen;
    unsigned char abPPS[RTMP_PPS_MAX_SIZE];
    unsigned short wVpsLen;
    unsigned char abVPS[RTMP_VPS_MAX_SIZE];
    unsigned char bNaluLenSize;
    T_RtmpNaluInfo atNaluInfo[12];//存在一帧图像切片成多个(nalu)的情况
    unsigned int dwNaluCnt;
    unsigned char *pbNaluData;//去掉了00 00 00 01，缓冲区地址
    unsigned int dwNaluDataLen;//dwNaluCnt个tNaluInfo组成的数据总大小
    unsigned int dwNaluDataMaxLen;//atNaluInfo组成的缓冲区总大小
}T_RtmpFrameInfo;//



/****************************************************************************/
#define FLV_HEADER_LEN		    9	// DataOffset included
#define FLV_PRE_TAG_LEN	        4	// previous tag size
#define FLV_TAG_HEADER_LEN	    11	// StreamID included
#define FLV_TAG_AUDIO_TYPE		8
#define FLV_TAG_VIDEO_TYPE		9
#define FLV_TAG_SCRIPT_TYPE		18

typedef struct FlvHeader
{
	unsigned char FLV[3];
	unsigned char bVersion;
	unsigned char bAudio;
	unsigned char bVideo;
	unsigned int dwOffset; // data offset
}T_FlvHeader;
typedef struct FlvTagHeader
{
	unsigned char bFilter; // 0-No pre-processing required
	unsigned char bType; // 8-audio, 9-video, 18-script data
	unsigned int dwSize; // data size
	unsigned int dwTimestamp;
	unsigned int dwStreamId;
}T_FlvTagHeader;

typedef struct FlvTag
{
	T_FlvTagHeader tTagHeader; 
	unsigned char *pbTagData; 
	unsigned int dwDataMaxLen; 
	unsigned int dwCurrentSize;//presize
}T_FlvTag;

/*****************************************************************************
-Class          : RtmpMediaHandle
-Description    : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
class RtmpMediaHandle
{
public:
    RtmpMediaHandle();
    RtmpMediaHandle(int i_iEnhancedFlag);
    virtual ~RtmpMediaHandle();
    
    int ParseNaluFromFrame(E_RTMP_ENC_TYPE i_eEncType,unsigned char *i_pbVideoData,int i_iVideoDataLen,T_RtmpFrameInfo * o_ptFrameInfo);
    int GenerateVideoData(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData);
    int GenerateAudioData(T_RtmpAudioInfo * i_ptRtmpAudioInfo,int i_iIsAACSeqHeader,unsigned char *o_pbAudioData,int i_iMaxAudioData);
    
    int GetVideoData(unsigned char *i_pbVideoTag,int i_iTagLen,T_RtmpFrameInfo * m_ptRtmpFrameInfo,unsigned char *o_pbVideoData,int i_iVideoDataMaxLen);
    int GetAudioData(unsigned char *i_pbAudioTag,int i_iTagLen,T_RtmpAudioParam * m_ptRtmpAudioParam,unsigned char *o_pbAudioData,int i_iAudioDataMaxLen);



    
    int ParseFlvHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvHeader * o_ptFlvHeader);
    int FlvReadHeader(unsigned char* i_pbBuf,unsigned int i_dwLen);
    int ParseFlvTagHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvTagHeader * o_ptFlvTagHeader);
    int FlvReadTagHeader(unsigned char* i_pbBuf,unsigned int i_dwLen,T_FlvTag * o_ptFlvTag);
private:
    int ParseH264NaluFromFrame(unsigned char *i_pbVideoData,int i_iVideoDataLen,T_RtmpFrameInfo * o_ptFrameInfo);
    int SetH264NaluData(unsigned char i_bNaluType,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_RtmpFrameInfo * o_ptFrameInfo);
    int ParseH265NaluFromFrame(unsigned char *i_pbVideoData,int i_iVideoDataLen,T_RtmpFrameInfo * o_ptFrameInfo);
    int SetH265NaluData(unsigned char i_bNaluType,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_RtmpFrameInfo * o_ptFrameInfo);
    int GenerateVideoDataH264(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData);
    int GenerateVideoDataH265(T_RtmpFrameInfo * i_ptFrameInfo,int i_iIsAvcSeqHeader,unsigned char *o_pbVideoData,int i_iMaxVideoData);
    int GetVideoDataNalu(unsigned char *i_pbVideoData,int i_iDataLen,T_RtmpFrameInfo * m_ptRtmpFrameInfo,unsigned char *o_pbVideoData,int i_iVideoDataMaxLen);
    
    int AnnexbToH265Extradata(T_RtmpFrameInfo * i_ptFrameInfo,T_RtmpH265Extradata *o_ptRtmpH265Extradata);
    int VpsToH265Extradata(unsigned char *i_pbVpsData,unsigned short i_wVpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata);
    int SpsToH265Extradata(unsigned char *i_pbSpsData,unsigned short i_wSpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata);
    int SpsToH264Resolution(unsigned char *i_pbSpsData,unsigned short i_wSpsLen,T_RtmpH265Extradata *o_ptRtmpH265Extradata);
    int DecodeEBSP(unsigned char* nalu, int bytes, unsigned char* sodb);
    int HevcProfileTierLevel(unsigned char* nalu, int bytes, unsigned char maxNumSubLayersMinus1,T_RtmpH265Extradata* hevc);
    unsigned int H264ReadBitByUE(unsigned char* data, int bytes, int* offset);

    unsigned char CreateAudioDataTagHeader(T_RtmpAudioParam * i_ptRtmpAudioParam);
    int CreateAudioSpecCfgAAC(unsigned int i_dwFrequency,unsigned int i_dwChannels,unsigned char *o_pbAudioData);

    unsigned char ParseAudioDataTagHeader(unsigned char i_bAudioTagHeader,T_RtmpAudioParam * o_ptRtmpAudioParam);
    int AddAdtsHeader(unsigned int i_dwSampleRate,unsigned int i_dwChannels,int i_iAudioRawDataLen,unsigned char *o_pbAudioData,int i_iDataMaxLen);

    int m_iEnhancedFlag;//0 否，1是
};










#endif


