#pragma once

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
}

class CVisuals
{
public:
	void DisableFlashDuration();
	struct damage_indicator_t {
		int dmg;
		bool initializes;
		float earse_time;
		float last_update;
		SDK::CBaseEntity * player;
		Vector Position;
	};
	std::vector<damage_indicator_t> dmg_indicator;
	void Radar();
	void Draw();
	void ClientDraw();
	void DrawBacktrack();
	void DrawWeaponIcon(SDK::CBaseEntity* entity, CColor color, int index);
	void apply_clantag();
	void DrawInaccuracy();
	void DrawBulletBeams();
	void ModulateWorld();
	void LagCompHitbox(SDK::CBaseEntity * entity, int index);
	void ModulateSky();
	void set_hitmarker_time( float time );
	void LogEvents();
	void DoFSN();
	void Clantag();
	void GameSense();
	void DrawDamageIndicator();
	void AsusProps();
private:
	void DrawBox(SDK::CBaseEntity* entity, CColor color, Vector pos, Vector top);
	void DrawName(SDK::CBaseEntity* entity, CColor color, int index, Vector pos, Vector top);
	void DrawWeapon(SDK::CBaseEntity * entity, CColor color, int index, Vector pos, Vector top);
	void DrawHealth(SDK::CBaseEntity * entity, CColor color, CColor dormant, Vector pos, Vector top);
	void BombPlanted(SDK::CBaseEntity * entity);
	void DrawDropped(SDK::CBaseEntity * entity);
	void DrawAmmo(SDK::CBaseEntity * entity, CColor color, CColor dormant, Vector pos, Vector top);
	float resolve_distance(Vector src, Vector dest);
	void DrawDistance(SDK::CBaseEntity * entity, CColor color, Vector pos, Vector top);
	void DrawInfo(SDK::CBaseEntity * entity, CColor color, CColor alt, Vector pos, Vector top);
	void DrawFovArrows(SDK::CBaseEntity* entity, CColor color);
	void DrawCrosshair();
	void DrawIndicator();
	void DrawHitmarker();
	void DrawBorderLines();
public:
	std::vector<std::pair<int, float>>				Entities;
	std::deque<UTILS::BulletImpact_t>				Impacts;
};

extern CVisuals* visuals;