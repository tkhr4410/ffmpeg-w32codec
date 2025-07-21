// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "dsfilter.h"

typedef struct {
	REFGUID		subtype;
	AVCodecID	codec_id;
} codec_map_t;

typedef struct {
	REFGUID			subtype;
	AVPixelFormat	pix_fmt;
	WORD			planes;
	WORD			bpp;
} pixfmt_map_t;

static codec_map_t g_codec_map_video[] = {
	{ MEDIASUBTYPE_MPEG1Payload,		AV_CODEC_ID_MPEG1VIDEO },
	{ MEDIASUBTYPE_NULL,				AV_CODEC_ID_NONE }
};

static codec_map_t g_codec_map_audio[] = {
	{ MEDIASUBTYPE_MPEG1AudioPayload,	AV_CODEC_ID_MP1 },
	{ MEDIASUBTYPE_MPEG2_AUDIO,			AV_CODEC_ID_MP2 },
	{ MEDIASUBTYPE_NULL,				AV_CODEC_ID_NONE }
};

static pixfmt_map_t g_pixfmt_map[] = {
	{ MEDIASUBTYPE_RGB32, AV_PIX_FMT_RGB32, 1, 32 },
	{ MEDIASUBTYPE_RGB24, AV_PIX_FMT_RGB24, 1, 24 },
};
static size_t g_pixfmt_map_cnt = ARRAYSIZE(g_pixfmt_map);

static REFGUID find_subtype(const codec_map_t *map, AVCodecID codec_id)
{
	if (codec_id == AV_CODEC_ID_NONE) {
		return MEDIASUBTYPE_NULL;
	}

	for (int i=0; map[i].codec_id != AV_CODEC_ID_NONE; i++) {
		if (map[i].codec_id == codec_id) {
			return map[i].subtype;
		}
	}

	LOGE("%s: %d -> MEDIASUBTYPE_NULL", __FUNCTION__, codec_id);

	return MEDIASUBTYPE_NULL;
}

static AVCodecID find_codec_id(const codec_map_t *map, REFGUID subtype)
{
	OLECHAR *type_str;

	if (subtype == MEDIASUBTYPE_NULL) {
		return AV_CODEC_ID_NONE;
	}

	for (int i=0; map[i].codec_id != AV_CODEC_ID_NONE; i++) {
		if (map[i].subtype == subtype) {
			return map[i].codec_id;
		}
	}

	StringFromCLSID(subtype, &type_str);
	LOGE("%s: subtype=%ls -> AV_CODEC_ID_NONE", __FUNCTION__, type_str);
	CoTaskMemFree(type_str);

	return AV_CODEC_ID_NONE;
}

REFGUID get_video_subtype(AVCodecID codec_id)
{
	return find_subtype(g_codec_map_video, codec_id);
}

REFGUID get_audio_subtype(AVCodecID codec_id)
{
	return find_subtype(g_codec_map_audio, codec_id);
}

AVCodecID get_video_codec_id(REFGUID subtype)
{
	return find_codec_id(g_codec_map_video, subtype);
}

AVCodecID get_audio_codec_id(REFGUID subtype)
{
	return find_codec_id(g_codec_map_audio, subtype);
}

bool is_subtype_video_out(REFGUID subtype)
{
	if (subtype == MEDIASUBTYPE_NULL) {
		return false;
	}

	for (int i=0; g_pixfmt_map[i].subtype != MEDIASUBTYPE_NULL; i++) {
		if (g_pixfmt_map[i].subtype == subtype) {
			return true;
		}
	}

	return false;
}

bool is_subtype_audio_out(REFGUID subtype)
{
	return (subtype == MEDIASUBTYPE_PCM);
}

AVPixelFormat get_pixel_format(REFGUID subtype)
{
	OLECHAR *type_str;

	if (subtype == MEDIASUBTYPE_NULL) {
		return AV_PIX_FMT_NONE;
	}

	for (unsigned int i=0; g_pixfmt_map_cnt; i++) {
		if (g_pixfmt_map[i].subtype == subtype) {
			return g_pixfmt_map[i].pix_fmt;
		}
	}

	StringFromCLSID(subtype, &type_str);
	LOGE("%s: subtype=%ls -> AV_CODEC_ID_NONE", __FUNCTION__, type_str);
	CoTaskMemFree(type_str);

	return AV_PIX_FMT_NONE;
}

REFGUID get_subtype_video_out(unsigned int index)
{
	if (index >= g_pixfmt_map_cnt) {
		return GUID_NULL;
	}
	return g_pixfmt_map[index].subtype;
}

REFGUID get_subtype_audio_out(unsigned int index)
{
	if (index != 0) {
		return GUID_NULL;
	}
	return MEDIASUBTYPE_PCM;
}

bool get_format_video_out(unsigned int index, VIDEOINFOHEADER *vih)
{
	if (index >= g_pixfmt_map_cnt) {
		return false;
	}

	vih->bmiHeader.biPlanes = g_pixfmt_map[index].planes;
	vih->bmiHeader.biBitCount = g_pixfmt_map[index].bpp;
	vih->bmiHeader.biCompression = BI_RGB;
	vih->bmiHeader.biSizeImage =
		((vih->bmiHeader.biWidth * vih->bmiHeader.biBitCount + 31) & ~31) / 8
		* vih->bmiHeader.biHeight;

	return true;
}

bool get_format_audio_out(unsigned int index, WAVEFORMATEX *wfx)
{
	if (index != 0) {
		return false;
	}

	wfx->wFormatTag = WAVE_FORMAT_PCM;
	wfx->wBitsPerSample = 16;
	wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample / 8;
	wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

	return true;
}

WORD get_wave_format_tag(AVCodecID codec_id)
{
	switch (codec_id)
	{
	case AV_CODEC_ID_PCM_S16LE:		return 0x0001;
	case AV_CODEC_ID_PCM_U8:		return 0x0001;
	case AV_CODEC_ID_MP1:			return 0x0050;
	case AV_CODEC_ID_MP2:			return 0x0050;
	case AV_CODEC_ID_MP3:			return 0x0055;
	default:
		LOGE("%s: unsupported codec id %d", __FUNCTION__, codec_id);
		return 0x0000;
	}
}

AVSampleFormat get_sample_format(const WAVEFORMATEX *wfx)
{
	if (wfx->wFormatTag != WAVE_FORMAT_PCM) {
		return AV_SAMPLE_FMT_NONE;
	}
	switch (wfx->wBitsPerSample)
	{
	case 8:		return AV_SAMPLE_FMT_U8;
	case 16:	return AV_SAMPLE_FMT_S16;
	default:	return AV_SAMPLE_FMT_NONE;
	}
}
