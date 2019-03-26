#pragma once
#include "stdafx.h"

/**
 * \brief Template for hooking DirectInput8Create
 */
typedef HRESULT(__stdcall* pDirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

/**
 * \brief Template for hooking CreateDevice
 */
typedef HRESULT(__stdcall* pCreateDevice)(LPDIRECTINPUT8, const GUID&, LPDIRECTINPUTDEVICE8*, LPUNKNOWN);

/**
 * \brief Template for hooking EnumDevices
 */
typedef HRESULT(__stdcall* pEnumDevices)(LPDIRECTINPUT8, DWORD, LPDIENUMDEVICESCALLBACK, LPVOID, DWORD);

/**
 * \brief Template for hooking SetDataFormat
 */
typedef HRESULT(__stdcall* pSetDataFormat)(LPDIRECTINPUTDEVICE8, LPCDIDATAFORMAT);

/**
 * \brief Template for hooking GetDeviceState
 */
typedef HRESULT(__stdcall* pGetDeviceState)(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);


/**
 * \brief Set up hook to DirectInput8Create on startup
 * \return True if the detouring succeeded
 */
BOOL Hook();

/**
 * \brief Undo any previously set hooks
 * \return True if the detouring succeeded
 */
BOOL UnHook();