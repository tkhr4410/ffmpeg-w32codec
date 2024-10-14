// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "dsfilter.h"

CFFmpegStreamOut::CFFmpegStreamOut(
	LPCTSTR pObjectName, CFFmpegDSFilter *pFilter, CCritSec *pLock,
	HRESULT *phr, LPCWSTR pName,
	const AVStream *pAVStream, const AVCodec *pAVCodec)
	: CBaseOutputPin(pObjectName, pFilter, pLock, phr, pName)
{
	VIDEOINFOHEADER *vih;
	WAVEFORMATEX *wfx;

	m_hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);

	switch (pAVCodec->type)
	{
	case AVMEDIA_TYPE_VIDEO:
		m_mt.majortype = MEDIATYPE_Video;
		m_mt.subtype = MEDIASUBTYPE_NULL;
		m_mt.bFixedSizeSamples = TRUE;
		m_mt.bTemporalCompression = FALSE;
		vih = (VIDEOINFOHEADER *)CoTaskMemAlloc(sizeof(*vih));
		ZeroMemory(vih, sizeof(*vih));
		vih->rcSource.right = pAVStream->codecpar->width;
		vih->rcSource.bottom = pAVStream->codecpar->height;
		vih->rcTarget = vih->rcSource;
		vih->dwBitRate = pAVStream->codecpar->bit_rate;
		vih->AvgTimePerFrame = (REFERENCE_TIME)
			(10000000 / av_q2d(pAVStream->codecpar->framerate));
		vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
		vih->bmiHeader.biWidth = pAVStream->codecpar->width;
		vih->bmiHeader.biHeight = pAVStream->codecpar->height;
		m_mt.lSampleSize =
			vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * 4;
		m_mt.formattype = FORMAT_VideoInfo;
		m_mt.cbFormat = sizeof(*vih);
		m_mt.pbFormat = (BYTE *)vih;
		break;

	case AVMEDIA_TYPE_AUDIO:
		m_mt.majortype = MEDIATYPE_Audio;
		m_mt.subtype = MEDIASUBTYPE_PCM;
		m_mt.bFixedSizeSamples = TRUE;
		m_mt.bTemporalCompression = FALSE;
		wfx = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(*wfx));
		ZeroMemory(wfx, sizeof(*wfx));
		wfx->wFormatTag = get_wave_format_tag(pAVCodec->id);
		wfx->nChannels = pAVStream->codecpar->ch_layout.nb_channels;
		wfx->nSamplesPerSec = pAVStream->codecpar->sample_rate;
		wfx->nAvgBytesPerSec = pAVStream->codecpar->bit_rate / 8;
		wfx->nBlockAlign = pAVStream->codecpar->block_align;
		wfx->wBitsPerSample = pAVStream->codecpar->bits_per_coded_sample;
		m_mt.lSampleSize = wfx->wBitsPerSample / 8;
		m_mt.formattype = FORMAT_WaveFormatEx;
		m_mt.cbFormat = sizeof(*wfx);
		m_mt.pbFormat = (BYTE *)wfx;
		break;

	default:
		*phr = E_FAIL;
		return;
	}

	m_timeBase = 10000000 * av_q2d(pAVStream->time_base);
	m_preroll = (LONGLONG)(pAVStream->start_time * m_timeBase);

	*phr = S_OK;
}

CFFmpegStreamOut::~CFFmpegStreamOut()
{
}

