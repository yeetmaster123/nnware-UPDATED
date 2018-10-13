#pragma once

#include <string>
#include <vector>
#include <locale>
#include <Windows.h>
#include <Iphlpapi.h>
#include "XorStr.h"

#include "Crypto.h"
#include "AntiCrack.h"

namespace ModuleSec
{
	std::string GetLoaderChecksum();
	std::string GetHWID();
	std::string GetStoredLoginPath();
	void StoreLogin(std::string User, std::string Pass, bool RememberMe);
	void GetLogin(std::string& User, std::string& Pass, bool& RememberMe);
	void StartSecurityThread();
	std::string GetBanReason();
	void SecurityThread();
}