#pragma once
#include <streams.h>

extern "C" const GUID CLSID_HKVCamera;



//MySourceFilter.h
class CMySourceFilter 
	//从SDK库中的CSource类派生
	:	public CSource			
{
public:
	//实例化接口
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
private:
	//构造函数
	CMySourceFilter(LPUNKNOWN lpunk, HRESULT *phr);	
};
