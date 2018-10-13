#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/IViewRenderBeams.h"
#include "../FEATURES/Backtracking.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseWeapon.h"
#include "../FEATURES/AutoWall.h"
#include "../SDK/CTrace.h"	
#include "../FEATURES/Resolver.h"
#include "../SDK/CGlobalVars.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Visuals.h"
#include "../UTILS/render.h"
#include "../SDK/IVDebugOverlay.h"
#include <string.h>
#define XorStr( s ) ( s )
float flPlayerAlpha[255];
CColor breaking;
CColor backtrack;
CColor world_color;
static bool bPerformed = false, bLastSetting;
float fade_alpha[65];
float dormant_time[65];
CColor main_color;
CColor ammo;
SDK::CBaseEntity *BombCarrier;


void CVisuals::DoFSN()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	static bool world_performed = false, world_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_performed != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!local_player)
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_lastsetting != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!world_performed)
	{
		if (SETTINGS::settings.night_col != world_color)
		{
			for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
			{
				SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

				if (!pMaterial)
					continue;

				if (SETTINGS::settings.sky_enabled)
				{
					if (strstr(pMaterial->GetTextureGroupName(), "World textures") || strstr(pMaterial->GetTextureGroupName(), "StaticProp textures"))
					{
						pMaterial->ColorModulate(world_color);
					}
					else
					{
						pMaterial->ColorModulate(1, 1, 1);
					}
				}
			}
			world_color = SETTINGS::settings.night_col;
		}
	}

}

void CVisuals::set_hitmarker_time( float time ) 
{
	GLOBAL::flHurtTime = time;
}
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))
void CVisuals::Draw()
{
	if ( !INTERFACES::Engine->IsInGame( ) ) {
		GLOBAL::flHurtTime = 0.f;
		return;
	}
	DrawCrosshair();
	for (int i = 1; i <= 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!entity) continue;
		if (!local_player) continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player) continue;
		if (entity->GetHealth() <= 0) continue;
		if (entity->GetVecOrigin() == Vector(0, 0, 0)) continue;
		if (entity->GetIsDormant() && flPlayerAlpha[i] > 0) flPlayerAlpha[i] -= .3;
		else if (!entity->GetIsDormant() && flPlayerAlpha[i] < 255) flPlayerAlpha[i] = 255;

		float playerAlpha = flPlayerAlpha[i];
		int enemy_hp = entity->GetHealth();
		int hp_red = 255 - (enemy_hp * 2.55);
		int hp_green = enemy_hp * 2.55;
		CColor health_color = CColor(hp_red, hp_green, 1, playerAlpha);
		CColor dormant_color = CColor(100, 100, 100, playerAlpha);
		CColor box_color, still_health, alt_color, zoom_color, name_color, weapon_color, distance_color, arrow_col;

		static auto alpha = 0.f; static auto plus_or_minus = false;
		if (alpha <= 0.f || alpha >= 255.f) plus_or_minus = !plus_or_minus;
		alpha += plus_or_minus ? (255.f / 7 * 0.015) : -(255.f / 7 * 0.015); alpha = clamp(alpha, 0.f, 255.f);
		float arrow_colour[3] = { SETTINGS::settings.fov_col.RGBA[0], SETTINGS::settings.fov_col.RGBA[1], SETTINGS::settings.fov_col.RGBA[2] };
		float arrowteam_colour[3] = { SETTINGS::settings.arrowteam_col.RGBA[0], SETTINGS::settings.arrowteam_col.RGBA[1], SETTINGS::settings.arrowteam_col.RGBA[2] };

		if (entity->GetIsDormant())
		{
			main_color = dormant_color;
			still_health = health_color;
			alt_color = CColor(20, 20, 20, playerAlpha);
			zoom_color = dormant_color;
			breaking = dormant_color;
			backtrack = dormant_color;
			box_color = dormant_color;
			ammo = dormant_color;
			name_color = dormant_color;
			weapon_color = dormant_color;
			distance_color = dormant_color;
			arrow_col = dormant_color;
		}
		else if (!entity->GetIsDormant())
		{
			main_color = CColor(255, 255, 255, playerAlpha);
			still_health = health_color;
			alt_color = CColor(0, 0, 0, 165);
			zoom_color = CColor(150, 150, 220, 165);
			breaking = CColor(220, 150, 150, 165);
			backtrack = CColor(155, 220, 150, 165);
			box_color = SETTINGS::settings.box_col;
			ammo = CColor(61, 135, 255, 165);
			name_color = SETTINGS::settings.name_col;
			weapon_color = SETTINGS::settings.weapon_col;
			distance_color = SETTINGS::settings.distance_col;
			arrow_col = SETTINGS::settings.fov_col;
		}
		Vector min, max, pos, pos3D, top, top3D; entity->GetRenderBounds(min, max);
		pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10); top3D = pos3D + Vector(0, 0, max.z + 10);

		if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
		{
			if (!is_teammate)
			{
				if (SETTINGS::settings.box_bool) DrawBox(entity, box_color, pos, top);
				if (SETTINGS::settings.name_bool) DrawName(entity, name_color, i, pos, top);
				if (SETTINGS::settings.weap_bool) DrawWeapon(entity, weapon_color, i, pos, top);
				if (SETTINGS::settings.draw_wep == 0) DrawWeapon(entity, weapon_color, i, pos, top);
				if (SETTINGS::settings.draw_wep == 1) DrawWeapon(entity, weapon_color, i, pos, top);
				if (SETTINGS::settings.health_bool) DrawHealth(entity, still_health, alt_color, pos, top);
				if (SETTINGS::settings.ammo_bool) DrawAmmo(entity, ammo, alt_color, pos, top);
				if (SETTINGS::settings.WeaponIconsOn) {
					visuals->DrawWeaponIcon(entity, main_color, i);
				}
			}
			else if (is_teammate)
			{
				if (SETTINGS::settings.boxteam) DrawBox(entity, SETTINGS::settings.boxteam_col, pos, top);
				if (SETTINGS::settings.nameteam) DrawName(entity, SETTINGS::settings.nameteam_col, i, pos, top);
				if (SETTINGS::settings.weaponteam) DrawWeapon(entity, SETTINGS::settings.weaponteam_col, i, pos, top);
				if (SETTINGS::settings.healthteam) DrawHealth(entity, still_health, alt_color, pos, top);
				if (SETTINGS::settings.ammoteam) DrawAmmo(entity, ammo, alt_color, pos, top);
			}
			DrawInfo(entity, main_color, zoom_color, pos, top);
		}
		if (!is_teammate)
		{
			if (SETTINGS::settings.fov_bool) DrawFovArrows(entity, CColor(arrow_colour[0], arrow_colour[1], arrow_colour[2], alpha));
		}
		else if  (is_teammate)
		{ 
			if (SETTINGS::settings.arrowteam) DrawFovArrows(entity, CColor(arrowteam_colour[0], arrowteam_colour[1], arrowteam_colour[2], alpha));
		}
	}
}

void CVisuals::DisableFlashDuration()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (local_player)
		return;

	if (SETTINGS::settings.no_flash_enabled)
		local_player->SetFlashDuration(0.f);
}

/*void CVisuals::DrawBacktrack()
{

	auto local = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	SDK::player_info_t pinfo;

	for (int i = 0; i < INTERFACES::ClientEntityList->GetHighestEntityIndex(); i++) {
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		if (INTERFACES::Engine->GetPlayerInfo(i, &pinfo) && entity->GetHealth() > 0) {
			if (local->GetHealth() > 0) {
				for (int t = 0; t < 12; ++t) {
					Vector screenbacktrack[64][12];
					int index = entity->GetIndex();
					if (RENDER::WorldToScreen(backtrack_hitbox[index][i][t], screenbacktrack[i][t]))
					{
						RENDER::DrawFilledRect(screenbacktrack[i][t].x, screenbacktrack[i][t].y, screenbacktrack[i][t].x + 2, screenbacktrack[i][t].y + 2, CColor(0, 255, 255));
					}
				}
			}
			else {
				memset(&headPositions[0][0], 0, sizeof(headPositions));
			}
		}
	}
}*/

void CVisuals::DrawBacktrack()
{
	for (int i = 0; i < INTERFACES::ClientEntityList->GetHighestEntityIndex(); i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		if (entity->GetHealth() > 0)
		{
			for (int t = 0; t < 12; t++)
			{
				Vector screenbacktrack[64][12];
				int index = entity->GetIndex();
				if (RENDER::WorldToScreen(backtrack_hitbox[index][i][t], screenbacktrack[i][t]))
				{
					RENDER::DrawFilledRect(screenbacktrack[i][t].x, screenbacktrack[i][t].y, screenbacktrack[i][t].x + 2, screenbacktrack[i][t].y + 2, CColor(0, 255, 255));
				}
			}
		}
	}
}

void CVisuals::ClientDraw()
{

	if (SETTINGS::settings.spread_bool) DrawInaccuracy();
	if (SETTINGS::settings.scope_bool) DrawBorderLines();

	DrawIndicator();
	DrawHitmarker();

	auto pLocal = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	static SDK::ConVar* crosshair = INTERFACES::cvar->FindVar("weapon_debug_spread_show");
	if (SETTINGS::settings.forcehair)
	{
		crosshair->SetValue(3);
	}
	else {
		crosshair->SetValue(0);
	}
}

std::string str_to_upper(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);

	return strToConvert;
}

void CVisuals::AsusProps() {
	static bool asuswalls_performed = false, asus_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame()) {
		if (asuswalls_performed)
			asuswalls_performed = false;
		return;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player) {
		if (asuswalls_performed)
			asuswalls_performed = false;
		return;
	}

	if (asus_lastsetting != SETTINGS::settings.asus_bool) {
		asus_lastsetting = SETTINGS::settings.asus_bool;
		asuswalls_performed = false;
	}

	if (!asuswalls_performed) {
		static SDK::ConVar* staticdrop = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		staticdrop->SetValue(0);

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i)) {
			SDK::IMaterial *pmat = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pmat)
				continue;
			if (strstr(pmat->GetTextureGroupName(), XorStr("StaticProp"))) {
				if (SETTINGS::settings.asus_bool) {
					pmat->AlphaModulate(0.60f);
					pmat->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, true);
				}
				else {
					pmat->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, false);
					pmat->AlphaModulate(1.0f);
				}
			}

		}
		asuswalls_performed = true;
	}
}

void CVisuals::DrawBox(SDK::CBaseEntity* entity, CColor color, Vector pos, Vector top)
{
	float alpha = flPlayerAlpha[entity->GetIndex()];
	int height = (pos.y - top.y), width = height / 2;

	RENDER::DrawEmptyRect(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, color);
	RENDER::DrawEmptyRect((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, CColor(20, 20, 20, alpha));
	RENDER::DrawEmptyRect((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, CColor(20, 20, 20, alpha));
}

void CVisuals::DrawName(SDK::CBaseEntity* entity, CColor color, int index, Vector pos, Vector top)
{
	SDK::player_info_t ent_info; INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	int height = (pos.y - top.y), width = height / 2;
	RENDER::DrawF(pos.x, top.y - 7, FONTS::visuals_name_font, true, true, color, ent_info.name);
}

float CVisuals::resolve_distance(Vector src, Vector dest)
{
	Vector delta = src - dest;
	float fl_dist = ::sqrtf((delta.Length()));
	if (fl_dist < 1.0f) return 1.0f;
	return fl_dist;
}

void CVisuals::DrawDistance(SDK::CBaseEntity* entity, CColor color, Vector pos, Vector top)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	SDK::player_info_t ent_info;
	Vector vecOrigin = entity->GetVecOrigin(), vecOriginLocal = local_player->GetVecOrigin();

	char dist_to[32]; int height = (pos.y - top.y), width = height / 2;

	sprintf_s(dist_to, "%.0f ft", resolve_distance(vecOriginLocal, vecOrigin));
	RENDER::DrawF(pos.x, SETTINGS::settings.ammo_bool ? pos.y + 12: pos.y + 8, FONTS::visuals_esp_font, true, true, color, dist_to);
}

std::string fix_item_name(std::string name)
{
	if (name[0] == 'C')
		name.erase(name.begin());

	auto startOfWeap = name.find("Weapon");
	if (startOfWeap != std::string::npos)
		name.erase(name.begin() + startOfWeap, name.begin() + startOfWeap + 6);

	return name;
}

void CVisuals::DrawWeapon(SDK::CBaseEntity* entity, CColor color, int index, Vector pos, Vector top)
{
	SDK::player_info_t ent_info; INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	if (!weapon) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	bool is_teammate = local_player->GetTeam() == entity->GetTeam(), distanceThing, distanceThing2;
	if (SETTINGS::settings.ammo_bool) distanceThing = true; else distanceThing = false; if (SETTINGS::settings.ammoteam) distanceThing2 = true; else distanceThing2 = false;
	int height = (pos.y - top.y), width = height / 2, distanceOn = distanceThing ? pos.y + 12 : pos.y + 8, distanceOn2 = distanceThing2 ? pos.y + 12 : pos.y + 8;
	if (SETTINGS::settings.draw_wep == 0)
	{
		if (c_baseweapon->is_revolver())
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "R8 REVOLVER");
		else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_USP_SILENCER)
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "USP-S");
		else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_M4A1_SILENCER)
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "M4A1-S");
		else
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));
	}
	else if (SETTINGS::settings.draw_wep == 1)
	{
		RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName) + " (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")");
	}

	if (SETTINGS::settings.weaponteam)
	{
		if (c_baseweapon->is_revolver())
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "R8 REVOLVER");
		else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_USP_SILENCER)
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "USP-S");
		else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_M4A1_SILENCER)
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "M4A1-S");
		else
			RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));
	}
}

void CVisuals::DrawHealth(SDK::CBaseEntity* entity, CColor color, CColor dormant, Vector pos, Vector top)
{
	int enemy_hp = entity->GetHealth(),
		hp_red = 255 - (enemy_hp * 2.55),
		hp_green = enemy_hp * 2.55,
		height = (pos.y - top.y),
		width = height / 2;

	float offset = (height / 4.f) + 5;
	UINT hp = height - (UINT)((height * enemy_hp) / 100);

	RENDER::DrawEmptyRect((pos.x - width / 2) - 6, top.y, (pos.x - width / 2) - 3, top.y + height, dormant);
	RENDER::DrawLine((pos.x - width / 2) - 4, top.y + hp, (pos.x - width / 2) - 4, top.y + height, color);
	RENDER::DrawLine((pos.x - width / 2) - 5, top.y + hp, (pos.x - width / 2) - 5, top.y + height, color);

	if (entity->GetHealth() < 100)
		RENDER::DrawF((pos.x - width / 2) - 4, top.y + hp, FONTS::visuals_esp_font, true, true, main_color, std::to_string(enemy_hp));
}

void CVisuals::DrawDamageIndicator() {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player->GetHealth() > 0)
		return;

	CColor dmg_color = SETTINGS::settings.dmg_color;

	float CurrentTime = local_player->GetTickBase() * INTERFACES::Globals->interval_per_tick;

	for (int i = 0; i < dmg_indicator.size(); i++) {
		if (dmg_indicator[i].earse_time < CurrentTime) {
			dmg_indicator.erase(dmg_indicator.begin() + i);
			continue;
		}

		if (!dmg_indicator[i].initializes) {
			dmg_indicator[i].Position = dmg_indicator[i].player->GetBonePosition(6);
			dmg_indicator[i].initializes = true;
		}

		if (CurrentTime - dmg_indicator[i].last_update > 0.0001f) {
			dmg_indicator[i].Position.z -= (0.1f * (CurrentTime - dmg_indicator[i].earse_time));
			dmg_indicator[i].last_update = CurrentTime;

			Vector ScreenPosition;

			if (RENDER::WorldToScreen(dmg_indicator[i].Position, ScreenPosition)) {
				RENDER::DrawF(ScreenPosition.x, ScreenPosition.y, FONTS::menu_window_font, true, true, dmg_color, std::to_string(dmg_indicator[i].dmg).c_str());
			}
		}
	}
}

void CVisuals::BombPlanted(SDK::CBaseEntity* entity)
{
	BombCarrier = nullptr;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector vOrig; Vector vScreen;
	vOrig = entity->GetVecOrigin();
	SDK::CCSBomb* Bomb = (SDK::CCSBomb*)entity;

	float flBlow = Bomb->GetC4BlowTime();
	float TimeRemaining = flBlow;
	char buffer[64];
	sprintf_s(buffer, "B - %.1fs", TimeRemaining);
	RENDER::DrawF(10, 10, FONTS::visuals_lby_font, false, false, CColor(124, 195, 13, 255), buffer);
}

void CVisuals::DrawDropped(SDK::CBaseEntity* entity)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	SDK::CBaseWeapon* weapon_cast = (SDK::CBaseWeapon*)entity;

	if (!weapon_cast)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (!weapon)
		return;

	SDK::CBaseEntity* plr = INTERFACES::ClientEntityList->GetClientEntityFromHandle((HANDLE)weapon_cast->GetOwnerHandle());
	if (!plr && RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		std::string ItemName = fix_item_name(weapon->GetClientClass()->m_pNetworkName);
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, pos.y, FONTS::visuals_esp_font, true, true, WHITE, ItemName.c_str());
	}
}

void CVisuals::DrawAmmo(SDK::CBaseEntity* entity, CColor color, CColor dormant, Vector pos, Vector top)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	int height = (pos.y - top.y);

	float offset = (height / 4.f) + 5;
	UINT hp = height - (UINT)((height * 3) / 100);

	auto animLayer = entity->GetAnimOverlay(1);
	if (!animLayer.m_pOwner)
		return;

	auto activity = entity->GetSequenceActivity(animLayer.m_nSequence);

	int iClip = c_baseweapon->GetLoadedAmmo();
	int iClipMax = c_baseweapon->get_full_info()->max_clip;

	float box_w = (float)fabs(height / 2);
	float width;
	if (activity == 967 && animLayer.m_flWeight != 0.f)
	{
		float cycle = animLayer.m_flCycle;
		width = (((box_w * cycle) / 1.f));
	}
	else
		width = (((box_w * iClip) / iClipMax));

	RENDER::DrawFilledRect((pos.x - box_w / 2), top.y + height + 3, (pos.x - box_w / 2) + box_w + 2, top.y + height + 7, dormant);
	RENDER::DrawFilledRect((pos.x - box_w / 2) + 1, top.y + height + 4, (pos.x - box_w / 2) + width + 1, top.y + height + 6, color);
}

void CVisuals::DrawInfo(SDK::CBaseEntity* entity, CColor color, CColor alt, Vector pos, Vector top)
{
	std::vector<std::pair<std::string, CColor>> stored_info;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (SETTINGS::settings.money_bool && !is_teammate)
		stored_info.push_back(std::pair<std::string, CColor>("$" + std::to_string(entity->GetMoney()), backtrack));
	else if (SETTINGS::settings.moneyteam && is_teammate)
		stored_info.push_back(std::pair<std::string, CColor>("$" + std::to_string(entity->GetMoney()), backtrack));

	if (SETTINGS::settings.info_bool && !is_teammate)
	{
		if (entity->GetArmor() > 0)
			stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

		if (entity->GetIsScoped())
			stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));
	}
	else if (SETTINGS::settings.flagsteam && is_teammate)
	{
		if (entity->GetArmor() > 0)
			stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

		if (entity->GetIsScoped())
			stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));
	}

	int height = (pos.y - top.y), width = height / 2, i = 0;
	for (auto Text : stored_info)
	{
		RENDER::DrawF((pos.x + width / 2) + 5, top.y + i, FONTS::visuals_esp_font, false, false, Text.second, Text.first);
		i += 8;
	}
}

void CVisuals::DrawInaccuracy()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon) return;

	int spread_Col[3] = { SETTINGS::settings.spread_Col.RGBA[0], SETTINGS::settings.spread_Col.RGBA[1], SETTINGS::settings.spread_Col.RGBA[2] };


	int W, H, cW, cH;
	INTERFACES::Engine->GetScreenSize(W, H);
	cW = W / 2; cH = H / 2;

	if (local_player->IsAlive())
	{
		auto accuracy = (weapon->GetInaccuracy() + weapon->GetSpreadCone()) * 500.f;

		float r;
		float alpha, newAlpha;

		for (r = accuracy; r>0; r--)
		{
			if (!weapon->is_grenade() && !weapon->is_knife())


				alpha = r / accuracy;
			newAlpha = pow(alpha, 5);

			RENDER::DrawCircle(cW, cH, r, 60, CColor(spread_Col[0], spread_Col[1], spread_Col[2], newAlpha * 130));
		}
	}
}

void CVisuals::DrawBulletBeams()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	if (!INTERFACES::Engine->IsInGame() || !local_player) { Impacts.clear(); return; }
	if (Impacts.size() > 30) Impacts.pop_back();

	for (int i = 0; i < Impacts.size(); i++)
	{
		auto current = Impacts.at(i);
		if (!current.pPlayer) continue;
		if (current.pPlayer->GetIsDormant()) continue;

		bool is_local_player = current.pPlayer == local_player;
		bool is_teammate = local_player->GetTeam() == current.pPlayer->GetTeam() && !is_local_player;

		if (current.pPlayer == local_player)
			current.color = SETTINGS::settings.bulletlocal_col;
		else if (current.pPlayer != local_player && !is_teammate)
			current.color = SETTINGS::settings.bulletenemy_col;
		else if (current.pPlayer != local_player && is_teammate)
			current.color = SETTINGS::settings.bulletteam_col;

		SDK::BeamInfo_t beamInfo;
		beamInfo.m_nType = SDK::TE_BEAMPOINTS;
		beamInfo.m_pszModelName = "sprites/purplelaser1.vmt";
		beamInfo.m_nModelIndex = -1;
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 3.0f;
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = 2.0f;
		beamInfo.m_flBrightness = 255.f;
		beamInfo.m_flSpeed = 0.2f;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0.f;
		beamInfo.m_flRed = current.color.RGBA[0];
		beamInfo.m_flGreen = current.color.RGBA[1];
		beamInfo.m_flBlue = current.color.RGBA[2];
		beamInfo.m_nSegments = 2;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = SDK::FBEAM_ONLYNOISEONCE | SDK::FBEAM_NOTILE | SDK::FBEAM_HALOBEAM;

		beamInfo.m_vecStart = current.pPlayer->GetVecOrigin() + current.pPlayer->GetViewOffset();
		beamInfo.m_vecEnd = current.vecImpactPos;

		auto beam = INTERFACES::ViewRenderBeams->CreateBeamPoints(beamInfo);
		if (beam) INTERFACES::ViewRenderBeams->DrawBeam(beam);

		Impacts.erase(Impacts.begin() + i);
	}
}

void CVisuals::DrawCrosshair()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto crosshair = INTERFACES::cvar->FindVar("crosshair");
	if (SETTINGS::settings.xhair_type == 0)
	{
		crosshair->SetValue("1");
		return;
	}
	else 
		crosshair->SetValue("0");

	int W, H, cW, cH;
	INTERFACES::Engine->GetScreenSize(W, H);

	cW = W / 2; cH = H / 2;

	int dX = W / 120.f, dY = H / 120.f;
	int drX, drY;

	if (SETTINGS::settings.xhair_type == 2)
	{
		drX = cW - (int)(dX * (((local_player->GetPunchAngles().y * 2.f) * 0.45f) + local_player->GetPunchAngles().y));
		drY = cH + (int)(dY * (((local_player->GetPunchAngles().x * 2.f) * 0.45f) + local_player->GetPunchAngles().x));
	}
	else
	{
		drX = cW;
		drY = cH;
	}

	INTERFACES::Surface->DrawSetColor(BLACK);
	INTERFACES::Surface->DrawFilledRect(drX - 4, drY - 2, drX - 4 + 8, drY - 2 + 4);
	INTERFACES::Surface->DrawFilledRect(drX - 2, drY - 4, drX - 2 + 4, drY - 4 + 8);

	INTERFACES::Surface->DrawSetColor(WHITE);
	INTERFACES::Surface->DrawFilledRect(drX - 3, drY - 1, drX - 3 + 6, drY - 1 + 2);
	INTERFACES::Surface->DrawFilledRect(drX - 1, drY - 3, drX - 1 + 2, drY - 3 + 6);
}

void CVisuals::DrawFovArrows(SDK::CBaseEntity* entity, CColor color)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
	if (entity->GetIsDormant()) return;

	Vector screenPos, client_viewangles;
	int screen_width = 0, screen_height = 0;
	float radius = 300.f;

	if (UTILS::IsOnScreen(aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_HEAD), screenPos)) return;

	INTERFACES::Engine->GetViewAngles(client_viewangles);
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	const auto screen_center = Vector(screen_width / 2.f, screen_height / 2.f, 0);
	const auto rot = DEG2RAD(client_viewangles.y - UTILS::CalcAngle(local_player->GetEyePosition(), aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_HEAD)).y - 90);

	std::vector<SDK::Vertex_t> vertices;

	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot + DEG2RAD(2)) * (radius - 16))));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot - DEG2RAD(2)) * (radius - 16))));

	RENDER::TexturedPolygon(3, vertices, color);
}

void CVisuals::DrawWeaponIcon(SDK::CBaseEntity* entity, CColor color, int index)
{

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	Vector pos, pos3D, top, top3D;
	Vector min, max;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);


	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, top.y + 7, FONTS::visuals_icon_font, true, true, color, weapon->GetGunIcon());
	}

}

void CVisuals::DrawIndicator()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
	if (local_player->GetHealth() <= 0) return;

	float breaking_lby_fraction = fabs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw())) / 180.f;
	float lby_delta = abs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw()));

	int screen_width, screen_height;
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	int iY = 88;
	if (SETTINGS::settings.overrideenable)
	{
		iY += 22; bool overridekeyenabled;
		if (SETTINGS::settings.overridemethod == 0)
			RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, SETTINGS::settings.overridething ? CColor(0, 255, 0) : CColor(255, 0, 0), "OVERRIDE");
		else if (SETTINGS::settings.overridemethod == 1)
		{
			GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey)) ? 
				RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor(0, 255, 0), "OVERRIDE") : 
				RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor(255, 0, 0), "OVERRIDE");
		}
	}
	if (SETTINGS::settings.Watermark)
	{
		iY += 22;
		RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, SETTINGS::settings.stop_flip ? CColor(255, 215, 0) : CColor(0, 255, 255), "nnware.cf best pasta");
	}
	if (SETTINGS::settings.aa_bool && SETTINGS::settings.lbyenable)
	{
		iY += 22;
		RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor((1.f - breaking_lby_fraction) * 255.f, breaking_lby_fraction * 255.f, 0), "LBY");
	}
	if (SETTINGS::settings.rifk_arrow)
	{
		auto client_viewangles = Vector();
		INTERFACES::Engine->GetViewAngles(client_viewangles);
		const auto screen_center = Vector2D(screen_width / 2.f, screen_height / 2.f);

		constexpr auto radius = 100.f;
		auto draw_arrow = [&](float rot, CColor color) -> void
		{
			std::vector<SDK::Vertex_t> vertices;
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(8)) * (radius - 12), screen_center.y + sinf(rot + DEG2RAD(8)) * (radius - 12)))); //25
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(8)) * (radius - 12), screen_center.y + sinf(rot - DEG2RAD(8)) * (radius - 12)))); //25
			RENDER::TexturedPolygon(3, vertices, color);
		};

		static auto alpha = 0.f; static auto plus_or_minus = false;
		if (alpha <= 0.f || alpha >= 255.f) plus_or_minus = !plus_or_minus;
		alpha += plus_or_minus ? (255.f / 7 * 0.015) : -(255.f / 7 * 0.015); alpha = clamp(alpha, 0.f, 255.f);

		auto fake_color = CColor(255, 215, 0, alpha);
		const auto fake_rot = DEG2RAD(client_viewangles.y - GLOBAL::fake_angles.y - 90);
		draw_arrow(fake_rot, fake_color);

		auto real_color = CColor(0, 255, 255, alpha);
		const auto real_rot = DEG2RAD(client_viewangles.y - GLOBAL::real_angles.y - 90);
		draw_arrow(real_rot, real_color);
	}
}

void CVisuals::LogEvents()
{
	static bool convar_performed = false, convar_lastsetting;

	if (convar_lastsetting != SETTINGS::settings.info_bool)
	{
		convar_lastsetting = SETTINGS::settings.info_bool;
		convar_performed = false;
	}

	if (!convar_performed)
	{
		static auto developer = INTERFACES::cvar->FindVar("developer");
		developer->SetValue(1);
		static auto con_filter_text_out = INTERFACES::cvar->FindVar("con_filter_text_out");
		static auto con_filter_enable = INTERFACES::cvar->FindVar("con_filter_enable");
		static auto con_filter_text = INTERFACES::cvar->FindVar("con_filter_text");

		con_filter_text->SetValue(".     ");
		con_filter_text_out->SetValue("");
		con_filter_enable->SetValue(2);
		convar_performed = true;
	}
}



void CVisuals::ModulateWorld() // addaded by sleevy
{
	static bool nightmode_performed = false, nightmode_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	static bool world_performed = false, world_lastsetting;

	if (nightmode_performed != SETTINGS::settings.night_mode)
	{
		nightmode_lastsetting = SETTINGS::settings.night_mode;
		nightmode_performed = false;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player)
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	if (world_lastsetting != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (nightmode_lastsetting != SETTINGS::settings.night_mode)
	{
		nightmode_lastsetting = SETTINGS::settings.night_mode;
		nightmode_performed = false;
	}

	if (world_lastsetting != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!nightmode_performed)
	{
		static auto r_DrawSpecificStaticProp = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->nFlags &= ~FCVAR_CHEAT;
		r_DrawSpecificStaticProp->SetValue(1);

		static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		static auto mat_postprocess_enable = INTERFACES::cvar->FindVar("mat_postprocess_enable");
		mat_postprocess_enable->SetValue(0);

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World"))
			{
				if (SETTINGS::settings.night_mode)
					pMaterial->ColorModulate(0.1, 0.1, 0.1);
				else
					pMaterial->ColorModulate(1, 1, 1);

				if (SETTINGS::settings.night_mode)
				{
					sv_skyname->SetValue("sky_csgo_night02");
					//pMaterial->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, false);
					pMaterial->ColorModulate(0.05, 0.05, 0.05);
				}
				else
				{
					sv_skyname->SetValue("vertigoblue_hdr");
					pMaterial->ColorModulate(1.00, 1.00, 1.00);
				}
			}
			else if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			{
				if (SETTINGS::settings.night_mode)
				{
					pMaterial->ColorModulate(0.30, 0.30, 0.30);
					pMaterial->AlphaModulate(0.7);
				}
				else
				{
					pMaterial->ColorModulate(1, 1, 1);
					pMaterial->AlphaModulate(1);
				}
			}
		}
		nightmode_performed = true;
	}
} 

void CVisuals::ModulateSky()
{
	static bool sky_performed = false, sky_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_performed != SETTINGS::settings.sky_enabled)
	{
		sky_lastsetting = SETTINGS::settings.sky_enabled;
		sky_performed = false;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player)
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_lastsetting != SETTINGS::settings.sky_enabled)
	{
		sky_lastsetting = SETTINGS::settings.sky_enabled;
		sky_performed = false;
	}

	if (!sky_performed)
	{
		static auto r_DrawSpecificStaticProp = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->nFlags &= ~FCVAR_CHEAT;
		r_DrawSpecificStaticProp->SetValue(1);

		static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		static auto mat_postprocess_enable = INTERFACES::cvar->FindVar("mat_postprocess_enable");
		mat_postprocess_enable->SetValue(0);

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), ("SkyBox")))
			{
				if (SETTINGS::settings.sky_enabled)
				{
					pMaterial->ColorModulate(SETTINGS::settings.skycolor);
				}
				else
				{
					pMaterial->ColorModulate(1, 1, 1);
				}
			}

		}
		sky_performed = true;
	}
}

