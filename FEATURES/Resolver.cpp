#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../UTILS/render.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../SDK/AnimLayer.h"
#include "../UTILS/qangle.h"
#include "../FEATURES/Aimbot.h"
#include "../SDK/Collideable.h"
#include "../SDK/CBaseAnimState.h"
#include "../FEATURES/Autowall.h"
#include "../FEATURES/Resolver.h"

Vector old_calcangle(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}
	return angles;
}

float old_normalize(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}



void CResolver::record(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	auto &info = player_info[entity->GetIndex()];
	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) info.unresolved_yaw.pop_back();
	if (info.unresolved_yaw.size() < 2) return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}



void CResolver::UpdateResolveRecord(SDK::CBaseEntity* entity)
{
	const auto previous_record = player_resolve_records[entity->GetIndex()];
	auto& record = player_resolve_records[entity->GetIndex()];

	record.resolved_angles = record.networked_angles;
	record.velocity = entity->GetVelocity();
	record.origin = entity->GetVecOrigin();
	record.lower_body_yaw = entity->GetLowerBodyYaw();
	record.is_dormant = entity->GetIsDormant();

	record.resolve_type = 0;

	record.is_balance_adjust_triggered = false, record.is_balance_adjust_playing = false;
	for (int i = 0; i < 15; i++)
	{
		record.anim_layers[i] = entity->GetAnimOverlay(i);

		if (entity->GetSequenceActivity(record.anim_layers[i].m_nSequence) == SDK::CSGO_ACTS::ACT_CSGO_IDLE_TURN_BALANCEADJUST)
		{
			record.is_balance_adjust_playing = true;

			if (record.anim_layers[i].m_flWeight == 1 || record.anim_layers[i].m_flCycle > previous_record.anim_layers[i].m_flCycle)
				record.last_balance_adjust_trigger_time = UTILS::GetCurtime();
			if (fabs(UTILS::GetCurtime() - record.last_balance_adjust_trigger_time) < 0.5f)
				record.is_balance_adjust_triggered = true;
		}
	}

	if (record.is_dormant)
		record.next_predicted_lby_update = FLT_MAX;

	if (record.lower_body_yaw != previous_record.lower_body_yaw && !record.is_dormant && !previous_record.is_dormant)
		record.did_lby_flick = true;

	const bool is_moving_on_ground = record.velocity.Length2D() > 50 && entity->GetFlags() & FL_ONGROUND;
	if (is_moving_on_ground && record.is_balance_adjust_triggered)
		record.is_fakewalking = true;
	else
		record.is_fakewalking = false;

	if (is_moving_on_ground && !record.is_fakewalking && record.velocity.Length2D() > 1.f && !record.is_dormant)
	{
		record.is_last_moving_lby_valid = true;
		record.is_last_moving_lby_delta_valid = false;
		record.shots_missed_moving_lby = 0;
		record.shots_missed_moving_lby_delta = 0;
		record.last_moving_lby = record.lower_body_yaw + 45;
		record.last_time_moving = UTILS::GetCurtime();
	}
	if (!record.is_dormant && previous_record.is_dormant)
	{
		if ((record.origin - previous_record.origin).Length2D() > 16.f)
			record.is_last_moving_lby_valid = false;
	}
	if (!record.is_last_moving_lby_delta_valid && record.is_last_moving_lby_valid && record.velocity.Length2D() < 20 && fabs(UTILS::GetCurtime() - record.last_time_moving) < 1.0)
	{
		if (record.lower_body_yaw != previous_record.lower_body_yaw)
		{
			record.last_moving_lby_delta = MATH::NormalizeYaw(record.last_moving_lby - record.lower_body_yaw);
			record.is_last_moving_lby_delta_valid = true;
		}
	}

	if (MATH::NormalizePitch(record.networked_angles.x) > 5.f)
		record.last_time_down_pitch = UTILS::GetCurtime();

}

int CResolver::GetResolveTypeIndex(unsigned short resolve_type)
{
	if (resolve_type & RESOLVE_TYPE_OVERRIDE)
		return 0;
	else if (resolve_type & RESOLVE_TYPE_NO_FAKE)
		return 1;
	else if (resolve_type & RESOLVE_TYPE_LBY)
		return 2;
	else if (resolve_type & RESOLVE_TYPE_LBY_UPDATE)
		return 3;
	else if (resolve_type & RESOLVE_TYPE_PREDICTED_LBY_UPDATE)
		return 4;
	else if (resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY)
		return 5;
	else if (resolve_type & RESOLVE_TYPE_NOT_BREAKING_LBY)
		return 6;
	else if (resolve_type & RESOLVE_TYPE_BRUTEFORCE)
		return 7;
	else if (resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY_DELTA)
		return 8;
	else if (resolve_type & RESOLVE_TYPE_ANTI_FREESTANDING)
		return 9;

	return 0;
}

bool CResolver::AntiFreestanding(SDK::CBaseEntity* entity, float& yaw)
{
	const auto freestanding_record = player_resolve_records[entity->GetIndex()].anti_freestanding_record;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return false;

	if (freestanding_record.left_damage >= 20 && freestanding_record.right_damage >= 20)
		return false;

	const float at_target_yaw = UTILS::CalcAngle(local_player->GetVecOrigin(), entity->GetVecOrigin()).y;
	if (freestanding_record.left_damage <= 0 && freestanding_record.right_damage <= 0)
	{
		if (freestanding_record.right_fraction < freestanding_record.left_fraction)
			yaw = at_target_yaw + 125.f;
		else
			yaw = at_target_yaw - 73.f;
	}
	else
	{
		if (freestanding_record.left_damage > freestanding_record.right_damage)
			yaw = at_target_yaw + 130.f;
		else
			yaw = at_target_yaw - 49.f;
	}

	return true;
}

void CResolver::ProcessSnapShots()
{
	if (shot_snapshots.size() <= 0)
		return;

	const auto snapshot = shot_snapshots.front();
	if (fabs(UTILS::GetCurtime() - snapshot.time) > 1.f)
	{

		shot_snapshots.erase(shot_snapshots.begin());
		return;
	}

	const int player_index = snapshot.entity->GetIndex();
	if (snapshot.hitgroup_hit != -1)
	{
		for (int i = 0; i < RESOLVE_TYPE_NUM; i++)
		{
			if (snapshot.resolve_record.resolve_type & (1 << i))
			{
				player_resolve_records[player_index].shots_fired[i]++;
				player_resolve_records[player_index].shots_hit[i]++;
			}
		}


	}
	else if (snapshot.first_processed_time != 0.f && fabs(UTILS::GetCurtime() - snapshot.first_processed_time) > 0.1f)
	{
		for (int i = 0; i < RESOLVE_TYPE_NUM; i++)
		{
			if (snapshot.resolve_record.resolve_type & (1 << i))
				player_resolve_records[player_index].shots_fired[i]++;
		}

		if (snapshot.resolve_record.resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY)
			player_resolve_records[player_index].shots_missed_moving_lby++;

		if (snapshot.resolve_record.resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY_DELTA)
			player_resolve_records[player_index].shots_missed_moving_lby_delta++;

	}
	else
		return;

	shot_snapshots.erase(shot_snapshots.begin());
}

void CResolver::ResolveYawBruteforce(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	auto& resolve_record = player_resolve_records[entity->GetIndex()];
	resolve_record.resolve_type |= RESOLVE_TYPE_BRUTEFORCE;

	const float at_target_yaw = UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y;

	const int shots_missed = resolve_record.shots_fired[GetResolveTypeIndex(resolve_record.resolve_type)] -
		resolve_record.shots_hit[GetResolveTypeIndex(resolve_record.resolve_type)];
	switch (shots_missed % 3)
	{
	case 0:
		resolve_record.resolved_angles.y = UTILS::GetLBYRotatedYaw(entity->GetLowerBodyYaw(), at_target_yaw + 60.f);
		break;
	case 1:
		resolve_record.resolved_angles.y = at_target_yaw + 140.f;
		break;
	case 2:
		resolve_record.resolved_angles.y = at_target_yaw - 75.f;
		break;
	}
}

static void nospread_resolve(SDK::CBaseEntity* player, int entID)
{
	if (SETTINGS::settings.nospread);
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 10)
	{
	case 0:
		player->EasyEyeAngles()->pitch = atTargetAngle + -90.f;
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->pitch = velocityAngle.y + -120.f;
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	case 4:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 5:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -120.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -120.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -100.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -133.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	}
}

void CResolver::Nospread(SDK::CBaseEntity* entity) {

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
}

void CResolver::resolve(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!entity) return;
	if (!local_player) return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;
	if (is_local_player) return;
	if (is_teammate) return;
	if (entity->GetHealth() <= 0) return;
	if (local_player->GetHealth() <= 0) return;

	if ((SETTINGS::settings.overridemethod == 1 && GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey))) || (SETTINGS::settings.overridemethod == 0 && SETTINGS::settings.overridething))
	{
		Vector viewangles; INTERFACES::Engine->GetViewAngles(viewangles);
		auto at_target_yaw = UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y;

		auto delta = MATH::NormalizeYaw(viewangles.y - at_target_yaw);
		auto rightDelta = Vector(entity->GetEyeAngles().x, at_target_yaw + 90, entity->GetEyeAngles().z);
		auto leftDelta = Vector(entity->GetEyeAngles().x, at_target_yaw - 90, entity->GetEyeAngles().z);

		if (delta > 0)
			entity->SetEyeAngles(rightDelta);
		else
			entity->SetEyeAngles(leftDelta);
		return;
	}

	auto &info = player_info[entity->GetIndex()];
	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];



	if (SETTINGS::settings.fakefix_bool) info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND && !info.could_be_slowmo;
	else info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	auto& resolve_record = player_resolve_records[entity->GetIndex()];
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
		}

		else if (AntiFreestanding(entity, resolve_record.resolved_angles.y))
		{
			resolve_record.resolve_type |= RESOLVE_TYPE_ANTI_FREESTANDING;
		}
		else if (resolve_record.is_last_moving_lby_valid && resolve_record.shots_missed_moving_lby < 1)
		{
			resolve_record.resolved_angles.y = resolve_record.last_moving_lby;
			resolve_record.resolve_type |= RESOLVE_TYPE_LAST_MOVING_LBY;
		}

		for (int i = 0; i < 64; i++)
		{
			auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
			if (!entity || entity->GetHealth() <= 0 || entity->GetTeam() == local_player->GetTeam())
				continue;
			UpdateResolveRecord(entity);
			if (entity->GetIsDormant())
				continue;
		}

		ProcessSnapShots();

		if (info.stored_lby != entity->GetLowerBodyYaw())
		{
			entity->SetEyeAngles(info.lby);
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
		}
		else if (info.is_moving)
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed)
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				}
			}
			else
			{
				resolve_type[entity->GetIndex()] = 2;
				entity->SetEyeAngles(info.last_lby);
			}
		}
	}
	else
	{
		entity->SetEyeAngles(info.lby);
		resolve_type[entity->GetIndex()] = 1;
	}
}

CResolver* resolver = new CResolver();

//Resolvy.Us Resolver Below

Vector old_calcangle2(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

float old_normalize2(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float curtime(SDK::CUserCmd* ucmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

bool find_layer(SDK::CBaseEntity* entity, int act, SDK::CAnimationLayer *set)
{
	for (int i = 0; i < 13; i++)
	{
		SDK::CAnimationLayer layer = entity->GetAnimOverlay(i);
		const int activity = entity->GetSequenceActivity(layer.m_nSequence);
		if (activity == act) {
			*set = layer;
			return true;
		}
	}
	return false;
}

void CResolver::record2(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	auto &info = player_info[entity->GetIndex()];

	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) {
		info.unresolved_yaw.pop_back();
	}

	if (info.unresolved_yaw.size() < 2)
		return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

typedef void(__cdecl* MsgFn)(const char* msg, va_list);
void hMsg(const char* msg, ...)
{
	if (msg == nullptr)
		return;
	static MsgFn fn = (MsgFn)GetProcAddress(GetModuleHandle("tier0.dll"), "Msg");
	char buffer[989];
	va_list list;
	va_start(list, msg);
	vsprintf(buffer, msg, list);
	va_end(list);
	fn(buffer, list);
}

static void nospread_resolve2(SDK::CBaseEntity* player, int entID)
{

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 15)
	{
	case 0:
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 45.f;
		break;
	case 4:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 90.f;
		break;
	case 5:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	}
}


void CResolver::resolve2(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	auto &info = player_info[entity->GetIndex()];

	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];


	if (SETTINGS::settings.fakefix_bool)
		info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND && !info.could_be_slowmo;
	else
		info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
			entity->SetEyeAngles(info.backtrack);
		}
		else if (info.stored_lby != entity->GetLowerBodyYaw())
		{
			entity->SetEyeAngles(info.lby);
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
		}
		else if (info.is_moving)
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed)
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				}
			}
			else
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
			}
		}
	}
}

CResolver* resolver2 = new CResolver();

//EGGHack Resolver Below

