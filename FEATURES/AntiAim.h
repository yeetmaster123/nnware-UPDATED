#pragma once
namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
}

class CAntiAim
{
public:
	void do_antiaim(SDK::CUserCmd* cmd);
	void BetaAA(SDK::CUserCmd* cmd);
	void fix_movement(SDK::CUserCmd* cmd);
	Vector fix_movement(SDK::CUserCmd* cmd, SDK::CUserCmd orignal);
private:
	void backwards(SDK::CUserCmd* cmd);
	void legit(SDK::CUserCmd* cmd);
	void sidespin(SDK::CUserCmd * cmd);
	void freestand(SDK::CUserCmd * cmd);
	void sideways(SDK::CUserCmd* cmd);
	void lowerbody(SDK::CUserCmd* cmd);
	void backjitter(SDK::CUserCmd* cmd);
};

extern CAntiAim* antiaim;