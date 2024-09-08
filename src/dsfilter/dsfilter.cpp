// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "dsfilter.h"

#define AVIO_BUFFER_SIZE	0x10000

CUnknown * WINAPI CFFmpegDSFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	return new CFFmpegDSFilter(pUnk, phr);
}

CFFmpegDSFilter::CFFmpegDSFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(
		NAME("ffmpeg-w32codec DirectShow Filter"), pUnk, this, __uuidof(this),
		phr),
	m_pVideoOut(nullptr), m_pAudioOut(nullptr), m_pAVFormatContext(nullptr),
	m_pVideoCodecContext(nullptr), m_pAudioCodecContext(nullptr),
	m_pSwsContext(nullptr), m_pSwrContext(nullptr),
	m_videoStreamIndex(-1), m_audioStreamIndex(-1)
{
	unsigned char *buf = (unsigned char *)av_malloc(AVIO_BUFFER_SIZE);
	if (buf == nullptr) {
		LOGE("av_malloc() failed.");
		*phr = E_OUTOFMEMORY;
		return;
	}

	m_pStreamIn = new CFFmpegStreamIn(
		NAME("ffmpeg-w32codec Stream In"), this, this, phr);

	m_pAVIOContext = avio_alloc_context(
		buf, AVIO_BUFFER_SIZE, 0, m_pStreamIn,
		CFFmpegStreamIn::Read, nullptr, CFFmpegStreamIn::Seek);
	if (m_pAVIOContext == nullptr) {
		LOGE("avio_alloc_context() failed.");
		*phr = E_OUTOFMEMORY;
		return;
	}

	*phr = S_OK;
}

CFFmpegDSFilter::~CFFmpegDSFilter()
{
	if (m_pStreamIn != nullptr) {
		delete m_pStreamIn;
	}
	if (m_pVideoOut != nullptr) {
		delete m_pVideoOut;
	}
	if (m_pAudioOut != nullptr) {
		delete m_pAudioOut;
	}

	if (m_pAVFormatContext != nullptr) {
		avformat_free_context(m_pAVFormatContext);
	}
	if (m_pAVIOContext != nullptr) {
		av_freep(&m_pAVIOContext->buffer);
		avio_context_free(&m_pAVIOContext);
	}
	if (m_pVideoCodecContext != nullptr) {
		avcodec_free_context(&m_pVideoCodecContext);
	}
	if (m_pAudioCodecContext != nullptr) {
		avcodec_free_context(&m_pAudioCodecContext);
	}
	if (m_pSwsContext != nullptr) {
		sws_freeContext(m_pSwsContext);
	}
	if (m_pSwrContext != nullptr) {
		swr_free(&m_pSwrContext);
	}
}

STDMETHODIMP CFFmpegDSFilter::NonDelegatingQueryInterface(
	REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking) {
		return GetInterface(static_cast<IMediaSeeking *>(this), ppv);
	}
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFFmpegDSFilter::Init(void)
{
	HRESULT hr;
	AVFormatContext *fmt;
	const AVStream *stream;
	const AVCodec *codec;
	int ret;
	int index;

	fmt = avformat_alloc_context();
	fmt->pb = m_pAVIOContext;

	ret = avformat_open_input(&fmt, nullptr, nullptr, nullptr);
	if (ret < 0) {
		LOGE("avformat_open_input() failed. (%d)", ret);
		goto failed;
	}

	if (fmt->probesize > AVIO_BUFFER_SIZE) {
		fmt->probesize = AVIO_BUFFER_SIZE;
	}
	ret = avformat_find_stream_info(fmt, nullptr);
	if (ret < 0) {
		LOGW("avformat_find_stream_info() failed. (%d)", ret);
		// continue
	}

	index = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (index >= 0) {
		stream = fmt->streams[index];
		codec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (codec == nullptr) {
			LOGE("avcodec_find_decoder() failed.");
			goto failed;
		}
		m_pVideoCodecContext = avcodec_alloc_context3(codec);
		m_pVideoCodecContext->width = stream->codecpar->width;
		m_pVideoCodecContext->height = stream->codecpar->height;
		if (0 != avcodec_open2(m_pVideoCodecContext, codec, nullptr)) {
			LOGE("avcodec_open2() failed.");
			goto failed;
		}
		m_pVideoOut = new CFFmpegStreamOut(
			NAME("ffmpeg-w32codec Video Out"), this, this, &hr, L"Video Out",
			stream, codec);
		if (FAILED(hr)) {
			LOGE("new CFFmpegStreamOut(video) failed.");
			delete m_pVideoOut;
			m_pVideoOut = nullptr;
			goto failed;
		}
	}
	m_videoStreamIndex = index;

	index = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (index >= 0) {
		stream = fmt->streams[index];
		codec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (codec == nullptr) {
			LOGE("avcodec_find_decoder() failed.");
			goto failed;
		}
		m_pAudioCodecContext = avcodec_alloc_context3(codec);
		m_pAudioCodecContext->ch_layout = stream->codecpar->ch_layout;
		m_pAudioCodecContext->sample_rate = stream->codecpar->sample_rate;
		m_pAudioCodecContext->block_align = stream->codecpar->block_align;
		m_pAudioCodecContext->bits_per_coded_sample =
			stream->codecpar->bits_per_coded_sample;
		if (0 != avcodec_open2(m_pAudioCodecContext, codec, nullptr)) {
			LOGE("avcodec_open2() failed.");
			goto failed;
		}
		m_pAudioOut = new CFFmpegStreamOut(
			NAME("ffmpeg-w32codec Audio Out"), this, this, &hr, L"Audio Out",
			stream, codec);
		if (FAILED(hr)) {
			LOGE("new CFFmpegStreamOut(audio) failed.");
			delete m_pAudioOut;
			m_pAudioOut = nullptr;
			goto failed;
		}
	}
	m_audioStreamIndex = index;

	if (m_pAVFormatContext != nullptr) {
		avformat_free_context(m_pAVFormatContext);
	}
	m_pAVFormatContext = fmt;

	return S_OK;

failed:
	avformat_free_context(fmt);
	return E_FAIL;
}

