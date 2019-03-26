// injector.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <detours.h>

/**
 * The entry point of the CLI application.
 * Uses the DetourCreateProcessWithDllEx function that first
 * starts up the target proceess and modifies its import tables
 */
int wmain(int32_t argc, wchar_t** argv)
{
	if (argc < 3)
	{
		wprintf(L"Usage: injector [dll path] [process_name.exe]\n");
		return 0;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	size_t i;
	char dll[MAX_PATH];
	wcstombs_s(&i, dll, argv[1], MAX_PATH);

	if (!DetourCreateProcessWithDllEx(argv[2], argv[3],
		nullptr, nullptr, TRUE,
		CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED,
		nullptr, nullptr, &si, &pi, dll, nullptr)) {
		wprintf(L"\r\nERR %02X\r\n", GetLastError());
	}

	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return 0;
}