STDMETHODIMP CFFmpegStreamOut::NonDelegatingQueryInterface(
	REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking) {
		return GetInterface(static_cast<IMediaSeeking *>(this), ppv);
	}
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFFmpegStreamOut::CheckConnect(IPin *pPin)
{
	HRESULT hr;
	PIN_INFO info;
	CLSID clsid;
	OLECHAR *clsid_str;

	hr = CBaseOutputPin::CheckConnect(pPin);
	if (FAILED(hr)) {
		LOGE("CBaseOutputPin::CheckConnect() failed. (0x%08x)", hr);
		return hr;
	}

	hr = pPin->QueryPinInfo(&info);
	if (FAILED(hr)) {
		LOGE("IPin::QueryPinInfo() failed. (0x%08x)", hr);
		return hr;
	}

	hr = info.pFilter->GetClassID(&clsid);
	if (FAILED(hr)) {
		LOGE("IBaseFilter::GetClassID() failed. (0x%08x)", hr);
		return hr;
	}
	if ((m_mt.majortype != MEDIATYPE_Video) &&
		(m_mt.majortype != MEDIATYPE_Audio)) {
		StringFromCLSID(clsid, &clsid_str);
		LOGD("%s: clsid=%ls", __FUNCTION__, clsid_str);
		CoTaskMemFree(clsid_str);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CFFmpegStreamOut::CheckMediaType(const CMediaType *pMediaType)
{
	OLECHAR *type_str;

	if (pMediaType->majortype != m_mt.majortype) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (pMediaType->majortype == MEDIATYPE_Video) {
		if (pMediaType->formattype == FORMAT_VideoInfo) {
			return S_OK;
		} else {
			return E_FAIL;
		}
	} else if (pMediaType->majortype == MEDIATYPE_Audio) {
		if (pMediaType->formattype == FORMAT_WaveFormatEx) {
			return S_OK;
		} else {
			return E_FAIL;
		}
	} else {
		StringFromCLSID(pMediaType->majortype, &type_str);
		LOGD("%s: majortype=%ls", __FUNCTION__, type_str);
		CoTaskMemFree(type_str);
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
}

HRESULT CFFmpegStreamOut::DecideBufferSize(
	IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
	HRESULT hr;
	ALLOCATOR_PROPERTIES actual;

	// TODO
	if (m_mt.majortype == MEDIATYPE_Audio) {
		pProperties->cBuffers = 2;
		pProperties->cbBuffer = (48000 * 2 * 2) / 20;
	} else {
		pProperties->cBuffers = 2;
		pProperties->cbBuffer = m_mt.lSampleSize;
	}
	pProperties->cbAlign  = 1;
	pProperties->cbPrefix = 0;

	hr = pAllocator->SetProperties(pProperties, &actual);
	if (FAILED(hr)) {
		LOGE("IMemAllocator::SetProperties() failed. (0x%08x)", hr);
		return hr;
	}

	return S_OK;
}

HRESULT CFFmpegStreamOut::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	OLECHAR *type_str;

	if (iPosition < 0) {
		return E_UNEXPECTED;
	}

	if (m_mt.majortype == MEDIATYPE_Video) {
		if (m_mt.formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER *vih =
				(VIDEOINFOHEADER *)CoTaskMemAlloc(sizeof(*vih));
			CopyMemory(vih, m_mt.pbFormat, sizeof(*vih));
			if (!get_format_video_out(iPosition, vih)) {
				CoTaskMemFree(vih);
				return VFW_S_NO_MORE_ITEMS;
			}
			pMediaType->majortype = MEDIATYPE_Video;
			pMediaType->subtype = get_subtype_video_out(iPosition);
			pMediaType->bFixedSizeSamples = TRUE;
			pMediaType->bTemporalCompression = FALSE;
			pMediaType->lSampleSize = vih->bmiHeader.biSizeImage;
			pMediaType->formattype = FORMAT_VideoInfo;
			pMediaType->pUnk = nullptr;
			pMediaType->cbFormat = sizeof(*vih);
			pMediaType->pbFormat = (BYTE *)vih;
		} else {
			StringFromCLSID(m_mt.formattype, &type_str);
			LOGD("%s: formattype=%ls", __FUNCTION__, type_str);
			CoTaskMemFree(type_str);
			return E_NOTIMPL;
		}
	} else if (m_mt.majortype == MEDIATYPE_Audio) {
		if (m_mt.formattype == FORMAT_WaveFormatEx) {
			WAVEFORMATEX *wfx =
				(WAVEFORMATEX *)CoTaskMemAlloc(sizeof(*wfx));
			CopyMemory(wfx, m_mt.pbFormat, sizeof(*wfx));
			if (!get_format_audio_out(iPosition, wfx)) {
				CoTaskMemFree(wfx);
				return VFW_S_NO_MORE_ITEMS;
			}
			pMediaType->majortype = MEDIATYPE_Audio;
			pMediaType->subtype = get_subtype_audio_out(iPosition);
			pMediaType->bFixedSizeSamples = TRUE;
			pMediaType->bTemporalCompression = FALSE;
			pMediaType->lSampleSize = wfx->nBlockAlign;
			pMediaType->formattype = FORMAT_WaveFormatEx;
			pMediaType->pUnk = nullptr;
			pMediaType->cbFormat = sizeof(*wfx);
			pMediaType->pbFormat = (BYTE *)wfx;
		} else {
			StringFromCLSID(m_mt.formattype, &type_str);
			LOGD("%s: formattype=%ls", __FUNCTION__, type_str);
			CoTaskMemFree(type_str);
			return E_NOTIMPL;
		}
	} else {
		StringFromCLSID(m_mt.majortype, &type_str);
		LOGD("%s: majortype=%ls", __FUNCTION__, type_str);
		CoTaskMemFree(type_str);
		return E_NOTIMPL;
	}

	return S_OK;
}

HRESULT CFFmpegStreamOut::Active(void)
{
	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		return hr;
	}
	Create();

	return S_OK;
}

HRESULT CFFmpegStreamOut::Inactive(void)
{
	if (ThreadExists()) {
		CallWorker(THREAD_REQ_EXIT);
		Close();
	}
	return CBaseOutputPin::Inactive();
}

STDMETHODIMP CFFmpegStreamOut::GetCapabilities(DWORD *pCapabilities)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetCapabilities(
		pCapabilities);
}

