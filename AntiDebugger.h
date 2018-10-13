#pragma once

namespace AntiDebugger
{
	bool CheckIfDebuggerIsPresent();
	bool CheckGlobalFlags();
	bool CheckHeapFlags();
	bool CheckHeapForceFlags();
	bool CheckIfDebuggerIsPresent_NtCall();
	bool CheckHiddenThreadDebugger();
	bool CheckInvalidateHandle();
	//bool CheckExceptionFilterTest();
	//bool CheckHardwareBreakpoints();
	//bool CheckSoftwareBreakpoints();
	//bool CheckMemoryBreakpoints();
	//bool CheckProtectedHandle();
};

