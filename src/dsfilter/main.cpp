// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "dsfilter.h"

static const AMOVIESETUP_MEDIATYPE g_media_types_stream_in[] = {
	{ &MEDIATYPE_Stream, &MEDIASUBTYPE_NULL },
};

static const AMOVIESETUP_MEDIATYPE g_media_types_video_out[] = {
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
};

static const AMOVIESETUP_MEDIATYPE g_media_types_audio_out[] = {
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM },
};

static const AMOVIESETUP_PIN g_pins[] = {
	{
		L"Stream In", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr,
		ARRAYSIZE(g_media_types_stream_in), g_media_types_stream_in
	},
	{
		L"Video Out", FALSE, TRUE, FALSE, TRUE, &CLSID_NULL, nullptr,
		ARRAYSIZE(g_media_types_video_out), g_media_types_video_out
	},
	{
		L"Audio Out", FALSE, TRUE, FALSE, TRUE, &CLSID_NULL, nullptr,
		ARRAYSIZE(g_media_types_audio_out), g_media_types_audio_out
	},
};

static const AMOVIESETUP_FILTER g_filter[] = {
	{
		&__uuidof(CFFmpegDSFilter), L"ffmpeg-w32codec DirectShow Filter",
		MERIT_NORMAL, ARRAYSIZE(g_pins), g_pins
	},
};

CFactoryTemplate g_Templates[] = {
	{
		g_filter[0].strName, g_filter[0].clsID,
		CFFmpegDSFilter::CreateInstance, nullptr, &g_filter[0]
	},
};

int g_cTemplates = ARRAYSIZE(g_Templates);

extern "C" BOOL WINAPI DllEntryPoint(
	HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return DllEntryPoint(hinstDLL, fdwReason, lpvReserved);
}

STDAPI DllRegisterServer(void)
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer(void)
{
	return AMovieDllRegisterServer2(FALSE);
}