STDMETHODIMP CFFmpegStreamOut::CheckCapabilities(DWORD *pCapabilities)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->CheckCapabilities(
		pCapabilities);
}

STDMETHODIMP CFFmpegStreamOut::IsFormatSupported(const GUID *pFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->IsFormatSupported(
		pFormat);
}

STDMETHODIMP CFFmpegStreamOut::QueryPreferredFormat(GUID *pFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->QueryPreferredFormat(
		pFormat);
}

STDMETHODIMP CFFmpegStreamOut::GetTimeFormat(GUID *pFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetTimeFormat(
		pFormat);
}

STDMETHODIMP CFFmpegStreamOut::IsUsingTimeFormat(const GUID *pFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->IsUsingTimeFormat(
		pFormat);
}

STDMETHODIMP CFFmpegStreamOut::SetTimeFormat(const GUID *pFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->SetTimeFormat(
		pFormat);
}

STDMETHODIMP CFFmpegStreamOut::GetDuration(LONGLONG *pDuration)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetDuration(
		pDuration);
}

STDMETHODIMP CFFmpegStreamOut::GetStopPosition(LONGLONG *pStop)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetStopPosition(
		pStop);
}

STDMETHODIMP CFFmpegStreamOut::GetCurrentPosition(LONGLONG *pCurrent)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetCurrentPosition(
		pCurrent);
}

STDMETHODIMP CFFmpegStreamOut::ConvertTimeFormat(
	LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source,
	const GUID *pSourceFormat)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->ConvertTimeFormat(
		pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CFFmpegStreamOut::SetPositions(
	LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,
	DWORD dwStopFlags)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->SetPositions(
		pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CFFmpegStreamOut::GetPositions(
	LONGLONG *pCurrent, LONGLONG *pStop)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetPositions(
		pCurrent, pStop);
}

STDMETHODIMP CFFmpegStreamOut::GetAvailable(
	LONGLONG *pEarliest, LONGLONG *pLatest)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetAvailable(
		pEarliest, pLatest);
}

STDMETHODIMP CFFmpegStreamOut::SetRate(double dRate)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->SetRate(
		dRate);
}

