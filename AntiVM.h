#pragma once

namespace AntiVM
{
	namespace Generic
	{
		bool CheckBlacklistedDlls();
		bool CheckNumCores();
		bool CheckSetupdiDD();
		bool CheckMemSize();
		bool CheckHypervisor();
		bool CheckHypervisorVendor();
	}

	namespace AntiWINE
	{
		bool CheckDllExports();
		bool CheckRegKeys();
	}

	namespace AntiVMWare
	{
		bool LDTCheck();
		bool STRCheck();
		bool CheckBlacklistedMAC();
		bool CheckRegKeys();
		bool CheckBlacklistedDrivers();
		bool CheckVirtualDevices();
		bool CheckVMWareDirectory();
	}

	namespace AntiVirtualBox
	{
		bool CheckBlacklistedMAC();
		bool CheckRegKeys();
		bool CheckBlacklistedFiles();
		bool CheckVirtualDevices();
		bool CheckVBoxDirectory();
		bool CheckBlacklistedProcesses();
	}
}