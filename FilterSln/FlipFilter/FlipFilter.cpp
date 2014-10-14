#include "FlipFilter.h"
#include <initguid.h>


#pragma comment(lib, "winmm")

// {80337959-D465-45e7-A8A7-DB388C41C19A}
DEFINE_GUID(CLSID_FlipFilter, 
			0x80337959, 0xd465, 0x45e7, 0xa8, 0xa7, 0xdb, 0x38, 0x8c, 0x41, 0xc1, 0x9a);


#define WIDTHBYTES(bits) ((DWORD)(((bits)+31) & (~31)) / 8)

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );

} 
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );

}



const AMOVIESETUP_MEDIATYPE sudInputPinTypes =
{
	&MEDIATYPE_Video, 
	&MEDIASUBTYPE_NULL 
};

const AMOVIESETUP_MEDIATYPE sudOutputPinTypes =
{
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_NULL 
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{ L"Input", 
	FALSE, 
	FALSE, 
	FALSE, 
	FALSE, 
	&CLSID_NULL, 
	NULL,
	1,
	&sudInputPinTypes
	},
	{ L"Output", 
	FALSE,  
	TRUE,
	FALSE, 
	FALSE,
	&CLSID_NULL,
	NULL, 
	1, 
	&sudOutputPinTypes 
	}
};

const AMOVIESETUP_FILTER sudFlipFilter =
{
	&CLSID_FlipFilter,
	L"FlipFilter", 
	MERIT_DO_NOT_USE,
	2,
	sudpPins
};

CFactoryTemplate g_Templates[] = {
	{ L"FlipFilter"
	, &CLSID_FlipFilter
	, CFlipFilter::CreateInstance
	, NULL
	, &sudFlipFilter }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

REGFILTER2 rf2FilterReg = {
	1, 
	MERIT_DO_NOT_USE, 
	2, 
	sudpPins 
};





CUnknown* CFlipFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CFlipFilter *pNewObject = new CFlipFilter(NAME("FlipFilter"), punk, phr);

	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}

CFlipFilter::CFlipFilter(TCHAR *tszName,
									   LPUNKNOWN punk,
									   HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_FlipFilter)
{

}


HRESULT CFlipFilter::CheckInputType(const CMediaType *mtIn)
{
	if (mtIn->majortype != MEDIATYPE_Video ||
		mtIn->subtype != MEDIASUBTYPE_RGB24 ||
		mtIn->formattype != FORMAT_VideoInfo )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	VIDEOINFO* pvi = (VIDEOINFO*)mtIn->Format();

	if (pvi->bmiHeader.biBitCount != 24)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CFlipFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	if (iPosition < 0) {
		return E_INVALIDARG;
	}

	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	CheckPointer(pMediaType,E_POINTER);
	*pMediaType = m_pInput->CurrentMediaType();

	return NOERROR;
}

HRESULT CFlipFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	if (*mtIn == *mtOut)
	{
		return NOERROR;
	}

	return E_FAIL;
}

HRESULT CFlipFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	CheckPointer(pAllocator,E_POINTER);
	CheckPointer(pprop,E_POINTER);
	HRESULT hr = NOERROR;

	pprop->cBuffers = 1;
	pprop->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
	ASSERT(pprop->cbBuffer);

	ALLOCATOR_PROPERTIES Actual;
	hr = pAllocator->SetProperties(pprop,&Actual);
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT( Actual.cBuffers == 1 );

	if (pprop->cBuffers > Actual.cBuffers ||
		pprop->cbBuffer > Actual.cbBuffer) {
			return E_FAIL;
	}
	return NOERROR;
}

