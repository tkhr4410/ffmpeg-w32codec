// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>

#include <assert.h>

#define LOG_TAG "Test"
#include "common.h"

int main(int argc, char *argv[])
{
	LRESULT ret = 0;
	HMODULE hModule = NULL;
	DRIVERPROC DriverProc = NULL;
	FOURCC fourcc_handler = mmioFOURCC('i', 'v', '5', '0');
	ICINFO ic_info = {};
	HIC hic = NULL;

	if (argc < 2) {
		return -1;
	}

	assert(S_OK == CoInitialize(nullptr));

	hModule = LoadLibraryA(argv[1]);
	assert(hModule != NULL);

	DriverProc = (DRIVERPROC)GetProcAddress(hModule, "DriverProc");
	assert(DriverProc != NULL);

	ret = ICInstall(
		ICTYPE_VIDEO, fourcc_handler, (LPARAM)DriverProc, nullptr,
		ICINSTALL_FUNCTION);
	assert(ret == TRUE);

	ret = ICInfo(ICTYPE_VIDEO, fourcc_handler, &ic_info);
	assert(ret == TRUE);

	hic = ICOpen(ICTYPE_VIDEO, fourcc_handler, ICMODE_DECOMPRESS);
	assert(hic != NULL);

	ret = ICClose(hic);
	assert(ret == ICERR_OK);

	if (argc >= 3) {
		HRESULT hr;
		PAVIFILE file;
		PAVISTREAM stream;
		PGETFRAME getframe;
		LPBITMAPINFO frame;
		AVISTREAMINFOA stream_info;
		BITMAPINFOHEADER bi = {};

		hr = AVIFileOpenA(&file, argv[2], OF_READ, nullptr);
		assert(hr == S_OK);

		hr = AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
		assert(hr == S_OK);

		hr = AVIStreamInfoA(stream, &stream_info, sizeof(stream_info));
		assert(hr == S_OK);

		bi.biSize = sizeof(bi);
		bi.biWidth = stream_info.rcFrame.right - stream_info.rcFrame.left;
		bi.biHeight = stream_info.rcFrame.bottom - stream_info.rcFrame.top;
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biSizeImage =
			((bi.biWidth * bi.biBitCount + 31) & ~31) / 8 * abs(bi.biHeight);
		getframe = AVIStreamGetFrameOpen(stream, &bi);
		assert(getframe != nullptr);

		frame = (LPBITMAPINFO)AVIStreamGetFrame(getframe, 0);
		assert(frame != nullptr);

		if (argc == 4) {
			BITMAPFILEHEADER bf = {};
			DWORD bitmap_size = sizeof(bi) + frame->bmiHeader.biSizeImage;
			DWORD cb = 0;
			HANDLE hFile = CreateFileA(
				argv[3], GENERIC_WRITE, 0, nullptr,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			assert(hFile != INVALID_HANDLE_VALUE);
			bf.bfType = 'B' | ('M' << 8);
			bf.bfSize = sizeof(bf) + bitmap_size;
			bf.bfOffBits = sizeof(bf) + sizeof(bi);
			assert(WriteFile(hFile, &bf, sizeof(bf), &cb, nullptr));
			assert(cb == sizeof(bf));
			assert(WriteFile(hFile, frame, bitmap_size, &cb, nullptr));
			assert(cb == bitmap_size);
			assert(CloseHandle(hFile));
		}

		hr = AVIStreamGetFrameClose(getframe);
		assert(hr == S_OK);

		assert(0 == AVIStreamRelease(stream));
		assert(0 == AVIFileRelease(file));
	}

	ret = ICRemove(ICTYPE_VIDEO, fourcc_handler, 0);
	assert(ret == TRUE);

	FreeLibrary(hModule);

	return 0;
}
