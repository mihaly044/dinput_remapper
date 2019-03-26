// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

LPCDIDATAFORMAT g_pDataFormat;

typedef HRESULT(__stdcall* pDirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
pDirectInput8Create pOrigDirectInput8Create;

typedef HRESULT(__stdcall* pCreateDevice)(LPDIRECTINPUT8, const GUID&, LPDIRECTINPUTDEVICE8*, LPUNKNOWN);
pCreateDevice pOriginalCreateDevice;

typedef HRESULT(__stdcall* pEnumDevices)(LPDIRECTINPUT8, DWORD, LPDIENUMDEVICESCALLBACK, LPVOID, DWORD);
pEnumDevices pOriginalEnumDevices;

typedef HRESULT(__stdcall* pSetDataFormat)(LPDIRECTINPUTDEVICE8, LPCDIDATAFORMAT);
pSetDataFormat pOriginalSetDataFormat;

typedef HRESULT(__stdcall* pGetDeviceState)(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);
pGetDeviceState pOriginalGetDeviceState;

HRESULT _stdcall MyGetDeviceState(LPDIRECTINPUTDEVICE8 lpDev, DWORD cbData, LPVOID lpvData)
{
	HRESULT status = pOriginalGetDeviceState(lpDev, cbData, lpvData);

	wchar_t sz[2048];
	if (cbData == sizeof(DIJOYSTATE2))
	{
		LPDIJOYSTATE2 lpData = (LPDIJOYSTATE2)(lpvData);
		lpData->rgbButtons; // 128

		/*
		*	Transpose the following button values:
		*	x -> y	3 -> 0
		*	a -> b	2 -> 1
		*/
		if (lpData->rgbButtons[3] == 0x80)
		{
			lpData->rgbButtons[3] = 0;
			lpData->rgbButtons[0] = 0x80;
		}
		else if (lpData->rgbButtons[0] == 0x80)
		{
			lpData->rgbButtons[0] = 0;
			lpData->rgbButtons[3] = 0x80;
		}

		if (lpData->rgbButtons[2] == 0x80)
		{
			lpData->rgbButtons[2] = 0;
			lpData->rgbButtons[1] = 0x80;
		}
		else if (lpData->rgbButtons[1] == 0x80)
		{
			lpData->rgbButtons[1] = 0;
			lpData->rgbButtons[2] = 0x80;
		}

	}
	else if (cbData == sizeof(DIJOYSTATE))
	{
		LPDIJOYSTATE lpData = (LPDIJOYSTATE)(lpvData);
		lpData->rgbButtons; // 32

		for (int i = 0; i < 32; i++)
		{
			if (lpData->rgbButtons[i] != 0)
			{
				wsprintf(sz, L"(32) %d %02X\r\n", i, lpData->rgbButtons[i]);
				OutputDebugString(sz);
			}
		}
	}

	return status;
}

HRESULT _stdcall MySetDataFormat(LPDIRECTINPUTDEVICE8 lpDev, LPCDIDATAFORMAT lpcdf)
{
	g_pDataFormat = lpcdf;
	return pOriginalSetDataFormat(lpDev, lpcdf);
}

HRESULT _stdcall MyCreateDevice(LPDIRECTINPUT8 i, const GUID& rguid, LPDIRECTINPUTDEVICE8* lpDirectInputDevice, LPUNKNOWN pUnkOuter)
{
	HRESULT status = pOriginalCreateDevice(i, rguid, lpDirectInputDevice, pUnkOuter);
	if (!FAILED(status))
	{
		pOriginalSetDataFormat = (*lpDirectInputDevice)->lpVtbl->SetDataFormat;
		pOriginalGetDeviceState = (*lpDirectInputDevice)->lpVtbl->GetDeviceState;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)pOriginalSetDataFormat, MySetDataFormat);
		DetourAttach(&(PVOID&)pOriginalGetDeviceState, MyGetDeviceState);

		if (DetourTransactionCommit() != NO_ERROR)
			MessageBox(NULL, L"Failed hooking DI:SetDataType or DI:GetDeviceState", L"Error", MB_OK);
	}

	return status;
}

HRESULT __stdcall MyDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
{
	HRESULT status = pOrigDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if (!FAILED(status))
	{
		//We got a LPDIRECTINPUT8 object. Now let's hook EnumDevices
		LPDIRECTINPUT8 pDIW = (LPDIRECTINPUT8)*ppvOut;
		pOriginalCreateDevice = pDIW->lpVtbl->CreateDevice;

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)pOriginalCreateDevice, MyCreateDevice);
		if (DetourTransactionCommit() != NO_ERROR)
			MessageBox(NULL, L"Failed hooking DI:CreateDevice", L"Error", MB_OK);
	}
	else
	{
		MessageBox(NULL, L"Failed acquiring DIRECTINPUT8", L"Error", MB_OK);
	}

	return status;
}



BOOL Hook()
{
	pOrigDirectInput8Create = (pDirectInput8Create)GetProcAddress(GetModuleHandle(L"dinput8.dll"), "DirectInput8Create");
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)pOrigDirectInput8Create, MyDirectInput8Create);
	return DetourTransactionCommit() == NO_ERROR;
}

BOOL UnHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)pOrigDirectInput8Create, MyDirectInput8Create);

	if (pOriginalCreateDevice)
	{
		DetourDetach(&(PVOID&)pOriginalCreateDevice, MyCreateDevice);
		if (pOriginalSetDataFormat)
		{
			DetourDetach(&(PVOID&)pOriginalSetDataFormat, MySetDataFormat);
			DetourAttach(&(PVOID&)pOriginalGetDeviceState, MyGetDeviceState);
		}
	}


	return DetourTransactionCommit() == NO_ERROR;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (DetourIsHelperProcess()) {
		return TRUE;
	}
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		if (!Hook())
		{
			MessageBox(NULL, L"Detouring failed", L"Error", MB_OK);
			return FALSE;
		}
		break;

	case DLL_PROCESS_DETACH:

		if (!UnHook())
		{
			MessageBox(NULL, L"Detouring failed", L"Error", MB_OK);
			return FALSE;
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

