// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "acmdrv.h"

namespace ffmpeg_w32codec { namespace acmdrv { namespace driver {
	typedef struct context {
		void *	reserved;
	} context_t;
}}}

using namespace ffmpeg_w32codec::acmdrv;

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

LRESULT driver::open(LPACMDRVOPENDESCW desc)
{
	context_t *driver;

	if (desc != nullptr) {
		if (desc->cbStruct != sizeof(*desc)) {
			LOGE("invalid structure size %u", desc->cbStruct);
			return 0;
		}
		if (desc->fccType != ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC) {
			LOGE("invalid type 0x%08x", desc->fccType);
			return 0;
		}
	}

	driver = (context_t *)HeapAlloc(
		GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*driver));

	return (LRESULT)(DWORD_PTR)driver;
}

LRESULT driver::close(context_t *driver)
{
	HeapFree(GetProcessHeap(), 0, driver);
	return DRV_OK;
}

LRESULT driver::query_configure(void)
{
	return DRV_CANCEL;
}

LRESULT driver::about(HWND hWnd)
{
	(void)hWnd;
	return MMSYSERR_NOTSUPPORTED;
}

LRESULT driver::details(LPACMDRIVERDETAILSW desc)
{
	if (desc->cbStruct != sizeof(*desc)) {
		LOGE("invalid structure size %u", desc->cbStruct);
		return ACMERR_NOTPOSSIBLE;
	}

	desc->fccType		= ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
	desc->fccComp		= ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
	desc->wMid			= MM_UNMAPPED;
	desc->wPid			= MM_PID_UNMAPPED;
	desc->vdwACM		= MAKE_ACM_VERSION(5, 0, 0);
	desc->vdwDriver		= MAKE_ACM_VERSION(0, 0, 0);
	desc->fdwSupport	= ACMDRIVERDETAILS_SUPPORTF_CODEC;
	desc->cFormatTags	= g_format_tags_cnt;
	desc->cFilterTags	= 0;
	desc->hicon			= NULL;
	wcscpy_s(desc->szShortName, L"ffmpeg-w32codec");
	wcscpy_s(desc->szLongName, L"ffmpeg-w32codec");
	desc->szCopyright[0] = '\0';
	desc->szLicensing[0] = '\0';
	desc->szFeatures[0] = '\0';

	return MMSYSERR_NOERROR;
}
