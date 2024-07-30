// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "acmdrv.h"

namespace ffmpeg_w32codec { namespace acmdrv { namespace stream {
	typedef struct context {
		AVCodecContext *	avctx;
	} context_t;
}}}

using namespace ffmpeg_w32codec::acmdrv;

LRESULT stream::open(LPACMDRVSTREAMINSTANCE inst)
{
	const format_t *src = nullptr;
	const format_t *dst = nullptr;
	context_t *stream = nullptr;
	const AVCodec *codec = nullptr;
	AVCodecContext *avctx = nullptr;

	for (DWORD i=1; i<g_format_tags_cnt; i++) {
		const format_t *fmts = g_format_tags[i]->fmts;
		if (inst->pwfxSrc->wFormatTag != g_format_tags[i]->tag) {
			continue;
		}
		for (DWORD j=0; j<g_format_tags[i]->count; j++) {
			if (inst->pwfxSrc->nChannels != fmts[j].channels) {
				continue;
			}
			if (inst->pwfxSrc->nSamplesPerSec != fmts[j].samples) {
				continue;
			}
			if (inst->pwfxSrc->wBitsPerSample != fmts[j].bits) {
				continue;
			}
			src = &fmts[j];
			break;
		}
		if (src != nullptr) {
			break;
		}
	}
	if (src == nullptr) {
		LOGE("invalid input format (0x%04x-0x%04x-0x%08x-0x%04x)",
			inst->pwfxSrc->wFormatTag, inst->pwfxSrc->nChannels,
			inst->pwfxSrc->nSamplesPerSec, inst->pwfxSrc->wBitsPerSample);
		return ACMERR_NOTPOSSIBLE;
	}

	if (inst->pwfxDst->wFormatTag == WAVE_FORMAT_PCM) {
		auto pcm_fmt = g_format_tags[0]->fmts;
		for (DWORD i=0; pcm_fmt[i].channels != 0; i++) {
			if (inst->pwfxDst->nChannels != pcm_fmt[i].channels) {
				continue;
			}
			if (inst->pwfxDst->nSamplesPerSec != pcm_fmt[i].samples) {
				continue;
			}
			if (inst->pwfxDst->wBitsPerSample != pcm_fmt[i].bits) {
				continue;
			}
			dst = &pcm_fmt[i];
			break;
		}
	}
	if (dst == nullptr) {
		LOGE("invalid output format (0x%04x-0x%04x-0x%08x-0x%04x)",
			inst->pwfxDst->wFormatTag, inst->pwfxDst->nChannels,
			inst->pwfxDst->nSamplesPerSec, inst->pwfxDst->wBitsPerSample);
		return ACMERR_NOTPOSSIBLE;
	}

	if (inst->fdwOpen & ACM_STREAMOPENF_QUERY) {
		return MMSYSERR_NOERROR;
	}

	codec = avcodec_find_decoder(src->codec_id);
	if (codec == nullptr) {
		LOGE("avcodec_find_decoder(%d) failed.", src->codec_id);
		return ACMERR_NOTPOSSIBLE;
	}

	avctx = avcodec_alloc_context3(codec);
	if (avctx == nullptr) {
		LOGE("avcodec_alloc_context3() failed.");
		return ACMERR_NOTPOSSIBLE;
	}
	avctx->sample_rate = inst->pwfxSrc->nSamplesPerSec;
	avctx->ch_layout.order = AV_CHANNEL_ORDER_UNSPEC;
	avctx->ch_layout.nb_channels = inst->pwfxSrc->nChannels;
	avctx->block_align = inst->pwfxSrc->nBlockAlign;
	avctx->bits_per_coded_sample = inst->pwfxSrc->wBitsPerSample;
	if (0 != avcodec_open2(avctx, codec, nullptr)) {
		LOGE("avcodec_open2() failed.");
		avcodec_free_context(&avctx);
		return ACMERR_NOTPOSSIBLE;
	}

	stream = (context_t *)HeapAlloc(
		GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*stream));
	stream->avctx = avctx;

	inst->dwDriver = (DWORD_PTR)stream;

	return MMSYSERR_NOERROR;
}

LRESULT stream::close(LPACMDRVSTREAMINSTANCE inst)
{
	context_t *stream = (context_t *)inst->dwDriver;

	if (stream->avctx != nullptr) {
		avcodec_free_context(&stream->avctx);
	}
	HeapFree(GetProcessHeap(), 0, stream);

	return MMSYSERR_NOERROR;
}