STDMETHODIMP CFFmpegStreamOut::GetRate(double *pdRate)
{
	return static_cast<CFFmpegDSFilter *>(m_pFilter)->GetRate(
		pdRate);
}

STDMETHODIMP CFFmpegStreamOut::GetPreroll(LONGLONG *pllPreroll)
{
	*pllPreroll = m_preroll;
	return S_OK;
}

DWORD CFFmpegStreamOut::ThreadProc(void)
{
	int ret;
	HRESULT hr = S_OK;
	CFFmpegDSFilter *filter = static_cast<CFFmpegDSFilter *>(m_pFilter);
	AVPacket *packet = nullptr;
	AVFrame *frame_org = av_frame_alloc();
	AVFrame *frame_cvt = av_frame_alloc();
	IMediaSample *pSample = nullptr;
	size_t offset;
	size_t size;

	while (true) {
		if (CheckRequest(nullptr)) {
			DWORD req = GetRequest();
			if (req == THREAD_REQ_EXIT) {
				break;
			}
			Reply(S_OK);
		}

		packet = DequeuePacket();
		if (packet == nullptr) {
			continue;
		} else if (packet == (AVPacket *)-1) {
			break;
		}

		ret = filter->Decode(frame_org, packet);
		av_packet_free(&packet);
		if (ret != 0) {
			continue;
		}

		size = Convert(frame_cvt, frame_org);
		if (size == (size_t)-1) {
			LOGE("failed to convert");
			break;
		}
		av_frame_unref(frame_org);

		offset = 0;
		while (offset < size) {
			if (pSample == nullptr) {
				hr = GetDeliveryBuffer(&pSample, nullptr, nullptr, 0);
				if (FAILED(hr)) {
					LOGE("GetDeliveryBuffer() failed. (0x%08x)", hr);
					break;
				}
				pSample->SetActualDataLength(0);
			}
			hr = Prepare(pSample, frame_cvt, size, &offset);
			if (hr == S_OK) {
				hr = Deliver(pSample);
				if (FAILED(hr)) {
					LOGE("Deliver() failed. (0x%08x)", hr);
					break;
				}
				pSample->Release();
				pSample = nullptr;
			} else {
				if (FAILED(hr)) {
					LOGE("Prepare() failed. (0x%08x)", hr);
				}
				break;
			}
		}
		av_frame_unref(frame_cvt);

		if (FAILED(hr)) {
			break;
		}
	}

	while (true) {
		packet = DequeuePacket();
		if ((packet == nullptr) || (packet == (AVPacket *)-1)) {
			break;
		}
		av_packet_free(&packet);
	}

	av_frame_free(&frame_org);
	av_frame_free(&frame_cvt);

	if (pSample != nullptr) {
		Deliver(pSample);
		pSample->Release();
		pSample = nullptr;
	}

	hr = DeliverEndOfStream();
	if (FAILED(hr)) {
		LOGE("DeliverEndOfStream() failed. (0x%08x)", hr);
	}
	Reply(hr);

	return 0;
}

size_t CFFmpegStreamOut::Convert(AVFrame *dst, const AVFrame *src)
{
	LONG ret = -1;
	CFFmpegDSFilter *filter = static_cast<CFFmpegDSFilter *>(m_pFilter);

	if (m_mt.majortype == MEDIATYPE_Video) {
		if (m_mt.formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)m_mt.pbFormat;
			ret = filter->ConvertVideoFrame(dst, src, vih, m_mt.subtype);
		}
	} else if (m_mt.majortype == MEDIATYPE_Audio) {
		if (m_mt.formattype == FORMAT_WaveFormatEx) {
			WAVEFORMATEX *wfx = (WAVEFORMATEX *)m_mt.pbFormat;
			ret = filter->ConvertAudioFrame(dst, src, wfx);
		}
	}

	if (ret == -1) {
		return ret;
	}

	dst->flags = src->flags;
	dst->best_effort_timestamp = src->best_effort_timestamp;
	dst->duration = src->duration;

	return ret;
}

