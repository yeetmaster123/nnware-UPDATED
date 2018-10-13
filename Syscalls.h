#pragma once
#include <Windows.h>

BOOL NtProtectVirtualMemory_Wrapper(HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T nSize, DWORD dwNewProtect, PDWORD pdwOldProtect);
BOOL NtReadVirtualMemory_Wrapper(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
BOOL NtWriteVirtualMemory_Wrapper(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
BOOL NtFreeVirtualMemory_Wrapper(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
LPVOID NtAllocateVirtualMemory_Wrapper(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);