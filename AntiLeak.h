#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>

#pragma pack(1)

typedef struct _PROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	void* PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	ULONG_PTR ParentProcessId;
} PROCESS_BASIC_INFORMATION;

typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING,
UNICODE_STRING, *PUNICODE_STRING;


typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING TypeName;
	ULONG TotalNumberOfHandles;
	ULONG TotalNumberOfObjects;
}OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_ALL_INFORMATION {
	ULONG NumberOfObjects;
	OBJECT_TYPE_INFORMATION ObjectTypeInformation[1];
}OBJECT_ALL_INFORMATION, *POBJECT_ALL_INFORMATION;

#pragma pack()

inline bool CheckDbgPresentCloseHandle()
{
	HANDLE Handle = (HANDLE)0x8000;
	__try
	{
		CloseHandle(Handle);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return true;
	}

	return false;
}

inline void ErasePEHeaderFromMemory()
{
	DWORD OldProtect = 0;

	char *pBaseAddr = (char*)GetModuleHandle(NULL);

	VirtualProtect(pBaseAddr, 4096,
		PAGE_READWRITE, &OldProtect);

	ZeroMemory(pBaseAddr, 4096);
}

inline DWORD GetProcessIdFromName(LPCTSTR ProcessName)
{
	PROCESSENTRY32 pe32;
	HANDLE hSnapshot = NULL;
	ZeroMemory(&pe32, sizeof(PROCESSENTRY32));

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnapshot, &pe32) == FALSE)
	{
		CloseHandle(hSnapshot);
		return 0;
	}

	if (_tcsicmp(pe32.szExeFile, ProcessName) == FALSE)
	{
		CloseHandle(hSnapshot);
		return pe32.th32ProcessID;
	}

	while (Process32Next(hSnapshot, &pe32))
	{
		if (_tcsicmp(pe32.szExeFile, ProcessName) == 0)
		{
			CloseHandle(hSnapshot);
			return pe32.th32ProcessID;
		}
	}
	CloseHandle(hSnapshot);
	return 0;
}

inline DWORD GetCsrssProcessId()
{
	OSVERSIONINFO osinfo;
	ZeroMemory(&osinfo, sizeof(OSVERSIONINFO));
	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osinfo);

	if (osinfo.dwMajorVersion >= 5 && osinfo.dwMinorVersion >= 1)

	{
		typedef DWORD(__stdcall *pCsrGetId)();

		pCsrGetId CsrGetProcessId = (pCsrGetId)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "CsrGetProcessId");

		if (CsrGetProcessId)
			return CsrGetProcessId();
		else
			return 0;
	}
	return GetProcessIdFromName(TEXT("csrss.exe"));
}

inline DWORD GetExplorerPIDbyShellWindow()
{
	DWORD PID = 0;

	GetWindowThreadProcessId(GetShellWindow(), &PID);

	return PID;
}

DWORD GetParentProcessId()
{
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	NTSTATUS Status = 0;
	PROCESS_BASIC_INFORMATION pbi;
	ZeroMemory(&pbi, sizeof(PROCESS_BASIC_INFORMATION));
	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	if (NtQIP == 0)
		return 0;
	Status = NtQIP(GetCurrentProcess(), 0, (void*)&pbi,
		sizeof(PROCESS_BASIC_INFORMATION), 0);

	if (Status != 0x00000000)
		return 0;
	else
		return pbi.ParentProcessId;
}

inline bool CanOpenCsrss()
{
	HANDLE Csrss = 0;

	Csrss = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCsrssProcessId());

	if (Csrss != NULL)
	{
		CloseHandle(Csrss);
		return true;
	}
	else
		return false;
}

bool IsParentExplorerExe()
{
	DWORD PPID = GetParentProcessId();
	if (PPID == GetExplorerPIDbyShellWindow())
		return true;
	else
		return false;
}

void DebugSelf()
{
	HANDLE hProcess = NULL;
	DEBUG_EVENT de;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&de, sizeof(DEBUG_EVENT));

	GetStartupInfo(&si);

	CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE,
	DEBUG_PROCESS, NULL, NULL, &si, &pi);
	ContinueDebugEvent(pi.dwProcessId, pi.dwThreadId, DBG_CONTINUE);
	WaitForDebugEvent(&de, INFINITE);
}

