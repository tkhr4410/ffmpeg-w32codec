// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#define INITGUID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>

#include <assert.h>

#define LOG_TAG "Test"
#include "common.h"

using DLLGETCLASSOBJECT = HRESULT (WINAPI *)(REFCLSID, REFIID, LPVOID *);

DEFINE_GUID(CLSID_FFmpegSplitter,
	0xb0eb2d5c, 0x7474, 0x4a05,
	0x9d, 0xb4, 0x36, 0x59, 0xf7, 0x08, 0x1a, 0x27);

DEFINE_GUID(CLSID_FFmpegVideoDecoder,
	0x8666cbba, 0x3a17, 0x413a,
	0x94, 0x50, 0x31, 0xf1, 0xe3, 0xa7, 0x70, 0xe9);

DEFINE_GUID(CLSID_FFmpegAudioDecoder,
	0x33c8d7d3, 0x4e96, 0x42df,
	0x86, 0x3f, 0xd5, 0xdb, 0x36, 0x58, 0xa8, 0x92);

int main(int argc, char *argv[])
{
	HRESULT hr;
	WCHAR wbuf[MAX_PATH];
	HMODULE hModule = NULL;
	DLLGETCLASSOBJECT pDllGetClassObject = nullptr;
	IClassFactory *pClassFactory = nullptr;
	IGraphBuilder *pGraphBuilder = nullptr;
	IMediaControl *pMediaControl = nullptr;
	IMediaEvent *pMediaEvent = nullptr;
	IBaseFilter *pSourceFilter = nullptr;
	IBaseFilter *pFFmpegDSFilter = nullptr;
	IBaseFilter *pVideoRenderer = nullptr;
	IBaseFilter *pAudioRenderer = nullptr;
	IEnumPins *pEnumPins = nullptr;
	IPin *pPin = nullptr;
	IPin *pPinSourceOut = nullptr;
	FILTER_STATE fs;
	LONG event;

	if (argc < 3) {
		return -1;
	}
	MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, wbuf, ARRAYSIZE(wbuf));

	assert(S_OK == CoInitialize(nullptr));

	hModule = LoadLibraryA(argv[1]);
	assert(hModule != NULL);

	pDllGetClassObject =
		(DLLGETCLASSOBJECT)GetProcAddress(hModule, "DllGetClassObject");
	assert(pDllGetClassObject != nullptr);

	// CLSID_FilterGraph (-> IGraphBuilder, IMediaControl, IMediaEventEx)
	hr = CoCreateInstance(
		CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pGraphBuilder));
	assert(SUCCEEDED(hr));
	assert(pGraphBuilder != nullptr);
	assert(2 == pGraphBuilder->AddRef());
	assert(1 == pGraphBuilder->Release());
	hr = pGraphBuilder->QueryInterface(&pMediaControl);
	assert(SUCCEEDED(hr));
	assert(pMediaControl != nullptr);
	assert(3 == pMediaControl->AddRef());
	assert(2 == pMediaControl->Release());
	hr = pGraphBuilder->QueryInterface(IID_IMediaEvent, (void **)&pMediaEvent);
	assert(SUCCEEDED(hr));
	assert(pMediaEvent != nullptr);
	assert(4 == pMediaEvent->AddRef());
	assert(3 == pMediaEvent->Release());

	// CLSID_FFmpegSplitter (-> IBaseFilter)
	hr = pDllGetClassObject(
		CLSID_FFmpegSplitter, IID_PPV_ARGS(&pClassFactory));
	assert(SUCCEEDED(hr));
	assert(pClassFactory != nullptr);
	hr = pClassFactory->CreateInstance(
		nullptr, IID_PPV_ARGS(&pFFmpegDSFilter));
	assert(SUCCEEDED(hr));
	assert(pFFmpegDSFilter != nullptr);
	assert(2 == pFFmpegDSFilter->AddRef());
	assert(1 == pFFmpegDSFilter->Release());
	assert(0 == pClassFactory->Release());

	// CLSID_VideoRenderer (-> IBaseFilter)
	hr = CoCreateInstance(
		CLSID_VideoRenderer, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pVideoRenderer));
	assert(SUCCEEDED(hr));
	assert(pVideoRenderer != nullptr);
	assert(2 == pVideoRenderer->AddRef());
	assert(1 == pVideoRenderer->Release());

	// CLSID_DSoundRender (-> IBaseFilter)
	hr = CoCreateInstance(
		CLSID_DSoundRender, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pAudioRenderer));
	assert(SUCCEEDED(hr));
	assert(pAudioRenderer != nullptr);
	assert(2 == pAudioRenderer->AddRef());
	assert(1 == pAudioRenderer->Release());

	hr = pGraphBuilder->AddSourceFilter(wbuf, L"Source", &pSourceFilter);
	assert(SUCCEEDED(hr));
	assert(3 == pSourceFilter->AddRef());
	assert(2 == pSourceFilter->Release());
	hr = pGraphBuilder->AddFilter(pFFmpegDSFilter, L"Splitter/Decoder");
	assert(SUCCEEDED(hr));
	assert(3 == pFFmpegDSFilter->AddRef());
	assert(2 == pFFmpegDSFilter->Release());
	hr = pGraphBuilder->AddFilter(pVideoRenderer, L"Video Renderer");
	assert(SUCCEEDED(hr));
	assert(3 == pVideoRenderer->AddRef());
	assert(2 == pVideoRenderer->Release());
	hr = pGraphBuilder->AddFilter(pAudioRenderer, L"Audio Renderer");
	assert(SUCCEEDED(hr));
	assert(3 == pAudioRenderer->AddRef());
	assert(2 == pAudioRenderer->Release());

	// Source -> Splitter
	hr = pSourceFilter->EnumPins(&pEnumPins);
	assert(SUCCEEDED(hr));
	hr = pEnumPins->Reset();
	assert(SUCCEEDED(hr));
	while (S_OK == pEnumPins->Next(1, &pPin, nullptr)) {
		PIN_INFO info;
		hr = pPin->QueryPinInfo(&info);
		assert(SUCCEEDED(hr));
		if (info.dir == PINDIR_OUTPUT) {
			pPinSourceOut = pPin;
			break;
		}
	}
	assert(pPinSourceOut != nullptr);
	assert(0 == pEnumPins->Release());

	hr = pGraphBuilder->Render(pPinSourceOut);
	assert(SUCCEEDED(hr));

	hr = pMediaControl->Run();
	assert(SUCCEEDED(hr));

	hr = pMediaEvent->WaitForCompletion(INFINITE, &event);
	assert(SUCCEEDED(hr) || (hr == E_ABORT));

	hr = pMediaControl->Stop();
	assert(SUCCEEDED(hr));

	assert(2 == pMediaEvent->Release());
	assert(1 == pMediaControl->Release());
	assert(0 == pGraphBuilder->Release());

	assert(0 == pFFmpegDSFilter->Release());

	FreeLibrary(hModule);

	return 0;
}
