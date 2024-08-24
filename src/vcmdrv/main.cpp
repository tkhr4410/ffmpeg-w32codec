// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "vcmdrv.h"

namespace ffmpeg_w32codec { namespace vcmdrv {
	FOURCC g_fourcc_type;
	FOURCC g_fourcc_handler;
}}

using namespace ffmpeg_w32codec;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	char path[MAX_PATH];
	char fname[_MAX_FNAME];
	char *p1;
	char *p2;

	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		GetModuleFileNameA(hinstDLL, path, sizeof(path));
		p1 = strrchr(path, '\\') + 1;
		p2 = strchr(path, '.');
		*p2 = '\0';
		p2 = fname;
		while (*p1 != '\0') {
			*p2 = tolower(*p1);
			p1++;
			p2++;
		}
		*p2 = '\0';
		if ((8 == strlen(fname)) && (0 == strncmp(fname, "vidc", 4))) {
			vcmdrv::g_fourcc_type =
				mmioFOURCC(fname[0], fname[1], fname[2], fname[3]);
			vcmdrv::g_fourcc_handler =
				mmioFOURCC(fname[4], fname[5], fname[6], fname[7]);
		}
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
	vcmdrv::ic_context_t *ic = (vcmdrv::ic_context_t *)dwDriverId;

	switch (uMsg)
	{
	case DRV_LOAD:
		LOGD("DRV_LOAD");
		return vcmdrv::driver::load();

	case DRV_FREE:
		LOGD("DRV_FREE");
		return vcmdrv::driver::free();

	case DRV_ENABLE:
		LOGD("DRV_ENABLE");
		return vcmdrv::driver::enable();

	case DRV_DISABLE:
		LOGD("DRV_DISABLE");
		return vcmdrv::driver::disable();

	case DRV_OPEN:
		LOGD("DRV_OPEN");
		return vcmdrv::driver::open((ICOPEN *)lParam2);

	case DRV_CLOSE:
		LOGD("DRV_CLOSE");
		return vcmdrv::driver::close(ic);

	case ICM_GETINFO:
		LOGD("ICM_GETINFO");
		return vcmdrv::ic::get_info((ICINFO *)lParam1, lParam2);

	case ICM_DECOMPRESS_QUERY:
		LOGD("ICM_DECOMPRESS_QUERY");
		return vcmdrv::ic::decompress::query(
			ic, (LPBITMAPINFO)lParam1, (LPBITMAPINFO)lParam2);

	case ICM_DECOMPRESS_BEGIN:
		LOGD("ICM_DECOMPRESS_BEGIN");
		return vcmdrv::ic::decompress::begin(
			ic, (LPBITMAPINFO)lParam1, (LPBITMAPINFO)lParam2);

	case ICM_DECOMPRESS:
		LOGD("ICM_DECOMPRESS");
		return vcmdrv::ic::decompress::run(
			ic, (ICDECOMPRESS *)lParam1, lParam2);

	case ICM_DECOMPRESS_END:
		LOGD("ICM_DECOMPRESS_END");
		return vcmdrv::ic::decompress::end(ic);

	default:
		LOGD("%s: uMsg=0x%04x", __FUNCTION__, uMsg);
		if (uMsg < DRV_USER) {
			return DefDriverProc(dwDriverId, hdrvr, uMsg, lParam1, lParam2);
		} else {
			return ICERR_UNSUPPORTED;
		}
	}
}
