/************************************************************************/
/* 该项目生成的单帧视频保存到文件里边，然后使用PYUV软件打开，逐帧显示   */
/************************************************************************/

#pragma once
#include <streams.h>

extern "C" const GUID CLSID_MyFilter;

class CMyFilter : public CTransformFilter
{
private:
	CMyFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
};