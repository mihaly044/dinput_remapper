#pragma once

#include "pch.h"
#include <detours.h>

typedef struct {
    PCCH FunctionName;
    PVOID* FunctionAddress;
} FunctionCharacteristics;

void ThrowWin32Exception(const char *funcname);
std::string ws2s(const std::wstring& wstr);
HANDLE StartProcess(LPCSTR injectdll, LPCWSTR exe, const std::wstring& args);
HANDLE AttachToProcess(DWORD pid, const std::wstring& injectdll);
void DetachFromProcess(HANDLE targetProcess, const std::wstring& injectdll);