LRESULT stream::size(LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMSIZE desc)
{
	context_t *stream = (context_t *)inst->dwDriver;
	int channels;
	int data_size;
	int samples;

	switch (desc->fdwSize & ACM_STREAMSIZEF_QUERYMASK)
	{
	case ACM_STREAMSIZEF_SOURCE:
		LOGD("- ACM_STREAMSIZEF_SOURCE");
		channels = inst->pwfxSrc->nChannels;
		data_size = inst->pwfxSrc->wBitsPerSample / 8 * channels;
		switch (inst->pwfxSrc->wFormatTag)
		{
		case WAVE_FORMAT_PCM:
		case WAVE_FORMAT_ALAW:
		case WAVE_FORMAT_MULAW:
			samples = desc->cbSrcLength / data_size;
			break;

		case WAVE_FORMAT_ADPCM:
			samples = (desc->cbSrcLength / inst->pwfxSrc->nBlockAlign) *
				((ADPCMWAVEFORMAT *)inst->pwfxSrc)->wSamplesPerBlock;
			break;

		case WAVE_FORMAT_IMA_ADPCM:
			samples = (desc->cbSrcLength / inst->pwfxSrc->nBlockAlign) *
				((IMAADPCMWAVEFORMAT *)inst->pwfxSrc)->wSamplesPerBlock;
			break;

		default:
			LOGE("invalid source format 0x%04x", inst->pwfxSrc->wFormatTag);
			return ACMERR_NOTPOSSIBLE;
		}
		desc->cbDstLength = av_samples_get_buffer_size(
			nullptr, channels, samples, stream->avctx->sample_fmt,
			inst->pwfxSrc->nBlockAlign);
		break;

	default:
		LOGE("invalid flags 0x%08x", desc->fdwSize);
		return ACMERR_NOTPOSSIBLE;
	}

	return MMSYSERR_NOERROR;
}

LRESULT stream::convert(
	LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc)
{
	LRESULT err = MMSYSERR_NOERROR;
	context_t *stream = (context_t *)inst->dwDriver;
	auto block_align = inst->pwfxSrc->nBlockAlign;
	auto packet_size = (block_align < 256)? 256 : block_align;
	auto is_planer = av_sample_fmt_is_planar(stream->avctx->sample_fmt);
	auto data_size = av_get_bytes_per_sample(stream->avctx->sample_fmt);
	auto channels = stream->avctx->ch_layout.nb_channels;
	auto src_offset = desc->cbSrcLengthUsed;
	auto dst_offset = desc->cbDstLengthUsed;
	auto src_len = desc->cbSrcLength;
	auto dst_len = desc->cbDstLength;
	auto src = desc->pbSrc;
	auto dst = desc->pbDst;
	uint8_t *packet_data = (uint8_t *)av_malloc(packet_size);
	AVFrame *frame = av_frame_alloc();
	AVPacket packet;
	int ret;

	if (!is_planer) {
		data_size *= channels;
	}

	av_init_packet(&packet);
	packet.data = packet_data;
	packet.size = packet_size;

	while (err == MMSYSERR_NOERROR) {
		if ((src_offset >= src_len) || (dst_offset >= dst_len)) {
			break;
		}
		if (src_offset + packet_size > src_len) {
			packet.size = src_len - src_offset;
			CopyMemory(packet.data, &src[src_offset], packet.size);
		} else {
			CopyMemory(packet.data, &src[src_offset], packet.size);
		}
		ret = avcodec_send_packet(stream->avctx, &packet);
		if (ret < 0) {
			LOGE("avcodec_send_packet() failed. (%d)", ret);
			err = ACMERR_NOTPOSSIBLE;
			break;
		}
		src_offset += packet.size;
		while (ret >= 0) {
			ret = avcodec_receive_frame(stream->avctx, frame);
			if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) {
				break;
			} else if (ret < 0) {
				LOGE("avcodec_receive_frame() failed. (%d)", ret);
				err = ACMERR_NOTPOSSIBLE;
				break;
			}
			for (int i=0; i<frame->nb_samples; i++) {
				if (is_planer) {
					if (dst_offset + data_size * channels > dst_len) {
						break;
					}
					for (int ch=0; ch<channels; ch++) {
						CopyMemory(
							&dst[dst_offset], frame->data[ch] + data_size * i,
							data_size);
						dst_offset += data_size;
					}
				} else {
					if (dst_offset + data_size > dst_len) {
						break;
					}
					CopyMemory(
						&dst[dst_offset], frame->data[0] + data_size * i,
						data_size);
					dst_offset += data_size;
				}
			}
		}
	}

	av_frame_free(&frame);
	av_free(packet_data);

	desc->cbSrcLengthUsed = src_offset;
	desc->cbDstLengthUsed = dst_offset;

	return err;
}

LRESULT stream::prepare(
	LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc)
{
	(void)inst;
	(void)desc;
	return MMSYSERR_NOERROR;
}

LRESULT stream::unprepare(
	LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc)
{
	(void)inst;
	(void)desc;
	return MMSYSERR_NOERROR;
}
