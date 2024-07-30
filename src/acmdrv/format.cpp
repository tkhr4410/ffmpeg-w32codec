// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "acmdrv.h"

namespace ffmpeg_w32codec { namespace acmdrv {
	static const format_t g_formats_pcm[] = {
		{ 1,  8000,   8000, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2,  8000,  16000, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1,  8000,  16000, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2,  8000,  32000, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 11025,  11025, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 11025,  22050, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 11025,  22050, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 11025,  44100, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 16000,  16000, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 16000,  32000, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 16000,  32000, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 16000,  64000, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 22050,  22050, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 22050,  44100, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 22050,  44100, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 22050,  88200, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 32000,  32000, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 32000,  64000, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 32000,  64000, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 32000, 128000, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 44100,  44100, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 44100,  88200, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 44100,  88200, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 44100, 176400, 4, 16, AV_CODEC_ID_PCM_S16LE },
		{ 1, 48000,  48000, 1,  8, AV_CODEC_ID_PCM_U8 },
		{ 2, 48000,  96000, 2,  8, AV_CODEC_ID_PCM_U8 },
		{ 1, 48000,  96000, 2, 16, AV_CODEC_ID_PCM_S16LE },
		{ 2, 48000, 192000, 4, 16, AV_CODEC_ID_PCM_S16LE },
	};

	static const format_t g_formats_adpcm[] = {
		{ 1,  8000,  4096,  256, 4, AV_CODEC_ID_ADPCM_MS },
		{ 2,  8000,  8192,  512, 4, AV_CODEC_ID_ADPCM_MS },
		{ 1, 11025,  5644,  256, 4, AV_CODEC_ID_ADPCM_MS },
		{ 2, 11025, 11289,  512, 4, AV_CODEC_ID_ADPCM_MS },
		{ 1, 22050, 11155,  512, 4, AV_CODEC_ID_ADPCM_MS },
		{ 2, 22050, 22311, 1024, 4, AV_CODEC_ID_ADPCM_MS },
		{ 1, 44100, 22179, 1024, 4, AV_CODEC_ID_ADPCM_MS },
		{ 2, 44100, 44359, 2048, 4, AV_CODEC_ID_ADPCM_MS },
	};