//HRESULT CFlipFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
//{
//	CheckPointer(pIn,E_POINTER);
//	CheckPointer(pOut,E_POINTER);
//
//	// Copy the sample data
//	BYTE *pSourceBuffer, *pDestBuffer;
//	long lSourceSize = pIn->GetActualDataLength();
//
//#ifdef DEBUG
//	long lDestSize = pOut->GetSize();
//	ASSERT(lDestSize >= lSourceSize);
//#endif
//
//	pIn->GetPointer(&pSourceBuffer);
//	pOut->GetPointer(&pDestBuffer);
//
//	CMediaType pMediaType1 = m_pInput->CurrentMediaType();
//	VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType1.pbFormat;
//	int nWidth = WIDTHBYTES(pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount);
//	for (int i = 0; i < pvi->bmiHeader.biHeight; i ++)
//	{
//		CopyMemory((PVOID) (pDestBuffer + nWidth * i),
//			(PVOID) (pSourceBuffer + nWidth * (pvi->bmiHeader.biHeight - i - 1)),
//			nWidth);
//	}	 
//
//	// Copy the sample times
//
//	REFERENCE_TIME TimeStart, TimeEnd;
//	if(NOERROR == pIn->GetTime(&TimeStart, &TimeEnd))
//	{
//		pOut->SetTime(&TimeStart, &TimeEnd);
//	}
//
//	LONGLONG MediaStart, MediaEnd;
//	if(pIn->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
//	{
//		pOut->SetMediaTime(&MediaStart,&MediaEnd);
//	}
//
//	// Copy the Sync point property
//
//	HRESULT hr = pIn->IsSyncPoint();
//	if(hr == S_OK)
//	{
//		pOut->SetSyncPoint(TRUE);
//	}
//	else if(hr == S_FALSE)
//	{
//		pOut->SetSyncPoint(FALSE);
//	}
//	else
//	{  // an unexpected error has occured...
//		return E_UNEXPECTED;
//	}
//
//	// Copy the media type
//
//	AM_MEDIA_TYPE *pMediaType;
//	pIn->GetMediaType(&pMediaType);
//	pOut->SetMediaType(pMediaType);
//	DeleteMediaType(pMediaType);
//
//	// Copy the preroll property
//
//	hr = pIn->IsPreroll();
//	if(hr == S_OK)
//	{
//		pOut->SetPreroll(TRUE);
//	}
//	else if(hr == S_FALSE)
//	{
//		pOut->SetPreroll(FALSE);
//	}
//	else
//	{  // an unexpected error has occured...
//		return E_UNEXPECTED;
//	}
//
//	// Copy the discontinuity property
//
//	hr = pIn->IsDiscontinuity();
//
//	if(hr == S_OK)
//	{
//		pOut->SetDiscontinuity(TRUE);
//	}
//	else if(hr == S_FALSE)
//	{
//		pOut->SetDiscontinuity(FALSE);
//	}
//	else
//	{  // an unexpected error has occured...
//		return E_UNEXPECTED;
//	}
//
//	// Copy the actual data length
//
//	long lDataLength = pIn->GetActualDataLength();
//	pOut->SetActualDataLength(lDataLength);
//
//	return NOERROR;
//
//}


HRESULT CFlipFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CheckPointer(pIn,E_POINTER);
	CheckPointer(pOut,E_POINTER);

	BYTE *pSourceBuffer, *pDestBuffer;
	long lSourceSize = pIn->GetActualDataLength();

	pIn->GetPointer(&pSourceBuffer);
	pOut->GetPointer(&pDestBuffer);

	//·­×ªÍ¼Ïñ
	CMediaType pMediaType1 = m_pInput->CurrentMediaType();
	VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType1.pbFormat;
	int nWidth = WIDTHBYTES(pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount);
	for (int i = 0; i < pvi->bmiHeader.biHeight; i ++)
	{
		CopyMemory((PVOID) (pDestBuffer + nWidth * i),
			(PVOID) (pSourceBuffer + nWidth * (pvi->bmiHeader.biHeight - i - 1)),
			nWidth);
	}	 

	REFERENCE_TIME TimeStart, TimeEnd;
	if(NOERROR == pIn->GetTime(&TimeStart, &TimeEnd))
	{
		pOut->SetTime(&TimeStart, &TimeEnd);
	}

	LONGLONG MediaStart, MediaEnd;
	if(pIn->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
	{
		pOut->SetMediaTime(&MediaStart,&MediaEnd);
	}

	HRESULT hr = pIn->IsSyncPoint();
	if(hr == S_OK)
	{
		pOut->SetSyncPoint(TRUE);
	}
	else if(hr == S_FALSE)
	{
		pOut->SetSyncPoint(FALSE);
	}
	else
	{
		return E_UNEXPECTED;
	}

	hr = pIn->IsPreroll();
	if(hr == S_OK)
	{
		pOut->SetPreroll(TRUE);
	}
	else if(hr == S_FALSE)
	{
		pOut->SetPreroll(FALSE);
	}
	else
	{ 
		return E_UNEXPECTED;
	}

	hr = pIn->IsDiscontinuity();

	if(hr == S_OK)
	{
		pOut->SetDiscontinuity(TRUE);
	}
	else if(hr == S_FALSE)
	{
		pOut->SetDiscontinuity(FALSE);
	}
	else
	{
		return E_UNEXPECTED;
	}

	long lDataLength = pIn->GetActualDataLength();
	pOut->SetActualDataLength(lDataLength);

	return NOERROR;
}