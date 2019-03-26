// payload.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "payload.h"

/**
 * \brief Pointer to device data format object
 * Could be useful when calling GetDeviceState
 */
LPCDIDATAFORMAT g_pDataFormat;

/**
 * \brief Function pointer to the original DirectInput8Create
 */
pDirectInput8Create g_pOrigDirectInput8Create;

/**
 * \brief Function pointer to the original CreateDevice
 */
pCreateDevice g_pOriginalCreateDevice;

/**
 * \brief Function pointer to the original EnumDevices
 */
pEnumDevices g_pOriginalEnumDevices;

/**
 * \brief Function pointer to the original SetDataFormat
 */
pSetDataFormat g_pOriginalSetDataFormat;

/**
 * \brief Function pointer to the original DeviceState
 */
pGetDeviceState g_pOriginalGetDeviceState;

/**
 * \brief Function to call on GetDeviceState
 * \param lpDev Pointer to a IDirectInputDevice8 that the data is coming from
 * \param cbData Size of the meaningful data located at lpvData
 * \param lpvData Pointer to where to read the incoming data from
 * \return The result of the original GetDeviceState
 */
HRESULT _stdcall MyGetDeviceState(LPDIRECTINPUTDEVICE8 lpDev, DWORD cbData, LPVOID lpvData)
{
	const auto status = g_pOriginalGetDeviceState(lpDev, cbData, lpvData);

	if (status == DI_OK)
	{
		if (cbData == sizeof(DIJOYSTATE2))
		{
			auto lpData = static_cast<LPDIJOYSTATE2>(lpvData);

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
			wchar_t sz[2048];
			auto lpData = static_cast<LPDIJOYSTATE>(lpvData);

			for (auto i = 0; i < 32; i++)
			{
				if (lpData->rgbButtons[i] != 0)
				{
					wsprintf(sz, L"(32) %d %02X\r\n", i, lpData->rgbButtons[i]);
					OutputDebugString(sz);
				}
			}
		}
	}

	return status;
}


/**
 * \brief Function to call on SetDataFormat
 * \param lpDev The IDirectInputDevice8 to use with the original SetDataFormat
 * \param lpcdf Pointer to a DIDATAFORMAT structure that describes the data format that will be set
 * \return The result of the original SetDataFormat
 */
HRESULT _stdcall MySetDataFormat(LPDIRECTINPUTDEVICE8 lpDev, LPCDIDATAFORMAT lpcdf)
{
	g_pDataFormat = lpcdf;
	return g_pOriginalSetDataFormat(lpDev, lpcdf);
}


/**
 * \brief The function to call on CreateDevice. This is where we set further function hooks
 * that will later ensure us being able to catch calls to GetDeviceState
 * \param i Interface to DirectInput
 * \param rguid GUID of the COM object
 * \param lpDirectInputDevice IDirectInputDevice8 pointer to where IDirectInputDevice8 will reside after being created
 * \param pUnkOuter Pointer to IUnknown outer data
 * \return
 */
HRESULT _stdcall MyCreateDevice(LPDIRECTINPUT8 i, const GUID& rguid, LPDIRECTINPUTDEVICE8* lpDirectInputDevice,
	LPUNKNOWN pUnkOuter)
{
	const auto status = g_pOriginalCreateDevice(i, rguid, lpDirectInputDevice, pUnkOuter);
	if (!FAILED(status))
	{
		g_pOriginalSetDataFormat = (*lpDirectInputDevice)->lpVtbl->SetDataFormat;
		g_pOriginalGetDeviceState = (*lpDirectInputDevice)->lpVtbl->GetDeviceState;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&reinterpret_cast<PVOID&>(g_pOriginalSetDataFormat), MySetDataFormat);
		DetourAttach(&reinterpret_cast<PVOID&>(g_pOriginalGetDeviceState), MyGetDeviceState);

		if (DetourTransactionCommit() != NO_ERROR)
			MessageBox(nullptr, L"Failed hooking DI:SetDataType or DI:GetDeviceState", L"Error", MB_OK);
	}

	return status;
}

/**
 * \brief The function to call on DirectInput8Create. Sets up hook to CreateDevice
 * \param hInst Instance pointer
 * \param dwVersion DirectInput version
 * \param riidltf Determines if we are using ANSI or UNICODE
 * \param ppvOut Pointer to an IDirectInput8 interface
 * \param punkOuter
 * \return
 */
HRESULT __stdcall MyDirectInput8Create(HINSTANCE hInst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut,
	LPUNKNOWN punkOuter)
{
	const auto status = g_pOrigDirectInput8Create(hInst, dwVersion, riidltf, ppvOut, punkOuter);

	if (status == DI_OK)
	{
		// Don't check for riidltf. We know for sure this is an Unicode project
		const auto pDIW = static_cast<LPDIRECTINPUT8>(*ppvOut);
		g_pOriginalCreateDevice = pDIW->lpVtbl->CreateDevice;

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&reinterpret_cast<PVOID&>(g_pOriginalCreateDevice), MyCreateDevice);
		if (DetourTransactionCommit() != NO_ERROR)
			MessageBox(nullptr, L"Failed hooking DI:CreateDevice", L"Error", MB_OK);
	}
	else
	{
		MessageBox(nullptr, L"Failed acquiring DIRECTINPUT8", L"Error", MB_OK);
	}

	return status;
}

BOOL Hook()
{
	g_pOrigDirectInput8Create = reinterpret_cast<pDirectInput8Create>(GetProcAddress(GetModuleHandle(L"dinput8.dll"), "DirectInput8Create"));
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&reinterpret_cast<PVOID&>(g_pOrigDirectInput8Create), MyDirectInput8Create);
	return DetourTransactionCommit() == NO_ERROR;
}

BOOL UnHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&reinterpret_cast<PVOID&>(g_pOrigDirectInput8Create), MyDirectInput8Create);

	if (g_pOriginalCreateDevice)
	{
		DetourDetach(&reinterpret_cast<PVOID&>(g_pOriginalCreateDevice), MyCreateDevice);
		if (g_pOriginalSetDataFormat)
		{
			DetourDetach(&reinterpret_cast<PVOID&>(g_pOriginalSetDataFormat), MySetDataFormat);
			DetourAttach(&reinterpret_cast<PVOID&>(g_pOriginalGetDeviceState), MyGetDeviceState);
		}
	}


	return DetourTransactionCommit() == NO_ERROR;
}