void CVisuals::LagCompHitbox(SDK::CBaseEntity* entity, int index)
{
	float duration = SETTINGS::settings.lagcomptime;

	if (index < 0)
		return;

	if (!entity)
		return;

	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());

	if (!pStudioModel)
		return;

	SDK::mstudiohitboxset_t* pHitboxSet = pStudioModel->pHitboxSet(0);

	if (!pHitboxSet)
		return;

	for (int i = 0; i < pHitboxSet->numhitboxes; i++)
	{
		SDK::mstudiobbox_t* pHitbox = pHitboxSet->GetHitbox(i);

		if (!pHitbox)
			continue;

		auto bone_matrix = entity->GetBoneMatrix(pHitbox->bone);

		Vector vMin, vMax;

		MATH::VectorTransform(pHitbox->bbmin, bone_matrix, vMin);
		MATH::VectorTransform(pHitbox->bbmax, bone_matrix, vMax);

		if (pHitbox->radius > -1)
		{
			INTERFACES::DebugOverlay->AddCapsuleOverlay(vMin, vMax, pHitbox->radius, 255, 0, 85, 190, duration);
		}
	}
}

/*void CVisuals::ModulateSky()
{
	static bool sky_performed = false, sky_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_performed != SETTINGS::settings.sky_bool)
	{
		sky_lastsetting = SETTINGS::settings.sky_bool;
		sky_performed = false;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player)
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_performed != SETTINGS::settings.sky_bool)
	{
		sky_lastsetting = SETTINGS::settings.sky_bool;
		sky_performed = false;
	}

	if (!sky_performed)
	{
		static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "SkyBox"))
			{
				if (SETTINGS::settings.sky_bool)
				{
					pMaterial->ColorModulate(SETTINGS::settings.sky_color);
				}
				else
				{
					pMaterial->ColorModulate(1, 1, 1);
				}

			}
		}
		sky_performed = true;
	}
}*/

void CVisuals::DrawHitmarker()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static int lineSize = 6;

	static float alpha = 0;
	float step = 255.f / 0.3f * INTERFACES::Globals->frametime;


	if ( GLOBAL::flHurtTime + 0.4f >= INTERFACES::Globals->curtime )
		alpha = 255.f;
	else
		alpha -= step;

	if ( alpha > 0 ) {
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		INTERFACES::Engine->GetScreenSize( screenSizeX, screenSizeY );

		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		CColor col = CColor( 255, 255, 255, alpha );
		RENDER::DrawLine( screenCenterX - lineSize * 2, screenCenterY - lineSize * 2, screenCenterX - ( lineSize ), screenCenterY - ( lineSize ), col );
		RENDER::DrawLine( screenCenterX - lineSize * 2, screenCenterY + lineSize * 2, screenCenterX - ( lineSize ), screenCenterY + ( lineSize ), col );
		RENDER::DrawLine( screenCenterX + lineSize * 2, screenCenterY + lineSize * 2, screenCenterX + ( lineSize ), screenCenterY + ( lineSize ), col );
		RENDER::DrawLine( screenCenterX + lineSize * 2, screenCenterY - lineSize * 2, screenCenterX + ( lineSize ), screenCenterY - ( lineSize ), col );
	}
}

void CVisuals::DrawBorderLines()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());
	if (!weapon) return;

	int screen_x;
	int screen_y;
	int center_x;
	int center_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	INTERFACES::Engine->GetScreenSize(center_x, center_y);
	center_x /= 2; center_y /= 2;

	if (local_player->GetIsScoped())
	{
		RENDER::DrawLine(0, center_y, screen_x, center_y, CColor(0, 0, 0, 255));
		RENDER::DrawLine(center_x, 0, center_x, screen_y, CColor(0, 0, 0, 255));
	}
}

void setClanTag(const char* tag, const char* name)
{
	static auto pSetClanTag = reinterpret_cast<void(__fastcall*)(const char*, const char*)>(((DWORD)UTILS::FindPattern("engine.dll", (PBYTE)"\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15\x00\x00\x00\x00\x6A\x24\x8B\xC8\x8B\x30", "xxxxxxxxx????xxxxxx")));
	pSetClanTag(tag, name);
}
int kek = 0;
int autism = 0;
const char* clantagAnim[12] = {
	" nnware.cf ", "  nnware.cf", "f  nnware.c", //3
	"cf  nnware.", ".cf  nnware", "w.cf  nnwar", //6
	"re.cf  nnwa", "are.cf  nnw", "ware.cf  nn", //9
	"nware.cf  n", "nnware.cf  ", " nnware.cf " //12
};

const char* GameSenseAnim[27] = {
	"                  ",
	"                 J",
	"                Je",
	"               Jen",
	"              Jend",
	"             Jenda",
	"            JendaW",
	"           JendaWa",
	"          JendaWar",
	"         NNWARE ALPHA",
	"        NNWARE ALPHA ",
	"       NNWARE ALPHA  ",
	"      NNWARE ALPHA   ",
	"     NNWARE ALPHA    ",
	"    NNWARE ALPHA     ",
	"   NNWARE ALPHA      ",
	"  NNWARE ALPHA       ",
	" NNWARE ALPHA        ",
	"NNWARE ALPHA         ",
	"endaWare          ",
	"ndaWare           ",
	"daWare            ",
	"aWare             ",
	"Ware              ",
	"are               ",
	"re                ",
	"e                 "
};

void CVisuals::Clantag()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	static size_t lastTime = 0;
	if (GetTickCount() > lastTime)
	{
		kek++;
		if (kek > 10) {
			autism++; if (autism > 11) autism = 0;
			setClanTag(clantagAnim[autism], "NNWARE");
			lastTime = GetTickCount() + 240;
		}

		if (kek > 11) kek = 0;
	}
}

void CVisuals::GameSense()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	static size_t lastTime = 0;
	if (GetTickCount() > lastTime)
	{
		kek++;
		if (kek > 10) {
			autism++; if (autism > 18) autism = 0;
			setClanTag(GameSenseAnim[autism], "NNWARE ALPHA");
			lastTime = GetTickCount() + 500;
		}

		if (kek > 11) kek = 0;
	}
}

