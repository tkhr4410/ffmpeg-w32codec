// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "vcmdrv.h"

using namespace ffmpeg_w32codec::vcmdrv;

static void log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
	char buf[1024];

	vsprintf_s(buf, fmt, vl);
	LOGD("%s", buf);
}

LRESULT driver::load(void)
{
	return DRV_OK;
}

LRESULT driver::free(void)
{
	return DRV_OK;
}

LRESULT driver::enable(void)
{
	return DRV_OK;
}

LRESULT driver::disable(void)
{
	return DRV_OK;
}

LRESULT driver::open(ICOPEN *desc)
{
	ic_context_t *ic;
	AVCodecID codec_id;
	const AVCodec *codec;

	av_log_set_callback(log_callback);

	if (desc->dwSize != sizeof(*desc)) {
		LOGE("invalid structure size %u", desc->dwSize);
		desc->dwError = ICERR_BADPARAM;
		return 0;
	}

	if (desc->fccType != ICTYPE_VIDEO) {
		LOGE("invalid type 0x%08x", desc->fccType);
		desc->dwError = ICERR_UNSUPPORTED;
		return 0;
	}

	switch (desc->fccHandler)
	{
	case mmioFOURCC('I', 'V', '5', '0'):
	case mmioFOURCC('i', 'v', '5', '0'):
		codec_id = AV_CODEC_ID_INDEO5;
		break;

	case mmioFOURCC('M', 'S', 'V', 'C'):
	case mmioFOURCC('m', 's', 'v', 'c'):
		codec_id = AV_CODEC_ID_MSVIDEO1;
		break;

	default:
		LOGE("invalid handler 0x%08x", desc->fccHandler);
		desc->dwError = ICERR_UNSUPPORTED;
		return 0;
	}

	codec = avcodec_find_decoder(codec_id);
	if (codec == nullptr) {
		LOGE("avcodec_find_decoder(%d) failed.", codec_id);
		desc->dwError = ICERR_UNSUPPORTED;
		return 0;
	}

	ic = (ic_context_t *)HeapAlloc(
		GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ic));
	ic->type = desc->fccType;
	ic->handler = desc->fccHandler;
	ic->codec = codec;

	desc->dwVersion = 0;	// TODO
	desc->pV1Reserved = nullptr;
	desc->pV2Reserved = nullptr;
	desc->dnDevNode = 0;
	desc->dwError = ICERR_OK;

	return (LRESULT)(DWORD_PTR)ic;
}

LRESULT driver::close(ic_context_t *ic)
{
	if (ic != nullptr) {
		if (ic->sws != nullptr) {
			sws_freeContext(ic->sws);
			ic->sws = nullptr;
		}
		if (ic->frame_sws != nullptr) {
			av_frame_free(&ic->frame_sws);
		}
		if (ic->frame != nullptr) {
			av_frame_free(&ic->frame);
		}
		if (ic->avpkt != nullptr) {
			av_packet_free(&ic->avpkt);
		}
		if (ic->avctx != nullptr) {
			avcodec_free_context(&ic->avctx);
		}
		HeapFree(GetProcessHeap(), 0, ic);
	}
	return DRV_OK;
}
