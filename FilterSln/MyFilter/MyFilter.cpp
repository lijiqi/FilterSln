#include "MyFilter.h"
#include <initguid.h>


#pragma comment(lib, "winmm")

// {80337959-D465-45e7-A8A7-DB388C41C19A}
DEFINE_GUID(CLSID_MyFilter, 
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
	&CLSID_MyFilter,
	L"MyFilter", 
	MERIT_DO_NOT_USE,
	2,
	sudpPins
};

CFactoryTemplate g_Templates[] = {
	{ L"MyFilter"
	, &CLSID_MyFilter
	, CMyFilter::CreateInstance
	, NULL
	, &sudFlipFilter }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
/*
REGFILTER2 rf2FilterReg = {
	1, 
	MERIT_DO_NOT_USE, 
	2, 
	sudpPins 
};*/

CUnknown* CMyFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CMyFilter *pNewObject = new CMyFilter(NAME("MyFilter"), punk, phr);

	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}

CMyFilter::CMyFilter(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) : CTransformFilter(tszName, punk, CLSID_MyFilter)
{

}


HRESULT CMyFilter::CheckInputType(const CMediaType *mtIn)
{
	if (mtIn->majortype == MEDIATYPE_Video &&
		mtIn->subtype == MEDIASUBTYPE_YUY2 &&
		mtIn->formattype == FORMAT_VideoInfo )
	{
		return S_OK;
	}
	
	/*VIDEOINFO* pvi = (VIDEOINFO*)mtIn->Format();

	if (pvi->bmiHeader.biBitCount != 12)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}*/
	
	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMyFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
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

HRESULT CMyFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	if (*mtIn == *mtOut)
	{
		return NOERROR;
	}

	return E_FAIL;
}

HRESULT CMyFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
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

HRESULT CMyFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CheckPointer(pIn,E_POINTER);
	CheckPointer(pOut,E_POINTER);

	BYTE *pSourceBuffer, *pDestBuffer;
	//long lSourceSize = pIn->GetActualDataLength();
	long lSourceSize = pIn->GetSize();

	pIn->GetPointer(&pSourceBuffer);
	pOut->GetPointer(&pDestBuffer);

	CopyMemory(pDestBuffer,pSourceBuffer,lSourceSize);
	
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

	HANDLE hFile = CreateFile(L"D:\\MyMediaFile\\Media.yuv",GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile)
	{
		DWORD dWritten;
		SetFilePointer(hFile,0,NULL,FILE_END);
		WriteFile(hFile,(LPCVOID)pDestBuffer,pOut->GetSize(),&dWritten,nullptr);
	}
	CloseHandle(hFile);

	return NOERROR;
}