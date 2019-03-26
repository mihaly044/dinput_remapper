// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "payload.h"

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	if (DetourIsHelperProcess())
	{
		return TRUE;
	}
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		if (!Hook())
		{
			MessageBox(nullptr, L"Detouring failed", L"Error", MB_OK);
			return FALSE;
		}
		break;

	case DLL_PROCESS_DETACH:

		if (!UnHook())
		{
			MessageBox(nullptr, L"Detouring failed", L"Error", MB_OK);
			return FALSE;
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

