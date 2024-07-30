// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "acmdrv.h"

using namespace ffmpeg_w32codec;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)hinstDLL;
	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	default:
		break;
	}

	return TRUE;
}

extern "C" LRESULT CALLBACK DriverProc(
	DWORD_PTR dwDriverId, HDRVR hdrvr,
	UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	switch (uMsg)
	{
	case DRV_LOAD:
		LOGD("DRV_LOAD");
		return acmdrv::driver::load();

	case DRV_FREE:
		LOGD("DRV_FREE");
		return acmdrv::driver::free();

	case DRV_ENABLE:
		LOGD("DRV_ENABLE");
		return acmdrv::driver::enable();

	case DRV_DISABLE:
		LOGD("DRV_DISABLE");
		return acmdrv::driver::disable();

	case DRV_OPEN:
		LOGD("DRV_OPEN");
		return acmdrv::driver::open((LPACMDRVOPENDESCW)lParam2);

	case DRV_CLOSE:
		LOGD("DRV_CLOSE");
		return acmdrv::driver::close((acmdrv::driver::context *)dwDriverId);

	case DRV_QUERYCONFIGURE:
		LOGD("DRV_QUERYCONFIGURE");
		return acmdrv::driver::query_configure();

	case ACMDM_DRIVER_ABOUT:
		LOGD("ACMDM_DRIVER_ABOUT");
		return acmdrv::driver::about((HWND)lParam1);

	case ACMDM_DRIVER_DETAILS:
		LOGD("ACMDM_DRIVER_DETAILS");
		return acmdrv::driver::details((LPACMDRIVERDETAILSW)lParam1);

	case ACMDM_FORMATTAG_DETAILS:
		LOGD("ACMDM_FORMATTAG_DETAILS");
		return acmdrv::format::details(
			(LPACMFORMATTAGDETAILSW)lParam1, lParam2);

	case ACMDM_FORMAT_DETAILS:
		LOGD("ACMDM_FORMAT_DETAILS");
		return acmdrv::format::details((LPACMFORMATDETAILSW)lParam1, lParam2);

	case ACMDM_FORMAT_SUGGEST:
		LOGD("ACMDM_FORMAT_SUGGEST");
		return acmdrv::format::suggest((LPACMDRVFORMATSUGGEST)lParam1);

	case ACMDM_STREAM_OPEN:
		LOGD("ACMDM_STREAM_OPEN");
		return acmdrv::stream::open((LPACMDRVSTREAMINSTANCE)lParam1);

	case ACMDM_STREAM_CLOSE:
		LOGD("ACMDM_STREAM_CLOSE");
		return acmdrv::stream::close((LPACMDRVSTREAMINSTANCE)lParam1);

	case ACMDM_STREAM_SIZE:
		LOGD("ACMDM_STREAM_SIZE");
		return acmdrv::stream::size(
			(LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMSIZE)lParam2);

	case ACMDM_STREAM_CONVERT:
		LOGD("ACMDM_STREAM_CONVERT");
		return acmdrv::stream::convert(
			(LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER)lParam2);

	case ACMDM_STREAM_PREPARE:
		LOGD("ACMDM_STREAM_PREPARE");
		return acmdrv::stream::prepare(
			(LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER)lParam2);

	case ACMDM_STREAM_UNPREPARE:
		LOGD("ACMDM_STREAM_UNPREPARE");
		return acmdrv::stream::unprepare(
			(LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER)lParam2);

	default:
		LOGD("%s: uMsg=0x%04x", __FUNCTION__, uMsg);
		break;
	}

	return 0;
}
