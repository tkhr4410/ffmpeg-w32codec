// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#define LOG_TAG "VCMDrv"
#include "common.h"

namespace ffmpeg_w32codec { namespace vcmdrv {
	typedef struct {
		FOURCC				type;
		FOURCC				handler;
		const AVCodec *		codec;
		AVCodecContext *	avctx;
		AVPacket *			avpkt;
		AVFrame *			frame;
		AVFrame *			frame_sws;
		SwsContext *		sws;
		int					pitch;
	} ic_context_t;

	extern FOURCC g_fourcc_type;
	extern FOURCC g_fourcc_handler;

	namespace driver {
		extern LRESULT load(void);
		extern LRESULT free(void);
		extern LRESULT enable(void);
		extern LRESULT disable(void);
		extern LRESULT open(ICOPEN *desc);
		extern LRESULT close(ic_context_t *ic);
	}

	namespace ic {
		extern LRESULT get_info(ICINFO *desc, SIZE_T size);
		namespace decompress {
			extern LRESULT query(
				ic_context_t *ic, LPBITMAPINFO in, LPBITMAPINFO out);
			extern LRESULT begin(
				ic_context_t *ic, LPBITMAPINFO in, LPBITMAPINFO out);
			extern LRESULT run(
				ic_context_t *ic, ICDECOMPRESS *desc, SIZE_T size);
			extern LRESULT end(ic_context_t *ic);
		}
	}
}}
