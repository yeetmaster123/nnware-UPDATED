#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <WbemCli.h>
#include <objbase.h>
#include <Iphlpapi.h>
#include <string>
#include <codecvt>
#include <Shlwapi.h>
#include <vector>
#include "XorStr.h"

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Mpr.lib")
#pragma comment(lib, "setupapi.lib")

#define	MALLOC(x)	HeapAlloc(GetProcessHeap(), 0, x)
#define FREE(x)		HeapFree(GetProcessHeap(), 0, x)

#define XOR_L(x) strtowstr(XorStr(x)).c_str()

#if _WIN32 || _WIN64
#if _WIN64
#define ENV64BIT
#else
#define ENV32BIT
#endif
#endif

std::wstring strtowstr(std::string source);
std::string wstrtostr(std::wstring source);

bool InitWMI(IWbemServices** pSvc, IWbemLocator** pLoc);
bool ExecWMIQuery(IWbemServices** pSvc, IWbemLocator** pLoc, IEnumWbemClassObject** pEnumerator, wchar_t* szQuery);
bool DoesRegKeyValueExist(HKEY hKey, const wchar_t* lpSubKey, const wchar_t* lpValueName, const wchar_t* search_str);
bool DoesRegKeyExist(HKEY hKey, const wchar_t* lpSubKey);
bool DoesFileExist(wchar_t* szPath);
bool DoesDirectoryExist(wchar_t* szPath);

DWORD FindProcessID(const std::wstring& ProcName);
ULONG GetNumberOfProcessors();
int CheckMACAddress(const wchar_t* szMac);
// just a feeble attempt at detecting detours
bool CheckForBytes(void* Ptr, int NumBytes, std::vector<uint8_t> Bytes);
DWORD SigScan(HANDLE hProcess, DWORD dwAddress, DWORD dwEnd, const char* szSignature);