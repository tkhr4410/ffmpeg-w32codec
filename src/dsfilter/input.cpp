// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#include "dsfilter.h"

CFFmpegStreamIn::CFFmpegStreamIn(
	LPCTSTR pObjectName, CFFmpegDSFilter *pFilter, CCritSec *pLock,
	HRESULT *phr)
	: CBaseInputPin(pObjectName, pFilter, pLock, phr, L"FFmpeg Stream In"),
	m_offset(0), m_pAsyncReader(nullptr)
{
	*phr = S_OK;
}

CFFmpegStreamIn::~CFFmpegStreamIn()
{
	if (m_pAsyncReader != nullptr) {
		m_pAsyncReader->Release();
	}
}

STDMETHODIMP CFFmpegStreamIn::NonDelegatingQueryInterface(
	REFIID riid, void **ppv)
{
	if (riid == IID_IMemInputPin) {
		return GetInterface(static_cast<IMemInputPin *>(this), ppv);
	}
	return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFFmpegStreamIn::CheckMediaType(const CMediaType *pMediaType)
{
	OLECHAR *type_str;

	if (pMediaType->majortype != MEDIATYPE_Stream) {
		StringFromCLSID(pMediaType->majortype, &type_str);
		LOGD("%s: majortype=%ls", __FUNCTION__, type_str);
		CoTaskMemFree(type_str);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CFFmpegStreamIn::CheckConnect(IPin *pPin)
{
	HRESULT hr;
	IAsyncReader *pReader;
	PIN_INFO info;
	CLSID clsid;
	OLECHAR *clsid_str;

	hr = CBaseInputPin::CheckConnect(pPin);
	if (FAILED(hr)) {
		LOGE("CBaseInputPin::CheckConnect() failed. (0x%08x)", hr);
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
	if (clsid != CLSID_AsyncReader) {
		StringFromCLSID(clsid, &clsid_str);
		LOGD("%s: clsid=%ls -> E_FAIL", __FUNCTION__, clsid_str);
		CoTaskMemFree(clsid_str);
		return E_FAIL;
	}

	hr = pPin->QueryInterface(&pReader);
	if (FAILED(hr)) {
		LOGE("IPin::QueryInterface(IAsyncReader) failed. (0x%08x)", hr);
		return hr;
	}
	pReader->Release();

	return S_OK;
}

HRESULT CFFmpegStreamIn::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr;

	hr = CBaseInputPin::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		LOGE("CBaseInputPin::CompleteConnect() failed. (0x%08x)", hr);
		return hr;
	}

	hr = pReceivePin->QueryInterface(&m_pAsyncReader);
	if (FAILED(hr)) {
		LOGE("IPin::QueryInterface(IAsyncReader) failed. (0x%08x)", hr);
		return hr;
	}

	return static_cast<CFFmpegDSFilter *>(m_pFilter)->Init();
}

int CFFmpegStreamIn::Read(void *opaque, uint8_t *buf, int buf_size)
{
	CFFmpegStreamIn *pin = static_cast<CFFmpegStreamIn *>(opaque);
	CAutoLock lock(pin);
	HRESULT hr;
	LONGLONG total;
	LONGLONG avail;

	hr = pin->m_pAsyncReader->SyncRead(pin->m_offset, buf_size, buf);
	if (FAILED(hr)) {
		LOGE("IAsyncReader::SyncRead() failed. (0x%08x)", hr);
		return AVERROR_EOF;
	}

	if (hr == S_FALSE) {
		hr = pin->m_pAsyncReader->Length(&total, &avail);
		if (FAILED(hr)) {
			LOGE("IAsyncReader::Length() failed.");
			return AVERROR_EOF;
		}
		buf_size = total - pin->m_offset;
		if (buf_size == 0) {
			return AVERROR_EOF;
		}
	}

	pin->m_offset += buf_size;

	return buf_size;
}

int64_t CFFmpegStreamIn::Seek(void *opaque, int64_t offset, int whence)
{
	CFFmpegStreamIn *pin = static_cast<CFFmpegStreamIn *>(opaque);
	CAutoLock lock(pin);
	HRESULT hr;
	LONGLONG total;
	LONGLONG avail;

	hr = pin->m_pAsyncReader->Length(&total, &avail);
	if (FAILED(hr)) {
		LOGE("IAsyncReader::Length() failed.");
		return -1;
	}

	switch (whence)
	{
	case SEEK_SET:
		pin->m_offset = offset;
		break;

	case SEEK_CUR:
		pin->m_offset += offset;
		break;

	case SEEK_END:
		pin->m_offset = total - offset;
		break;

	case AVSEEK_SIZE:
		return total;

	default:
		LOGE("invalid whence %d", whence);
		return -1;
	}
	
	return pin->m_offset;
}
