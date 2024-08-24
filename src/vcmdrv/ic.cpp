// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "vcmdrv.h"

using namespace ffmpeg_w32codec::vcmdrv;

LRESULT ic::get_info(ICINFO *desc, SIZE_T size)
{
	if (size < sizeof(*desc)) {
		LOGE("invalid structure size %u", size);
		return 0;
	}

	desc->fccType		= g_fourcc_type;
	desc->fccHandler	= g_fourcc_handler;
	desc->dwFlags		= 0;	// TODO
	desc->dwVersion		= 0;
	desc->dwVersionICM	= ICVERSION;
	wcscpy_s(desc->szName, L"ffmpeg-w32codec");
	wcscpy_s(desc->szDescription, L"ffmpeg-w32codec");
	desc->szDriver[0] = '\0';

	return size;
}

LRESULT ic::decompress::query(
	ic_context_t *ic, LPBITMAPINFO in, LPBITMAPINFO out)
{
	if (ic == nullptr) {
		return ICERR_BADPARAM;
	}

	if (out->bmiHeader.biBitCount != 24) {
		return ICERR_UNSUPPORTED;
	}
	if (out->bmiHeader.biCompression != BI_RGB) {
		return ICERR_UNSUPPORTED;
	}

	return ICERR_OK;
}

LRESULT ic::decompress::begin(
	ic_context_t *ic, LPBITMAPINFO in, LPBITMAPINFO out)
{
	int ret;
	AVPixelFormat dst_fmt;

	if (ic == nullptr) {
		return ICERR_BADPARAM;
	}

	switch (out->bmiHeader.biBitCount)
	{
	case 24:
		dst_fmt = AV_PIX_FMT_BGR24;
		break;

	default:
		LOGE("unsupported output bits %u", out->bmiHeader.biBitCount);
		return ICERR_UNSUPPORTED;
	}

	if (ic->avctx != nullptr) {
		avcodec_free_context(&ic->avctx);
	}
	ic->avctx = avcodec_alloc_context3(ic->codec);
	if (ic->avctx == nullptr) {
		LOGE("avcodec_alloc_context3() failed.");
		return ICERR_UNSUPPORTED;
	}

	ic->avctx->width = in->bmiHeader.biWidth;
	ic->avctx->height = in->bmiHeader.biHeight;
	ret = avcodec_open2(ic->avctx, ic->codec, nullptr);
	if (ret < 0) {
		LOGE("avcodec_open2() failed. (%d)", ret);
		return ICERR_UNSUPPORTED;
	}

	ic->avpkt = av_packet_alloc();
	if (ic->avpkt == nullptr) {
		LOGE("av_packet_alloc() failed.");
		return ICERR_UNSUPPORTED;
	}

	ic->frame = av_frame_alloc();
	if (ic->frame == nullptr) {
		LOGE("av_frame_alloc() failed.");
		return ICERR_UNSUPPORTED;
	}

	ic->frame_sws = av_frame_alloc();
	if (ic->frame_sws == nullptr) {
		LOGE("av_frame_alloc() failed.");
		return ICERR_UNSUPPORTED;
	}

	ic->sws = sws_getContext(
		ic->avctx->width, ic->avctx->height, ic->avctx->pix_fmt,
		out->bmiHeader.biWidth, abs(out->bmiHeader.biHeight), dst_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (ret < 0) {
		LOGE("sws_getContext() failed.");
		return ICERR_UNSUPPORTED;
	}

	ic->pitch =
		((out->bmiHeader.biWidth * out->bmiHeader.biBitCount + 31) & ~31) / 8;

	return ICERR_OK;
}

LRESULT ic::decompress::run(
	ic_context_t *ic, ICDECOMPRESS *desc, SIZE_T size)
{
	int ret;
	int h;
	int pitch;
	uint8_t *src;
	uint8_t *dst;

	if (ic == nullptr) {
		return ICERR_BADPARAM;
	}
	if (size < sizeof(*desc)) {
		LOGE("invalid structure size %u", size);
		return ICERR_BADPARAM;
	}

	ic->avpkt->data = (uint8_t *)desc->lpInput;
	ic->avpkt->size = desc->lpbiInput->biSizeImage;
	ret = avcodec_send_packet(ic->avctx, ic->avpkt);
	if (ret < 0) {
		LOGE("avcodec_send_packet() failed. (%d:0x%08x)", ret, ret);
		return ICERR_UNSUPPORTED;
	}

	ret = avcodec_receive_frame(ic->avctx, ic->frame);
	if (ret < 0) {
		LOGE("avcodec_receive_frame() failed. (%d)", ret);
		return ICERR_UNSUPPORTED;
	}

	ret = sws_scale_frame(ic->sws, ic->frame_sws, ic->frame);
	if (ret < 0) {
		LOGE("sws_scale_frame() failed. (%d)", ret);
		return ICERR_UNSUPPORTED;
	}

	h = desc->lpbiOutput->biHeight;
	pitch = ic->frame_sws->linesize[0];
	if (h > 0) {
		src = (uint8_t *)ic->frame_sws->data[0] + (h - 1) * pitch;
		dst = (uint8_t *)desc->lpOutput;
		for (int y=0; y<h; y++) {
			CopyMemory(dst, src, pitch);
			src -= pitch;
			dst += ic->pitch;
		}
	} else {
		src = (uint8_t *)ic->frame_sws->data[0];
		dst = (uint8_t *)desc->lpOutput;
		h = -h;
		for (int y=0; y<h; y++) {
			CopyMemory(dst, src, pitch);
			src += pitch;
			dst += ic->pitch;
		}
	}

	return ICERR_OK;
}

LRESULT ic::decompress::end(ic_context_t *ic)
{
	if (ic == nullptr) {
		return ICERR_BADPARAM;
	}

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

	return ICERR_OK;
}
