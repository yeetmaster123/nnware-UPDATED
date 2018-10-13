#pragma once
#include "AntiAnalysis.h"
#include "AntiDebugger.h"
#include "AntiVM.h"
#include "AntiDump.h"

namespace AntiCrack
{
	void Protect();
	bool CheckForDebugger();
	bool CheckForProcesses();
	bool CheckForVM();
	bool DoChecks();
}