#pragma once

namespace SDK
{
	class CUserCmd;
	class CBaseWeapon;
}

class CMovement
{
public:
	bool is_stopping;
	void bunnyhop(SDK::CUserCmd* cmd);
	void autostrafer(SDK::CUserCmd* cmd);
	void duckinair(SDK::CUserCmd* cmd);
	void full_stop(SDK::CUserCmd * cmd);
	void quick_stop(SDK::CBaseEntity * entity, SDK::CUserCmd * cmd);
	void circle_strafe(SDK::CUserCmd* cmd, float* circle_yaw);
};

extern CMovement* movement;