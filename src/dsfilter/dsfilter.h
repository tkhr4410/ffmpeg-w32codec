// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#pragma once

#include "streams.h"
#undef __in
#undef __out

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define LOG_TAG "DSFilter"
#include "common.h"

#include <queue>

extern REFGUID get_video_subtype(AVCodecID codec_id);
extern REFGUID get_audio_subtype(AVCodecID codec_id);

extern AVCodecID get_video_codec_id(REFGUID subtype);
extern AVCodecID get_audio_codec_id(REFGUID subtype);

extern bool is_subtype_video_out(REFGUID subtype);
extern bool is_subtype_audio_out(REFGUID subtype);

extern AVPixelFormat get_pixel_format(REFGUID subtype);

extern REFGUID get_subtype_video_out(unsigned int index);
extern REFGUID get_subtype_audio_out(unsigned int index);

extern bool get_format_video_out(unsigned int index, VIDEOINFOHEADER *vih);
extern bool get_format_audio_out(unsigned int index, WAVEFORMATEX *wfx);

extern WORD get_wave_format_tag(AVCodecID codec_id);

extern AVSampleFormat get_sample_format(const WAVEFORMATEX *wfx);

///////////////////////////////////////////////////////////////////////////////

class CFFmpegStreamIn;
class CFFmpegStreamOut;

enum {
	THREAD_REQ_NONE,
	THREAD_REQ_EXIT,
};

class DECLSPEC_UUID("b0eb2d5c-7474-4a05-9db4-3659f7081a27") CFFmpegDSFilter
	: public CBaseFilter, public CCritSec, protected CAMThread, IMediaSeeking
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	CFFmpegDSFilter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CFFmpegDSFilter();

	HRESULT Init(void);
	int Decode(AVFrame *frame, const AVPacket *packet);
	size_t ConvertVideoFrame(
		AVFrame *dst, const AVFrame *src, const VIDEOINFOHEADER *vih,
		REFGUID fmt);
	size_t ConvertAudioFrame(
		AVFrame *dst, const AVFrame *src, const WAVEFORMATEX *wfx);

	// CUnknown methods
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// CBaseFilter methods
	int GetPinCount();
	CBasePin * GetPin(int n);
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IMediaSeeking methods
	STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID *pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
	STDMETHODIMP SetTimeFormat(const GUID *pFormat);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP ConvertTimeFormat(
		LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source,
		const GUID *pSourceFormat);
	STDMETHODIMP SetPositions(
		LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,
		DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double *pdRate);
	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);

protected:
	DWORD ThreadProc(void);

private:
	CFFmpegStreamIn *	m_pStreamIn;
	CFFmpegStreamOut *	m_pVideoOut;
	CFFmpegStreamOut *	m_pAudioOut;

	AVIOContext *		m_pAVIOContext;
	AVFormatContext *	m_pAVFormatContext;
	AVCodecContext *	m_pVideoCodecContext;
	AVCodecContext *	m_pAudioCodecContext;
	SwsContext *		m_pSwsContext;
	SwrContext *		m_pSwrContext;
	int					m_videoStreamIndex;
	int					m_audioStreamIndex;
};

#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(CFFmpegDSFilter,
	0xb0eb2d5c, 0x7474, 0x4a05,
	0x9d, 0xb4, 0x36, 0x59, 0xf7, 0x08, 0x1a, 0x27);
#endif // __CRT_UUID_DECL

///////////////////////////////////////////////////////////////////////////////

class CFFmpegStreamIn : public CBaseInputPin, public CCritSec
{
public:
	CFFmpegStreamIn(
		LPCTSTR pObjectName, CFFmpegDSFilter *pFilter, CCritSec *pLock,
		HRESULT *phr);
	virtual ~CFFmpegStreamIn();

	static int Read(void *opaque, uint8_t *buf, int buf_size);
	static int64_t Seek(void *opaque, int64_t offset, int whence);

	// CUnknown methods
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// CBaseInputPin methods
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT CheckConnect(IPin *pPin);
	HRESULT CompleteConnect(IPin *pReceivePin);

private:
	int64_t				m_offset;
	IAsyncReader *		m_pAsyncReader;
};

///////////////////////////////////////////////////////////////////////////////

class CFFmpegStreamOut
	: public CBaseOutputPin, public CCritSec, protected CAMThread,
	  IMediaSeeking
{
public:
	CFFmpegStreamOut(
		LPCTSTR pObjectName, CFFmpegDSFilter *pFilter, CCritSec *pLock,
		HRESULT *phr, LPCWSTR pName,
		const AVStream *pAVStream, const AVCodec *pAVCodec);
	virtual ~CFFmpegStreamOut();

	void EnqueuePacket(AVPacket *packet) {
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvent, INFINITE)) {
			CAutoLock lock(this);
			m_packetQueue.push(packet);
			if (m_packetQueue.size() >= 256) {
				ResetEvent(m_hEvent);
			}
		}
	}

	void WaitThread(void) {
		WaitForSingleObject(m_hThread, INFINITE);
	}

	// CUnknown methods
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// CBaseOutputPin methods
	HRESULT CheckConnect(IPin *pPin);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT DecideBufferSize(
		IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT Active(void);
	HRESULT Inactive(void);

	// IQualityControl methods
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) {
		return E_NOTIMPL;
	}

	// IMediaSeeking methods
	STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID *pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
	STDMETHODIMP SetTimeFormat(const GUID *pFormat);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP ConvertTimeFormat(
		LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source,
		const GUID *pSourceFormat);
	STDMETHODIMP SetPositions(
		LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,
		DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double *pdRate);
	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);

protected:
	DWORD ThreadProc(void);

private:
	AVPacket * DequeuePacket(void) {
		CAutoLock lock(this);
		AVPacket *packet;
		if (m_packetQueue.empty()) {
			return nullptr;
		}
		packet = m_packetQueue.front();
		m_packetQueue.pop();
		SetEvent(m_hEvent);
		return packet;
	}

	size_t Convert(AVFrame *dst, const AVFrame *src);
	HRESULT Prepare(
		IMediaSample *pSample, const AVFrame *frame, size_t size,
		size_t *offset);

	std::queue<AVPacket *>	m_packetQueue;
	HANDLE					m_hEvent;
	double					m_timeBase;
	LONGLONG				m_preroll;
};
