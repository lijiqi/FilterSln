#pragma once
#include <streams.h>

extern "C" const GUID CLSID_HKVCamera;



//MySourceFilter.h
class CMySourceFilter 
	//��SDK���е�CSource������
	:	public CSource			
{
public:
	//ʵ�����ӿ�
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
private:
	//���캯��
	CMySourceFilter(LPUNKNOWN lpunk, HRESULT *phr);	
};
