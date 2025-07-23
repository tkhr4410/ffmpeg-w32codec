// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#include <aviriff.h>

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
#if 0
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
#else
		const DWORD target_frame_index = 0;
		MMRESULT err;
		HMMIO hmmio;
		LONG offset;
		LONG len;
		MMCKINFO mmck_riff;
		MMCKINFO mmck_hdrl;
		MMCKINFO mmck_avih;
		MMCKINFO mmck_strl;
		MMCKINFO mmck_strh;
		MMCKINFO mmck_strf;
		MMCKINFO mmck_movi;
		MMCKINFO mmck_idx1;
		MMCKINFO mmck_xxdc;
		MainAVIHeader avih;
		AVIStreamHeader strh;
		AVISTREAMHEADER strh_;
		BITMAPINFOHEADER bi;
		AVIINDEXENTRY *idx1;
		DWORD idx1_count;
		DWORD stream_index;
		DWORD frame_index;
		DWORD frame_ckid;
		DWORD frame_offset;
		LPBYTE frame_data;
		LPBYTE bitmap;
		BITMAPINFOHEADER src_fmt;
		BITMAPINFOHEADER dst_fmt;

		hmmio = mmioOpenA(argv[2], nullptr, MMIO_READ);
		assert(hmmio != NULL);

		ZeroMemory(&mmck_riff, sizeof(mmck_riff));
		mmck_riff.fccType = formtypeAVI;
		err = mmioDescend(hmmio, &mmck_riff, nullptr, MMIO_FINDRIFF);
		assert(err == MMSYSERR_NOERROR);

		ZeroMemory(&mmck_hdrl, sizeof(mmck_hdrl));
		mmck_hdrl.fccType = listtypeAVIHEADER;
		err = mmioDescend(hmmio, &mmck_hdrl, &mmck_riff, MMIO_FINDLIST);
		assert(err == MMSYSERR_NOERROR);

		ZeroMemory(&mmck_avih, sizeof(mmck_avih));
		mmck_avih.ckid = ckidAVIMAINHDR;
		err = mmioDescend(hmmio, &mmck_avih, &mmck_hdrl, MMIO_FINDCHUNK);
		assert(err == MMSYSERR_NOERROR);
		offset = mmioSeek(hmmio, mmck_avih.dwDataOffset, SEEK_SET);
		assert(offset == mmck_avih.dwDataOffset);
		len = mmioRead(hmmio, (HPSTR)&avih, sizeof(avih));
		assert(len == sizeof(avih));

		ZeroMemory(&strh, sizeof(strh));
		ZeroMemory(&bi, sizeof(bi));

		for (stream_index = 0; stream_index < avih.dwStreams; stream_index++) {
			ZeroMemory(&mmck_strl, sizeof(mmck_strl));
			mmck_strl.fccType = listtypeSTREAMHEADER;
			err = mmioDescend(hmmio, &mmck_strl, &mmck_hdrl, MMIO_FINDLIST);
			assert(err == MMSYSERR_NOERROR);

			ZeroMemory(&mmck_strh, sizeof(mmck_strh));
			mmck_strh.ckid = ckidSTREAMHEADER;
			err = mmioDescend(hmmio, &mmck_strh, &mmck_strl, MMIO_FINDCHUNK);
			assert(err == MMSYSERR_NOERROR);
			if (mmck_strh.cksize == sizeof(strh)) {
				offset = mmioSeek(hmmio, mmck_strh.dwDataOffset, SEEK_SET);
				assert(offset == mmck_strh.dwDataOffset);
				len = mmioRead(hmmio, (HPSTR)&strh, sizeof(strh));
				assert(len == sizeof(strh));
			} else if (mmck_strh.cksize == sizeof(strh_) - 8) {
				offset = mmioSeek(hmmio, mmck_strh.dwDataOffset - 8, SEEK_SET);
				assert(offset == mmck_strh.dwDataOffset - 8);
				len = mmioRead(hmmio, (HPSTR)&strh_, sizeof(strh_));
				assert(len == sizeof(strh_));
				assert(strh_.fcc == ckidSTREAMHEADER);
				assert(strh_.cb == len - 8);
				strh.fccType				= strh_.fccType;
				strh.fccHandler				= strh_.fccHandler;
				strh.dwFlags				= strh_.dwFlags;
				strh.wPriority				= strh_.wPriority;
				strh.wLanguage				= strh_.wLanguage;
				strh.dwInitialFrames		= strh_.dwInitialFrames;
				strh.dwScale				= strh_.dwScale;
				strh.dwRate					= strh_.dwRate;
				strh.dwStart				= strh_.dwStart;
				strh.dwLength				= strh_.dwLength;
				strh.dwSuggestedBufferSize	= strh_.dwSuggestedBufferSize;
				strh.dwQuality				= strh_.dwQuality;
				strh.dwSampleSize			= strh_.dwSampleSize;
				strh.rcFrame.left			= strh_.rcFrame.left;
				strh.rcFrame.top			= strh_.rcFrame.top;
				strh.rcFrame.right			= strh_.rcFrame.right;
				strh.rcFrame.bottom			= strh_.rcFrame.bottom;
			}
			if (strh.fccType != streamtypeVIDEO) {
				continue;
			}
			ZeroMemory(&mmck_strf, sizeof(mmck_strf));
			mmck_strf.ckid = ckidSTREAMFORMAT;
			err = mmioDescend(hmmio, &mmck_strf, &mmck_strl, MMIO_FINDCHUNK);
			assert(err == MMSYSERR_NOERROR);
			assert(mmck_strf.cksize == sizeof(bi));
			offset = mmioSeek(hmmio, mmck_strf.dwDataOffset, SEEK_SET);
			assert(offset == mmck_strf.dwDataOffset);
			len = mmioRead(hmmio, (HPSTR)&bi, sizeof(bi));
			assert(len == sizeof(bi));
			break;
		}
		assert(strh.fccType == streamtypeVIDEO);
		assert(bi.biSize == sizeof(bi));

		ZeroMemory(&mmck_movi, sizeof(mmck_movi));
		mmck_movi.fccType = listtypeAVIMOVIE;
		err = mmioDescend(hmmio, &mmck_movi, &mmck_riff, MMIO_FINDLIST);
		assert(err == MMSYSERR_NOERROR);

		assert(avih.dwFlags & AVIF_HASINDEX);
		ZeroMemory(&mmck_idx1, sizeof(mmck_idx1));
		mmck_idx1.ckid = ckidAVINEWINDEX;
		err = mmioDescend(hmmio, &mmck_idx1, &mmck_riff, MMIO_FINDCHUNK);
		assert(err == MMSYSERR_NOERROR);
		idx1_count = mmck_idx1.cksize / sizeof(idx1[0]);
		assert(idx1_count != 0);
		idx1 = new AVIINDEXENTRY[idx1_count];
		assert(idx1 != nullptr);
		offset = mmioSeek(hmmio, mmck_idx1.dwDataOffset, SEEK_SET);
		assert(offset == mmck_idx1.dwDataOffset);
		len = mmioRead(hmmio, (HPSTR)idx1, mmck_idx1.cksize);
		assert(len == mmck_idx1.cksize);
		if (idx1[0].dwChunkOffset < mmck_movi.dwDataOffset) {
			for (DWORD i=0; i<idx1_count; i++) {
				idx1[i].dwChunkOffset += mmck_movi.dwDataOffset;
			}
		}

		assert(stream_index < 10);
		frame_ckid = mmioFOURCC('0', '0' + stream_index, 'd', 'c');
		frame_index = 0;
		frame_offset = 0;
		for (DWORD i=0; i<idx1_count; i++) {
			if (idx1[i].ckid == frame_ckid) {
				if (frame_index == target_frame_index) {
					frame_offset = idx1[i].dwChunkOffset;
					break;
				}
				frame_index++;
			}
		}
		assert(frame_offset != 0);

		offset = mmioSeek(hmmio, frame_offset, SEEK_SET);
		assert(offset == frame_offset);
		err = mmioDescend(hmmio, &mmck_xxdc, nullptr, 0);
		assert(err == MMSYSERR_NOERROR);
		frame_data = new BYTE[mmck_xxdc.cksize];
		assert(frame_data != nullptr);
		len = mmioRead(hmmio, (HPSTR)frame_data, mmck_xxdc.cksize);
		assert(len == mmck_xxdc.cksize);

		err = mmioClose(hmmio, 0);
		assert(err == MMSYSERR_NOERROR);

		ret = ICInstall(
			ICTYPE_VIDEO, fourcc_handler, (LPARAM)DriverProc, nullptr,
			ICINSTALL_FUNCTION);
		assert(ret == TRUE);

		ret = ICInfo(ICTYPE_VIDEO, fourcc_handler, &ic_info);
		assert(ret == TRUE);

		hic = ICOpen(ICTYPE_VIDEO, strh.fccHandler, ICMODE_DECOMPRESS);
		assert(hic != NULL);

		src_fmt = bi;
		ZeroMemory(&dst_fmt, sizeof(dst_fmt));
		dst_fmt.biSize			= sizeof(dst_fmt);
		dst_fmt.biWidth			= src_fmt.biWidth;
		dst_fmt.biHeight		= src_fmt.biHeight;
		dst_fmt.biPlanes		= 1;
		dst_fmt.biBitCount		= 24;
		dst_fmt.biCompression	= BI_RGB;

		src_fmt.biSizeImage = mmck_xxdc.cksize;
		dst_fmt.biSizeImage = abs(dst_fmt.biHeight)
			* (((dst_fmt.biWidth * dst_fmt.biBitCount + 31) & ~31) / 8);
		ret = ICDecompressQuery(hic, &src_fmt, &dst_fmt);
		assert(ret == ICERR_OK);
		ret = ICDecompressBegin(hic, &src_fmt, &dst_fmt);
		assert(ret == ICERR_OK);

		bitmap = new BYTE[dst_fmt.biSizeImage];
		ret = ICDecompress(hic, 0, &src_fmt, frame_data, &dst_fmt, bitmap);
		assert(ret == ICERR_OK);

		ret = ICClose(hic);
		assert(ret == ICERR_OK);

		if (argc == 4) {
			BITMAPFILEHEADER bf = {};
			DWORD bitmap_size = sizeof(bi) + dst_fmt.biSizeImage;
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
			assert(WriteFile(hFile, &dst_fmt, sizeof(dst_fmt), &cb, nullptr));
			assert(cb == sizeof(dst_fmt));
			assert(WriteFile(hFile, bitmap, dst_fmt.biSizeImage, &cb, nullptr));
			assert(cb == dst_fmt.biSizeImage);
			assert(CloseHandle(hFile));
		}

		delete [] idx1;
		delete [] frame_data;
		delete [] bitmap;
#endif
	}

	ret = ICRemove(ICTYPE_VIDEO, fourcc_handler, 0);
	assert(ret == TRUE);

	FreeLibrary(hModule);

	return 0;
}
