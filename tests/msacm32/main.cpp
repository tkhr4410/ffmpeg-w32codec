// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <assert.h>

#define LOG_TAG "Test"
#include "common.h"

int main(int argc, char *argv[])
{
	HANDLE hHeap = GetProcessHeap();
	MMRESULT err = MMSYSERR_NOERROR;
	HMODULE hModule = NULL;
	DRIVERPROC DriverProc = NULL;
	HACMDRIVERID hadid = NULL;
	HACMDRIVER had = NULL;
	ACMDRIVERDETAILS drv = {};

	if (argc < 2) {
		return -1;
	}

	hModule = LoadLibraryA(argv[1]);
	assert(hModule != NULL);

	DriverProc = (DRIVERPROC)GetProcAddress(hModule, "DriverProc");
	assert(DriverProc != NULL);

	// acmDriverAdd
	err = acmDriverAdd(
		&hadid, hModule, (LPARAM)DriverProc, 0, ACM_DRIVERADDF_FUNCTION);
	assert(err == MMSYSERR_NOERROR);
	assert(hadid != NULL);

	// acmDriverOpen
	err = acmDriverOpen(&had, hadid, 0);
	assert(err == MMSYSERR_NOERROR);
	assert(had != NULL);

	// acmDriverDetails
	drv.cbStruct = sizeof(drv);
	err = acmDriverDetails(hadid, &drv, 0);
	assert(err == MMSYSERR_NOERROR);
	assert(drv.cbStruct == sizeof(drv));
	assert(drv.fccType == ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC);
	assert(drv.fccComp == ACMDRIVERDETAILS_FCCCOMP_UNDEFINED);
	assert(drv.wMid == MM_UNMAPPED);
	assert(drv.wPid == MM_PID_UNMAPPED);
	assert(drv.fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC);
	assert(drv.cFormatTags != 0);

	// acmFormatTagDetails
	for (DWORD i=0; i<drv.cFormatTags; i++) {
		ACMFORMATTAGDETAILS tag = {};
		tag.cbStruct = sizeof(tag);
		tag.dwFormatTagIndex = i;
		err = acmFormatTagDetails(had, &tag, ACM_FORMATTAGDETAILSF_INDEX);
		assert(err == MMSYSERR_NOERROR);
		assert(tag.cbStruct == sizeof(tag));
		assert(tag.dwFormatTagIndex == i);
		assert(tag.dwFormatTag != 0);
		assert(tag.cbFormatSize != 0);
		assert(tag.fdwSupport == ACMDRIVERDETAILS_SUPPORTF_CODEC);
		assert(tag.cStandardFormats != 0);

		// acmFormatDetails
		for (DWORD j=0; j<tag.cStandardFormats; j++) {
			ACMFORMATDETAILS fmt = {};
			WAVEFORMATEX wfx = {};
			WAVEFORMATEX pcm = {};
			HACMSTREAM has = NULL;
			DWORD bytes = 0;
			fmt.cbStruct = sizeof(fmt);
			fmt.dwFormatIndex = j;
			fmt.dwFormatTag = tag.dwFormatTag;
			fmt.pwfx = &wfx;
			fmt.cbwfx = sizeof(wfx);
			err = acmFormatDetails(had, &fmt, ACM_FORMATDETAILSF_INDEX);
			assert(err == MMSYSERR_NOERROR);
			assert(fmt.cbStruct == sizeof(fmt));
			assert(fmt.dwFormatIndex == j);
			assert(fmt.dwFormatTag == tag.dwFormatTag);
			assert(fmt.fdwSupport == ACMDRIVERDETAILS_SUPPORTF_CODEC);
			assert(wfx.wFormatTag == fmt.dwFormatTag);
			assert((wfx.nChannels == 1) || (wfx.nChannels == 2));
			assert(wfx.nSamplesPerSec != 0);
			assert(wfx.nAvgBytesPerSec != 0);
			assert(wfx.nBlockAlign != 0);
			LOGI("");
			LOGI("[%s] - [%s]", tag.szFormatTag, fmt.szFormat);
			LOGI("");
			if (wfx.wFormatTag == WAVE_FORMAT_PCM) {
				continue;
			}

			// acmFormatSuggest
			pcm.wFormatTag = WAVE_FORMAT_PCM;
			err = acmFormatSuggest(
				had, &wfx, &pcm, sizeof(pcm), ACM_FORMATSUGGESTF_WFORMATTAG);
			assert(err == MMSYSERR_NOERROR);
			assert(pcm.wFormatTag == WAVE_FORMAT_PCM);
			assert(pcm.nChannels == wfx.nChannels);
			assert(pcm.nSamplesPerSec == wfx.nSamplesPerSec);
		}
	}

	if (argc >= 3) {
		HMMIO hmmio = NULL;
		MMCKINFO mmck_riff = {};
		MMCKINFO mmck_fmt = {};
		MMCKINFO mmck_data = {};
		LONG read_len = 0;
		LPWAVEFORMATEX src_fmt = nullptr;
		LPBYTE src = nullptr;
		WAVEFORMATEX dst_fmt = {};
		LPBYTE dst = nullptr;
		DWORD dst_len = 0;
		HACMSTREAM has = NULL;
		ACMSTREAMHEADER ash = {};

		hmmio = mmioOpenA(argv[2], nullptr, MMIO_READ);
		assert(hmmio != NULL);
		mmck_riff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		err = mmioDescend(hmmio, &mmck_riff, nullptr, MMIO_FINDRIFF);
		assert(err == MMSYSERR_NOERROR);
		mmck_fmt.ckid = mmioFOURCC('f', 'm', 't', ' ');
		err = mmioDescend(hmmio, &mmck_fmt, &mmck_riff, MMIO_FINDCHUNK);
		assert(err == MMSYSERR_NOERROR);
		src_fmt = (LPWAVEFORMATEX)HeapAlloc(hHeap, 0, mmck_fmt.cksize);
		assert(src_fmt != nullptr);
		read_len = mmioRead(hmmio, (HPSTR)src_fmt, mmck_fmt.cksize);
		assert(read_len == mmck_fmt.cksize);
		err = mmioAscend(hmmio, &mmck_fmt, 0);
		assert(err == MMSYSERR_NOERROR);
		mmck_data.ckid = mmioFOURCC('d', 'a', 't', 'a');
		err = mmioDescend(hmmio, &mmck_data, &mmck_riff, MMIO_FINDCHUNK);
		assert(err == MMSYSERR_NOERROR);
		src = (LPBYTE)HeapAlloc(hHeap, 0, mmck_data.cksize);
		assert(src != nullptr);
		read_len = mmioRead(hmmio, (HPSTR)src, mmck_data.cksize);
		assert(read_len == mmck_data.cksize);
		err = mmioAscend(hmmio, &mmck_data, 0);
		assert(err == MMSYSERR_NOERROR);
		err = mmioClose(hmmio, 0);
		assert(err == MMSYSERR_NOERROR);

		// acmFormatSuggest
		dst_fmt.wFormatTag = WAVE_FORMAT_PCM;
		err = acmFormatSuggest(
			had, src_fmt, &dst_fmt, sizeof(dst_fmt),
			ACM_FORMATSUGGESTF_WFORMATTAG);
		assert(err == MMSYSERR_NOERROR);

		// acmStreamOpen
		err = acmStreamOpen(&has, had, src_fmt, &dst_fmt, nullptr, 0, 0, 0);
		assert(err == MMSYSERR_NOERROR);

		// acmStreamSize
		err = acmStreamSize(
			has, mmck_data.cksize, &dst_len, ACM_STREAMSIZEF_SOURCE);
		assert(err == MMSYSERR_NOERROR);
		dst = (LPBYTE)HeapAlloc(hHeap, 0, dst_len);
		assert(dst != nullptr);

		// acmStreamPrepareHeader
		ash.cbStruct = sizeof(ash);
		ash.pbSrc = src;
		ash.cbSrcLength = mmck_data.cksize;
		ash.pbDst = dst;
		ash.cbDstLength = dst_len;
		err = acmStreamPrepareHeader(has, &ash, 0);
		assert(err == MMSYSERR_NOERROR);

		// acmStreamConvert
		err = acmStreamConvert(has, &ash, 0);
		assert(err == MMSYSERR_NOERROR);

		// acmStreamUnprepareHeader
		err = acmStreamUnprepareHeader(has, &ash, 0);
		assert(err == MMSYSERR_NOERROR);

		// acmStreamClose
		err = acmStreamClose(has, 0);
		assert(err == MMSYSERR_NOERROR);

		if (argc == 4) {
			DWORD cb = 0;
			HANDLE hFile = CreateFileA(
				argv[3], GENERIC_WRITE, 0, nullptr,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			assert(hFile != INVALID_HANDLE_VALUE);
			assert(WriteFile(hFile, dst, ash.cbDstLengthUsed, &cb, nullptr));
			assert(cb == ash.cbDstLengthUsed);
			assert(CloseHandle(hFile));
		}

		assert(HeapFree(hHeap, 0, src_fmt));
		assert(HeapFree(hHeap, 0, src));
		assert(HeapFree(hHeap, 0, dst));
	}

	// acmDriverClose
	err = acmDriverClose(had, 0);
	assert(err == MMSYSERR_NOERROR);

	// acmDriverRemove
	err = acmDriverRemove(hadid, 0);
	assert(err == MMSYSERR_NOERROR);

	FreeLibrary(hModule);

	return 0;
}
