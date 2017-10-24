// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

void __declspec(dllexport) test()
{
	OutputDebugString("__declspec(dllexport) test()\r\n");
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		test();
		break;
	}
	case DLL_THREAD_ATTACH:
	{
		//MessageBox(NULL, "DLL_THREAD_ATTACH", "TestDllInject", MB_OK);
		//test();
		break;
	}
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