	static const format_t g_formats_ima_adpcm[] = {
		{ 1,  8000,  4055,  256, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 2,  8000,  8110,  512, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 1, 11025,  5588,  256, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 2, 11025, 11177,  512, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 1, 22050, 11100,  512, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 2, 22050, 22201, 1024, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 1, 44100, 22125, 1024, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
		{ 2, 44100, 44251, 2048, 4, AV_CODEC_ID_ADPCM_IMA_WAV },
	};

	static const format_t g_formats_alaw[] = {
		{ 1,  8000,  8000, 1, 8, AV_CODEC_ID_PCM_ALAW },
		{ 2,  8000, 16000, 2, 8, AV_CODEC_ID_PCM_ALAW },
		{ 1, 11025, 11025, 1, 8, AV_CODEC_ID_PCM_ALAW },
		{ 2, 11025, 22050, 2, 8, AV_CODEC_ID_PCM_ALAW },
		{ 1, 16000, 16000, 1, 8, AV_CODEC_ID_PCM_ALAW },
		{ 2, 16000, 32000, 2, 8, AV_CODEC_ID_PCM_ALAW },
		{ 1, 22050, 22050, 1, 8, AV_CODEC_ID_PCM_ALAW },
		{ 2, 22050, 44100, 2, 8, AV_CODEC_ID_PCM_ALAW },
		{ 1, 44100, 44100, 1, 8, AV_CODEC_ID_PCM_ALAW },
		{ 2, 44100, 88200, 2, 8, AV_CODEC_ID_PCM_ALAW },
	};

	static const format_t g_formats_mulaw[] = {
		{ 1,  8000,  8000, 1, 8, AV_CODEC_ID_PCM_MULAW },
		{ 2,  8000, 16000, 2, 8, AV_CODEC_ID_PCM_MULAW },
		{ 1, 11025, 11025, 1, 8, AV_CODEC_ID_PCM_MULAW },
		{ 2, 11025, 22050, 2, 8, AV_CODEC_ID_PCM_MULAW },
		{ 1, 16000, 16000, 1, 8, AV_CODEC_ID_PCM_MULAW },
		{ 2, 16000, 32000, 2, 8, AV_CODEC_ID_PCM_MULAW },
		{ 1, 22050, 22050, 1, 8, AV_CODEC_ID_PCM_MULAW },
		{ 2, 22050, 44100, 2, 8, AV_CODEC_ID_PCM_MULAW },
		{ 1, 44100, 44100, 1, 8, AV_CODEC_ID_PCM_MULAW },
		{ 2, 44100, 88200, 2, 8, AV_CODEC_ID_PCM_MULAW },
	};

	static const format_tag_t g_format_tag_pcm = {
		WAVE_FORMAT_PCM,
		sizeof(PCMWAVEFORMAT),
		L"PCM",
		ARRAYSIZE(g_formats_pcm),
		g_formats_pcm,
	};

	static const format_tag_t g_format_tag_adpcm = {
		WAVE_FORMAT_ADPCM,
		50,
		L"Microsoft ADPCM",
		ARRAYSIZE(g_formats_adpcm),
		g_formats_adpcm,
	};

	static const format_tag_t g_format_tag_ima_adpcm = {
		WAVE_FORMAT_IMA_ADPCM,
		sizeof(IMAADPCMWAVEFORMAT),
		L"IMA ADPCM",
		ARRAYSIZE(g_formats_ima_adpcm),
		g_formats_ima_adpcm,
	};

	static const format_tag_t g_format_tag_alaw = {
		WAVE_FORMAT_ALAW,
		sizeof(WAVEFORMATEX),
		L"CCITT A-Law",
		ARRAYSIZE(g_formats_alaw),
		g_formats_alaw,
	};

	static const format_tag_t g_format_tag_mulaw = {
		WAVE_FORMAT_MULAW,
		sizeof(WAVEFORMATEX),
		L"CCITT u-Law",
		ARRAYSIZE(g_formats_mulaw),
		g_formats_mulaw,
	};

	const format_tag_t * g_format_tags[] = {
		&g_format_tag_pcm,	// DO NOT MOVE
		&g_format_tag_adpcm,
		&g_format_tag_ima_adpcm,
		&g_format_tag_alaw,
		&g_format_tag_mulaw,
	};
	const size_t g_format_tags_cnt = ARRAYSIZE(g_format_tags);
}}

using namespace ffmpeg_w32codec::acmdrv;

LRESULT format::details(LPACMFORMATTAGDETAILSW desc, DWORD flags)
{
	const format_tag_t *fmt = nullptr;

	if (desc->cbStruct != sizeof(*desc)) {
		LOGE("invalid structure size %u", desc->cbStruct);
		return ACMERR_NOTPOSSIBLE;
	}

	switch (flags & ACM_FORMATTAGDETAILSF_QUERYMASK)
	{
	case ACM_FORMATTAGDETAILSF_INDEX:
		LOGD("- ACM_FORMATTAGDETAILSF_INDEX");
		if (desc->dwFormatTagIndex >= ARRAYSIZE(g_format_tags)) {
			LOGE("invalid format tag index %u", desc->dwFormatTagIndex);
			return ACMERR_NOTPOSSIBLE;
		}
		fmt = g_format_tags[desc->dwFormatTagIndex];
		desc->dwFormatTag = fmt->tag;
		break;

	case ACM_FORMATTAGDETAILSF_FORMATTAG:
		LOGD("- ACM_FORMATTAGDETAILSF_FORMATTAG");
		for (DWORD i=0; i<ARRAYSIZE(g_format_tags); i++) {
			if (g_format_tags[i]->tag == desc->dwFormatTag) {
				desc->dwFormatTagIndex = i;
				fmt = g_format_tags[i];
				break;
			}
		}
		if (fmt == nullptr) {
			LOGE("invalid format tag 0x%08x", desc->dwFormatTag);
			return ACMERR_NOTPOSSIBLE;
		}
		break;

	default:
		LOGE("invalid flags 0x%08x", flags);
		return ACMERR_NOTPOSSIBLE;
	}

	desc->cbFormatSize		= fmt->size;
	desc->fdwSupport		= ACMDRIVERDETAILS_SUPPORTF_CODEC;
	desc->cStandardFormats	= fmt->count;
	wcscpy_s(desc->szFormatTag, fmt->name);

	return MMSYSERR_NOERROR;
}

LRESULT format::details(LPACMFORMATDETAILSW desc, DWORD flags)
{
	const format_tag_t *fmt = nullptr;
	DWORD index = 0;
	WAVEFORMATEX wfx = {};

	if (desc->cbStruct != sizeof(*desc)) {
		LOGE("invalid structure size %u", desc->cbStruct);
		return ACMERR_NOTPOSSIBLE;
	}

	for (DWORD i=0; i<ARRAYSIZE(g_format_tags); i++) {
		if (g_format_tags[i]->tag == desc->dwFormatTag) {
			fmt = g_format_tags[i];
			break;
		}
	}
	if (fmt == nullptr) {
		LOGE("invalid format tag 0x%08x", desc->dwFormatTag);
		return ACMERR_NOTPOSSIBLE;
	}

	switch (flags & ACM_FORMATDETAILSF_QUERYMASK)
	{
	case ACM_FORMATDETAILSF_INDEX:
		LOGD("- ACM_FORMATDETAILSF_INDEX");
		if (desc->dwFormatIndex >= fmt->count) {
			LOGE("invalid format index %u", desc->dwFormatIndex);
			return ACMERR_NOTPOSSIBLE;
		}
		index = desc->dwFormatIndex;
		break;

	default:
		LOGE("invalid flags 0x%08x", flags);
		return ACMERR_NOTPOSSIBLE;
	}

	wfx.wFormatTag		= fmt->tag;
	wfx.nChannels		= fmt->fmts[index].channels;
	wfx.nSamplesPerSec	= fmt->fmts[index].samples;
	wfx.nAvgBytesPerSec	= fmt->fmts[index].avg_bytes;
	wfx.nBlockAlign		= fmt->fmts[index].align;
	wfx.wBitsPerSample	= fmt->fmts[index].bits;
	if (fmt->size >= sizeof(wfx)) {
		wfx.cbSize = fmt->size - sizeof(wfx);
	}

	desc->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	memcpy_s(desc->pwfx, desc->cbwfx, &wfx, sizeof(wfx));
	if (wfx.wBitsPerSample != 0) {
		wsprintfW(
			desc->szFormat, L"%u Hz, %u bits, %s",
			wfx.nSamplesPerSec, wfx.wBitsPerSample,
			(wfx.nChannels == 1)? L"Mono" : L"Stereo");
	} else {
		wsprintfW(
			desc->szFormat, L"%u Hz, %s",
			wfx.nSamplesPerSec,
			(wfx.nChannels == 1)? L"Mono" : L"Stereo");
	}

	return MMSYSERR_NOERROR;
}

LRESULT format::suggest(LPACMDRVFORMATSUGGEST desc)
{
	DWORD type;
	int src_bits;
	int dst_bits;

	if (desc->cbStruct != sizeof(*desc)) {
		LOGE("invalid structure size %u", desc->cbStruct);
		return ACMERR_NOTPOSSIBLE;
	}

	src_bits = desc->pwfxSrc->wBitsPerSample;
	dst_bits = ((src_bits == 8) || (src_bits == 16))? src_bits : 16;

	type = desc->fdwSuggest & ACM_FORMATSUGGESTF_TYPEMASK;
	if (type & ACM_FORMATSUGGESTF_WFORMATTAG) {
		if (desc->pwfxDst->wFormatTag != WAVE_FORMAT_PCM) {
			LOGE("invalid format tag 0x%04x", desc->pwfxDst->wFormatTag);
			return ACMERR_NOTPOSSIBLE;
		}
	}
	if (type & ACM_FORMATSUGGESTF_NCHANNELS) {
		if (desc->pwfxDst->nChannels != desc->pwfxSrc->nChannels) {
			LOGE("invalid channels %u", desc->pwfxDst->nChannels);
			return ACMERR_NOTPOSSIBLE;
		}
	}
	if (type & ACM_FORMATSUGGESTF_NSAMPLESPERSEC) {
		if (desc->pwfxDst->nSamplesPerSec != desc->pwfxSrc->nSamplesPerSec) {
			LOGE("invalid sample rate %u", desc->pwfxDst->nSamplesPerSec);
			return ACMERR_NOTPOSSIBLE;
		}
	}
	if (type & ACM_FORMATSUGGESTF_WBITSPERSAMPLE) {
		if (desc->pwfxDst->wBitsPerSample != dst_bits) {
			LOGE("invalid sample bits %u", desc->pwfxDst->wBitsPerSample);
			return ACMERR_NOTPOSSIBLE;
		}
	}

	ZeroMemory(desc->pwfxDst, desc->cbwfxDst);
	desc->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
	desc->pwfxDst->nChannels = desc->pwfxSrc->nChannels;
	desc->pwfxDst->nSamplesPerSec = desc->pwfxSrc->nSamplesPerSec;
	desc->pwfxDst->wBitsPerSample = dst_bits;
	desc->pwfxDst->nBlockAlign = desc->pwfxDst->nChannels * (dst_bits / 8);
	desc->pwfxDst->nAvgBytesPerSec =
		desc->pwfxDst->nSamplesPerSec * desc->pwfxDst->nBlockAlign;

	return MMSYSERR_NOERROR;
}