inline bool HideThread(HANDLE hThread)
{
	typedef NTSTATUS(NTAPI *pNtSetInformationThread)
		(HANDLE, UINT, PVOID, ULONG);

	NTSTATUS Status;

	pNtSetInformationThread NtSIT = (pNtSetInformationThread)
		GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtSetInformationThread");
	if (NtSIT == NULL)
		return false;

	if (hThread == NULL)
		Status = NtSIT(GetCurrentThread(),
			0x11,
			0, 0);
	else
		Status = NtSIT(hThread, 0x11, 0, 0);

	if (Status != 0x00000000)
		return false;
	else
		return true;
}

inline bool DebugObjectCheck()
{
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	HANDLE hDebugObject = NULL;
	NTSTATUS Status;

	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	Status = NtQIP(GetCurrentProcess(),
		0x1e,
		&hDebugObject, 4, NULL);

	if (Status != 0x00000000)
		return false;

	if (hDebugObject)
		return true;
	else
		return false;
}

inline bool CheckProcessDebugFlags()
{
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	DWORD NoDebugInherit = 0;
	NTSTATUS Status;

	pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)
		GetProcAddress(
			GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

	Status = NtQIP(GetCurrentProcess(),
		0x1f,
		&NoDebugInherit, 4, NULL);

	if (Status != 0x00000000)
		return false;

	if (NoDebugInherit == FALSE)
		return true;
	else
		return false;
}

inline bool CheckOutputDebugString(LPCTSTR String)
{
	OutputDebugString(String);
	if (GetLastError() == 0)
		return true;
	else
		return false;
}

inline bool Int2DCheck()
{
	__try
	{
		__asm
		{
			int 0x2d
			xor eax, eax
			add eax, 2
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

inline void PushPopSS()
{

	__asm
	{
		push ss
		pop ss
		mov eax, 9
		xor edx, edx
	}
}

inline bool ObjectListCheck()
{
	typedef NTSTATUS(NTAPI *pNtQueryObject)
		(HANDLE, UINT, PVOID, ULONG, PULONG);

	POBJECT_ALL_INFORMATION pObjectAllInfo = NULL;
	void *pMemory = NULL;
	NTSTATUS Status;
	unsigned long Size = 0;

	pNtQueryObject NtQO = (pNtQueryObject)GetProcAddress(
		GetModuleHandle(TEXT("ntdll.dll")), "NtQueryObject");
	Status = NtQO(NULL, 3,
		&Size, 4, &Size);
	pMemory = VirtualAlloc(NULL, Size, MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	if (pMemory == NULL)
		return false;
	Status = NtQO((HANDLE)-1, 3, pMemory, Size, NULL);
	if (Status != 0x00000000)
	{
		VirtualFree(pMemory, 0, MEM_RELEASE);
		return false;
	}
	pObjectAllInfo = (POBJECT_ALL_INFORMATION)pMemory;

	unsigned char *pObjInfoLocation =
		(unsigned char*)pObjectAllInfo->ObjectTypeInformation;

	ULONG NumObjects = pObjectAllInfo->NumberOfObjects;

	for (UINT i = 0; i < NumObjects; i++)
	{

		POBJECT_TYPE_INFORMATION pObjectTypeInfo =
			(POBJECT_TYPE_INFORMATION)pObjInfoLocation;

		if (wcscmp(L"DebugObject", pObjectTypeInfo->TypeName.Buffer) == 0)
		{
			if (pObjectTypeInfo->TotalNumberOfObjects > 0)
			{
				VirtualFree(pMemory, 0, MEM_RELEASE);
				return true;
			}
			else
			{
				VirtualFree(pMemory, 0, MEM_RELEASE);
				return false;
			}
		}

		pObjInfoLocation =
			(unsigned char*)pObjectTypeInfo->TypeName.Buffer;

		pObjInfoLocation +=
			pObjectTypeInfo->TypeName.Length;

		ULONG tmp = ((ULONG)pObjInfoLocation) & -4;

		pObjInfoLocation = ((unsigned char*)tmp) +
			sizeof(unsigned long);
	}

	VirtualFree(pMemory, 0, MEM_RELEASE);
	return true;
}

inline bool IsDbgPresentPrefixCheck()
{
	__try
	{
		__asm __emit 0xF3
		__asm __emit 0x64
		__asm __emit 0xF1
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

class CAntiLeak
{
public:
	void ErasePE();
}; CAntiLeak *AntiLeak = new CAntiLeak;