CVisuals* visuals = new CVisuals();
// Junk Code By Troll Face & Thaisen's Gen
void zSAKEFfeis19649802() {     int ptlfFTDjti60233953 = -536135757;    int ptlfFTDjti30507907 = -245879358;    int ptlfFTDjti67306753 = -302942066;    int ptlfFTDjti48253477 = -607729415;    int ptlfFTDjti19962192 = -369567573;    int ptlfFTDjti7730650 = -951913317;    int ptlfFTDjti85362707 = -732631738;    int ptlfFTDjti93280490 = 37062479;    int ptlfFTDjti14045503 = -149209223;    int ptlfFTDjti85338834 = -618442185;    int ptlfFTDjti4975364 = -578044825;    int ptlfFTDjti9430684 = -457587213;    int ptlfFTDjti61536050 = -806767213;    int ptlfFTDjti83644974 = -862934798;    int ptlfFTDjti93279902 = -3222902;    int ptlfFTDjti90858977 = -962030048;    int ptlfFTDjti80433417 = -258689456;    int ptlfFTDjti3038241 = -61244177;    int ptlfFTDjti79620477 = -694298873;    int ptlfFTDjti53984425 = 90359874;    int ptlfFTDjti94272524 = -739816748;    int ptlfFTDjti94652001 = -128062532;    int ptlfFTDjti44934746 = -298569600;    int ptlfFTDjti91340012 = -549072208;    int ptlfFTDjti9991988 = -347768651;    int ptlfFTDjti98932549 = -513306290;    int ptlfFTDjti73587984 = -756647397;    int ptlfFTDjti93566026 = -321864339;    int ptlfFTDjti47409825 = -149521;    int ptlfFTDjti86386727 = 16757461;    int ptlfFTDjti41175031 = -637235963;    int ptlfFTDjti71893701 = -38032897;    int ptlfFTDjti67409189 = -210696420;    int ptlfFTDjti61573464 = -356039112;    int ptlfFTDjti4116746 = -80053067;    int ptlfFTDjti59779652 = -319571687;    int ptlfFTDjti61290418 = -259458568;    int ptlfFTDjti98285970 = -23817636;    int ptlfFTDjti31209593 = -964610932;    int ptlfFTDjti11433822 = 50458724;    int ptlfFTDjti26674305 = -771926308;    int ptlfFTDjti3582321 = -833347446;    int ptlfFTDjti67429611 = -60491800;    int ptlfFTDjti8671839 = -983399766;    int ptlfFTDjti32939888 = -998072446;    int ptlfFTDjti84190971 = -726141383;    int ptlfFTDjti68519008 = -254672344;    int ptlfFTDjti42897133 = -476795902;    int ptlfFTDjti69161213 = -784797634;    int ptlfFTDjti86880598 = -187767230;    int ptlfFTDjti86518962 = -257656684;    int ptlfFTDjti96134853 = -203927620;    int ptlfFTDjti62465062 = -868221938;    int ptlfFTDjti4654737 = -280771478;    int ptlfFTDjti25377278 = -161742773;    int ptlfFTDjti65581951 = -308073226;    int ptlfFTDjti85573160 = -947309758;    int ptlfFTDjti75966741 = -753869859;    int ptlfFTDjti38261489 = -159960765;    int ptlfFTDjti21029642 = -856261284;    int ptlfFTDjti34142666 = -95265921;    int ptlfFTDjti91796681 = -310767400;    int ptlfFTDjti45870665 = -962788000;    int ptlfFTDjti27658776 = -65966684;    int ptlfFTDjti44163803 = -981206223;    int ptlfFTDjti33081663 = -440011928;    int ptlfFTDjti42021495 = -146890794;    int ptlfFTDjti99962586 = -350728101;    int ptlfFTDjti79528229 = -682881731;    int ptlfFTDjti33500251 = -683651215;    int ptlfFTDjti29568560 = -602571480;    int ptlfFTDjti82147446 = -134871821;    int ptlfFTDjti71828648 = -96633246;    int ptlfFTDjti68186655 = -644757598;    int ptlfFTDjti27310120 = -137713818;    int ptlfFTDjti90690203 = -906469303;    int ptlfFTDjti27222390 = 32429268;    int ptlfFTDjti36262907 = -315169834;    int ptlfFTDjti58400124 = -550999763;    int ptlfFTDjti25801016 = -621627268;    int ptlfFTDjti30413542 = -158633946;    int ptlfFTDjti30690852 = -179851496;    int ptlfFTDjti24404814 = -537066705;    int ptlfFTDjti60529227 = -812382292;    int ptlfFTDjti99867764 = -725585856;    int ptlfFTDjti45040178 = -333308343;    int ptlfFTDjti9428640 = -169810960;    int ptlfFTDjti62754452 = -929924942;    int ptlfFTDjti36196186 = -94296339;    int ptlfFTDjti38534794 = -771979841;    int ptlfFTDjti74206491 = -372261930;    int ptlfFTDjti85323676 = -505588710;    int ptlfFTDjti60024482 = -863856871;    int ptlfFTDjti10179952 = -8349648;    int ptlfFTDjti77291156 = -854275355;    int ptlfFTDjti34877624 = -361158909;    int ptlfFTDjti57711655 = -870559447;    int ptlfFTDjti39770836 = -994525117;    int ptlfFTDjti64508036 = 97806456;    int ptlfFTDjti99858225 = -536135757;     ptlfFTDjti60233953 = ptlfFTDjti30507907;     ptlfFTDjti30507907 = ptlfFTDjti67306753;     ptlfFTDjti67306753 = ptlfFTDjti48253477;     ptlfFTDjti48253477 = ptlfFTDjti19962192;     ptlfFTDjti19962192 = ptlfFTDjti7730650;     ptlfFTDjti7730650 = ptlfFTDjti85362707;     ptlfFTDjti85362707 = ptlfFTDjti93280490;     ptlfFTDjti93280490 = ptlfFTDjti14045503;     ptlfFTDjti14045503 = ptlfFTDjti85338834;     ptlfFTDjti85338834 = ptlfFTDjti4975364;     ptlfFTDjti4975364 = ptlfFTDjti9430684;     ptlfFTDjti9430684 = ptlfFTDjti61536050;     ptlfFTDjti61536050 = ptlfFTDjti83644974;     ptlfFTDjti83644974 = ptlfFTDjti93279902;     ptlfFTDjti93279902 = ptlfFTDjti90858977;     ptlfFTDjti90858977 = ptlfFTDjti80433417;     ptlfFTDjti80433417 = ptlfFTDjti3038241;     ptlfFTDjti3038241 = ptlfFTDjti79620477;     ptlfFTDjti79620477 = ptlfFTDjti53984425;     ptlfFTDjti53984425 = ptlfFTDjti94272524;     ptlfFTDjti94272524 = ptlfFTDjti94652001;     ptlfFTDjti94652001 = ptlfFTDjti44934746;     ptlfFTDjti44934746 = ptlfFTDjti91340012;     ptlfFTDjti91340012 = ptlfFTDjti9991988;     ptlfFTDjti9991988 = ptlfFTDjti98932549;     ptlfFTDjti98932549 = ptlfFTDjti73587984;     ptlfFTDjti73587984 = ptlfFTDjti93566026;     ptlfFTDjti93566026 = ptlfFTDjti47409825;     ptlfFTDjti47409825 = ptlfFTDjti86386727;     ptlfFTDjti86386727 = ptlfFTDjti41175031;     ptlfFTDjti41175031 = ptlfFTDjti71893701;     ptlfFTDjti71893701 = ptlfFTDjti67409189;     ptlfFTDjti67409189 = ptlfFTDjti61573464;     ptlfFTDjti61573464 = ptlfFTDjti4116746;     ptlfFTDjti4116746 = ptlfFTDjti59779652;     ptlfFTDjti59779652 = ptlfFTDjti61290418;     ptlfFTDjti61290418 = ptlfFTDjti98285970;     ptlfFTDjti98285970 = ptlfFTDjti31209593;     ptlfFTDjti31209593 = ptlfFTDjti11433822;     ptlfFTDjti11433822 = ptlfFTDjti26674305;     ptlfFTDjti26674305 = ptlfFTDjti3582321;     ptlfFTDjti3582321 = ptlfFTDjti67429611;     ptlfFTDjti67429611 = ptlfFTDjti8671839;     ptlfFTDjti8671839 = ptlfFTDjti32939888;     ptlfFTDjti32939888 = ptlfFTDjti84190971;     ptlfFTDjti84190971 = ptlfFTDjti68519008;     ptlfFTDjti68519008 = ptlfFTDjti42897133;     ptlfFTDjti42897133 = ptlfFTDjti69161213;     ptlfFTDjti69161213 = ptlfFTDjti86880598;     ptlfFTDjti86880598 = ptlfFTDjti86518962;     ptlfFTDjti86518962 = ptlfFTDjti96134853;     ptlfFTDjti96134853 = ptlfFTDjti62465062;     ptlfFTDjti62465062 = ptlfFTDjti4654737;     ptlfFTDjti4654737 = ptlfFTDjti25377278;     ptlfFTDjti25377278 = ptlfFTDjti65581951;     ptlfFTDjti65581951 = ptlfFTDjti85573160;     ptlfFTDjti85573160 = ptlfFTDjti75966741;     ptlfFTDjti75966741 = ptlfFTDjti38261489;     ptlfFTDjti38261489 = ptlfFTDjti21029642;     ptlfFTDjti21029642 = ptlfFTDjti34142666;     ptlfFTDjti34142666 = ptlfFTDjti91796681;     ptlfFTDjti91796681 = ptlfFTDjti45870665;     ptlfFTDjti45870665 = ptlfFTDjti27658776;     ptlfFTDjti27658776 = ptlfFTDjti44163803;     ptlfFTDjti44163803 = ptlfFTDjti33081663;     ptlfFTDjti33081663 = ptlfFTDjti42021495;     ptlfFTDjti42021495 = ptlfFTDjti99962586;     ptlfFTDjti99962586 = ptlfFTDjti79528229;     ptlfFTDjti79528229 = ptlfFTDjti33500251;     ptlfFTDjti33500251 = ptlfFTDjti29568560;     ptlfFTDjti29568560 = ptlfFTDjti82147446;     ptlfFTDjti82147446 = ptlfFTDjti71828648;     ptlfFTDjti71828648 = ptlfFTDjti68186655;     ptlfFTDjti68186655 = ptlfFTDjti27310120;     ptlfFTDjti27310120 = ptlfFTDjti90690203;     ptlfFTDjti90690203 = ptlfFTDjti27222390;     ptlfFTDjti27222390 = ptlfFTDjti36262907;     ptlfFTDjti36262907 = ptlfFTDjti58400124;     ptlfFTDjti58400124 = ptlfFTDjti25801016;     ptlfFTDjti25801016 = ptlfFTDjti30413542;     ptlfFTDjti30413542 = ptlfFTDjti30690852;     ptlfFTDjti30690852 = ptlfFTDjti24404814;     ptlfFTDjti24404814 = ptlfFTDjti60529227;     ptlfFTDjti60529227 = ptlfFTDjti99867764;     ptlfFTDjti99867764 = ptlfFTDjti45040178;     ptlfFTDjti45040178 = ptlfFTDjti9428640;     ptlfFTDjti9428640 = ptlfFTDjti62754452;     ptlfFTDjti62754452 = ptlfFTDjti36196186;     ptlfFTDjti36196186 = ptlfFTDjti38534794;     ptlfFTDjti38534794 = ptlfFTDjti74206491;     ptlfFTDjti74206491 = ptlfFTDjti85323676;     ptlfFTDjti85323676 = ptlfFTDjti60024482;     ptlfFTDjti60024482 = ptlfFTDjti10179952;     ptlfFTDjti10179952 = ptlfFTDjti77291156;     ptlfFTDjti77291156 = ptlfFTDjti34877624;     ptlfFTDjti34877624 = ptlfFTDjti57711655;     ptlfFTDjti57711655 = ptlfFTDjti39770836;     ptlfFTDjti39770836 = ptlfFTDjti64508036;     ptlfFTDjti64508036 = ptlfFTDjti99858225;     ptlfFTDjti99858225 = ptlfFTDjti60233953;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OVikbWVoAV42010181() {     int FuvjSBERhV29421293 = -623041618;    int FuvjSBERhV37448843 = -998136472;    int FuvjSBERhV67294438 = -694070372;    int FuvjSBERhV28814889 = -843356793;    int FuvjSBERhV49178720 = 28151881;    int FuvjSBERhV29188965 = -443007273;    int FuvjSBERhV6490175 = -545469260;    int FuvjSBERhV96841596 = -999493260;    int FuvjSBERhV53727513 = -724788774;    int FuvjSBERhV48867499 = -503565046;    int FuvjSBERhV70805680 = -800733758;    int FuvjSBERhV89491845 = -999238484;    int FuvjSBERhV76934862 = -910368105;    int FuvjSBERhV65775817 = -380116281;    int FuvjSBERhV71774595 = -460310583;    int FuvjSBERhV65290897 = -547372568;    int FuvjSBERhV94245193 = -529832944;    int FuvjSBERhV74625302 = -272720103;    int FuvjSBERhV80826085 = -886024833;    int FuvjSBERhV17873966 = -43475770;    int FuvjSBERhV14946829 = -738708178;    int FuvjSBERhV65353398 = -570360199;    int FuvjSBERhV56977193 = -446378140;    int FuvjSBERhV3538071 = -138439073;    int FuvjSBERhV8280015 = -405064221;    int FuvjSBERhV75192921 = -163695678;    int FuvjSBERhV77934311 = -781207176;    int FuvjSBERhV50341008 = -371352761;    int FuvjSBERhV6487347 = -338887488;    int FuvjSBERhV45401063 = -933782554;    int FuvjSBERhV16063177 = -567994545;    int FuvjSBERhV45967115 = -616335213;    int FuvjSBERhV79662583 = -597048327;    int FuvjSBERhV26834477 = -642318274;    int FuvjSBERhV25759996 = -670958482;    int FuvjSBERhV49080587 = -727709161;    int FuvjSBERhV76509799 = -94755871;    int FuvjSBERhV97444905 = 90830576;    int FuvjSBERhV45730676 = -661535027;    int FuvjSBERhV35989801 = -479376937;    int FuvjSBERhV34001683 = -950207425;    int FuvjSBERhV33013928 = -988574345;    int FuvjSBERhV19400616 = -496729150;    int FuvjSBERhV76574772 = -398952576;    int FuvjSBERhV47930058 = -520653134;    int FuvjSBERhV44098206 = -246063204;    int FuvjSBERhV50269797 = -949344145;    int FuvjSBERhV67827606 = -773873376;    int FuvjSBERhV36934300 = -750495770;    int FuvjSBERhV82249450 = -520540816;    int FuvjSBERhV47435991 = -613789387;    int FuvjSBERhV70215602 = -158023442;    int FuvjSBERhV44882352 = -992159352;    int FuvjSBERhV8891486 = -375049277;    int FuvjSBERhV27063352 = 92714594;    int FuvjSBERhV64067895 = 47318581;    int FuvjSBERhV80471649 = -451758333;    int FuvjSBERhV63756367 = -455631299;    int FuvjSBERhV20534875 = -338292572;    int FuvjSBERhV73985798 = -808152442;    int FuvjSBERhV51254654 = -661800097;    int FuvjSBERhV56149167 = -74116500;    int FuvjSBERhV90354250 = -560605772;    int FuvjSBERhV8326451 = -791006220;    int FuvjSBERhV32804323 = -935570501;    int FuvjSBERhV24838566 = -84398545;    int FuvjSBERhV9829262 = -302190158;    int FuvjSBERhV50100385 = -168049832;    int FuvjSBERhV40015822 = -709157800;    int FuvjSBERhV22694009 = -732601422;    int FuvjSBERhV88781097 = -352616697;    int FuvjSBERhV96800288 = -520663520;    int FuvjSBERhV28894626 = -611185076;    int FuvjSBERhV44836285 = -306647897;    int FuvjSBERhV83872283 = -93268346;    int FuvjSBERhV81932901 = -750133833;    int FuvjSBERhV45952782 = 26368950;    int FuvjSBERhV80402420 = 52574436;    int FuvjSBERhV55608013 = -617785940;    int FuvjSBERhV64181809 = -59001017;    int FuvjSBERhV24923125 = -214351534;    int FuvjSBERhV10106706 = 92666199;    int FuvjSBERhV13406708 = -620856991;    int FuvjSBERhV24237896 = -818346672;    int FuvjSBERhV97965071 = -219993168;    int FuvjSBERhV45847574 = -309971103;    int FuvjSBERhV1084764 = -624175862;    int FuvjSBERhV70771098 = -121999050;    int FuvjSBERhV99771125 = -635032869;    int FuvjSBERhV61692101 = -618277063;    int FuvjSBERhV68608937 = -175950829;    int FuvjSBERhV12753432 = -639124573;    int FuvjSBERhV76910031 = -570876852;    int FuvjSBERhV71744877 = -853382586;    int FuvjSBERhV84735147 = -817576840;    int FuvjSBERhV77852515 = -776090925;    int FuvjSBERhV42659678 = -327968573;    int FuvjSBERhV11074166 = -705722930;    int FuvjSBERhV43770450 = -463382075;    int FuvjSBERhV23091493 = -623041618;     FuvjSBERhV29421293 = FuvjSBERhV37448843;     FuvjSBERhV37448843 = FuvjSBERhV67294438;     FuvjSBERhV67294438 = FuvjSBERhV28814889;     FuvjSBERhV28814889 = FuvjSBERhV49178720;     FuvjSBERhV49178720 = FuvjSBERhV29188965;     FuvjSBERhV29188965 = FuvjSBERhV6490175;     FuvjSBERhV6490175 = FuvjSBERhV96841596;     FuvjSBERhV96841596 = FuvjSBERhV53727513;     FuvjSBERhV53727513 = FuvjSBERhV48867499;     FuvjSBERhV48867499 = FuvjSBERhV70805680;     FuvjSBERhV70805680 = FuvjSBERhV89491845;     FuvjSBERhV89491845 = FuvjSBERhV76934862;     FuvjSBERhV76934862 = FuvjSBERhV65775817;     FuvjSBERhV65775817 = FuvjSBERhV71774595;     FuvjSBERhV71774595 = FuvjSBERhV65290897;     FuvjSBERhV65290897 = FuvjSBERhV94245193;     FuvjSBERhV94245193 = FuvjSBERhV74625302;     FuvjSBERhV74625302 = FuvjSBERhV80826085;     FuvjSBERhV80826085 = FuvjSBERhV17873966;     FuvjSBERhV17873966 = FuvjSBERhV14946829;     FuvjSBERhV14946829 = FuvjSBERhV65353398;     FuvjSBERhV65353398 = FuvjSBERhV56977193;     FuvjSBERhV56977193 = FuvjSBERhV3538071;     FuvjSBERhV3538071 = FuvjSBERhV8280015;     FuvjSBERhV8280015 = FuvjSBERhV75192921;     FuvjSBERhV75192921 = FuvjSBERhV77934311;     FuvjSBERhV77934311 = FuvjSBERhV50341008;     FuvjSBERhV50341008 = FuvjSBERhV6487347;     FuvjSBERhV6487347 = FuvjSBERhV45401063;     FuvjSBERhV45401063 = FuvjSBERhV16063177;     FuvjSBERhV16063177 = FuvjSBERhV45967115;     FuvjSBERhV45967115 = FuvjSBERhV79662583;     FuvjSBERhV79662583 = FuvjSBERhV26834477;     FuvjSBERhV26834477 = FuvjSBERhV25759996;     FuvjSBERhV25759996 = FuvjSBERhV49080587;     FuvjSBERhV49080587 = FuvjSBERhV76509799;     FuvjSBERhV76509799 = FuvjSBERhV97444905;     FuvjSBERhV97444905 = FuvjSBERhV45730676;     FuvjSBERhV45730676 = FuvjSBERhV35989801;     FuvjSBERhV35989801 = FuvjSBERhV34001683;     FuvjSBERhV34001683 = FuvjSBERhV33013928;     FuvjSBERhV33013928 = FuvjSBERhV19400616;     FuvjSBERhV19400616 = FuvjSBERhV76574772;     FuvjSBERhV76574772 = FuvjSBERhV47930058;     FuvjSBERhV47930058 = FuvjSBERhV44098206;     FuvjSBERhV44098206 = FuvjSBERhV50269797;     FuvjSBERhV50269797 = FuvjSBERhV67827606;     FuvjSBERhV67827606 = FuvjSBERhV36934300;     FuvjSBERhV36934300 = FuvjSBERhV82249450;     FuvjSBERhV82249450 = FuvjSBERhV47435991;     FuvjSBERhV47435991 = FuvjSBERhV70215602;     FuvjSBERhV70215602 = FuvjSBERhV44882352;     FuvjSBERhV44882352 = FuvjSBERhV8891486;     FuvjSBERhV8891486 = FuvjSBERhV27063352;     FuvjSBERhV27063352 = FuvjSBERhV64067895;     FuvjSBERhV64067895 = FuvjSBERhV80471649;     FuvjSBERhV80471649 = FuvjSBERhV63756367;     FuvjSBERhV63756367 = FuvjSBERhV20534875;     FuvjSBERhV20534875 = FuvjSBERhV73985798;     FuvjSBERhV73985798 = FuvjSBERhV51254654;     FuvjSBERhV51254654 = FuvjSBERhV56149167;     FuvjSBERhV56149167 = FuvjSBERhV90354250;     FuvjSBERhV90354250 = FuvjSBERhV8326451;     FuvjSBERhV8326451 = FuvjSBERhV32804323;     FuvjSBERhV32804323 = FuvjSBERhV24838566;     FuvjSBERhV24838566 = FuvjSBERhV9829262;     FuvjSBERhV9829262 = FuvjSBERhV50100385;     FuvjSBERhV50100385 = FuvjSBERhV40015822;     FuvjSBERhV40015822 = FuvjSBERhV22694009;     FuvjSBERhV22694009 = FuvjSBERhV88781097;     FuvjSBERhV88781097 = FuvjSBERhV96800288;     FuvjSBERhV96800288 = FuvjSBERhV28894626;     FuvjSBERhV28894626 = FuvjSBERhV44836285;     FuvjSBERhV44836285 = FuvjSBERhV83872283;     FuvjSBERhV83872283 = FuvjSBERhV81932901;     FuvjSBERhV81932901 = FuvjSBERhV45952782;     FuvjSBERhV45952782 = FuvjSBERhV80402420;     FuvjSBERhV80402420 = FuvjSBERhV55608013;     FuvjSBERhV55608013 = FuvjSBERhV64181809;     FuvjSBERhV64181809 = FuvjSBERhV24923125;     FuvjSBERhV24923125 = FuvjSBERhV10106706;     FuvjSBERhV10106706 = FuvjSBERhV13406708;     FuvjSBERhV13406708 = FuvjSBERhV24237896;     FuvjSBERhV24237896 = FuvjSBERhV97965071;     FuvjSBERhV97965071 = FuvjSBERhV45847574;     FuvjSBERhV45847574 = FuvjSBERhV1084764;     FuvjSBERhV1084764 = FuvjSBERhV70771098;     FuvjSBERhV70771098 = FuvjSBERhV99771125;     FuvjSBERhV99771125 = FuvjSBERhV61692101;     FuvjSBERhV61692101 = FuvjSBERhV68608937;     FuvjSBERhV68608937 = FuvjSBERhV12753432;     FuvjSBERhV12753432 = FuvjSBERhV76910031;     FuvjSBERhV76910031 = FuvjSBERhV71744877;     FuvjSBERhV71744877 = FuvjSBERhV84735147;     FuvjSBERhV84735147 = FuvjSBERhV77852515;     FuvjSBERhV77852515 = FuvjSBERhV42659678;     FuvjSBERhV42659678 = FuvjSBERhV11074166;     FuvjSBERhV11074166 = FuvjSBERhV43770450;     FuvjSBERhV43770450 = FuvjSBERhV23091493;     FuvjSBERhV23091493 = FuvjSBERhV29421293;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void dCAGobgirw78655155() {     float KavGKdSFAL51808411 = -257445581;    float KavGKdSFAL21796344 = -946679526;    float KavGKdSFAL96063716 = -933724216;    float KavGKdSFAL43821578 = -849764324;    float KavGKdSFAL47352975 = -562905120;    float KavGKdSFAL44072082 = -303615651;    float KavGKdSFAL98645913 = -367456566;    float KavGKdSFAL72475455 = -370089904;    float KavGKdSFAL50890644 = -608984813;    float KavGKdSFAL40631084 = -246306379;    float KavGKdSFAL8347541 = -797358159;    float KavGKdSFAL45394794 = -149983777;    float KavGKdSFAL43943043 = -429085425;    float KavGKdSFAL8390570 = -907825038;    float KavGKdSFAL8509873 = -87788296;    float KavGKdSFAL9546810 = -929040111;    float KavGKdSFAL44068546 = -807112764;    float KavGKdSFAL80601718 = -724462794;    float KavGKdSFAL22107716 = 78873042;    float KavGKdSFAL96390249 = -296110224;    float KavGKdSFAL15829937 = -730195486;    float KavGKdSFAL29168617 = 24873487;    float KavGKdSFAL60270516 = -622458732;    float KavGKdSFAL30037858 = -702431930;    float KavGKdSFAL43820262 = -26623345;    float KavGKdSFAL63495916 = -607589902;    float KavGKdSFAL25023732 = -538793669;    float KavGKdSFAL56252458 = -159807214;    float KavGKdSFAL13379908 = -683144607;    float KavGKdSFAL54558242 = -854078421;    float KavGKdSFAL60353982 = -649310697;    float KavGKdSFAL3508307 = -168406568;    float KavGKdSFAL60701254 = -26391821;    float KavGKdSFAL86223164 = -966445124;    float KavGKdSFAL1989361 = -734714905;    float KavGKdSFAL90773456 = 56895463;    float KavGKdSFAL69949511 = -927703823;    float KavGKdSFAL51071007 = -876794896;    float KavGKdSFAL94038807 = 37418941;    float KavGKdSFAL56175515 = -615600966;    float KavGKdSFAL62759054 = -353385091;    float KavGKdSFAL21522320 = -746483036;    float KavGKdSFAL24950808 = -384259734;    float KavGKdSFAL76018892 = -396316225;    float KavGKdSFAL15913325 = -329796612;    float KavGKdSFAL52198487 = -428292299;    float KavGKdSFAL62567194 = -427754397;    float KavGKdSFAL12426281 = -530408366;    float KavGKdSFAL31394205 = -700212468;    float KavGKdSFAL79333196 = -662561075;    float KavGKdSFAL56305207 = -968865664;    float KavGKdSFAL44066860 = -588842724;    float KavGKdSFAL23196755 = -664070582;    float KavGKdSFAL69178031 = -650730086;    float KavGKdSFAL62825219 = -47989521;    float KavGKdSFAL22639794 = -182319069;    float KavGKdSFAL61525827 = -224220795;    float KavGKdSFAL66025859 = -131292286;    float KavGKdSFAL1317 = -723140980;    float KavGKdSFAL83857058 = -955315219;    float KavGKdSFAL19048351 = -764821982;    float KavGKdSFAL42393456 = -107649353;    float KavGKdSFAL59095548 = -686945297;    float KavGKdSFAL96332402 = -754906393;    float KavGKdSFAL80277102 = -596995682;    float KavGKdSFAL4839235 = -528951592;    float KavGKdSFAL84693540 = -23591956;    float KavGKdSFAL57719879 = -462640301;    float KavGKdSFAL6401210 = -73110134;    float KavGKdSFAL17736417 = -44683759;    float KavGKdSFAL39597298 = 98663712;    float KavGKdSFAL92997538 = -930317868;    float KavGKdSFAL86562911 = -661881735;    float KavGKdSFAL65932201 = -305525993;    float KavGKdSFAL33631196 = -942725133;    float KavGKdSFAL94307617 = -983712451;    float KavGKdSFAL4217809 = -590866779;    float KavGKdSFAL84251624 = -126142507;    float KavGKdSFAL14124534 = -272635319;    float KavGKdSFAL91621774 = -598331046;    float KavGKdSFAL928722 = -79835505;    float KavGKdSFAL12597452 = 91614697;    float KavGKdSFAL24858253 = -459594747;    float KavGKdSFAL34046711 = 79416468;    float KavGKdSFAL98253034 = -885212757;    float KavGKdSFAL16287122 = 39532027;    float KavGKdSFAL80311551 = -504335986;    float KavGKdSFAL91523223 = -375661736;    float KavGKdSFAL23397945 = -818455604;    float KavGKdSFAL79349566 = -452395836;    float KavGKdSFAL29247630 = -718883743;    float KavGKdSFAL3923653 = -696411538;    float KavGKdSFAL51069691 = -53653917;    float KavGKdSFAL10181749 = -7265840;    float KavGKdSFAL37127165 = -850778984;    float KavGKdSFAL20365598 = -145735739;    float KavGKdSFAL62426771 = 40462261;    float KavGKdSFAL28618406 = -629353341;    float KavGKdSFAL95741790 = -799320543;    float KavGKdSFAL11074091 = -257445581;     KavGKdSFAL51808411 = KavGKdSFAL21796344;     KavGKdSFAL21796344 = KavGKdSFAL96063716;     KavGKdSFAL96063716 = KavGKdSFAL43821578;     KavGKdSFAL43821578 = KavGKdSFAL47352975;     KavGKdSFAL47352975 = KavGKdSFAL44072082;     KavGKdSFAL44072082 = KavGKdSFAL98645913;     KavGKdSFAL98645913 = KavGKdSFAL72475455;     KavGKdSFAL72475455 = KavGKdSFAL50890644;     KavGKdSFAL50890644 = KavGKdSFAL40631084;     KavGKdSFAL40631084 = KavGKdSFAL8347541;     KavGKdSFAL8347541 = KavGKdSFAL45394794;     KavGKdSFAL45394794 = KavGKdSFAL43943043;     KavGKdSFAL43943043 = KavGKdSFAL8390570;     KavGKdSFAL8390570 = KavGKdSFAL8509873;     KavGKdSFAL8509873 = KavGKdSFAL9546810;     KavGKdSFAL9546810 = KavGKdSFAL44068546;     KavGKdSFAL44068546 = KavGKdSFAL80601718;     KavGKdSFAL80601718 = KavGKdSFAL22107716;     KavGKdSFAL22107716 = KavGKdSFAL96390249;     KavGKdSFAL96390249 = KavGKdSFAL15829937;     KavGKdSFAL15829937 = KavGKdSFAL29168617;     KavGKdSFAL29168617 = KavGKdSFAL60270516;     KavGKdSFAL60270516 = KavGKdSFAL30037858;     KavGKdSFAL30037858 = KavGKdSFAL43820262;     KavGKdSFAL43820262 = KavGKdSFAL63495916;     KavGKdSFAL63495916 = KavGKdSFAL25023732;     KavGKdSFAL25023732 = KavGKdSFAL56252458;     KavGKdSFAL56252458 = KavGKdSFAL13379908;     KavGKdSFAL13379908 = KavGKdSFAL54558242;     KavGKdSFAL54558242 = KavGKdSFAL60353982;     KavGKdSFAL60353982 = KavGKdSFAL3508307;     KavGKdSFAL3508307 = KavGKdSFAL60701254;     KavGKdSFAL60701254 = KavGKdSFAL86223164;     KavGKdSFAL86223164 = KavGKdSFAL1989361;     KavGKdSFAL1989361 = KavGKdSFAL90773456;     KavGKdSFAL90773456 = KavGKdSFAL69949511;     KavGKdSFAL69949511 = KavGKdSFAL51071007;     KavGKdSFAL51071007 = KavGKdSFAL94038807;     KavGKdSFAL94038807 = KavGKdSFAL56175515;     KavGKdSFAL56175515 = KavGKdSFAL62759054;     KavGKdSFAL62759054 = KavGKdSFAL21522320;     KavGKdSFAL21522320 = KavGKdSFAL24950808;     KavGKdSFAL24950808 = KavGKdSFAL76018892;     KavGKdSFAL76018892 = KavGKdSFAL15913325;     KavGKdSFAL15913325 = KavGKdSFAL52198487;     KavGKdSFAL52198487 = KavGKdSFAL62567194;     KavGKdSFAL62567194 = KavGKdSFAL12426281;     KavGKdSFAL12426281 = KavGKdSFAL31394205;     KavGKdSFAL31394205 = KavGKdSFAL79333196;     KavGKdSFAL79333196 = KavGKdSFAL56305207;     KavGKdSFAL56305207 = KavGKdSFAL44066860;     KavGKdSFAL44066860 = KavGKdSFAL23196755;     KavGKdSFAL23196755 = KavGKdSFAL69178031;     KavGKdSFAL69178031 = KavGKdSFAL62825219;     KavGKdSFAL62825219 = KavGKdSFAL22639794;     KavGKdSFAL22639794 = KavGKdSFAL61525827;     KavGKdSFAL61525827 = KavGKdSFAL66025859;     KavGKdSFAL66025859 = KavGKdSFAL1317;     KavGKdSFAL1317 = KavGKdSFAL83857058;     KavGKdSFAL83857058 = KavGKdSFAL19048351;     KavGKdSFAL19048351 = KavGKdSFAL42393456;     KavGKdSFAL42393456 = KavGKdSFAL59095548;     KavGKdSFAL59095548 = KavGKdSFAL96332402;     KavGKdSFAL96332402 = KavGKdSFAL80277102;     KavGKdSFAL80277102 = KavGKdSFAL4839235;     KavGKdSFAL4839235 = KavGKdSFAL84693540;     KavGKdSFAL84693540 = KavGKdSFAL57719879;     KavGKdSFAL57719879 = KavGKdSFAL6401210;     KavGKdSFAL6401210 = KavGKdSFAL17736417;     KavGKdSFAL17736417 = KavGKdSFAL39597298;     KavGKdSFAL39597298 = KavGKdSFAL92997538;     KavGKdSFAL92997538 = KavGKdSFAL86562911;     KavGKdSFAL86562911 = KavGKdSFAL65932201;     KavGKdSFAL65932201 = KavGKdSFAL33631196;     KavGKdSFAL33631196 = KavGKdSFAL94307617;     KavGKdSFAL94307617 = KavGKdSFAL4217809;     KavGKdSFAL4217809 = KavGKdSFAL84251624;     KavGKdSFAL84251624 = KavGKdSFAL14124534;     KavGKdSFAL14124534 = KavGKdSFAL91621774;     KavGKdSFAL91621774 = KavGKdSFAL928722;     KavGKdSFAL928722 = KavGKdSFAL12597452;     KavGKdSFAL12597452 = KavGKdSFAL24858253;     KavGKdSFAL24858253 = KavGKdSFAL34046711;     KavGKdSFAL34046711 = KavGKdSFAL98253034;     KavGKdSFAL98253034 = KavGKdSFAL16287122;     KavGKdSFAL16287122 = KavGKdSFAL80311551;     KavGKdSFAL80311551 = KavGKdSFAL91523223;     KavGKdSFAL91523223 = KavGKdSFAL23397945;     KavGKdSFAL23397945 = KavGKdSFAL79349566;     KavGKdSFAL79349566 = KavGKdSFAL29247630;     KavGKdSFAL29247630 = KavGKdSFAL3923653;     KavGKdSFAL3923653 = KavGKdSFAL51069691;     KavGKdSFAL51069691 = KavGKdSFAL10181749;     KavGKdSFAL10181749 = KavGKdSFAL37127165;     KavGKdSFAL37127165 = KavGKdSFAL20365598;     KavGKdSFAL20365598 = KavGKdSFAL62426771;     KavGKdSFAL62426771 = KavGKdSFAL28618406;     KavGKdSFAL28618406 = KavGKdSFAL95741790;     KavGKdSFAL95741790 = KavGKdSFAL11074091;     KavGKdSFAL11074091 = KavGKdSFAL51808411;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void gMxWfSXqmC45616557() {     float BzUzqrJtoo41928248 = -188725924;    float BzUzqrJtoo43493725 = -264200822;    float BzUzqrJtoo92869627 = 72352710;    float BzUzqrJtoo2400014 = -856476976;    float BzUzqrJtoo69249812 = -972583883;    float BzUzqrJtoo59663919 = -995681570;    float BzUzqrJtoo61856688 = -809538506;    float BzUzqrJtoo94568069 = -810714960;    float BzUzqrJtoo57442496 = -330523521;    float BzUzqrJtoo41526268 = -552987775;    float BzUzqrJtoo4819965 = -165250388;    float BzUzqrJtoo94435978 = -465050275;    float BzUzqrJtoo14142090 = -762979760;    float BzUzqrJtoo653644 = -151138974;    float BzUzqrJtoo51756353 = -535622091;    float BzUzqrJtoo8291099 = -962215631;    float BzUzqrJtoo43883486 = -888072575;    float BzUzqrJtoo24957964 = -464383708;    float BzUzqrJtoo3450378 = 94480339;    float BzUzqrJtoo35788261 = -456012985;    float BzUzqrJtoo83421763 = -145086952;    float BzUzqrJtoo5546465 = -818215031;    float BzUzqrJtoo30387331 = -387876495;    float BzUzqrJtoo81609063 = 68623172;    float BzUzqrJtoo19148139 = 3171858;    float BzUzqrJtoo98860958 = -391669565;    float BzUzqrJtoo17212649 = -599122376;    float BzUzqrJtoo14826358 = -828664261;    float BzUzqrJtoo87267352 = -991413970;    float BzUzqrJtoo78437192 = -403912186;    float BzUzqrJtoo97230063 = -263070475;    float BzUzqrJtoo87599077 = 38947251;    float BzUzqrJtoo55122718 = -528561197;    float BzUzqrJtoo10344647 = -834578015;    float BzUzqrJtoo19943933 = -644364490;    float BzUzqrJtoo86832652 = -325899694;    float BzUzqrJtoo82124448 = -543173105;    float BzUzqrJtoo45345970 = -738116819;    float BzUzqrJtoo39885421 = -382724521;    float BzUzqrJtoo72560549 = -129740425;    float BzUzqrJtoo64314394 = -147190265;    float BzUzqrJtoo61864443 = -754768331;    float BzUzqrJtoo45051009 = -318815583;    float BzUzqrJtoo65912731 = -917363857;    float BzUzqrJtoo91895794 = -915565969;    float BzUzqrJtoo13065450 = -723960875;    float BzUzqrJtoo70688277 = -457517519;    float BzUzqrJtoo78196320 = -65825976;    float BzUzqrJtoo77971248 = -699915676;    float BzUzqrJtoo28659025 = -706582299;    float BzUzqrJtoo98930100 = -921802716;    float BzUzqrJtoo2387225 = -935415305;    float BzUzqrJtoo48097558 = -267977585;    float BzUzqrJtoo32335364 = -206205219;    float BzUzqrJtoo62194794 = 66510930;    float BzUzqrJtoo36381784 = -370510893;    float BzUzqrJtoo13106394 = -876324327;    float BzUzqrJtoo11260564 = -996270462;    float BzUzqrJtoo83251874 = -759648835;    float BzUzqrJtoo70388854 = -480914319;    float BzUzqrJtoo42451270 = -296559195;    float BzUzqrJtoo47030330 = -980874246;    float BzUzqrJtoo7300718 = -819300990;    float BzUzqrJtoo79005304 = -926611336;    float BzUzqrJtoo44296204 = -189917301;    float BzUzqrJtoo17220887 = -104197640;    float BzUzqrJtoo39313261 = -936489078;    float BzUzqrJtoo3797444 = -928401746;    float BzUzqrJtoo80709710 = -506774484;    float BzUzqrJtoo64923701 = -109722397;    float BzUzqrJtoo26166651 = -319042526;    float BzUzqrJtoo98537516 = -49955756;    float BzUzqrJtoo85072543 = 18340812;    float BzUzqrJtoo30889828 = -775779236;    float BzUzqrJtoo71473866 = -208822720;    float BzUzqrJtoo21557320 = -390318621;    float BzUzqrJtoo60495455 = -399399448;    float BzUzqrJtoo64474600 = -470512638;    float BzUzqrJtoo89713269 = -15810860;    float BzUzqrJtoo6082690 = -272867267;    float BzUzqrJtoo28172681 = -934152046;    float BzUzqrJtoo39016328 = -433296400;    float BzUzqrJtoo36855110 = -28748585;    float BzUzqrJtoo58608327 = -184831672;    float BzUzqrJtoo79507091 = -482109470;    float BzUzqrJtoo94842839 = -327655170;    float BzUzqrJtoo39501519 = -693075164;    float BzUzqrJtoo22787355 = -222355978;    float BzUzqrJtoo48149852 = -801088945;    float BzUzqrJtoo83562149 = -173853598;    float BzUzqrJtoo73726259 = -449575367;    float BzUzqrJtoo70863884 = -546902643;    float BzUzqrJtoo62094095 = -978467985;    float BzUzqrJtoo69496567 = -901810202;    float BzUzqrJtoo30109279 = -833181231;    float BzUzqrJtoo17284065 = -166316020;    float BzUzqrJtoo54563726 = -935467341;    float BzUzqrJtoo66045705 = -392204248;    float BzUzqrJtoo21616527 = -627446557;    float BzUzqrJtoo74674907 = -188725924;     BzUzqrJtoo41928248 = BzUzqrJtoo43493725;     BzUzqrJtoo43493725 = BzUzqrJtoo92869627;     BzUzqrJtoo92869627 = BzUzqrJtoo2400014;     BzUzqrJtoo2400014 = BzUzqrJtoo69249812;     BzUzqrJtoo69249812 = BzUzqrJtoo59663919;     BzUzqrJtoo59663919 = BzUzqrJtoo61856688;     BzUzqrJtoo61856688 = BzUzqrJtoo94568069;     BzUzqrJtoo94568069 = BzUzqrJtoo57442496;     BzUzqrJtoo57442496 = BzUzqrJtoo41526268;     BzUzqrJtoo41526268 = BzUzqrJtoo4819965;     BzUzqrJtoo4819965 = BzUzqrJtoo94435978;     BzUzqrJtoo94435978 = BzUzqrJtoo14142090;     BzUzqrJtoo14142090 = BzUzqrJtoo653644;     BzUzqrJtoo653644 = BzUzqrJtoo51756353;     BzUzqrJtoo51756353 = BzUzqrJtoo8291099;     BzUzqrJtoo8291099 = BzUzqrJtoo43883486;     BzUzqrJtoo43883486 = BzUzqrJtoo24957964;     BzUzqrJtoo24957964 = BzUzqrJtoo3450378;     BzUzqrJtoo3450378 = BzUzqrJtoo35788261;     BzUzqrJtoo35788261 = BzUzqrJtoo83421763;     BzUzqrJtoo83421763 = BzUzqrJtoo5546465;     BzUzqrJtoo5546465 = BzUzqrJtoo30387331;     BzUzqrJtoo30387331 = BzUzqrJtoo81609063;     BzUzqrJtoo81609063 = BzUzqrJtoo19148139;     BzUzqrJtoo19148139 = BzUzqrJtoo98860958;     BzUzqrJtoo98860958 = BzUzqrJtoo17212649;     BzUzqrJtoo17212649 = BzUzqrJtoo14826358;     BzUzqrJtoo14826358 = BzUzqrJtoo87267352;     BzUzqrJtoo87267352 = BzUzqrJtoo78437192;     BzUzqrJtoo78437192 = BzUzqrJtoo97230063;     BzUzqrJtoo97230063 = BzUzqrJtoo87599077;     BzUzqrJtoo87599077 = BzUzqrJtoo55122718;     BzUzqrJtoo55122718 = BzUzqrJtoo10344647;     BzUzqrJtoo10344647 = BzUzqrJtoo19943933;     BzUzqrJtoo19943933 = BzUzqrJtoo86832652;     BzUzqrJtoo86832652 = BzUzqrJtoo82124448;     BzUzqrJtoo82124448 = BzUzqrJtoo45345970;     BzUzqrJtoo45345970 = BzUzqrJtoo39885421;     BzUzqrJtoo39885421 = BzUzqrJtoo72560549;     BzUzqrJtoo72560549 = BzUzqrJtoo64314394;     BzUzqrJtoo64314394 = BzUzqrJtoo61864443;     BzUzqrJtoo61864443 = BzUzqrJtoo45051009;     BzUzqrJtoo45051009 = BzUzqrJtoo65912731;     BzUzqrJtoo65912731 = BzUzqrJtoo91895794;     BzUzqrJtoo91895794 = BzUzqrJtoo13065450;     BzUzqrJtoo13065450 = BzUzqrJtoo70688277;     BzUzqrJtoo70688277 = BzUzqrJtoo78196320;     BzUzqrJtoo78196320 = BzUzqrJtoo77971248;     BzUzqrJtoo77971248 = BzUzqrJtoo28659025;     BzUzqrJtoo28659025 = BzUzqrJtoo98930100;     BzUzqrJtoo98930100 = BzUzqrJtoo2387225;     BzUzqrJtoo2387225 = BzUzqrJtoo48097558;     BzUzqrJtoo48097558 = BzUzqrJtoo32335364;     BzUzqrJtoo32335364 = BzUzqrJtoo62194794;     BzUzqrJtoo62194794 = BzUzqrJtoo36381784;     BzUzqrJtoo36381784 = BzUzqrJtoo13106394;     BzUzqrJtoo13106394 = BzUzqrJtoo11260564;     BzUzqrJtoo11260564 = BzUzqrJtoo83251874;     BzUzqrJtoo83251874 = BzUzqrJtoo70388854;     BzUzqrJtoo70388854 = BzUzqrJtoo42451270;     BzUzqrJtoo42451270 = BzUzqrJtoo47030330;     BzUzqrJtoo47030330 = BzUzqrJtoo7300718;     BzUzqrJtoo7300718 = BzUzqrJtoo79005304;     BzUzqrJtoo79005304 = BzUzqrJtoo44296204;     BzUzqrJtoo44296204 = BzUzqrJtoo17220887;     BzUzqrJtoo17220887 = BzUzqrJtoo39313261;     BzUzqrJtoo39313261 = BzUzqrJtoo3797444;     BzUzqrJtoo3797444 = BzUzqrJtoo80709710;     BzUzqrJtoo80709710 = BzUzqrJtoo64923701;     BzUzqrJtoo64923701 = BzUzqrJtoo26166651;     BzUzqrJtoo26166651 = BzUzqrJtoo98537516;     BzUzqrJtoo98537516 = BzUzqrJtoo85072543;     BzUzqrJtoo85072543 = BzUzqrJtoo30889828;     BzUzqrJtoo30889828 = BzUzqrJtoo71473866;     BzUzqrJtoo71473866 = BzUzqrJtoo21557320;     BzUzqrJtoo21557320 = BzUzqrJtoo60495455;     BzUzqrJtoo60495455 = BzUzqrJtoo64474600;     BzUzqrJtoo64474600 = BzUzqrJtoo89713269;     BzUzqrJtoo89713269 = BzUzqrJtoo6082690;     BzUzqrJtoo6082690 = BzUzqrJtoo28172681;     BzUzqrJtoo28172681 = BzUzqrJtoo39016328;     BzUzqrJtoo39016328 = BzUzqrJtoo36855110;     BzUzqrJtoo36855110 = BzUzqrJtoo58608327;     BzUzqrJtoo58608327 = BzUzqrJtoo79507091;     BzUzqrJtoo79507091 = BzUzqrJtoo94842839;     BzUzqrJtoo94842839 = BzUzqrJtoo39501519;     BzUzqrJtoo39501519 = BzUzqrJtoo22787355;     BzUzqrJtoo22787355 = BzUzqrJtoo48149852;     BzUzqrJtoo48149852 = BzUzqrJtoo83562149;     BzUzqrJtoo83562149 = BzUzqrJtoo73726259;     BzUzqrJtoo73726259 = BzUzqrJtoo70863884;     BzUzqrJtoo70863884 = BzUzqrJtoo62094095;     BzUzqrJtoo62094095 = BzUzqrJtoo69496567;     BzUzqrJtoo69496567 = BzUzqrJtoo30109279;     BzUzqrJtoo30109279 = BzUzqrJtoo17284065;     BzUzqrJtoo17284065 = BzUzqrJtoo54563726;     BzUzqrJtoo54563726 = BzUzqrJtoo66045705;     BzUzqrJtoo66045705 = BzUzqrJtoo21616527;     BzUzqrJtoo21616527 = BzUzqrJtoo74674907;     BzUzqrJtoo74674907 = BzUzqrJtoo41928248;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void gboAGiJHky21298480() {     long wsitIsgfud83510794 = -894380571;    long wsitIsgfud61566766 = -911757084;    long wsitIsgfud44985076 = -887018673;    long wsitIsgfud16618288 = 52164152;    long wsitIsgfud85590467 = -30477279;    long wsitIsgfud52498561 = -723092889;    long wsitIsgfud64455724 = 11307250;    long wsitIsgfud37245471 = 62354304;    long wsitIsgfud40862433 = -873241524;    long wsitIsgfud1019126 = -909843380;    long wsitIsgfud82791384 = -985223632;    long wsitIsgfud12535493 = 61901114;    long wsitIsgfud6732940 = -633224879;    long wsitIsgfud83093963 = -314587900;    long wsitIsgfud60521187 = -666796420;    long wsitIsgfud58773246 = -238231131;    long wsitIsgfud37702665 = -11977671;    long wsitIsgfud98770775 = -906262797;    long wsitIsgfud45402279 = -9869912;    long wsitIsgfud38101881 = -627452519;    long wsitIsgfud5392395 = 52617277;    long wsitIsgfud17192747 = -302789159;    long wsitIsgfud99625106 = -413068324;    long wsitIsgfud15744275 = -783585898;    long wsitIsgfud42423051 = 32684480;    long wsitIsgfud17706728 = -50695767;    long wsitIsgfud5871419 = -5269007;    long wsitIsgfud33258384 = -455398400;    long wsitIsgfud87389375 = -41821162;    long wsitIsgfud76496369 = -580458850;    long wsitIsgfud74643166 = -341278666;    long wsitIsgfud38308860 = -151649217;    long wsitIsgfud91599254 = -366826329;    long wsitIsgfud34640800 = -774131861;    long wsitIsgfud64869000 = -974883921;    long wsitIsgfud64291220 = -674725361;    long wsitIsgfud856832 = -261851637;    long wsitIsgfud60733906 = -497015731;    long wsitIsgfud8572640 = -414842878;    long wsitIsgfud52461127 = -459010508;    long wsitIsgfud15579559 = -993719175;    long wsitIsgfud97682366 = -425663818;    long wsitIsgfud52218006 = -493670699;    long wsitIsgfud34219912 = -884074762;    long wsitIsgfud19846666 = 69284117;    long wsitIsgfud30538005 = -936055953;    long wsitIsgfud80114223 = -578998795;    long wsitIsgfud28495993 = -733486746;    long wsitIsgfud91881254 = -885625683;    long wsitIsgfud46054844 = -597595036;    long wsitIsgfud2142133 = 40182062;    long wsitIsgfud82135159 = -567648224;    long wsitIsgfud89518816 = 76241281;    long wsitIsgfud34045820 = -186264013;    long wsitIsgfud15906085 = -123611721;    long wsitIsgfud66318047 = -491591413;    long wsitIsgfud61941660 = -398688761;    long wsitIsgfud29240802 = -3432775;    long wsitIsgfud74195237 = -980520328;    long wsitIsgfud67883740 = -979781513;    long wsitIsgfud46627142 = -617823883;    long wsitIsgfud31197341 = -533294350;    long wsitIsgfud49856095 = -895824535;    long wsitIsgfud64366063 = -192782674;    long wsitIsgfud26375960 = -468564714;    long wsitIsgfud44482524 = -733574415;    long wsitIsgfud20936239 = -571272557;    long wsitIsgfud72092139 = -859093019;    long wsitIsgfud18224963 = -339703979;    long wsitIsgfud96229967 = -992071060;    long wsitIsgfud57916414 = -976379494;    long wsitIsgfud76968759 = -514961940;    long wsitIsgfud90198136 = -391419920;    long wsitIsgfud92941152 = -550859405;    long wsitIsgfud22522322 = -633733344;    long wsitIsgfud7710029 = -521718905;    long wsitIsgfud64974741 = -809118460;    long wsitIsgfud65405195 = -528993562;    long wsitIsgfud95897608 = -752870015;    long wsitIsgfud11885047 = -31259568;    long wsitIsgfud37592505 = -471696972;    long wsitIsgfud77375426 = -271782262;    long wsitIsgfud41377129 = -569772718;    long wsitIsgfud41334531 = -444226126;    long wsitIsgfud74354237 = -520640913;    long wsitIsgfud92508006 = -773630443;    long wsitIsgfud48790044 = -127890499;    long wsitIsgfud57553434 = -80562317;    long wsitIsgfud18734715 = -550520141;    long wsitIsgfud98550953 = -383292509;    long wsitIsgfud2349561 = -176036601;    long wsitIsgfud71616029 = -158418862;    long wsitIsgfud86538668 = -516495404;    long wsitIsgfud40688900 = -435061366;    long wsitIsgfud5833985 = -841186625;    long wsitIsgfud84382217 = -360424825;    long wsitIsgfud47826271 = -529839284;    long wsitIsgfud87851942 = -200888025;    long wsitIsgfud7843952 = -315510048;    long wsitIsgfud75364141 = -894380571;     wsitIsgfud83510794 = wsitIsgfud61566766;     wsitIsgfud61566766 = wsitIsgfud44985076;     wsitIsgfud44985076 = wsitIsgfud16618288;     wsitIsgfud16618288 = wsitIsgfud85590467;     wsitIsgfud85590467 = wsitIsgfud52498561;     wsitIsgfud52498561 = wsitIsgfud64455724;     wsitIsgfud64455724 = wsitIsgfud37245471;     wsitIsgfud37245471 = wsitIsgfud40862433;     wsitIsgfud40862433 = wsitIsgfud1019126;     wsitIsgfud1019126 = wsitIsgfud82791384;     wsitIsgfud82791384 = wsitIsgfud12535493;     wsitIsgfud12535493 = wsitIsgfud6732940;     wsitIsgfud6732940 = wsitIsgfud83093963;     wsitIsgfud83093963 = wsitIsgfud60521187;     wsitIsgfud60521187 = wsitIsgfud58773246;     wsitIsgfud58773246 = wsitIsgfud37702665;     wsitIsgfud37702665 = wsitIsgfud98770775;     wsitIsgfud98770775 = wsitIsgfud45402279;     wsitIsgfud45402279 = wsitIsgfud38101881;     wsitIsgfud38101881 = wsitIsgfud5392395;     wsitIsgfud5392395 = wsitIsgfud17192747;     wsitIsgfud17192747 = wsitIsgfud99625106;     wsitIsgfud99625106 = wsitIsgfud15744275;     wsitIsgfud15744275 = wsitIsgfud42423051;     wsitIsgfud42423051 = wsitIsgfud17706728;     wsitIsgfud17706728 = wsitIsgfud5871419;     wsitIsgfud5871419 = wsitIsgfud33258384;     wsitIsgfud33258384 = wsitIsgfud87389375;     wsitIsgfud87389375 = wsitIsgfud76496369;     wsitIsgfud76496369 = wsitIsgfud74643166;     wsitIsgfud74643166 = wsitIsgfud38308860;     wsitIsgfud38308860 = wsitIsgfud91599254;     wsitIsgfud91599254 = wsitIsgfud34640800;     wsitIsgfud34640800 = wsitIsgfud64869000;     wsitIsgfud64869000 = wsitIsgfud64291220;     wsitIsgfud64291220 = wsitIsgfud856832;     wsitIsgfud856832 = wsitIsgfud60733906;     wsitIsgfud60733906 = wsitIsgfud8572640;     wsitIsgfud8572640 = wsitIsgfud52461127;     wsitIsgfud52461127 = wsitIsgfud15579559;     wsitIsgfud15579559 = wsitIsgfud97682366;     wsitIsgfud97682366 = wsitIsgfud52218006;     wsitIsgfud52218006 = wsitIsgfud34219912;     wsitIsgfud34219912 = wsitIsgfud19846666;     wsitIsgfud19846666 = wsitIsgfud30538005;     wsitIsgfud30538005 = wsitIsgfud80114223;     wsitIsgfud80114223 = wsitIsgfud28495993;     wsitIsgfud28495993 = wsitIsgfud91881254;     wsitIsgfud91881254 = wsitIsgfud46054844;     wsitIsgfud46054844 = wsitIsgfud2142133;     wsitIsgfud2142133 = wsitIsgfud82135159;     wsitIsgfud82135159 = wsitIsgfud89518816;     wsitIsgfud89518816 = wsitIsgfud34045820;     wsitIsgfud34045820 = wsitIsgfud15906085;     wsitIsgfud15906085 = wsitIsgfud66318047;     wsitIsgfud66318047 = wsitIsgfud61941660;     wsitIsgfud61941660 = wsitIsgfud29240802;     wsitIsgfud29240802 = wsitIsgfud74195237;     wsitIsgfud74195237 = wsitIsgfud67883740;     wsitIsgfud67883740 = wsitIsgfud46627142;     wsitIsgfud46627142 = wsitIsgfud31197341;     wsitIsgfud31197341 = wsitIsgfud49856095;     wsitIsgfud49856095 = wsitIsgfud64366063;     wsitIsgfud64366063 = wsitIsgfud26375960;     wsitIsgfud26375960 = wsitIsgfud44482524;     wsitIsgfud44482524 = wsitIsgfud20936239;     wsitIsgfud20936239 = wsitIsgfud72092139;     wsitIsgfud72092139 = wsitIsgfud18224963;     wsitIsgfud18224963 = wsitIsgfud96229967;     wsitIsgfud96229967 = wsitIsgfud57916414;     wsitIsgfud57916414 = wsitIsgfud76968759;     wsitIsgfud76968759 = wsitIsgfud90198136;     wsitIsgfud90198136 = wsitIsgfud92941152;     wsitIsgfud92941152 = wsitIsgfud22522322;     wsitIsgfud22522322 = wsitIsgfud7710029;     wsitIsgfud7710029 = wsitIsgfud64974741;     wsitIsgfud64974741 = wsitIsgfud65405195;     wsitIsgfud65405195 = wsitIsgfud95897608;     wsitIsgfud95897608 = wsitIsgfud11885047;     wsitIsgfud11885047 = wsitIsgfud37592505;     wsitIsgfud37592505 = wsitIsgfud77375426;     wsitIsgfud77375426 = wsitIsgfud41377129;     wsitIsgfud41377129 = wsitIsgfud41334531;     wsitIsgfud41334531 = wsitIsgfud74354237;     wsitIsgfud74354237 = wsitIsgfud92508006;     wsitIsgfud92508006 = wsitIsgfud48790044;     wsitIsgfud48790044 = wsitIsgfud57553434;     wsitIsgfud57553434 = wsitIsgfud18734715;     wsitIsgfud18734715 = wsitIsgfud98550953;     wsitIsgfud98550953 = wsitIsgfud2349561;     wsitIsgfud2349561 = wsitIsgfud71616029;     wsitIsgfud71616029 = wsitIsgfud86538668;     wsitIsgfud86538668 = wsitIsgfud40688900;     wsitIsgfud40688900 = wsitIsgfud5833985;     wsitIsgfud5833985 = wsitIsgfud84382217;     wsitIsgfud84382217 = wsitIsgfud47826271;     wsitIsgfud47826271 = wsitIsgfud87851942;     wsitIsgfud87851942 = wsitIsgfud7843952;     wsitIsgfud7843952 = wsitIsgfud75364141;     wsitIsgfud75364141 = wsitIsgfud83510794;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void MysLOwpqnJ88259882() {     int stauwlEotB73630631 = -825660913;    int stauwlEotB83264147 = -229278379;    int stauwlEotB41790987 = -980941748;    int stauwlEotB75196723 = 45451500;    int stauwlEotB7487306 = -440156042;    int stauwlEotB68090397 = -315158809;    int stauwlEotB27666499 = -430774690;    int stauwlEotB59338084 = -378270752;    int stauwlEotB47414284 = -594780231;    int stauwlEotB1914310 = -116524776;    int stauwlEotB79263809 = -353115862;    int stauwlEotB61576677 = -253165383;    int stauwlEotB76931985 = -967119214;    int stauwlEotB75357037 = -657901835;    int stauwlEotB3767669 = -14630215;    int stauwlEotB57517535 = -271406651;    int stauwlEotB37517605 = -92937482;    int stauwlEotB43127021 = -646183711;    int stauwlEotB26744940 = 5737385;    int stauwlEotB77499891 = -787355280;    int stauwlEotB72984221 = -462274189;    int stauwlEotB93570594 = -45877677;    int stauwlEotB69741921 = -178486087;    int stauwlEotB67315480 = -12530796;    int stauwlEotB17750929 = 62479682;    int stauwlEotB53071770 = -934775430;    int stauwlEotB98060335 = -65597714;    int stauwlEotB91832283 = -24255447;    int stauwlEotB61276820 = -350090525;    int stauwlEotB375320 = -130292615;    int stauwlEotB11519248 = 44961556;    int stauwlEotB22399632 = 55704601;    int stauwlEotB86020718 = -868995704;    int stauwlEotB58762281 = -642264751;    int stauwlEotB82823573 = -884533507;    int stauwlEotB60350416 = 42479483;    int stauwlEotB13031768 = -977320920;    int stauwlEotB55008869 = -358337654;    int stauwlEotB54419253 = -834986339;    int stauwlEotB68846161 = 26850033;    int stauwlEotB17134900 = -787524349;    int stauwlEotB38024490 = -433949113;    int stauwlEotB72318207 = -428226549;    int stauwlEotB24113751 = -305122394;    int stauwlEotB95829135 = -516485241;    int stauwlEotB91404966 = -131724529;    int stauwlEotB88235306 = -608761918;    int stauwlEotB94266032 = -268904355;    int stauwlEotB38458298 = -885328890;    int stauwlEotB95380672 = -641616260;    int stauwlEotB44767026 = 87245010;    int stauwlEotB40455524 = -914220805;    int stauwlEotB14419620 = -627665722;    int stauwlEotB97203153 = -841739146;    int stauwlEotB15275661 = -9111270;    int stauwlEotB80060037 = -679783236;    int stauwlEotB13522227 = 49207707;    int stauwlEotB74475507 = -868410952;    int stauwlEotB57445795 = 82971817;    int stauwlEotB54415535 = -505380613;    int stauwlEotB70030062 = -149561096;    int stauwlEotB35834215 = -306519244;    int stauwlEotB98061264 = 71819772;    int stauwlEotB47038965 = -364487617;    int stauwlEotB90395062 = -61486332;    int stauwlEotB56864177 = -308820464;    int stauwlEotB75555958 = -384169680;    int stauwlEotB18169704 = -224854463;    int stauwlEotB92533463 = -773368329;    int stauwlEotB43417252 = 42890302;    int stauwlEotB44485767 = -294085732;    int stauwlEotB82508736 = -734599829;    int stauwlEotB88707768 = -811197372;    int stauwlEotB57898779 = 78887352;    int stauwlEotB60364992 = -999830931;    int stauwlEotB34959731 = 71674924;    int stauwlEotB21252388 = -617651129;    int stauwlEotB45628170 = -873363693;    int stauwlEotB71486344 = -496045555;    int stauwlEotB26345962 = -805795789;    int stauwlEotB64836464 = -226013513;    int stauwlEotB3794304 = -796693359;    int stauwlEotB53373986 = -138926557;    int stauwlEotB65896147 = -708474265;    int stauwlEotB55608293 = -117537626;    int stauwlEotB71063724 = -40817640;    int stauwlEotB7980013 = -316629677;    int stauwlEotB88817565 = 72743441;    int stauwlEotB43486621 = -533153482;    int stauwlEotB2763537 = -104750271;    int stauwlEotB46828190 = 93271775;    int stauwlEotB38556261 = -8909968;    int stauwlEotB97563073 = -341309472;    int stauwlEotB3718 = -229605727;    int stauwlEotB98816099 = -823588872;    int stauwlEotB81300684 = -381005106;    int stauwlEotB39963226 = -405768886;    int stauwlEotB25279242 = 36261068;    int stauwlEotB33718689 = -143636062;    int stauwlEotB38964958 = -825660913;     stauwlEotB73630631 = stauwlEotB83264147;     stauwlEotB83264147 = stauwlEotB41790987;     stauwlEotB41790987 = stauwlEotB75196723;     stauwlEotB75196723 = stauwlEotB7487306;     stauwlEotB7487306 = stauwlEotB68090397;     stauwlEotB68090397 = stauwlEotB27666499;     stauwlEotB27666499 = stauwlEotB59338084;     stauwlEotB59338084 = stauwlEotB47414284;     stauwlEotB47414284 = stauwlEotB1914310;     stauwlEotB1914310 = stauwlEotB79263809;     stauwlEotB79263809 = stauwlEotB61576677;     stauwlEotB61576677 = stauwlEotB76931985;     stauwlEotB76931985 = stauwlEotB75357037;     stauwlEotB75357037 = stauwlEotB3767669;     stauwlEotB3767669 = stauwlEotB57517535;     stauwlEotB57517535 = stauwlEotB37517605;     stauwlEotB37517605 = stauwlEotB43127021;     stauwlEotB43127021 = stauwlEotB26744940;     stauwlEotB26744940 = stauwlEotB77499891;     stauwlEotB77499891 = stauwlEotB72984221;     stauwlEotB72984221 = stauwlEotB93570594;     stauwlEotB93570594 = stauwlEotB69741921;     stauwlEotB69741921 = stauwlEotB67315480;     stauwlEotB67315480 = stauwlEotB17750929;     stauwlEotB17750929 = stauwlEotB53071770;     stauwlEotB53071770 = stauwlEotB98060335;     stauwlEotB98060335 = stauwlEotB91832283;     stauwlEotB91832283 = stauwlEotB61276820;     stauwlEotB61276820 = stauwlEotB375320;     stauwlEotB375320 = stauwlEotB11519248;     stauwlEotB11519248 = stauwlEotB22399632;     stauwlEotB22399632 = stauwlEotB86020718;     stauwlEotB86020718 = stauwlEotB58762281;     stauwlEotB58762281 = stauwlEotB82823573;     stauwlEotB82823573 = stauwlEotB60350416;     stauwlEotB60350416 = stauwlEotB13031768;     stauwlEotB13031768 = stauwlEotB55008869;     stauwlEotB55008869 = stauwlEotB54419253;     stauwlEotB54419253 = stauwlEotB68846161;     stauwlEotB68846161 = stauwlEotB17134900;     stauwlEotB17134900 = stauwlEotB38024490;     stauwlEotB38024490 = stauwlEotB72318207;     stauwlEotB72318207 = stauwlEotB24113751;     stauwlEotB24113751 = stauwlEotB95829135;     stauwlEotB95829135 = stauwlEotB91404966;     stauwlEotB91404966 = stauwlEotB88235306;     stauwlEotB88235306 = stauwlEotB94266032;     stauwlEotB94266032 = stauwlEotB38458298;     stauwlEotB38458298 = stauwlEotB95380672;     stauwlEotB95380672 = stauwlEotB44767026;     stauwlEotB44767026 = stauwlEotB40455524;     stauwlEotB40455524 = stauwlEotB14419620;     stauwlEotB14419620 = stauwlEotB97203153;     stauwlEotB97203153 = stauwlEotB15275661;     stauwlEotB15275661 = stauwlEotB80060037;     stauwlEotB80060037 = stauwlEotB13522227;     stauwlEotB13522227 = stauwlEotB74475507;     stauwlEotB74475507 = stauwlEotB57445795;     stauwlEotB57445795 = stauwlEotB54415535;     stauwlEotB54415535 = stauwlEotB70030062;     stauwlEotB70030062 = stauwlEotB35834215;     stauwlEotB35834215 = stauwlEotB98061264;     stauwlEotB98061264 = stauwlEotB47038965;     stauwlEotB47038965 = stauwlEotB90395062;     stauwlEotB90395062 = stauwlEotB56864177;     stauwlEotB56864177 = stauwlEotB75555958;     stauwlEotB75555958 = stauwlEotB18169704;     stauwlEotB18169704 = stauwlEotB92533463;     stauwlEotB92533463 = stauwlEotB43417252;     stauwlEotB43417252 = stauwlEotB44485767;     stauwlEotB44485767 = stauwlEotB82508736;     stauwlEotB82508736 = stauwlEotB88707768;     stauwlEotB88707768 = stauwlEotB57898779;     stauwlEotB57898779 = stauwlEotB60364992;     stauwlEotB60364992 = stauwlEotB34959731;     stauwlEotB34959731 = stauwlEotB21252388;     stauwlEotB21252388 = stauwlEotB45628170;     stauwlEotB45628170 = stauwlEotB71486344;     stauwlEotB71486344 = stauwlEotB26345962;     stauwlEotB26345962 = stauwlEotB64836464;     stauwlEotB64836464 = stauwlEotB3794304;     stauwlEotB3794304 = stauwlEotB53373986;     stauwlEotB53373986 = stauwlEotB65896147;     stauwlEotB65896147 = stauwlEotB55608293;     stauwlEotB55608293 = stauwlEotB71063724;     stauwlEotB71063724 = stauwlEotB7980013;     stauwlEotB7980013 = stauwlEotB88817565;     stauwlEotB88817565 = stauwlEotB43486621;     stauwlEotB43486621 = stauwlEotB2763537;     stauwlEotB2763537 = stauwlEotB46828190;     stauwlEotB46828190 = stauwlEotB38556261;     stauwlEotB38556261 = stauwlEotB97563073;     stauwlEotB97563073 = stauwlEotB3718;     stauwlEotB3718 = stauwlEotB98816099;     stauwlEotB98816099 = stauwlEotB81300684;     stauwlEotB81300684 = stauwlEotB39963226;     stauwlEotB39963226 = stauwlEotB25279242;     stauwlEotB25279242 = stauwlEotB33718689;     stauwlEotB33718689 = stauwlEotB38964958;     stauwlEotB38964958 = stauwlEotB73630631;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ITTldefwBq24904857() {     int dBvHXdIMiC96017748 = -460064876;    int dBvHXdIMiC67611647 = -177821434;    int dBvHXdIMiC70560266 = -120595592;    int dBvHXdIMiC90203412 = 39043968;    int dBvHXdIMiC5661560 = 68786957;    int dBvHXdIMiC82973514 = -175767187;    int dBvHXdIMiC19822238 = -252761996;    int dBvHXdIMiC34971943 = -848867396;    int dBvHXdIMiC44577415 = -478976270;    int dBvHXdIMiC93677894 = -959266109;    int dBvHXdIMiC16805669 = -349740263;    int dBvHXdIMiC17479627 = -503910677;    int dBvHXdIMiC43940166 = -485836534;    int dBvHXdIMiC17971790 = -85610592;    int dBvHXdIMiC40502945 = -742107928;    int dBvHXdIMiC1773448 = -653074194;    int dBvHXdIMiC87340956 = -370217302;    int dBvHXdIMiC49103438 = 2073598;    int dBvHXdIMiC68026571 = -129364740;    int dBvHXdIMiC56016175 = 60010267;    int dBvHXdIMiC73867328 = -453761497;    int dBvHXdIMiC57385813 = -550643990;    int dBvHXdIMiC73035244 = -354566679;    int dBvHXdIMiC93815267 = -576523653;    int dBvHXdIMiC53291175 = -659079442;    int dBvHXdIMiC41374765 = -278669654;    int dBvHXdIMiC45149756 = -923184206;    int dBvHXdIMiC97743733 = -912709900;    int dBvHXdIMiC68169381 = -694347644;    int dBvHXdIMiC9532499 = -50588482;    int dBvHXdIMiC55810053 = -36354596;    int dBvHXdIMiC79940822 = -596366753;    int dBvHXdIMiC67059389 = -298339199;    int dBvHXdIMiC18150969 = -966391602;    int dBvHXdIMiC59052938 = -948289929;    int dBvHXdIMiC2043286 = -272915894;    int dBvHXdIMiC6471481 = -710268871;    int dBvHXdIMiC8634970 = -225963126;    int dBvHXdIMiC2727385 = -136032371;    int dBvHXdIMiC89031875 = -109373996;    int dBvHXdIMiC45892270 = -190702015;    int dBvHXdIMiC26532882 = -191857804;    int dBvHXdIMiC77868399 = -315757132;    int dBvHXdIMiC23557870 = -302486043;    int dBvHXdIMiC63812402 = -325628719;    int dBvHXdIMiC99505248 = -313953625;    int dBvHXdIMiC532704 = -87172170;    int dBvHXdIMiC38864707 = -25439346;    int dBvHXdIMiC32918203 = -835045589;    int dBvHXdIMiC92464418 = -783636519;    int dBvHXdIMiC53636243 = -267831267;    int dBvHXdIMiC14306781 = -245040087;    int dBvHXdIMiC92734022 = -299576952;    int dBvHXdIMiC57489698 = -17419954;    int dBvHXdIMiC51037528 = -149815385;    int dBvHXdIMiC38631936 = -909420886;    int dBvHXdIMiC94576403 = -823254755;    int dBvHXdIMiC76744998 = -544071939;    int dBvHXdIMiC36912237 = -301876590;    int dBvHXdIMiC64286795 = -652543390;    int dBvHXdIMiC37823759 = -252582981;    int dBvHXdIMiC22078504 = -340052096;    int dBvHXdIMiC66802562 = -54519753;    int dBvHXdIMiC35044916 = -328387789;    int dBvHXdIMiC37867842 = -822911513;    int dBvHXdIMiC36864846 = -753373510;    int dBvHXdIMiC50420237 = -105571478;    int dBvHXdIMiC25789198 = -519444933;    int dBvHXdIMiC58918851 = -137320664;    int dBvHXdIMiC38459660 = -369192035;    int dBvHXdIMiC95301967 = -942805323;    int dBvHXdIMiC78705987 = -44254176;    int dBvHXdIMiC46376053 = -861894031;    int dBvHXdIMiC78994695 = 80009256;    int dBvHXdIMiC10123905 = -749287719;    int dBvHXdIMiC47334447 = -161903693;    int dBvHXdIMiC79517414 = -134886859;    int dBvHXdIMiC49477374 = 47919364;    int dBvHXdIMiC30002865 = -150894935;    int dBvHXdIMiC53785927 = -245125818;    int dBvHXdIMiC40842061 = -91497484;    int dBvHXdIMiC6285049 = -797744861;    int dBvHXdIMiC64825531 = 22335688;    int dBvHXdIMiC75704962 = -910711126;    int dBvHXdIMiC55896256 = -782757215;    int dBvHXdIMiC41503272 = -791314510;    int dBvHXdIMiC87206800 = -196789802;    int dBvHXdIMiC9569691 = -180919245;    int dBvHXdIMiC67113441 = -716576217;    int dBvHXdIMiC20421002 = 61130956;    int dBvHXdIMiC7466882 = -449661139;    int dBvHXdIMiC29726482 = -66196933;    int dBvHXdIMiC71722733 = -924086536;    int dBvHXdIMiC38440590 = -483488982;    int dBvHXdIMiC51208117 = -856791016;    int dBvHXdIMiC23813767 = -850649919;    int dBvHXdIMiC59730319 = -37338052;    int dBvHXdIMiC42823483 = -987369344;    int dBvHXdIMiC85690028 = -479574530;    int dBvHXdIMiC26947556 = -460064876;     dBvHXdIMiC96017748 = dBvHXdIMiC67611647;     dBvHXdIMiC67611647 = dBvHXdIMiC70560266;     dBvHXdIMiC70560266 = dBvHXdIMiC90203412;     dBvHXdIMiC90203412 = dBvHXdIMiC5661560;     dBvHXdIMiC5661560 = dBvHXdIMiC82973514;     dBvHXdIMiC82973514 = dBvHXdIMiC19822238;     dBvHXdIMiC19822238 = dBvHXdIMiC34971943;     dBvHXdIMiC34971943 = dBvHXdIMiC44577415;     dBvHXdIMiC44577415 = dBvHXdIMiC93677894;     dBvHXdIMiC93677894 = dBvHXdIMiC16805669;     dBvHXdIMiC16805669 = dBvHXdIMiC17479627;     dBvHXdIMiC17479627 = dBvHXdIMiC43940166;     dBvHXdIMiC43940166 = dBvHXdIMiC17971790;     dBvHXdIMiC17971790 = dBvHXdIMiC40502945;     dBvHXdIMiC40502945 = dBvHXdIMiC1773448;     dBvHXdIMiC1773448 = dBvHXdIMiC87340956;     dBvHXdIMiC87340956 = dBvHXdIMiC49103438;     dBvHXdIMiC49103438 = dBvHXdIMiC68026571;     dBvHXdIMiC68026571 = dBvHXdIMiC56016175;     dBvHXdIMiC56016175 = dBvHXdIMiC73867328;     dBvHXdIMiC73867328 = dBvHXdIMiC57385813;     dBvHXdIMiC57385813 = dBvHXdIMiC73035244;     dBvHXdIMiC73035244 = dBvHXdIMiC93815267;     dBvHXdIMiC93815267 = dBvHXdIMiC53291175;     dBvHXdIMiC53291175 = dBvHXdIMiC41374765;     dBvHXdIMiC41374765 = dBvHXdIMiC45149756;     dBvHXdIMiC45149756 = dBvHXdIMiC97743733;     dBvHXdIMiC97743733 = dBvHXdIMiC68169381;     dBvHXdIMiC68169381 = dBvHXdIMiC9532499;     dBvHXdIMiC9532499 = dBvHXdIMiC55810053;     dBvHXdIMiC55810053 = dBvHXdIMiC79940822;     dBvHXdIMiC79940822 = dBvHXdIMiC67059389;     dBvHXdIMiC67059389 = dBvHXdIMiC18150969;     dBvHXdIMiC18150969 = dBvHXdIMiC59052938;     dBvHXdIMiC59052938 = dBvHXdIMiC2043286;     dBvHXdIMiC2043286 = dBvHXdIMiC6471481;     dBvHXdIMiC6471481 = dBvHXdIMiC8634970;     dBvHXdIMiC8634970 = dBvHXdIMiC2727385;     dBvHXdIMiC2727385 = dBvHXdIMiC89031875;     dBvHXdIMiC89031875 = dBvHXdIMiC45892270;     dBvHXdIMiC45892270 = dBvHXdIMiC26532882;     dBvHXdIMiC26532882 = dBvHXdIMiC77868399;     dBvHXdIMiC77868399 = dBvHXdIMiC23557870;     dBvHXdIMiC23557870 = dBvHXdIMiC63812402;     dBvHXdIMiC63812402 = dBvHXdIMiC99505248;     dBvHXdIMiC99505248 = dBvHXdIMiC532704;     dBvHXdIMiC532704 = dBvHXdIMiC38864707;     dBvHXdIMiC38864707 = dBvHXdIMiC32918203;     dBvHXdIMiC32918203 = dBvHXdIMiC92464418;     dBvHXdIMiC92464418 = dBvHXdIMiC53636243;     dBvHXdIMiC53636243 = dBvHXdIMiC14306781;     dBvHXdIMiC14306781 = dBvHXdIMiC92734022;     dBvHXdIMiC92734022 = dBvHXdIMiC57489698;     dBvHXdIMiC57489698 = dBvHXdIMiC51037528;     dBvHXdIMiC51037528 = dBvHXdIMiC38631936;     dBvHXdIMiC38631936 = dBvHXdIMiC94576403;     dBvHXdIMiC94576403 = dBvHXdIMiC76744998;     dBvHXdIMiC76744998 = dBvHXdIMiC36912237;     dBvHXdIMiC36912237 = dBvHXdIMiC64286795;     dBvHXdIMiC64286795 = dBvHXdIMiC37823759;     dBvHXdIMiC37823759 = dBvHXdIMiC22078504;     dBvHXdIMiC22078504 = dBvHXdIMiC66802562;     dBvHXdIMiC66802562 = dBvHXdIMiC35044916;     dBvHXdIMiC35044916 = dBvHXdIMiC37867842;     dBvHXdIMiC37867842 = dBvHXdIMiC36864846;     dBvHXdIMiC36864846 = dBvHXdIMiC50420237;     dBvHXdIMiC50420237 = dBvHXdIMiC25789198;     dBvHXdIMiC25789198 = dBvHXdIMiC58918851;     dBvHXdIMiC58918851 = dBvHXdIMiC38459660;     dBvHXdIMiC38459660 = dBvHXdIMiC95301967;     dBvHXdIMiC95301967 = dBvHXdIMiC78705987;     dBvHXdIMiC78705987 = dBvHXdIMiC46376053;     dBvHXdIMiC46376053 = dBvHXdIMiC78994695;     dBvHXdIMiC78994695 = dBvHXdIMiC10123905;     dBvHXdIMiC10123905 = dBvHXdIMiC47334447;     dBvHXdIMiC47334447 = dBvHXdIMiC79517414;     dBvHXdIMiC79517414 = dBvHXdIMiC49477374;     dBvHXdIMiC49477374 = dBvHXdIMiC30002865;     dBvHXdIMiC30002865 = dBvHXdIMiC53785927;     dBvHXdIMiC53785927 = dBvHXdIMiC40842061;     dBvHXdIMiC40842061 = dBvHXdIMiC6285049;     dBvHXdIMiC6285049 = dBvHXdIMiC64825531;     dBvHXdIMiC64825531 = dBvHXdIMiC75704962;     dBvHXdIMiC75704962 = dBvHXdIMiC55896256;     dBvHXdIMiC55896256 = dBvHXdIMiC41503272;     dBvHXdIMiC41503272 = dBvHXdIMiC87206800;     dBvHXdIMiC87206800 = dBvHXdIMiC9569691;     dBvHXdIMiC9569691 = dBvHXdIMiC67113441;     dBvHXdIMiC67113441 = dBvHXdIMiC20421002;     dBvHXdIMiC20421002 = dBvHXdIMiC7466882;     dBvHXdIMiC7466882 = dBvHXdIMiC29726482;     dBvHXdIMiC29726482 = dBvHXdIMiC71722733;     dBvHXdIMiC71722733 = dBvHXdIMiC38440590;     dBvHXdIMiC38440590 = dBvHXdIMiC51208117;     dBvHXdIMiC51208117 = dBvHXdIMiC23813767;     dBvHXdIMiC23813767 = dBvHXdIMiC59730319;     dBvHXdIMiC59730319 = dBvHXdIMiC42823483;     dBvHXdIMiC42823483 = dBvHXdIMiC85690028;     dBvHXdIMiC85690028 = dBvHXdIMiC26947556;     dBvHXdIMiC26947556 = dBvHXdIMiC96017748;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XgpNMIrZTL77581662() {     int oScyMLRTjQ32937809 = -843847116;    int oScyMLRTjQ11902466 = -299056788;    int oScyMLRTjQ38584583 = -365993128;    int oScyMLRTjQ14336572 = -196888530;    int oScyMLRTjQ58600672 = -452115351;    int oScyMLRTjQ5140550 = -498318684;    int oScyMLRTjQ12004740 = -685694152;    int oScyMLRTjQ84991804 = -755451546;    int oScyMLRTjQ93648146 = -891898490;    int oScyMLRTjQ66338158 = -308329033;    int oScyMLRTjQ41566550 = 56302976;    int oScyMLRTjQ90679023 = -9883152;    int oScyMLRTjQ62529844 = -304614441;    int oScyMLRTjQ49750953 = -518397254;    int oScyMLRTjQ25508842 = -919551691;    int oScyMLRTjQ30693744 = -989924693;    int oScyMLRTjQ51144322 = -445040781;    int oScyMLRTjQ59070328 = -597580551;    int oScyMLRTjQ9293209 = -170381277;    int oScyMLRTjQ80787443 = 18906315;    int oScyMLRTjQ61250353 = -976057084;    int oScyMLRTjQ40649839 = -231263864;    int oScyMLRTjQ51901183 = -91712389;    int oScyMLRTjQ31084745 = 69157441;    int oScyMLRTjQ91366833 = 34979315;    int oScyMLRTjQ64697184 = -369244481;    int oScyMLRTjQ94595579 = -150486199;    int oScyMLRTjQ7181165 = -742600915;    int oScyMLRTjQ94241785 = -997097854;    int oScyMLRTjQ83268605 = -630666395;    int oScyMLRTjQ23283475 = -599556804;    int oScyMLRTjQ80563817 = -315243895;    int oScyMLRTjQ92695577 = -657516987;    int oScyMLRTjQ48144776 = -796676804;    int oScyMLRTjQ22421396 = -285088507;    int oScyMLRTjQ45710548 = -748453147;    int oScyMLRTjQ40426086 = -428087505;    int oScyMLRTjQ48442767 = -105011366;    int oScyMLRTjQ14786951 = -952053897;    int oScyMLRTjQ9787174 = -17125087;    int oScyMLRTjQ26017618 = -759610640;    int oScyMLRTjQ7798223 = -597461307;    int oScyMLRTjQ44389413 = -799019748;    int oScyMLRTjQ81910524 = -241722837;    int oScyMLRTjQ86801775 = -624835287;    int oScyMLRTjQ12179163 = 52685074;    int oScyMLRTjQ78107177 = -233196840;    int oScyMLRTjQ84966545 = -101399438;    int oScyMLRTjQ52808428 = -850730234;    int oScyMLRTjQ40075354 = 81588930;    int oScyMLRTjQ48308948 = -221824745;    int oScyMLRTjQ72856637 = -114889208;    int oScyMLRTjQ21737713 = -355510138;    int oScyMLRTjQ64597235 = -491492078;    int oScyMLRTjQ16331310 = -740153452;    int oScyMLRTjQ92287969 = -512583253;    int oScyMLRTjQ60001282 = -107344400;    int oScyMLRTjQ7499839 = -335150569;    int oScyMLRTjQ22969739 = -131867845;    int oScyMLRTjQ93903488 = 17129130;    int oScyMLRTjQ10544971 = -247832485;    int oScyMLRTjQ4823575 = -943093237;    int oScyMLRTjQ90750018 = -758353693;    int oScyMLRTjQ10379542 = -161232096;    int oScyMLRTjQ43054684 = -708772229;    int oScyMLRTjQ61002733 = -628453129;    int oScyMLRTjQ97983445 = -352366166;    int oScyMLRTjQ14385068 = -507937638;    int oScyMLRTjQ27329558 = -133308748;    int oScyMLRTjQ79798294 = -71098544;    int oScyMLRTjQ90267658 = -461837188;    int oScyMLRTjQ2701556 = -240029416;    int oScyMLRTjQ44283378 = -645526655;    int oScyMLRTjQ99506034 = -53256191;    int oScyMLRTjQ54769825 = -221483046;    int oScyMLRTjQ53452131 = -278595777;    int oScyMLRTjQ96260426 = -432244116;    int oScyMLRTjQ69990659 = -849989553;    int oScyMLRTjQ44282969 = -306007273;    int oScyMLRTjQ79187670 = 82294241;    int oScyMLRTjQ86590006 = -36047641;    int oScyMLRTjQ9629034 = 50913239;    int oScyMLRTjQ54372737 = -891870682;    int oScyMLRTjQ54166432 = -978686785;    int oScyMLRTjQ34959657 = -308841650;    int oScyMLRTjQ50426837 = -384667597;    int oScyMLRTjQ58826104 = -959733758;    int oScyMLRTjQ28098342 = -66024910;    int oScyMLRTjQ31813467 = 43476648;    int oScyMLRTjQ30133427 = -772505255;    int oScyMLRTjQ85709265 = -541108748;    int oScyMLRTjQ32926247 = 7063063;    int oScyMLRTjQ25473028 = -973143521;    int oScyMLRTjQ20883462 = -869183027;    int oScyMLRTjQ99242203 = -769292603;    int oScyMLRTjQ21194043 = -816517403;    int oScyMLRTjQ17048204 = -839107615;    int oScyMLRTjQ34009872 = -537787653;    int oScyMLRTjQ38855840 = -532950608;    int oScyMLRTjQ25799043 = -843847116;     oScyMLRTjQ32937809 = oScyMLRTjQ11902466;     oScyMLRTjQ11902466 = oScyMLRTjQ38584583;     oScyMLRTjQ38584583 = oScyMLRTjQ14336572;     oScyMLRTjQ14336572 = oScyMLRTjQ58600672;     oScyMLRTjQ58600672 = oScyMLRTjQ5140550;     oScyMLRTjQ5140550 = oScyMLRTjQ12004740;     oScyMLRTjQ12004740 = oScyMLRTjQ84991804;     oScyMLRTjQ84991804 = oScyMLRTjQ93648146;     oScyMLRTjQ93648146 = oScyMLRTjQ66338158;     oScyMLRTjQ66338158 = oScyMLRTjQ41566550;     oScyMLRTjQ41566550 = oScyMLRTjQ90679023;     oScyMLRTjQ90679023 = oScyMLRTjQ62529844;     oScyMLRTjQ62529844 = oScyMLRTjQ49750953;     oScyMLRTjQ49750953 = oScyMLRTjQ25508842;     oScyMLRTjQ25508842 = oScyMLRTjQ30693744;     oScyMLRTjQ30693744 = oScyMLRTjQ51144322;     oScyMLRTjQ51144322 = oScyMLRTjQ59070328;     oScyMLRTjQ59070328 = oScyMLRTjQ9293209;     oScyMLRTjQ9293209 = oScyMLRTjQ80787443;     oScyMLRTjQ80787443 = oScyMLRTjQ61250353;     oScyMLRTjQ61250353 = oScyMLRTjQ40649839;     oScyMLRTjQ40649839 = oScyMLRTjQ51901183;     oScyMLRTjQ51901183 = oScyMLRTjQ31084745;     oScyMLRTjQ31084745 = oScyMLRTjQ91366833;     oScyMLRTjQ91366833 = oScyMLRTjQ64697184;     oScyMLRTjQ64697184 = oScyMLRTjQ94595579;     oScyMLRTjQ94595579 = oScyMLRTjQ7181165;     oScyMLRTjQ7181165 = oScyMLRTjQ94241785;     oScyMLRTjQ94241785 = oScyMLRTjQ83268605;     oScyMLRTjQ83268605 = oScyMLRTjQ23283475;     oScyMLRTjQ23283475 = oScyMLRTjQ80563817;     oScyMLRTjQ80563817 = oScyMLRTjQ92695577;     oScyMLRTjQ92695577 = oScyMLRTjQ48144776;     oScyMLRTjQ48144776 = oScyMLRTjQ22421396;     oScyMLRTjQ22421396 = oScyMLRTjQ45710548;     oScyMLRTjQ45710548 = oScyMLRTjQ40426086;     oScyMLRTjQ40426086 = oScyMLRTjQ48442767;     oScyMLRTjQ48442767 = oScyMLRTjQ14786951;     oScyMLRTjQ14786951 = oScyMLRTjQ9787174;     oScyMLRTjQ9787174 = oScyMLRTjQ26017618;     oScyMLRTjQ26017618 = oScyMLRTjQ7798223;     oScyMLRTjQ7798223 = oScyMLRTjQ44389413;     oScyMLRTjQ44389413 = oScyMLRTjQ81910524;     oScyMLRTjQ81910524 = oScyMLRTjQ86801775;     oScyMLRTjQ86801775 = oScyMLRTjQ12179163;     oScyMLRTjQ12179163 = oScyMLRTjQ78107177;     oScyMLRTjQ78107177 = oScyMLRTjQ84966545;     oScyMLRTjQ84966545 = oScyMLRTjQ52808428;     oScyMLRTjQ52808428 = oScyMLRTjQ40075354;     oScyMLRTjQ40075354 = oScyMLRTjQ48308948;     oScyMLRTjQ48308948 = oScyMLRTjQ72856637;     oScyMLRTjQ72856637 = oScyMLRTjQ21737713;     oScyMLRTjQ21737713 = oScyMLRTjQ64597235;     oScyMLRTjQ64597235 = oScyMLRTjQ16331310;     oScyMLRTjQ16331310 = oScyMLRTjQ92287969;     oScyMLRTjQ92287969 = oScyMLRTjQ60001282;     oScyMLRTjQ60001282 = oScyMLRTjQ7499839;     oScyMLRTjQ7499839 = oScyMLRTjQ22969739;     oScyMLRTjQ22969739 = oScyMLRTjQ93903488;     oScyMLRTjQ93903488 = oScyMLRTjQ10544971;     oScyMLRTjQ10544971 = oScyMLRTjQ4823575;     oScyMLRTjQ4823575 = oScyMLRTjQ90750018;     oScyMLRTjQ90750018 = oScyMLRTjQ10379542;     oScyMLRTjQ10379542 = oScyMLRTjQ43054684;     oScyMLRTjQ43054684 = oScyMLRTjQ61002733;     oScyMLRTjQ61002733 = oScyMLRTjQ97983445;     oScyMLRTjQ97983445 = oScyMLRTjQ14385068;     oScyMLRTjQ14385068 = oScyMLRTjQ27329558;     oScyMLRTjQ27329558 = oScyMLRTjQ79798294;     oScyMLRTjQ79798294 = oScyMLRTjQ90267658;     oScyMLRTjQ90267658 = oScyMLRTjQ2701556;     oScyMLRTjQ2701556 = oScyMLRTjQ44283378;     oScyMLRTjQ44283378 = oScyMLRTjQ99506034;     oScyMLRTjQ99506034 = oScyMLRTjQ54769825;     oScyMLRTjQ54769825 = oScyMLRTjQ53452131;     oScyMLRTjQ53452131 = oScyMLRTjQ96260426;     oScyMLRTjQ96260426 = oScyMLRTjQ69990659;     oScyMLRTjQ69990659 = oScyMLRTjQ44282969;     oScyMLRTjQ44282969 = oScyMLRTjQ79187670;     oScyMLRTjQ79187670 = oScyMLRTjQ86590006;     oScyMLRTjQ86590006 = oScyMLRTjQ9629034;     oScyMLRTjQ9629034 = oScyMLRTjQ54372737;     oScyMLRTjQ54372737 = oScyMLRTjQ54166432;     oScyMLRTjQ54166432 = oScyMLRTjQ34959657;     oScyMLRTjQ34959657 = oScyMLRTjQ50426837;     oScyMLRTjQ50426837 = oScyMLRTjQ58826104;     oScyMLRTjQ58826104 = oScyMLRTjQ28098342;     oScyMLRTjQ28098342 = oScyMLRTjQ31813467;     oScyMLRTjQ31813467 = oScyMLRTjQ30133427;     oScyMLRTjQ30133427 = oScyMLRTjQ85709265;     oScyMLRTjQ85709265 = oScyMLRTjQ32926247;     oScyMLRTjQ32926247 = oScyMLRTjQ25473028;     oScyMLRTjQ25473028 = oScyMLRTjQ20883462;     oScyMLRTjQ20883462 = oScyMLRTjQ99242203;     oScyMLRTjQ99242203 = oScyMLRTjQ21194043;     oScyMLRTjQ21194043 = oScyMLRTjQ17048204;     oScyMLRTjQ17048204 = oScyMLRTjQ34009872;     oScyMLRTjQ34009872 = oScyMLRTjQ38855840;     oScyMLRTjQ38855840 = oScyMLRTjQ25799043;     oScyMLRTjQ25799043 = oScyMLRTjQ32937809;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CJzsFYZLhO14226638() {     int RMOSbpITlw55324926 = -478251079;    int RMOSbpITlw96249965 = -247599843;    int RMOSbpITlw67353861 = -605646972;    int RMOSbpITlw29343260 = -203296061;    int RMOSbpITlw56774926 = 56827648;    int RMOSbpITlw20023667 = -358927062;    int RMOSbpITlw4160479 = -507681458;    int RMOSbpITlw60625663 = -126048191;    int RMOSbpITlw90811277 = -776094529;    int RMOSbpITlw58101743 = -51070366;    int RMOSbpITlw79108409 = 59678575;    int RMOSbpITlw46581972 = -260628445;    int RMOSbpITlw29538024 = -923331761;    int RMOSbpITlw92365705 = 53893989;    int RMOSbpITlw62244119 = -547029404;    int RMOSbpITlw74949656 = -271592235;    int RMOSbpITlw967674 = -722320601;    int RMOSbpITlw65046745 = 50676758;    int RMOSbpITlw50574840 = -305483402;    int RMOSbpITlw59303727 = -233728139;    int RMOSbpITlw62133460 = -967544392;    int RMOSbpITlw4465058 = -736030177;    int RMOSbpITlw55194506 = -267792981;    int RMOSbpITlw57584532 = -494835417;    int RMOSbpITlw26907080 = -686579809;    int RMOSbpITlw53000178 = -813138705;    int RMOSbpITlw41685000 = 91927308;    int RMOSbpITlw13092616 = -531055369;    int RMOSbpITlw1134347 = -241354973;    int RMOSbpITlw92425784 = -550962261;    int RMOSbpITlw67574280 = -680872956;    int RMOSbpITlw38105008 = -967315250;    int RMOSbpITlw73734248 = -86860482;    int RMOSbpITlw7533464 = -20803654;    int RMOSbpITlw98650760 = -348844930;    int RMOSbpITlw87403416 = 36151476;    int RMOSbpITlw33865798 = -161035457;    int RMOSbpITlw2068868 = 27363163;    int RMOSbpITlw63095081 = -253099928;    int RMOSbpITlw29972889 = -153349116;    int RMOSbpITlw54774989 = -162788306;    int RMOSbpITlw96306613 = -355369998;    int RMOSbpITlw49939605 = -686550332;    int RMOSbpITlw81354643 = -239086486;    int RMOSbpITlw54785042 = -433978765;    int RMOSbpITlw20279444 = -129544022;    int RMOSbpITlw90404575 = -811607093;    int RMOSbpITlw29565221 = -957934429;    int RMOSbpITlw47268333 = -800446932;    int RMOSbpITlw37159099 = -60431329;    int RMOSbpITlw57178164 = -576901022;    int RMOSbpITlw46707895 = -545708490;    int RMOSbpITlw52116 = -27421368;    int RMOSbpITlw24883781 = -767172886;    int RMOSbpITlw52093177 = -880857567;    int RMOSbpITlw50859869 = -742220903;    int RMOSbpITlw41055459 = -979806862;    int RMOSbpITlw9769330 = -10811556;    int RMOSbpITlw2436181 = -516716253;    int RMOSbpITlw3774749 = -130033648;    int RMOSbpITlw78338667 = -350854370;    int RMOSbpITlw91067863 = -976626090;    int RMOSbpITlw59491316 = -884693218;    int RMOSbpITlw98385492 = -125132268;    int RMOSbpITlw90527463 = -370197410;    int RMOSbpITlw41003402 = 26993825;    int RMOSbpITlw72847724 = -73767964;    int RMOSbpITlw22004561 = -802528108;    int RMOSbpITlw93714945 = -597261082;    int RMOSbpITlw74840702 = -483180881;    int RMOSbpITlw41083858 = -10556779;    int RMOSbpITlw98898806 = -649683764;    int RMOSbpITlw1951664 = -696223314;    int RMOSbpITlw20601952 = -52134287;    int RMOSbpITlw4528738 = 29060167;    int RMOSbpITlw65826847 = -512174395;    int RMOSbpITlw54525453 = 50520154;    int RMOSbpITlw73839863 = 71293504;    int RMOSbpITlw2799490 = 39143348;    int RMOSbpITlw6627636 = -457035788;    int RMOSbpITlw62595603 = 98468388;    int RMOSbpITlw12119780 = 49861737;    int RMOSbpITlw65824282 = -730608437;    int RMOSbpITlw63975247 = -80923645;    int RMOSbpITlw35247620 = -974061240;    int RMOSbpITlw20866385 = -35164467;    int RMOSbpITlw38052892 = -839893882;    int RMOSbpITlw48850467 = -319687596;    int RMOSbpITlw55440286 = -139946087;    int RMOSbpITlw47790892 = -606624027;    int RMOSbpITlw46347958 = 15958338;    int RMOSbpITlw24096468 = -50223901;    int RMOSbpITlw99632687 = -455920585;    int RMOSbpITlw59320333 = -23066281;    int RMOSbpITlw51634222 = -802494747;    int RMOSbpITlw63707125 = -186162217;    int RMOSbpITlw36815297 = -470676781;    int RMOSbpITlw51554112 = -461418064;    int RMOSbpITlw90827179 = -868889076;    int RMOSbpITlw13781640 = -478251079;     RMOSbpITlw55324926 = RMOSbpITlw96249965;     RMOSbpITlw96249965 = RMOSbpITlw67353861;     RMOSbpITlw67353861 = RMOSbpITlw29343260;     RMOSbpITlw29343260 = RMOSbpITlw56774926;     RMOSbpITlw56774926 = RMOSbpITlw20023667;     RMOSbpITlw20023667 = RMOSbpITlw4160479;     RMOSbpITlw4160479 = RMOSbpITlw60625663;     RMOSbpITlw60625663 = RMOSbpITlw90811277;     RMOSbpITlw90811277 = RMOSbpITlw58101743;     RMOSbpITlw58101743 = RMOSbpITlw79108409;     RMOSbpITlw79108409 = RMOSbpITlw46581972;     RMOSbpITlw46581972 = RMOSbpITlw29538024;     RMOSbpITlw29538024 = RMOSbpITlw92365705;     RMOSbpITlw92365705 = RMOSbpITlw62244119;     RMOSbpITlw62244119 = RMOSbpITlw74949656;     RMOSbpITlw74949656 = RMOSbpITlw967674;     RMOSbpITlw967674 = RMOSbpITlw65046745;     RMOSbpITlw65046745 = RMOSbpITlw50574840;     RMOSbpITlw50574840 = RMOSbpITlw59303727;     RMOSbpITlw59303727 = RMOSbpITlw62133460;     RMOSbpITlw62133460 = RMOSbpITlw4465058;     RMOSbpITlw4465058 = RMOSbpITlw55194506;     RMOSbpITlw55194506 = RMOSbpITlw57584532;     RMOSbpITlw57584532 = RMOSbpITlw26907080;     RMOSbpITlw26907080 = RMOSbpITlw53000178;     RMOSbpITlw53000178 = RMOSbpITlw41685000;     RMOSbpITlw41685000 = RMOSbpITlw13092616;     RMOSbpITlw13092616 = RMOSbpITlw1134347;     RMOSbpITlw1134347 = RMOSbpITlw92425784;     RMOSbpITlw92425784 = RMOSbpITlw67574280;     RMOSbpITlw67574280 = RMOSbpITlw38105008;     RMOSbpITlw38105008 = RMOSbpITlw73734248;     RMOSbpITlw73734248 = RMOSbpITlw7533464;     RMOSbpITlw7533464 = RMOSbpITlw98650760;     RMOSbpITlw98650760 = RMOSbpITlw87403416;     RMOSbpITlw87403416 = RMOSbpITlw33865798;     RMOSbpITlw33865798 = RMOSbpITlw2068868;     RMOSbpITlw2068868 = RMOSbpITlw63095081;     RMOSbpITlw63095081 = RMOSbpITlw29972889;     RMOSbpITlw29972889 = RMOSbpITlw54774989;     RMOSbpITlw54774989 = RMOSbpITlw96306613;     RMOSbpITlw96306613 = RMOSbpITlw49939605;     RMOSbpITlw49939605 = RMOSbpITlw81354643;     RMOSbpITlw81354643 = RMOSbpITlw54785042;     RMOSbpITlw54785042 = RMOSbpITlw20279444;     RMOSbpITlw20279444 = RMOSbpITlw90404575;     RMOSbpITlw90404575 = RMOSbpITlw29565221;     RMOSbpITlw29565221 = RMOSbpITlw47268333;     RMOSbpITlw47268333 = RMOSbpITlw37159099;     RMOSbpITlw37159099 = RMOSbpITlw57178164;     RMOSbpITlw57178164 = RMOSbpITlw46707895;     RMOSbpITlw46707895 = RMOSbpITlw52116;     RMOSbpITlw52116 = RMOSbpITlw24883781;     RMOSbpITlw24883781 = RMOSbpITlw52093177;     RMOSbpITlw52093177 = RMOSbpITlw50859869;     RMOSbpITlw50859869 = RMOSbpITlw41055459;     RMOSbpITlw41055459 = RMOSbpITlw9769330;     RMOSbpITlw9769330 = RMOSbpITlw2436181;     RMOSbpITlw2436181 = RMOSbpITlw3774749;     RMOSbpITlw3774749 = RMOSbpITlw78338667;     RMOSbpITlw78338667 = RMOSbpITlw91067863;     RMOSbpITlw91067863 = RMOSbpITlw59491316;     RMOSbpITlw59491316 = RMOSbpITlw98385492;     RMOSbpITlw98385492 = RMOSbpITlw90527463;     RMOSbpITlw90527463 = RMOSbpITlw41003402;     RMOSbpITlw41003402 = RMOSbpITlw72847724;     RMOSbpITlw72847724 = RMOSbpITlw22004561;     RMOSbpITlw22004561 = RMOSbpITlw93714945;     RMOSbpITlw93714945 = RMOSbpITlw74840702;     RMOSbpITlw74840702 = RMOSbpITlw41083858;     RMOSbpITlw41083858 = RMOSbpITlw98898806;     RMOSbpITlw98898806 = RMOSbpITlw1951664;     RMOSbpITlw1951664 = RMOSbpITlw20601952;     RMOSbpITlw20601952 = RMOSbpITlw4528738;     RMOSbpITlw4528738 = RMOSbpITlw65826847;     RMOSbpITlw65826847 = RMOSbpITlw54525453;     RMOSbpITlw54525453 = RMOSbpITlw73839863;     RMOSbpITlw73839863 = RMOSbpITlw2799490;     RMOSbpITlw2799490 = RMOSbpITlw6627636;     RMOSbpITlw6627636 = RMOSbpITlw62595603;     RMOSbpITlw62595603 = RMOSbpITlw12119780;     RMOSbpITlw12119780 = RMOSbpITlw65824282;     RMOSbpITlw65824282 = RMOSbpITlw63975247;     RMOSbpITlw63975247 = RMOSbpITlw35247620;     RMOSbpITlw35247620 = RMOSbpITlw20866385;     RMOSbpITlw20866385 = RMOSbpITlw38052892;     RMOSbpITlw38052892 = RMOSbpITlw48850467;     RMOSbpITlw48850467 = RMOSbpITlw55440286;     RMOSbpITlw55440286 = RMOSbpITlw47790892;     RMOSbpITlw47790892 = RMOSbpITlw46347958;     RMOSbpITlw46347958 = RMOSbpITlw24096468;     RMOSbpITlw24096468 = RMOSbpITlw99632687;     RMOSbpITlw99632687 = RMOSbpITlw59320333;     RMOSbpITlw59320333 = RMOSbpITlw51634222;     RMOSbpITlw51634222 = RMOSbpITlw63707125;     RMOSbpITlw63707125 = RMOSbpITlw36815297;     RMOSbpITlw36815297 = RMOSbpITlw51554112;     RMOSbpITlw51554112 = RMOSbpITlw90827179;     RMOSbpITlw90827179 = RMOSbpITlw13781640;     RMOSbpITlw13781640 = RMOSbpITlw55324926;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CUkQGPPugt50871612() {     int tMKXnlXcrX77712043 = -112655042;    int tMKXnlXcrX80597466 = -196142898;    int tMKXnlXcrX96123140 = -845300816;    int tMKXnlXcrX44349948 = -209703593;    int tMKXnlXcrX54949181 = -534229353;    int tMKXnlXcrX34906784 = -219535440;    int tMKXnlXcrX96316217 = -329668764;    int tMKXnlXcrX36259522 = -596644835;    int tMKXnlXcrX87974408 = -660290568;    int tMKXnlXcrX49865328 = -893811699;    int tMKXnlXcrX16650270 = 63054174;    int tMKXnlXcrX2484921 = -511373739;    int tMKXnlXcrX96546204 = -442049081;    int tMKXnlXcrX34980458 = -473814768;    int tMKXnlXcrX98979396 = -174507117;    int tMKXnlXcrX19205569 = -653259778;    int tMKXnlXcrX50791025 = -999600421;    int tMKXnlXcrX71023161 = -401065933;    int tMKXnlXcrX91856470 = -440585528;    int tMKXnlXcrX37820010 = -486362592;    int tMKXnlXcrX63016567 = -959031701;    int tMKXnlXcrX68280276 = -140796490;    int tMKXnlXcrX58487829 = -443873573;    int tMKXnlXcrX84084319 = 41171726;    int tMKXnlXcrX62447326 = -308138934;    int tMKXnlXcrX41303173 = -157032929;    int tMKXnlXcrX88774420 = -765659185;    int tMKXnlXcrX19004066 = -319509822;    int tMKXnlXcrX8026908 = -585612092;    int tMKXnlXcrX1582965 = -471258128;    int tMKXnlXcrX11865085 = -762189108;    int tMKXnlXcrX95646198 = -519386605;    int tMKXnlXcrX54772918 = -616203976;    int tMKXnlXcrX66922151 = -344930504;    int tMKXnlXcrX74880125 = -412601352;    int tMKXnlXcrX29096286 = -279243900;    int tMKXnlXcrX27305510 = -993983408;    int tMKXnlXcrX55694968 = -940262309;    int tMKXnlXcrX11403213 = -654145960;    int tMKXnlXcrX50158603 = -289573146;    int tMKXnlXcrX83532360 = -665965972;    int tMKXnlXcrX84815004 = -113278689;    int tMKXnlXcrX55489797 = -574080915;    int tMKXnlXcrX80798762 = -236450135;    int tMKXnlXcrX22768309 = -243122242;    int tMKXnlXcrX28379726 = -311773117;    int tMKXnlXcrX2701973 = -290017346;    int tMKXnlXcrX74163895 = -714469420;    int tMKXnlXcrX41728238 = -750163630;    int tMKXnlXcrX34242845 = -202451588;    int tMKXnlXcrX66047381 = -931977299;    int tMKXnlXcrX20559152 = -976527772;    int tMKXnlXcrX78366519 = -799332598;    int tMKXnlXcrX85170326 = 57146305;    int tMKXnlXcrX87855044 = 78438318;    int tMKXnlXcrX9431768 = -971858553;    int tMKXnlXcrX22109637 = -752269325;    int tMKXnlXcrX12038822 = -786472543;    int tMKXnlXcrX81902621 = -901564660;    int tMKXnlXcrX13646008 = -277196425;    int tMKXnlXcrX46132364 = -453876255;    int tMKXnlXcrX77312152 = 89841058;    int tMKXnlXcrX28232614 = 88967257;    int tMKXnlXcrX86391444 = -89032441;    int tMKXnlXcrX38000243 = -31622591;    int tMKXnlXcrX21004071 = -417559222;    int tMKXnlXcrX47712003 = -895169763;    int tMKXnlXcrX29624054 = 2881423;    int tMKXnlXcrX60100333 = 38786583;    int tMKXnlXcrX69883110 = -895263217;    int tMKXnlXcrX91900058 = -659276370;    int tMKXnlXcrX95096057 = 40661888;    int tMKXnlXcrX59619948 = -746919973;    int tMKXnlXcrX41697868 = -51012383;    int tMKXnlXcrX54287650 = -820396621;    int tMKXnlXcrX78201562 = -745753012;    int tMKXnlXcrX12790480 = -566715575;    int tMKXnlXcrX77689067 = -107423439;    int tMKXnlXcrX61316010 = -715706032;    int tMKXnlXcrX34067601 = -996365817;    int tMKXnlXcrX38601200 = -867015583;    int tMKXnlXcrX14610526 = 48810235;    int tMKXnlXcrX77275827 = -569346192;    int tMKXnlXcrX73784062 = -283160505;    int tMKXnlXcrX35535583 = -539280829;    int tMKXnlXcrX91305933 = -785661337;    int tMKXnlXcrX17279680 = -720054007;    int tMKXnlXcrX69602592 = -573350282;    int tMKXnlXcrX79067106 = -323368822;    int tMKXnlXcrX65448357 = -440742800;    int tMKXnlXcrX6986650 = -526974576;    int tMKXnlXcrX15266689 = -107510866;    int tMKXnlXcrX73792346 = 61302350;    int tMKXnlXcrX97757204 = -276949536;    int tMKXnlXcrX4026240 = -835696891;    int tMKXnlXcrX6220208 = -655807030;    int tMKXnlXcrX56582391 = -102245947;    int tMKXnlXcrX69098352 = -385048475;    int tMKXnlXcrX42798520 = -104827544;    int tMKXnlXcrX1764238 = -112655042;     tMKXnlXcrX77712043 = tMKXnlXcrX80597466;     tMKXnlXcrX80597466 = tMKXnlXcrX96123140;     tMKXnlXcrX96123140 = tMKXnlXcrX44349948;     tMKXnlXcrX44349948 = tMKXnlXcrX54949181;     tMKXnlXcrX54949181 = tMKXnlXcrX34906784;     tMKXnlXcrX34906784 = tMKXnlXcrX96316217;     tMKXnlXcrX96316217 = tMKXnlXcrX36259522;     tMKXnlXcrX36259522 = tMKXnlXcrX87974408;     tMKXnlXcrX87974408 = tMKXnlXcrX49865328;     tMKXnlXcrX49865328 = tMKXnlXcrX16650270;     tMKXnlXcrX16650270 = tMKXnlXcrX2484921;     tMKXnlXcrX2484921 = tMKXnlXcrX96546204;     tMKXnlXcrX96546204 = tMKXnlXcrX34980458;     tMKXnlXcrX34980458 = tMKXnlXcrX98979396;     tMKXnlXcrX98979396 = tMKXnlXcrX19205569;     tMKXnlXcrX19205569 = tMKXnlXcrX50791025;     tMKXnlXcrX50791025 = tMKXnlXcrX71023161;     tMKXnlXcrX71023161 = tMKXnlXcrX91856470;     tMKXnlXcrX91856470 = tMKXnlXcrX37820010;     tMKXnlXcrX37820010 = tMKXnlXcrX63016567;     tMKXnlXcrX63016567 = tMKXnlXcrX68280276;     tMKXnlXcrX68280276 = tMKXnlXcrX58487829;     tMKXnlXcrX58487829 = tMKXnlXcrX84084319;     tMKXnlXcrX84084319 = tMKXnlXcrX62447326;     tMKXnlXcrX62447326 = tMKXnlXcrX41303173;     tMKXnlXcrX41303173 = tMKXnlXcrX88774420;     tMKXnlXcrX88774420 = tMKXnlXcrX19004066;     tMKXnlXcrX19004066 = tMKXnlXcrX8026908;     tMKXnlXcrX8026908 = tMKXnlXcrX1582965;     tMKXnlXcrX1582965 = tMKXnlXcrX11865085;     tMKXnlXcrX11865085 = tMKXnlXcrX95646198;     tMKXnlXcrX95646198 = tMKXnlXcrX54772918;     tMKXnlXcrX54772918 = tMKXnlXcrX66922151;     tMKXnlXcrX66922151 = tMKXnlXcrX74880125;     tMKXnlXcrX74880125 = tMKXnlXcrX29096286;     tMKXnlXcrX29096286 = tMKXnlXcrX27305510;     tMKXnlXcrX27305510 = tMKXnlXcrX55694968;     tMKXnlXcrX55694968 = tMKXnlXcrX11403213;     tMKXnlXcrX11403213 = tMKXnlXcrX50158603;     tMKXnlXcrX50158603 = tMKXnlXcrX83532360;     tMKXnlXcrX83532360 = tMKXnlXcrX84815004;     tMKXnlXcrX84815004 = tMKXnlXcrX55489797;     tMKXnlXcrX55489797 = tMKXnlXcrX80798762;     tMKXnlXcrX80798762 = tMKXnlXcrX22768309;     tMKXnlXcrX22768309 = tMKXnlXcrX28379726;     tMKXnlXcrX28379726 = tMKXnlXcrX2701973;     tMKXnlXcrX2701973 = tMKXnlXcrX74163895;     tMKXnlXcrX74163895 = tMKXnlXcrX41728238;     tMKXnlXcrX41728238 = tMKXnlXcrX34242845;     tMKXnlXcrX34242845 = tMKXnlXcrX66047381;     tMKXnlXcrX66047381 = tMKXnlXcrX20559152;     tMKXnlXcrX20559152 = tMKXnlXcrX78366519;     tMKXnlXcrX78366519 = tMKXnlXcrX85170326;     tMKXnlXcrX85170326 = tMKXnlXcrX87855044;     tMKXnlXcrX87855044 = tMKXnlXcrX9431768;     tMKXnlXcrX9431768 = tMKXnlXcrX22109637;     tMKXnlXcrX22109637 = tMKXnlXcrX12038822;     tMKXnlXcrX12038822 = tMKXnlXcrX81902621;     tMKXnlXcrX81902621 = tMKXnlXcrX13646008;     tMKXnlXcrX13646008 = tMKXnlXcrX46132364;     tMKXnlXcrX46132364 = tMKXnlXcrX77312152;     tMKXnlXcrX77312152 = tMKXnlXcrX28232614;     tMKXnlXcrX28232614 = tMKXnlXcrX86391444;     tMKXnlXcrX86391444 = tMKXnlXcrX38000243;     tMKXnlXcrX38000243 = tMKXnlXcrX21004071;     tMKXnlXcrX21004071 = tMKXnlXcrX47712003;     tMKXnlXcrX47712003 = tMKXnlXcrX29624054;     tMKXnlXcrX29624054 = tMKXnlXcrX60100333;     tMKXnlXcrX60100333 = tMKXnlXcrX69883110;     tMKXnlXcrX69883110 = tMKXnlXcrX91900058;     tMKXnlXcrX91900058 = tMKXnlXcrX95096057;     tMKXnlXcrX95096057 = tMKXnlXcrX59619948;     tMKXnlXcrX59619948 = tMKXnlXcrX41697868;     tMKXnlXcrX41697868 = tMKXnlXcrX54287650;     tMKXnlXcrX54287650 = tMKXnlXcrX78201562;     tMKXnlXcrX78201562 = tMKXnlXcrX12790480;     tMKXnlXcrX12790480 = tMKXnlXcrX77689067;     tMKXnlXcrX77689067 = tMKXnlXcrX61316010;     tMKXnlXcrX61316010 = tMKXnlXcrX34067601;     tMKXnlXcrX34067601 = tMKXnlXcrX38601200;     tMKXnlXcrX38601200 = tMKXnlXcrX14610526;     tMKXnlXcrX14610526 = tMKXnlXcrX77275827;     tMKXnlXcrX77275827 = tMKXnlXcrX73784062;     tMKXnlXcrX73784062 = tMKXnlXcrX35535583;     tMKXnlXcrX35535583 = tMKXnlXcrX91305933;     tMKXnlXcrX91305933 = tMKXnlXcrX17279680;     tMKXnlXcrX17279680 = tMKXnlXcrX69602592;     tMKXnlXcrX69602592 = tMKXnlXcrX79067106;     tMKXnlXcrX79067106 = tMKXnlXcrX65448357;     tMKXnlXcrX65448357 = tMKXnlXcrX6986650;     tMKXnlXcrX6986650 = tMKXnlXcrX15266689;     tMKXnlXcrX15266689 = tMKXnlXcrX73792346;     tMKXnlXcrX73792346 = tMKXnlXcrX97757204;     tMKXnlXcrX97757204 = tMKXnlXcrX4026240;     tMKXnlXcrX4026240 = tMKXnlXcrX6220208;     tMKXnlXcrX6220208 = tMKXnlXcrX56582391;     tMKXnlXcrX56582391 = tMKXnlXcrX69098352;     tMKXnlXcrX69098352 = tMKXnlXcrX42798520;     tMKXnlXcrX42798520 = tMKXnlXcrX1764238;     tMKXnlXcrX1764238 = tMKXnlXcrX77712043;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void deRIpZrjVi87516586() {     int tWayDsAcID99161 = -847059005;    int tWayDsAcID64944966 = -144685952;    int tWayDsAcID24892419 = 15045340;    int tWayDsAcID59356637 = -216111125;    int tWayDsAcID53123435 = -25286355;    int tWayDsAcID49789901 = -80143818;    int tWayDsAcID88471956 = -151656070;    int tWayDsAcID11893380 = 32758521;    int tWayDsAcID85137539 = -544486607;    int tWayDsAcID41628913 = -636553032;    int tWayDsAcID54192129 = 66429773;    int tWayDsAcID58387870 = -762119032;    int tWayDsAcID63554385 = 39233600;    int tWayDsAcID77595210 = 98476475;    int tWayDsAcID35714673 = -901984830;    int tWayDsAcID63461481 = 65072680;    int tWayDsAcID614378 = -176880241;    int tWayDsAcID76999577 = -852808623;    int tWayDsAcID33138102 = -575687653;    int tWayDsAcID16336294 = -738997046;    int tWayDsAcID63899674 = -950519009;    int tWayDsAcID32095494 = -645562804;    int tWayDsAcID61781153 = -619954165;    int tWayDsAcID10584107 = -522821131;    int tWayDsAcID97987573 = 70301942;    int tWayDsAcID29606168 = -600927153;    int tWayDsAcID35863841 = -523245678;    int tWayDsAcID24915516 = -107964276;    int tWayDsAcID14919468 = -929869212;    int tWayDsAcID10740144 = -391553995;    int tWayDsAcID56155890 = -843505260;    int tWayDsAcID53187390 = -71457959;    int tWayDsAcID35811589 = -45547471;    int tWayDsAcID26310838 = -669057354;    int tWayDsAcID51109490 = -476357775;    int tWayDsAcID70789155 = -594639276;    int tWayDsAcID20745222 = -726931359;    int tWayDsAcID9321070 = -807887781;    int tWayDsAcID59711344 = 44808008;    int tWayDsAcID70344318 = -425797175;    int tWayDsAcID12289731 = -69143638;    int tWayDsAcID73323396 = -971187380;    int tWayDsAcID61039989 = -461611499;    int tWayDsAcID80242881 = -233813784;    int tWayDsAcID90751575 = -52265720;    int tWayDsAcID36480008 = -494002213;    int tWayDsAcID14999370 = -868427599;    int tWayDsAcID18762570 = -471004411;    int tWayDsAcID36188143 = -699880328;    int tWayDsAcID31326591 = -344471847;    int tWayDsAcID74916597 = -187053576;    int tWayDsAcID94410408 = -307347054;    int tWayDsAcID56680922 = -471243829;    int tWayDsAcID45456871 = -218534504;    int tWayDsAcID23616912 = -62265798;    int tWayDsAcID68003667 = -101496202;    int tWayDsAcID3163814 = -524731787;    int tWayDsAcID14308313 = -462133530;    int tWayDsAcID61369063 = -186413067;    int tWayDsAcID23517268 = -424359202;    int tWayDsAcID13926061 = -556898140;    int tWayDsAcID63556441 = 56308205;    int tWayDsAcID96973911 = -37372268;    int tWayDsAcID74397396 = -52932613;    int tWayDsAcID85473022 = -793047772;    int tWayDsAcID1004740 = -862112268;    int tWayDsAcID22576281 = -616571561;    int tWayDsAcID37243547 = -291709047;    int tWayDsAcID26485721 = -425165751;    int tWayDsAcID64925518 = -207345554;    int tWayDsAcID42716259 = -207995961;    int tWayDsAcID91293308 = -368992460;    int tWayDsAcID17288234 = -797616632;    int tWayDsAcID62793784 = -49890479;    int tWayDsAcID4046563 = -569853408;    int tWayDsAcID90576278 = -979331629;    int tWayDsAcID71055505 = -83951305;    int tWayDsAcID81538271 = -286140382;    int tWayDsAcID19832531 = -370555411;    int tWayDsAcID61507565 = -435695846;    int tWayDsAcID14606798 = -732499554;    int tWayDsAcID17101271 = 47758733;    int tWayDsAcID88727372 = -408083948;    int tWayDsAcID83592877 = -485397365;    int tWayDsAcID35823546 = -104500419;    int tWayDsAcID61745481 = -436158207;    int tWayDsAcID96506467 = -600214131;    int tWayDsAcID90354717 = -827012968;    int tWayDsAcID2693926 = -506791557;    int tWayDsAcID83105823 = -274861573;    int tWayDsAcID67625341 = 30092510;    int tWayDsAcID6436910 = -164797830;    int tWayDsAcID47952006 = -521474715;    int tWayDsAcID36194077 = -530832790;    int tWayDsAcID56418258 = -868899035;    int tWayDsAcID48733289 = -25451844;    int tWayDsAcID76349484 = -833815113;    int tWayDsAcID86642592 = -308678887;    int tWayDsAcID94769859 = -440766012;    int tWayDsAcID89746835 = -847059005;     tWayDsAcID99161 = tWayDsAcID64944966;     tWayDsAcID64944966 = tWayDsAcID24892419;     tWayDsAcID24892419 = tWayDsAcID59356637;     tWayDsAcID59356637 = tWayDsAcID53123435;     tWayDsAcID53123435 = tWayDsAcID49789901;     tWayDsAcID49789901 = tWayDsAcID88471956;     tWayDsAcID88471956 = tWayDsAcID11893380;     tWayDsAcID11893380 = tWayDsAcID85137539;     tWayDsAcID85137539 = tWayDsAcID41628913;     tWayDsAcID41628913 = tWayDsAcID54192129;     tWayDsAcID54192129 = tWayDsAcID58387870;     tWayDsAcID58387870 = tWayDsAcID63554385;     tWayDsAcID63554385 = tWayDsAcID77595210;     tWayDsAcID77595210 = tWayDsAcID35714673;     tWayDsAcID35714673 = tWayDsAcID63461481;     tWayDsAcID63461481 = tWayDsAcID614378;     tWayDsAcID614378 = tWayDsAcID76999577;     tWayDsAcID76999577 = tWayDsAcID33138102;     tWayDsAcID33138102 = tWayDsAcID16336294;     tWayDsAcID16336294 = tWayDsAcID63899674;     tWayDsAcID63899674 = tWayDsAcID32095494;     tWayDsAcID32095494 = tWayDsAcID61781153;     tWayDsAcID61781153 = tWayDsAcID10584107;     tWayDsAcID10584107 = tWayDsAcID97987573;     tWayDsAcID97987573 = tWayDsAcID29606168;     tWayDsAcID29606168 = tWayDsAcID35863841;     tWayDsAcID35863841 = tWayDsAcID24915516;     tWayDsAcID24915516 = tWayDsAcID14919468;     tWayDsAcID14919468 = tWayDsAcID10740144;     tWayDsAcID10740144 = tWayDsAcID56155890;     tWayDsAcID56155890 = tWayDsAcID53187390;     tWayDsAcID53187390 = tWayDsAcID35811589;     tWayDsAcID35811589 = tWayDsAcID26310838;     tWayDsAcID26310838 = tWayDsAcID51109490;     tWayDsAcID51109490 = tWayDsAcID70789155;     tWayDsAcID70789155 = tWayDsAcID20745222;     tWayDsAcID20745222 = tWayDsAcID9321070;     tWayDsAcID9321070 = tWayDsAcID59711344;     tWayDsAcID59711344 = tWayDsAcID70344318;     tWayDsAcID70344318 = tWayDsAcID12289731;     tWayDsAcID12289731 = tWayDsAcID73323396;     tWayDsAcID73323396 = tWayDsAcID61039989;     tWayDsAcID61039989 = tWayDsAcID80242881;     tWayDsAcID80242881 = tWayDsAcID90751575;     tWayDsAcID90751575 = tWayDsAcID36480008;     tWayDsAcID36480008 = tWayDsAcID14999370;     tWayDsAcID14999370 = tWayDsAcID18762570;     tWayDsAcID18762570 = tWayDsAcID36188143;     tWayDsAcID36188143 = tWayDsAcID31326591;     tWayDsAcID31326591 = tWayDsAcID74916597;     tWayDsAcID74916597 = tWayDsAcID94410408;     tWayDsAcID94410408 = tWayDsAcID56680922;     tWayDsAcID56680922 = tWayDsAcID45456871;     tWayDsAcID45456871 = tWayDsAcID23616912;     tWayDsAcID23616912 = tWayDsAcID68003667;     tWayDsAcID68003667 = tWayDsAcID3163814;     tWayDsAcID3163814 = tWayDsAcID14308313;     tWayDsAcID14308313 = tWayDsAcID61369063;     tWayDsAcID61369063 = tWayDsAcID23517268;     tWayDsAcID23517268 = tWayDsAcID13926061;     tWayDsAcID13926061 = tWayDsAcID63556441;     tWayDsAcID63556441 = tWayDsAcID96973911;     tWayDsAcID96973911 = tWayDsAcID74397396;     tWayDsAcID74397396 = tWayDsAcID85473022;     tWayDsAcID85473022 = tWayDsAcID1004740;     tWayDsAcID1004740 = tWayDsAcID22576281;     tWayDsAcID22576281 = tWayDsAcID37243547;     tWayDsAcID37243547 = tWayDsAcID26485721;     tWayDsAcID26485721 = tWayDsAcID64925518;     tWayDsAcID64925518 = tWayDsAcID42716259;     tWayDsAcID42716259 = tWayDsAcID91293308;     tWayDsAcID91293308 = tWayDsAcID17288234;     tWayDsAcID17288234 = tWayDsAcID62793784;     tWayDsAcID62793784 = tWayDsAcID4046563;     tWayDsAcID4046563 = tWayDsAcID90576278;     tWayDsAcID90576278 = tWayDsAcID71055505;     tWayDsAcID71055505 = tWayDsAcID81538271;     tWayDsAcID81538271 = tWayDsAcID19832531;     tWayDsAcID19832531 = tWayDsAcID61507565;     tWayDsAcID61507565 = tWayDsAcID14606798;     tWayDsAcID14606798 = tWayDsAcID17101271;     tWayDsAcID17101271 = tWayDsAcID88727372;     tWayDsAcID88727372 = tWayDsAcID83592877;     tWayDsAcID83592877 = tWayDsAcID35823546;     tWayDsAcID35823546 = tWayDsAcID61745481;     tWayDsAcID61745481 = tWayDsAcID96506467;     tWayDsAcID96506467 = tWayDsAcID90354717;     tWayDsAcID90354717 = tWayDsAcID2693926;     tWayDsAcID2693926 = tWayDsAcID83105823;     tWayDsAcID83105823 = tWayDsAcID67625341;     tWayDsAcID67625341 = tWayDsAcID6436910;     tWayDsAcID6436910 = tWayDsAcID47952006;     tWayDsAcID47952006 = tWayDsAcID36194077;     tWayDsAcID36194077 = tWayDsAcID56418258;     tWayDsAcID56418258 = tWayDsAcID48733289;     tWayDsAcID48733289 = tWayDsAcID76349484;     tWayDsAcID76349484 = tWayDsAcID86642592;     tWayDsAcID86642592 = tWayDsAcID94769859;     tWayDsAcID94769859 = tWayDsAcID89746835;     tWayDsAcID89746835 = tWayDsAcID99161;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void kLKepIGrPx24161561() {     int UBNAaVNnSn22486279 = -481462968;    int UBNAaVNnSn49292467 = -93229007;    int UBNAaVNnSn53661698 = -224608504;    int UBNAaVNnSn74363325 = -222518656;    int UBNAaVNnSn51297689 = -616343356;    int UBNAaVNnSn64673018 = 59247805;    int UBNAaVNnSn80627695 = 26356624;    int UBNAaVNnSn87527238 = -437838123;    int UBNAaVNnSn82300670 = -428682646;    int UBNAaVNnSn33392497 = -379294365;    int UBNAaVNnSn91733988 = 69805372;    int UBNAaVNnSn14290819 = 87135675;    int UBNAaVNnSn30562566 = -579483720;    int UBNAaVNnSn20209963 = -429232282;    int UBNAaVNnSn72449950 = -529462543;    int UBNAaVNnSn7717394 = -316594862;    int UBNAaVNnSn50437729 = -454160061;    int UBNAaVNnSn82975993 = -204551314;    int UBNAaVNnSn74419732 = -710789778;    int UBNAaVNnSn94852576 = -991631499;    int UBNAaVNnSn64782782 = -942006318;    int UBNAaVNnSn95910712 = -50329117;    int UBNAaVNnSn65074476 = -796034757;    int UBNAaVNnSn37083893 = 13186012;    int UBNAaVNnSn33527820 = -651257183;    int UBNAaVNnSn17909162 = 55178623;    int UBNAaVNnSn82953261 = -280832171;    int UBNAaVNnSn30826966 = -996418729;    int UBNAaVNnSn21812029 = -174126331;    int UBNAaVNnSn19897323 = -311849861;    int UBNAaVNnSn446696 = -924821412;    int UBNAaVNnSn10728581 = -723529314;    int UBNAaVNnSn16850260 = -574890966;    int UBNAaVNnSn85699525 = -993184204;    int UBNAaVNnSn27338855 = -540114197;    int UBNAaVNnSn12482025 = -910034653;    int UBNAaVNnSn14184935 = -459879311;    int UBNAaVNnSn62947170 = -675513253;    int UBNAaVNnSn8019476 = -356238024;    int UBNAaVNnSn90530032 = -562021204;    int UBNAaVNnSn41047102 = -572321304;    int UBNAaVNnSn61831787 = -729096071;    int UBNAaVNnSn66590181 = -349142083;    int UBNAaVNnSn79687001 = -231177433;    int UBNAaVNnSn58734842 = -961409198;    int UBNAaVNnSn44580290 = -676231308;    int UBNAaVNnSn27296768 = -346837852;    int UBNAaVNnSn63361244 = -227539402;    int UBNAaVNnSn30648048 = -649597027;    int UBNAaVNnSn28410336 = -486492106;    int UBNAaVNnSn83785813 = -542129853;    int UBNAaVNnSn68261666 = -738166336;    int UBNAaVNnSn34995326 = -143155059;    int UBNAaVNnSn5743417 = -494215313;    int UBNAaVNnSn59378779 = -202969913;    int UBNAaVNnSn26575566 = -331133852;    int UBNAaVNnSn84217990 = -297194250;    int UBNAaVNnSn16577805 = -137794516;    int UBNAaVNnSn40835505 = -571261474;    int UBNAaVNnSn33388527 = -571521980;    int UBNAaVNnSn81719756 = -659920025;    int UBNAaVNnSn49800730 = 22775353;    int UBNAaVNnSn65715210 = -163711793;    int UBNAaVNnSn62403347 = -16832786;    int UBNAaVNnSn32945802 = -454472953;    int UBNAaVNnSn81005408 = -206665314;    int UBNAaVNnSn97440559 = -337973360;    int UBNAaVNnSn44863041 = -586299516;    int UBNAaVNnSn92871108 = -889118086;    int UBNAaVNnSn59967926 = -619427891;    int UBNAaVNnSn93532459 = -856715552;    int UBNAaVNnSn87490558 = -778646808;    int UBNAaVNnSn74956518 = -848313291;    int UBNAaVNnSn83889700 = -48768574;    int UBNAaVNnSn53805475 = -319310196;    int UBNAaVNnSn2950995 = -112910247;    int UBNAaVNnSn29320532 = -701187034;    int UBNAaVNnSn85387474 = -464857325;    int UBNAaVNnSn78349051 = -25404791;    int UBNAaVNnSn88947530 = -975025875;    int UBNAaVNnSn90612394 = -597983525;    int UBNAaVNnSn19592017 = 46707231;    int UBNAaVNnSn178918 = -246821703;    int UBNAaVNnSn93401692 = -687634225;    int UBNAaVNnSn36111509 = -769720008;    int UBNAaVNnSn32185029 = -86655077;    int UBNAaVNnSn75733255 = -480374256;    int UBNAaVNnSn11106843 = 19324346;    int UBNAaVNnSn26320746 = -690214292;    int UBNAaVNnSn763289 = -108980345;    int UBNAaVNnSn28264034 = -512840403;    int UBNAaVNnSn97607129 = -222084795;    int UBNAaVNnSn22111665 = -4251780;    int UBNAaVNnSn74630948 = -784716044;    int UBNAaVNnSn8810276 = -902101179;    int UBNAaVNnSn91246371 = -495096657;    int UBNAaVNnSn96116577 = -465384279;    int UBNAaVNnSn4186834 = -232309298;    int UBNAaVNnSn46741199 = -776704480;    int UBNAaVNnSn77729433 = -481462968;     UBNAaVNnSn22486279 = UBNAaVNnSn49292467;     UBNAaVNnSn49292467 = UBNAaVNnSn53661698;     UBNAaVNnSn53661698 = UBNAaVNnSn74363325;     UBNAaVNnSn74363325 = UBNAaVNnSn51297689;     UBNAaVNnSn51297689 = UBNAaVNnSn64673018;     UBNAaVNnSn64673018 = UBNAaVNnSn80627695;     UBNAaVNnSn80627695 = UBNAaVNnSn87527238;     UBNAaVNnSn87527238 = UBNAaVNnSn82300670;     UBNAaVNnSn82300670 = UBNAaVNnSn33392497;     UBNAaVNnSn33392497 = UBNAaVNnSn91733988;     UBNAaVNnSn91733988 = UBNAaVNnSn14290819;     UBNAaVNnSn14290819 = UBNAaVNnSn30562566;     UBNAaVNnSn30562566 = UBNAaVNnSn20209963;     UBNAaVNnSn20209963 = UBNAaVNnSn72449950;     UBNAaVNnSn72449950 = UBNAaVNnSn7717394;     UBNAaVNnSn7717394 = UBNAaVNnSn50437729;     UBNAaVNnSn50437729 = UBNAaVNnSn82975993;     UBNAaVNnSn82975993 = UBNAaVNnSn74419732;     UBNAaVNnSn74419732 = UBNAaVNnSn94852576;     UBNAaVNnSn94852576 = UBNAaVNnSn64782782;     UBNAaVNnSn64782782 = UBNAaVNnSn95910712;     UBNAaVNnSn95910712 = UBNAaVNnSn65074476;     UBNAaVNnSn65074476 = UBNAaVNnSn37083893;     UBNAaVNnSn37083893 = UBNAaVNnSn33527820;     UBNAaVNnSn33527820 = UBNAaVNnSn17909162;     UBNAaVNnSn17909162 = UBNAaVNnSn82953261;     UBNAaVNnSn82953261 = UBNAaVNnSn30826966;     UBNAaVNnSn30826966 = UBNAaVNnSn21812029;     UBNAaVNnSn21812029 = UBNAaVNnSn19897323;     UBNAaVNnSn19897323 = UBNAaVNnSn446696;     UBNAaVNnSn446696 = UBNAaVNnSn10728581;     UBNAaVNnSn10728581 = UBNAaVNnSn16850260;     UBNAaVNnSn16850260 = UBNAaVNnSn85699525;     UBNAaVNnSn85699525 = UBNAaVNnSn27338855;     UBNAaVNnSn27338855 = UBNAaVNnSn12482025;     UBNAaVNnSn12482025 = UBNAaVNnSn14184935;     UBNAaVNnSn14184935 = UBNAaVNnSn62947170;     UBNAaVNnSn62947170 = UBNAaVNnSn8019476;     UBNAaVNnSn8019476 = UBNAaVNnSn90530032;     UBNAaVNnSn90530032 = UBNAaVNnSn41047102;     UBNAaVNnSn41047102 = UBNAaVNnSn61831787;     UBNAaVNnSn61831787 = UBNAaVNnSn66590181;     UBNAaVNnSn66590181 = UBNAaVNnSn79687001;     UBNAaVNnSn79687001 = UBNAaVNnSn58734842;     UBNAaVNnSn58734842 = UBNAaVNnSn44580290;     UBNAaVNnSn44580290 = UBNAaVNnSn27296768;     UBNAaVNnSn27296768 = UBNAaVNnSn63361244;     UBNAaVNnSn63361244 = UBNAaVNnSn30648048;     UBNAaVNnSn30648048 = UBNAaVNnSn28410336;     UBNAaVNnSn28410336 = UBNAaVNnSn83785813;     UBNAaVNnSn83785813 = UBNAaVNnSn68261666;     UBNAaVNnSn68261666 = UBNAaVNnSn34995326;     UBNAaVNnSn34995326 = UBNAaVNnSn5743417;     UBNAaVNnSn5743417 = UBNAaVNnSn59378779;     UBNAaVNnSn59378779 = UBNAaVNnSn26575566;     UBNAaVNnSn26575566 = UBNAaVNnSn84217990;     UBNAaVNnSn84217990 = UBNAaVNnSn16577805;     UBNAaVNnSn16577805 = UBNAaVNnSn40835505;     UBNAaVNnSn40835505 = UBNAaVNnSn33388527;     UBNAaVNnSn33388527 = UBNAaVNnSn81719756;     UBNAaVNnSn81719756 = UBNAaVNnSn49800730;     UBNAaVNnSn49800730 = UBNAaVNnSn65715210;     UBNAaVNnSn65715210 = UBNAaVNnSn62403347;     UBNAaVNnSn62403347 = UBNAaVNnSn32945802;     UBNAaVNnSn32945802 = UBNAaVNnSn81005408;     UBNAaVNnSn81005408 = UBNAaVNnSn97440559;     UBNAaVNnSn97440559 = UBNAaVNnSn44863041;     UBNAaVNnSn44863041 = UBNAaVNnSn92871108;     UBNAaVNnSn92871108 = UBNAaVNnSn59967926;     UBNAaVNnSn59967926 = UBNAaVNnSn93532459;     UBNAaVNnSn93532459 = UBNAaVNnSn87490558;     UBNAaVNnSn87490558 = UBNAaVNnSn74956518;     UBNAaVNnSn74956518 = UBNAaVNnSn83889700;     UBNAaVNnSn83889700 = UBNAaVNnSn53805475;     UBNAaVNnSn53805475 = UBNAaVNnSn2950995;     UBNAaVNnSn2950995 = UBNAaVNnSn29320532;     UBNAaVNnSn29320532 = UBNAaVNnSn85387474;     UBNAaVNnSn85387474 = UBNAaVNnSn78349051;     UBNAaVNnSn78349051 = UBNAaVNnSn88947530;     UBNAaVNnSn88947530 = UBNAaVNnSn90612394;     UBNAaVNnSn90612394 = UBNAaVNnSn19592017;     UBNAaVNnSn19592017 = UBNAaVNnSn178918;     UBNAaVNnSn178918 = UBNAaVNnSn93401692;     UBNAaVNnSn93401692 = UBNAaVNnSn36111509;     UBNAaVNnSn36111509 = UBNAaVNnSn32185029;     UBNAaVNnSn32185029 = UBNAaVNnSn75733255;     UBNAaVNnSn75733255 = UBNAaVNnSn11106843;     UBNAaVNnSn11106843 = UBNAaVNnSn26320746;     UBNAaVNnSn26320746 = UBNAaVNnSn763289;     UBNAaVNnSn763289 = UBNAaVNnSn28264034;     UBNAaVNnSn28264034 = UBNAaVNnSn97607129;     UBNAaVNnSn97607129 = UBNAaVNnSn22111665;     UBNAaVNnSn22111665 = UBNAaVNnSn74630948;     UBNAaVNnSn74630948 = UBNAaVNnSn8810276;     UBNAaVNnSn8810276 = UBNAaVNnSn91246371;     UBNAaVNnSn91246371 = UBNAaVNnSn96116577;     UBNAaVNnSn96116577 = UBNAaVNnSn4186834;     UBNAaVNnSn4186834 = UBNAaVNnSn46741199;     UBNAaVNnSn46741199 = UBNAaVNnSn77729433;     UBNAaVNnSn77729433 = UBNAaVNnSn22486279;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BYXcojWSoL60806536() {     int maiFsbUFBD44873396 = -115866932;    int maiFsbUFBD33639967 = -41772062;    int maiFsbUFBD82430976 = -464262348;    int maiFsbUFBD89370013 = -228926188;    int maiFsbUFBD49471943 = -107400357;    int maiFsbUFBD79556134 = -901360573;    int maiFsbUFBD72783434 = -895630682;    int maiFsbUFBD63161097 = -908434767;    int maiFsbUFBD79463801 = -312878685;    int maiFsbUFBD25156082 = -122035698;    int maiFsbUFBD29275849 = 73180972;    int maiFsbUFBD70193768 = -163609619;    int maiFsbUFBD97570746 = -98201040;    int maiFsbUFBD62824715 = -956941039;    int maiFsbUFBD9185228 = -156940256;    int maiFsbUFBD51973306 = -698262405;    int maiFsbUFBD261081 = -731439881;    int maiFsbUFBD88952410 = -656294005;    int maiFsbUFBD15701364 = -845891903;    int maiFsbUFBD73368860 = -144265953;    int maiFsbUFBD65665889 = -933493626;    int maiFsbUFBD59725931 = -555095430;    int maiFsbUFBD68367799 = -972115350;    int maiFsbUFBD63583680 = -550806845;    int maiFsbUFBD69068067 = -272816307;    int maiFsbUFBD6212157 = -388715601;    int maiFsbUFBD30042682 = -38418664;    int maiFsbUFBD36738416 = -784873183;    int maiFsbUFBD28704590 = -518383450;    int maiFsbUFBD29054503 = -232145728;    int maiFsbUFBD44737500 = 93862436;    int maiFsbUFBD68269771 = -275600669;    int maiFsbUFBD97888929 = -4234461;    int maiFsbUFBD45088213 = -217311054;    int maiFsbUFBD3568220 = -603870619;    int maiFsbUFBD54174893 = -125430029;    int maiFsbUFBD7624647 = -192827262;    int maiFsbUFBD16573271 = -543138725;    int maiFsbUFBD56327607 = -757284055;    int maiFsbUFBD10715747 = -698245233;    int maiFsbUFBD69804472 = 24501030;    int maiFsbUFBD50340178 = -487004762;    int maiFsbUFBD72140372 = -236672667;    int maiFsbUFBD79131120 = -228541081;    int maiFsbUFBD26718109 = -770552675;    int maiFsbUFBD52680572 = -858460403;    int maiFsbUFBD39594165 = -925248105;    int maiFsbUFBD7959919 = 15925607;    int maiFsbUFBD25107953 = -599313725;    int maiFsbUFBD25494082 = -628512365;    int maiFsbUFBD92655030 = -897206130;    int maiFsbUFBD42112923 = -68985618;    int maiFsbUFBD13309729 = -915066289;    int maiFsbUFBD66029961 = -769896121;    int maiFsbUFBD95140647 = -343674028;    int maiFsbUFBD85147464 = -560771502;    int maiFsbUFBD65272168 = -69656713;    int maiFsbUFBD18847296 = -913455503;    int maiFsbUFBD20301947 = -956109881;    int maiFsbUFBD43259787 = -718684757;    int maiFsbUFBD49513453 = -762941910;    int maiFsbUFBD36045019 = -10757500;    int maiFsbUFBD34456508 = -290051318;    int maiFsbUFBD50409299 = 19267042;    int maiFsbUFBD80418581 = -115898134;    int maiFsbUFBD61006077 = -651218360;    int maiFsbUFBD72304838 = -59375158;    int maiFsbUFBD52482534 = -880889986;    int maiFsbUFBD59256496 = -253070420;    int maiFsbUFBD55010334 = 68489773;    int maiFsbUFBD44348660 = -405435143;    int maiFsbUFBD83687809 = -88301156;    int maiFsbUFBD32624804 = -899009950;    int maiFsbUFBD4985617 = -47646670;    int maiFsbUFBD3564388 = -68766983;    int maiFsbUFBD15325711 = -346488864;    int maiFsbUFBD87585558 = -218422764;    int maiFsbUFBD89236678 = -643574269;    int maiFsbUFBD36865572 = -780254171;    int maiFsbUFBD16387496 = -414355905;    int maiFsbUFBD66617992 = -463467496;    int maiFsbUFBD22082763 = 45655729;    int maiFsbUFBD11630463 = -85559458;    int maiFsbUFBD3210509 = -889871086;    int maiFsbUFBD36399472 = -334939598;    int maiFsbUFBD2624578 = -837151947;    int maiFsbUFBD54960043 = -360534380;    int maiFsbUFBD31858968 = -234338340;    int maiFsbUFBD49947566 = -873637027;    int maiFsbUFBD18420755 = 56900882;    int maiFsbUFBD88902725 = 44226683;    int maiFsbUFBD88777350 = -279371760;    int maiFsbUFBD96271324 = -587028845;    int maiFsbUFBD13067820 = 61400701;    int maiFsbUFBD61202294 = -935303324;    int maiFsbUFBD33759454 = -964741471;    int maiFsbUFBD15883671 = -96953445;    int maiFsbUFBD21731074 = -155939709;    int maiFsbUFBD98712538 = -12642948;    int maiFsbUFBD65712031 = -115866932;     maiFsbUFBD44873396 = maiFsbUFBD33639967;     maiFsbUFBD33639967 = maiFsbUFBD82430976;     maiFsbUFBD82430976 = maiFsbUFBD89370013;     maiFsbUFBD89370013 = maiFsbUFBD49471943;     maiFsbUFBD49471943 = maiFsbUFBD79556134;     maiFsbUFBD79556134 = maiFsbUFBD72783434;     maiFsbUFBD72783434 = maiFsbUFBD63161097;     maiFsbUFBD63161097 = maiFsbUFBD79463801;     maiFsbUFBD79463801 = maiFsbUFBD25156082;     maiFsbUFBD25156082 = maiFsbUFBD29275849;     maiFsbUFBD29275849 = maiFsbUFBD70193768;     maiFsbUFBD70193768 = maiFsbUFBD97570746;     maiFsbUFBD97570746 = maiFsbUFBD62824715;     maiFsbUFBD62824715 = maiFsbUFBD9185228;     maiFsbUFBD9185228 = maiFsbUFBD51973306;     maiFsbUFBD51973306 = maiFsbUFBD261081;     maiFsbUFBD261081 = maiFsbUFBD88952410;     maiFsbUFBD88952410 = maiFsbUFBD15701364;     maiFsbUFBD15701364 = maiFsbUFBD73368860;     maiFsbUFBD73368860 = maiFsbUFBD65665889;     maiFsbUFBD65665889 = maiFsbUFBD59725931;     maiFsbUFBD59725931 = maiFsbUFBD68367799;     maiFsbUFBD68367799 = maiFsbUFBD63583680;     maiFsbUFBD63583680 = maiFsbUFBD69068067;     maiFsbUFBD69068067 = maiFsbUFBD6212157;     maiFsbUFBD6212157 = maiFsbUFBD30042682;     maiFsbUFBD30042682 = maiFsbUFBD36738416;     maiFsbUFBD36738416 = maiFsbUFBD28704590;     maiFsbUFBD28704590 = maiFsbUFBD29054503;     maiFsbUFBD29054503 = maiFsbUFBD44737500;     maiFsbUFBD44737500 = maiFsbUFBD68269771;     maiFsbUFBD68269771 = maiFsbUFBD97888929;     maiFsbUFBD97888929 = maiFsbUFBD45088213;     maiFsbUFBD45088213 = maiFsbUFBD3568220;     maiFsbUFBD3568220 = maiFsbUFBD54174893;     maiFsbUFBD54174893 = maiFsbUFBD7624647;     maiFsbUFBD7624647 = maiFsbUFBD16573271;     maiFsbUFBD16573271 = maiFsbUFBD56327607;     maiFsbUFBD56327607 = maiFsbUFBD10715747;     maiFsbUFBD10715747 = maiFsbUFBD69804472;     maiFsbUFBD69804472 = maiFsbUFBD50340178;     maiFsbUFBD50340178 = maiFsbUFBD72140372;     maiFsbUFBD72140372 = maiFsbUFBD79131120;     maiFsbUFBD79131120 = maiFsbUFBD26718109;     maiFsbUFBD26718109 = maiFsbUFBD52680572;     maiFsbUFBD52680572 = maiFsbUFBD39594165;     maiFsbUFBD39594165 = maiFsbUFBD7959919;     maiFsbUFBD7959919 = maiFsbUFBD25107953;     maiFsbUFBD25107953 = maiFsbUFBD25494082;     maiFsbUFBD25494082 = maiFsbUFBD92655030;     maiFsbUFBD92655030 = maiFsbUFBD42112923;     maiFsbUFBD42112923 = maiFsbUFBD13309729;     maiFsbUFBD13309729 = maiFsbUFBD66029961;     maiFsbUFBD66029961 = maiFsbUFBD95140647;     maiFsbUFBD95140647 = maiFsbUFBD85147464;     maiFsbUFBD85147464 = maiFsbUFBD65272168;     maiFsbUFBD65272168 = maiFsbUFBD18847296;     maiFsbUFBD18847296 = maiFsbUFBD20301947;     maiFsbUFBD20301947 = maiFsbUFBD43259787;     maiFsbUFBD43259787 = maiFsbUFBD49513453;     maiFsbUFBD49513453 = maiFsbUFBD36045019;     maiFsbUFBD36045019 = maiFsbUFBD34456508;     maiFsbUFBD34456508 = maiFsbUFBD50409299;     maiFsbUFBD50409299 = maiFsbUFBD80418581;     maiFsbUFBD80418581 = maiFsbUFBD61006077;     maiFsbUFBD61006077 = maiFsbUFBD72304838;     maiFsbUFBD72304838 = maiFsbUFBD52482534;     maiFsbUFBD52482534 = maiFsbUFBD59256496;     maiFsbUFBD59256496 = maiFsbUFBD55010334;     maiFsbUFBD55010334 = maiFsbUFBD44348660;     maiFsbUFBD44348660 = maiFsbUFBD83687809;     maiFsbUFBD83687809 = maiFsbUFBD32624804;     maiFsbUFBD32624804 = maiFsbUFBD4985617;     maiFsbUFBD4985617 = maiFsbUFBD3564388;     maiFsbUFBD3564388 = maiFsbUFBD15325711;     maiFsbUFBD15325711 = maiFsbUFBD87585558;     maiFsbUFBD87585558 = maiFsbUFBD89236678;     maiFsbUFBD89236678 = maiFsbUFBD36865572;     maiFsbUFBD36865572 = maiFsbUFBD16387496;     maiFsbUFBD16387496 = maiFsbUFBD66617992;     maiFsbUFBD66617992 = maiFsbUFBD22082763;     maiFsbUFBD22082763 = maiFsbUFBD11630463;     maiFsbUFBD11630463 = maiFsbUFBD3210509;     maiFsbUFBD3210509 = maiFsbUFBD36399472;     maiFsbUFBD36399472 = maiFsbUFBD2624578;     maiFsbUFBD2624578 = maiFsbUFBD54960043;     maiFsbUFBD54960043 = maiFsbUFBD31858968;     maiFsbUFBD31858968 = maiFsbUFBD49947566;     maiFsbUFBD49947566 = maiFsbUFBD18420755;     maiFsbUFBD18420755 = maiFsbUFBD88902725;     maiFsbUFBD88902725 = maiFsbUFBD88777350;     maiFsbUFBD88777350 = maiFsbUFBD96271324;     maiFsbUFBD96271324 = maiFsbUFBD13067820;     maiFsbUFBD13067820 = maiFsbUFBD61202294;     maiFsbUFBD61202294 = maiFsbUFBD33759454;     maiFsbUFBD33759454 = maiFsbUFBD15883671;     maiFsbUFBD15883671 = maiFsbUFBD21731074;     maiFsbUFBD21731074 = maiFsbUFBD98712538;     maiFsbUFBD98712538 = maiFsbUFBD65712031;     maiFsbUFBD65712031 = maiFsbUFBD44873396;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void pSPefXAakH97451510() {     int hMcWQpFJOO67260513 = -850270895;    int hMcWQpFJOO17987468 = 9684884;    int hMcWQpFJOO11200255 = -703916192;    int hMcWQpFJOO4376703 = -235333720;    int hMcWQpFJOO47646197 = -698457359;    int hMcWQpFJOO94439251 = -761968951;    int hMcWQpFJOO64939173 = -717617988;    int hMcWQpFJOO38794956 = -279031412;    int hMcWQpFJOO76626933 = -197074724;    int hMcWQpFJOO16919667 = -964777031;    int hMcWQpFJOO66817708 = 76556571;    int hMcWQpFJOO26096717 = -414354912;    int hMcWQpFJOO64578927 = -716918359;    int hMcWQpFJOO5439467 = -384649796;    int hMcWQpFJOO45920504 = -884417969;    int hMcWQpFJOO96229218 = 20070053;    int hMcWQpFJOO50084432 = 91280299;    int hMcWQpFJOO94928826 = -8036695;    int hMcWQpFJOO56982994 = -980994029;    int hMcWQpFJOO51885143 = -396900407;    int hMcWQpFJOO66548996 = -924980935;    int hMcWQpFJOO23541150 = 40138257;    int hMcWQpFJOO71661122 = -48195942;    int hMcWQpFJOO90083467 = -14799703;    int hMcWQpFJOO4608314 = -994375432;    int hMcWQpFJOO94515151 = -832609824;    int hMcWQpFJOO77132101 = -896005156;    int hMcWQpFJOO42649866 = -573327636;    int hMcWQpFJOO35597151 = -862640569;    int hMcWQpFJOO38211682 = -152441594;    int hMcWQpFJOO89028305 = 12546284;    int hMcWQpFJOO25810963 = -927672023;    int hMcWQpFJOO78927600 = -533577956;    int hMcWQpFJOO4476900 = -541437904;    int hMcWQpFJOO79797583 = -667627042;    int hMcWQpFJOO95867762 = -440825406;    int hMcWQpFJOO1064359 = 74224786;    int hMcWQpFJOO70199372 = -410764197;    int hMcWQpFJOO4635738 = -58330087;    int hMcWQpFJOO30901462 = -834469263;    int hMcWQpFJOO98561843 = -478676636;    int hMcWQpFJOO38848570 = -244913454;    int hMcWQpFJOO77690564 = -124203250;    int hMcWQpFJOO78575239 = -225904730;    int hMcWQpFJOO94701375 = -579696153;    int hMcWQpFJOO60780853 = 59310501;    int hMcWQpFJOO51891562 = -403658358;    int hMcWQpFJOO52558593 = -840609383;    int hMcWQpFJOO19567858 = -549030423;    int hMcWQpFJOO22577828 = -770532624;    int hMcWQpFJOO1524247 = -152282407;    int hMcWQpFJOO15964181 = -499804900;    int hMcWQpFJOO91624131 = -586977519;    int hMcWQpFJOO26316507 = 54423070;    int hMcWQpFJOO30902515 = -484378143;    int hMcWQpFJOO43719364 = -790409152;    int hMcWQpFJOO46326345 = -942119175;    int hMcWQpFJOO21116788 = -589116490;    int hMcWQpFJOO99768388 = -240958288;    int hMcWQpFJOO53131046 = -865847535;    int hMcWQpFJOO17307150 = -865963795;    int hMcWQpFJOO22289308 = -44290353;    int hMcWQpFJOO3197806 = -416390843;    int hMcWQpFJOO38415251 = 55366869;    int hMcWQpFJOO27891361 = -877323315;    int hMcWQpFJOO41006746 = 4228594;    int hMcWQpFJOO47169117 = -880776956;    int hMcWQpFJOO60102027 = -75480456;    int hMcWQpFJOO25641884 = -717022754;    int hMcWQpFJOO50052742 = -343592564;    int hMcWQpFJOO95164860 = 45845266;    int hMcWQpFJOO79885060 = -497955504;    int hMcWQpFJOO90293088 = -949706609;    int hMcWQpFJOO26081533 = -46524766;    int hMcWQpFJOO53323300 = -918223771;    int hMcWQpFJOO27700427 = -580067482;    int hMcWQpFJOO45850585 = -835658493;    int hMcWQpFJOO93085882 = -822291212;    int hMcWQpFJOO95382092 = -435103550;    int hMcWQpFJOO43827460 = -953685934;    int hMcWQpFJOO42623589 = -328951467;    int hMcWQpFJOO24573509 = 44604227;    int hMcWQpFJOO23082008 = 75702787;    int hMcWQpFJOO13019324 = 7892054;    int hMcWQpFJOO36687435 = 99840813;    int hMcWQpFJOO73064125 = -487648817;    int hMcWQpFJOO34186831 = -240694505;    int hMcWQpFJOO52611093 = -488001026;    int hMcWQpFJOO73574385 = 42940238;    int hMcWQpFJOO36078220 = -877217891;    int hMcWQpFJOO49541417 = -498706231;    int hMcWQpFJOO79947571 = -336658724;    int hMcWQpFJOO70430984 = -69805910;    int hMcWQpFJOO51504692 = -192482553;    int hMcWQpFJOO13594312 = -968505468;    int hMcWQpFJOO76272535 = -334386284;    int hMcWQpFJOO35650764 = -828522611;    int hMcWQpFJOO39275314 = -79570120;    int hMcWQpFJOO50683879 = -348581416;    int hMcWQpFJOO53694629 = -850270895;     hMcWQpFJOO67260513 = hMcWQpFJOO17987468;     hMcWQpFJOO17987468 = hMcWQpFJOO11200255;     hMcWQpFJOO11200255 = hMcWQpFJOO4376703;     hMcWQpFJOO4376703 = hMcWQpFJOO47646197;     hMcWQpFJOO47646197 = hMcWQpFJOO94439251;     hMcWQpFJOO94439251 = hMcWQpFJOO64939173;     hMcWQpFJOO64939173 = hMcWQpFJOO38794956;     hMcWQpFJOO38794956 = hMcWQpFJOO76626933;     hMcWQpFJOO76626933 = hMcWQpFJOO16919667;     hMcWQpFJOO16919667 = hMcWQpFJOO66817708;     hMcWQpFJOO66817708 = hMcWQpFJOO26096717;     hMcWQpFJOO26096717 = hMcWQpFJOO64578927;     hMcWQpFJOO64578927 = hMcWQpFJOO5439467;     hMcWQpFJOO5439467 = hMcWQpFJOO45920504;     hMcWQpFJOO45920504 = hMcWQpFJOO96229218;     hMcWQpFJOO96229218 = hMcWQpFJOO50084432;     hMcWQpFJOO50084432 = hMcWQpFJOO94928826;     hMcWQpFJOO94928826 = hMcWQpFJOO56982994;     hMcWQpFJOO56982994 = hMcWQpFJOO51885143;     hMcWQpFJOO51885143 = hMcWQpFJOO66548996;     hMcWQpFJOO66548996 = hMcWQpFJOO23541150;     hMcWQpFJOO23541150 = hMcWQpFJOO71661122;     hMcWQpFJOO71661122 = hMcWQpFJOO90083467;     hMcWQpFJOO90083467 = hMcWQpFJOO4608314;     hMcWQpFJOO4608314 = hMcWQpFJOO94515151;     hMcWQpFJOO94515151 = hMcWQpFJOO77132101;     hMcWQpFJOO77132101 = hMcWQpFJOO42649866;     hMcWQpFJOO42649866 = hMcWQpFJOO35597151;     hMcWQpFJOO35597151 = hMcWQpFJOO38211682;     hMcWQpFJOO38211682 = hMcWQpFJOO89028305;     hMcWQpFJOO89028305 = hMcWQpFJOO25810963;     hMcWQpFJOO25810963 = hMcWQpFJOO78927600;     hMcWQpFJOO78927600 = hMcWQpFJOO4476900;     hMcWQpFJOO4476900 = hMcWQpFJOO79797583;     hMcWQpFJOO79797583 = hMcWQpFJOO95867762;     hMcWQpFJOO95867762 = hMcWQpFJOO1064359;     hMcWQpFJOO1064359 = hMcWQpFJOO70199372;     hMcWQpFJOO70199372 = hMcWQpFJOO4635738;     hMcWQpFJOO4635738 = hMcWQpFJOO30901462;     hMcWQpFJOO30901462 = hMcWQpFJOO98561843;     hMcWQpFJOO98561843 = hMcWQpFJOO38848570;     hMcWQpFJOO38848570 = hMcWQpFJOO77690564;     hMcWQpFJOO77690564 = hMcWQpFJOO78575239;     hMcWQpFJOO78575239 = hMcWQpFJOO94701375;     hMcWQpFJOO94701375 = hMcWQpFJOO60780853;     hMcWQpFJOO60780853 = hMcWQpFJOO51891562;     hMcWQpFJOO51891562 = hMcWQpFJOO52558593;     hMcWQpFJOO52558593 = hMcWQpFJOO19567858;     hMcWQpFJOO19567858 = hMcWQpFJOO22577828;     hMcWQpFJOO22577828 = hMcWQpFJOO1524247;     hMcWQpFJOO1524247 = hMcWQpFJOO15964181;     hMcWQpFJOO15964181 = hMcWQpFJOO91624131;     hMcWQpFJOO91624131 = hMcWQpFJOO26316507;     hMcWQpFJOO26316507 = hMcWQpFJOO30902515;     hMcWQpFJOO30902515 = hMcWQpFJOO43719364;     hMcWQpFJOO43719364 = hMcWQpFJOO46326345;     hMcWQpFJOO46326345 = hMcWQpFJOO21116788;     hMcWQpFJOO21116788 = hMcWQpFJOO99768388;     hMcWQpFJOO99768388 = hMcWQpFJOO53131046;     hMcWQpFJOO53131046 = hMcWQpFJOO17307150;     hMcWQpFJOO17307150 = hMcWQpFJOO22289308;     hMcWQpFJOO22289308 = hMcWQpFJOO3197806;     hMcWQpFJOO3197806 = hMcWQpFJOO38415251;     hMcWQpFJOO38415251 = hMcWQpFJOO27891361;     hMcWQpFJOO27891361 = hMcWQpFJOO41006746;     hMcWQpFJOO41006746 = hMcWQpFJOO47169117;     hMcWQpFJOO47169117 = hMcWQpFJOO60102027;     hMcWQpFJOO60102027 = hMcWQpFJOO25641884;     hMcWQpFJOO25641884 = hMcWQpFJOO50052742;     hMcWQpFJOO50052742 = hMcWQpFJOO95164860;     hMcWQpFJOO95164860 = hMcWQpFJOO79885060;     hMcWQpFJOO79885060 = hMcWQpFJOO90293088;     hMcWQpFJOO90293088 = hMcWQpFJOO26081533;     hMcWQpFJOO26081533 = hMcWQpFJOO53323300;     hMcWQpFJOO53323300 = hMcWQpFJOO27700427;     hMcWQpFJOO27700427 = hMcWQpFJOO45850585;     hMcWQpFJOO45850585 = hMcWQpFJOO93085882;     hMcWQpFJOO93085882 = hMcWQpFJOO95382092;     hMcWQpFJOO95382092 = hMcWQpFJOO43827460;     hMcWQpFJOO43827460 = hMcWQpFJOO42623589;     hMcWQpFJOO42623589 = hMcWQpFJOO24573509;     hMcWQpFJOO24573509 = hMcWQpFJOO23082008;     hMcWQpFJOO23082008 = hMcWQpFJOO13019324;     hMcWQpFJOO13019324 = hMcWQpFJOO36687435;     hMcWQpFJOO36687435 = hMcWQpFJOO73064125;     hMcWQpFJOO73064125 = hMcWQpFJOO34186831;     hMcWQpFJOO34186831 = hMcWQpFJOO52611093;     hMcWQpFJOO52611093 = hMcWQpFJOO73574385;     hMcWQpFJOO73574385 = hMcWQpFJOO36078220;     hMcWQpFJOO36078220 = hMcWQpFJOO49541417;     hMcWQpFJOO49541417 = hMcWQpFJOO79947571;     hMcWQpFJOO79947571 = hMcWQpFJOO70430984;     hMcWQpFJOO70430984 = hMcWQpFJOO51504692;     hMcWQpFJOO51504692 = hMcWQpFJOO13594312;     hMcWQpFJOO13594312 = hMcWQpFJOO76272535;     hMcWQpFJOO76272535 = hMcWQpFJOO35650764;     hMcWQpFJOO35650764 = hMcWQpFJOO39275314;     hMcWQpFJOO39275314 = hMcWQpFJOO50683879;     hMcWQpFJOO50683879 = hMcWQpFJOO53694629;     hMcWQpFJOO53694629 = hMcWQpFJOO67260513;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xaSEqhcXHJ34096485() {     int fIWAFGXuPB89647630 = -484674858;    int fIWAFGXuPB2334968 = 61141829;    int fIWAFGXuPB39969534 = -943570036;    int fIWAFGXuPB19383391 = -241741251;    int fIWAFGXuPB45820452 = -189514360;    int fIWAFGXuPB9322369 = -622577329;    int fIWAFGXuPB57094912 = -539605294;    int fIWAFGXuPB14428815 = -749628056;    int fIWAFGXuPB73790064 = -81270763;    int fIWAFGXuPB8683252 = -707518364;    int fIWAFGXuPB4359568 = 79932170;    int fIWAFGXuPB81999665 = -665100205;    int fIWAFGXuPB31587108 = -235635679;    int fIWAFGXuPB48054219 = -912358553;    int fIWAFGXuPB82655781 = -511895682;    int fIWAFGXuPB40485131 = -361597490;    int fIWAFGXuPB99907784 = -185999520;    int fIWAFGXuPB905243 = -459779386;    int fIWAFGXuPB98264625 = -16096154;    int fIWAFGXuPB30401427 = -649534860;    int fIWAFGXuPB67432103 = -916468243;    int fIWAFGXuPB87356367 = -464628057;    int fIWAFGXuPB74954445 = -224276534;    int fIWAFGXuPB16583255 = -578792560;    int fIWAFGXuPB40148561 = -615934557;    int fIWAFGXuPB82818145 = -176504048;    int fIWAFGXuPB24221522 = -653591649;    int fIWAFGXuPB48561316 = -361782090;    int fIWAFGXuPB42489712 = -106897688;    int fIWAFGXuPB47368862 = -72737461;    int fIWAFGXuPB33319111 = -68769868;    int fIWAFGXuPB83352153 = -479743378;    int fIWAFGXuPB59966270 = 37078549;    int fIWAFGXuPB63865587 = -865564754;    int fIWAFGXuPB56026948 = -731383464;    int fIWAFGXuPB37560632 = -756220782;    int fIWAFGXuPB94504070 = -758723165;    int fIWAFGXuPB23825473 = -278389669;    int fIWAFGXuPB52943869 = -459376119;    int fIWAFGXuPB51087176 = -970693292;    int fIWAFGXuPB27319214 = -981854302;    int fIWAFGXuPB27356961 = -2822145;    int fIWAFGXuPB83240756 = -11733834;    int fIWAFGXuPB78019358 = -223268379;    int fIWAFGXuPB62684641 = -388839631;    int fIWAFGXuPB68881135 = -122918594;    int fIWAFGXuPB64188959 = -982068611;    int fIWAFGXuPB97157267 = -597144374;    int fIWAFGXuPB14027764 = -498747122;    int fIWAFGXuPB19661573 = -912552883;    int fIWAFGXuPB10393464 = -507358684;    int fIWAFGXuPB89815437 = -930624182;    int fIWAFGXuPB69938535 = -258888749;    int fIWAFGXuPB86603052 = -221257739;    int fIWAFGXuPB66664382 = -625082259;    int fIWAFGXuPB2291263 = 79953198;    int fIWAFGXuPB27380523 = -714581638;    int fIWAFGXuPB23386279 = -264777477;    int fIWAFGXuPB79234829 = -625806695;    int fIWAFGXuPB63002306 = 86989688;    int fIWAFGXuPB85100846 = -968985681;    int fIWAFGXuPB8533597 = -77823205;    int fIWAFGXuPB71939103 = -542730368;    int fIWAFGXuPB26421203 = 91466697;    int fIWAFGXuPB75364140 = -538748496;    int fIWAFGXuPB21007415 = -440324453;    int fIWAFGXuPB22033395 = -602178755;    int fIWAFGXuPB67721520 = -370070925;    int fIWAFGXuPB92027271 = -80975089;    int fIWAFGXuPB45095150 = -755674901;    int fIWAFGXuPB45981060 = -602874325;    int fIWAFGXuPB76082311 = -907609852;    int fIWAFGXuPB47961373 = 99596732;    int fIWAFGXuPB47177449 = -45402862;    int fIWAFGXuPB3082213 = -667680558;    int fIWAFGXuPB40075142 = -813646099;    int fIWAFGXuPB4115611 = -352894223;    int fIWAFGXuPB96935086 = 98991845;    int fIWAFGXuPB53898613 = -89952930;    int fIWAFGXuPB71267425 = -393015963;    int fIWAFGXuPB18629186 = -194435438;    int fIWAFGXuPB27064255 = 43552725;    int fIWAFGXuPB34533553 = -863034969;    int fIWAFGXuPB22828139 = -194344806;    int fIWAFGXuPB36975398 = -565378777;    int fIWAFGXuPB43503673 = -138145687;    int fIWAFGXuPB13413619 = -120854629;    int fIWAFGXuPB73363218 = -741663713;    int fIWAFGXuPB97201205 = -140482496;    int fIWAFGXuPB53735685 = -711336663;    int fIWAFGXuPB10180110 = 58360855;    int fIWAFGXuPB71117792 = -393945689;    int fIWAFGXuPB44590643 = -652582975;    int fIWAFGXuPB89941563 = -446365807;    int fIWAFGXuPB65986330 = 98292388;    int fIWAFGXuPB18785618 = -804031098;    int fIWAFGXuPB55417857 = -460091777;    int fIWAFGXuPB56819554 = -3200532;    int fIWAFGXuPB2655219 = -684519884;    int fIWAFGXuPB41677227 = -484674858;     fIWAFGXuPB89647630 = fIWAFGXuPB2334968;     fIWAFGXuPB2334968 = fIWAFGXuPB39969534;     fIWAFGXuPB39969534 = fIWAFGXuPB19383391;     fIWAFGXuPB19383391 = fIWAFGXuPB45820452;     fIWAFGXuPB45820452 = fIWAFGXuPB9322369;     fIWAFGXuPB9322369 = fIWAFGXuPB57094912;     fIWAFGXuPB57094912 = fIWAFGXuPB14428815;     fIWAFGXuPB14428815 = fIWAFGXuPB73790064;     fIWAFGXuPB73790064 = fIWAFGXuPB8683252;     fIWAFGXuPB8683252 = fIWAFGXuPB4359568;     fIWAFGXuPB4359568 = fIWAFGXuPB81999665;     fIWAFGXuPB81999665 = fIWAFGXuPB31587108;     fIWAFGXuPB31587108 = fIWAFGXuPB48054219;     fIWAFGXuPB48054219 = fIWAFGXuPB82655781;     fIWAFGXuPB82655781 = fIWAFGXuPB40485131;     fIWAFGXuPB40485131 = fIWAFGXuPB99907784;     fIWAFGXuPB99907784 = fIWAFGXuPB905243;     fIWAFGXuPB905243 = fIWAFGXuPB98264625;     fIWAFGXuPB98264625 = fIWAFGXuPB30401427;     fIWAFGXuPB30401427 = fIWAFGXuPB67432103;     fIWAFGXuPB67432103 = fIWAFGXuPB87356367;     fIWAFGXuPB87356367 = fIWAFGXuPB74954445;     fIWAFGXuPB74954445 = fIWAFGXuPB16583255;     fIWAFGXuPB16583255 = fIWAFGXuPB40148561;     fIWAFGXuPB40148561 = fIWAFGXuPB82818145;     fIWAFGXuPB82818145 = fIWAFGXuPB24221522;     fIWAFGXuPB24221522 = fIWAFGXuPB48561316;     fIWAFGXuPB48561316 = fIWAFGXuPB42489712;     fIWAFGXuPB42489712 = fIWAFGXuPB47368862;     fIWAFGXuPB47368862 = fIWAFGXuPB33319111;     fIWAFGXuPB33319111 = fIWAFGXuPB83352153;     fIWAFGXuPB83352153 = fIWAFGXuPB59966270;     fIWAFGXuPB59966270 = fIWAFGXuPB63865587;     fIWAFGXuPB63865587 = fIWAFGXuPB56026948;     fIWAFGXuPB56026948 = fIWAFGXuPB37560632;     fIWAFGXuPB37560632 = fIWAFGXuPB94504070;     fIWAFGXuPB94504070 = fIWAFGXuPB23825473;     fIWAFGXuPB23825473 = fIWAFGXuPB52943869;     fIWAFGXuPB52943869 = fIWAFGXuPB51087176;     fIWAFGXuPB51087176 = fIWAFGXuPB27319214;     fIWAFGXuPB27319214 = fIWAFGXuPB27356961;     fIWAFGXuPB27356961 = fIWAFGXuPB83240756;     fIWAFGXuPB83240756 = fIWAFGXuPB78019358;     fIWAFGXuPB78019358 = fIWAFGXuPB62684641;     fIWAFGXuPB62684641 = fIWAFGXuPB68881135;     fIWAFGXuPB68881135 = fIWAFGXuPB64188959;     fIWAFGXuPB64188959 = fIWAFGXuPB97157267;     fIWAFGXuPB97157267 = fIWAFGXuPB14027764;     fIWAFGXuPB14027764 = fIWAFGXuPB19661573;     fIWAFGXuPB19661573 = fIWAFGXuPB10393464;     fIWAFGXuPB10393464 = fIWAFGXuPB89815437;     fIWAFGXuPB89815437 = fIWAFGXuPB69938535;     fIWAFGXuPB69938535 = fIWAFGXuPB86603052;     fIWAFGXuPB86603052 = fIWAFGXuPB66664382;     fIWAFGXuPB66664382 = fIWAFGXuPB2291263;     fIWAFGXuPB2291263 = fIWAFGXuPB27380523;     fIWAFGXuPB27380523 = fIWAFGXuPB23386279;     fIWAFGXuPB23386279 = fIWAFGXuPB79234829;     fIWAFGXuPB79234829 = fIWAFGXuPB63002306;     fIWAFGXuPB63002306 = fIWAFGXuPB85100846;     fIWAFGXuPB85100846 = fIWAFGXuPB8533597;     fIWAFGXuPB8533597 = fIWAFGXuPB71939103;     fIWAFGXuPB71939103 = fIWAFGXuPB26421203;     fIWAFGXuPB26421203 = fIWAFGXuPB75364140;     fIWAFGXuPB75364140 = fIWAFGXuPB21007415;     fIWAFGXuPB21007415 = fIWAFGXuPB22033395;     fIWAFGXuPB22033395 = fIWAFGXuPB67721520;     fIWAFGXuPB67721520 = fIWAFGXuPB92027271;     fIWAFGXuPB92027271 = fIWAFGXuPB45095150;     fIWAFGXuPB45095150 = fIWAFGXuPB45981060;     fIWAFGXuPB45981060 = fIWAFGXuPB76082311;     fIWAFGXuPB76082311 = fIWAFGXuPB47961373;     fIWAFGXuPB47961373 = fIWAFGXuPB47177449;     fIWAFGXuPB47177449 = fIWAFGXuPB3082213;     fIWAFGXuPB3082213 = fIWAFGXuPB40075142;     fIWAFGXuPB40075142 = fIWAFGXuPB4115611;     fIWAFGXuPB4115611 = fIWAFGXuPB96935086;     fIWAFGXuPB96935086 = fIWAFGXuPB53898613;     fIWAFGXuPB53898613 = fIWAFGXuPB71267425;     fIWAFGXuPB71267425 = fIWAFGXuPB18629186;     fIWAFGXuPB18629186 = fIWAFGXuPB27064255;     fIWAFGXuPB27064255 = fIWAFGXuPB34533553;     fIWAFGXuPB34533553 = fIWAFGXuPB22828139;     fIWAFGXuPB22828139 = fIWAFGXuPB36975398;     fIWAFGXuPB36975398 = fIWAFGXuPB43503673;     fIWAFGXuPB43503673 = fIWAFGXuPB13413619;     fIWAFGXuPB13413619 = fIWAFGXuPB73363218;     fIWAFGXuPB73363218 = fIWAFGXuPB97201205;     fIWAFGXuPB97201205 = fIWAFGXuPB53735685;     fIWAFGXuPB53735685 = fIWAFGXuPB10180110;     fIWAFGXuPB10180110 = fIWAFGXuPB71117792;     fIWAFGXuPB71117792 = fIWAFGXuPB44590643;     fIWAFGXuPB44590643 = fIWAFGXuPB89941563;     fIWAFGXuPB89941563 = fIWAFGXuPB65986330;     fIWAFGXuPB65986330 = fIWAFGXuPB18785618;     fIWAFGXuPB18785618 = fIWAFGXuPB55417857;     fIWAFGXuPB55417857 = fIWAFGXuPB56819554;     fIWAFGXuPB56819554 = fIWAFGXuPB2655219;     fIWAFGXuPB2655219 = fIWAFGXuPB41677227;     fIWAFGXuPB41677227 = fIWAFGXuPB89647630;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void NAHJaCyFhI70741460() {     int XBUuWAbuve12034748 = -119078821;    int XBUuWAbuve86682468 = -987401226;    int XBUuWAbuve68738812 = -83223880;    int XBUuWAbuve34390079 = -248148783;    int XBUuWAbuve43994706 = -780571361;    int XBUuWAbuve24205486 = -483185707;    int XBUuWAbuve49250651 = -361592601;    int XBUuWAbuve90062673 = -120224700;    int XBUuWAbuve70953195 = 34533197;    int XBUuWAbuve446836 = -450259696;    int XBUuWAbuve41901427 = 83307769;    int XBUuWAbuve37902615 = -915845499;    int XBUuWAbuve98595288 = -854352999;    int XBUuWAbuve90668971 = -340067310;    int XBUuWAbuve19391059 = -139373395;    int XBUuWAbuve84741043 = -743265032;    int XBUuWAbuve49731136 = -463279340;    int XBUuWAbuve6881659 = -911522077;    int XBUuWAbuve39546256 = -151198279;    int XBUuWAbuve8917710 = -902169314;    int XBUuWAbuve68315210 = -907955552;    int XBUuWAbuve51171586 = -969394370;    int XBUuWAbuve78247768 = -400357126;    int XBUuWAbuve43083042 = -42785417;    int XBUuWAbuve75688807 = -237493681;    int XBUuWAbuve71121140 = -620398272;    int XBUuWAbuve71310942 = -411178142;    int XBUuWAbuve54472766 = -150236543;    int XBUuWAbuve49382272 = -451154807;    int XBUuWAbuve56526041 = 6966672;    int XBUuWAbuve77609916 = -150086020;    int XBUuWAbuve40893344 = -31814733;    int XBUuWAbuve41004941 = -492264946;    int XBUuWAbuve23254275 = -89691604;    int XBUuWAbuve32256313 = -795139887;    int XBUuWAbuve79253501 = 28383841;    int XBUuWAbuve87943783 = -491671117;    int XBUuWAbuve77451573 = -146015141;    int XBUuWAbuve1252001 = -860422150;    int XBUuWAbuve71272891 = -6917321;    int XBUuWAbuve56076585 = -385031968;    int XBUuWAbuve15865352 = -860730836;    int XBUuWAbuve88790948 = -999264418;    int XBUuWAbuve77463478 = -220632028;    int XBUuWAbuve30667908 = -197983108;    int XBUuWAbuve76981417 = -305147690;    int XBUuWAbuve76486357 = -460478863;    int XBUuWAbuve41755942 = -353679365;    int XBUuWAbuve8487669 = -448463820;    int XBUuWAbuve16745319 = 45426858;    int XBUuWAbuve19262680 = -862434961;    int XBUuWAbuve63666695 = -261443464;    int XBUuWAbuve48252938 = 69200021;    int XBUuWAbuve46889597 = -496938548;    int XBUuWAbuve2426250 = -765786374;    int XBUuWAbuve60863162 = -149684451;    int XBUuWAbuve8434700 = -487044101;    int XBUuWAbuve25655771 = 59561536;    int XBUuWAbuve58701271 = 89344898;    int XBUuWAbuve72873565 = -60173089;    int XBUuWAbuve52894543 = 27992434;    int XBUuWAbuve94777885 = -111356058;    int XBUuWAbuve40680401 = -669069893;    int XBUuWAbuve14427154 = -972433476;    int XBUuWAbuve22836920 = -200173677;    int XBUuWAbuve1008084 = -884877499;    int XBUuWAbuve96897673 = -323580553;    int XBUuWAbuve75341013 = -664661395;    int XBUuWAbuve58412659 = -544927423;    int XBUuWAbuve40137558 = -67757237;    int XBUuWAbuve96797260 = -151593916;    int XBUuWAbuve72279562 = -217264200;    int XBUuWAbuve5629659 = 48900073;    int XBUuWAbuve68273365 = -44280958;    int XBUuWAbuve52841125 = -417137346;    int XBUuWAbuve52449858 = 52775284;    int XBUuWAbuve62380637 = -970129953;    int XBUuWAbuve784291 = -79725098;    int XBUuWAbuve12415134 = -844802309;    int XBUuWAbuve98707390 = -932345992;    int XBUuWAbuve94634783 = -59919409;    int XBUuWAbuve29555001 = 42501223;    int XBUuWAbuve45985098 = -701772724;    int XBUuWAbuve32636954 = -396581666;    int XBUuWAbuve37263361 = -130598367;    int XBUuWAbuve13943221 = -888642557;    int XBUuWAbuve92640406 = -1014754;    int XBUuWAbuve94115343 = -995326399;    int XBUuWAbuve20828025 = -323905231;    int XBUuWAbuve71393151 = -545455436;    int XBUuWAbuve70818801 = -484572059;    int XBUuWAbuve62288012 = -451232653;    int XBUuWAbuve18750303 = -135360039;    int XBUuWAbuve28378435 = -700249061;    int XBUuWAbuve18378348 = 65090244;    int XBUuWAbuve61298700 = -173675911;    int XBUuWAbuve75184951 = -91660943;    int XBUuWAbuve74363795 = 73169057;    int XBUuWAbuve54626558 = 79541648;    int XBUuWAbuve29659825 = -119078821;     XBUuWAbuve12034748 = XBUuWAbuve86682468;     XBUuWAbuve86682468 = XBUuWAbuve68738812;     XBUuWAbuve68738812 = XBUuWAbuve34390079;     XBUuWAbuve34390079 = XBUuWAbuve43994706;     XBUuWAbuve43994706 = XBUuWAbuve24205486;     XBUuWAbuve24205486 = XBUuWAbuve49250651;     XBUuWAbuve49250651 = XBUuWAbuve90062673;     XBUuWAbuve90062673 = XBUuWAbuve70953195;     XBUuWAbuve70953195 = XBUuWAbuve446836;     XBUuWAbuve446836 = XBUuWAbuve41901427;     XBUuWAbuve41901427 = XBUuWAbuve37902615;     XBUuWAbuve37902615 = XBUuWAbuve98595288;     XBUuWAbuve98595288 = XBUuWAbuve90668971;     XBUuWAbuve90668971 = XBUuWAbuve19391059;     XBUuWAbuve19391059 = XBUuWAbuve84741043;     XBUuWAbuve84741043 = XBUuWAbuve49731136;     XBUuWAbuve49731136 = XBUuWAbuve6881659;     XBUuWAbuve6881659 = XBUuWAbuve39546256;     XBUuWAbuve39546256 = XBUuWAbuve8917710;     XBUuWAbuve8917710 = XBUuWAbuve68315210;     XBUuWAbuve68315210 = XBUuWAbuve51171586;     XBUuWAbuve51171586 = XBUuWAbuve78247768;     XBUuWAbuve78247768 = XBUuWAbuve43083042;     XBUuWAbuve43083042 = XBUuWAbuve75688807;     XBUuWAbuve75688807 = XBUuWAbuve71121140;     XBUuWAbuve71121140 = XBUuWAbuve71310942;     XBUuWAbuve71310942 = XBUuWAbuve54472766;     XBUuWAbuve54472766 = XBUuWAbuve49382272;     XBUuWAbuve49382272 = XBUuWAbuve56526041;     XBUuWAbuve56526041 = XBUuWAbuve77609916;     XBUuWAbuve77609916 = XBUuWAbuve40893344;     XBUuWAbuve40893344 = XBUuWAbuve41004941;     XBUuWAbuve41004941 = XBUuWAbuve23254275;     XBUuWAbuve23254275 = XBUuWAbuve32256313;     XBUuWAbuve32256313 = XBUuWAbuve79253501;     XBUuWAbuve79253501 = XBUuWAbuve87943783;     XBUuWAbuve87943783 = XBUuWAbuve77451573;     XBUuWAbuve77451573 = XBUuWAbuve1252001;     XBUuWAbuve1252001 = XBUuWAbuve71272891;     XBUuWAbuve71272891 = XBUuWAbuve56076585;     XBUuWAbuve56076585 = XBUuWAbuve15865352;     XBUuWAbuve15865352 = XBUuWAbuve88790948;     XBUuWAbuve88790948 = XBUuWAbuve77463478;     XBUuWAbuve77463478 = XBUuWAbuve30667908;     XBUuWAbuve30667908 = XBUuWAbuve76981417;     XBUuWAbuve76981417 = XBUuWAbuve76486357;     XBUuWAbuve76486357 = XBUuWAbuve41755942;     XBUuWAbuve41755942 = XBUuWAbuve8487669;     XBUuWAbuve8487669 = XBUuWAbuve16745319;     XBUuWAbuve16745319 = XBUuWAbuve19262680;     XBUuWAbuve19262680 = XBUuWAbuve63666695;     XBUuWAbuve63666695 = XBUuWAbuve48252938;     XBUuWAbuve48252938 = XBUuWAbuve46889597;     XBUuWAbuve46889597 = XBUuWAbuve2426250;     XBUuWAbuve2426250 = XBUuWAbuve60863162;     XBUuWAbuve60863162 = XBUuWAbuve8434700;     XBUuWAbuve8434700 = XBUuWAbuve25655771;     XBUuWAbuve25655771 = XBUuWAbuve58701271;     XBUuWAbuve58701271 = XBUuWAbuve72873565;     XBUuWAbuve72873565 = XBUuWAbuve52894543;     XBUuWAbuve52894543 = XBUuWAbuve94777885;     XBUuWAbuve94777885 = XBUuWAbuve40680401;     XBUuWAbuve40680401 = XBUuWAbuve14427154;     XBUuWAbuve14427154 = XBUuWAbuve22836920;     XBUuWAbuve22836920 = XBUuWAbuve1008084;     XBUuWAbuve1008084 = XBUuWAbuve96897673;     XBUuWAbuve96897673 = XBUuWAbuve75341013;     XBUuWAbuve75341013 = XBUuWAbuve58412659;     XBUuWAbuve58412659 = XBUuWAbuve40137558;     XBUuWAbuve40137558 = XBUuWAbuve96797260;     XBUuWAbuve96797260 = XBUuWAbuve72279562;     XBUuWAbuve72279562 = XBUuWAbuve5629659;     XBUuWAbuve5629659 = XBUuWAbuve68273365;     XBUuWAbuve68273365 = XBUuWAbuve52841125;     XBUuWAbuve52841125 = XBUuWAbuve52449858;     XBUuWAbuve52449858 = XBUuWAbuve62380637;     XBUuWAbuve62380637 = XBUuWAbuve784291;     XBUuWAbuve784291 = XBUuWAbuve12415134;     XBUuWAbuve12415134 = XBUuWAbuve98707390;     XBUuWAbuve98707390 = XBUuWAbuve94634783;     XBUuWAbuve94634783 = XBUuWAbuve29555001;     XBUuWAbuve29555001 = XBUuWAbuve45985098;     XBUuWAbuve45985098 = XBUuWAbuve32636954;     XBUuWAbuve32636954 = XBUuWAbuve37263361;     XBUuWAbuve37263361 = XBUuWAbuve13943221;     XBUuWAbuve13943221 = XBUuWAbuve92640406;     XBUuWAbuve92640406 = XBUuWAbuve94115343;     XBUuWAbuve94115343 = XBUuWAbuve20828025;     XBUuWAbuve20828025 = XBUuWAbuve71393151;     XBUuWAbuve71393151 = XBUuWAbuve70818801;     XBUuWAbuve70818801 = XBUuWAbuve62288012;     XBUuWAbuve62288012 = XBUuWAbuve18750303;     XBUuWAbuve18750303 = XBUuWAbuve28378435;     XBUuWAbuve28378435 = XBUuWAbuve18378348;     XBUuWAbuve18378348 = XBUuWAbuve61298700;     XBUuWAbuve61298700 = XBUuWAbuve75184951;     XBUuWAbuve75184951 = XBUuWAbuve74363795;     XBUuWAbuve74363795 = XBUuWAbuve54626558;     XBUuWAbuve54626558 = XBUuWAbuve29659825;     XBUuWAbuve29659825 = XBUuWAbuve12034748;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void RzSXOdKqAL7386435() {     int wsUYQkoeiW34421865 = -853482784;    int wsUYQkoeiW71029968 = -935944280;    int wsUYQkoeiW97508090 = -322877725;    int wsUYQkoeiW49396767 = -254556314;    int wsUYQkoeiW42168960 = -271628362;    int wsUYQkoeiW39088603 = -343794085;    int wsUYQkoeiW41406390 = -183579907;    int wsUYQkoeiW65696532 = -590821344;    int wsUYQkoeiW68116326 = -949662842;    int wsUYQkoeiW92210420 = -193001029;    int wsUYQkoeiW79443287 = 86683368;    int wsUYQkoeiW93805563 = -66590792;    int wsUYQkoeiW65603469 = -373070318;    int wsUYQkoeiW33283724 = -867776067;    int wsUYQkoeiW56126335 = -866851109;    int wsUYQkoeiW28996956 = -24932575;    int wsUYQkoeiW99554487 = -740559160;    int wsUYQkoeiW12858076 = -263264768;    int wsUYQkoeiW80827887 = -286300404;    int wsUYQkoeiW87433993 = -54803767;    int wsUYQkoeiW69198317 = -899442860;    int wsUYQkoeiW14986805 = -374160683;    int wsUYQkoeiW81541091 = -576437718;    int wsUYQkoeiW69582829 = -606778274;    int wsUYQkoeiW11229055 = -959052806;    int wsUYQkoeiW59424135 = 35707504;    int wsUYQkoeiW18400363 = -168764635;    int wsUYQkoeiW60384216 = 61309003;    int wsUYQkoeiW56274833 = -795411926;    int wsUYQkoeiW65683220 = 86670806;    int wsUYQkoeiW21900721 = -231402172;    int wsUYQkoeiW98434535 = -683886087;    int wsUYQkoeiW22043612 = 78391559;    int wsUYQkoeiW82642962 = -413818455;    int wsUYQkoeiW8485678 = -858896309;    int wsUYQkoeiW20946370 = -287011535;    int wsUYQkoeiW81383495 = -224619068;    int wsUYQkoeiW31077675 = -13640613;    int wsUYQkoeiW49560132 = -161468182;    int wsUYQkoeiW91458605 = -143141351;    int wsUYQkoeiW84833956 = -888209634;    int wsUYQkoeiW4373744 = -618639527;    int wsUYQkoeiW94341140 = -886795002;    int wsUYQkoeiW76907597 = -217995677;    int wsUYQkoeiW98651174 = -7126586;    int wsUYQkoeiW85081699 = -487376785;    int wsUYQkoeiW88783754 = 61110884;    int wsUYQkoeiW86354616 = -110214356;    int wsUYQkoeiW2947574 = -398180518;    int wsUYQkoeiW13829065 = -96593401;    int wsUYQkoeiW28131897 = -117511238;    int wsUYQkoeiW37517952 = -692262746;    int wsUYQkoeiW26567342 = -702711210;    int wsUYQkoeiW7176143 = -772619356;    int wsUYQkoeiW38188117 = -906490489;    int wsUYQkoeiW19435061 = -379322101;    int wsUYQkoeiW89488876 = -259506563;    int wsUYQkoeiW27925262 = -716099451;    int wsUYQkoeiW38167713 = -295503509;    int wsUYQkoeiW82744825 = -207335867;    int wsUYQkoeiW20688240 = -75029451;    int wsUYQkoeiW81022174 = -144888910;    int wsUYQkoeiW9421699 = -795409418;    int wsUYQkoeiW2433106 = -936333648;    int wsUYQkoeiW70309699 = -961598858;    int wsUYQkoeiW81008752 = -229430545;    int wsUYQkoeiW71761952 = -44982352;    int wsUYQkoeiW82960507 = -959251864;    int wsUYQkoeiW24798047 = 91120242;    int wsUYQkoeiW35179966 = -479839574;    int wsUYQkoeiW47613461 = -800313507;    int wsUYQkoeiW68476813 = -626918548;    int wsUYQkoeiW63297943 = -1796586;    int wsUYQkoeiW89369281 = -43159054;    int wsUYQkoeiW2600038 = -166594134;    int wsUYQkoeiW64824574 = -180803334;    int wsUYQkoeiW20645664 = -487365682;    int wsUYQkoeiW4633495 = -258442041;    int wsUYQkoeiW70931654 = -499651689;    int wsUYQkoeiW26147355 = -371676021;    int wsUYQkoeiW70640380 = 74596620;    int wsUYQkoeiW32045746 = 41449721;    int wsUYQkoeiW57436643 = -540510479;    int wsUYQkoeiW42445769 = -598818526;    int wsUYQkoeiW37551324 = -795817956;    int wsUYQkoeiW84382769 = -539139427;    int wsUYQkoeiW71867193 = -981174878;    int wsUYQkoeiW14867469 = -148989085;    int wsUYQkoeiW44454845 = -507327966;    int wsUYQkoeiW89050616 = -379574209;    int wsUYQkoeiW31457493 = 72495027;    int wsUYQkoeiW53458233 = -508519618;    int wsUYQkoeiW92909961 = -718137104;    int wsUYQkoeiW66815307 = -954132316;    int wsUYQkoeiW70770366 = 31888100;    int wsUYQkoeiW3811783 = -643320725;    int wsUYQkoeiW94952044 = -823230109;    int wsUYQkoeiW91908035 = -950461354;    int wsUYQkoeiW6597898 = -256396820;    int wsUYQkoeiW17642423 = -853482784;     wsUYQkoeiW34421865 = wsUYQkoeiW71029968;     wsUYQkoeiW71029968 = wsUYQkoeiW97508090;     wsUYQkoeiW97508090 = wsUYQkoeiW49396767;     wsUYQkoeiW49396767 = wsUYQkoeiW42168960;     wsUYQkoeiW42168960 = wsUYQkoeiW39088603;     wsUYQkoeiW39088603 = wsUYQkoeiW41406390;     wsUYQkoeiW41406390 = wsUYQkoeiW65696532;     wsUYQkoeiW65696532 = wsUYQkoeiW68116326;     wsUYQkoeiW68116326 = wsUYQkoeiW92210420;     wsUYQkoeiW92210420 = wsUYQkoeiW79443287;     wsUYQkoeiW79443287 = wsUYQkoeiW93805563;     wsUYQkoeiW93805563 = wsUYQkoeiW65603469;     wsUYQkoeiW65603469 = wsUYQkoeiW33283724;     wsUYQkoeiW33283724 = wsUYQkoeiW56126335;     wsUYQkoeiW56126335 = wsUYQkoeiW28996956;     wsUYQkoeiW28996956 = wsUYQkoeiW99554487;     wsUYQkoeiW99554487 = wsUYQkoeiW12858076;     wsUYQkoeiW12858076 = wsUYQkoeiW80827887;     wsUYQkoeiW80827887 = wsUYQkoeiW87433993;     wsUYQkoeiW87433993 = wsUYQkoeiW69198317;     wsUYQkoeiW69198317 = wsUYQkoeiW14986805;     wsUYQkoeiW14986805 = wsUYQkoeiW81541091;     wsUYQkoeiW81541091 = wsUYQkoeiW69582829;     wsUYQkoeiW69582829 = wsUYQkoeiW11229055;     wsUYQkoeiW11229055 = wsUYQkoeiW59424135;     wsUYQkoeiW59424135 = wsUYQkoeiW18400363;     wsUYQkoeiW18400363 = wsUYQkoeiW60384216;     wsUYQkoeiW60384216 = wsUYQkoeiW56274833;     wsUYQkoeiW56274833 = wsUYQkoeiW65683220;     wsUYQkoeiW65683220 = wsUYQkoeiW21900721;     wsUYQkoeiW21900721 = wsUYQkoeiW98434535;     wsUYQkoeiW98434535 = wsUYQkoeiW22043612;     wsUYQkoeiW22043612 = wsUYQkoeiW82642962;     wsUYQkoeiW82642962 = wsUYQkoeiW8485678;     wsUYQkoeiW8485678 = wsUYQkoeiW20946370;     wsUYQkoeiW20946370 = wsUYQkoeiW81383495;     wsUYQkoeiW81383495 = wsUYQkoeiW31077675;     wsUYQkoeiW31077675 = wsUYQkoeiW49560132;     wsUYQkoeiW49560132 = wsUYQkoeiW91458605;     wsUYQkoeiW91458605 = wsUYQkoeiW84833956;     wsUYQkoeiW84833956 = wsUYQkoeiW4373744;     wsUYQkoeiW4373744 = wsUYQkoeiW94341140;     wsUYQkoeiW94341140 = wsUYQkoeiW76907597;     wsUYQkoeiW76907597 = wsUYQkoeiW98651174;     wsUYQkoeiW98651174 = wsUYQkoeiW85081699;     wsUYQkoeiW85081699 = wsUYQkoeiW88783754;     wsUYQkoeiW88783754 = wsUYQkoeiW86354616;     wsUYQkoeiW86354616 = wsUYQkoeiW2947574;     wsUYQkoeiW2947574 = wsUYQkoeiW13829065;     wsUYQkoeiW13829065 = wsUYQkoeiW28131897;     wsUYQkoeiW28131897 = wsUYQkoeiW37517952;     wsUYQkoeiW37517952 = wsUYQkoeiW26567342;     wsUYQkoeiW26567342 = wsUYQkoeiW7176143;     wsUYQkoeiW7176143 = wsUYQkoeiW38188117;     wsUYQkoeiW38188117 = wsUYQkoeiW19435061;     wsUYQkoeiW19435061 = wsUYQkoeiW89488876;     wsUYQkoeiW89488876 = wsUYQkoeiW27925262;     wsUYQkoeiW27925262 = wsUYQkoeiW38167713;     wsUYQkoeiW38167713 = wsUYQkoeiW82744825;     wsUYQkoeiW82744825 = wsUYQkoeiW20688240;     wsUYQkoeiW20688240 = wsUYQkoeiW81022174;     wsUYQkoeiW81022174 = wsUYQkoeiW9421699;     wsUYQkoeiW9421699 = wsUYQkoeiW2433106;     wsUYQkoeiW2433106 = wsUYQkoeiW70309699;     wsUYQkoeiW70309699 = wsUYQkoeiW81008752;     wsUYQkoeiW81008752 = wsUYQkoeiW71761952;     wsUYQkoeiW71761952 = wsUYQkoeiW82960507;     wsUYQkoeiW82960507 = wsUYQkoeiW24798047;     wsUYQkoeiW24798047 = wsUYQkoeiW35179966;     wsUYQkoeiW35179966 = wsUYQkoeiW47613461;     wsUYQkoeiW47613461 = wsUYQkoeiW68476813;     wsUYQkoeiW68476813 = wsUYQkoeiW63297943;     wsUYQkoeiW63297943 = wsUYQkoeiW89369281;     wsUYQkoeiW89369281 = wsUYQkoeiW2600038;     wsUYQkoeiW2600038 = wsUYQkoeiW64824574;     wsUYQkoeiW64824574 = wsUYQkoeiW20645664;     wsUYQkoeiW20645664 = wsUYQkoeiW4633495;     wsUYQkoeiW4633495 = wsUYQkoeiW70931654;     wsUYQkoeiW70931654 = wsUYQkoeiW26147355;     wsUYQkoeiW26147355 = wsUYQkoeiW70640380;     wsUYQkoeiW70640380 = wsUYQkoeiW32045746;     wsUYQkoeiW32045746 = wsUYQkoeiW57436643;     wsUYQkoeiW57436643 = wsUYQkoeiW42445769;     wsUYQkoeiW42445769 = wsUYQkoeiW37551324;     wsUYQkoeiW37551324 = wsUYQkoeiW84382769;     wsUYQkoeiW84382769 = wsUYQkoeiW71867193;     wsUYQkoeiW71867193 = wsUYQkoeiW14867469;     wsUYQkoeiW14867469 = wsUYQkoeiW44454845;     wsUYQkoeiW44454845 = wsUYQkoeiW89050616;     wsUYQkoeiW89050616 = wsUYQkoeiW31457493;     wsUYQkoeiW31457493 = wsUYQkoeiW53458233;     wsUYQkoeiW53458233 = wsUYQkoeiW92909961;     wsUYQkoeiW92909961 = wsUYQkoeiW66815307;     wsUYQkoeiW66815307 = wsUYQkoeiW70770366;     wsUYQkoeiW70770366 = wsUYQkoeiW3811783;     wsUYQkoeiW3811783 = wsUYQkoeiW94952044;     wsUYQkoeiW94952044 = wsUYQkoeiW91908035;     wsUYQkoeiW91908035 = wsUYQkoeiW6597898;     wsUYQkoeiW6597898 = wsUYQkoeiW17642423;     wsUYQkoeiW17642423 = wsUYQkoeiW34421865;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mGzSZFsZrI24182738() {     int yvEzbUvbdD1872137 = 32735153;    int yvEzbUvbdD96940001 = -521950420;    int yvEzbUvbdD24023719 = -630928799;    int yvEzbUvbdD51152743 = -904049757;    int yvEzbUvbdD96871581 = -833347085;    int yvEzbUvbdD44364922 = -700719017;    int yvEzbUvbdD30893848 = -924339949;    int yvEzbUvbdD17769673 = -879641858;    int yvEzbUvbdD27185301 = -837805200;    int yvEzbUvbdD86101840 = -270679625;    int yvEzbUvbdD55061056 = -714151111;    int yvEzbUvbdD67083266 = -57130153;    int yvEzbUvbdD51784716 = -597905568;    int yvEzbUvbdD45107903 = -294565266;    int yvEzbUvbdD41898798 = -736889293;    int yvEzbUvbdD85342739 = -256790052;    int yvEzbUvbdD71358929 = -48511601;    int yvEzbUvbdD79512348 = -936432104;    int yvEzbUvbdD2566714 = -654607748;    int yvEzbUvbdD99612400 = -81377376;    int yvEzbUvbdD64042626 = -193142715;    int yvEzbUvbdD27842814 = -495475314;    int yvEzbUvbdD1453624 = -955748270;    int yvEzbUvbdD50043165 = -844783319;    int yvEzbUvbdD20211896 = -352367402;    int yvEzbUvbdD7122612 = -896123587;    int yvEzbUvbdD76473451 = -906115625;    int yvEzbUvbdD27880854 = -307090480;    int yvEzbUvbdD93771892 = -970768569;    int yvEzbUvbdD48734940 = -320826256;    int yvEzbUvbdD67923227 = -476051596;    int yvEzbUvbdD55659183 = -486369652;    int yvEzbUvbdD7566803 = -601064515;    int yvEzbUvbdD53950971 = -733670884;    int yvEzbUvbdD2513310 = -197820562;    int yvEzbUvbdD39254743 = -753921617;    int yvEzbUvbdD24939992 = -265474972;    int yvEzbUvbdD96882417 = -914295707;    int yvEzbUvbdD53134772 = -966245237;    int yvEzbUvbdD83900390 = -781719266;    int yvEzbUvbdD20441164 = -694317938;    int yvEzbUvbdD70204366 = -833794824;    int yvEzbUvbdD79799753 = 87961617;    int yvEzbUvbdD91682687 = -697400921;    int yvEzbUvbdD12616650 = -972525042;    int yvEzbUvbdD53401478 = -361417833;    int yvEzbUvbdD41292797 = -841540619;    int yvEzbUvbdD76146521 = -980077473;    int yvEzbUvbdD11366806 = -565866890;    int yvEzbUvbdD75553013 = -467111828;    int yvEzbUvbdD1683875 = -659778538;    int yvEzbUvbdD33255763 = -555295445;    int yvEzbUvbdD29607972 = -230548937;    int yvEzbUvbdD93916218 = -10077806;    int yvEzbUvbdD60140112 = -561494740;    int yvEzbUvbdD74029323 = -471789534;    int yvEzbUvbdD95486377 = -566202150;    int yvEzbUvbdD73980554 = -786145480;    int yvEzbUvbdD30940848 = -451682355;    int yvEzbUvbdD89748970 = -937223498;    int yvEzbUvbdD67891471 = -794603393;    int yvEzbUvbdD3012995 = -517249470;    int yvEzbUvbdD23997780 = -908873289;    int yvEzbUvbdD78450361 = -416978944;    int yvEzbUvbdD18178613 = -794628029;    int yvEzbUvbdD99401873 = -127781460;    int yvEzbUvbdD59516464 = -456065638;    int yvEzbUvbdD97833744 = -864234685;    int yvEzbUvbdD42594594 = 3255296;    int yvEzbUvbdD2644056 = -982967677;    int yvEzbUvbdD60402748 = -991315081;    int yvEzbUvbdD74476512 = -134215894;    int yvEzbUvbdD26377577 = -970186868;    int yvEzbUvbdD18666323 = -872888482;    int yvEzbUvbdD79171237 = -387059438;    int yvEzbUvbdD93838259 = -359347891;    int yvEzbUvbdD48043060 = -483436932;    int yvEzbUvbdD9770937 = -158347350;    int yvEzbUvbdD37426515 = -872258278;    int yvEzbUvbdD66810417 = -990949570;    int yvEzbUvbdD65829815 = 45417031;    int yvEzbUvbdD326930 = -926038152;    int yvEzbUvbdD16514049 = -741223590;    int yvEzbUvbdD18218880 = -403656742;    int yvEzbUvbdD47051065 = -661047719;    int yvEzbUvbdD34667465 = -920756152;    int yvEzbUvbdD26051211 = -155820715;    int yvEzbUvbdD13650584 = -490986710;    int yvEzbUvbdD93810859 = -72176144;    int yvEzbUvbdD28483986 = -726031029;    int yvEzbUvbdD43768365 = -87719467;    int yvEzbUvbdD50959438 = -479329492;    int yvEzbUvbdD65941569 = -362613352;    int yvEzbUvbdD63385802 = 70978261;    int yvEzbUvbdD16008920 = -987115874;    int yvEzbUvbdD17428170 = -77068469;    int yvEzbUvbdD46206587 = -924921536;    int yvEzbUvbdD1349393 = -495059440;    int yvEzbUvbdD73504074 = -902772892;    int yvEzbUvbdD13214777 = 32735153;     yvEzbUvbdD1872137 = yvEzbUvbdD96940001;     yvEzbUvbdD96940001 = yvEzbUvbdD24023719;     yvEzbUvbdD24023719 = yvEzbUvbdD51152743;     yvEzbUvbdD51152743 = yvEzbUvbdD96871581;     yvEzbUvbdD96871581 = yvEzbUvbdD44364922;     yvEzbUvbdD44364922 = yvEzbUvbdD30893848;     yvEzbUvbdD30893848 = yvEzbUvbdD17769673;     yvEzbUvbdD17769673 = yvEzbUvbdD27185301;     yvEzbUvbdD27185301 = yvEzbUvbdD86101840;     yvEzbUvbdD86101840 = yvEzbUvbdD55061056;     yvEzbUvbdD55061056 = yvEzbUvbdD67083266;     yvEzbUvbdD67083266 = yvEzbUvbdD51784716;     yvEzbUvbdD51784716 = yvEzbUvbdD45107903;     yvEzbUvbdD45107903 = yvEzbUvbdD41898798;     yvEzbUvbdD41898798 = yvEzbUvbdD85342739;     yvEzbUvbdD85342739 = yvEzbUvbdD71358929;     yvEzbUvbdD71358929 = yvEzbUvbdD79512348;     yvEzbUvbdD79512348 = yvEzbUvbdD2566714;     yvEzbUvbdD2566714 = yvEzbUvbdD99612400;     yvEzbUvbdD99612400 = yvEzbUvbdD64042626;     yvEzbUvbdD64042626 = yvEzbUvbdD27842814;     yvEzbUvbdD27842814 = yvEzbUvbdD1453624;     yvEzbUvbdD1453624 = yvEzbUvbdD50043165;     yvEzbUvbdD50043165 = yvEzbUvbdD20211896;     yvEzbUvbdD20211896 = yvEzbUvbdD7122612;     yvEzbUvbdD7122612 = yvEzbUvbdD76473451;     yvEzbUvbdD76473451 = yvEzbUvbdD27880854;     yvEzbUvbdD27880854 = yvEzbUvbdD93771892;     yvEzbUvbdD93771892 = yvEzbUvbdD48734940;     yvEzbUvbdD48734940 = yvEzbUvbdD67923227;     yvEzbUvbdD67923227 = yvEzbUvbdD55659183;     yvEzbUvbdD55659183 = yvEzbUvbdD7566803;     yvEzbUvbdD7566803 = yvEzbUvbdD53950971;     yvEzbUvbdD53950971 = yvEzbUvbdD2513310;     yvEzbUvbdD2513310 = yvEzbUvbdD39254743;     yvEzbUvbdD39254743 = yvEzbUvbdD24939992;     yvEzbUvbdD24939992 = yvEzbUvbdD96882417;     yvEzbUvbdD96882417 = yvEzbUvbdD53134772;     yvEzbUvbdD53134772 = yvEzbUvbdD83900390;     yvEzbUvbdD83900390 = yvEzbUvbdD20441164;     yvEzbUvbdD20441164 = yvEzbUvbdD70204366;     yvEzbUvbdD70204366 = yvEzbUvbdD79799753;     yvEzbUvbdD79799753 = yvEzbUvbdD91682687;     yvEzbUvbdD91682687 = yvEzbUvbdD12616650;     yvEzbUvbdD12616650 = yvEzbUvbdD53401478;     yvEzbUvbdD53401478 = yvEzbUvbdD41292797;     yvEzbUvbdD41292797 = yvEzbUvbdD76146521;     yvEzbUvbdD76146521 = yvEzbUvbdD11366806;     yvEzbUvbdD11366806 = yvEzbUvbdD75553013;     yvEzbUvbdD75553013 = yvEzbUvbdD1683875;     yvEzbUvbdD1683875 = yvEzbUvbdD33255763;     yvEzbUvbdD33255763 = yvEzbUvbdD29607972;     yvEzbUvbdD29607972 = yvEzbUvbdD93916218;     yvEzbUvbdD93916218 = yvEzbUvbdD60140112;     yvEzbUvbdD60140112 = yvEzbUvbdD74029323;     yvEzbUvbdD74029323 = yvEzbUvbdD95486377;     yvEzbUvbdD95486377 = yvEzbUvbdD73980554;     yvEzbUvbdD73980554 = yvEzbUvbdD30940848;     yvEzbUvbdD30940848 = yvEzbUvbdD89748970;     yvEzbUvbdD89748970 = yvEzbUvbdD67891471;     yvEzbUvbdD67891471 = yvEzbUvbdD3012995;     yvEzbUvbdD3012995 = yvEzbUvbdD23997780;     yvEzbUvbdD23997780 = yvEzbUvbdD78450361;     yvEzbUvbdD78450361 = yvEzbUvbdD18178613;     yvEzbUvbdD18178613 = yvEzbUvbdD99401873;     yvEzbUvbdD99401873 = yvEzbUvbdD59516464;     yvEzbUvbdD59516464 = yvEzbUvbdD97833744;     yvEzbUvbdD97833744 = yvEzbUvbdD42594594;     yvEzbUvbdD42594594 = yvEzbUvbdD2644056;     yvEzbUvbdD2644056 = yvEzbUvbdD60402748;     yvEzbUvbdD60402748 = yvEzbUvbdD74476512;     yvEzbUvbdD74476512 = yvEzbUvbdD26377577;     yvEzbUvbdD26377577 = yvEzbUvbdD18666323;     yvEzbUvbdD18666323 = yvEzbUvbdD79171237;     yvEzbUvbdD79171237 = yvEzbUvbdD93838259;     yvEzbUvbdD93838259 = yvEzbUvbdD48043060;     yvEzbUvbdD48043060 = yvEzbUvbdD9770937;     yvEzbUvbdD9770937 = yvEzbUvbdD37426515;     yvEzbUvbdD37426515 = yvEzbUvbdD66810417;     yvEzbUvbdD66810417 = yvEzbUvbdD65829815;     yvEzbUvbdD65829815 = yvEzbUvbdD326930;     yvEzbUvbdD326930 = yvEzbUvbdD16514049;     yvEzbUvbdD16514049 = yvEzbUvbdD18218880;     yvEzbUvbdD18218880 = yvEzbUvbdD47051065;     yvEzbUvbdD47051065 = yvEzbUvbdD34667465;     yvEzbUvbdD34667465 = yvEzbUvbdD26051211;     yvEzbUvbdD26051211 = yvEzbUvbdD13650584;     yvEzbUvbdD13650584 = yvEzbUvbdD93810859;     yvEzbUvbdD93810859 = yvEzbUvbdD28483986;     yvEzbUvbdD28483986 = yvEzbUvbdD43768365;     yvEzbUvbdD43768365 = yvEzbUvbdD50959438;     yvEzbUvbdD50959438 = yvEzbUvbdD65941569;     yvEzbUvbdD65941569 = yvEzbUvbdD63385802;     yvEzbUvbdD63385802 = yvEzbUvbdD16008920;     yvEzbUvbdD16008920 = yvEzbUvbdD17428170;     yvEzbUvbdD17428170 = yvEzbUvbdD46206587;     yvEzbUvbdD46206587 = yvEzbUvbdD1349393;     yvEzbUvbdD1349393 = yvEzbUvbdD73504074;     yvEzbUvbdD73504074 = yvEzbUvbdD13214777;     yvEzbUvbdD13214777 = yvEzbUvbdD1872137;}
// Junk Finished
