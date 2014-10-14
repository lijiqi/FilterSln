#include "MyOutputPin.h"

const REFERENCE_TIME TS_ONE = UNITS;
const REFERENCE_TIME TS_HALF = UNITS / 2;

BYTE* LoadBitmapFileToMemory(TCHAR* pFileName, int& nWidth, int& nHeight, int& nImageDataSize)
{
	HBITMAP hBitmap = (HBITMAP)LoadImage( NULL, pFileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );

	if(hBitmap == NULL)
		return NULL;

	HDC hDC = CreateCompatibleDC(NULL);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDC, hBitmap);

	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);

	BITMAPINFOHEADER bih = {0};//位图信息头
	bih.biBitCount = bmp.bmBitsPixel;//每个像素字节大小
	bih.biCompression = BI_RGB;
	bih.biHeight = bmp.bmHeight;//高度
	bih.biPlanes = 1;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;//图像数据大小
	bih.biWidth = bmp.bmWidth;//宽度

	nImageDataSize = bmp.bmWidthBytes * bmp.bmHeight;
	byte * p = new byte[nImageDataSize];//申请内存保存位图数据
	GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, p,
		(LPBITMAPINFO) &bih, DIB_RGB_COLORS);//获取位图数据

	SelectObject(hDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hDC);

	nWidth = bmp.bmWidth;
	nHeight = bmp.bmHeight;

	return p;
}

//MyOutputPin.cpp

//构造函数
CMyOutputPin::CMyOutputPin(HRESULT *phr, CSource *pFilter)
: CSourceStream(L"MySourceFilter",phr,pFilter,L"Out")
, m_nWidth(0)
, m_nHeight(0)
, m_nImageSize(0)
, m_nCount(0)
{
	//把图片读到内存中，准备好数据
	m_pData[0] = LoadBitmapFileToMemory(L"E:\\Image\\0.bmp",
		m_nWidth,m_nHeight,m_nImageSize);
	m_pData[1] = LoadBitmapFileToMemory(L"E:\\Image\\1.bmp",
		m_nWidth,m_nHeight,m_nImageSize);
	m_pData[2] = LoadBitmapFileToMemory(L"E:\\Image\\2.bmp",
		m_nWidth,m_nHeight,m_nImageSize);
}

//析构函数
CMyOutputPin::~CMyOutputPin(void)
{
	//释放内存
	delete []m_pData[0];
	delete []m_pData[1];
	delete []m_pData[2];
}

//获取媒体类型
//填充pmt
//最简单的源Filter，因此只支持一种类型，所以iPosition为0
HRESULT CMyOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt,E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());
	if(iPosition < 0)
	{
		return E_INVALIDARG;
	}
	// Have we run off the end of types?
	if(iPosition > 0)
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	//给媒体类型申请Format的空间
	//填充每一个对象，主要是BITMAPINFOHEADER结构
	VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
	if(NULL == pvi)
		return(E_OUTOFMEMORY);

	ZeroMemory(pvi, sizeof(VIDEOINFO));
	pvi->bmiHeader.biBitCount = 32;
	pvi->bmiHeader.biHeight = m_nHeight;
	pvi->bmiHeader.biWidth = m_nWidth;
	pvi->bmiHeader.biSizeImage = m_nImageSize;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&pvi->rcSource);
	SetRectEmpty(&pvi->rcTarget);

	pmt->SetType(&MEDIATYPE_Video);//设置主媒体类型
	pmt->SetSubtype(&MEDIASUBTYPE_RGB32);//设置子媒体类型
	pmt->SetFormatType(&FORMAT_VideoInfo);//设置详细格式类型
	pmt->SetSampleSize(m_nImageSize);//设置Sample的大小
	pmt->SetTemporalCompression(FALSE);

	return NOERROR;
}

//检查媒体类型
//主要是对GetMediaType中设置的各个参数进行比较
HRESULT CMyOutputPin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType,E_POINTER);

	if (*(pMediaType->Type()) != MEDIATYPE_Video
		|| !(pMediaType->IsFixedSize()))
	{
		return E_INVALIDARG;
	}

	const GUID *SubType = pMediaType->Subtype();
	if (SubType == NULL)
	{
		return E_INVALIDARG;
	}
	if (*SubType != MEDIASUBTYPE_RGB32)
	{
		return E_INVALIDARG;
	}
	const GUID* FormatType = pMediaType->FormatType();
	if (FormatType == NULL)
	{
		return E_INVALIDARG;
	}
	if (*FormatType != FORMAT_VideoInfo)
	{
		return E_INVALIDARG;
	}

	VIDEOINFO* pvi = (VIDEOINFO*)pMediaType->Format();
	if (pvi == NULL)
	{
		return E_INVALIDARG;
	}
	if (pvi->bmiHeader.biBitCount != 32 || 
		pvi->bmiHeader.biWidth != m_nWidth ||
		pvi->bmiHeader.biHeight != m_nHeight)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

//协商Sample的大小
HRESULT CMyOutputPin::DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pIMemAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
	//确定只有一个buffer
	pProperties->cBuffers = 1;
	//设置buffer的大小
	pProperties->cbBuffer = m_nImageSize;

	ASSERT(pProperties->cbBuffer);

	//设置属性页
	ALLOCATOR_PROPERTIES Actual;
	hr = pIMemAlloc->SetProperties(pProperties,&Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}

//填充Sample
HRESULT CMyOutputPin::FillBuffer(IMediaSample *pMediaSample)
{
	CheckPointer(pMediaSample,E_POINTER);
	BYTE* pData = NULL;
	long lDataSize = 0;

	//获得Sample中存放数据的地址
	pMediaSample->GetPointer(&pData);
	//取得Sample分配的内存大小
	lDataSize = pMediaSample->GetSize();

	ZeroMemory(pData,lDataSize);
	//把当前需要显示的数据拷贝到内存中
	CopyMemory(pData,m_pData[m_nCount%3],m_nImageSize);

	//设置时间戳
	REFERENCE_TIME start = TS_ONE * m_nCount;
	REFERENCE_TIME stop = TS_ONE + start;
	pMediaSample->SetTime(&start,&stop);

	//准备下一帧数据
	m_nCount++;

	pMediaSample->SetSyncPoint(TRUE);

	return NOERROR;
}