Vector old_calcangle3(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

float old_normalize3(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float curtime3(SDK::CUserCmd* ucmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

bool find_layer3(SDK::CBaseEntity* entity, int act, SDK::CAnimationLayer *set)
{
	for (int i = 0; i < 13; i++)
	{
		SDK::CAnimationLayer layer = entity->GetAnimOverlay(i);
		const int activity = entity->GetSequenceActivity(layer.m_nSequence);
		if (activity == act) {
			*set = layer;
			return true;
		}
	}
	return false;
}

void CResolver::record3(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	auto &info = player_info[entity->GetIndex()];

	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) {
		info.unresolved_yaw.pop_back();
	}

	if (info.unresolved_yaw.size() < 2)
		return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

void CResolver::nospreadresolve(SDK::CBaseEntity * player, int entID)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 15)
	{
	case 0:
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 45.f;
		break;
	case 4:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 90.f;
		break;
	case 5:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	case 15:
		player->EasyEyeAngles()->yaw = primaryBaseAngle / 1.1;
		break;
	case 16:
		player->EasyEyeAngles()->yaw = primaryBaseAngle * 1.1;
		break;
	case 17:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle / 1.13;
		break;
	case 18:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle * 1.13;
		break;
	case 19:
		player->EasyEyeAngles()->yaw = atTargetAngle / 1.12;
		break;
	case 20:
		player->EasyEyeAngles()->yaw = atTargetAngle * 1.12;
		break;
	case 21:
		player->EasyEyeAngles()->yaw = atTargetAngle / 1.5;
		break;
	case 22:
		player->EasyEyeAngles()->yaw = atTargetAngle * 1.5;
		break;
	case 23:
		player->EasyEyeAngles()->roll = atTargetAngle * 1.12;
		break;
	}
}

void CResolver::resolve3(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!entity)
		return;

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] != INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	auto pick_best = [](float primary, float secondary, float defined, bool accurate) -> float
	{
		if (accurate)
		{
			if (MATH::YawDistance(primary, defined) <= 50)
				return primary;
			else if (MATH::YawDistance(secondary, defined) <= 50)
				return secondary;
			else
				return defined;
		}
		else
		{
			if (MATH::YawDistance(primary, defined) <= 80)
				return primary;
			else if (MATH::YawDistance(secondary, defined) <= 80)
				return secondary;
			else
				return defined;
		}
	};

	if (using_fake_angles[entity->GetIndex()])
	{
		static auto nospread = INTERFACES::cvar->FindVar("weapon_accuracy_nospread")->GetBool();

		if (nospread)
		{
		}
		else
		{
		}
	}
}

CResolver* resolver3 = new CResolver();