HRESULT CFFmpegStreamOut::Prepare(
	IMediaSample *pSample, const AVFrame *frame, size_t size, size_t *offset)
{
	HRESULT hr;
	const BYTE *src;
	BYTE *dst;
	LONG src_offset;
	LONG dst_offset;
	LONG src_len;
	LONG dst_len;
	BOOL sync_point = (frame->flags & AV_FRAME_FLAG_KEY)? TRUE:FALSE;
	BOOL discontinuity = (frame->flags & AV_FRAME_FLAG_CORRUPT)? TRUE:FALSE;
	REFERENCE_TIME rt_start;
	REFERENCE_TIME rt_end;
	REFERENCE_TIME rt_duration;
	int64_t pts = frame->best_effort_timestamp;
	int64_t duration;
	const double time_base = m_timeBase;
	bool delivery = false;

	hr = pSample->GetPointer(&dst);
	if (FAILED(hr)) {
		LOGE("IMediaSample::GetPointer() failed. (0x%08x)", hr);
		return hr;
	}

	src_offset = *offset;
	dst_offset = pSample->GetActualDataLength();
	src_len = size - src_offset;
	dst_len = pSample->GetSize() - dst_offset;

	if (dst_offset == 0) {
		pts += frame->duration * (src_offset / (double)size);
	}

	if (m_mt.majortype == MEDIATYPE_Video) {
		int h = frame->height;
		int pitch = frame->linesize[0];
		if ((src_offset != 0) || (dst_offset != 0)) {
			return E_FAIL;
		}
		if (src_len > dst_len) {
			return E_FAIL;
		}
		src = (BYTE *)frame->data[0] + (h - 1) * pitch;
		for (int y=0; y<h; y++) {
			CopyMemory(dst, src, pitch);
			src -= pitch;
			dst += pitch;
		}
		*offset = size;
		duration = frame->duration;
		delivery = true;
	} else if (m_mt.majortype == MEDIATYPE_Audio) {
		src = frame->data[0] + src_offset;
		if (src_len > dst_len) {
			src_len = dst_len;
		}
		CopyMemory(dst + dst_offset, src, src_len);
		*offset += src_len;
		duration = frame->duration * (src_len / (double)size);
		delivery = (dst_offset + src_len == pSample->GetSize());
	} else {
		return E_FAIL;
	}

	hr = pSample->SetActualDataLength(dst_offset + src_len);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetActualDataLength() failed. (0x%08x)", hr);
		return hr;
	}
	hr = pSample->SetSyncPoint(sync_point);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetSyncPoint() failed. (0x%08x)", hr);
		return hr;
	}
	hr = pSample->SetDiscontinuity(discontinuity);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetDiscontinuity() failed. (0x%08x)", hr);
		return hr;
	}

	if (pts == AV_NOPTS_VALUE) {
		return S_OK;
	}
	rt_start = (REFERENCE_TIME)(pts * time_base);
	rt_duration = (REFERENCE_TIME)(duration * time_base);
	rt_end = rt_start + rt_duration;

	hr = pSample->SetTime(&rt_start, &rt_end);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetTime() failed. (0x%08x)", hr);
		return hr;
	}
	hr = pSample->SetMediaTime(nullptr, nullptr);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetMediaTime() failed. (0x%08x)", hr);
		return hr;
	}
	hr = pSample->SetPreroll((rt_start < 0)? TRUE:FALSE);
	if (FAILED(hr)) {
		LOGE("IMediaSample::SetPreroll() failed. (0x%08x)", hr);
		return hr;
	}

//	LOGD("%c(%d): %d - %d (%d / %d / %d)",
//		(m_mt.majortype == MEDIATYPE_Video)? 'V':'A', sync_point,
//		(int)(rt_start / 100), (int)(rt_end / 100), (int)(rt_duration / 100),
//		send_len, dst_len);

	return delivery? S_OK:S_FALSE;
}
