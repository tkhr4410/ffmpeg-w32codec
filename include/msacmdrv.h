// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#pragma once

#define MAKE_ACM_VERSION(major, minor, build) \
( \
	((DWORD)((major) & 0xFF) << 24) | \
	((DWORD)((minor) & 0xFF) << 16) | \
	((DWORD)((build) & 0xFFFF)) \
)

enum {
	ACMDM_DRIVER_NOTIFY					= ACMDM_BASE + 1,
	ACMDM_DRIVER_DETAILS				= ACMDM_BASE + 10,
//	ACMDM_DRIVER_ABOUT					= ACMDM_BASE + 11,
	ACMDM_HARDWARE_WAVE_CAPS_INPUT		= ACMDM_BASE + 20,
	ACMDM_HARDWARE_WAVE_CAPS_OUTPUT		= ACMDM_BASE + 21,
	ACMDM_FORMATTAG_DETAILS				= ACMDM_BASE + 25,
	ACMDM_FORMAT_DETAILS				= ACMDM_BASE + 26,
	ACMDM_FORMAT_SUGGEST				= ACMDM_BASE + 27,
	ACMDM_FILTERTAG_DETAILS				= ACMDM_BASE + 50,
	ACMDM_FILTER_DETAILS				= ACMDM_BASE + 51,
	ACMDM_STREAM_OPEN					= ACMDM_BASE + 76,
	ACMDM_STREAM_CLOSE					= ACMDM_BASE + 77,
	ACMDM_STREAM_SIZE					= ACMDM_BASE + 78,
	ACMDM_STREAM_CONVERT				= ACMDM_BASE + 79,
	ACMDM_STREAM_RESET					= ACMDM_BASE + 80,
	ACMDM_STREAM_PREPARE				= ACMDM_BASE + 81,
	ACMDM_STREAM_UNPREPARE				= ACMDM_BASE + 82,
	ACMDM_STREAM_UPDATE					= ACMDM_BASE + 83,
};

//==============================================================================
// ACMDRVOPENDESCW
//==============================================================================
#pragma pack(1)
typedef struct {
	DWORD		cbStruct;
	FOURCC		fccType;
	FOURCC		fccComp;
	DWORD		dwVersion;
	DWORD		dwFlags;
	DWORD		dwError;
	LPCWSTR		pszSectionName;
	LPCWSTR		pszAliasName;
	DWORD		dnDevNode;
} ACMDRVOPENDESCW, *PACMDRVOPENDESCW, *LPACMDRVOPENDESCW;
#pragma pack()

//==============================================================================
// ACMDRVFORMATSUGGEST
//==============================================================================
#pragma pack(1)
typedef struct {
	DWORD			cbStruct;
	DWORD			fdwSuggest;
	LPWAVEFORMATEX	pwfxSrc;
	DWORD			cbwfxSrc;
	LPWAVEFORMATEX	pwfxDst;
	DWORD			cbwfxDst;
} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST, *LPACMDRVFORMATSUGGEST;
#pragma pack()

//==============================================================================
// ACMDRVSTREAMINSTANCE
//==============================================================================
#pragma pack(1)
typedef struct {
	DWORD			cbStruct;
	LPWAVEFORMATEX	pwfxSrc;
	LPWAVEFORMATEX	pwfxDst;
	LPWAVEFILTER	pwfltr;
	DWORD_PTR		dwCallback;
	DWORD_PTR		dwInstance;
	DWORD			fdwOpen;
	DWORD			fdwDriver;
	DWORD_PTR		dwDriver;
	HACMSTREAM		has;
} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE, *LPACMDRVSTREAMINSTANCE;
#pragma pack()

//==============================================================================
// ACMDRVSTREAMSIZE
//==============================================================================
#pragma pack(1)
typedef struct {
	DWORD	cbStruct;
	DWORD	fdwSize;
	DWORD	cbSrcLength;
	DWORD	cbDstLength;
} ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE, *LPACMDRVSTREAMSIZE;
#pragma pack()

//==============================================================================
// ACMDRVSTREAMHEADER
//==============================================================================
#pragma pack(1)
typedef struct _ACMDRVSTREAMHEADER {
	DWORD					cbStruct;
	DWORD					fdwStatus;
	DWORD_PTR				dwUser;
	LPBYTE					pbSrc;
	DWORD					cbSrcLength;
	DWORD					cbSrcLengthUsed;
	DWORD_PTR				dwSrcUser;
	LPBYTE					pbDst;
	DWORD					cbDstLength;
	DWORD					cbDstLengthUsed;
	DWORD_PTR				dwDstUser;
	DWORD					fdwConvert;
	_ACMDRVSTREAMHEADER *	padshNext;
	DWORD					fdwDriver;
	DWORD_PTR				dwDriver;
	DWORD					fdwPrepared;
	DWORD_PTR				dwPrepared;
	LPBYTE					pbPreparedSrc;
	DWORD					cbPreparedSrcLength;
	LPBYTE					pbPreparedDst;
	DWORD					cbPreparedDstLength;
} ACMDRVSTREAMHEADER, *PACMDRVSTREAMHEADER, *LPACMDRVSTREAMHEADER;
#pragma pack()
