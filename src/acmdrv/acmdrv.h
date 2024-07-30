// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#define LOG_TAG "ACMDrv"
#include "common.h"

namespace ffmpeg_w32codec { namespace acmdrv {
	typedef struct {
		WORD		channels;
		DWORD		samples;
		DWORD		avg_bytes;
		DWORD		align;
		WORD		bits;
		AVCodecID	codec_id;
	} format_t;

	typedef struct {
		WORD				tag;	// WAVE_FORMAT_*
		size_t				size;
		LPCWSTR				name;
		size_t				count;
		const format_t *	fmts;
	} format_tag_t;

	extern const format_tag_t * g_format_tags[];
	extern const size_t g_format_tags_cnt;

	namespace driver {
		struct context;
		extern LRESULT load(void);
		extern LRESULT free(void);
		extern LRESULT enable(void);
		extern LRESULT disable(void);
		extern LRESULT open(LPACMDRVOPENDESCW desc);
		extern LRESULT close(struct context *driver);
		extern LRESULT query_configure(void);
		extern LRESULT about(HWND hWnd);
		extern LRESULT details(LPACMDRIVERDETAILSW desc);
	}

	namespace format {
		extern LRESULT details(LPACMFORMATTAGDETAILSW desc, DWORD flags);
		extern LRESULT details(LPACMFORMATDETAILSW desc, DWORD flags);
		extern LRESULT suggest(LPACMDRVFORMATSUGGEST desc);
	}

	namespace stream {
		extern LRESULT open(LPACMDRVSTREAMINSTANCE inst);
		extern LRESULT close(LPACMDRVSTREAMINSTANCE inst);
		extern LRESULT size(
			LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMSIZE desc);
		extern LRESULT convert(
			LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc);
		extern LRESULT prepare(
			LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc);
		extern LRESULT unprepare(
			LPACMDRVSTREAMINSTANCE inst, LPACMDRVSTREAMHEADER desc);
	}
}}