int CFFmpegDSFilter::Decode(AVFrame *frame, const AVPacket *packet)
{
	int ret;
	AVCodecContext *avctx;

	if (packet->stream_index == m_videoStreamIndex) {
		avctx = m_pVideoCodecContext;
	} else if (packet->stream_index == m_audioStreamIndex) {
		avctx = m_pAudioCodecContext;
	} else {
		return -1;
	}

	ret = avcodec_send_packet(avctx, packet);
	if (ret < 0) {
		LOGE("avcodec_send_packet() failed. (%d)", ret);
		return ret;
	}

	ret = avcodec_receive_frame(avctx, frame);
	if (ret < 0) {
		if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
			LOGE("avcodec_receive_frame() failed. (%d)", ret);
		}
	}

	return ret;
}

size_t CFFmpegDSFilter::ConvertVideoFrame(
	AVFrame *dst, const AVFrame *src, const VIDEOINFOHEADER *vih,
	REFGUID subtype)
{
	int ret;

	m_pSwsContext = sws_getCachedContext(
		m_pSwsContext,
		m_pVideoCodecContext->width, m_pVideoCodecContext->height,
		m_pVideoCodecContext->pix_fmt,
		vih->bmiHeader.biWidth, abs(vih->bmiHeader.biHeight),
		get_pixel_format(subtype),
		SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

	ret = sws_scale_frame(m_pSwsContext, dst, src);
	if (ret < 0) {
		LOGE("sws_scale_frame() failed. (%d)", ret);
		return (size_t)-1;
	}

	return dst->linesize[0] * vih->bmiHeader.biHeight;
}

size_t CFFmpegDSFilter::ConvertAudioFrame(
	AVFrame *dst, const AVFrame *src, const WAVEFORMATEX *wfx)
{
	int ret;

	av_channel_layout_default(&dst->ch_layout, wfx->nChannels);
	dst->sample_rate = wfx->nSamplesPerSec;
	dst->format = get_sample_format(wfx);

	if (m_pSwrContext == nullptr) {
		ret = swr_alloc_set_opts2(
			&m_pSwrContext, &dst->ch_layout,
			(AVSampleFormat)dst->format, dst->sample_rate,
			&src->ch_layout, (AVSampleFormat)src->format, src->sample_rate,
			0, nullptr);
		if (ret < 0) {
			LOGE("swr_alloc_set_opts2() failed. (%d)", ret);
			return (size_t)-1;
		}
		ret = swr_init(m_pSwrContext);
		if (ret < 0) {
			LOGE("swr_init() failed. (%d)", ret);
			return (size_t)-1;
		}
	}

	ret = swr_config_frame(m_pSwrContext, dst, src);
	if (ret < 0) {
		LOGE("swr_config_frame() failed. (%d)", ret);
		return (size_t)-1;
	}

	ret = swr_convert_frame(m_pSwrContext, dst, src);
	if (ret < 0) {
		LOGE("swr_convert_frame() failed. (%d)", ret);
		av_frame_free(&dst);
		return (size_t)-1;
	}

	return dst->nb_samples * wfx->nBlockAlign;
}

int CFFmpegDSFilter::GetPinCount()
{
	int ret = 0;

	if (m_pStreamIn != nullptr) {
		ret++;
	}
	if (m_pVideoOut != nullptr) {
		ret++;
	}
	if (m_pAudioOut != nullptr) {
		ret++;
	}

	return ret;
}

CBasePin * CFFmpegDSFilter::GetPin(int n)
{
	switch (n)
	{
	case 0:
		return m_pStreamIn;

	case 1:
		if (m_pVideoOut != nullptr) {
			return m_pVideoOut;
		} else {
			return m_pAudioOut;
		}

	case 2:
		if (m_pVideoOut != nullptr) {
			return m_pAudioOut;
		} else {
			return nullptr;
		}

	default:
		return nullptr;
	}
}

STDMETHODIMP CFFmpegDSFilter::Stop()
{
	HRESULT hr;

	if (ThreadExists()) {
		CallWorker(THREAD_REQ_EXIT);
		Close();
	}
	hr = CBaseFilter::Stop();
	if (FAILED(hr) && (hr != VFW_E_NO_ALLOCATOR)) {
		LOGE("CBaseFilter::Stop() failed. (0x%08x)", hr);
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CFFmpegDSFilter::Pause()
{
	HRESULT hr = CBaseFilter::Pause();
	if (FAILED(hr)) {
		return hr;
	}
	if (!ThreadExists()) {
		Create();
	}

	return S_OK;
}

STDMETHODIMP CFFmpegDSFilter::Run(REFERENCE_TIME tStart)
{
	return CBaseFilter::Run(tStart);
}

STDMETHODIMP CFFmpegDSFilter::GetCapabilities(DWORD *pCapabilities)
{
	*pCapabilities =
		AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards |
		AM_SEEKING_CanGetCurrentPos |
		AM_SEEKING_CanGetStopPos |
		AM_SEEKING_CanGetDuration;

	return S_OK;
}

STDMETHODIMP CFFmpegDSFilter::CheckCapabilities(DWORD *pCapabilities)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::IsFormatSupported(const GUID *pFormat)
{
	OLECHAR *fmt_str;

	if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
		return S_OK;
	} else {
		StringFromCLSID(*pFormat, &fmt_str);
		LOGE("%s: pFormat=%ls -> E_FAIL", __FUNCTION__, fmt_str);
		CoTaskMemFree(fmt_str);
		return E_FAIL;
	}
}

STDMETHODIMP CFFmpegDSFilter::QueryPreferredFormat(GUID *pFormat)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetTimeFormat(GUID *pFormat)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::IsUsingTimeFormat(const GUID *pFormat)
{
	if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
		return S_OK;
	} else {
		OLECHAR *fmt_str;
		StringFromCLSID(*pFormat, &fmt_str);
		LOGE("%s: pFormat=%ls -> E_FAIL", __FUNCTION__, fmt_str);
		CoTaskMemFree(fmt_str);
		return E_FAIL;
	}
}

STDMETHODIMP CFFmpegDSFilter::SetTimeFormat(const GUID *pFormat)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetDuration(LONGLONG *pDuration)
{
	AVFormatContext *fmt = m_pAVFormatContext;
	const double time_base = 10000000.0f / AV_TIME_BASE;

	if (fmt->duration == AV_NOPTS_VALUE) {
		LOGE("fmt->duration == AV_NOPTS_VALUE");
		return E_FAIL;
	}
	*pDuration = (LONGLONG)(fmt->duration / time_base);

	return S_OK;
}

STDMETHODIMP CFFmpegDSFilter::GetStopPosition(LONGLONG *pStop)
{
	return GetDuration(pStop);
}

STDMETHODIMP CFFmpegDSFilter::GetCurrentPosition(LONGLONG *pCurrent)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::ConvertTimeFormat(
	LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source,
	const GUID *pSourceFormat)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::SetPositions(
	LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,
	DWORD dwStopFlags)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetPositions(
	LONGLONG *pCurrent, LONGLONG *pStop)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetAvailable(
	LONGLONG *pEarliest, LONGLONG *pLatest)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::SetRate(double dRate)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetRate(double *pdRate)
{
	LOGW("%s", __FUNCTION__);
	return E_NOTIMPL;
}

STDMETHODIMP CFFmpegDSFilter::GetPreroll(LONGLONG *pllPreroll)
{
	*pllPreroll = 0;
	return S_OK;
}

DWORD CFFmpegDSFilter::ThreadProc(void)
{
	int ret;
	int video_stream = m_videoStreamIndex;
	int audio_stream = m_audioStreamIndex;
	AVFormatContext *fmt = m_pAVFormatContext;
	AVPacket *packet = nullptr;

	while (true) {
		if (CheckRequest(nullptr)) {
			DWORD req = GetRequest();
			if (req == THREAD_REQ_EXIT) {
				break;
			}
			Reply(S_OK);
		}
		packet = av_packet_alloc();
		ret = av_read_frame(fmt, packet);
		if (ret < 0) {
			if (ret != AVERROR_EOF) {
				LOGE("av_read_frame() failed. (%d)", ret);
			}
			break;
		}
		if (packet->stream_index == video_stream) {
			m_pVideoOut->EnqueuePacket(packet);
		} else if (packet->stream_index == audio_stream) {
			m_pAudioOut->EnqueuePacket(packet);
		}
	}

	if (m_pVideoOut != nullptr) {
		m_pVideoOut->EnqueuePacket((AVPacket *)-1);
		m_pVideoOut->WaitThread();
	}
	if (m_pAudioOut != nullptr) {
		m_pAudioOut->EnqueuePacket((AVPacket *)-1);
		m_pAudioOut->WaitThread();
	}

	Reply(S_OK);

	av_packet_free(&packet);

	return 0;
}
