#include "MySourceFilter.h"
#include "MyOutputPin.h"
#include <initguid.h>

#pragma comment(lib, "winmm")

// {159386E0-5193-48ac-8A57-1788C73340C1}
DEFINE_GUID(CLSID_MySourceFilter, 
			0x159386e0, 0x5193, 0x48ac, 0x8a, 0x57, 0x17, 0x88, 0xc7, 0x33, 0x40, 0xc1);

CMySourceFilter::CMySourceFilter(LPUNKNOWN lpunk, HRESULT *phr)
	: CSource(L"MySourceFilter",lpunk,CLSID_MySourceFilter,phr)
{
	//创建一个pin的对象实例
	//在CSourceStream的构造函数中，会把pin添加到Filter中
	CMyOutputPin* pOutPin = new CMyOutputPin(phr,this);
	if (FAILED(*phr))
	{ 
		//因此，在创建失败的时候，要将这个pin从Filter中移除
		RemovePin(pOutPin);
		pOutPin->Release();
	}
}

CUnknown* CMySourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	//实例化函数的工作就是实例化一个源Filter的对象
	CUnknown *punk = new CMySourceFilter(lpunk,phr);
	if (punk == NULL)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
	L"Output",              // Pin string name
	FALSE,                  // Is it rendered
	TRUE,                   // Is it an output
	FALSE,                  // Can we have none
	FALSE,                  // Can we have many
	&CLSID_NULL,            // Connects to filter
	NULL,                   // Connects to pin
	1,                      // Number of types
	&sudOpPinTypes          // Pin details
};                          

const AMOVIESETUP_FILTER sudBallax =
{
	&CLSID_MySourceFilter,    // Filter CLSID
	L"MySourceFilter",       // String name
	MERIT_DO_NOT_USE,       // Filter merit
	1,                      // Number pins
	&sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
	{ L"MySourceFilter"
	, &CLSID_MySourceFilter
	, CMySourceFilter::CreateInstance
	, NULL
	, &sudBallax }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//////////////////////////////////////////////////////////////////////

// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).

//////////////////////////////////////////////////////////////////////


// DllRegisterServer

// Exported entry points for registration and unregistration

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);

} // DllUnregisterServer


//
// DllEntryPoint
//

//动态库工程自然也要有入口函数（固定格式）
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
	DWORD  dwReason, 
	LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}