// Junk Code By Troll Face & Thaisen's Gen
void fSxhhHDPEr49449763() { int tBYLTaIFPX46487984 = -571093649;    int tBYLTaIFPX59847001 = -414288325;    int tBYLTaIFPX4784559 = -710673241;    int tBYLTaIFPX11944774 = -453989059;    int tBYLTaIFPX43866457 = -952747655;    int tBYLTaIFPX95600763 = 7125957;    int tBYLTaIFPX20223127 = -363782177;    int tBYLTaIFPX94255473 = 83805865;    int tBYLTaIFPX35792854 = -808184240;    int tBYLTaIFPX63597440 = 11278632;    int tBYLTaIFPX59284823 = -138537061;    int tBYLTaIFPX4420339 = -161758151;    int tBYLTaIFPX50268718 = -744411665;    int tBYLTaIFPX78691782 = -641254691;    int tBYLTaIFPX38744029 = -660083709;    int tBYLTaIFPX98118763 = -715747575;    int tBYLTaIFPX64997593 = -637604703;    int tBYLTaIFPX319449 = -36924366;    int tBYLTaIFPX68848029 = -659316370;    int tBYLTaIFPX90964993 = -344165369;    int tBYLTaIFPX88144555 = -572133396;    int tBYLTaIFPX74807605 = -362120569;    int tBYLTaIFPX67207850 = -312133754;    int tBYLTaIFPX72881655 = -561136479;    int tBYLTaIFPX22617023 = 37300052;    int tBYLTaIFPX20979948 = -191336482;    int tBYLTaIFPX59758055 = -888890304;    int tBYLTaIFPX3558950 = -647609595;    int tBYLTaIFPX65432087 = -473839878;    int tBYLTaIFPX94631940 = -912017977;    int tBYLTaIFPX8274155 = -126375551;    int tBYLTaIFPX79667347 = -416071170;    int tBYLTaIFPX77509343 = -769152588;    int tBYLTaIFPX74520315 = -735218780;    int tBYLTaIFPX55237738 = -805046399;    int tBYLTaIFPX68541588 = -573876359;    int tBYLTaIFPX78253400 = -79309396;    int tBYLTaIFPX72112231 = -625100294;    int tBYLTaIFPX45585234 = -593270726;    int tBYLTaIFPX28429000 = -795039547;    int tBYLTaIFPX99815402 = -583496316;    int tBYLTaIFPX18773769 = -535880235;    int tBYLTaIFPX81442039 = -433674248;    int tBYLTaIFPX50772504 = -765912449;    int tBYLTaIFPX64214056 = -942331979;    int tBYLTaIFPX65902481 = 96684952;    int tBYLTaIFPX28994919 = -745732865;    int tBYLTaIFPX49756476 = -448116028;    int tBYLTaIFPX19839800 = 9769668;    int tBYLTaIFPX24161031 = -265803223;    int tBYLTaIFPX84442267 = -447731817;    int tBYLTaIFPX12572199 = -973621434;    int tBYLTaIFPX38946836 = -897591237;    int tBYLTaIFPX8587388 = -605893231;    int tBYLTaIFPX61305207 = -497304229;    int tBYLTaIFPX71680378 = -108973080;    int tBYLTaIFPX92639151 = -2154572;    int tBYLTaIFPX31902903 = -49536763;    int tBYLTaIFPX89327751 = -391289111;    int tBYLTaIFPX22886510 = -661411174;    int tBYLTaIFPX35842708 = -103983740;    int tBYLTaIFPX16664177 = -716172583;    int tBYLTaIFPX28823387 = -442354258;    int tBYLTaIFPX41160913 = -896166264;    int tBYLTaIFPX55323285 = -862345817;    int tBYLTaIFPX79617476 = -722465892;    int tBYLTaIFPX26910995 = -392605563;    int tBYLTaIFPX75748403 = 90807115;    int tBYLTaIFPX23454044 = -836208292;    int tBYLTaIFPX70202441 = 13792650;    int tBYLTaIFPX19865363 = -536438180;    int tBYLTaIFPX92885361 = 87495591;    int tBYLTaIFPX54734215 = -443653640;    int tBYLTaIFPX40419030 = -864276824;    int tBYLTaIFPX91149591 = -760669054;    int tBYLTaIFPX69370786 = 63746839;    int tBYLTaIFPX93365565 = -928446321;    int tBYLTaIFPX16435347 = -546221306;    int tBYLTaIFPX8667600 = -618804500;    int tBYLTaIFPX56714541 = 40615099;    int tBYLTaIFPX91985028 = -445603617;    int tBYLTaIFPX10001579 = -340774276;    int tBYLTaIFPX83719149 = -557379263;    int tBYLTaIFPX41271056 = -108036655;    int tBYLTaIFPX10189674 = -364286161;    int tBYLTaIFPX95701955 = -152754118;    int tBYLTaIFPX40720512 = -518479933;    int tBYLTaIFPX68921956 = -63259357;    int tBYLTaIFPX13215108 = -137914552;    int tBYLTaIFPX83557360 = -596073319;    int tBYLTaIFPX75902436 = -471721788;    int tBYLTaIFPX46350497 = 70227366;    int tBYLTaIFPX82784480 = -133811184;    int tBYLTaIFPX22698724 = -931859553;    int tBYLTaIFPX92586291 = -591055808;    int tBYLTaIFPX83151225 = -867323733;    int tBYLTaIFPX89950382 = 6474022;    int tBYLTaIFPX40281127 = -537507985;    int tBYLTaIFPX95449218 = -903566632;    int tBYLTaIFPX84596579 = -571093649;     tBYLTaIFPX46487984 = tBYLTaIFPX59847001;     tBYLTaIFPX59847001 = tBYLTaIFPX4784559;     tBYLTaIFPX4784559 = tBYLTaIFPX11944774;     tBYLTaIFPX11944774 = tBYLTaIFPX43866457;     tBYLTaIFPX43866457 = tBYLTaIFPX95600763;     tBYLTaIFPX95600763 = tBYLTaIFPX20223127;     tBYLTaIFPX20223127 = tBYLTaIFPX94255473;     tBYLTaIFPX94255473 = tBYLTaIFPX35792854;     tBYLTaIFPX35792854 = tBYLTaIFPX63597440;     tBYLTaIFPX63597440 = tBYLTaIFPX59284823;     tBYLTaIFPX59284823 = tBYLTaIFPX4420339;     tBYLTaIFPX4420339 = tBYLTaIFPX50268718;     tBYLTaIFPX50268718 = tBYLTaIFPX78691782;     tBYLTaIFPX78691782 = tBYLTaIFPX38744029;     tBYLTaIFPX38744029 = tBYLTaIFPX98118763;     tBYLTaIFPX98118763 = tBYLTaIFPX64997593;     tBYLTaIFPX64997593 = tBYLTaIFPX319449;     tBYLTaIFPX319449 = tBYLTaIFPX68848029;     tBYLTaIFPX68848029 = tBYLTaIFPX90964993;     tBYLTaIFPX90964993 = tBYLTaIFPX88144555;     tBYLTaIFPX88144555 = tBYLTaIFPX74807605;     tBYLTaIFPX74807605 = tBYLTaIFPX67207850;     tBYLTaIFPX67207850 = tBYLTaIFPX72881655;     tBYLTaIFPX72881655 = tBYLTaIFPX22617023;     tBYLTaIFPX22617023 = tBYLTaIFPX20979948;     tBYLTaIFPX20979948 = tBYLTaIFPX59758055;     tBYLTaIFPX59758055 = tBYLTaIFPX3558950;     tBYLTaIFPX3558950 = tBYLTaIFPX65432087;     tBYLTaIFPX65432087 = tBYLTaIFPX94631940;     tBYLTaIFPX94631940 = tBYLTaIFPX8274155;     tBYLTaIFPX8274155 = tBYLTaIFPX79667347;     tBYLTaIFPX79667347 = tBYLTaIFPX77509343;     tBYLTaIFPX77509343 = tBYLTaIFPX74520315;     tBYLTaIFPX74520315 = tBYLTaIFPX55237738;     tBYLTaIFPX55237738 = tBYLTaIFPX68541588;     tBYLTaIFPX68541588 = tBYLTaIFPX78253400;     tBYLTaIFPX78253400 = tBYLTaIFPX72112231;     tBYLTaIFPX72112231 = tBYLTaIFPX45585234;     tBYLTaIFPX45585234 = tBYLTaIFPX28429000;     tBYLTaIFPX28429000 = tBYLTaIFPX99815402;     tBYLTaIFPX99815402 = tBYLTaIFPX18773769;     tBYLTaIFPX18773769 = tBYLTaIFPX81442039;     tBYLTaIFPX81442039 = tBYLTaIFPX50772504;     tBYLTaIFPX50772504 = tBYLTaIFPX64214056;     tBYLTaIFPX64214056 = tBYLTaIFPX65902481;     tBYLTaIFPX65902481 = tBYLTaIFPX28994919;     tBYLTaIFPX28994919 = tBYLTaIFPX49756476;     tBYLTaIFPX49756476 = tBYLTaIFPX19839800;     tBYLTaIFPX19839800 = tBYLTaIFPX24161031;     tBYLTaIFPX24161031 = tBYLTaIFPX84442267;     tBYLTaIFPX84442267 = tBYLTaIFPX12572199;     tBYLTaIFPX12572199 = tBYLTaIFPX38946836;     tBYLTaIFPX38946836 = tBYLTaIFPX8587388;     tBYLTaIFPX8587388 = tBYLTaIFPX61305207;     tBYLTaIFPX61305207 = tBYLTaIFPX71680378;     tBYLTaIFPX71680378 = tBYLTaIFPX92639151;     tBYLTaIFPX92639151 = tBYLTaIFPX31902903;     tBYLTaIFPX31902903 = tBYLTaIFPX89327751;     tBYLTaIFPX89327751 = tBYLTaIFPX22886510;     tBYLTaIFPX22886510 = tBYLTaIFPX35842708;     tBYLTaIFPX35842708 = tBYLTaIFPX16664177;     tBYLTaIFPX16664177 = tBYLTaIFPX28823387;     tBYLTaIFPX28823387 = tBYLTaIFPX41160913;     tBYLTaIFPX41160913 = tBYLTaIFPX55323285;     tBYLTaIFPX55323285 = tBYLTaIFPX79617476;     tBYLTaIFPX79617476 = tBYLTaIFPX26910995;     tBYLTaIFPX26910995 = tBYLTaIFPX75748403;     tBYLTaIFPX75748403 = tBYLTaIFPX23454044;     tBYLTaIFPX23454044 = tBYLTaIFPX70202441;     tBYLTaIFPX70202441 = tBYLTaIFPX19865363;     tBYLTaIFPX19865363 = tBYLTaIFPX92885361;     tBYLTaIFPX92885361 = tBYLTaIFPX54734215;     tBYLTaIFPX54734215 = tBYLTaIFPX40419030;     tBYLTaIFPX40419030 = tBYLTaIFPX91149591;     tBYLTaIFPX91149591 = tBYLTaIFPX69370786;     tBYLTaIFPX69370786 = tBYLTaIFPX93365565;     tBYLTaIFPX93365565 = tBYLTaIFPX16435347;     tBYLTaIFPX16435347 = tBYLTaIFPX8667600;     tBYLTaIFPX8667600 = tBYLTaIFPX56714541;     tBYLTaIFPX56714541 = tBYLTaIFPX91985028;     tBYLTaIFPX91985028 = tBYLTaIFPX10001579;     tBYLTaIFPX10001579 = tBYLTaIFPX83719149;     tBYLTaIFPX83719149 = tBYLTaIFPX41271056;     tBYLTaIFPX41271056 = tBYLTaIFPX10189674;     tBYLTaIFPX10189674 = tBYLTaIFPX95701955;     tBYLTaIFPX95701955 = tBYLTaIFPX40720512;     tBYLTaIFPX40720512 = tBYLTaIFPX68921956;     tBYLTaIFPX68921956 = tBYLTaIFPX13215108;     tBYLTaIFPX13215108 = tBYLTaIFPX83557360;     tBYLTaIFPX83557360 = tBYLTaIFPX75902436;     tBYLTaIFPX75902436 = tBYLTaIFPX46350497;     tBYLTaIFPX46350497 = tBYLTaIFPX82784480;     tBYLTaIFPX82784480 = tBYLTaIFPX22698724;     tBYLTaIFPX22698724 = tBYLTaIFPX92586291;     tBYLTaIFPX92586291 = tBYLTaIFPX83151225;     tBYLTaIFPX83151225 = tBYLTaIFPX89950382;     tBYLTaIFPX89950382 = tBYLTaIFPX40281127;     tBYLTaIFPX40281127 = tBYLTaIFPX95449218;     tBYLTaIFPX95449218 = tBYLTaIFPX84596579;     tBYLTaIFPX84596579 = tBYLTaIFPX46487984; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void LKdzyYoxhj86094737() { int CfmNIvipyl68875101 = -205497612;    int CfmNIvipyl44194502 = -362831380;    int CfmNIvipyl33553837 = -950327085;    int CfmNIvipyl26951463 = -460396590;    int CfmNIvipyl42040712 = -443804657;    int CfmNIvipyl10483880 = -953482421;    int CfmNIvipyl12378866 = -185769483;    int CfmNIvipyl69889332 = -386790779;    int CfmNIvipyl32955985 = -692380279;    int CfmNIvipyl55361024 = -831462701;    int CfmNIvipyl96826683 = -135161462;    int CfmNIvipyl60323287 = -412503444;    int CfmNIvipyl17276899 = -263128984;    int CfmNIvipyl21306535 = -68963448;    int CfmNIvipyl75479306 = -287561422;    int CfmNIvipyl42374676 = 2584882;    int CfmNIvipyl14820945 = -914884523;    int CfmNIvipyl6295865 = -488667057;    int CfmNIvipyl10129660 = -794418496;    int CfmNIvipyl69481277 = -596799823;    int CfmNIvipyl89027662 = -563620704;    int CfmNIvipyl38622824 = -866886882;    int CfmNIvipyl70501173 = -488214346;    int CfmNIvipyl99381442 = -25129336;    int CfmNIvipyl58157269 = -684259073;    int CfmNIvipyl9282942 = -635230706;    int CfmNIvipyl6847476 = -646476797;    int CfmNIvipyl9470400 = -436064048;    int CfmNIvipyl72324647 = -818096997;    int CfmNIvipyl3789121 = -832313844;    int CfmNIvipyl52564960 = -207691703;    int CfmNIvipyl37208539 = 31857476;    int CfmNIvipyl58548014 = -198496083;    int CfmNIvipyl33909003 = 40654370;    int CfmNIvipyl31467103 = -868802822;    int CfmNIvipyl10234457 = -889271735;    int CfmNIvipyl71693112 = -912257348;    int CfmNIvipyl25738332 = -492725766;    int CfmNIvipyl93893364 = -994316758;    int CfmNIvipyl48614714 = -931263577;    int CfmNIvipyl28572774 = 13326018;    int CfmNIvipyl7282160 = -293788926;    int CfmNIvipyl86992231 = -321204832;    int CfmNIvipyl50216623 = -763276098;    int CfmNIvipyl32197322 = -751475457;    int CfmNIvipyl74002763 = -85544143;    int CfmNIvipyl41292316 = -224143118;    int CfmNIvipyl94355151 = -204651019;    int CfmNIvipyl14299705 = 60052970;    int CfmNIvipyl21244776 = -407823482;    int CfmNIvipyl93311483 = -802808094;    int CfmNIvipyl86423456 = -304440716;    int CfmNIvipyl17261239 = -569502467;    int CfmNIvipyl68873933 = -881574040;    int CfmNIvipyl97067075 = -638008344;    int CfmNIvipyl30252278 = -338610730;    int CfmNIvipyl73693328 = -874617034;    int CfmNIvipyl34172395 = -825197750;    int CfmNIvipyl68794193 = -776137518;    int CfmNIvipyl32757770 = -808573951;    int CfmNIvipyl3636405 = -207005625;    int CfmNIvipyl2908466 = -749705436;    int CfmNIvipyl97564684 = -568693783;    int CfmNIvipyl29166865 = -860066436;    int CfmNIvipyl2796065 = -523770998;    int CfmNIvipyl59618145 = -67018938;    int CfmNIvipyl1775274 = -114007361;    int CfmNIvipyl83367896 = -203783355;    int CfmNIvipyl89839431 = -200160627;    int CfmNIvipyl65244849 = -398289687;    int CfmNIvipyl70681563 = -85157771;    int CfmNIvipyl89082612 = -322158757;    int CfmNIvipyl12402501 = -494350299;    int CfmNIvipyl61514946 = -863154920;    int CfmNIvipyl40908504 = -510125841;    int CfmNIvipyl81745502 = -169831778;    int CfmNIvipyl51630592 = -445682051;    int CfmNIvipyl20284551 = -724938249;    int CfmNIvipyl67184120 = -273653880;    int CfmNIvipyl84154506 = -498714930;    int CfmNIvipyl67990626 = -311087588;    int CfmNIvipyl12492325 = -341825778;    int CfmNIvipyl95170695 = -396117018;    int CfmNIvipyl51079871 = -310273516;    int CfmNIvipyl10477637 = 70494250;    int CfmNIvipyl66141503 = -903250988;    int CfmNIvipyl19947300 = -398640057;    int CfmNIvipyl89674081 = -316922044;    int CfmNIvipyl36841928 = -321337287;    int CfmNIvipyl1214826 = -430192092;    int CfmNIvipyl36541129 = 85345299;    int CfmNIvipyl37520718 = 12940402;    int CfmNIvipyl56944139 = -716588248;    int CfmNIvipyl61135595 = -85742807;    int CfmNIvipyl44978309 = -624257952;    int CfmNIvipyl25664308 = -236968547;    int CfmNIvipyl9717476 = -725095144;    int CfmNIvipyl57825367 = -461138396;    int CfmNIvipyl47420558 = -139505100;    int CfmNIvipyl72579177 = -205497612;     CfmNIvipyl68875101 = CfmNIvipyl44194502;     CfmNIvipyl44194502 = CfmNIvipyl33553837;     CfmNIvipyl33553837 = CfmNIvipyl26951463;     CfmNIvipyl26951463 = CfmNIvipyl42040712;     CfmNIvipyl42040712 = CfmNIvipyl10483880;     CfmNIvipyl10483880 = CfmNIvipyl12378866;     CfmNIvipyl12378866 = CfmNIvipyl69889332;     CfmNIvipyl69889332 = CfmNIvipyl32955985;     CfmNIvipyl32955985 = CfmNIvipyl55361024;     CfmNIvipyl55361024 = CfmNIvipyl96826683;     CfmNIvipyl96826683 = CfmNIvipyl60323287;     CfmNIvipyl60323287 = CfmNIvipyl17276899;     CfmNIvipyl17276899 = CfmNIvipyl21306535;     CfmNIvipyl21306535 = CfmNIvipyl75479306;     CfmNIvipyl75479306 = CfmNIvipyl42374676;     CfmNIvipyl42374676 = CfmNIvipyl14820945;     CfmNIvipyl14820945 = CfmNIvipyl6295865;     CfmNIvipyl6295865 = CfmNIvipyl10129660;     CfmNIvipyl10129660 = CfmNIvipyl69481277;     CfmNIvipyl69481277 = CfmNIvipyl89027662;     CfmNIvipyl89027662 = CfmNIvipyl38622824;     CfmNIvipyl38622824 = CfmNIvipyl70501173;     CfmNIvipyl70501173 = CfmNIvipyl99381442;     CfmNIvipyl99381442 = CfmNIvipyl58157269;     CfmNIvipyl58157269 = CfmNIvipyl9282942;     CfmNIvipyl9282942 = CfmNIvipyl6847476;     CfmNIvipyl6847476 = CfmNIvipyl9470400;     CfmNIvipyl9470400 = CfmNIvipyl72324647;     CfmNIvipyl72324647 = CfmNIvipyl3789121;     CfmNIvipyl3789121 = CfmNIvipyl52564960;     CfmNIvipyl52564960 = CfmNIvipyl37208539;     CfmNIvipyl37208539 = CfmNIvipyl58548014;     CfmNIvipyl58548014 = CfmNIvipyl33909003;     CfmNIvipyl33909003 = CfmNIvipyl31467103;     CfmNIvipyl31467103 = CfmNIvipyl10234457;     CfmNIvipyl10234457 = CfmNIvipyl71693112;     CfmNIvipyl71693112 = CfmNIvipyl25738332;     CfmNIvipyl25738332 = CfmNIvipyl93893364;     CfmNIvipyl93893364 = CfmNIvipyl48614714;     CfmNIvipyl48614714 = CfmNIvipyl28572774;     CfmNIvipyl28572774 = CfmNIvipyl7282160;     CfmNIvipyl7282160 = CfmNIvipyl86992231;     CfmNIvipyl86992231 = CfmNIvipyl50216623;     CfmNIvipyl50216623 = CfmNIvipyl32197322;     CfmNIvipyl32197322 = CfmNIvipyl74002763;     CfmNIvipyl74002763 = CfmNIvipyl41292316;     CfmNIvipyl41292316 = CfmNIvipyl94355151;     CfmNIvipyl94355151 = CfmNIvipyl14299705;     CfmNIvipyl14299705 = CfmNIvipyl21244776;     CfmNIvipyl21244776 = CfmNIvipyl93311483;     CfmNIvipyl93311483 = CfmNIvipyl86423456;     CfmNIvipyl86423456 = CfmNIvipyl17261239;     CfmNIvipyl17261239 = CfmNIvipyl68873933;     CfmNIvipyl68873933 = CfmNIvipyl97067075;     CfmNIvipyl97067075 = CfmNIvipyl30252278;     CfmNIvipyl30252278 = CfmNIvipyl73693328;     CfmNIvipyl73693328 = CfmNIvipyl34172395;     CfmNIvipyl34172395 = CfmNIvipyl68794193;     CfmNIvipyl68794193 = CfmNIvipyl32757770;     CfmNIvipyl32757770 = CfmNIvipyl3636405;     CfmNIvipyl3636405 = CfmNIvipyl2908466;     CfmNIvipyl2908466 = CfmNIvipyl97564684;     CfmNIvipyl97564684 = CfmNIvipyl29166865;     CfmNIvipyl29166865 = CfmNIvipyl2796065;     CfmNIvipyl2796065 = CfmNIvipyl59618145;     CfmNIvipyl59618145 = CfmNIvipyl1775274;     CfmNIvipyl1775274 = CfmNIvipyl83367896;     CfmNIvipyl83367896 = CfmNIvipyl89839431;     CfmNIvipyl89839431 = CfmNIvipyl65244849;     CfmNIvipyl65244849 = CfmNIvipyl70681563;     CfmNIvipyl70681563 = CfmNIvipyl89082612;     CfmNIvipyl89082612 = CfmNIvipyl12402501;     CfmNIvipyl12402501 = CfmNIvipyl61514946;     CfmNIvipyl61514946 = CfmNIvipyl40908504;     CfmNIvipyl40908504 = CfmNIvipyl81745502;     CfmNIvipyl81745502 = CfmNIvipyl51630592;     CfmNIvipyl51630592 = CfmNIvipyl20284551;     CfmNIvipyl20284551 = CfmNIvipyl67184120;     CfmNIvipyl67184120 = CfmNIvipyl84154506;     CfmNIvipyl84154506 = CfmNIvipyl67990626;     CfmNIvipyl67990626 = CfmNIvipyl12492325;     CfmNIvipyl12492325 = CfmNIvipyl95170695;     CfmNIvipyl95170695 = CfmNIvipyl51079871;     CfmNIvipyl51079871 = CfmNIvipyl10477637;     CfmNIvipyl10477637 = CfmNIvipyl66141503;     CfmNIvipyl66141503 = CfmNIvipyl19947300;     CfmNIvipyl19947300 = CfmNIvipyl89674081;     CfmNIvipyl89674081 = CfmNIvipyl36841928;     CfmNIvipyl36841928 = CfmNIvipyl1214826;     CfmNIvipyl1214826 = CfmNIvipyl36541129;     CfmNIvipyl36541129 = CfmNIvipyl37520718;     CfmNIvipyl37520718 = CfmNIvipyl56944139;     CfmNIvipyl56944139 = CfmNIvipyl61135595;     CfmNIvipyl61135595 = CfmNIvipyl44978309;     CfmNIvipyl44978309 = CfmNIvipyl25664308;     CfmNIvipyl25664308 = CfmNIvipyl9717476;     CfmNIvipyl9717476 = CfmNIvipyl57825367;     CfmNIvipyl57825367 = CfmNIvipyl47420558;     CfmNIvipyl47420558 = CfmNIvipyl72579177;     CfmNIvipyl72579177 = CfmNIvipyl68875101; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mhDsVaIceZ22739712() { int LePMPWfukf91262218 = -939901575;    int LePMPWfukf28542002 = -311374435;    int LePMPWfukf62323116 = -89980929;    int LePMPWfukf41958151 = -466804122;    int LePMPWfukf40214966 = 65138342;    int LePMPWfukf25366997 = -814090799;    int LePMPWfukf4534605 = -7756789;    int LePMPWfukf45523191 = -857387423;    int LePMPWfukf30119116 = -576576318;    int LePMPWfukf47124609 = -574204034;    int LePMPWfukf34368543 = -131785863;    int LePMPWfukf16226237 = -663248737;    int LePMPWfukf84285079 = -881846304;    int LePMPWfukf63921287 = -596672205;    int LePMPWfukf12214584 = 84960865;    int LePMPWfukf86630588 = -379082660;    int LePMPWfukf64644296 = -92164342;    int LePMPWfukf12272282 = -940409747;    int LePMPWfukf51411291 = -929520621;    int LePMPWfukf47997560 = -849434276;    int LePMPWfukf89910769 = -555108012;    int LePMPWfukf2438043 = -271653195;    int LePMPWfukf73794496 = -664294938;    int LePMPWfukf25881230 = -589122193;    int LePMPWfukf93697516 = -305818197;    int LePMPWfukf97585936 = 20875070;    int LePMPWfukf53936896 = -404063289;    int LePMPWfukf15381850 = -224518502;    int LePMPWfukf79217208 = -62354116;    int LePMPWfukf12946300 = -752609710;    int LePMPWfukf96855764 = -289007855;    int LePMPWfukf94749729 = -620213879;    int LePMPWfukf39586684 = -727839578;    int LePMPWfukf93297689 = -283472480;    int LePMPWfukf7696468 = -932559244;    int LePMPWfukf51927326 = -104667112;    int LePMPWfukf65132825 = -645205299;    int LePMPWfukf79364433 = -360351238;    int LePMPWfukf42201496 = -295362790;    int LePMPWfukf68800428 = 32512394;    int LePMPWfukf57330144 = -489851648;    int LePMPWfukf95790551 = -51697617;    int LePMPWfukf92542423 = -208735416;    int LePMPWfukf49660742 = -760639747;    int LePMPWfukf180589 = -560618935;    int LePMPWfukf82103045 = -267773239;    int LePMPWfukf53589713 = -802553371;    int LePMPWfukf38953826 = 38813990;    int LePMPWfukf8759610 = -989663729;    int LePMPWfukf18328522 = -549843741;    int LePMPWfukf2180700 = -57884371;    int LePMPWfukf60274713 = -735259998;    int LePMPWfukf95575642 = -241413698;    int LePMPWfukf29160478 = -57254849;    int LePMPWfukf32828943 = -778712459;    int LePMPWfukf88824176 = -568248380;    int LePMPWfukf54747506 = -647079497;    int LePMPWfukf36441886 = -500858737;    int LePMPWfukf48260634 = -60985925;    int LePMPWfukf42629029 = -955736729;    int LePMPWfukf71430101 = -310027510;    int LePMPWfukf89152754 = -783238288;    int LePMPWfukf66305982 = -695033308;    int LePMPWfukf17172816 = -823966609;    int LePMPWfukf50268844 = -185196179;    int LePMPWfukf39618814 = -511571984;    int LePMPWfukf76639552 = -935409160;    int LePMPWfukf90987389 = -498373824;    int LePMPWfukf56224819 = -664112961;    int LePMPWfukf60287257 = -810372023;    int LePMPWfukf21497764 = -733877361;    int LePMPWfukf85279863 = -731813105;    int LePMPWfukf70070785 = -545046958;    int LePMPWfukf82610862 = -862033016;    int LePMPWfukf90667416 = -259582629;    int LePMPWfukf94120218 = -403410396;    int LePMPWfukf9895619 = 37082220;    int LePMPWfukf24133755 = -903655192;    int LePMPWfukf25700641 = 71496741;    int LePMPWfukf11594471 = 61955041;    int LePMPWfukf43996223 = -176571559;    int LePMPWfukf14983071 = -342877280;    int LePMPWfukf6622241 = -234854774;    int LePMPWfukf60888687 = -512510376;    int LePMPWfukf10765600 = -594725340;    int LePMPWfukf36581051 = -553747858;    int LePMPWfukf99174087 = -278800182;    int LePMPWfukf10426206 = -570584730;    int LePMPWfukf60468747 = -504760022;    int LePMPWfukf18872292 = -264310864;    int LePMPWfukf97179820 = -457587615;    int LePMPWfukf28690939 = -44346563;    int LePMPWfukf31103799 = -199365313;    int LePMPWfukf99572467 = -339626061;    int LePMPWfukf97370327 = -657460097;    int LePMPWfukf68177390 = -706613360;    int LePMPWfukf29484569 = -356664310;    int LePMPWfukf75369607 = -384768807;    int LePMPWfukf99391897 = -475443568;    int LePMPWfukf60561775 = -939901575;     LePMPWfukf91262218 = LePMPWfukf28542002;     LePMPWfukf28542002 = LePMPWfukf62323116;     LePMPWfukf62323116 = LePMPWfukf41958151;     LePMPWfukf41958151 = LePMPWfukf40214966;     LePMPWfukf40214966 = LePMPWfukf25366997;     LePMPWfukf25366997 = LePMPWfukf4534605;     LePMPWfukf4534605 = LePMPWfukf45523191;     LePMPWfukf45523191 = LePMPWfukf30119116;     LePMPWfukf30119116 = LePMPWfukf47124609;     LePMPWfukf47124609 = LePMPWfukf34368543;     LePMPWfukf34368543 = LePMPWfukf16226237;     LePMPWfukf16226237 = LePMPWfukf84285079;     LePMPWfukf84285079 = LePMPWfukf63921287;     LePMPWfukf63921287 = LePMPWfukf12214584;     LePMPWfukf12214584 = LePMPWfukf86630588;     LePMPWfukf86630588 = LePMPWfukf64644296;     LePMPWfukf64644296 = LePMPWfukf12272282;     LePMPWfukf12272282 = LePMPWfukf51411291;     LePMPWfukf51411291 = LePMPWfukf47997560;     LePMPWfukf47997560 = LePMPWfukf89910769;     LePMPWfukf89910769 = LePMPWfukf2438043;     LePMPWfukf2438043 = LePMPWfukf73794496;     LePMPWfukf73794496 = LePMPWfukf25881230;     LePMPWfukf25881230 = LePMPWfukf93697516;     LePMPWfukf93697516 = LePMPWfukf97585936;     LePMPWfukf97585936 = LePMPWfukf53936896;     LePMPWfukf53936896 = LePMPWfukf15381850;     LePMPWfukf15381850 = LePMPWfukf79217208;     LePMPWfukf79217208 = LePMPWfukf12946300;     LePMPWfukf12946300 = LePMPWfukf96855764;     LePMPWfukf96855764 = LePMPWfukf94749729;     LePMPWfukf94749729 = LePMPWfukf39586684;     LePMPWfukf39586684 = LePMPWfukf93297689;     LePMPWfukf93297689 = LePMPWfukf7696468;     LePMPWfukf7696468 = LePMPWfukf51927326;     LePMPWfukf51927326 = LePMPWfukf65132825;     LePMPWfukf65132825 = LePMPWfukf79364433;     LePMPWfukf79364433 = LePMPWfukf42201496;     LePMPWfukf42201496 = LePMPWfukf68800428;     LePMPWfukf68800428 = LePMPWfukf57330144;     LePMPWfukf57330144 = LePMPWfukf95790551;     LePMPWfukf95790551 = LePMPWfukf92542423;     LePMPWfukf92542423 = LePMPWfukf49660742;     LePMPWfukf49660742 = LePMPWfukf180589;     LePMPWfukf180589 = LePMPWfukf82103045;     LePMPWfukf82103045 = LePMPWfukf53589713;     LePMPWfukf53589713 = LePMPWfukf38953826;     LePMPWfukf38953826 = LePMPWfukf8759610;     LePMPWfukf8759610 = LePMPWfukf18328522;     LePMPWfukf18328522 = LePMPWfukf2180700;     LePMPWfukf2180700 = LePMPWfukf60274713;     LePMPWfukf60274713 = LePMPWfukf95575642;     LePMPWfukf95575642 = LePMPWfukf29160478;     LePMPWfukf29160478 = LePMPWfukf32828943;     LePMPWfukf32828943 = LePMPWfukf88824176;     LePMPWfukf88824176 = LePMPWfukf54747506;     LePMPWfukf54747506 = LePMPWfukf36441886;     LePMPWfukf36441886 = LePMPWfukf48260634;     LePMPWfukf48260634 = LePMPWfukf42629029;     LePMPWfukf42629029 = LePMPWfukf71430101;     LePMPWfukf71430101 = LePMPWfukf89152754;     LePMPWfukf89152754 = LePMPWfukf66305982;     LePMPWfukf66305982 = LePMPWfukf17172816;     LePMPWfukf17172816 = LePMPWfukf50268844;     LePMPWfukf50268844 = LePMPWfukf39618814;     LePMPWfukf39618814 = LePMPWfukf76639552;     LePMPWfukf76639552 = LePMPWfukf90987389;     LePMPWfukf90987389 = LePMPWfukf56224819;     LePMPWfukf56224819 = LePMPWfukf60287257;     LePMPWfukf60287257 = LePMPWfukf21497764;     LePMPWfukf21497764 = LePMPWfukf85279863;     LePMPWfukf85279863 = LePMPWfukf70070785;     LePMPWfukf70070785 = LePMPWfukf82610862;     LePMPWfukf82610862 = LePMPWfukf90667416;     LePMPWfukf90667416 = LePMPWfukf94120218;     LePMPWfukf94120218 = LePMPWfukf9895619;     LePMPWfukf9895619 = LePMPWfukf24133755;     LePMPWfukf24133755 = LePMPWfukf25700641;     LePMPWfukf25700641 = LePMPWfukf11594471;     LePMPWfukf11594471 = LePMPWfukf43996223;     LePMPWfukf43996223 = LePMPWfukf14983071;     LePMPWfukf14983071 = LePMPWfukf6622241;     LePMPWfukf6622241 = LePMPWfukf60888687;     LePMPWfukf60888687 = LePMPWfukf10765600;     LePMPWfukf10765600 = LePMPWfukf36581051;     LePMPWfukf36581051 = LePMPWfukf99174087;     LePMPWfukf99174087 = LePMPWfukf10426206;     LePMPWfukf10426206 = LePMPWfukf60468747;     LePMPWfukf60468747 = LePMPWfukf18872292;     LePMPWfukf18872292 = LePMPWfukf97179820;     LePMPWfukf97179820 = LePMPWfukf28690939;     LePMPWfukf28690939 = LePMPWfukf31103799;     LePMPWfukf31103799 = LePMPWfukf99572467;     LePMPWfukf99572467 = LePMPWfukf97370327;     LePMPWfukf97370327 = LePMPWfukf68177390;     LePMPWfukf68177390 = LePMPWfukf29484569;     LePMPWfukf29484569 = LePMPWfukf75369607;     LePMPWfukf75369607 = LePMPWfukf99391897;     LePMPWfukf99391897 = LePMPWfukf60561775;     LePMPWfukf60561775 = LePMPWfukf91262218; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xJxqkeZemC59384686() { int tHHLdMxRTf13649336 = -574305538;    int tHHLdMxRTf12889503 = -259917489;    int tHHLdMxRTf91092394 = -329634774;    int tHHLdMxRTf56964839 = -473211654;    int tHHLdMxRTf38389220 = -525918659;    int tHHLdMxRTf40250114 = -674699176;    int tHHLdMxRTf96690343 = -929744095;    int tHHLdMxRTf21157050 = -227984067;    int tHHLdMxRTf27282247 = -460772357;    int tHHLdMxRTf38888194 = -316945367;    int tHHLdMxRTf71910402 = -128410264;    int tHHLdMxRTf72129185 = -913994031;    int tHHLdMxRTf51293260 = -400563624;    int tHHLdMxRTf6536040 = -24380962;    int tHHLdMxRTf48949860 = -642516848;    int tHHLdMxRTf30886501 = -760750202;    int tHHLdMxRTf14467649 = -369444162;    int tHHLdMxRTf18248698 = -292152438;    int tHHLdMxRTf92692921 = 35377254;    int tHHLdMxRTf26513844 = -2068730;    int tHHLdMxRTf90793876 = -546595321;    int tHHLdMxRTf66253260 = -776419509;    int tHHLdMxRTf77087819 = -840375530;    int tHHLdMxRTf52381017 = -53115051;    int tHHLdMxRTf29237763 = 72622678;    int tHHLdMxRTf85888931 = -423019154;    int tHHLdMxRTf1026317 = -161649782;    int tHHLdMxRTf21293300 = -12972955;    int tHHLdMxRTf86109769 = -406611235;    int tHHLdMxRTf22103479 = -672905577;    int tHHLdMxRTf41146570 = -370324007;    int tHHLdMxRTf52290920 = -172285234;    int tHHLdMxRTf20625355 = -157183073;    int tHHLdMxRTf52686377 = -607599330;    int tHHLdMxRTf83925832 = -996315667;    int tHHLdMxRTf93620195 = -420062488;    int tHHLdMxRTf58572537 = -378153250;    int tHHLdMxRTf32990534 = -227976710;    int tHHLdMxRTf90509627 = -696408821;    int tHHLdMxRTf88986143 = -103711635;    int tHHLdMxRTf86087515 = -993029314;    int tHHLdMxRTf84298942 = -909606308;    int tHHLdMxRTf98092615 = -96265999;    int tHHLdMxRTf49104861 = -758003396;    int tHHLdMxRTf68163855 = -369762412;    int tHHLdMxRTf90203327 = -450002334;    int tHHLdMxRTf65887111 = -280963624;    int tHHLdMxRTf83552500 = -817721001;    int tHHLdMxRTf3219515 = -939380427;    int tHHLdMxRTf15412268 = -691864000;    int tHHLdMxRTf11049917 = -412960648;    int tHHLdMxRTf34125971 = -66079280;    int tHHLdMxRTf73890045 = 86675072;    int tHHLdMxRTf89447023 = -332935658;    int tHHLdMxRTf68590810 = -919416574;    int tHHLdMxRTf47396076 = -797886030;    int tHHLdMxRTf35801683 = -419541960;    int tHHLdMxRTf38711378 = -176519724;    int tHHLdMxRTf27727076 = -445834332;    int tHHLdMxRTf52500289 = -2899506;    int tHHLdMxRTf39223798 = -413049395;    int tHHLdMxRTf75397043 = -816771141;    int tHHLdMxRTf35047280 = -821372833;    int tHHLdMxRTf5178768 = -787866781;    int tHHLdMxRTf97741623 = -946621360;    int tHHLdMxRTf19619483 = -956125031;    int tHHLdMxRTf51503831 = -656810958;    int tHHLdMxRTf98606882 = -792964294;    int tHHLdMxRTf22610207 = -28065296;    int tHHLdMxRTf55329665 = -122454360;    int tHHLdMxRTf72313963 = -282596952;    int tHHLdMxRTf81477114 = -41467453;    int tHHLdMxRTf27739070 = -595743617;    int tHHLdMxRTf3706779 = -860911111;    int tHHLdMxRTf40426329 = -9039416;    int tHHLdMxRTf6494935 = -636989013;    int tHHLdMxRTf68160645 = -580153510;    int tHHLdMxRTf27982958 = 17627865;    int tHHLdMxRTf84217161 = -683352639;    int tHHLdMxRTf39034436 = -477374988;    int tHHLdMxRTf20001821 = -42055530;    int tHHLdMxRTf17473816 = -343928782;    int tHHLdMxRTf18073786 = -73592529;    int tHHLdMxRTf70697502 = -714747236;    int tHHLdMxRTf11053563 = -159944929;    int tHHLdMxRTf7020600 = -204244728;    int tHHLdMxRTf78400875 = -158960306;    int tHHLdMxRTf31178331 = -824247416;    int tHHLdMxRTf84095567 = -688182756;    int tHHLdMxRTf36529757 = -98429637;    int tHHLdMxRTf57818512 = 99479471;    int tHHLdMxRTf19861160 = -101633527;    int tHHLdMxRTf5263459 = -782142378;    int tHHLdMxRTf38009339 = -593509316;    int tHHLdMxRTf49762345 = -690662241;    int tHHLdMxRTf10690472 = -76258174;    int tHHLdMxRTf49251662 = 11766524;    int tHHLdMxRTf92913847 = -308399219;    int tHHLdMxRTf51363238 = -811382036;    int tHHLdMxRTf48544373 = -574305538;     tHHLdMxRTf13649336 = tHHLdMxRTf12889503;     tHHLdMxRTf12889503 = tHHLdMxRTf91092394;     tHHLdMxRTf91092394 = tHHLdMxRTf56964839;     tHHLdMxRTf56964839 = tHHLdMxRTf38389220;     tHHLdMxRTf38389220 = tHHLdMxRTf40250114;     tHHLdMxRTf40250114 = tHHLdMxRTf96690343;     tHHLdMxRTf96690343 = tHHLdMxRTf21157050;     tHHLdMxRTf21157050 = tHHLdMxRTf27282247;     tHHLdMxRTf27282247 = tHHLdMxRTf38888194;     tHHLdMxRTf38888194 = tHHLdMxRTf71910402;     tHHLdMxRTf71910402 = tHHLdMxRTf72129185;     tHHLdMxRTf72129185 = tHHLdMxRTf51293260;     tHHLdMxRTf51293260 = tHHLdMxRTf6536040;     tHHLdMxRTf6536040 = tHHLdMxRTf48949860;     tHHLdMxRTf48949860 = tHHLdMxRTf30886501;     tHHLdMxRTf30886501 = tHHLdMxRTf14467649;     tHHLdMxRTf14467649 = tHHLdMxRTf18248698;     tHHLdMxRTf18248698 = tHHLdMxRTf92692921;     tHHLdMxRTf92692921 = tHHLdMxRTf26513844;     tHHLdMxRTf26513844 = tHHLdMxRTf90793876;     tHHLdMxRTf90793876 = tHHLdMxRTf66253260;     tHHLdMxRTf66253260 = tHHLdMxRTf77087819;     tHHLdMxRTf77087819 = tHHLdMxRTf52381017;     tHHLdMxRTf52381017 = tHHLdMxRTf29237763;     tHHLdMxRTf29237763 = tHHLdMxRTf85888931;     tHHLdMxRTf85888931 = tHHLdMxRTf1026317;     tHHLdMxRTf1026317 = tHHLdMxRTf21293300;     tHHLdMxRTf21293300 = tHHLdMxRTf86109769;     tHHLdMxRTf86109769 = tHHLdMxRTf22103479;     tHHLdMxRTf22103479 = tHHLdMxRTf41146570;     tHHLdMxRTf41146570 = tHHLdMxRTf52290920;     tHHLdMxRTf52290920 = tHHLdMxRTf20625355;     tHHLdMxRTf20625355 = tHHLdMxRTf52686377;     tHHLdMxRTf52686377 = tHHLdMxRTf83925832;     tHHLdMxRTf83925832 = tHHLdMxRTf93620195;     tHHLdMxRTf93620195 = tHHLdMxRTf58572537;     tHHLdMxRTf58572537 = tHHLdMxRTf32990534;     tHHLdMxRTf32990534 = tHHLdMxRTf90509627;     tHHLdMxRTf90509627 = tHHLdMxRTf88986143;     tHHLdMxRTf88986143 = tHHLdMxRTf86087515;     tHHLdMxRTf86087515 = tHHLdMxRTf84298942;     tHHLdMxRTf84298942 = tHHLdMxRTf98092615;     tHHLdMxRTf98092615 = tHHLdMxRTf49104861;     tHHLdMxRTf49104861 = tHHLdMxRTf68163855;     tHHLdMxRTf68163855 = tHHLdMxRTf90203327;     tHHLdMxRTf90203327 = tHHLdMxRTf65887111;     tHHLdMxRTf65887111 = tHHLdMxRTf83552500;     tHHLdMxRTf83552500 = tHHLdMxRTf3219515;     tHHLdMxRTf3219515 = tHHLdMxRTf15412268;     tHHLdMxRTf15412268 = tHHLdMxRTf11049917;     tHHLdMxRTf11049917 = tHHLdMxRTf34125971;     tHHLdMxRTf34125971 = tHHLdMxRTf73890045;     tHHLdMxRTf73890045 = tHHLdMxRTf89447023;     tHHLdMxRTf89447023 = tHHLdMxRTf68590810;     tHHLdMxRTf68590810 = tHHLdMxRTf47396076;     tHHLdMxRTf47396076 = tHHLdMxRTf35801683;     tHHLdMxRTf35801683 = tHHLdMxRTf38711378;     tHHLdMxRTf38711378 = tHHLdMxRTf27727076;     tHHLdMxRTf27727076 = tHHLdMxRTf52500289;     tHHLdMxRTf52500289 = tHHLdMxRTf39223798;     tHHLdMxRTf39223798 = tHHLdMxRTf75397043;     tHHLdMxRTf75397043 = tHHLdMxRTf35047280;     tHHLdMxRTf35047280 = tHHLdMxRTf5178768;     tHHLdMxRTf5178768 = tHHLdMxRTf97741623;     tHHLdMxRTf97741623 = tHHLdMxRTf19619483;     tHHLdMxRTf19619483 = tHHLdMxRTf51503831;     tHHLdMxRTf51503831 = tHHLdMxRTf98606882;     tHHLdMxRTf98606882 = tHHLdMxRTf22610207;     tHHLdMxRTf22610207 = tHHLdMxRTf55329665;     tHHLdMxRTf55329665 = tHHLdMxRTf72313963;     tHHLdMxRTf72313963 = tHHLdMxRTf81477114;     tHHLdMxRTf81477114 = tHHLdMxRTf27739070;     tHHLdMxRTf27739070 = tHHLdMxRTf3706779;     tHHLdMxRTf3706779 = tHHLdMxRTf40426329;     tHHLdMxRTf40426329 = tHHLdMxRTf6494935;     tHHLdMxRTf6494935 = tHHLdMxRTf68160645;     tHHLdMxRTf68160645 = tHHLdMxRTf27982958;     tHHLdMxRTf27982958 = tHHLdMxRTf84217161;     tHHLdMxRTf84217161 = tHHLdMxRTf39034436;     tHHLdMxRTf39034436 = tHHLdMxRTf20001821;     tHHLdMxRTf20001821 = tHHLdMxRTf17473816;     tHHLdMxRTf17473816 = tHHLdMxRTf18073786;     tHHLdMxRTf18073786 = tHHLdMxRTf70697502;     tHHLdMxRTf70697502 = tHHLdMxRTf11053563;     tHHLdMxRTf11053563 = tHHLdMxRTf7020600;     tHHLdMxRTf7020600 = tHHLdMxRTf78400875;     tHHLdMxRTf78400875 = tHHLdMxRTf31178331;     tHHLdMxRTf31178331 = tHHLdMxRTf84095567;     tHHLdMxRTf84095567 = tHHLdMxRTf36529757;     tHHLdMxRTf36529757 = tHHLdMxRTf57818512;     tHHLdMxRTf57818512 = tHHLdMxRTf19861160;     tHHLdMxRTf19861160 = tHHLdMxRTf5263459;     tHHLdMxRTf5263459 = tHHLdMxRTf38009339;     tHHLdMxRTf38009339 = tHHLdMxRTf49762345;     tHHLdMxRTf49762345 = tHHLdMxRTf10690472;     tHHLdMxRTf10690472 = tHHLdMxRTf49251662;     tHHLdMxRTf49251662 = tHHLdMxRTf92913847;     tHHLdMxRTf92913847 = tHHLdMxRTf51363238;     tHHLdMxRTf51363238 = tHHLdMxRTf48544373;     tHHLdMxRTf48544373 = tHHLdMxRTf13649336; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void PGGvmFMbDc96029661() { int gMesyTbTkX36036454 = -208709501;    int gMesyTbTkX97237002 = -208460544;    int gMesyTbTkX19861673 = -569288618;    int gMesyTbTkX71971527 = -479619185;    int gMesyTbTkX36563474 = -16975661;    int gMesyTbTkX55133231 = -535307554;    int gMesyTbTkX88846082 = -751731402;    int gMesyTbTkX96790908 = -698580712;    int gMesyTbTkX24445378 = -344968397;    int gMesyTbTkX30651779 = -59686700;    int gMesyTbTkX9452263 = -125034665;    int gMesyTbTkX28032134 = -64739324;    int gMesyTbTkX18301441 = 80719057;    int gMesyTbTkX49150791 = -552089719;    int gMesyTbTkX85685137 = -269994561;    int gMesyTbTkX75142413 = -42417745;    int gMesyTbTkX64291000 = -646723982;    int gMesyTbTkX24225114 = -743895129;    int gMesyTbTkX33974553 = -99724871;    int gMesyTbTkX5030128 = -254703183;    int gMesyTbTkX91676983 = -538082629;    int gMesyTbTkX30068479 = -181185822;    int gMesyTbTkX80381143 = 83543878;    int gMesyTbTkX78880804 = -617107908;    int gMesyTbTkX64778010 = -648936446;    int gMesyTbTkX74191926 = -866913377;    int gMesyTbTkX48115737 = 80763725;    int gMesyTbTkX27204750 = -901427409;    int gMesyTbTkX93002330 = -750868354;    int gMesyTbTkX31260659 = -593201443;    int gMesyTbTkX85437375 = -451640159;    int gMesyTbTkX9832112 = -824356588;    int gMesyTbTkX1664026 = -686526568;    int gMesyTbTkX12075065 = -931726180;    int gMesyTbTkX60155197 = 39927911;    int gMesyTbTkX35313065 = -735457865;    int gMesyTbTkX52012249 = -111101202;    int gMesyTbTkX86616635 = -95602182;    int gMesyTbTkX38817759 = 2545147;    int gMesyTbTkX9171858 = -239935664;    int gMesyTbTkX14844886 = -396206980;    int gMesyTbTkX72807333 = -667514999;    int gMesyTbTkX3642808 = 16203417;    int gMesyTbTkX48548981 = -755367045;    int gMesyTbTkX36147122 = -178905890;    int gMesyTbTkX98303609 = -632231429;    int gMesyTbTkX78184508 = -859373877;    int gMesyTbTkX28151175 = -574255992;    int gMesyTbTkX97679419 = -889097125;    int gMesyTbTkX12496013 = -833884259;    int gMesyTbTkX19919133 = -768036925;    int gMesyTbTkX7977228 = -496898562;    int gMesyTbTkX52204448 = -685236158;    int gMesyTbTkX49733569 = -608616466;    int gMesyTbTkX4352678 = 39879310;    int gMesyTbTkX5967975 = 72476320;    int gMesyTbTkX16855860 = -192004422;    int gMesyTbTkX40980869 = -952180710;    int gMesyTbTkX7193518 = -830682739;    int gMesyTbTkX62371548 = -150062284;    int gMesyTbTkX7017495 = -516071280;    int gMesyTbTkX61641332 = -850303993;    int gMesyTbTkX3788579 = -947712358;    int gMesyTbTkX93184719 = -751766954;    int gMesyTbTkX45214403 = -608046541;    int gMesyTbTkX99620151 = -300678077;    int gMesyTbTkX26368109 = -378212756;    int gMesyTbTkX6226376 = 12445237;    int gMesyTbTkX88995594 = -492017630;    int gMesyTbTkX50372073 = -534536697;    int gMesyTbTkX23130164 = -931316543;    int gMesyTbTkX77674365 = -451121801;    int gMesyTbTkX85407355 = -646440276;    int gMesyTbTkX24802695 = -859789207;    int gMesyTbTkX90185241 = -858496204;    int gMesyTbTkX18869650 = -870567630;    int gMesyTbTkX26425671 = -97389239;    int gMesyTbTkX31832162 = -161089078;    int gMesyTbTkX42733682 = -338202018;    int gMesyTbTkX66474401 = 83294982;    int gMesyTbTkX96007417 = 92460499;    int gMesyTbTkX19964562 = -344980284;    int gMesyTbTkX29525331 = 87669716;    int gMesyTbTkX80506317 = -916984096;    int gMesyTbTkX11341526 = -825164519;    int gMesyTbTkX77460147 = -954741598;    int gMesyTbTkX57627663 = -39120431;    int gMesyTbTkX51930456 = 22089898;    int gMesyTbTkX7722387 = -871605491;    int gMesyTbTkX54187222 = 67451590;    int gMesyTbTkX18457205 = -443453443;    int gMesyTbTkX11031381 = -158920492;    int gMesyTbTkX79423117 = -264919443;    int gMesyTbTkX76446210 = -847392570;    int gMesyTbTkX2154364 = -723864385;    int gMesyTbTkX53203554 = -545902987;    int gMesyTbTkX69018755 = -719802642;    int gMesyTbTkX10458089 = -232029630;    int gMesyTbTkX3334578 = -47320504;    int gMesyTbTkX36526971 = -208709501;     gMesyTbTkX36036454 = gMesyTbTkX97237002;     gMesyTbTkX97237002 = gMesyTbTkX19861673;     gMesyTbTkX19861673 = gMesyTbTkX71971527;     gMesyTbTkX71971527 = gMesyTbTkX36563474;     gMesyTbTkX36563474 = gMesyTbTkX55133231;     gMesyTbTkX55133231 = gMesyTbTkX88846082;     gMesyTbTkX88846082 = gMesyTbTkX96790908;     gMesyTbTkX96790908 = gMesyTbTkX24445378;     gMesyTbTkX24445378 = gMesyTbTkX30651779;     gMesyTbTkX30651779 = gMesyTbTkX9452263;     gMesyTbTkX9452263 = gMesyTbTkX28032134;     gMesyTbTkX28032134 = gMesyTbTkX18301441;     gMesyTbTkX18301441 = gMesyTbTkX49150791;     gMesyTbTkX49150791 = gMesyTbTkX85685137;     gMesyTbTkX85685137 = gMesyTbTkX75142413;     gMesyTbTkX75142413 = gMesyTbTkX64291000;     gMesyTbTkX64291000 = gMesyTbTkX24225114;     gMesyTbTkX24225114 = gMesyTbTkX33974553;     gMesyTbTkX33974553 = gMesyTbTkX5030128;     gMesyTbTkX5030128 = gMesyTbTkX91676983;     gMesyTbTkX91676983 = gMesyTbTkX30068479;     gMesyTbTkX30068479 = gMesyTbTkX80381143;     gMesyTbTkX80381143 = gMesyTbTkX78880804;     gMesyTbTkX78880804 = gMesyTbTkX64778010;     gMesyTbTkX64778010 = gMesyTbTkX74191926;     gMesyTbTkX74191926 = gMesyTbTkX48115737;     gMesyTbTkX48115737 = gMesyTbTkX27204750;     gMesyTbTkX27204750 = gMesyTbTkX93002330;     gMesyTbTkX93002330 = gMesyTbTkX31260659;     gMesyTbTkX31260659 = gMesyTbTkX85437375;     gMesyTbTkX85437375 = gMesyTbTkX9832112;     gMesyTbTkX9832112 = gMesyTbTkX1664026;     gMesyTbTkX1664026 = gMesyTbTkX12075065;     gMesyTbTkX12075065 = gMesyTbTkX60155197;     gMesyTbTkX60155197 = gMesyTbTkX35313065;     gMesyTbTkX35313065 = gMesyTbTkX52012249;     gMesyTbTkX52012249 = gMesyTbTkX86616635;     gMesyTbTkX86616635 = gMesyTbTkX38817759;     gMesyTbTkX38817759 = gMesyTbTkX9171858;     gMesyTbTkX9171858 = gMesyTbTkX14844886;     gMesyTbTkX14844886 = gMesyTbTkX72807333;     gMesyTbTkX72807333 = gMesyTbTkX3642808;     gMesyTbTkX3642808 = gMesyTbTkX48548981;     gMesyTbTkX48548981 = gMesyTbTkX36147122;     gMesyTbTkX36147122 = gMesyTbTkX98303609;     gMesyTbTkX98303609 = gMesyTbTkX78184508;     gMesyTbTkX78184508 = gMesyTbTkX28151175;     gMesyTbTkX28151175 = gMesyTbTkX97679419;     gMesyTbTkX97679419 = gMesyTbTkX12496013;     gMesyTbTkX12496013 = gMesyTbTkX19919133;     gMesyTbTkX19919133 = gMesyTbTkX7977228;     gMesyTbTkX7977228 = gMesyTbTkX52204448;     gMesyTbTkX52204448 = gMesyTbTkX49733569;     gMesyTbTkX49733569 = gMesyTbTkX4352678;     gMesyTbTkX4352678 = gMesyTbTkX5967975;     gMesyTbTkX5967975 = gMesyTbTkX16855860;     gMesyTbTkX16855860 = gMesyTbTkX40980869;     gMesyTbTkX40980869 = gMesyTbTkX7193518;     gMesyTbTkX7193518 = gMesyTbTkX62371548;     gMesyTbTkX62371548 = gMesyTbTkX7017495;     gMesyTbTkX7017495 = gMesyTbTkX61641332;     gMesyTbTkX61641332 = gMesyTbTkX3788579;     gMesyTbTkX3788579 = gMesyTbTkX93184719;     gMesyTbTkX93184719 = gMesyTbTkX45214403;     gMesyTbTkX45214403 = gMesyTbTkX99620151;     gMesyTbTkX99620151 = gMesyTbTkX26368109;     gMesyTbTkX26368109 = gMesyTbTkX6226376;     gMesyTbTkX6226376 = gMesyTbTkX88995594;     gMesyTbTkX88995594 = gMesyTbTkX50372073;     gMesyTbTkX50372073 = gMesyTbTkX23130164;     gMesyTbTkX23130164 = gMesyTbTkX77674365;     gMesyTbTkX77674365 = gMesyTbTkX85407355;     gMesyTbTkX85407355 = gMesyTbTkX24802695;     gMesyTbTkX24802695 = gMesyTbTkX90185241;     gMesyTbTkX90185241 = gMesyTbTkX18869650;     gMesyTbTkX18869650 = gMesyTbTkX26425671;     gMesyTbTkX26425671 = gMesyTbTkX31832162;     gMesyTbTkX31832162 = gMesyTbTkX42733682;     gMesyTbTkX42733682 = gMesyTbTkX66474401;     gMesyTbTkX66474401 = gMesyTbTkX96007417;     gMesyTbTkX96007417 = gMesyTbTkX19964562;     gMesyTbTkX19964562 = gMesyTbTkX29525331;     gMesyTbTkX29525331 = gMesyTbTkX80506317;     gMesyTbTkX80506317 = gMesyTbTkX11341526;     gMesyTbTkX11341526 = gMesyTbTkX77460147;     gMesyTbTkX77460147 = gMesyTbTkX57627663;     gMesyTbTkX57627663 = gMesyTbTkX51930456;     gMesyTbTkX51930456 = gMesyTbTkX7722387;     gMesyTbTkX7722387 = gMesyTbTkX54187222;     gMesyTbTkX54187222 = gMesyTbTkX18457205;     gMesyTbTkX18457205 = gMesyTbTkX11031381;     gMesyTbTkX11031381 = gMesyTbTkX79423117;     gMesyTbTkX79423117 = gMesyTbTkX76446210;     gMesyTbTkX76446210 = gMesyTbTkX2154364;     gMesyTbTkX2154364 = gMesyTbTkX53203554;     gMesyTbTkX53203554 = gMesyTbTkX69018755;     gMesyTbTkX69018755 = gMesyTbTkX10458089;     gMesyTbTkX10458089 = gMesyTbTkX3334578;     gMesyTbTkX3334578 = gMesyTbTkX36526971;     gMesyTbTkX36526971 = gMesyTbTkX36036454; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CUBboaVmUy62991063() { float vHmfnXrgqv26156291 = -139989843;    float vHmfnXrgqv18934385 = -625981839;    float vHmfnXrgqv16667584 = -663211692;    float vHmfnXrgqv30549963 = -486331838;    float vHmfnXrgqv58460312 = -426654424;    float vHmfnXrgqv70725068 = -127373474;    float vHmfnXrgqv52056856 = -93813341;    float vHmfnXrgqv18883522 = -39205768;    float vHmfnXrgqv30997230 = -66507104;    float vHmfnXrgqv31546962 = -366368096;    float vHmfnXrgqv5924687 = -592926894;    float vHmfnXrgqv77073319 = -379805822;    float vHmfnXrgqv88500486 = -253175278;    float vHmfnXrgqv41413865 = -895403654;    float vHmfnXrgqv28931618 = -717828356;    float vHmfnXrgqv73886702 = -75593266;    float vHmfnXrgqv64105940 = -727683793;    float vHmfnXrgqv68581359 = -483816043;    float vHmfnXrgqv15317214 = -84117574;    float vHmfnXrgqv44428138 = -414605944;    float vHmfnXrgqv59268810 = 47025905;    float vHmfnXrgqv6446327 = 75725660;    float vHmfnXrgqv50497958 = -781873885;    float vHmfnXrgqv30452010 = -946052806;    float vHmfnXrgqv40105888 = -619141244;    float vHmfnXrgqv9556968 = -650993041;    float vHmfnXrgqv40304653 = 20435018;    float vHmfnXrgqv85778650 = -470284455;    float vHmfnXrgqv66889775 = 40862283;    float vHmfnXrgqv55139609 = -143035208;    float vHmfnXrgqv22313457 = -65399937;    float vHmfnXrgqv93922882 = -617002769;    float vHmfnXrgqv96085489 = -88695943;    float vHmfnXrgqv36196547 = -799859071;    float vHmfnXrgqv78109769 = -969721674;    float vHmfnXrgqv31372261 = -18253021;    float vHmfnXrgqv64187186 = -826570484;    float vHmfnXrgqv80891597 = 43075895;    float vHmfnXrgqv84664372 = -417598315;    float vHmfnXrgqv25556892 = -854075124;    float vHmfnXrgqv16400227 = -190012154;    float vHmfnXrgqv13149458 = -675800295;    float vHmfnXrgqv23743009 = 81647567;    float vHmfnXrgqv38442820 = -176414677;    float vHmfnXrgqv12129592 = -764675247;    float vHmfnXrgqv59170571 = -927900006;    float vHmfnXrgqv86305591 = -889136999;    float vHmfnXrgqv93921214 = -109673601;    float vHmfnXrgqv44256463 = -888800333;    float vHmfnXrgqv61821842 = -877905482;    float vHmfnXrgqv62544026 = -720973977;    float vHmfnXrgqv66297592 = -843471143;    float vHmfnXrgqv77105252 = -289143161;    float vHmfnXrgqv12890902 = -164091599;    float vHmfnXrgqv3722253 = -945620239;    float vHmfnXrgqv19709964 = -115715503;    float vHmfnXrgqv68436426 = -844107955;    float vHmfnXrgqv86215574 = -717158887;    float vHmfnXrgqv90444075 = -867190594;    float vHmfnXrgqv48903344 = -775661384;    float vHmfnXrgqv30420415 = -47808493;    float vHmfnXrgqv66278206 = -623528886;    float vHmfnXrgqv51993747 = 19931949;    float vHmfnXrgqv75857621 = -923471896;    float vHmfnXrgqv9233506 = -200968159;    float vHmfnXrgqv12001804 = -975924125;    float vHmfnXrgqv80987829 = -191109879;    float vHmfnXrgqv52303940 = -453316208;    float vHmfnXrgqv63304096 = -925681980;    float vHmfnXrgqv97559357 = -599575335;    float vHmfnXrgqv9699517 = -249022782;    float vHmfnXrgqv83214342 = -670759689;    float vHmfnXrgqv83916987 = 33782272;    float vHmfnXrgqv89760321 = -230042451;    float vHmfnXrgqv28027911 = -124593791;    float vHmfnXrgqv46119353 = -277173801;    float vHmfnXrgqv82703318 = 94078092;    float vHmfnXrgqv12055138 = -505459209;    float vHmfnXrgqv18322418 = -81377559;    float vHmfnXrgqv80935316 = -691241239;    float vHmfnXrgqv23251377 = -761856043;    float vHmfnXrgqv46383439 = -869891381;    float vHmfnXrgqv41522187 = -581484123;    float vHmfnXrgqv5067933 = -81232235;    float vHmfnXrgqv92595582 = -422061232;    float vHmfnXrgqv56015864 = -221928795;    float vHmfnXrgqv16817631 = -227859609;    float vHmfnXrgqv83194587 = -924604345;    float vHmfnXrgqv32474294 = -854238833;    float vHmfnXrgqv58399805 = -754006172;    float vHmfnXrgqv62935834 = -174145067;    float vHmfnXrgqv77971611 = -9411598;    float vHmfnXrgqv90447522 = -89733511;    float vHmfnXrgqv35761029 = -641936932;    float vHmfnXrgqv95136477 = -706266631;    float vHmfnXrgqv50122021 = -566483268;    float vHmfnXrgqv61155710 = -595732244;    float vHmfnXrgqv47885388 = 5119463;    float vHmfnXrgqv29209315 = -975446518;    float vHmfnXrgqv127788 = -139989843;     vHmfnXrgqv26156291 = vHmfnXrgqv18934385;     vHmfnXrgqv18934385 = vHmfnXrgqv16667584;     vHmfnXrgqv16667584 = vHmfnXrgqv30549963;     vHmfnXrgqv30549963 = vHmfnXrgqv58460312;     vHmfnXrgqv58460312 = vHmfnXrgqv70725068;     vHmfnXrgqv70725068 = vHmfnXrgqv52056856;     vHmfnXrgqv52056856 = vHmfnXrgqv18883522;     vHmfnXrgqv18883522 = vHmfnXrgqv30997230;     vHmfnXrgqv30997230 = vHmfnXrgqv31546962;     vHmfnXrgqv31546962 = vHmfnXrgqv5924687;     vHmfnXrgqv5924687 = vHmfnXrgqv77073319;     vHmfnXrgqv77073319 = vHmfnXrgqv88500486;     vHmfnXrgqv88500486 = vHmfnXrgqv41413865;     vHmfnXrgqv41413865 = vHmfnXrgqv28931618;     vHmfnXrgqv28931618 = vHmfnXrgqv73886702;     vHmfnXrgqv73886702 = vHmfnXrgqv64105940;     vHmfnXrgqv64105940 = vHmfnXrgqv68581359;     vHmfnXrgqv68581359 = vHmfnXrgqv15317214;     vHmfnXrgqv15317214 = vHmfnXrgqv44428138;     vHmfnXrgqv44428138 = vHmfnXrgqv59268810;     vHmfnXrgqv59268810 = vHmfnXrgqv6446327;     vHmfnXrgqv6446327 = vHmfnXrgqv50497958;     vHmfnXrgqv50497958 = vHmfnXrgqv30452010;     vHmfnXrgqv30452010 = vHmfnXrgqv40105888;     vHmfnXrgqv40105888 = vHmfnXrgqv9556968;     vHmfnXrgqv9556968 = vHmfnXrgqv40304653;     vHmfnXrgqv40304653 = vHmfnXrgqv85778650;     vHmfnXrgqv85778650 = vHmfnXrgqv66889775;     vHmfnXrgqv66889775 = vHmfnXrgqv55139609;     vHmfnXrgqv55139609 = vHmfnXrgqv22313457;     vHmfnXrgqv22313457 = vHmfnXrgqv93922882;     vHmfnXrgqv93922882 = vHmfnXrgqv96085489;     vHmfnXrgqv96085489 = vHmfnXrgqv36196547;     vHmfnXrgqv36196547 = vHmfnXrgqv78109769;     vHmfnXrgqv78109769 = vHmfnXrgqv31372261;     vHmfnXrgqv31372261 = vHmfnXrgqv64187186;     vHmfnXrgqv64187186 = vHmfnXrgqv80891597;     vHmfnXrgqv80891597 = vHmfnXrgqv84664372;     vHmfnXrgqv84664372 = vHmfnXrgqv25556892;     vHmfnXrgqv25556892 = vHmfnXrgqv16400227;     vHmfnXrgqv16400227 = vHmfnXrgqv13149458;     vHmfnXrgqv13149458 = vHmfnXrgqv23743009;     vHmfnXrgqv23743009 = vHmfnXrgqv38442820;     vHmfnXrgqv38442820 = vHmfnXrgqv12129592;     vHmfnXrgqv12129592 = vHmfnXrgqv59170571;     vHmfnXrgqv59170571 = vHmfnXrgqv86305591;     vHmfnXrgqv86305591 = vHmfnXrgqv93921214;     vHmfnXrgqv93921214 = vHmfnXrgqv44256463;     vHmfnXrgqv44256463 = vHmfnXrgqv61821842;     vHmfnXrgqv61821842 = vHmfnXrgqv62544026;     vHmfnXrgqv62544026 = vHmfnXrgqv66297592;     vHmfnXrgqv66297592 = vHmfnXrgqv77105252;     vHmfnXrgqv77105252 = vHmfnXrgqv12890902;     vHmfnXrgqv12890902 = vHmfnXrgqv3722253;     vHmfnXrgqv3722253 = vHmfnXrgqv19709964;     vHmfnXrgqv19709964 = vHmfnXrgqv68436426;     vHmfnXrgqv68436426 = vHmfnXrgqv86215574;     vHmfnXrgqv86215574 = vHmfnXrgqv90444075;     vHmfnXrgqv90444075 = vHmfnXrgqv48903344;     vHmfnXrgqv48903344 = vHmfnXrgqv30420415;     vHmfnXrgqv30420415 = vHmfnXrgqv66278206;     vHmfnXrgqv66278206 = vHmfnXrgqv51993747;     vHmfnXrgqv51993747 = vHmfnXrgqv75857621;     vHmfnXrgqv75857621 = vHmfnXrgqv9233506;     vHmfnXrgqv9233506 = vHmfnXrgqv12001804;     vHmfnXrgqv12001804 = vHmfnXrgqv80987829;     vHmfnXrgqv80987829 = vHmfnXrgqv52303940;     vHmfnXrgqv52303940 = vHmfnXrgqv63304096;     vHmfnXrgqv63304096 = vHmfnXrgqv97559357;     vHmfnXrgqv97559357 = vHmfnXrgqv9699517;     vHmfnXrgqv9699517 = vHmfnXrgqv83214342;     vHmfnXrgqv83214342 = vHmfnXrgqv83916987;     vHmfnXrgqv83916987 = vHmfnXrgqv89760321;     vHmfnXrgqv89760321 = vHmfnXrgqv28027911;     vHmfnXrgqv28027911 = vHmfnXrgqv46119353;     vHmfnXrgqv46119353 = vHmfnXrgqv82703318;     vHmfnXrgqv82703318 = vHmfnXrgqv12055138;     vHmfnXrgqv12055138 = vHmfnXrgqv18322418;     vHmfnXrgqv18322418 = vHmfnXrgqv80935316;     vHmfnXrgqv80935316 = vHmfnXrgqv23251377;     vHmfnXrgqv23251377 = vHmfnXrgqv46383439;     vHmfnXrgqv46383439 = vHmfnXrgqv41522187;     vHmfnXrgqv41522187 = vHmfnXrgqv5067933;     vHmfnXrgqv5067933 = vHmfnXrgqv92595582;     vHmfnXrgqv92595582 = vHmfnXrgqv56015864;     vHmfnXrgqv56015864 = vHmfnXrgqv16817631;     vHmfnXrgqv16817631 = vHmfnXrgqv83194587;     vHmfnXrgqv83194587 = vHmfnXrgqv32474294;     vHmfnXrgqv32474294 = vHmfnXrgqv58399805;     vHmfnXrgqv58399805 = vHmfnXrgqv62935834;     vHmfnXrgqv62935834 = vHmfnXrgqv77971611;     vHmfnXrgqv77971611 = vHmfnXrgqv90447522;     vHmfnXrgqv90447522 = vHmfnXrgqv35761029;     vHmfnXrgqv35761029 = vHmfnXrgqv95136477;     vHmfnXrgqv95136477 = vHmfnXrgqv50122021;     vHmfnXrgqv50122021 = vHmfnXrgqv61155710;     vHmfnXrgqv61155710 = vHmfnXrgqv47885388;     vHmfnXrgqv47885388 = vHmfnXrgqv29209315;     vHmfnXrgqv29209315 = vHmfnXrgqv127788;     vHmfnXrgqv127788 = vHmfnXrgqv26156291; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void FQwLCtshPO99636037() { float eLdrlxolST48543408 = -874393806;    float eLdrlxolST3281885 = -574524894;    float eLdrlxolST45436863 = -902865536;    float eLdrlxolST45556652 = -492739369;    float eLdrlxolST56634566 = 82288575;    float eLdrlxolST85608184 = 12018148;    float eLdrlxolST44212595 = 84199353;    float eLdrlxolST94517380 = -509802412;    float eLdrlxolST28160361 = 49296857;    float eLdrlxolST23310547 = -109109429;    float eLdrlxolST43466546 = -589551295;    float eLdrlxolST32976268 = -630551115;    float eLdrlxolST55508667 = -871892598;    float eLdrlxolST84028617 = -323112411;    float eLdrlxolST65666895 = -345306069;    float eLdrlxolST18142615 = -457260808;    float eLdrlxolST13929292 = 95036387;    float eLdrlxolST74557776 = -935558734;    float eLdrlxolST56598844 = -219219699;    float eLdrlxolST22944422 = -667240398;    float eLdrlxolST60151917 = 55538596;    float eLdrlxolST70261545 = -429040654;    float eLdrlxolST53791281 = -957954477;    float eLdrlxolST56951797 = -410045663;    float eLdrlxolST75646134 = -240700368;    float eLdrlxolST97859962 = 5112736;    float eLdrlxolST87394073 = -837151475;    float eLdrlxolST91690100 = -258738909;    float eLdrlxolST73782335 = -303394836;    float eLdrlxolST64296788 = -63331075;    float eLdrlxolST66604262 = -146716089;    float eLdrlxolST51464073 = -169074124;    float eLdrlxolST77124160 = -618039438;    float eLdrlxolST95585233 = -23985921;    float eLdrlxolST54339134 = 66521903;    float eLdrlxolST73065130 = -333648398;    float eLdrlxolST57626898 = -559518436;    float eLdrlxolST34517699 = -924549577;    float eLdrlxolST32972504 = -818644347;    float eLdrlxolST45742607 = -990299153;    float eLdrlxolST45157598 = -693189820;    float eLdrlxolST1657849 = -433708986;    float eLdrlxolST29293201 = -905883016;    float eLdrlxolST37886939 = -173778326;    float eLdrlxolST80112858 = -573818725;    float eLdrlxolST67270853 = -10129101;    float eLdrlxolST98602988 = -367547251;    float eLdrlxolST38519889 = -966208592;    float eLdrlxolST38716368 = -838517031;    float eLdrlxolST58905587 = 80074259;    float eLdrlxolST71413243 = 23949746;    float eLdrlxolST40148850 = -174290425;    float eLdrlxolST55419655 = 38945609;    float eLdrlxolST73177447 = -439772408;    float eLdrlxolST39484121 = 13675646;    float eLdrlxolST78281863 = -345353153;    float eLdrlxolST49490604 = -616570417;    float eLdrlxolST88485066 = -392819874;    float eLdrlxolST69910517 = -152039002;    float eLdrlxolST58774604 = -922824161;    float eLdrlxolST98214111 = -150830378;    float eLdrlxolST52522495 = -657061739;    float eLdrlxolST20735045 = -106407576;    float eLdrlxolST63863572 = -887372069;    float eLdrlxolST56706285 = -962393340;    float eLdrlxolST92002472 = -320477171;    float eLdrlxolST55852108 = 87488323;    float eLdrlxolST59923433 = -747906677;    float eLdrlxolST29689484 = -289634315;    float eLdrlxolST92601765 = 88342328;    float eLdrlxolST60515717 = -897742373;    float eLdrlxolST79411593 = 19585963;    float eLdrlxolST41585272 = -16914387;    float eLdrlxolST10856238 = -228920547;    float eLdrlxolST77786823 = -974050579;    float eLdrlxolST58494068 = -510752418;    float eLdrlxolST40968344 = -523157638;    float eLdrlxolST15904342 = -684176152;    float eLdrlxolST76838938 = -836226938;    float eLdrlxolST8375282 = -130571268;    float eLdrlxolST99256973 = -627340014;    float eLdrlxolST48874184 = -870942883;    float eLdrlxolST52973732 = -420221878;    float eLdrlxolST14876748 = -283469095;    float eLdrlxolST92883545 = 12719178;    float eLdrlxolST26455412 = -972425665;    float eLdrlxolST96044418 = -108019734;    float eLdrlxolST3946713 = -78267031;    float eLdrlxolST56101113 = 62338433;    float eLdrlxolST76057271 = -588124944;    float eLdrlxolST23574526 = -717077981;    float eLdrlxolST69141832 = -66698562;    float eLdrlxolST64607181 = -672510576;    float eLdrlxolST74197900 = -895820186;    float eLdrlxolST47528496 = -739468776;    float eLdrlxolST92635103 = 63871919;    float eLdrlxolST80922804 = -227301410;    float eLdrlxolST65429628 = 81489052;    float eLdrlxolST81180654 = -211384986;    float eLdrlxolST88110385 = -874393806;     eLdrlxolST48543408 = eLdrlxolST3281885;     eLdrlxolST3281885 = eLdrlxolST45436863;     eLdrlxolST45436863 = eLdrlxolST45556652;     eLdrlxolST45556652 = eLdrlxolST56634566;     eLdrlxolST56634566 = eLdrlxolST85608184;     eLdrlxolST85608184 = eLdrlxolST44212595;     eLdrlxolST44212595 = eLdrlxolST94517380;     eLdrlxolST94517380 = eLdrlxolST28160361;     eLdrlxolST28160361 = eLdrlxolST23310547;     eLdrlxolST23310547 = eLdrlxolST43466546;     eLdrlxolST43466546 = eLdrlxolST32976268;     eLdrlxolST32976268 = eLdrlxolST55508667;     eLdrlxolST55508667 = eLdrlxolST84028617;     eLdrlxolST84028617 = eLdrlxolST65666895;     eLdrlxolST65666895 = eLdrlxolST18142615;     eLdrlxolST18142615 = eLdrlxolST13929292;     eLdrlxolST13929292 = eLdrlxolST74557776;     eLdrlxolST74557776 = eLdrlxolST56598844;     eLdrlxolST56598844 = eLdrlxolST22944422;     eLdrlxolST22944422 = eLdrlxolST60151917;     eLdrlxolST60151917 = eLdrlxolST70261545;     eLdrlxolST70261545 = eLdrlxolST53791281;     eLdrlxolST53791281 = eLdrlxolST56951797;     eLdrlxolST56951797 = eLdrlxolST75646134;     eLdrlxolST75646134 = eLdrlxolST97859962;     eLdrlxolST97859962 = eLdrlxolST87394073;     eLdrlxolST87394073 = eLdrlxolST91690100;     eLdrlxolST91690100 = eLdrlxolST73782335;     eLdrlxolST73782335 = eLdrlxolST64296788;     eLdrlxolST64296788 = eLdrlxolST66604262;     eLdrlxolST66604262 = eLdrlxolST51464073;     eLdrlxolST51464073 = eLdrlxolST77124160;     eLdrlxolST77124160 = eLdrlxolST95585233;     eLdrlxolST95585233 = eLdrlxolST54339134;     eLdrlxolST54339134 = eLdrlxolST73065130;     eLdrlxolST73065130 = eLdrlxolST57626898;     eLdrlxolST57626898 = eLdrlxolST34517699;     eLdrlxolST34517699 = eLdrlxolST32972504;     eLdrlxolST32972504 = eLdrlxolST45742607;     eLdrlxolST45742607 = eLdrlxolST45157598;     eLdrlxolST45157598 = eLdrlxolST1657849;     eLdrlxolST1657849 = eLdrlxolST29293201;     eLdrlxolST29293201 = eLdrlxolST37886939;     eLdrlxolST37886939 = eLdrlxolST80112858;     eLdrlxolST80112858 = eLdrlxolST67270853;     eLdrlxolST67270853 = eLdrlxolST98602988;     eLdrlxolST98602988 = eLdrlxolST38519889;     eLdrlxolST38519889 = eLdrlxolST38716368;     eLdrlxolST38716368 = eLdrlxolST58905587;     eLdrlxolST58905587 = eLdrlxolST71413243;     eLdrlxolST71413243 = eLdrlxolST40148850;     eLdrlxolST40148850 = eLdrlxolST55419655;     eLdrlxolST55419655 = eLdrlxolST73177447;     eLdrlxolST73177447 = eLdrlxolST39484121;     eLdrlxolST39484121 = eLdrlxolST78281863;     eLdrlxolST78281863 = eLdrlxolST49490604;     eLdrlxolST49490604 = eLdrlxolST88485066;     eLdrlxolST88485066 = eLdrlxolST69910517;     eLdrlxolST69910517 = eLdrlxolST58774604;     eLdrlxolST58774604 = eLdrlxolST98214111;     eLdrlxolST98214111 = eLdrlxolST52522495;     eLdrlxolST52522495 = eLdrlxolST20735045;     eLdrlxolST20735045 = eLdrlxolST63863572;     eLdrlxolST63863572 = eLdrlxolST56706285;     eLdrlxolST56706285 = eLdrlxolST92002472;     eLdrlxolST92002472 = eLdrlxolST55852108;     eLdrlxolST55852108 = eLdrlxolST59923433;     eLdrlxolST59923433 = eLdrlxolST29689484;     eLdrlxolST29689484 = eLdrlxolST92601765;     eLdrlxolST92601765 = eLdrlxolST60515717;     eLdrlxolST60515717 = eLdrlxolST79411593;     eLdrlxolST79411593 = eLdrlxolST41585272;     eLdrlxolST41585272 = eLdrlxolST10856238;     eLdrlxolST10856238 = eLdrlxolST77786823;     eLdrlxolST77786823 = eLdrlxolST58494068;     eLdrlxolST58494068 = eLdrlxolST40968344;     eLdrlxolST40968344 = eLdrlxolST15904342;     eLdrlxolST15904342 = eLdrlxolST76838938;     eLdrlxolST76838938 = eLdrlxolST8375282;     eLdrlxolST8375282 = eLdrlxolST99256973;     eLdrlxolST99256973 = eLdrlxolST48874184;     eLdrlxolST48874184 = eLdrlxolST52973732;     eLdrlxolST52973732 = eLdrlxolST14876748;     eLdrlxolST14876748 = eLdrlxolST92883545;     eLdrlxolST92883545 = eLdrlxolST26455412;     eLdrlxolST26455412 = eLdrlxolST96044418;     eLdrlxolST96044418 = eLdrlxolST3946713;     eLdrlxolST3946713 = eLdrlxolST56101113;     eLdrlxolST56101113 = eLdrlxolST76057271;     eLdrlxolST76057271 = eLdrlxolST23574526;     eLdrlxolST23574526 = eLdrlxolST69141832;     eLdrlxolST69141832 = eLdrlxolST64607181;     eLdrlxolST64607181 = eLdrlxolST74197900;     eLdrlxolST74197900 = eLdrlxolST47528496;     eLdrlxolST47528496 = eLdrlxolST92635103;     eLdrlxolST92635103 = eLdrlxolST80922804;     eLdrlxolST80922804 = eLdrlxolST65429628;     eLdrlxolST65429628 = eLdrlxolST81180654;     eLdrlxolST81180654 = eLdrlxolST88110385;     eLdrlxolST88110385 = eLdrlxolST48543408; }
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void oZDirHOkMq36281013() { float VurYMmyWpm70930525 = -508797769;    float VurYMmyWpm87629385 = -523067949;    float VurYMmyWpm74206141 = -42519381;    float VurYMmyWpm60563340 = -499146901;    float VurYMmyWpm54808820 = -508768426;    float VurYMmyWpm491302 = -948590230;    float VurYMmyWpm36368334 = -837787953;    float VurYMmyWpm70151239 = -980399056;    float VurYMmyWpm25323492 = -934899182;    float VurYMmyWpm15074132 = -951850762;    float VurYMmyWpm81008406 = -586175696;    float VurYMmyWpm88879216 = -881296408;    float VurYMmyWpm22516848 = -390609917;    float VurYMmyWpm26643370 = -850821168;    float VurYMmyWpm2402173 = 27216218;    float VurYMmyWpm62398527 = -838928350;    float VurYMmyWpm63752643 = -182243433;    float VurYMmyWpm80534192 = -287301424;    float VurYMmyWpm97880475 = -354321824;    float VurYMmyWpm1460705 = -919874851;    float VurYMmyWpm61035024 = 64051288;    float VurYMmyWpm34076764 = -933806967;    float VurYMmyWpm57084604 = -34035069;    float VurYMmyWpm83451584 = -974038520;    float VurYMmyWpm11186382 = -962259493;    float VurYMmyWpm86162956 = -438781488;    float VurYMmyWpm34483494 = -594737967;    float VurYMmyWpm97601550 = -47193362;    float VurYMmyWpm80674896 = -647651955;    float VurYMmyWpm73453967 = 16373058;    float VurYMmyWpm10895067 = -228032241;    float VurYMmyWpm9005265 = -821145479;    float VurYMmyWpm58162830 = -47382933;    float VurYMmyWpm54973921 = -348112771;    float VurYMmyWpm30568499 = 2765481;    float VurYMmyWpm14757999 = -649043774;    float VurYMmyWpm51066610 = -292466387;    float VurYMmyWpm88143799 = -792175049;    float VurYMmyWpm81280635 = -119690378;    float VurYMmyWpm65928321 = -26523182;    float VurYMmyWpm73914968 = -96367486;    float VurYMmyWpm90166240 = -191617677;    float VurYMmyWpm34843393 = -793413600;    float VurYMmyWpm37331059 = -171141975;    float VurYMmyWpm48096125 = -382962203;    float VurYMmyWpm75371135 = -192358196;    float VurYMmyWpm10900386 = -945957504;    float VurYMmyWpm83118564 = -722743583;    float VurYMmyWpm33176273 = -788233730;    float VurYMmyWpm55989333 = -61946000;    float VurYMmyWpm80282459 = -331126531;    float VurYMmyWpm14000107 = -605109707;    float VurYMmyWpm33734058 = -732965621;    float VurYMmyWpm33463992 = -715453217;    float VurYMmyWpm75245988 = -127028469;    float VurYMmyWpm36853762 = -574990803;    float VurYMmyWpm30544781 = -389032880;    float VurYMmyWpm90754557 = -68480861;    float VurYMmyWpm49376959 = -536887409;    float VurYMmyWpm68645863 = 30013061;    float VurYMmyWpm66007807 = -253852263;    float VurYMmyWpm38766784 = -690594591;    float VurYMmyWpm89476343 = -232747101;    float VurYMmyWpm51869524 = -851272241;    float VurYMmyWpm4179065 = -623818521;    float VurYMmyWpm72003141 = -765030217;    float VurYMmyWpm30716387 = -733913476;    float VurYMmyWpm67542927 = 57502853;    float VurYMmyWpm96074871 = -753586649;    float VurYMmyWpm87644173 = -323740008;    float VurYMmyWpm11331918 = -446461964;    float VurYMmyWpm75608844 = -390068385;    float VurYMmyWpm99253557 = -67611046;    float VurYMmyWpm31952154 = -227798643;    float VurYMmyWpm27545736 = -723507366;    float VurYMmyWpm70868784 = -744331036;    float VurYMmyWpm99233370 = -40393367;    float VurYMmyWpm19753546 = -862893095;    float VurYMmyWpm35355459 = -491076318;    float VurYMmyWpm35815247 = -669901297;    float VurYMmyWpm75262570 = -492823985;    float VurYMmyWpm51364930 = -871994385;    float VurYMmyWpm64425277 = -258959633;    float VurYMmyWpm24685564 = -485705956;    float VurYMmyWpm93171508 = -652500411;    float VurYMmyWpm96894960 = -622922535;    float VurYMmyWpm75271206 = 11820142;    float VurYMmyWpm24698838 = -331929717;    float VurYMmyWpm79727933 = -121084302;    float VurYMmyWpm93714736 = -422243717;    float VurYMmyWpm84213218 = -160010895;    float VurYMmyWpm60312053 = -123985527;    float VurYMmyWpm38766841 = -155287641;    float VurYMmyWpm12634772 = -49703440;    float VurYMmyWpm99920513 = -772670920;    float VurYMmyWpm35148185 = -405772895;    float VurYMmyWpm689898 = -958870576;    float VurYMmyWpm82973868 = -942141359;    float VurYMmyWpm33151994 = -547323454;    float VurYMmyWpm76092983 = -508797769;     VurYMmyWpm70930525 = VurYMmyWpm87629385;     VurYMmyWpm87629385 = VurYMmyWpm74206141;     VurYMmyWpm74206141 = VurYMmyWpm60563340;     VurYMmyWpm60563340 = VurYMmyWpm54808820;     VurYMmyWpm54808820 = VurYMmyWpm491302;     VurYMmyWpm491302 = VurYMmyWpm36368334;     VurYMmyWpm36368334 = VurYMmyWpm70151239;     VurYMmyWpm70151239 = VurYMmyWpm25323492;     VurYMmyWpm25323492 = VurYMmyWpm15074132;     VurYMmyWpm15074132 = VurYMmyWpm81008406;     VurYMmyWpm81008406 = VurYMmyWpm88879216;     VurYMmyWpm88879216 = VurYMmyWpm22516848;     VurYMmyWpm22516848 = VurYMmyWpm26643370;     VurYMmyWpm26643370 = VurYMmyWpm2402173;     VurYMmyWpm2402173 = VurYMmyWpm62398527;     VurYMmyWpm62398527 = VurYMmyWpm63752643;     VurYMmyWpm63752643 = VurYMmyWpm80534192;     VurYMmyWpm80534192 = VurYMmyWpm97880475;     VurYMmyWpm97880475 = VurYMmyWpm1460705;     VurYMmyWpm1460705 = VurYMmyWpm61035024;     VurYMmyWpm61035024 = VurYMmyWpm34076764;     VurYMmyWpm34076764 = VurYMmyWpm57084604;     VurYMmyWpm57084604 = VurYMmyWpm83451584;     VurYMmyWpm83451584 = VurYMmyWpm11186382;     VurYMmyWpm11186382 = VurYMmyWpm86162956;     VurYMmyWpm86162956 = VurYMmyWpm34483494;     VurYMmyWpm34483494 = VurYMmyWpm97601550;     VurYMmyWpm97601550 = VurYMmyWpm80674896;     VurYMmyWpm80674896 = VurYMmyWpm73453967;     VurYMmyWpm73453967 = VurYMmyWpm10895067;     VurYMmyWpm10895067 = VurYMmyWpm9005265;     VurYMmyWpm9005265 = VurYMmyWpm58162830;     VurYMmyWpm58162830 = VurYMmyWpm54973921;     VurYMmyWpm54973921 = VurYMmyWpm30568499;     VurYMmyWpm30568499 = VurYMmyWpm14757999;     VurYMmyWpm14757999 = VurYMmyWpm51066610;     VurYMmyWpm51066610 = VurYMmyWpm88143799;     VurYMmyWpm88143799 = VurYMmyWpm81280635;     VurYMmyWpm81280635 = VurYMmyWpm65928321;     VurYMmyWpm65928321 = VurYMmyWpm73914968;     VurYMmyWpm73914968 = VurYMmyWpm90166240;     VurYMmyWpm90166240 = VurYMmyWpm34843393;     VurYMmyWpm34843393 = VurYMmyWpm37331059;     VurYMmyWpm37331059 = VurYMmyWpm48096125;     VurYMmyWpm48096125 = VurYMmyWpm75371135;     VurYMmyWpm75371135 = VurYMmyWpm10900386;     VurYMmyWpm10900386 = VurYMmyWpm83118564;     VurYMmyWpm83118564 = VurYMmyWpm33176273;     VurYMmyWpm33176273 = VurYMmyWpm55989333;     VurYMmyWpm55989333 = VurYMmyWpm80282459;     VurYMmyWpm80282459 = VurYMmyWpm14000107;     VurYMmyWpm14000107 = VurYMmyWpm33734058;     VurYMmyWpm33734058 = VurYMmyWpm33463992;     VurYMmyWpm33463992 = VurYMmyWpm75245988;     VurYMmyWpm75245988 = VurYMmyWpm36853762;     VurYMmyWpm36853762 = VurYMmyWpm30544781;     VurYMmyWpm30544781 = VurYMmyWpm90754557;     VurYMmyWpm90754557 = VurYMmyWpm49376959;     VurYMmyWpm49376959 = VurYMmyWpm68645863;     VurYMmyWpm68645863 = VurYMmyWpm66007807;     VurYMmyWpm66007807 = VurYMmyWpm38766784;     VurYMmyWpm38766784 = VurYMmyWpm89476343;     VurYMmyWpm89476343 = VurYMmyWpm51869524;     VurYMmyWpm51869524 = VurYMmyWpm4179065;     VurYMmyWpm4179065 = VurYMmyWpm72003141;     VurYMmyWpm72003141 = VurYMmyWpm30716387;     VurYMmyWpm30716387 = VurYMmyWpm67542927;     VurYMmyWpm67542927 = VurYMmyWpm96074871;     VurYMmyWpm96074871 = VurYMmyWpm87644173;     VurYMmyWpm87644173 = VurYMmyWpm11331918;     VurYMmyWpm11331918 = VurYMmyWpm75608844;     VurYMmyWpm75608844 = VurYMmyWpm99253557;     VurYMmyWpm99253557 = VurYMmyWpm31952154;     VurYMmyWpm31952154 = VurYMmyWpm27545736;     VurYMmyWpm27545736 = VurYMmyWpm70868784;     VurYMmyWpm70868784 = VurYMmyWpm99233370;     VurYMmyWpm99233370 = VurYMmyWpm19753546;     VurYMmyWpm19753546 = VurYMmyWpm35355459;     VurYMmyWpm35355459 = VurYMmyWpm35815247;     VurYMmyWpm35815247 = VurYMmyWpm75262570;     VurYMmyWpm75262570 = VurYMmyWpm51364930;     VurYMmyWpm51364930 = VurYMmyWpm64425277;     VurYMmyWpm64425277 = VurYMmyWpm24685564;     VurYMmyWpm24685564 = VurYMmyWpm93171508;     VurYMmyWpm93171508 = VurYMmyWpm96894960;     VurYMmyWpm96894960 = VurYMmyWpm75271206;     VurYMmyWpm75271206 = VurYMmyWpm24698838;     VurYMmyWpm24698838 = VurYMmyWpm79727933;     VurYMmyWpm79727933 = VurYMmyWpm93714736;     VurYMmyWpm93714736 = VurYMmyWpm84213218;     VurYMmyWpm84213218 = VurYMmyWpm60312053;     VurYMmyWpm60312053 = VurYMmyWpm38766841;     VurYMmyWpm38766841 = VurYMmyWpm12634772;     VurYMmyWpm12634772 = VurYMmyWpm99920513;     VurYMmyWpm99920513 = VurYMmyWpm35148185;     VurYMmyWpm35148185 = VurYMmyWpm689898;     VurYMmyWpm689898 = VurYMmyWpm82973868;     VurYMmyWpm82973868 = VurYMmyWpm33151994;     VurYMmyWpm33151994 = VurYMmyWpm76092983;     VurYMmyWpm76092983 = VurYMmyWpm70930525; }
// Junk Finished
