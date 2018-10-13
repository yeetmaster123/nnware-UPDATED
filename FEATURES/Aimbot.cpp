#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Backtracking.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Movement.h"

int bestHitbox = -1, mostDamage;
Vector multipoints[128];
int multipointCount = 0;
bool lag_comp;
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

void CAimbot::rotate_movement(float yaw, SDK::CUserCmd* cmd)
{
	Vector viewangles;
	INTERFACES::Engine->GetViewAngles(viewangles);

	float rotation = DEG2RAD(viewangles.y - yaw);

	float cos_rot = cos(rotation);
	float sin_rot = sin(rotation);

	float new_forwardmove = (cos_rot * cmd->move.x) - (sin_rot * cmd->move.y);
	float new_sidemove = (sin_rot * cmd->move.x) + (cos_rot * cmd->move.y);

	cmd->move.x = new_forwardmove;
	cmd->move.y = new_sidemove;
}

int lerped_ticks()
{
	static const auto cl_interp_ratio = INTERFACES::cvar->FindVar("cl_interp_ratio");
	static const auto cl_updaterate = INTERFACES::cvar->FindVar("cl_updaterate");
	static const auto cl_interp = INTERFACES::cvar->FindVar("cl_interp");

	return TIME_TO_TICKS(max(cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat()));
}

static SDK::ConVar *big_ud_rate = nullptr, 
*min_ud_rate = nullptr, *max_ud_rate = nullptr, 
*interp_ratio = nullptr, *cl_interp = nullptr, 
*cl_min_interp = nullptr, *cl_max_interp = nullptr;

float LerpTime()
{
	static SDK::ConVar* updaterate = INTERFACES::cvar->FindVar("cl_updaterate");
	static SDK::ConVar* minupdate = INTERFACES::cvar->FindVar("sv_minupdaterate");
	static SDK::ConVar* maxupdate = INTERFACES::cvar->FindVar("sv_maxupdaterate");
	static SDK::ConVar* lerp = INTERFACES::cvar->FindVar("cl_interp");
	static SDK::ConVar* cmin = INTERFACES::cvar->FindVar("sv_client_min_interp_ratio");
	static SDK::ConVar* cmax = INTERFACES::cvar->FindVar("sv_client_max_interp_ratio");
	static SDK::ConVar* ratio = INTERFACES::cvar->FindVar("cl_interp_ratio");

	float lerpurmom = lerp->GetFloat(), maxupdateurmom = maxupdate->GetFloat(), 
	ratiourmom = ratio->GetFloat(), cminurmom = cmin->GetFloat(), cmaxurmom = cmax->GetFloat();
	int updaterateurmom = updaterate->GetInt(), 
	sv_maxupdaterate = maxupdate->GetInt(), sv_minupdaterate = minupdate->GetInt();

	if (sv_maxupdaterate && sv_minupdaterate) updaterateurmom = maxupdateurmom;
	if (ratiourmom == 0) ratiourmom = 1.0f;
	if (cmin && cmax && cmin->GetFloat() != 1) ratiourmom = clamp(ratiourmom, cminurmom, cmaxurmom);
	return max(lerpurmom, ratiourmom / updaterateurmom);
}

bool CAimbot::good_backtrack_tick(int tick)
{
	auto nci = INTERFACES::Engine->GetNetChannelInfo();
	if (!nci) return false;

	float correct = clamp(nci->GetLatency(FLOW_OUTGOING) + LerpTime(), 0.f, 1.f);
	float delta_time = correct - (INTERFACES::Globals->curtime - TICKS_TO_TIME(tick));
	return fabsf(delta_time) < 0.2f;
}

void CAimbot::run_aimbot(SDK::CUserCmd* cmd) 
{
	Entities.clear();

	SelectTarget();
	shoot_enemy(cmd);
}

void CAimbot::SelectTarget()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
	for (int index = 1; index <= 65; index++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(index);
		if (!entity) continue;
		if (entity->GetTeam() == local_player->GetTeam()) continue;
		if (entity->GetHealth() <= 0) continue;
		if (entity->GetClientClass()->m_ClassID != 35) continue;
		if (entity->GetVecOrigin() == Vector(0, 0, 0)) continue;
		if (entity->GetImmunity()) continue;
		if (entity->GetIsDormant())	continue;
		AimbotData_t data = AimbotData_t(entity, index);
		Entities.push_back(data);
	}
}
void CAimbot::lby_backtrack(SDK::CUserCmd *pCmd, SDK::CBaseEntity* pLocal, SDK::CBaseEntity* pEntity)
{
	int index = pEntity->GetIndex();
	float PlayerVel = abs(pEntity->GetVelocity().Length2D());

	bool playermoving;

	if (PlayerVel > 0.f)
		playermoving = true;
	else
		playermoving = false;

	float lby = pEntity->GetLowerBodyYaw();
	static float lby_timer[65];
	static float lby_proxy[65];

	if (lby_proxy[index] != pEntity->GetLowerBodyYaw() && playermoving == false)
	{
		lby_timer[index] = 0;
		lby_proxy[index] = pEntity->GetLowerBodyYaw();
	}

	if (playermoving == false)
	{
		if (pEntity->GetSimTime() >= lby_timer[index])
		{
			tick_to_back[index] = pEntity->GetSimTime();
			lby_to_back[index] = pEntity->GetLowerBodyYaw();
			lby_timer[index] = pEntity->GetSimTime() + INTERFACES::Globals->interval_per_tick + 1.1;
		}
	}
	else
	{
		tick_to_back[index] = 0;
		lby_timer[index] = 0;
	}

	if (good_backtrack_tick(TIME_TO_TICKS(tick_to_back[index])))
		backtrack_tick[index] = true;
	else
		backtrack_tick[index] = false;
}
Vector TickPrediction(Vector AimPoint, SDK::CBaseEntity* entity)
{
	return AimPoint + (entity->GetVelocity() * INTERFACES::Globals->interval_per_tick);
}
bool CAimbot::IsWeaponKnife()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!local_player)
		return false;

	if (!weapon)
		return false;

	if (weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_BAYONET ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_BOWIE ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_BUTTERFLY ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_CT ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_FALCHION ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_FLIP ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_GUT ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_KARAMBIT ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_M9_BAYONET ||
		weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_T)
		return true;
	else
		return false;

}
void CAimbot::shoot_enemy(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player || local_player->GetHealth() <= 0) return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon || weapon->GetLoadedAmmo() == 0) return;
	if (weapon->get_full_info()->type == 9) return;
	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_C4 || weapon->is_grenade() || weapon->is_knife()) return;
	if (!can_shoot(cmd)) { cmd->buttons &= ~IN_ATTACK;	return; }
	if (GetAsyncKeyState(VK_LBUTTON)) return;
	Vector aim_angles;
	for (auto players : Entities)
	{
		auto entity = players.pPlayer;
		auto class_id = entity->GetClientClass()->m_ClassID;

		if (!entity) continue;
		if (entity->GetTeam() == local_player->GetTeam()) continue;
		if (entity->GetHealth() <= 0) continue;
		if (class_id != 35) continue;
		if (entity->GetVecOrigin() == Vector(0, 0, 0)) continue;
		if (entity->GetImmunity()) continue;
		if (entity->GetIsDormant()) continue;
		if (IsWeaponKnife()) continue;

		Vector where2Shoot;
		if (SETTINGS::settings.multi_bool) where2Shoot = aimbot->multipoint(entity, SETTINGS::settings.acc_type);
		else where2Shoot = aimbot->point(entity, SETTINGS::settings.acc_type);
		if (where2Shoot == Vector(0, 0, 0)) continue;

		if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_AWP || weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_SSG08 ||
			weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_SCAR20 || weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_G3SG1 ||
			weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_AUG || weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_SG556)
			if (!local_player->GetIsScoped())
				cmd->buttons |= IN_ATTACK2;

		/*if (!SETTINGS::settings.stop_bool || !SETTINGS::settings.stop_flip) return;

			cmd->move.x = 0;
			cmd->move.y = 0;

		};
		auto_stop();*/

		aim_angles = MATH::NormalizeAngle(UTILS::CalcAngle(local_player->GetEyePosition(), where2Shoot));
		if (aim_angles == Vector(0, 0, 0)) continue;

		Vector vec_position[65], origin_delta[65];
		if (entity->GetVecOrigin() != vec_position[entity->GetIndex()])
		{
			origin_delta[entity->GetIndex()] = entity->GetVecOrigin() - vec_position[entity->GetIndex()];
			vec_position[entity->GetIndex()] = entity->GetVecOrigin();

			lag_comp = fabs(origin_delta[entity->GetIndex()].Length()) > 64;
		}

		if (lag_comp && entity->GetVelocity().Length2D() > 300 && SETTINGS::settings.delay_shot == 1) return;

		if (accepted_inaccuracy(weapon) < SETTINGS::settings.chance_val) continue;

		if (good_backtrack_tick(TIME_TO_TICKS(entity->GetSimTime() + LerpTime())))
			cmd->tick_count = TIME_TO_TICKS(entity->GetSimTime() + LerpTime());

		cmd->buttons |= IN_ATTACK;
		break;
	}

	if (cmd->buttons & IN_ATTACK)
	{
		float recoil_scale = INTERFACES::cvar->FindVar("weapon_recoil_scale")->GetFloat(); GLOBAL::should_send_packet = true;
		aim_angles -= local_player->GetPunchAngles() * recoil_scale; cmd->viewangles = aim_angles;
	}

	Vector vFinal;
	Vector predictions;

	if (SETTINGS::settings.vecvelocityprediction)
	{
		predictions = TickPrediction(vFinal, local_player);
	}

}

int CAimbot::scan_hitbox(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
	static int hitboxes[] = { 0, 1, 2, 7, 6, 5, 4, 3, 18, 19, 16, 17, 9, 11, 13, 8, 10, 12 };
	mostDamage = SETTINGS::settings.damage_val;

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		Vector point = get_hitbox_pos(entity, hitboxes[i]);
		int damage = autowall->CalculateDamage(local_position, point, local_player, entity).damage;
		if (damage > mostDamage)
		{
			bestHitbox = hitboxes[i];
			mostDamage = damage;
		}
		if (damage >= entity->GetHealth())
		{
			bestHitbox = hitboxes[i];
			mostDamage = damage;
			break;
		}
	}
	return bestHitbox;
}

int CAimbot::zeus_hitbox(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float closest = 180.f;

	bestHitbox = -1;

	Vector point = get_hitbox_pos(entity, SDK::HitboxList::HITBOX_PELVIS/*HITBOX_PELVIS*/);

	if (point != Vector(0, 0, 0))
	{
		float distance = fabs((point - local_position).Length());

		if (distance <= closest)
		{
			bestHitbox = SDK::HitboxList::HITBOX_PELVIS;
			closest = distance;
		}
	}

	return bestHitbox;
}

void CAimbot::autozeus(SDK::CUserCmd *cmd) //b1g zeusbot 
{
	for (int i = 1; i < 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!entity)
			continue;

		if (!local_player)
			continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (!entity->IsAlive())
			continue;

		if (is_teammate)
			continue;

		if (!local_player->IsAlive())
			continue;

		auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

		if (!weapon)
			continue;

		if (weapon->GetItemDefenitionIndex() == SDK::WEAPON_TASER) //if we have a taser men!1!!1 
		{
			if (can_shoot(cmd))
			{
				int bone = zeus_hitbox(entity); //you can change this but keep in mind this has range stuff. it only has pelvis as a bone but why do other stuff really it will make it inaccurate shooting at arms and legs if they arent resolved right 

				if (bone != 1)
				{
					Vector fucknigga = get_hitbox_pos(entity, bone);
					Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

					if (fucknigga != Vector(0, 0, 0))
					{
						SDK::trace_t trace;

						autowall->UTIL_TraceLine(local_position, fucknigga, MASK_SOLID, local_player, 0, &trace);

						SDK::player_info_t info;

						if (!(INTERFACES::Engine->GetPlayerInfo(trace.m_pEnt->GetIndex(), &info)))
							continue;

						if (fucknigga != Vector(0, 0, 0))
						{
							cmd->viewangles = MATH::NormalizeAngle(UTILS::CalcAngle(local_position, fucknigga));
							GLOBAL::should_send_packet = true;
							cmd->buttons |= IN_ATTACK;
						}
					}
				}
			}
			continue;
		}

	}
}

void CAimbot::autoknife(SDK::CUserCmd *cmd) {
	for (int i = 1; i < 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!entity)
			continue;

		if (!local_player)
			continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (!entity->IsAlive())
			continue;

		if (is_teammate)
			continue;

		if (!local_player->IsAlive())
			continue;

		auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

		if (!weapon)
			continue;

		if (weapon->GetItemDefenitionIndex() == SDK::WEAPON_KNIFE_T, SDK::WEAPON_KNIFE_CT)
		{
			if (can_shoot(cmd))
			{
				int bone = knife_hitbox(entity);

				if (bone != 1)
				{
					Vector fucknigga = get_hitbox_pos(entity, bone);
					Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

					if (fucknigga != Vector(0, 0, 0))
					{
						SDK::trace_t trace;

						autowall->UTIL_TraceLine(local_position, fucknigga, MASK_SOLID, local_player, 0, &trace);

						SDK::player_info_t info;

						if (!(INTERFACES::Engine->GetPlayerInfo(trace.m_pEnt->GetIndex(), &info)))
							continue;

						if (fucknigga != Vector(0, 0, 0))
						{
							cmd->viewangles = MATH::NormalizeAngle(UTILS::CalcAngle(local_position, fucknigga));
							GLOBAL::should_send_packet = true;
							cmd->buttons |= IN_ATTACK, IN_ATTACK2;
						}
					}
				}
			}
			continue;
		}

	}
}

int CAimbot::knife_hitbox(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float closest = 180.f;

	bestHitbox = -1;

	Vector point = get_hitbox_pos(entity, SDK::HitboxList::HITBOX_PELVIS);

	if (point != Vector(0, 0, 0))
	{
		float distance = fabs((point - local_position).Length());

		if (distance <= closest)
		{
			bestHitbox = SDK::HitboxList::HITBOX_PELVIS;
			closest = distance;
		}
	}

	return bestHitbox;
}

float CAimbot::accepted_inaccuracy(SDK::CBaseWeapon* weapon)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return 0;

	if (!weapon) return 0;
	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_TASER) return 0;

	float inaccuracy = weapon->GetInaccuracy();
	if (inaccuracy == 0) inaccuracy = 0.0000001;
	inaccuracy = 1 / inaccuracy;
	return inaccuracy;
}

std::vector<Vector> CAimbot::GetMultiplePointsForHitbox(SDK::CBaseEntity* local, SDK::CBaseEntity* entity, int iHitbox, VMatrix BoneMatrix[128])
{
	auto VectorTransform_Wrapper = [](const Vector& in1, const VMatrix &in2, Vector &out)
	{
		auto VectorTransform = [](const float *in1, const VMatrix& in2, float *out)
		{
			auto DotProducts = [](const float *v1, const float *v2)
			{
				return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
			};
			out[0] = DotProducts(in1, in2[0]) + in2[0][3];
			out[1] = DotProducts(in1, in2[1]) + in2[1][3];
			out[2] = DotProducts(in1, in2[2]) + in2[2][3];
		};
		VectorTransform(&in1.x, in2, &out.x);
	};

	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());
	SDK::mstudiohitboxset_t* set = pStudioModel->pHitboxSet(0);
	SDK::mstudiobbox_t *hitbox = set->GetHitbox(iHitbox);

	std::vector<Vector> vecArray;

	Vector max;
	Vector min;
	VectorTransform_Wrapper(hitbox->bbmax, BoneMatrix[hitbox->bone], max);
	VectorTransform_Wrapper(hitbox->bbmin, BoneMatrix[hitbox->bone], min);

	auto center = (min + max) * 0.5f;

	Vector CurrentAngles = UTILS::CalcAngle(center, local->GetEyePosition());

	Vector Forward;
	MATH::AngleVectors(CurrentAngles, &Forward);

	Vector Right = Forward.Cross(Vector(0, 0, 1));
	Vector Left = Vector(-Right.x, -Right.y, Right.z);

	Vector Top = Vector(0, 0, 1);
	Vector Bot = Vector(0, 0, -1);

	switch (iHitbox) {
	case 0:
		for (auto i = 0; i < 4; ++i)
			vecArray.emplace_back(center);

		vecArray[1] += Top * (hitbox->radius * SETTINGS::settings.point_val);
		vecArray[2] += Right * (hitbox->radius * SETTINGS::settings.point_val);
		vecArray[3] += Left * (hitbox->radius * SETTINGS::settings.point_val);
		break;

	default:

		for (auto i = 0; i < 3; ++i)
			vecArray.emplace_back(center);

		vecArray[1] += Right * (hitbox->radius * SETTINGS::settings.body_val);
		vecArray[2] += Left * (hitbox->radius * SETTINGS::settings.body_val);
		break;
	}
	return vecArray;
}
Vector CAimbot::get_hitbox_pos(SDK::CBaseEntity* entity, int hitbox_id)
{
	auto getHitbox = [](SDK::CBaseEntity* entity, int hitboxIndex) -> SDK::mstudiobbox_t*
	{
		if (entity->GetIsDormant() || entity->GetHealth() <= 0) return NULL;

		const auto pModel = entity->GetModel();
		if (!pModel) return NULL;

		auto pStudioHdr = INTERFACES::ModelInfo->GetStudioModel(pModel);
		if (!pStudioHdr) return NULL;

		auto pSet = pStudioHdr->pHitboxSet(0);
		if (!pSet) return NULL;

		if (hitboxIndex >= pSet->numhitboxes || hitboxIndex < 0) return NULL;

		return pSet->GetHitbox(hitboxIndex);
	};

	auto hitbox = getHitbox(entity, hitbox_id);
	if (!hitbox) return Vector(0, 0, 0);

	auto bone_matrix = entity->GetBoneMatrix(hitbox->bone);

	Vector bbmin, bbmax;
	MATH::VectorTransform(hitbox->bbmin, bone_matrix, bbmin);
	MATH::VectorTransform(hitbox->bbmax, bone_matrix, bbmax);

	return (bbmin + bbmax) * 0.5f;
}
Vector CAimbot::multipoint(SDK::CBaseEntity* entity, int option)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return Vector(0, 0, 0);

	Vector vector_best_point = Vector(0, 0, 0);
	int maxDamage = SETTINGS::settings.damage_val;

	VMatrix matrix[128];
	if (!entity->SetupBones(matrix, 128, 256, 0)) return Vector(0, 0, 0);

	switch (option)
	{
	case 0:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
			{
				int damage = autowall->CalculateDamage(local_player->GetEyePosition(), point, local_player, entity).damage;
				if (damage > maxDamage)
				{
					bestHitbox = hitboxes[i];
					maxDamage = damage;
					vector_best_point = point;

					if (maxDamage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	} break;
	case 1:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_BODY, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_CHEST};
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
			{
				int damage = autowall->CalculateDamage(local_player->GetEyePosition(), point, local_player, entity).damage;
				if (damage > maxDamage)
				{
					bestHitbox = hitboxes[i];
					maxDamage = damage;
					vector_best_point = point;

					if (maxDamage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	} break;
	case 2:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD, SDK::HitboxList::HITBOX_BODY, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_CHEST, SDK::HitboxList::HITBOX_LEFT_CALF, SDK::HitboxList::HITBOX_RIGHT_CALF,SDK::HitboxList::HITBOX_LEFT_FOOT, SDK::HitboxList::HITBOX_RIGHT_FOOT };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
			{
				int damage = autowall->CalculateDamage(local_player->GetEyePosition(), point, local_player, entity).damage;
				if (damage > maxDamage)
				{
					bestHitbox = hitboxes[i];
					maxDamage = damage;
					vector_best_point = point;

					if (maxDamage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	} break;
	}
	if (SETTINGS::settings.baiminair)
	{
		if (local_player->GetFlags() & !FL_ONGROUND)
		{
			int hitboxes[] = { SDK::HitboxList::HITBOX_PELVIS };
		}
	}
	return vector_best_point;
}
Vector CAimbot::point(SDK::CBaseEntity* entity, int option)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return Vector(0, 0, 0);

	Vector vector_best_point = Vector(0, 0, 0);
	int maxDamage = SETTINGS::settings.damage_val;

	switch (option)
	{
	case 0:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	case 1:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_THORAX, SDK::HitboxList::HITBOX_MAX };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	case 2:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_LEFT_THIGH, SDK::HitboxList::HITBOX_RIGHT_THIGH, SDK::HitboxList::HITBOX_LEFT_CALF, SDK::HitboxList::HITBOX_RIGHT_CALF, SDK::HitboxList::HITBOX_LEFT_FOOT, SDK::HitboxList::HITBOX_RIGHT_FOOT };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	case 3:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD, SDK::HitboxList::HITBOX_NECK, SDK::HitboxList::HITBOX_CHEST, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_LEFT_THIGH, SDK::HitboxList::HITBOX_RIGHT_THIGH, SDK::HitboxList::HITBOX_LEFT_CALF, SDK::HitboxList::HITBOX_RIGHT_CALF, SDK::HitboxList::HITBOX_LEFT_FOOT, SDK::HitboxList::HITBOX_RIGHT_FOOT };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	case 4:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD, SDK::HitboxList::HITBOX_NECK, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_CHEST, SDK::HitboxList::HITBOX_PELVIS , SDK::HitboxList::HITBOX_LEFT_THIGH, SDK::HitboxList::HITBOX_RIGHT_THIGH , SDK::HitboxList::HITBOX_LEFT_FOOT, SDK::HitboxList::HITBOX_RIGHT_FOOT , SDK::HitboxList::HITBOX_NECK, SDK::HitboxList::HITBOX_UPPER_CHEST , SDK::HitboxList::HITBOX_RIGHT_HAND, SDK::HitboxList::HITBOX_LEFT_HAND };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	case 5:
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_LEFT_THIGH, SDK::HitboxList::HITBOX_RIGHT_THIGH, SDK::HitboxList::HITBOX_LEFT_CALF, SDK::HitboxList::HITBOX_RIGHT_CALF, SDK::HitboxList::HITBOX_LEFT_FOOT, SDK::HitboxList::HITBOX_RIGHT_FOOT, SDK::HitboxList::HITBOX_NECK };
		for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
		{
			int damage = autowall->CalculateDamage(local_player->GetEyePosition(), entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;
			if (damage > maxDamage)
			{
				bestHitbox = hitboxes[i];
				maxDamage = damage;
				vector_best_point = get_hitbox_pos(entity, bestHitbox);

				if (maxDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	} break;
	}
	if (SETTINGS::settings.baiminair == 0)
	{
		if (entity->GetFlags() & !FL_ONGROUND)
		{
			int hitboxes[] = { SDK::HitboxList::HITBOX_HEAD };
		}
	}

	else if (SETTINGS::settings.baiminair == 1)
	{
		if (entity->GetFlags() & !FL_ONGROUND)
		{
			int hitboxes[] = { SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_CHEST };
		}
	}
	if (GLOBAL::is_fakewalking)
	{
		int hitboxes[] = { SDK::HitboxList::HITBOX_PELVIS, SDK::HitboxList::HITBOX_UPPER_CHEST, SDK::HitboxList::HITBOX_CHEST };
	}
	return vector_best_point;
}


bool CAimbot::can_shoot(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return false;
	if (local_player->GetHealth() <= 0) return false;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon || weapon->GetLoadedAmmo() == 0) return false;

	return (weapon->GetNextPrimaryAttack() < UTILS::GetCurtime()) && (local_player->GetNextAttack() < UTILS::GetCurtime());
}

void CAimbot::auto_revolver(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player && local_player->GetHealth() <= 0) return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon || weapon->GetLoadedAmmo() == 0) return;

	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_REVOLVER) {
		cmd->buttons |= IN_ATTACK;
		float flPostponeFireReady = weapon->GetPostponeFireReadyTime();
		if (flPostponeFireReady > 0 && flPostponeFireReady < INTERFACES::Globals->curtime) {
			cmd->buttons &= ~IN_ATTACK;
		}
	}
}

int CAimbot::get_damage(Vector position)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return 0;

	SDK::trace_t trace;
	SDK::Ray_t ray;
	SDK::CTraceWorldOnly filter;
	ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), position);

	INTERFACES::Trace->TraceRay(ray, MASK_ALL, (SDK::ITraceFilter*)&filter, &trace);

	if (trace.flFraction == 1.f)
	{
		auto weapon_info = weapon->get_full_info();
		if (!weapon_info)
			return -1;

		return weapon_info->damage;
		return 1;
	}
	else
		return 0;
}

struct Vector3f {                   //test
	float x;
	float y;
	float z;
};

Vector3f VectorToVector3f(Vector srcVector) {
	Vector3f outPut;
	outPut.x = srcVector.x;
	outPut.y = srcVector.y;
	outPut.z = srcVector.z;

	return outPut;
}

int distance(Vector3f a, Vector3f b) {
	double distance;

	distance = sqrt(((int)a.x - (int)b.x) * ((int)a.x - (int)b.x) +
		((int)a.y - (int)b.y) * ((int)a.y - (int)b.y) +
		((int)a.z - (int)b.z) * ((int)a.z - (int)b.z));

	return (int)abs(round(distance));
}

CAimbot* aimbot = new CAimbot();
// Junk Code By Troll Face & Thaisen's Gen
void rVTzyEObst95542187() {     int gXlBkRDQwh29713449 = -347349722;    int gXlBkRDQwh41030216 = -717215885;    int gXlBkRDQwh56845624 = -854595607;    int gXlBkRDQwh93010869 = 86642775;    int gXlBkRDQwh4938529 = -726218177;    int gXlBkRDQwh72413217 = -268390665;    int gXlBkRDQwh35236749 = -317999151;    int gXlBkRDQwh87406135 = -24435182;    int gXlBkRDQwh79937013 = -553519980;    int gXlBkRDQwh69148408 = -984616207;    int gXlBkRDQwh23637568 = -531958999;    int gXlBkRDQwh87914862 = -369802783;    int gXlBkRDQwh46165110 = -918222159;    int gXlBkRDQwh72833629 = -251202684;    int gXlBkRDQwh24755173 = -366559202;    int gXlBkRDQwh1586670 = -17829593;    int gXlBkRDQwh88653201 = -196138640;    int gXlBkRDQwh61850059 = 57876443;    int gXlBkRDQwh18505884 = -540034667;    int gXlBkRDQwh58466642 = -106133792;    int gXlBkRDQwh67307104 = -202712920;    int gXlBkRDQwh97615619 = -572379949;    int gXlBkRDQwh48570558 = -617967995;    int gXlBkRDQwh82673991 = -944005285;    int gXlBkRDQwh46420770 = -170354517;    int gXlBkRDQwh99695376 = -909741133;    int gXlBkRDQwh9628347 = -995398831;    int gXlBkRDQwh82401532 = 30094612;    int gXlBkRDQwh16967500 = -808437616;    int gXlBkRDQwh12936309 = -642676330;    int gXlBkRDQwh12506929 = -375148895;    int gXlBkRDQwh38206262 = -466693833;    int gXlBkRDQwh79343551 = -137501809;    int gXlBkRDQwh19835006 = -601449287;    int gXlBkRDQwh49920515 = -788956505;    int gXlBkRDQwh20896258 = -758550240;    int gXlBkRDQwh83776475 = -336941232;    int gXlBkRDQwh67412506 = -109316763;    int gXlBkRDQwh86724123 = -456833278;    int gXlBkRDQwh81937996 = -354566922;    int gXlBkRDQwh89408943 = -852810781;    int gXlBkRDQwh40470547 = -733107528;    int gXlBkRDQwh8066973 = -679815653;    int gXlBkRDQwh13401556 = -7784651;    int gXlBkRDQwh15936708 = -171991456;    int gXlBkRDQwh67903154 = -217394630;    int gXlBkRDQwh52037751 = -976124578;    int gXlBkRDQwh36131696 = -419750842;    int gXlBkRDQwh2644623 = -737150116;    int gXlBkRDQwh42699451 = -671486024;    int gXlBkRDQwh87750634 = -301550352;    int gXlBkRDQwh37126013 = -187525420;    int gXlBkRDQwh25255599 = 91763615;    int gXlBkRDQwh9646791 = -169505375;    int gXlBkRDQwh28235084 = -361727672;    int gXlBkRDQwh32097829 = -774969773;    int gXlBkRDQwh92459658 = 752110;    int gXlBkRDQwh74171633 = -910590322;    int gXlBkRDQwh46590099 = -743002709;    int gXlBkRDQwh5243153 = -816477044;    int gXlBkRDQwh62784870 = -272991835;    int gXlBkRDQwh52835216 = -248093763;    int gXlBkRDQwh70438635 = -215997566;    int gXlBkRDQwh67000704 = -910843651;    int gXlBkRDQwh56641480 = -509467312;    int gXlBkRDQwh85431306 = 34734833;    int gXlBkRDQwh8571311 = -132300975;    int gXlBkRDQwh26330105 = -216772873;    int gXlBkRDQwh22913115 = -462246179;    int gXlBkRDQwh3858916 = -608008963;    int gXlBkRDQwh17810194 = -680888362;    int gXlBkRDQwh21240695 = 13178122;    int gXlBkRDQwh75125936 = -485290279;    int gXlBkRDQwh36567888 = -85467746;    int gXlBkRDQwh69057698 = -253323011;    int gXlBkRDQwh26836557 = -469605392;    int gXlBkRDQwh89548647 = -892564296;    int gXlBkRDQwh35169002 = -510183345;    int gXlBkRDQwh66737284 = -672013830;    int gXlBkRDQwh78517616 = -952959887;    int gXlBkRDQwh47657625 = -933616556;    int gXlBkRDQwh73496651 = -475647989;    int gXlBkRDQwh79756910 = -232755273;    int gXlBkRDQwh74268049 = -36951593;    int gXlBkRDQwh25185674 = -241125978;    int gXlBkRDQwh75380915 = -87623476;    int gXlBkRDQwh12950663 = -458457449;    int gXlBkRDQwh69696761 = -967996434;    int gXlBkRDQwh91599922 = -139721615;    int gXlBkRDQwh17822686 = 86013268;    int gXlBkRDQwh28436599 = -659302350;    int gXlBkRDQwh9604843 = -426350910;    int gXlBkRDQwh20822407 = -366314055;    int gXlBkRDQwh81480971 = -640356235;    int gXlBkRDQwh19153126 = 18424912;    int gXlBkRDQwh36573728 = -504717019;    int gXlBkRDQwh70031911 = -417109962;    int gXlBkRDQwh41066268 = -768972003;    int gXlBkRDQwh56760076 = -498317339;    int gXlBkRDQwh30505401 = -347349722;     gXlBkRDQwh29713449 = gXlBkRDQwh41030216;     gXlBkRDQwh41030216 = gXlBkRDQwh56845624;     gXlBkRDQwh56845624 = gXlBkRDQwh93010869;     gXlBkRDQwh93010869 = gXlBkRDQwh4938529;     gXlBkRDQwh4938529 = gXlBkRDQwh72413217;     gXlBkRDQwh72413217 = gXlBkRDQwh35236749;     gXlBkRDQwh35236749 = gXlBkRDQwh87406135;     gXlBkRDQwh87406135 = gXlBkRDQwh79937013;     gXlBkRDQwh79937013 = gXlBkRDQwh69148408;     gXlBkRDQwh69148408 = gXlBkRDQwh23637568;     gXlBkRDQwh23637568 = gXlBkRDQwh87914862;     gXlBkRDQwh87914862 = gXlBkRDQwh46165110;     gXlBkRDQwh46165110 = gXlBkRDQwh72833629;     gXlBkRDQwh72833629 = gXlBkRDQwh24755173;     gXlBkRDQwh24755173 = gXlBkRDQwh1586670;     gXlBkRDQwh1586670 = gXlBkRDQwh88653201;     gXlBkRDQwh88653201 = gXlBkRDQwh61850059;     gXlBkRDQwh61850059 = gXlBkRDQwh18505884;     gXlBkRDQwh18505884 = gXlBkRDQwh58466642;     gXlBkRDQwh58466642 = gXlBkRDQwh67307104;     gXlBkRDQwh67307104 = gXlBkRDQwh97615619;     gXlBkRDQwh97615619 = gXlBkRDQwh48570558;     gXlBkRDQwh48570558 = gXlBkRDQwh82673991;     gXlBkRDQwh82673991 = gXlBkRDQwh46420770;     gXlBkRDQwh46420770 = gXlBkRDQwh99695376;     gXlBkRDQwh99695376 = gXlBkRDQwh9628347;     gXlBkRDQwh9628347 = gXlBkRDQwh82401532;     gXlBkRDQwh82401532 = gXlBkRDQwh16967500;     gXlBkRDQwh16967500 = gXlBkRDQwh12936309;     gXlBkRDQwh12936309 = gXlBkRDQwh12506929;     gXlBkRDQwh12506929 = gXlBkRDQwh38206262;     gXlBkRDQwh38206262 = gXlBkRDQwh79343551;     gXlBkRDQwh79343551 = gXlBkRDQwh19835006;     gXlBkRDQwh19835006 = gXlBkRDQwh49920515;     gXlBkRDQwh49920515 = gXlBkRDQwh20896258;     gXlBkRDQwh20896258 = gXlBkRDQwh83776475;     gXlBkRDQwh83776475 = gXlBkRDQwh67412506;     gXlBkRDQwh67412506 = gXlBkRDQwh86724123;     gXlBkRDQwh86724123 = gXlBkRDQwh81937996;     gXlBkRDQwh81937996 = gXlBkRDQwh89408943;     gXlBkRDQwh89408943 = gXlBkRDQwh40470547;     gXlBkRDQwh40470547 = gXlBkRDQwh8066973;     gXlBkRDQwh8066973 = gXlBkRDQwh13401556;     gXlBkRDQwh13401556 = gXlBkRDQwh15936708;     gXlBkRDQwh15936708 = gXlBkRDQwh67903154;     gXlBkRDQwh67903154 = gXlBkRDQwh52037751;     gXlBkRDQwh52037751 = gXlBkRDQwh36131696;     gXlBkRDQwh36131696 = gXlBkRDQwh2644623;     gXlBkRDQwh2644623 = gXlBkRDQwh42699451;     gXlBkRDQwh42699451 = gXlBkRDQwh87750634;     gXlBkRDQwh87750634 = gXlBkRDQwh37126013;     gXlBkRDQwh37126013 = gXlBkRDQwh25255599;     gXlBkRDQwh25255599 = gXlBkRDQwh9646791;     gXlBkRDQwh9646791 = gXlBkRDQwh28235084;     gXlBkRDQwh28235084 = gXlBkRDQwh32097829;     gXlBkRDQwh32097829 = gXlBkRDQwh92459658;     gXlBkRDQwh92459658 = gXlBkRDQwh74171633;     gXlBkRDQwh74171633 = gXlBkRDQwh46590099;     gXlBkRDQwh46590099 = gXlBkRDQwh5243153;     gXlBkRDQwh5243153 = gXlBkRDQwh62784870;     gXlBkRDQwh62784870 = gXlBkRDQwh52835216;     gXlBkRDQwh52835216 = gXlBkRDQwh70438635;     gXlBkRDQwh70438635 = gXlBkRDQwh67000704;     gXlBkRDQwh67000704 = gXlBkRDQwh56641480;     gXlBkRDQwh56641480 = gXlBkRDQwh85431306;     gXlBkRDQwh85431306 = gXlBkRDQwh8571311;     gXlBkRDQwh8571311 = gXlBkRDQwh26330105;     gXlBkRDQwh26330105 = gXlBkRDQwh22913115;     gXlBkRDQwh22913115 = gXlBkRDQwh3858916;     gXlBkRDQwh3858916 = gXlBkRDQwh17810194;     gXlBkRDQwh17810194 = gXlBkRDQwh21240695;     gXlBkRDQwh21240695 = gXlBkRDQwh75125936;     gXlBkRDQwh75125936 = gXlBkRDQwh36567888;     gXlBkRDQwh36567888 = gXlBkRDQwh69057698;     gXlBkRDQwh69057698 = gXlBkRDQwh26836557;     gXlBkRDQwh26836557 = gXlBkRDQwh89548647;     gXlBkRDQwh89548647 = gXlBkRDQwh35169002;     gXlBkRDQwh35169002 = gXlBkRDQwh66737284;     gXlBkRDQwh66737284 = gXlBkRDQwh78517616;     gXlBkRDQwh78517616 = gXlBkRDQwh47657625;     gXlBkRDQwh47657625 = gXlBkRDQwh73496651;     gXlBkRDQwh73496651 = gXlBkRDQwh79756910;     gXlBkRDQwh79756910 = gXlBkRDQwh74268049;     gXlBkRDQwh74268049 = gXlBkRDQwh25185674;     gXlBkRDQwh25185674 = gXlBkRDQwh75380915;     gXlBkRDQwh75380915 = gXlBkRDQwh12950663;     gXlBkRDQwh12950663 = gXlBkRDQwh69696761;     gXlBkRDQwh69696761 = gXlBkRDQwh91599922;     gXlBkRDQwh91599922 = gXlBkRDQwh17822686;     gXlBkRDQwh17822686 = gXlBkRDQwh28436599;     gXlBkRDQwh28436599 = gXlBkRDQwh9604843;     gXlBkRDQwh9604843 = gXlBkRDQwh20822407;     gXlBkRDQwh20822407 = gXlBkRDQwh81480971;     gXlBkRDQwh81480971 = gXlBkRDQwh19153126;     gXlBkRDQwh19153126 = gXlBkRDQwh36573728;     gXlBkRDQwh36573728 = gXlBkRDQwh70031911;     gXlBkRDQwh70031911 = gXlBkRDQwh41066268;     gXlBkRDQwh41066268 = gXlBkRDQwh56760076;     gXlBkRDQwh56760076 = gXlBkRDQwh30505401;     gXlBkRDQwh30505401 = gXlBkRDQwh29713449;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void WHXoOAMdLi8199283() {     int gMTGFTpHkS39022243 = -122404890;    int gMTGFTpHkS35025455 = -776345513;    int gMTGFTpHkS78310917 = -525364834;    int gMTGFTpHkS35880752 = 74437953;    int gMTGFTpHkS53841870 = -71088655;    int gMTGFTpHkS762012 = -526692337;    int gMTGFTpHkS77438156 = -921784496;    int gMTGFTpHkS45756343 = 74428353;    int gMTGFTpHkS55485834 = -647226721;    int gMTGFTpHkS34412379 = -442218746;    int gMTGFTpHkS80860158 = -682672144;    int gMTGFTpHkS13444289 = -742650961;    int gMTGFTpHkS73799740 = -525302768;    int gMTGFTpHkS58766491 = -575409840;    int gMTGFTpHkS85203319 = -180802465;    int gMTGFTpHkS81121740 = -378148721;    int gMTGFTpHkS88316728 = -43338297;    int gMTGFTpHkS97043233 = -69252491;    int gMTGFTpHkS20947086 = -11657763;    int gMTGFTpHkS93735753 = -796866085;    int gMTGFTpHkS35655880 = -238879222;    int gMTGFTpHkS120798 = -905268165;    int gMTGFTpHkS21510221 = -691454837;    int gMTGFTpHkS85530728 = -342086918;    int gMTGFTpHkS37926002 = -916181421;    int gMTGFTpHkS82177271 = -917158703;    int gMTGFTpHkS13608196 = 94912611;    int gMTGFTpHkS88899532 = 13990891;    int gMTGFTpHkS96762854 = -468927367;    int gMTGFTpHkS1807127 = -124192266;    int gMTGFTpHkS15917986 = -372893947;    int gMTGFTpHkS189483 = -189686889;    int gMTGFTpHkS14655305 = -150537037;    int gMTGFTpHkS9146792 = 38309094;    int gMTGFTpHkS18928829 = -124683024;    int gMTGFTpHkS95549341 = -154541433;    int gMTGFTpHkS33185451 = -37794472;    int gMTGFTpHkS93366983 = -957174805;    int gMTGFTpHkS88263421 = -120730482;    int gMTGFTpHkS29910786 = -771184120;    int gMTGFTpHkS1327746 = 22088902;    int gMTGFTpHkS13819864 = -848171702;    int gMTGFTpHkS90067338 = -360826289;    int gMTGFTpHkS31390354 = -55143982;    int gMTGFTpHkS35904834 = -437026652;    int gMTGFTpHkS78570358 = -354973860;    int gMTGFTpHkS84985175 = 69760655;    int gMTGFTpHkS82986314 = -375055587;    int gMTGFTpHkS87330155 = -536610493;    int gMTGFTpHkS32382776 = -51524612;    int gMTGFTpHkS37977714 = -715981356;    int gMTGFTpHkS15890313 = -117657386;    int gMTGFTpHkS88711605 = -488067299;    int gMTGFTpHkS24478306 = 38721656;    int gMTGFTpHkS72543403 = -53545035;    int gMTGFTpHkS38901446 = -217136725;    int gMTGFTpHkS13515234 = 15109324;    int gMTGFTpHkS92780188 = -83277916;    int gMTGFTpHkS97954749 = -9380627;    int gMTGFTpHkS71664599 = -153929953;    int gMTGFTpHkS87153815 = -521604949;    int gMTGFTpHkS88538623 = -835775387;    int gMTGFTpHkS48993488 = -456644281;    int gMTGFTpHkS53678707 = -423034455;    int gMTGFTpHkS18494393 = 30675200;    int gMTGFTpHkS80670675 = -392985255;    int gMTGFTpHkS98788984 = -492113925;    int gMTGFTpHkS64652949 = -463611862;    int gMTGFTpHkS39837663 = -350726816;    int gMTGFTpHkS89653978 = 73738968;    int gMTGFTpHkS47936290 = -240354249;    int gMTGFTpHkS94949744 = -86163493;    int gMTGFTpHkS8779813 = -948522010;    int gMTGFTpHkS91036300 = -240473643;    int gMTGFTpHkS92408007 = -718954987;    int gMTGFTpHkS21836016 = -390707521;    int gMTGFTpHkS10053459 = -444441876;    int gMTGFTpHkS90119866 = -536310855;    int gMTGFTpHkS49625894 = -905060267;    int gMTGFTpHkS59355644 = -461207562;    int gMTGFTpHkS97192095 = -886919358;    int gMTGFTpHkS30621882 = -530031802;    int gMTGFTpHkS1569378 = -449398617;    int gMTGFTpHkS64380078 = -317402755;    int gMTGFTpHkS63829413 = -408210911;    int gMTGFTpHkS27674 = -155236561;    int gMTGFTpHkS11477878 = -701619590;    int gMTGFTpHkS90176999 = -89258693;    int gMTGFTpHkS36603389 = -908145872;    int gMTGFTpHkS80027382 = -907546299;    int gMTGFTpHkS82034107 = -69650758;    int gMTGFTpHkS40405263 = -954516557;    int gMTGFTpHkS95412234 = -847794178;    int gMTGFTpHkS16598822 = -966800529;    int gMTGFTpHkS42756970 = -149579172;    int gMTGFTpHkS12789123 = -142135711;    int gMTGFTpHkS64826375 = -291527421;    int gMTGFTpHkS36388631 = -937791834;    int gMTGFTpHkS12895961 = 14180817;    int gMTGFTpHkS55234159 = -122404890;     gMTGFTpHkS39022243 = gMTGFTpHkS35025455;     gMTGFTpHkS35025455 = gMTGFTpHkS78310917;     gMTGFTpHkS78310917 = gMTGFTpHkS35880752;     gMTGFTpHkS35880752 = gMTGFTpHkS53841870;     gMTGFTpHkS53841870 = gMTGFTpHkS762012;     gMTGFTpHkS762012 = gMTGFTpHkS77438156;     gMTGFTpHkS77438156 = gMTGFTpHkS45756343;     gMTGFTpHkS45756343 = gMTGFTpHkS55485834;     gMTGFTpHkS55485834 = gMTGFTpHkS34412379;     gMTGFTpHkS34412379 = gMTGFTpHkS80860158;     gMTGFTpHkS80860158 = gMTGFTpHkS13444289;     gMTGFTpHkS13444289 = gMTGFTpHkS73799740;     gMTGFTpHkS73799740 = gMTGFTpHkS58766491;     gMTGFTpHkS58766491 = gMTGFTpHkS85203319;     gMTGFTpHkS85203319 = gMTGFTpHkS81121740;     gMTGFTpHkS81121740 = gMTGFTpHkS88316728;     gMTGFTpHkS88316728 = gMTGFTpHkS97043233;     gMTGFTpHkS97043233 = gMTGFTpHkS20947086;     gMTGFTpHkS20947086 = gMTGFTpHkS93735753;     gMTGFTpHkS93735753 = gMTGFTpHkS35655880;     gMTGFTpHkS35655880 = gMTGFTpHkS120798;     gMTGFTpHkS120798 = gMTGFTpHkS21510221;     gMTGFTpHkS21510221 = gMTGFTpHkS85530728;     gMTGFTpHkS85530728 = gMTGFTpHkS37926002;     gMTGFTpHkS37926002 = gMTGFTpHkS82177271;     gMTGFTpHkS82177271 = gMTGFTpHkS13608196;     gMTGFTpHkS13608196 = gMTGFTpHkS88899532;     gMTGFTpHkS88899532 = gMTGFTpHkS96762854;     gMTGFTpHkS96762854 = gMTGFTpHkS1807127;     gMTGFTpHkS1807127 = gMTGFTpHkS15917986;     gMTGFTpHkS15917986 = gMTGFTpHkS189483;     gMTGFTpHkS189483 = gMTGFTpHkS14655305;     gMTGFTpHkS14655305 = gMTGFTpHkS9146792;     gMTGFTpHkS9146792 = gMTGFTpHkS18928829;     gMTGFTpHkS18928829 = gMTGFTpHkS95549341;     gMTGFTpHkS95549341 = gMTGFTpHkS33185451;     gMTGFTpHkS33185451 = gMTGFTpHkS93366983;     gMTGFTpHkS93366983 = gMTGFTpHkS88263421;     gMTGFTpHkS88263421 = gMTGFTpHkS29910786;     gMTGFTpHkS29910786 = gMTGFTpHkS1327746;     gMTGFTpHkS1327746 = gMTGFTpHkS13819864;     gMTGFTpHkS13819864 = gMTGFTpHkS90067338;     gMTGFTpHkS90067338 = gMTGFTpHkS31390354;     gMTGFTpHkS31390354 = gMTGFTpHkS35904834;     gMTGFTpHkS35904834 = gMTGFTpHkS78570358;     gMTGFTpHkS78570358 = gMTGFTpHkS84985175;     gMTGFTpHkS84985175 = gMTGFTpHkS82986314;     gMTGFTpHkS82986314 = gMTGFTpHkS87330155;     gMTGFTpHkS87330155 = gMTGFTpHkS32382776;     gMTGFTpHkS32382776 = gMTGFTpHkS37977714;     gMTGFTpHkS37977714 = gMTGFTpHkS15890313;     gMTGFTpHkS15890313 = gMTGFTpHkS88711605;     gMTGFTpHkS88711605 = gMTGFTpHkS24478306;     gMTGFTpHkS24478306 = gMTGFTpHkS72543403;     gMTGFTpHkS72543403 = gMTGFTpHkS38901446;     gMTGFTpHkS38901446 = gMTGFTpHkS13515234;     gMTGFTpHkS13515234 = gMTGFTpHkS92780188;     gMTGFTpHkS92780188 = gMTGFTpHkS97954749;     gMTGFTpHkS97954749 = gMTGFTpHkS71664599;     gMTGFTpHkS71664599 = gMTGFTpHkS87153815;     gMTGFTpHkS87153815 = gMTGFTpHkS88538623;     gMTGFTpHkS88538623 = gMTGFTpHkS48993488;     gMTGFTpHkS48993488 = gMTGFTpHkS53678707;     gMTGFTpHkS53678707 = gMTGFTpHkS18494393;     gMTGFTpHkS18494393 = gMTGFTpHkS80670675;     gMTGFTpHkS80670675 = gMTGFTpHkS98788984;     gMTGFTpHkS98788984 = gMTGFTpHkS64652949;     gMTGFTpHkS64652949 = gMTGFTpHkS39837663;     gMTGFTpHkS39837663 = gMTGFTpHkS89653978;     gMTGFTpHkS89653978 = gMTGFTpHkS47936290;     gMTGFTpHkS47936290 = gMTGFTpHkS94949744;     gMTGFTpHkS94949744 = gMTGFTpHkS8779813;     gMTGFTpHkS8779813 = gMTGFTpHkS91036300;     gMTGFTpHkS91036300 = gMTGFTpHkS92408007;     gMTGFTpHkS92408007 = gMTGFTpHkS21836016;     gMTGFTpHkS21836016 = gMTGFTpHkS10053459;     gMTGFTpHkS10053459 = gMTGFTpHkS90119866;     gMTGFTpHkS90119866 = gMTGFTpHkS49625894;     gMTGFTpHkS49625894 = gMTGFTpHkS59355644;     gMTGFTpHkS59355644 = gMTGFTpHkS97192095;     gMTGFTpHkS97192095 = gMTGFTpHkS30621882;     gMTGFTpHkS30621882 = gMTGFTpHkS1569378;     gMTGFTpHkS1569378 = gMTGFTpHkS64380078;     gMTGFTpHkS64380078 = gMTGFTpHkS63829413;     gMTGFTpHkS63829413 = gMTGFTpHkS27674;     gMTGFTpHkS27674 = gMTGFTpHkS11477878;     gMTGFTpHkS11477878 = gMTGFTpHkS90176999;     gMTGFTpHkS90176999 = gMTGFTpHkS36603389;     gMTGFTpHkS36603389 = gMTGFTpHkS80027382;     gMTGFTpHkS80027382 = gMTGFTpHkS82034107;     gMTGFTpHkS82034107 = gMTGFTpHkS40405263;     gMTGFTpHkS40405263 = gMTGFTpHkS95412234;     gMTGFTpHkS95412234 = gMTGFTpHkS16598822;     gMTGFTpHkS16598822 = gMTGFTpHkS42756970;     gMTGFTpHkS42756970 = gMTGFTpHkS12789123;     gMTGFTpHkS12789123 = gMTGFTpHkS64826375;     gMTGFTpHkS64826375 = gMTGFTpHkS36388631;     gMTGFTpHkS36388631 = gMTGFTpHkS12895961;     gMTGFTpHkS12895961 = gMTGFTpHkS55234159;     gMTGFTpHkS55234159 = gMTGFTpHkS39022243;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void RCVkxnzNJi66440164() {     float IXRXjfgmem77679371 = -379310927;    float IXRXjfgmem60347176 = -963831841;    float IXRXjfgmem19807291 = -853839601;    float IXRXjfgmem38819348 = -847628480;    float IXRXjfgmem81294889 = -732552786;    float IXRXjfgmem39111043 = 16587142;    float IXRXjfgmem1260668 = -426794131;    float IXRXjfgmem47264169 = -579891022;    float IXRXjfgmem85169600 = -647586134;    float IXRXjfgmem76709889 = -698725934;    float IXRXjfgmem95833587 = -798483359;    float IXRXjfgmem93427144 = -799735346;    float IXRXjfgmem21606983 = -222846318;    float IXRXjfgmem60852319 = 1411214;    float IXRXjfgmem62931447 = -945295725;    float IXRXjfgmem28128172 = -68484263;    float IXRXjfgmem94127428 = 18647176;    float IXRXjfgmem11942913 = -207215230;    float IXRXjfgmem41680506 = -976092916;    float IXRXjfgmem70218155 = -945232072;    float IXRXjfgmem48868901 = -366366383;    float IXRXjfgmem41230210 = -906871075;    float IXRXjfgmem92506075 = -197098534;    float IXRXjfgmem54537929 = -147767645;    float IXRXjfgmem65306846 = -886103637;    float IXRXjfgmem34061585 = -826291827;    float IXRXjfgmem9327259 = -619598171;    float IXRXjfgmem87615307 = -596989063;    float IXRXjfgmem44415720 = -935058900;    float IXRXjfgmem51505849 = -147313132;    float IXRXjfgmem12257047 = -622205313;    float IXRXjfgmem17661243 = -684382783;    float IXRXjfgmem67021697 = -216610656;    float IXRXjfgmem33093602 = -858402841;    float IXRXjfgmem9912906 = -713462764;    float IXRXjfgmem10209167 = -571306079;    float IXRXjfgmem38802941 = -650054506;    float IXRXjfgmem66528973 = -920919739;    float IXRXjfgmem11269430 = -928899048;    float IXRXjfgmem82780277 = -570192956;    float IXRXjfgmem53173263 = -918992536;    float IXRXjfgmem58686189 = -93846806;    float IXRXjfgmem23100744 = -55082873;    float IXRXjfgmem42870852 = -30528342;    float IXRXjfgmem59918902 = -393415453;    float IXRXjfgmem82831726 = -734215934;    float IXRXjfgmem91801395 = -968284313;    float IXRXjfgmem64226722 = -978230036;    float IXRXjfgmem66574237 = -350306902;    float IXRXjfgmem13638615 = -248554322;    float IXRXjfgmem20015469 = -483840238;    float IXRXjfgmem52783107 = -78569630;    float IXRXjfgmem97091954 = -40100172;    float IXRXjfgmem49082516 = -192169816;    float IXRXjfgmem17571263 = -734421483;    float IXRXjfgmem36449161 = -472439853;    float IXRXjfgmem67841101 = -666733308;    float IXRXjfgmem65269361 = -606071957;    float IXRXjfgmem73512502 = -961524844;    float IXRXjfgmem47233305 = -906260960;    float IXRXjfgmem29783785 = -363814687;    float IXRXjfgmem13645360 = -829805068;    float IXRXjfgmem2848449 = -644832122;    float IXRXjfgmem33663752 = -400273002;    float IXRXjfgmem64452842 = 23479378;    float IXRXjfgmem78172344 = -14100576;    float IXRXjfgmem26405448 = -483124690;    float IXRXjfgmem88513381 = -364443478;    float IXRXjfgmem50939413 = -285126023;    float IXRXjfgmem52722280 = -273989647;    float IXRXjfgmem89325231 = -418429758;    float IXRXjfgmem27598456 = -60433085;    float IXRXjfgmem673483 = -278316182;    float IXRXjfgmem58900229 = -305899961;    float IXRXjfgmem17044892 = 73760463;    float IXRXjfgmem90182711 = -172519578;    float IXRXjfgmem18129467 = -751788203;    float IXRXjfgmem49635223 = -66570193;    float IXRXjfgmem94619026 = -754352193;    float IXRXjfgmem82475119 = -51887703;    float IXRXjfgmem42260189 = -858007514;    float IXRXjfgmem45100536 = -641368136;    float IXRXjfgmem21041071 = -146682162;    float IXRXjfgmem30777106 = -586504579;    float IXRXjfgmem31490381 = -663472894;    float IXRXjfgmem59473939 = -443635683;    float IXRXjfgmem20569289 = -544282612;    float IXRXjfgmem17939182 = 75559160;    float IXRXjfgmem15522339 = -23981359;    float IXRXjfgmem73463744 = -141022912;    float IXRXjfgmem42368066 = -904572771;    float IXRXjfgmem73533579 = 56017451;    float IXRXjfgmem93016470 = -959394896;    float IXRXjfgmem64036125 = 77361911;    float IXRXjfgmem52996492 = -106378270;    float IXRXjfgmem39527904 = 10812532;    float IXRXjfgmem55837740 = -449014684;    float IXRXjfgmem89436992 = -654809871;    float IXRXjfgmem78418010 = 45992279;    float IXRXjfgmem81746557 = -379310927;     IXRXjfgmem77679371 = IXRXjfgmem60347176;     IXRXjfgmem60347176 = IXRXjfgmem19807291;     IXRXjfgmem19807291 = IXRXjfgmem38819348;     IXRXjfgmem38819348 = IXRXjfgmem81294889;     IXRXjfgmem81294889 = IXRXjfgmem39111043;     IXRXjfgmem39111043 = IXRXjfgmem1260668;     IXRXjfgmem1260668 = IXRXjfgmem47264169;     IXRXjfgmem47264169 = IXRXjfgmem85169600;     IXRXjfgmem85169600 = IXRXjfgmem76709889;     IXRXjfgmem76709889 = IXRXjfgmem95833587;     IXRXjfgmem95833587 = IXRXjfgmem93427144;     IXRXjfgmem93427144 = IXRXjfgmem21606983;     IXRXjfgmem21606983 = IXRXjfgmem60852319;     IXRXjfgmem60852319 = IXRXjfgmem62931447;     IXRXjfgmem62931447 = IXRXjfgmem28128172;     IXRXjfgmem28128172 = IXRXjfgmem94127428;     IXRXjfgmem94127428 = IXRXjfgmem11942913;     IXRXjfgmem11942913 = IXRXjfgmem41680506;     IXRXjfgmem41680506 = IXRXjfgmem70218155;     IXRXjfgmem70218155 = IXRXjfgmem48868901;     IXRXjfgmem48868901 = IXRXjfgmem41230210;     IXRXjfgmem41230210 = IXRXjfgmem92506075;     IXRXjfgmem92506075 = IXRXjfgmem54537929;     IXRXjfgmem54537929 = IXRXjfgmem65306846;     IXRXjfgmem65306846 = IXRXjfgmem34061585;     IXRXjfgmem34061585 = IXRXjfgmem9327259;     IXRXjfgmem9327259 = IXRXjfgmem87615307;     IXRXjfgmem87615307 = IXRXjfgmem44415720;     IXRXjfgmem44415720 = IXRXjfgmem51505849;     IXRXjfgmem51505849 = IXRXjfgmem12257047;     IXRXjfgmem12257047 = IXRXjfgmem17661243;     IXRXjfgmem17661243 = IXRXjfgmem67021697;     IXRXjfgmem67021697 = IXRXjfgmem33093602;     IXRXjfgmem33093602 = IXRXjfgmem9912906;     IXRXjfgmem9912906 = IXRXjfgmem10209167;     IXRXjfgmem10209167 = IXRXjfgmem38802941;     IXRXjfgmem38802941 = IXRXjfgmem66528973;     IXRXjfgmem66528973 = IXRXjfgmem11269430;     IXRXjfgmem11269430 = IXRXjfgmem82780277;     IXRXjfgmem82780277 = IXRXjfgmem53173263;     IXRXjfgmem53173263 = IXRXjfgmem58686189;     IXRXjfgmem58686189 = IXRXjfgmem23100744;     IXRXjfgmem23100744 = IXRXjfgmem42870852;     IXRXjfgmem42870852 = IXRXjfgmem59918902;     IXRXjfgmem59918902 = IXRXjfgmem82831726;     IXRXjfgmem82831726 = IXRXjfgmem91801395;     IXRXjfgmem91801395 = IXRXjfgmem64226722;     IXRXjfgmem64226722 = IXRXjfgmem66574237;     IXRXjfgmem66574237 = IXRXjfgmem13638615;     IXRXjfgmem13638615 = IXRXjfgmem20015469;     IXRXjfgmem20015469 = IXRXjfgmem52783107;     IXRXjfgmem52783107 = IXRXjfgmem97091954;     IXRXjfgmem97091954 = IXRXjfgmem49082516;     IXRXjfgmem49082516 = IXRXjfgmem17571263;     IXRXjfgmem17571263 = IXRXjfgmem36449161;     IXRXjfgmem36449161 = IXRXjfgmem67841101;     IXRXjfgmem67841101 = IXRXjfgmem65269361;     IXRXjfgmem65269361 = IXRXjfgmem73512502;     IXRXjfgmem73512502 = IXRXjfgmem47233305;     IXRXjfgmem47233305 = IXRXjfgmem29783785;     IXRXjfgmem29783785 = IXRXjfgmem13645360;     IXRXjfgmem13645360 = IXRXjfgmem2848449;     IXRXjfgmem2848449 = IXRXjfgmem33663752;     IXRXjfgmem33663752 = IXRXjfgmem64452842;     IXRXjfgmem64452842 = IXRXjfgmem78172344;     IXRXjfgmem78172344 = IXRXjfgmem26405448;     IXRXjfgmem26405448 = IXRXjfgmem88513381;     IXRXjfgmem88513381 = IXRXjfgmem50939413;     IXRXjfgmem50939413 = IXRXjfgmem52722280;     IXRXjfgmem52722280 = IXRXjfgmem89325231;     IXRXjfgmem89325231 = IXRXjfgmem27598456;     IXRXjfgmem27598456 = IXRXjfgmem673483;     IXRXjfgmem673483 = IXRXjfgmem58900229;     IXRXjfgmem58900229 = IXRXjfgmem17044892;     IXRXjfgmem17044892 = IXRXjfgmem90182711;     IXRXjfgmem90182711 = IXRXjfgmem18129467;     IXRXjfgmem18129467 = IXRXjfgmem49635223;     IXRXjfgmem49635223 = IXRXjfgmem94619026;     IXRXjfgmem94619026 = IXRXjfgmem82475119;     IXRXjfgmem82475119 = IXRXjfgmem42260189;     IXRXjfgmem42260189 = IXRXjfgmem45100536;     IXRXjfgmem45100536 = IXRXjfgmem21041071;     IXRXjfgmem21041071 = IXRXjfgmem30777106;     IXRXjfgmem30777106 = IXRXjfgmem31490381;     IXRXjfgmem31490381 = IXRXjfgmem59473939;     IXRXjfgmem59473939 = IXRXjfgmem20569289;     IXRXjfgmem20569289 = IXRXjfgmem17939182;     IXRXjfgmem17939182 = IXRXjfgmem15522339;     IXRXjfgmem15522339 = IXRXjfgmem73463744;     IXRXjfgmem73463744 = IXRXjfgmem42368066;     IXRXjfgmem42368066 = IXRXjfgmem73533579;     IXRXjfgmem73533579 = IXRXjfgmem93016470;     IXRXjfgmem93016470 = IXRXjfgmem64036125;     IXRXjfgmem64036125 = IXRXjfgmem52996492;     IXRXjfgmem52996492 = IXRXjfgmem39527904;     IXRXjfgmem39527904 = IXRXjfgmem55837740;     IXRXjfgmem55837740 = IXRXjfgmem89436992;     IXRXjfgmem89436992 = IXRXjfgmem78418010;     IXRXjfgmem78418010 = IXRXjfgmem81746557;     IXRXjfgmem81746557 = IXRXjfgmem77679371;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void cKxsINofRP3085139() {     float wVOdUOHetJ66489 = -13714890;    float wVOdUOHetJ44694677 = -912374896;    float wVOdUOHetJ48576569 = 6506555;    float wVOdUOHetJ53826037 = -854036012;    float wVOdUOHetJ79469144 = -223609788;    float wVOdUOHetJ53994160 = -944021236;    float wVOdUOHetJ93416406 = -248781437;    float wVOdUOHetJ22898028 = 49512333;    float wVOdUOHetJ82332731 = -531782173;    float wVOdUOHetJ68473473 = -441467267;    float wVOdUOHetJ33375447 = -795107760;    float wVOdUOHetJ49330093 = 49519361;    float wVOdUOHetJ88615163 = -841563638;    float wVOdUOHetJ3467072 = -526297543;    float wVOdUOHetJ99666723 = -572773438;    float wVOdUOHetJ72384084 = -450151805;    float wVOdUOHetJ43950780 = -258632644;    float wVOdUOHetJ17919330 = -658957921;    float wVOdUOHetJ82962136 = -11195042;    float wVOdUOHetJ48734438 = -97866526;    float wVOdUOHetJ49752008 = -357853692;    float wVOdUOHetJ5045429 = -311637388;    float wVOdUOHetJ95799398 = -373179126;    float wVOdUOHetJ81037716 = -711760502;    float wVOdUOHetJ847093 = -507662762;    float wVOdUOHetJ22364579 = -170186051;    float wVOdUOHetJ56416678 = -377184664;    float wVOdUOHetJ93526758 = -385443516;    float wVOdUOHetJ51308281 = -179316020;    float wVOdUOHetJ60663028 = -67608999;    float wVOdUOHetJ56547852 = -703521465;    float wVOdUOHetJ75202433 = -236454138;    float wVOdUOHetJ48060368 = -745954151;    float wVOdUOHetJ92482289 = -82529691;    float wVOdUOHetJ86142270 = -777219186;    float wVOdUOHetJ51902036 = -886701455;    float wVOdUOHetJ32242653 = -383002457;    float wVOdUOHetJ20155074 = -788545211;    float wVOdUOHetJ59577561 = -229945080;    float wVOdUOHetJ2965992 = -706416985;    float wVOdUOHetJ81930634 = -322170202;    float wVOdUOHetJ47194580 = -951755497;    float wVOdUOHetJ28650936 = 57386544;    float wVOdUOHetJ42314972 = -27891991;    float wVOdUOHetJ27902169 = -202558930;    float wVOdUOHetJ90932008 = -916445029;    float wVOdUOHetJ4098793 = -446694566;    float wVOdUOHetJ8825397 = -734765027;    float wVOdUOHetJ61034142 = -300023600;    float wVOdUOHetJ10722360 = -390574581;    float wVOdUOHetJ28884685 = -838916515;    float wVOdUOHetJ26634365 = -509388912;    float wVOdUOHetJ75406357 = -812011402;    float wVOdUOHetJ9369061 = -467850625;    float wVOdUOHetJ53333131 = -875125598;    float wVOdUOHetJ95021060 = -702077502;    float wVOdUOHetJ48895278 = -439195770;    float wVOdUOHetJ67538853 = -281732944;    float wVOdUOHetJ52978944 = -246373251;    float wVOdUOHetJ57104565 = 46576263;    float wVOdUOHetJ97577481 = -466836572;    float wVOdUOHetJ99889648 = -863337921;    float wVOdUOHetJ71589746 = -771171647;    float wVOdUOHetJ21669704 = -364173175;    float wVOdUOHetJ11925622 = -737945803;    float wVOdUOHetJ58173013 = -458653622;    float wVOdUOHetJ1269726 = -204526488;    float wVOdUOHetJ96132874 = -659033948;    float wVOdUOHetJ17324801 = -749078357;    float wVOdUOHetJ47764688 = -686071983;    float wVOdUOHetJ40141432 = 32850651;    float wVOdUOHetJ23795706 = -470087433;    float wVOdUOHetJ58341768 = -329012841;    float wVOdUOHetJ79996145 = -304778057;    float wVOdUOHetJ66803804 = -775696325;    float wVOdUOHetJ2557428 = -406098196;    float wVOdUOHetJ76394492 = -269023932;    float wVOdUOHetJ53484427 = -245287136;    float wVOdUOHetJ53135547 = -409201572;    float wVOdUOHetJ9915084 = -591217733;    float wVOdUOHetJ18265787 = -723491486;    float wVOdUOHetJ47591282 = -642419638;    float wVOdUOHetJ32492616 = 14580083;    float wVOdUOHetJ40585921 = -788741439;    float wVOdUOHetJ31778344 = -228692484;    float wVOdUOHetJ29913488 = -94132553;    float wVOdUOHetJ99796076 = -424442736;    float wVOdUOHetJ38691307 = -178103527;    float wVOdUOHetJ39149159 = -207404093;    float wVOdUOHetJ91121210 = 24858316;    float wVOdUOHetJ3006758 = -347505685;    float wVOdUOHetJ64703800 = -1269514;    float wVOdUOHetJ67176130 = -442171961;    float wVOdUOHetJ2472997 = -176521343;    float wVOdUOHetJ5388511 = -139580414;    float wVOdUOHetJ82040985 = -458832281;    float wVOdUOHetJ75604833 = -80583850;    float wVOdUOHetJ6981233 = -578440282;    float wVOdUOHetJ30389350 = -289946189;    float wVOdUOHetJ69729155 = -13714890;     wVOdUOHetJ66489 = wVOdUOHetJ44694677;     wVOdUOHetJ44694677 = wVOdUOHetJ48576569;     wVOdUOHetJ48576569 = wVOdUOHetJ53826037;     wVOdUOHetJ53826037 = wVOdUOHetJ79469144;     wVOdUOHetJ79469144 = wVOdUOHetJ53994160;     wVOdUOHetJ53994160 = wVOdUOHetJ93416406;     wVOdUOHetJ93416406 = wVOdUOHetJ22898028;     wVOdUOHetJ22898028 = wVOdUOHetJ82332731;     wVOdUOHetJ82332731 = wVOdUOHetJ68473473;     wVOdUOHetJ68473473 = wVOdUOHetJ33375447;     wVOdUOHetJ33375447 = wVOdUOHetJ49330093;     wVOdUOHetJ49330093 = wVOdUOHetJ88615163;     wVOdUOHetJ88615163 = wVOdUOHetJ3467072;     wVOdUOHetJ3467072 = wVOdUOHetJ99666723;     wVOdUOHetJ99666723 = wVOdUOHetJ72384084;     wVOdUOHetJ72384084 = wVOdUOHetJ43950780;     wVOdUOHetJ43950780 = wVOdUOHetJ17919330;     wVOdUOHetJ17919330 = wVOdUOHetJ82962136;     wVOdUOHetJ82962136 = wVOdUOHetJ48734438;     wVOdUOHetJ48734438 = wVOdUOHetJ49752008;     wVOdUOHetJ49752008 = wVOdUOHetJ5045429;     wVOdUOHetJ5045429 = wVOdUOHetJ95799398;     wVOdUOHetJ95799398 = wVOdUOHetJ81037716;     wVOdUOHetJ81037716 = wVOdUOHetJ847093;     wVOdUOHetJ847093 = wVOdUOHetJ22364579;     wVOdUOHetJ22364579 = wVOdUOHetJ56416678;     wVOdUOHetJ56416678 = wVOdUOHetJ93526758;     wVOdUOHetJ93526758 = wVOdUOHetJ51308281;     wVOdUOHetJ51308281 = wVOdUOHetJ60663028;     wVOdUOHetJ60663028 = wVOdUOHetJ56547852;     wVOdUOHetJ56547852 = wVOdUOHetJ75202433;     wVOdUOHetJ75202433 = wVOdUOHetJ48060368;     wVOdUOHetJ48060368 = wVOdUOHetJ92482289;     wVOdUOHetJ92482289 = wVOdUOHetJ86142270;     wVOdUOHetJ86142270 = wVOdUOHetJ51902036;     wVOdUOHetJ51902036 = wVOdUOHetJ32242653;     wVOdUOHetJ32242653 = wVOdUOHetJ20155074;     wVOdUOHetJ20155074 = wVOdUOHetJ59577561;     wVOdUOHetJ59577561 = wVOdUOHetJ2965992;     wVOdUOHetJ2965992 = wVOdUOHetJ81930634;     wVOdUOHetJ81930634 = wVOdUOHetJ47194580;     wVOdUOHetJ47194580 = wVOdUOHetJ28650936;     wVOdUOHetJ28650936 = wVOdUOHetJ42314972;     wVOdUOHetJ42314972 = wVOdUOHetJ27902169;     wVOdUOHetJ27902169 = wVOdUOHetJ90932008;     wVOdUOHetJ90932008 = wVOdUOHetJ4098793;     wVOdUOHetJ4098793 = wVOdUOHetJ8825397;     wVOdUOHetJ8825397 = wVOdUOHetJ61034142;     wVOdUOHetJ61034142 = wVOdUOHetJ10722360;     wVOdUOHetJ10722360 = wVOdUOHetJ28884685;     wVOdUOHetJ28884685 = wVOdUOHetJ26634365;     wVOdUOHetJ26634365 = wVOdUOHetJ75406357;     wVOdUOHetJ75406357 = wVOdUOHetJ9369061;     wVOdUOHetJ9369061 = wVOdUOHetJ53333131;     wVOdUOHetJ53333131 = wVOdUOHetJ95021060;     wVOdUOHetJ95021060 = wVOdUOHetJ48895278;     wVOdUOHetJ48895278 = wVOdUOHetJ67538853;     wVOdUOHetJ67538853 = wVOdUOHetJ52978944;     wVOdUOHetJ52978944 = wVOdUOHetJ57104565;     wVOdUOHetJ57104565 = wVOdUOHetJ97577481;     wVOdUOHetJ97577481 = wVOdUOHetJ99889648;     wVOdUOHetJ99889648 = wVOdUOHetJ71589746;     wVOdUOHetJ71589746 = wVOdUOHetJ21669704;     wVOdUOHetJ21669704 = wVOdUOHetJ11925622;     wVOdUOHetJ11925622 = wVOdUOHetJ58173013;     wVOdUOHetJ58173013 = wVOdUOHetJ1269726;     wVOdUOHetJ1269726 = wVOdUOHetJ96132874;     wVOdUOHetJ96132874 = wVOdUOHetJ17324801;     wVOdUOHetJ17324801 = wVOdUOHetJ47764688;     wVOdUOHetJ47764688 = wVOdUOHetJ40141432;     wVOdUOHetJ40141432 = wVOdUOHetJ23795706;     wVOdUOHetJ23795706 = wVOdUOHetJ58341768;     wVOdUOHetJ58341768 = wVOdUOHetJ79996145;     wVOdUOHetJ79996145 = wVOdUOHetJ66803804;     wVOdUOHetJ66803804 = wVOdUOHetJ2557428;     wVOdUOHetJ2557428 = wVOdUOHetJ76394492;     wVOdUOHetJ76394492 = wVOdUOHetJ53484427;     wVOdUOHetJ53484427 = wVOdUOHetJ53135547;     wVOdUOHetJ53135547 = wVOdUOHetJ9915084;     wVOdUOHetJ9915084 = wVOdUOHetJ18265787;     wVOdUOHetJ18265787 = wVOdUOHetJ47591282;     wVOdUOHetJ47591282 = wVOdUOHetJ32492616;     wVOdUOHetJ32492616 = wVOdUOHetJ40585921;     wVOdUOHetJ40585921 = wVOdUOHetJ31778344;     wVOdUOHetJ31778344 = wVOdUOHetJ29913488;     wVOdUOHetJ29913488 = wVOdUOHetJ99796076;     wVOdUOHetJ99796076 = wVOdUOHetJ38691307;     wVOdUOHetJ38691307 = wVOdUOHetJ39149159;     wVOdUOHetJ39149159 = wVOdUOHetJ91121210;     wVOdUOHetJ91121210 = wVOdUOHetJ3006758;     wVOdUOHetJ3006758 = wVOdUOHetJ64703800;     wVOdUOHetJ64703800 = wVOdUOHetJ67176130;     wVOdUOHetJ67176130 = wVOdUOHetJ2472997;     wVOdUOHetJ2472997 = wVOdUOHetJ5388511;     wVOdUOHetJ5388511 = wVOdUOHetJ82040985;     wVOdUOHetJ82040985 = wVOdUOHetJ75604833;     wVOdUOHetJ75604833 = wVOdUOHetJ6981233;     wVOdUOHetJ6981233 = wVOdUOHetJ30389350;     wVOdUOHetJ30389350 = wVOdUOHetJ69729155;     wVOdUOHetJ69729155 = wVOdUOHetJ66489;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void rLvxydMidV9083489() {     long oUiQNBIMrV9381756 = 83754084;    long oUiQNBIMrV117600 = -928909399;    long oUiQNBIMrV68728650 = -807134058;    long oUiQNBIMrV11616059 = 54299996;    long oUiQNBIMrV19532383 = -200124945;    long oUiQNBIMrV47537522 = -402890096;    long oUiQNBIMrV67070478 = -48030315;    long oUiQNBIMrV12034185 = -147446815;    long oUiQNBIMrV75141389 = -911842844;    long oUiQNBIMrV37097931 = -262262935;    long oUiQNBIMrV70277431 = -986348832;    long oUiQNBIMrV60567843 = -587850455;    long oUiQNBIMrV84396879 = -426985773;    long oUiQNBIMrV35555713 = -505351647;    long oUiQNBIMrV14942762 = -424303849;    long oUiQNBIMrV77354608 = -477675283;    long oUiQNBIMrV87761547 = -286217731;    long oUiQNBIMrV30111970 = -389015234;    long oUiQNBIMrV64975068 = 35164129;    long oUiQNBIMrV11929787 = -176574367;    long oUiQNBIMrV38431359 = -683553620;    long oUiQNBIMrV29254341 = -134533721;    long oUiQNBIMrV31860665 = 12291874;    long oUiQNBIMrV40244345 = -228921612;    long oUiQNBIMrV63909635 = -826795812;    long oUiQNBIMrV88272396 = -269397692;    long oUiQNBIMrV90174945 = -86073509;    long oUiQNBIMrV64621233 = -892580249;    long oUiQNBIMrV18425189 = -293735455;    long oUiQNBIMrV73443976 = -973693561;    long oUiQNBIMrV26546231 = -314173282;    long oUiQNBIMrV52461797 = -667625433;    long oUiQNBIMrV97919697 = -557045164;    long oUiQNBIMrV81511237 = -666089578;    long oUiQNBIMrV72792546 = -953631780;    long oUiQNBIMrV83726930 = -202926902;    long oUiQNBIMrV69710260 = 15797680;    long oUiQNBIMrV76191872 = -541140574;    long oUiQNBIMrV25803263 = -281160867;    long oUiQNBIMrV79065888 = -413602498;    long oUiQNBIMrV5993769 = -459326620;    long oUiQNBIMrV34846236 = -873027588;    long oUiQNBIMrV50367942 = -164493838;    long oUiQNBIMrV1071872 = -518286879;    long oUiQNBIMrV63852243 = 5665276;    long oUiQNBIMrV61171244 = -141979588;    long oUiQNBIMrV9348424 = -19528711;    long oUiQNBIMrV80296434 = -81308415;    long oUiQNBIMrV27061286 = -535720117;    long oUiQNBIMrV80360262 = -183588283;    long oUiQNBIMrV65852394 = -574792512;    long oUiQNBIMrV90851406 = -57375130;    long oUiQNBIMrV63414015 = -399788308;    long oUiQNBIMrV13950305 = -827703743;    long oUiQNBIMrV70652129 = -810043683;    long oUiQNBIMrV80127414 = -781712196;    long oUiQNBIMrV68256934 = -841201273;    long oUiQNBIMrV28484305 = -478212447;    long oUiQNBIMrV47706423 = -118904192;    long oUiQNBIMrV31259987 = -930727253;    long oUiQNBIMrV57362576 = -216816588;    long oUiQNBIMrV2449245 = -155450066;    long oUiQNBIMrV93608996 = -853711360;    long oUiQNBIMrV1697413 = -938149283;    long oUiQNBIMrV10551700 = -948089654;    long oUiQNBIMrV17815635 = -218723400;    long oUiQNBIMrV62648145 = 69194709;    long oUiQNBIMrV2885642 = -760896195;    long oUiQNBIMrV62763167 = -551719867;    long oUiQNBIMrV31215832 = -121376947;    long oUiQNBIMrV7644348 = -393472964;    long oUiQNBIMrV11569676 = -745077158;    long oUiQNBIMrV4308708 = -7854367;    long oUiQNBIMrV85909180 = -551233373;    long oUiQNBIMrV5936018 = -717247748;    long oUiQNBIMrV3585124 = -810526033;    long oUiQNBIMrV78886399 = -970039883;    long oUiQNBIMrV30788794 = -469421248;    long oUiQNBIMrV76392102 = -134586888;    long oUiQNBIMrV2738392 = -584816225;    long oUiQNBIMrV78923972 = -149868981;    long oUiQNBIMrV9878512 = 95234906;    long oUiQNBIMrV37559948 = -256860133;    long oUiQNBIMrV38064926 = -10147173;    long oUiQNBIMrV7591583 = -298901050;    long oUiQNBIMrV35694824 = -156798153;    long oUiQNBIMrV89047781 = -167837125;    long oUiQNBIMrV83969392 = -729341421;    long oUiQNBIMrV10859108 = -856045896;    long oUiQNBIMrV92665131 = -71919585;    long oUiQNBIMrV15469997 = -361725630;    long oUiQNBIMrV41225956 = -505989874;    long oUiQNBIMrV28485449 = -322236382;    long oUiQNBIMrV94543275 = -350433614;    long oUiQNBIMrV21703313 = -96785911;    long oUiQNBIMrV3544524 = -203876554;    long oUiQNBIMrV41237240 = 80683771;    long oUiQNBIMrV48670529 = -226344555;    long oUiQNBIMrV90520171 = -570197226;    long oUiQNBIMrV46036609 = 83754084;     oUiQNBIMrV9381756 = oUiQNBIMrV117600;     oUiQNBIMrV117600 = oUiQNBIMrV68728650;     oUiQNBIMrV68728650 = oUiQNBIMrV11616059;     oUiQNBIMrV11616059 = oUiQNBIMrV19532383;     oUiQNBIMrV19532383 = oUiQNBIMrV47537522;     oUiQNBIMrV47537522 = oUiQNBIMrV67070478;     oUiQNBIMrV67070478 = oUiQNBIMrV12034185;     oUiQNBIMrV12034185 = oUiQNBIMrV75141389;     oUiQNBIMrV75141389 = oUiQNBIMrV37097931;     oUiQNBIMrV37097931 = oUiQNBIMrV70277431;     oUiQNBIMrV70277431 = oUiQNBIMrV60567843;     oUiQNBIMrV60567843 = oUiQNBIMrV84396879;     oUiQNBIMrV84396879 = oUiQNBIMrV35555713;     oUiQNBIMrV35555713 = oUiQNBIMrV14942762;     oUiQNBIMrV14942762 = oUiQNBIMrV77354608;     oUiQNBIMrV77354608 = oUiQNBIMrV87761547;     oUiQNBIMrV87761547 = oUiQNBIMrV30111970;     oUiQNBIMrV30111970 = oUiQNBIMrV64975068;     oUiQNBIMrV64975068 = oUiQNBIMrV11929787;     oUiQNBIMrV11929787 = oUiQNBIMrV38431359;     oUiQNBIMrV38431359 = oUiQNBIMrV29254341;     oUiQNBIMrV29254341 = oUiQNBIMrV31860665;     oUiQNBIMrV31860665 = oUiQNBIMrV40244345;     oUiQNBIMrV40244345 = oUiQNBIMrV63909635;     oUiQNBIMrV63909635 = oUiQNBIMrV88272396;     oUiQNBIMrV88272396 = oUiQNBIMrV90174945;     oUiQNBIMrV90174945 = oUiQNBIMrV64621233;     oUiQNBIMrV64621233 = oUiQNBIMrV18425189;     oUiQNBIMrV18425189 = oUiQNBIMrV73443976;     oUiQNBIMrV73443976 = oUiQNBIMrV26546231;     oUiQNBIMrV26546231 = oUiQNBIMrV52461797;     oUiQNBIMrV52461797 = oUiQNBIMrV97919697;     oUiQNBIMrV97919697 = oUiQNBIMrV81511237;     oUiQNBIMrV81511237 = oUiQNBIMrV72792546;     oUiQNBIMrV72792546 = oUiQNBIMrV83726930;     oUiQNBIMrV83726930 = oUiQNBIMrV69710260;     oUiQNBIMrV69710260 = oUiQNBIMrV76191872;     oUiQNBIMrV76191872 = oUiQNBIMrV25803263;     oUiQNBIMrV25803263 = oUiQNBIMrV79065888;     oUiQNBIMrV79065888 = oUiQNBIMrV5993769;     oUiQNBIMrV5993769 = oUiQNBIMrV34846236;     oUiQNBIMrV34846236 = oUiQNBIMrV50367942;     oUiQNBIMrV50367942 = oUiQNBIMrV1071872;     oUiQNBIMrV1071872 = oUiQNBIMrV63852243;     oUiQNBIMrV63852243 = oUiQNBIMrV61171244;     oUiQNBIMrV61171244 = oUiQNBIMrV9348424;     oUiQNBIMrV9348424 = oUiQNBIMrV80296434;     oUiQNBIMrV80296434 = oUiQNBIMrV27061286;     oUiQNBIMrV27061286 = oUiQNBIMrV80360262;     oUiQNBIMrV80360262 = oUiQNBIMrV65852394;     oUiQNBIMrV65852394 = oUiQNBIMrV90851406;     oUiQNBIMrV90851406 = oUiQNBIMrV63414015;     oUiQNBIMrV63414015 = oUiQNBIMrV13950305;     oUiQNBIMrV13950305 = oUiQNBIMrV70652129;     oUiQNBIMrV70652129 = oUiQNBIMrV80127414;     oUiQNBIMrV80127414 = oUiQNBIMrV68256934;     oUiQNBIMrV68256934 = oUiQNBIMrV28484305;     oUiQNBIMrV28484305 = oUiQNBIMrV47706423;     oUiQNBIMrV47706423 = oUiQNBIMrV31259987;     oUiQNBIMrV31259987 = oUiQNBIMrV57362576;     oUiQNBIMrV57362576 = oUiQNBIMrV2449245;     oUiQNBIMrV2449245 = oUiQNBIMrV93608996;     oUiQNBIMrV93608996 = oUiQNBIMrV1697413;     oUiQNBIMrV1697413 = oUiQNBIMrV10551700;     oUiQNBIMrV10551700 = oUiQNBIMrV17815635;     oUiQNBIMrV17815635 = oUiQNBIMrV62648145;     oUiQNBIMrV62648145 = oUiQNBIMrV2885642;     oUiQNBIMrV2885642 = oUiQNBIMrV62763167;     oUiQNBIMrV62763167 = oUiQNBIMrV31215832;     oUiQNBIMrV31215832 = oUiQNBIMrV7644348;     oUiQNBIMrV7644348 = oUiQNBIMrV11569676;     oUiQNBIMrV11569676 = oUiQNBIMrV4308708;     oUiQNBIMrV4308708 = oUiQNBIMrV85909180;     oUiQNBIMrV85909180 = oUiQNBIMrV5936018;     oUiQNBIMrV5936018 = oUiQNBIMrV3585124;     oUiQNBIMrV3585124 = oUiQNBIMrV78886399;     oUiQNBIMrV78886399 = oUiQNBIMrV30788794;     oUiQNBIMrV30788794 = oUiQNBIMrV76392102;     oUiQNBIMrV76392102 = oUiQNBIMrV2738392;     oUiQNBIMrV2738392 = oUiQNBIMrV78923972;     oUiQNBIMrV78923972 = oUiQNBIMrV9878512;     oUiQNBIMrV9878512 = oUiQNBIMrV37559948;     oUiQNBIMrV37559948 = oUiQNBIMrV38064926;     oUiQNBIMrV38064926 = oUiQNBIMrV7591583;     oUiQNBIMrV7591583 = oUiQNBIMrV35694824;     oUiQNBIMrV35694824 = oUiQNBIMrV89047781;     oUiQNBIMrV89047781 = oUiQNBIMrV83969392;     oUiQNBIMrV83969392 = oUiQNBIMrV10859108;     oUiQNBIMrV10859108 = oUiQNBIMrV92665131;     oUiQNBIMrV92665131 = oUiQNBIMrV15469997;     oUiQNBIMrV15469997 = oUiQNBIMrV41225956;     oUiQNBIMrV41225956 = oUiQNBIMrV28485449;     oUiQNBIMrV28485449 = oUiQNBIMrV94543275;     oUiQNBIMrV94543275 = oUiQNBIMrV21703313;     oUiQNBIMrV21703313 = oUiQNBIMrV3544524;     oUiQNBIMrV3544524 = oUiQNBIMrV41237240;     oUiQNBIMrV41237240 = oUiQNBIMrV48670529;     oUiQNBIMrV48670529 = oUiQNBIMrV90520171;     oUiQNBIMrV90520171 = oUiQNBIMrV46036609;     oUiQNBIMrV46036609 = oUiQNBIMrV9381756;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DlJdnQDpYq45728463() {     long ptqMXIHuYY31768873 = -650649879;    long ptqMXIHuYY84465099 = -877452453;    long ptqMXIHuYY97497928 = 53212098;    long ptqMXIHuYY26622747 = 47892464;    long ptqMXIHuYY17706638 = -791181946;    long ptqMXIHuYY62420639 = -263498474;    long ptqMXIHuYY59226217 = -970017621;    long ptqMXIHuYY87668043 = -618043459;    long ptqMXIHuYY72304520 = -796038883;    long ptqMXIHuYY28861516 = -5004268;    long ptqMXIHuYY7819291 = -982973233;    long ptqMXIHuYY16470792 = -838595748;    long ptqMXIHuYY51405060 = 54296908;    long ptqMXIHuYY78170464 = 66939596;    long ptqMXIHuYY51678039 = -51781562;    long ptqMXIHuYY21610521 = -859342826;    long ptqMXIHuYY37584900 = -563497551;    long ptqMXIHuYY36088387 = -840757924;    long ptqMXIHuYY6256700 = -99937996;    long ptqMXIHuYY90446069 = -429208821;    long ptqMXIHuYY39314466 = -675040928;    long ptqMXIHuYY93069559 = -639300034;    long ptqMXIHuYY35153988 = -163788718;    long ptqMXIHuYY66744132 = -792914469;    long ptqMXIHuYY99449882 = -448354937;    long ptqMXIHuYY76575391 = -713291916;    long ptqMXIHuYY37264366 = -943660002;    long ptqMXIHuYY70532683 = -681034703;    long ptqMXIHuYY25317749 = -637992575;    long ptqMXIHuYY82601156 = -893989428;    long ptqMXIHuYY70837036 = -395489434;    long ptqMXIHuYY10002988 = -219696787;    long ptqMXIHuYY78958368 = 13611341;    long ptqMXIHuYY40899925 = -990216428;    long ptqMXIHuYY49021910 = 82611797;    long ptqMXIHuYY25419800 = -518322279;    long ptqMXIHuYY63149973 = -817150271;    long ptqMXIHuYY29817973 = -408766046;    long ptqMXIHuYY74111393 = -682206899;    long ptqMXIHuYY99251603 = -549826527;    long ptqMXIHuYY34751139 = -962504286;    long ptqMXIHuYY23354627 = -630936279;    long ptqMXIHuYY55918134 = -52024422;    long ptqMXIHuYY515992 = -515650528;    long ptqMXIHuYY31835510 = -903478202;    long ptqMXIHuYY69271526 = -324208683;    long ptqMXIHuYY21645822 = -597938964;    long ptqMXIHuYY24895109 = -937843406;    long ptqMXIHuYY21521191 = -485436815;    long ptqMXIHuYY77444007 = -325608542;    long ptqMXIHuYY74721610 = -929868789;    long ptqMXIHuYY64702664 = -488194412;    long ptqMXIHuYY41728418 = -71699539;    long ptqMXIHuYY74236850 = -3384552;    long ptqMXIHuYY6413997 = -950747798;    long ptqMXIHuYY38699314 = 88650154;    long ptqMXIHuYY49311111 = -613663736;    long ptqMXIHuYY30753796 = -153873433;    long ptqMXIHuYY27172865 = -503752599;    long ptqMXIHuYY41131246 = 22109969;    long ptqMXIHuYY25156273 = -319838473;    long ptqMXIHuYY88693533 = -188982919;    long ptqMXIHuYY62350294 = -980050885;    long ptqMXIHuYY89703364 = -902049456;    long ptqMXIHuYY58024479 = -609514835;    long ptqMXIHuYY97816303 = -663276446;    long ptqMXIHuYY37512424 = -752207090;    long ptqMXIHuYY10505136 = 44513335;    long ptqMXIHuYY29148555 = 84327798;    long ptqMXIHuYY26258240 = -533459284;    long ptqMXIHuYY58460548 = 57807445;    long ptqMXIHuYY7766927 = -54731506;    long ptqMXIHuYY61976993 = -58551026;    long ptqMXIHuYY7005097 = -550111469;    long ptqMXIHuYY55694930 = -466704536;    long ptqMXIHuYY15959840 = 55895350;    long ptqMXIHuYY37151426 = -487275613;    long ptqMXIHuYY34637997 = -648138191;    long ptqMXIHuYY34908623 = -889436268;    long ptqMXIHuYY30178356 = -24146254;    long ptqMXIHuYY54929570 = -15352952;    long ptqMXIHuYY12369257 = 94183404;    long ptqMXIHuYY49011493 = -95597888;    long ptqMXIHuYY47873741 = -212384033;    long ptqMXIHuYY7879546 = -964120639;    long ptqMXIHuYY6134373 = -907295023;    long ptqMXIHuYY68274569 = -47997249;    long ptqMXIHuYY4721518 = -983004107;    long ptqMXIHuYY34485928 = 60531370;    long ptqMXIHuYY10322597 = 93961642;    long ptqMXIHuYY76108688 = -904658543;    long ptqMXIHuYY32396177 = -563276839;    long ptqMXIHuYY2645109 = -905013447;    long ptqMXIHuYY32980148 = -604316868;    long ptqMXIHuYY74095330 = -129988055;    long ptqMXIHuYY46057606 = -673521367;    long ptqMXIHuYY61004333 = -650885394;    long ptqMXIHuYY66214770 = -149974966;    long ptqMXIHuYY42491512 = -906135694;    long ptqMXIHuYY34019207 = -650649879;     ptqMXIHuYY31768873 = ptqMXIHuYY84465099;     ptqMXIHuYY84465099 = ptqMXIHuYY97497928;     ptqMXIHuYY97497928 = ptqMXIHuYY26622747;     ptqMXIHuYY26622747 = ptqMXIHuYY17706638;     ptqMXIHuYY17706638 = ptqMXIHuYY62420639;     ptqMXIHuYY62420639 = ptqMXIHuYY59226217;     ptqMXIHuYY59226217 = ptqMXIHuYY87668043;     ptqMXIHuYY87668043 = ptqMXIHuYY72304520;     ptqMXIHuYY72304520 = ptqMXIHuYY28861516;     ptqMXIHuYY28861516 = ptqMXIHuYY7819291;     ptqMXIHuYY7819291 = ptqMXIHuYY16470792;     ptqMXIHuYY16470792 = ptqMXIHuYY51405060;     ptqMXIHuYY51405060 = ptqMXIHuYY78170464;     ptqMXIHuYY78170464 = ptqMXIHuYY51678039;     ptqMXIHuYY51678039 = ptqMXIHuYY21610521;     ptqMXIHuYY21610521 = ptqMXIHuYY37584900;     ptqMXIHuYY37584900 = ptqMXIHuYY36088387;     ptqMXIHuYY36088387 = ptqMXIHuYY6256700;     ptqMXIHuYY6256700 = ptqMXIHuYY90446069;     ptqMXIHuYY90446069 = ptqMXIHuYY39314466;     ptqMXIHuYY39314466 = ptqMXIHuYY93069559;     ptqMXIHuYY93069559 = ptqMXIHuYY35153988;     ptqMXIHuYY35153988 = ptqMXIHuYY66744132;     ptqMXIHuYY66744132 = ptqMXIHuYY99449882;     ptqMXIHuYY99449882 = ptqMXIHuYY76575391;     ptqMXIHuYY76575391 = ptqMXIHuYY37264366;     ptqMXIHuYY37264366 = ptqMXIHuYY70532683;     ptqMXIHuYY70532683 = ptqMXIHuYY25317749;     ptqMXIHuYY25317749 = ptqMXIHuYY82601156;     ptqMXIHuYY82601156 = ptqMXIHuYY70837036;     ptqMXIHuYY70837036 = ptqMXIHuYY10002988;     ptqMXIHuYY10002988 = ptqMXIHuYY78958368;     ptqMXIHuYY78958368 = ptqMXIHuYY40899925;     ptqMXIHuYY40899925 = ptqMXIHuYY49021910;     ptqMXIHuYY49021910 = ptqMXIHuYY25419800;     ptqMXIHuYY25419800 = ptqMXIHuYY63149973;     ptqMXIHuYY63149973 = ptqMXIHuYY29817973;     ptqMXIHuYY29817973 = ptqMXIHuYY74111393;     ptqMXIHuYY74111393 = ptqMXIHuYY99251603;     ptqMXIHuYY99251603 = ptqMXIHuYY34751139;     ptqMXIHuYY34751139 = ptqMXIHuYY23354627;     ptqMXIHuYY23354627 = ptqMXIHuYY55918134;     ptqMXIHuYY55918134 = ptqMXIHuYY515992;     ptqMXIHuYY515992 = ptqMXIHuYY31835510;     ptqMXIHuYY31835510 = ptqMXIHuYY69271526;     ptqMXIHuYY69271526 = ptqMXIHuYY21645822;     ptqMXIHuYY21645822 = ptqMXIHuYY24895109;     ptqMXIHuYY24895109 = ptqMXIHuYY21521191;     ptqMXIHuYY21521191 = ptqMXIHuYY77444007;     ptqMXIHuYY77444007 = ptqMXIHuYY74721610;     ptqMXIHuYY74721610 = ptqMXIHuYY64702664;     ptqMXIHuYY64702664 = ptqMXIHuYY41728418;     ptqMXIHuYY41728418 = ptqMXIHuYY74236850;     ptqMXIHuYY74236850 = ptqMXIHuYY6413997;     ptqMXIHuYY6413997 = ptqMXIHuYY38699314;     ptqMXIHuYY38699314 = ptqMXIHuYY49311111;     ptqMXIHuYY49311111 = ptqMXIHuYY30753796;     ptqMXIHuYY30753796 = ptqMXIHuYY27172865;     ptqMXIHuYY27172865 = ptqMXIHuYY41131246;     ptqMXIHuYY41131246 = ptqMXIHuYY25156273;     ptqMXIHuYY25156273 = ptqMXIHuYY88693533;     ptqMXIHuYY88693533 = ptqMXIHuYY62350294;     ptqMXIHuYY62350294 = ptqMXIHuYY89703364;     ptqMXIHuYY89703364 = ptqMXIHuYY58024479;     ptqMXIHuYY58024479 = ptqMXIHuYY97816303;     ptqMXIHuYY97816303 = ptqMXIHuYY37512424;     ptqMXIHuYY37512424 = ptqMXIHuYY10505136;     ptqMXIHuYY10505136 = ptqMXIHuYY29148555;     ptqMXIHuYY29148555 = ptqMXIHuYY26258240;     ptqMXIHuYY26258240 = ptqMXIHuYY58460548;     ptqMXIHuYY58460548 = ptqMXIHuYY7766927;     ptqMXIHuYY7766927 = ptqMXIHuYY61976993;     ptqMXIHuYY61976993 = ptqMXIHuYY7005097;     ptqMXIHuYY7005097 = ptqMXIHuYY55694930;     ptqMXIHuYY55694930 = ptqMXIHuYY15959840;     ptqMXIHuYY15959840 = ptqMXIHuYY37151426;     ptqMXIHuYY37151426 = ptqMXIHuYY34637997;     ptqMXIHuYY34637997 = ptqMXIHuYY34908623;     ptqMXIHuYY34908623 = ptqMXIHuYY30178356;     ptqMXIHuYY30178356 = ptqMXIHuYY54929570;     ptqMXIHuYY54929570 = ptqMXIHuYY12369257;     ptqMXIHuYY12369257 = ptqMXIHuYY49011493;     ptqMXIHuYY49011493 = ptqMXIHuYY47873741;     ptqMXIHuYY47873741 = ptqMXIHuYY7879546;     ptqMXIHuYY7879546 = ptqMXIHuYY6134373;     ptqMXIHuYY6134373 = ptqMXIHuYY68274569;     ptqMXIHuYY68274569 = ptqMXIHuYY4721518;     ptqMXIHuYY4721518 = ptqMXIHuYY34485928;     ptqMXIHuYY34485928 = ptqMXIHuYY10322597;     ptqMXIHuYY10322597 = ptqMXIHuYY76108688;     ptqMXIHuYY76108688 = ptqMXIHuYY32396177;     ptqMXIHuYY32396177 = ptqMXIHuYY2645109;     ptqMXIHuYY2645109 = ptqMXIHuYY32980148;     ptqMXIHuYY32980148 = ptqMXIHuYY74095330;     ptqMXIHuYY74095330 = ptqMXIHuYY46057606;     ptqMXIHuYY46057606 = ptqMXIHuYY61004333;     ptqMXIHuYY61004333 = ptqMXIHuYY66214770;     ptqMXIHuYY66214770 = ptqMXIHuYY42491512;     ptqMXIHuYY42491512 = ptqMXIHuYY34019207;     ptqMXIHuYY34019207 = ptqMXIHuYY31768873;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mNLpByAQAJ12689865() {     int uepphavhNI21888710 = -581930222;    int uepphavhNI6162481 = -194973749;    int uepphavhNI94303839 = -40710977;    int uepphavhNI85201182 = 41179812;    int uepphavhNI39603475 = -100860709;    int uepphavhNI78012475 = -955564394;    int uepphavhNI22436991 = -312099561;    int uepphavhNI9760657 = 41331485;    int uepphavhNI78856371 = -517577591;    int uepphavhNI29756699 = -311685665;    int uepphavhNI4291716 = -350865462;    int uepphavhNI65511976 = -53662246;    int uepphavhNI21604106 = -279597427;    int uepphavhNI70433538 = -276374340;    int uepphavhNI94924519 = -499615357;    int uepphavhNI20354811 = -892518346;    int uepphavhNI37399840 = -644457362;    int uepphavhNI80444632 = -580678838;    int uepphavhNI87599360 = -84330698;    int uepphavhNI29844081 = -589111582;    int uepphavhNI6906293 = -89932394;    int uepphavhNI69447407 = -382388553;    int uepphavhNI5270804 = 70793519;    int uepphavhNI18315338 = -21859367;    int uepphavhNI74777759 = -418559734;    int uepphavhNI11940433 = -497371579;    int uepphavhNI29453283 = 96011291;    int uepphavhNI29106584 = -249891749;    int uepphavhNI99205193 = -946261937;    int uepphavhNI6480106 = -443823193;    int uepphavhNI7713118 = -9249212;    int uepphavhNI94093759 = -12342968;    int uepphavhNI73379832 = -488558034;    int uepphavhNI65021406 = -858349318;    int uepphavhNI66976483 = -927037788;    int uepphavhNI21478996 = -901117435;    int uepphavhNI75324909 = -432619554;    int uepphavhNI24092936 = -270087969;    int uepphavhNI19958008 = -2350361;    int uepphavhNI15636638 = -63965986;    int uepphavhNI36306480 = -756309460;    int uepphavhNI63696751 = -639221574;    int uepphavhNI76018335 = 13419729;    int uepphavhNI90409830 = 63301840;    int uepphavhNI7817980 = -389247560;    int uepphavhNI30138488 = -619877260;    int uepphavhNI29766904 = -627702086;    int uepphavhNI90665148 = -473261016;    int uepphavhNI68098234 = -485140023;    int uepphavhNI26769837 = -369629766;    int uepphavhNI17346504 = -882805841;    int uepphavhNI23023029 = -834766993;    int uepphavhNI66629221 = -775606542;    int uepphavhNI37394183 = -658859685;    int uepphavhNI5783573 = -836247347;    int uepphavhNI52441303 = -99541670;    int uepphavhNI891678 = -165767268;    int uepphavhNI75988501 = 81148390;    int uepphavhNI10423423 = -540260454;    int uepphavhNI27663042 = -603489131;    int uepphavhNI48559193 = -951575686;    int uepphavhNI93330407 = 37792188;    int uepphavhNI10555463 = -12406578;    int uepphavhNI72376265 = 26245602;    int uepphavhNI22043582 = -202436453;    int uepphavhNI10197957 = -238522494;    int uepphavhNI92132144 = -565104212;    int uepphavhNI56582699 = -421248109;    int uepphavhNI3457056 = -349336552;    int uepphavhNI73445524 = -598497922;    int uepphavhNI45029901 = -359898793;    int uepphavhNI13306904 = -274369394;    int uepphavhNI60486625 = -478328478;    int uepphavhNI71962723 = 79635288;    int uepphavhNI93537600 = -832802123;    int uepphavhNI43209542 = -450710821;    int uepphavhNI93429072 = -295808282;    int uepphavhNI14860973 = -992508322;    int uepphavhNI10497359 = -632611808;    int uepphavhNI44639272 = -798682475;    int uepphavhNI82173529 = -869669493;    int uepphavhNI38788134 = -430727694;    int uepphavhNI61008349 = -764751727;    int uepphavhNI72435357 = -476632172;    int uepphavhNI89133601 = -561017352;    int uepphavhNI84690089 = -174482220;    int uepphavhNI27464538 = -236736427;    int uepphavhNI35985649 = -829698350;    int uepphavhNI59237834 = 77898028;    int uepphavhNI14535180 = -727496119;    int uepphavhNI20587318 = -635350167;    int uepphavhNI99336407 = -413767944;    int uepphavhNI13669514 = -729827515;    int uepphavhNI92294965 = -398861230;    int uepphavhNI67077444 = -112390301;    int uepphavhNI42976073 = -694101648;    int uepphavhNI53141288 = -526814997;    int uepphavhNI3642070 = 87174127;    int uepphavhNI68366248 = -734261708;    int uepphavhNI97620023 = -581930222;     uepphavhNI21888710 = uepphavhNI6162481;     uepphavhNI6162481 = uepphavhNI94303839;     uepphavhNI94303839 = uepphavhNI85201182;     uepphavhNI85201182 = uepphavhNI39603475;     uepphavhNI39603475 = uepphavhNI78012475;     uepphavhNI78012475 = uepphavhNI22436991;     uepphavhNI22436991 = uepphavhNI9760657;     uepphavhNI9760657 = uepphavhNI78856371;     uepphavhNI78856371 = uepphavhNI29756699;     uepphavhNI29756699 = uepphavhNI4291716;     uepphavhNI4291716 = uepphavhNI65511976;     uepphavhNI65511976 = uepphavhNI21604106;     uepphavhNI21604106 = uepphavhNI70433538;     uepphavhNI70433538 = uepphavhNI94924519;     uepphavhNI94924519 = uepphavhNI20354811;     uepphavhNI20354811 = uepphavhNI37399840;     uepphavhNI37399840 = uepphavhNI80444632;     uepphavhNI80444632 = uepphavhNI87599360;     uepphavhNI87599360 = uepphavhNI29844081;     uepphavhNI29844081 = uepphavhNI6906293;     uepphavhNI6906293 = uepphavhNI69447407;     uepphavhNI69447407 = uepphavhNI5270804;     uepphavhNI5270804 = uepphavhNI18315338;     uepphavhNI18315338 = uepphavhNI74777759;     uepphavhNI74777759 = uepphavhNI11940433;     uepphavhNI11940433 = uepphavhNI29453283;     uepphavhNI29453283 = uepphavhNI29106584;     uepphavhNI29106584 = uepphavhNI99205193;     uepphavhNI99205193 = uepphavhNI6480106;     uepphavhNI6480106 = uepphavhNI7713118;     uepphavhNI7713118 = uepphavhNI94093759;     uepphavhNI94093759 = uepphavhNI73379832;     uepphavhNI73379832 = uepphavhNI65021406;     uepphavhNI65021406 = uepphavhNI66976483;     uepphavhNI66976483 = uepphavhNI21478996;     uepphavhNI21478996 = uepphavhNI75324909;     uepphavhNI75324909 = uepphavhNI24092936;     uepphavhNI24092936 = uepphavhNI19958008;     uepphavhNI19958008 = uepphavhNI15636638;     uepphavhNI15636638 = uepphavhNI36306480;     uepphavhNI36306480 = uepphavhNI63696751;     uepphavhNI63696751 = uepphavhNI76018335;     uepphavhNI76018335 = uepphavhNI90409830;     uepphavhNI90409830 = uepphavhNI7817980;     uepphavhNI7817980 = uepphavhNI30138488;     uepphavhNI30138488 = uepphavhNI29766904;     uepphavhNI29766904 = uepphavhNI90665148;     uepphavhNI90665148 = uepphavhNI68098234;     uepphavhNI68098234 = uepphavhNI26769837;     uepphavhNI26769837 = uepphavhNI17346504;     uepphavhNI17346504 = uepphavhNI23023029;     uepphavhNI23023029 = uepphavhNI66629221;     uepphavhNI66629221 = uepphavhNI37394183;     uepphavhNI37394183 = uepphavhNI5783573;     uepphavhNI5783573 = uepphavhNI52441303;     uepphavhNI52441303 = uepphavhNI891678;     uepphavhNI891678 = uepphavhNI75988501;     uepphavhNI75988501 = uepphavhNI10423423;     uepphavhNI10423423 = uepphavhNI27663042;     uepphavhNI27663042 = uepphavhNI48559193;     uepphavhNI48559193 = uepphavhNI93330407;     uepphavhNI93330407 = uepphavhNI10555463;     uepphavhNI10555463 = uepphavhNI72376265;     uepphavhNI72376265 = uepphavhNI22043582;     uepphavhNI22043582 = uepphavhNI10197957;     uepphavhNI10197957 = uepphavhNI92132144;     uepphavhNI92132144 = uepphavhNI56582699;     uepphavhNI56582699 = uepphavhNI3457056;     uepphavhNI3457056 = uepphavhNI73445524;     uepphavhNI73445524 = uepphavhNI45029901;     uepphavhNI45029901 = uepphavhNI13306904;     uepphavhNI13306904 = uepphavhNI60486625;     uepphavhNI60486625 = uepphavhNI71962723;     uepphavhNI71962723 = uepphavhNI93537600;     uepphavhNI93537600 = uepphavhNI43209542;     uepphavhNI43209542 = uepphavhNI93429072;     uepphavhNI93429072 = uepphavhNI14860973;     uepphavhNI14860973 = uepphavhNI10497359;     uepphavhNI10497359 = uepphavhNI44639272;     uepphavhNI44639272 = uepphavhNI82173529;     uepphavhNI82173529 = uepphavhNI38788134;     uepphavhNI38788134 = uepphavhNI61008349;     uepphavhNI61008349 = uepphavhNI72435357;     uepphavhNI72435357 = uepphavhNI89133601;     uepphavhNI89133601 = uepphavhNI84690089;     uepphavhNI84690089 = uepphavhNI27464538;     uepphavhNI27464538 = uepphavhNI35985649;     uepphavhNI35985649 = uepphavhNI59237834;     uepphavhNI59237834 = uepphavhNI14535180;     uepphavhNI14535180 = uepphavhNI20587318;     uepphavhNI20587318 = uepphavhNI99336407;     uepphavhNI99336407 = uepphavhNI13669514;     uepphavhNI13669514 = uepphavhNI92294965;     uepphavhNI92294965 = uepphavhNI67077444;     uepphavhNI67077444 = uepphavhNI42976073;     uepphavhNI42976073 = uepphavhNI53141288;     uepphavhNI53141288 = uepphavhNI3642070;     uepphavhNI3642070 = uepphavhNI68366248;     uepphavhNI68366248 = uepphavhNI97620023;     uepphavhNI97620023 = uepphavhNI21888710;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void iiDrikEyuB65366671() {     int FAIzQXvgOq58808770 = -965712462;    int FAIzQXvgOq50453298 = -316209103;    int FAIzQXvgOq62328157 = -286108513;    int FAIzQXvgOq9334342 = -194752686;    int FAIzQXvgOq92542587 = -621763017;    int FAIzQXvgOq179511 = -178115891;    int FAIzQXvgOq14619494 = -745031716;    int FAIzQXvgOq59780518 = -965252665;    int FAIzQXvgOq27927103 = -930499810;    int FAIzQXvgOq2416964 = -760748588;    int FAIzQXvgOq29052597 = 55177776;    int FAIzQXvgOq38711374 = -659634721;    int FAIzQXvgOq40193784 = -98375335;    int FAIzQXvgOq2212703 = -709161002;    int FAIzQXvgOq79930416 = -677059120;    int FAIzQXvgOq49275106 = -129368845;    int FAIzQXvgOq1203205 = -719280841;    int FAIzQXvgOq90411523 = -80332988;    int FAIzQXvgOq28865999 = -125347236;    int FAIzQXvgOq54615349 = -630215534;    int FAIzQXvgOq94289317 = -612227981;    int FAIzQXvgOq52711433 = -63008426;    int FAIzQXvgOq84136742 = -766352192;    int FAIzQXvgOq55584815 = -476178274;    int FAIzQXvgOq12853418 = -824500977;    int FAIzQXvgOq35262852 = -587946407;    int FAIzQXvgOq78899105 = -231290702;    int FAIzQXvgOq38544015 = -79782764;    int FAIzQXvgOq25277599 = -149012148;    int FAIzQXvgOq80216212 = 76098894;    int FAIzQXvgOq75186539 = -572451420;    int FAIzQXvgOq94716753 = -831220111;    int FAIzQXvgOq99016020 = -847735822;    int FAIzQXvgOq95015213 = -688634520;    int FAIzQXvgOq30344941 = -263836366;    int FAIzQXvgOq65146258 = -276654688;    int FAIzQXvgOq9279515 = -150438188;    int FAIzQXvgOq63900733 = -149136208;    int FAIzQXvgOq32017573 = -818371886;    int FAIzQXvgOq36391936 = 28282923;    int FAIzQXvgOq16431828 = -225218085;    int FAIzQXvgOq44962092 = 55174923;    int FAIzQXvgOq42539349 = -469842887;    int FAIzQXvgOq48762484 = -975934954;    int FAIzQXvgOq30807353 = -688454128;    int FAIzQXvgOq42812402 = -253238561;    int FAIzQXvgOq7341379 = -773726756;    int FAIzQXvgOq36766988 = -549221108;    int FAIzQXvgOq87988459 = -500824667;    int FAIzQXvgOq74380771 = -604404317;    int FAIzQXvgOq12019209 = -836799320;    int FAIzQXvgOq81572885 = -704616114;    int FAIzQXvgOq95632911 = -831539728;    int FAIzQXvgOq44501720 = -32931808;    int FAIzQXvgOq71077354 = -326585414;    int FAIzQXvgOq6097337 = -802704036;    int FAIzQXvgOq66316556 = -549856912;    int FAIzQXvgOq6743342 = -809930240;    int FAIzQXvgOq96480924 = -370251710;    int FAIzQXvgOq57279735 = 66183389;    int FAIzQXvgOq21280405 = -946825190;    int FAIzQXvgOq76075478 = -565248953;    int FAIzQXvgOq34502919 = -716240518;    int FAIzQXvgOq47710891 = -906598705;    int FAIzQXvgOq27230424 = -88297169;    int FAIzQXvgOq34335844 = -113602114;    int FAIzQXvgOq39695353 = -811898900;    int FAIzQXvgOq45178570 = -409740815;    int FAIzQXvgOq71867761 = -345324636;    int FAIzQXvgOq14784159 = -300404432;    int FAIzQXvgOq39995591 = -978930658;    int FAIzQXvgOq37302472 = -470144634;    int FAIzQXvgOq58393950 = -261961102;    int FAIzQXvgOq92474062 = -53630159;    int FAIzQXvgOq38183521 = -304997450;    int FAIzQXvgOq49327225 = -567402905;    int FAIzQXvgOq10172085 = -593165540;    int FAIzQXvgOq35374258 = -790417239;    int FAIzQXvgOq24777463 = -787724146;    int FAIzQXvgOq70041016 = -471262416;    int FAIzQXvgOq27921474 = -814219651;    int FAIzQXvgOq42132118 = -682069594;    int FAIzQXvgOq50555555 = -578958097;    int FAIzQXvgOq50896827 = -544607831;    int FAIzQXvgOq68197003 = -87101787;    int FAIzQXvgOq93613654 = -867835307;    int FAIzQXvgOq99083841 = -999680383;    int FAIzQXvgOq54514300 = -714804014;    int FAIzQXvgOq23937860 = -262049107;    int FAIzQXvgOq24247605 = -461132330;    int FAIzQXvgOq98829701 = -726797777;    int FAIzQXvgOq2536174 = -340507948;    int FAIzQXvgOq67419808 = -778884499;    int FAIzQXvgOq74737837 = -784555275;    int FAIzQXvgOq15111531 = -24891888;    int FAIzQXvgOq40356349 = -659969132;    int FAIzQXvgOq10459173 = -228584560;    int FAIzQXvgOq94828457 = -563244182;    int FAIzQXvgOq21532060 = -787637785;    int FAIzQXvgOq96471509 = -965712462;     FAIzQXvgOq58808770 = FAIzQXvgOq50453298;     FAIzQXvgOq50453298 = FAIzQXvgOq62328157;     FAIzQXvgOq62328157 = FAIzQXvgOq9334342;     FAIzQXvgOq9334342 = FAIzQXvgOq92542587;     FAIzQXvgOq92542587 = FAIzQXvgOq179511;     FAIzQXvgOq179511 = FAIzQXvgOq14619494;     FAIzQXvgOq14619494 = FAIzQXvgOq59780518;     FAIzQXvgOq59780518 = FAIzQXvgOq27927103;     FAIzQXvgOq27927103 = FAIzQXvgOq2416964;     FAIzQXvgOq2416964 = FAIzQXvgOq29052597;     FAIzQXvgOq29052597 = FAIzQXvgOq38711374;     FAIzQXvgOq38711374 = FAIzQXvgOq40193784;     FAIzQXvgOq40193784 = FAIzQXvgOq2212703;     FAIzQXvgOq2212703 = FAIzQXvgOq79930416;     FAIzQXvgOq79930416 = FAIzQXvgOq49275106;     FAIzQXvgOq49275106 = FAIzQXvgOq1203205;     FAIzQXvgOq1203205 = FAIzQXvgOq90411523;     FAIzQXvgOq90411523 = FAIzQXvgOq28865999;     FAIzQXvgOq28865999 = FAIzQXvgOq54615349;     FAIzQXvgOq54615349 = FAIzQXvgOq94289317;     FAIzQXvgOq94289317 = FAIzQXvgOq52711433;     FAIzQXvgOq52711433 = FAIzQXvgOq84136742;     FAIzQXvgOq84136742 = FAIzQXvgOq55584815;     FAIzQXvgOq55584815 = FAIzQXvgOq12853418;     FAIzQXvgOq12853418 = FAIzQXvgOq35262852;     FAIzQXvgOq35262852 = FAIzQXvgOq78899105;     FAIzQXvgOq78899105 = FAIzQXvgOq38544015;     FAIzQXvgOq38544015 = FAIzQXvgOq25277599;     FAIzQXvgOq25277599 = FAIzQXvgOq80216212;     FAIzQXvgOq80216212 = FAIzQXvgOq75186539;     FAIzQXvgOq75186539 = FAIzQXvgOq94716753;     FAIzQXvgOq94716753 = FAIzQXvgOq99016020;     FAIzQXvgOq99016020 = FAIzQXvgOq95015213;     FAIzQXvgOq95015213 = FAIzQXvgOq30344941;     FAIzQXvgOq30344941 = FAIzQXvgOq65146258;     FAIzQXvgOq65146258 = FAIzQXvgOq9279515;     FAIzQXvgOq9279515 = FAIzQXvgOq63900733;     FAIzQXvgOq63900733 = FAIzQXvgOq32017573;     FAIzQXvgOq32017573 = FAIzQXvgOq36391936;     FAIzQXvgOq36391936 = FAIzQXvgOq16431828;     FAIzQXvgOq16431828 = FAIzQXvgOq44962092;     FAIzQXvgOq44962092 = FAIzQXvgOq42539349;     FAIzQXvgOq42539349 = FAIzQXvgOq48762484;     FAIzQXvgOq48762484 = FAIzQXvgOq30807353;     FAIzQXvgOq30807353 = FAIzQXvgOq42812402;     FAIzQXvgOq42812402 = FAIzQXvgOq7341379;     FAIzQXvgOq7341379 = FAIzQXvgOq36766988;     FAIzQXvgOq36766988 = FAIzQXvgOq87988459;     FAIzQXvgOq87988459 = FAIzQXvgOq74380771;     FAIzQXvgOq74380771 = FAIzQXvgOq12019209;     FAIzQXvgOq12019209 = FAIzQXvgOq81572885;     FAIzQXvgOq81572885 = FAIzQXvgOq95632911;     FAIzQXvgOq95632911 = FAIzQXvgOq44501720;     FAIzQXvgOq44501720 = FAIzQXvgOq71077354;     FAIzQXvgOq71077354 = FAIzQXvgOq6097337;     FAIzQXvgOq6097337 = FAIzQXvgOq66316556;     FAIzQXvgOq66316556 = FAIzQXvgOq6743342;     FAIzQXvgOq6743342 = FAIzQXvgOq96480924;     FAIzQXvgOq96480924 = FAIzQXvgOq57279735;     FAIzQXvgOq57279735 = FAIzQXvgOq21280405;     FAIzQXvgOq21280405 = FAIzQXvgOq76075478;     FAIzQXvgOq76075478 = FAIzQXvgOq34502919;     FAIzQXvgOq34502919 = FAIzQXvgOq47710891;     FAIzQXvgOq47710891 = FAIzQXvgOq27230424;     FAIzQXvgOq27230424 = FAIzQXvgOq34335844;     FAIzQXvgOq34335844 = FAIzQXvgOq39695353;     FAIzQXvgOq39695353 = FAIzQXvgOq45178570;     FAIzQXvgOq45178570 = FAIzQXvgOq71867761;     FAIzQXvgOq71867761 = FAIzQXvgOq14784159;     FAIzQXvgOq14784159 = FAIzQXvgOq39995591;     FAIzQXvgOq39995591 = FAIzQXvgOq37302472;     FAIzQXvgOq37302472 = FAIzQXvgOq58393950;     FAIzQXvgOq58393950 = FAIzQXvgOq92474062;     FAIzQXvgOq92474062 = FAIzQXvgOq38183521;     FAIzQXvgOq38183521 = FAIzQXvgOq49327225;     FAIzQXvgOq49327225 = FAIzQXvgOq10172085;     FAIzQXvgOq10172085 = FAIzQXvgOq35374258;     FAIzQXvgOq35374258 = FAIzQXvgOq24777463;     FAIzQXvgOq24777463 = FAIzQXvgOq70041016;     FAIzQXvgOq70041016 = FAIzQXvgOq27921474;     FAIzQXvgOq27921474 = FAIzQXvgOq42132118;     FAIzQXvgOq42132118 = FAIzQXvgOq50555555;     FAIzQXvgOq50555555 = FAIzQXvgOq50896827;     FAIzQXvgOq50896827 = FAIzQXvgOq68197003;     FAIzQXvgOq68197003 = FAIzQXvgOq93613654;     FAIzQXvgOq93613654 = FAIzQXvgOq99083841;     FAIzQXvgOq99083841 = FAIzQXvgOq54514300;     FAIzQXvgOq54514300 = FAIzQXvgOq23937860;     FAIzQXvgOq23937860 = FAIzQXvgOq24247605;     FAIzQXvgOq24247605 = FAIzQXvgOq98829701;     FAIzQXvgOq98829701 = FAIzQXvgOq2536174;     FAIzQXvgOq2536174 = FAIzQXvgOq67419808;     FAIzQXvgOq67419808 = FAIzQXvgOq74737837;     FAIzQXvgOq74737837 = FAIzQXvgOq15111531;     FAIzQXvgOq15111531 = FAIzQXvgOq40356349;     FAIzQXvgOq40356349 = FAIzQXvgOq10459173;     FAIzQXvgOq10459173 = FAIzQXvgOq94828457;     FAIzQXvgOq94828457 = FAIzQXvgOq21532060;     FAIzQXvgOq21532060 = FAIzQXvgOq96471509;     FAIzQXvgOq96471509 = FAIzQXvgOq58808770;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zMksIcbFLZ2011646() {     int xLYmKhZuYv81195887 = -600116425;    int xLYmKhZuYv34800799 = -264752158;    int xLYmKhZuYv91097435 = -525762357;    int xLYmKhZuYv24341031 = -201160217;    int xLYmKhZuYv90716841 = -112820018;    int xLYmKhZuYv15062628 = -38724269;    int xLYmKhZuYv6775233 = -567019022;    int xLYmKhZuYv35414377 = -335849309;    int xLYmKhZuYv25090234 = -814695849;    int xLYmKhZuYv94180548 = -503489921;    int xLYmKhZuYv66594456 = 58553375;    int xLYmKhZuYv94614322 = -910380014;    int xLYmKhZuYv7201965 = -717092655;    int xLYmKhZuYv44827455 = -136869759;    int xLYmKhZuYv16665694 = -304536833;    int xLYmKhZuYv93531018 = -511036388;    int xLYmKhZuYv51026556 = -996560661;    int xLYmKhZuYv96387939 = -532075678;    int xLYmKhZuYv70147629 = -260449361;    int xLYmKhZuYv33131633 = -882849987;    int xLYmKhZuYv95172424 = -603715290;    int xLYmKhZuYv16526652 = -567774739;    int xLYmKhZuYv87430065 = -942432784;    int xLYmKhZuYv82084602 = 59828869;    int xLYmKhZuYv48393664 = -446060101;    int xLYmKhZuYv23565847 = 68159370;    int xLYmKhZuYv25988526 = 11122805;    int xLYmKhZuYv44455465 = -968237217;    int xLYmKhZuYv32170160 = -493269267;    int xLYmKhZuYv89373391 = -944196973;    int xLYmKhZuYv19477345 = -653767572;    int xLYmKhZuYv52257944 = -383291465;    int xLYmKhZuYv80054691 = -277079317;    int xLYmKhZuYv54403901 = 87238629;    int xLYmKhZuYv6574306 = -327592789;    int xLYmKhZuYv6839127 = -592050065;    int xLYmKhZuYv2719227 = -983386139;    int xLYmKhZuYv17526834 = -16761680;    int xLYmKhZuYv80325704 = -119417918;    int xLYmKhZuYv56577650 = -107941107;    int xLYmKhZuYv45189199 = -728395751;    int xLYmKhZuYv33470483 = -802733768;    int xLYmKhZuYv48089541 = -357373470;    int xLYmKhZuYv48206603 = -973298603;    int xLYmKhZuYv98790619 = -497597605;    int xLYmKhZuYv50912683 = -435467657;    int xLYmKhZuYv19638776 = -252137009;    int xLYmKhZuYv81365662 = -305756099;    int xLYmKhZuYv82448364 = -450541366;    int xLYmKhZuYv71464517 = -746424576;    int xLYmKhZuYv20888426 = -91875597;    int xLYmKhZuYv55424142 = -35435396;    int xLYmKhZuYv73947315 = -503450958;    int xLYmKhZuYv4788266 = -308612617;    int xLYmKhZuYv6839222 = -467289529;    int xLYmKhZuYv64669236 = 67658314;    int xLYmKhZuYv47370733 = -322319375;    int xLYmKhZuYv9012833 = -485591227;    int xLYmKhZuYv75947366 = -755100117;    int xLYmKhZuYv67150995 = -80979389;    int xLYmKhZuYv89074101 = 50152925;    int xLYmKhZuYv62319767 = -598781806;    int xLYmKhZuYv3244217 = -842580043;    int xLYmKhZuYv35716842 = -870498877;    int xLYmKhZuYv74703203 = -849722350;    int xLYmKhZuYv14336513 = -558155160;    int xLYmKhZuYv14559632 = -533300698;    int xLYmKhZuYv52798063 = -704331285;    int xLYmKhZuYv38253149 = -809276971;    int xLYmKhZuYv9826567 = -712486769;    int xLYmKhZuYv90811791 = -527650249;    int xLYmKhZuYv33499723 = -879798982;    int xLYmKhZuYv16062235 = -312657761;    int xLYmKhZuYv13569980 = -52508255;    int xLYmKhZuYv87942433 = -54454237;    int xLYmKhZuYv61701941 = -800981522;    int xLYmKhZuYv68437110 = -110401269;    int xLYmKhZuYv39223462 = -969134182;    int xLYmKhZuYv83293983 = -442573526;    int xLYmKhZuYv97480980 = 89407555;    int xLYmKhZuYv3927071 = -679703622;    int xLYmKhZuYv44622864 = -683121096;    int xLYmKhZuYv62007100 = -417695852;    int xLYmKhZuYv60705642 = -746844692;    int xLYmKhZuYv68484966 = -752321376;    int xLYmKhZuYv64053202 = -518332177;    int xLYmKhZuYv78310629 = -879840507;    int xLYmKhZuYv75266425 = -968466700;    int xLYmKhZuYv47564680 = -445471842;    int xLYmKhZuYv41905070 = -295251103;    int xLYmKhZuYv59468394 = -169730691;    int xLYmKhZuYv93706394 = -397794913;    int xLYmKhZuYv41579468 = -261661564;    int xLYmKhZuYv13174710 = 61561470;    int xLYmKhZuYv67503549 = -58094032;    int xLYmKhZuYv82869431 = -29613945;    int xLYmKhZuYv30226266 = -960153726;    int xLYmKhZuYv12372699 = -486874594;    int xLYmKhZuYv73503399 = -23576253;    int xLYmKhZuYv84454107 = -600116425;     xLYmKhZuYv81195887 = xLYmKhZuYv34800799;     xLYmKhZuYv34800799 = xLYmKhZuYv91097435;     xLYmKhZuYv91097435 = xLYmKhZuYv24341031;     xLYmKhZuYv24341031 = xLYmKhZuYv90716841;     xLYmKhZuYv90716841 = xLYmKhZuYv15062628;     xLYmKhZuYv15062628 = xLYmKhZuYv6775233;     xLYmKhZuYv6775233 = xLYmKhZuYv35414377;     xLYmKhZuYv35414377 = xLYmKhZuYv25090234;     xLYmKhZuYv25090234 = xLYmKhZuYv94180548;     xLYmKhZuYv94180548 = xLYmKhZuYv66594456;     xLYmKhZuYv66594456 = xLYmKhZuYv94614322;     xLYmKhZuYv94614322 = xLYmKhZuYv7201965;     xLYmKhZuYv7201965 = xLYmKhZuYv44827455;     xLYmKhZuYv44827455 = xLYmKhZuYv16665694;     xLYmKhZuYv16665694 = xLYmKhZuYv93531018;     xLYmKhZuYv93531018 = xLYmKhZuYv51026556;     xLYmKhZuYv51026556 = xLYmKhZuYv96387939;     xLYmKhZuYv96387939 = xLYmKhZuYv70147629;     xLYmKhZuYv70147629 = xLYmKhZuYv33131633;     xLYmKhZuYv33131633 = xLYmKhZuYv95172424;     xLYmKhZuYv95172424 = xLYmKhZuYv16526652;     xLYmKhZuYv16526652 = xLYmKhZuYv87430065;     xLYmKhZuYv87430065 = xLYmKhZuYv82084602;     xLYmKhZuYv82084602 = xLYmKhZuYv48393664;     xLYmKhZuYv48393664 = xLYmKhZuYv23565847;     xLYmKhZuYv23565847 = xLYmKhZuYv25988526;     xLYmKhZuYv25988526 = xLYmKhZuYv44455465;     xLYmKhZuYv44455465 = xLYmKhZuYv32170160;     xLYmKhZuYv32170160 = xLYmKhZuYv89373391;     xLYmKhZuYv89373391 = xLYmKhZuYv19477345;     xLYmKhZuYv19477345 = xLYmKhZuYv52257944;     xLYmKhZuYv52257944 = xLYmKhZuYv80054691;     xLYmKhZuYv80054691 = xLYmKhZuYv54403901;     xLYmKhZuYv54403901 = xLYmKhZuYv6574306;     xLYmKhZuYv6574306 = xLYmKhZuYv6839127;     xLYmKhZuYv6839127 = xLYmKhZuYv2719227;     xLYmKhZuYv2719227 = xLYmKhZuYv17526834;     xLYmKhZuYv17526834 = xLYmKhZuYv80325704;     xLYmKhZuYv80325704 = xLYmKhZuYv56577650;     xLYmKhZuYv56577650 = xLYmKhZuYv45189199;     xLYmKhZuYv45189199 = xLYmKhZuYv33470483;     xLYmKhZuYv33470483 = xLYmKhZuYv48089541;     xLYmKhZuYv48089541 = xLYmKhZuYv48206603;     xLYmKhZuYv48206603 = xLYmKhZuYv98790619;     xLYmKhZuYv98790619 = xLYmKhZuYv50912683;     xLYmKhZuYv50912683 = xLYmKhZuYv19638776;     xLYmKhZuYv19638776 = xLYmKhZuYv81365662;     xLYmKhZuYv81365662 = xLYmKhZuYv82448364;     xLYmKhZuYv82448364 = xLYmKhZuYv71464517;     xLYmKhZuYv71464517 = xLYmKhZuYv20888426;     xLYmKhZuYv20888426 = xLYmKhZuYv55424142;     xLYmKhZuYv55424142 = xLYmKhZuYv73947315;     xLYmKhZuYv73947315 = xLYmKhZuYv4788266;     xLYmKhZuYv4788266 = xLYmKhZuYv6839222;     xLYmKhZuYv6839222 = xLYmKhZuYv64669236;     xLYmKhZuYv64669236 = xLYmKhZuYv47370733;     xLYmKhZuYv47370733 = xLYmKhZuYv9012833;     xLYmKhZuYv9012833 = xLYmKhZuYv75947366;     xLYmKhZuYv75947366 = xLYmKhZuYv67150995;     xLYmKhZuYv67150995 = xLYmKhZuYv89074101;     xLYmKhZuYv89074101 = xLYmKhZuYv62319767;     xLYmKhZuYv62319767 = xLYmKhZuYv3244217;     xLYmKhZuYv3244217 = xLYmKhZuYv35716842;     xLYmKhZuYv35716842 = xLYmKhZuYv74703203;     xLYmKhZuYv74703203 = xLYmKhZuYv14336513;     xLYmKhZuYv14336513 = xLYmKhZuYv14559632;     xLYmKhZuYv14559632 = xLYmKhZuYv52798063;     xLYmKhZuYv52798063 = xLYmKhZuYv38253149;     xLYmKhZuYv38253149 = xLYmKhZuYv9826567;     xLYmKhZuYv9826567 = xLYmKhZuYv90811791;     xLYmKhZuYv90811791 = xLYmKhZuYv33499723;     xLYmKhZuYv33499723 = xLYmKhZuYv16062235;     xLYmKhZuYv16062235 = xLYmKhZuYv13569980;     xLYmKhZuYv13569980 = xLYmKhZuYv87942433;     xLYmKhZuYv87942433 = xLYmKhZuYv61701941;     xLYmKhZuYv61701941 = xLYmKhZuYv68437110;     xLYmKhZuYv68437110 = xLYmKhZuYv39223462;     xLYmKhZuYv39223462 = xLYmKhZuYv83293983;     xLYmKhZuYv83293983 = xLYmKhZuYv97480980;     xLYmKhZuYv97480980 = xLYmKhZuYv3927071;     xLYmKhZuYv3927071 = xLYmKhZuYv44622864;     xLYmKhZuYv44622864 = xLYmKhZuYv62007100;     xLYmKhZuYv62007100 = xLYmKhZuYv60705642;     xLYmKhZuYv60705642 = xLYmKhZuYv68484966;     xLYmKhZuYv68484966 = xLYmKhZuYv64053202;     xLYmKhZuYv64053202 = xLYmKhZuYv78310629;     xLYmKhZuYv78310629 = xLYmKhZuYv75266425;     xLYmKhZuYv75266425 = xLYmKhZuYv47564680;     xLYmKhZuYv47564680 = xLYmKhZuYv41905070;     xLYmKhZuYv41905070 = xLYmKhZuYv59468394;     xLYmKhZuYv59468394 = xLYmKhZuYv93706394;     xLYmKhZuYv93706394 = xLYmKhZuYv41579468;     xLYmKhZuYv41579468 = xLYmKhZuYv13174710;     xLYmKhZuYv13174710 = xLYmKhZuYv67503549;     xLYmKhZuYv67503549 = xLYmKhZuYv82869431;     xLYmKhZuYv82869431 = xLYmKhZuYv30226266;     xLYmKhZuYv30226266 = xLYmKhZuYv12372699;     xLYmKhZuYv12372699 = xLYmKhZuYv73503399;     xLYmKhZuYv73503399 = xLYmKhZuYv84454107;     xLYmKhZuYv84454107 = xLYmKhZuYv81195887;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void PKkYJmfnQL38656620() {     int IAiwbxCCXL3583005 = -234520388;    int IAiwbxCCXL19148299 = -213295213;    int IAiwbxCCXL19866714 = -765416201;    int IAiwbxCCXL39347719 = -207567749;    int IAiwbxCCXL88891096 = -703877020;    int IAiwbxCCXL29945745 = -999332647;    int IAiwbxCCXL98930971 = -389006329;    int IAiwbxCCXL11048236 = -806445953;    int IAiwbxCCXL22253365 = -698891888;    int IAiwbxCCXL85944133 = -246231254;    int IAiwbxCCXL4136317 = 61928974;    int IAiwbxCCXL50517271 = -61125307;    int IAiwbxCCXL74210144 = -235809974;    int IAiwbxCCXL87442207 = -664578516;    int IAiwbxCCXL53400970 = 67985454;    int IAiwbxCCXL37786931 = -892703930;    int IAiwbxCCXL849909 = -173840481;    int IAiwbxCCXL2364356 = -983818369;    int IAiwbxCCXL11429261 = -395551486;    int IAiwbxCCXL11647916 = -35484441;    int IAiwbxCCXL96055531 = -595202598;    int IAiwbxCCXL80341869 = 27458948;    int IAiwbxCCXL90723388 = -18513376;    int IAiwbxCCXL8584390 = -504163988;    int IAiwbxCCXL83933911 = -67619226;    int IAiwbxCCXL11868842 = -375734854;    int IAiwbxCCXL73077946 = -846463687;    int IAiwbxCCXL50366915 = -756691671;    int IAiwbxCCXL39062720 = -837526386;    int IAiwbxCCXL98530570 = -864492839;    int IAiwbxCCXL63768150 = -735083724;    int IAiwbxCCXL9799136 = 64637180;    int IAiwbxCCXL61093362 = -806422811;    int IAiwbxCCXL13792589 = -236888221;    int IAiwbxCCXL82803670 = -391349211;    int IAiwbxCCXL48531996 = -907445441;    int IAiwbxCCXL96158939 = -716334091;    int IAiwbxCCXL71152934 = -984387152;    int IAiwbxCCXL28633836 = -520463950;    int IAiwbxCCXL76763365 = -244165136;    int IAiwbxCCXL73946569 = -131573417;    int IAiwbxCCXL21978875 = -560642459;    int IAiwbxCCXL53639733 = -244904054;    int IAiwbxCCXL47650723 = -970662252;    int IAiwbxCCXL66773886 = -306741083;    int IAiwbxCCXL59012965 = -617696752;    int IAiwbxCCXL31936174 = -830547262;    int IAiwbxCCXL25964337 = -62291090;    int IAiwbxCCXL76908270 = -400258064;    int IAiwbxCCXL68548263 = -888444835;    int IAiwbxCCXL29757642 = -446951874;    int IAiwbxCCXL29275400 = -466254678;    int IAiwbxCCXL52261718 = -175362188;    int IAiwbxCCXL65074811 = -584293425;    int IAiwbxCCXL42601089 = -607993644;    int IAiwbxCCXL23241135 = -161979336;    int IAiwbxCCXL28424911 = -94781837;    int IAiwbxCCXL11282324 = -161252214;    int IAiwbxCCXL55413808 = -39948524;    int IAiwbxCCXL77022254 = -228142166;    int IAiwbxCCXL56867798 = -52868960;    int IAiwbxCCXL48564056 = -632314658;    int IAiwbxCCXL71985515 = -968919568;    int IAiwbxCCXL23722794 = -834399050;    int IAiwbxCCXL22175983 = -511147531;    int IAiwbxCCXL94337181 = 97291794;    int IAiwbxCCXL89423909 = -254702496;    int IAiwbxCCXL60417556 = -998921754;    int IAiwbxCCXL4638537 = -173229305;    int IAiwbxCCXL4868975 = -24569105;    int IAiwbxCCXL41627992 = -76369840;    int IAiwbxCCXL29696974 = -189453330;    int IAiwbxCCXL73730520 = -363354420;    int IAiwbxCCXL34665896 = -51386351;    int IAiwbxCCXL37701346 = -903911025;    int IAiwbxCCXL74076657 = 65439861;    int IAiwbxCCXL26702137 = -727636999;    int IAiwbxCCXL43072666 = -47851125;    int IAiwbxCCXL41810504 = -97422905;    int IAiwbxCCXL24920946 = -449922474;    int IAiwbxCCXL79932668 = -545187593;    int IAiwbxCCXL47113610 = -684172598;    int IAiwbxCCXL73458645 = -256433607;    int IAiwbxCCXL70514457 = -949081552;    int IAiwbxCCXL68772929 = -317540966;    int IAiwbxCCXL34492751 = -168829047;    int IAiwbxCCXL57537417 = -760000632;    int IAiwbxCCXL96018550 = -122129387;    int IAiwbxCCXL71191499 = -628894577;    int IAiwbxCCXL59562536 = -129369876;    int IAiwbxCCXL20107086 = -712663604;    int IAiwbxCCXL84876615 = -455081878;    int IAiwbxCCXL15739127 = -844438629;    int IAiwbxCCXL51611581 = -192321784;    int IAiwbxCCXL19895567 = -91296176;    int IAiwbxCCXL25382514 = -499258759;    int IAiwbxCCXL49993359 = -591722892;    int IAiwbxCCXL29916939 = -410505005;    int IAiwbxCCXL25474740 = -359514721;    int IAiwbxCCXL72436705 = -234520388;     IAiwbxCCXL3583005 = IAiwbxCCXL19148299;     IAiwbxCCXL19148299 = IAiwbxCCXL19866714;     IAiwbxCCXL19866714 = IAiwbxCCXL39347719;     IAiwbxCCXL39347719 = IAiwbxCCXL88891096;     IAiwbxCCXL88891096 = IAiwbxCCXL29945745;     IAiwbxCCXL29945745 = IAiwbxCCXL98930971;     IAiwbxCCXL98930971 = IAiwbxCCXL11048236;     IAiwbxCCXL11048236 = IAiwbxCCXL22253365;     IAiwbxCCXL22253365 = IAiwbxCCXL85944133;     IAiwbxCCXL85944133 = IAiwbxCCXL4136317;     IAiwbxCCXL4136317 = IAiwbxCCXL50517271;     IAiwbxCCXL50517271 = IAiwbxCCXL74210144;     IAiwbxCCXL74210144 = IAiwbxCCXL87442207;     IAiwbxCCXL87442207 = IAiwbxCCXL53400970;     IAiwbxCCXL53400970 = IAiwbxCCXL37786931;     IAiwbxCCXL37786931 = IAiwbxCCXL849909;     IAiwbxCCXL849909 = IAiwbxCCXL2364356;     IAiwbxCCXL2364356 = IAiwbxCCXL11429261;     IAiwbxCCXL11429261 = IAiwbxCCXL11647916;     IAiwbxCCXL11647916 = IAiwbxCCXL96055531;     IAiwbxCCXL96055531 = IAiwbxCCXL80341869;     IAiwbxCCXL80341869 = IAiwbxCCXL90723388;     IAiwbxCCXL90723388 = IAiwbxCCXL8584390;     IAiwbxCCXL8584390 = IAiwbxCCXL83933911;     IAiwbxCCXL83933911 = IAiwbxCCXL11868842;     IAiwbxCCXL11868842 = IAiwbxCCXL73077946;     IAiwbxCCXL73077946 = IAiwbxCCXL50366915;     IAiwbxCCXL50366915 = IAiwbxCCXL39062720;     IAiwbxCCXL39062720 = IAiwbxCCXL98530570;     IAiwbxCCXL98530570 = IAiwbxCCXL63768150;     IAiwbxCCXL63768150 = IAiwbxCCXL9799136;     IAiwbxCCXL9799136 = IAiwbxCCXL61093362;     IAiwbxCCXL61093362 = IAiwbxCCXL13792589;     IAiwbxCCXL13792589 = IAiwbxCCXL82803670;     IAiwbxCCXL82803670 = IAiwbxCCXL48531996;     IAiwbxCCXL48531996 = IAiwbxCCXL96158939;     IAiwbxCCXL96158939 = IAiwbxCCXL71152934;     IAiwbxCCXL71152934 = IAiwbxCCXL28633836;     IAiwbxCCXL28633836 = IAiwbxCCXL76763365;     IAiwbxCCXL76763365 = IAiwbxCCXL73946569;     IAiwbxCCXL73946569 = IAiwbxCCXL21978875;     IAiwbxCCXL21978875 = IAiwbxCCXL53639733;     IAiwbxCCXL53639733 = IAiwbxCCXL47650723;     IAiwbxCCXL47650723 = IAiwbxCCXL66773886;     IAiwbxCCXL66773886 = IAiwbxCCXL59012965;     IAiwbxCCXL59012965 = IAiwbxCCXL31936174;     IAiwbxCCXL31936174 = IAiwbxCCXL25964337;     IAiwbxCCXL25964337 = IAiwbxCCXL76908270;     IAiwbxCCXL76908270 = IAiwbxCCXL68548263;     IAiwbxCCXL68548263 = IAiwbxCCXL29757642;     IAiwbxCCXL29757642 = IAiwbxCCXL29275400;     IAiwbxCCXL29275400 = IAiwbxCCXL52261718;     IAiwbxCCXL52261718 = IAiwbxCCXL65074811;     IAiwbxCCXL65074811 = IAiwbxCCXL42601089;     IAiwbxCCXL42601089 = IAiwbxCCXL23241135;     IAiwbxCCXL23241135 = IAiwbxCCXL28424911;     IAiwbxCCXL28424911 = IAiwbxCCXL11282324;     IAiwbxCCXL11282324 = IAiwbxCCXL55413808;     IAiwbxCCXL55413808 = IAiwbxCCXL77022254;     IAiwbxCCXL77022254 = IAiwbxCCXL56867798;     IAiwbxCCXL56867798 = IAiwbxCCXL48564056;     IAiwbxCCXL48564056 = IAiwbxCCXL71985515;     IAiwbxCCXL71985515 = IAiwbxCCXL23722794;     IAiwbxCCXL23722794 = IAiwbxCCXL22175983;     IAiwbxCCXL22175983 = IAiwbxCCXL94337181;     IAiwbxCCXL94337181 = IAiwbxCCXL89423909;     IAiwbxCCXL89423909 = IAiwbxCCXL60417556;     IAiwbxCCXL60417556 = IAiwbxCCXL4638537;     IAiwbxCCXL4638537 = IAiwbxCCXL4868975;     IAiwbxCCXL4868975 = IAiwbxCCXL41627992;     IAiwbxCCXL41627992 = IAiwbxCCXL29696974;     IAiwbxCCXL29696974 = IAiwbxCCXL73730520;     IAiwbxCCXL73730520 = IAiwbxCCXL34665896;     IAiwbxCCXL34665896 = IAiwbxCCXL37701346;     IAiwbxCCXL37701346 = IAiwbxCCXL74076657;     IAiwbxCCXL74076657 = IAiwbxCCXL26702137;     IAiwbxCCXL26702137 = IAiwbxCCXL43072666;     IAiwbxCCXL43072666 = IAiwbxCCXL41810504;     IAiwbxCCXL41810504 = IAiwbxCCXL24920946;     IAiwbxCCXL24920946 = IAiwbxCCXL79932668;     IAiwbxCCXL79932668 = IAiwbxCCXL47113610;     IAiwbxCCXL47113610 = IAiwbxCCXL73458645;     IAiwbxCCXL73458645 = IAiwbxCCXL70514457;     IAiwbxCCXL70514457 = IAiwbxCCXL68772929;     IAiwbxCCXL68772929 = IAiwbxCCXL34492751;     IAiwbxCCXL34492751 = IAiwbxCCXL57537417;     IAiwbxCCXL57537417 = IAiwbxCCXL96018550;     IAiwbxCCXL96018550 = IAiwbxCCXL71191499;     IAiwbxCCXL71191499 = IAiwbxCCXL59562536;     IAiwbxCCXL59562536 = IAiwbxCCXL20107086;     IAiwbxCCXL20107086 = IAiwbxCCXL84876615;     IAiwbxCCXL84876615 = IAiwbxCCXL15739127;     IAiwbxCCXL15739127 = IAiwbxCCXL51611581;     IAiwbxCCXL51611581 = IAiwbxCCXL19895567;     IAiwbxCCXL19895567 = IAiwbxCCXL25382514;     IAiwbxCCXL25382514 = IAiwbxCCXL49993359;     IAiwbxCCXL49993359 = IAiwbxCCXL29916939;     IAiwbxCCXL29916939 = IAiwbxCCXL25474740;     IAiwbxCCXL25474740 = IAiwbxCCXL72436705;     IAiwbxCCXL72436705 = IAiwbxCCXL3583005;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void nHIcYQUsYL75301595() {     int bTlUPVxoTM25970122 = -968924351;    int bTlUPVxoTM3495800 = -161838267;    int bTlUPVxoTM48635993 = 94929955;    int bTlUPVxoTM54354407 = -213975281;    int bTlUPVxoTM87065350 = -194934021;    int bTlUPVxoTM44828862 = -859941025;    int bTlUPVxoTM91086710 = -210993635;    int bTlUPVxoTM86682093 = -177042598;    int bTlUPVxoTM19416496 = -583087927;    int bTlUPVxoTM77707717 = 11027413;    int bTlUPVxoTM41678176 = 65304574;    int bTlUPVxoTM6420221 = -311870601;    int bTlUPVxoTM41218325 = -854527294;    int bTlUPVxoTM30056960 = -92287273;    int bTlUPVxoTM90136247 = -659492259;    int bTlUPVxoTM82042843 = -174371472;    int bTlUPVxoTM50673260 = -451120301;    int bTlUPVxoTM8340772 = -335561060;    int bTlUPVxoTM52710891 = -530653611;    int bTlUPVxoTM90164199 = -288118895;    int bTlUPVxoTM96938638 = -586689907;    int bTlUPVxoTM44157088 = -477307366;    int bTlUPVxoTM94016711 = -194593968;    int bTlUPVxoTM35084177 = 31843155;    int bTlUPVxoTM19474158 = -789178350;    int bTlUPVxoTM171836 = -819629078;    int bTlUPVxoTM20167367 = -604050180;    int bTlUPVxoTM56278365 = -545146124;    int bTlUPVxoTM45955281 = -81783505;    int bTlUPVxoTM7687751 = -784788706;    int bTlUPVxoTM8058955 = -816399876;    int bTlUPVxoTM67340326 = -587434174;    int bTlUPVxoTM42132032 = -235766306;    int bTlUPVxoTM73181275 = -561015071;    int bTlUPVxoTM59033035 = -455105634;    int bTlUPVxoTM90224865 = -122840818;    int bTlUPVxoTM89598651 = -449282042;    int bTlUPVxoTM24779036 = -852012624;    int bTlUPVxoTM76941967 = -921509981;    int bTlUPVxoTM96949079 = -380389165;    int bTlUPVxoTM2703941 = -634751083;    int bTlUPVxoTM10487266 = -318551150;    int bTlUPVxoTM59189925 = -132434638;    int bTlUPVxoTM47094842 = -968025901;    int bTlUPVxoTM34757153 = -115884561;    int bTlUPVxoTM67113247 = -799925847;    int bTlUPVxoTM44233571 = -308957515;    int bTlUPVxoTM70563011 = -918826081;    int bTlUPVxoTM71368175 = -349974762;    int bTlUPVxoTM65632008 = 69534906;    int bTlUPVxoTM38626859 = -802028151;    int bTlUPVxoTM3126657 = -897073960;    int bTlUPVxoTM30576121 = -947273419;    int bTlUPVxoTM25361356 = -859974234;    int bTlUPVxoTM78362956 = -748697759;    int bTlUPVxoTM81813033 = -391616986;    int bTlUPVxoTM9479088 = -967244300;    int bTlUPVxoTM13551816 = -936913201;    int bTlUPVxoTM34880250 = -424796931;    int bTlUPVxoTM86893514 = -375304943;    int bTlUPVxoTM24661495 = -155890845;    int bTlUPVxoTM34808345 = -665847511;    int bTlUPVxoTM40726813 = 4740907;    int bTlUPVxoTM11728746 = -798299222;    int bTlUPVxoTM69648762 = -172572712;    int bTlUPVxoTM74337850 = -347261252;    int bTlUPVxoTM64288188 = 23895705;    int bTlUPVxoTM68037049 = -193512224;    int bTlUPVxoTM71023924 = -637181640;    int bTlUPVxoTM99911382 = -436651442;    int bTlUPVxoTM92444192 = -725089431;    int bTlUPVxoTM25894225 = -599107677;    int bTlUPVxoTM31398805 = -414051079;    int bTlUPVxoTM55761812 = -50264447;    int bTlUPVxoTM87460258 = -653367812;    int bTlUPVxoTM86451373 = -168138757;    int bTlUPVxoTM84967163 = -244872728;    int bTlUPVxoTM46921870 = -226568068;    int bTlUPVxoTM327025 = -852272285;    int bTlUPVxoTM52360911 = -989252503;    int bTlUPVxoTM55938265 = -410671564;    int bTlUPVxoTM49604356 = -685224100;    int bTlUPVxoTM84910190 = -95171363;    int bTlUPVxoTM80323272 = -51318412;    int bTlUPVxoTM69060892 = -982760556;    int bTlUPVxoTM4932299 = -919325917;    int bTlUPVxoTM36764205 = -640160756;    int bTlUPVxoTM16770676 = -375792073;    int bTlUPVxoTM94818319 = -812317312;    int bTlUPVxoTM77220001 = 36511352;    int bTlUPVxoTM80745777 = -155596518;    int bTlUPVxoTM76046835 = -512368842;    int bTlUPVxoTM89898786 = -327215694;    int bTlUPVxoTM90048452 = -446205038;    int bTlUPVxoTM72287585 = -124498321;    int bTlUPVxoTM67895595 = -968903573;    int bTlUPVxoTM69760453 = -223292058;    int bTlUPVxoTM47461179 = -334135416;    int bTlUPVxoTM77446079 = -695453189;    int bTlUPVxoTM60419303 = -968924351;     bTlUPVxoTM25970122 = bTlUPVxoTM3495800;     bTlUPVxoTM3495800 = bTlUPVxoTM48635993;     bTlUPVxoTM48635993 = bTlUPVxoTM54354407;     bTlUPVxoTM54354407 = bTlUPVxoTM87065350;     bTlUPVxoTM87065350 = bTlUPVxoTM44828862;     bTlUPVxoTM44828862 = bTlUPVxoTM91086710;     bTlUPVxoTM91086710 = bTlUPVxoTM86682093;     bTlUPVxoTM86682093 = bTlUPVxoTM19416496;     bTlUPVxoTM19416496 = bTlUPVxoTM77707717;     bTlUPVxoTM77707717 = bTlUPVxoTM41678176;     bTlUPVxoTM41678176 = bTlUPVxoTM6420221;     bTlUPVxoTM6420221 = bTlUPVxoTM41218325;     bTlUPVxoTM41218325 = bTlUPVxoTM30056960;     bTlUPVxoTM30056960 = bTlUPVxoTM90136247;     bTlUPVxoTM90136247 = bTlUPVxoTM82042843;     bTlUPVxoTM82042843 = bTlUPVxoTM50673260;     bTlUPVxoTM50673260 = bTlUPVxoTM8340772;     bTlUPVxoTM8340772 = bTlUPVxoTM52710891;     bTlUPVxoTM52710891 = bTlUPVxoTM90164199;     bTlUPVxoTM90164199 = bTlUPVxoTM96938638;     bTlUPVxoTM96938638 = bTlUPVxoTM44157088;     bTlUPVxoTM44157088 = bTlUPVxoTM94016711;     bTlUPVxoTM94016711 = bTlUPVxoTM35084177;     bTlUPVxoTM35084177 = bTlUPVxoTM19474158;     bTlUPVxoTM19474158 = bTlUPVxoTM171836;     bTlUPVxoTM171836 = bTlUPVxoTM20167367;     bTlUPVxoTM20167367 = bTlUPVxoTM56278365;     bTlUPVxoTM56278365 = bTlUPVxoTM45955281;     bTlUPVxoTM45955281 = bTlUPVxoTM7687751;     bTlUPVxoTM7687751 = bTlUPVxoTM8058955;     bTlUPVxoTM8058955 = bTlUPVxoTM67340326;     bTlUPVxoTM67340326 = bTlUPVxoTM42132032;     bTlUPVxoTM42132032 = bTlUPVxoTM73181275;     bTlUPVxoTM73181275 = bTlUPVxoTM59033035;     bTlUPVxoTM59033035 = bTlUPVxoTM90224865;     bTlUPVxoTM90224865 = bTlUPVxoTM89598651;     bTlUPVxoTM89598651 = bTlUPVxoTM24779036;     bTlUPVxoTM24779036 = bTlUPVxoTM76941967;     bTlUPVxoTM76941967 = bTlUPVxoTM96949079;     bTlUPVxoTM96949079 = bTlUPVxoTM2703941;     bTlUPVxoTM2703941 = bTlUPVxoTM10487266;     bTlUPVxoTM10487266 = bTlUPVxoTM59189925;     bTlUPVxoTM59189925 = bTlUPVxoTM47094842;     bTlUPVxoTM47094842 = bTlUPVxoTM34757153;     bTlUPVxoTM34757153 = bTlUPVxoTM67113247;     bTlUPVxoTM67113247 = bTlUPVxoTM44233571;     bTlUPVxoTM44233571 = bTlUPVxoTM70563011;     bTlUPVxoTM70563011 = bTlUPVxoTM71368175;     bTlUPVxoTM71368175 = bTlUPVxoTM65632008;     bTlUPVxoTM65632008 = bTlUPVxoTM38626859;     bTlUPVxoTM38626859 = bTlUPVxoTM3126657;     bTlUPVxoTM3126657 = bTlUPVxoTM30576121;     bTlUPVxoTM30576121 = bTlUPVxoTM25361356;     bTlUPVxoTM25361356 = bTlUPVxoTM78362956;     bTlUPVxoTM78362956 = bTlUPVxoTM81813033;     bTlUPVxoTM81813033 = bTlUPVxoTM9479088;     bTlUPVxoTM9479088 = bTlUPVxoTM13551816;     bTlUPVxoTM13551816 = bTlUPVxoTM34880250;     bTlUPVxoTM34880250 = bTlUPVxoTM86893514;     bTlUPVxoTM86893514 = bTlUPVxoTM24661495;     bTlUPVxoTM24661495 = bTlUPVxoTM34808345;     bTlUPVxoTM34808345 = bTlUPVxoTM40726813;     bTlUPVxoTM40726813 = bTlUPVxoTM11728746;     bTlUPVxoTM11728746 = bTlUPVxoTM69648762;     bTlUPVxoTM69648762 = bTlUPVxoTM74337850;     bTlUPVxoTM74337850 = bTlUPVxoTM64288188;     bTlUPVxoTM64288188 = bTlUPVxoTM68037049;     bTlUPVxoTM68037049 = bTlUPVxoTM71023924;     bTlUPVxoTM71023924 = bTlUPVxoTM99911382;     bTlUPVxoTM99911382 = bTlUPVxoTM92444192;     bTlUPVxoTM92444192 = bTlUPVxoTM25894225;     bTlUPVxoTM25894225 = bTlUPVxoTM31398805;     bTlUPVxoTM31398805 = bTlUPVxoTM55761812;     bTlUPVxoTM55761812 = bTlUPVxoTM87460258;     bTlUPVxoTM87460258 = bTlUPVxoTM86451373;     bTlUPVxoTM86451373 = bTlUPVxoTM84967163;     bTlUPVxoTM84967163 = bTlUPVxoTM46921870;     bTlUPVxoTM46921870 = bTlUPVxoTM327025;     bTlUPVxoTM327025 = bTlUPVxoTM52360911;     bTlUPVxoTM52360911 = bTlUPVxoTM55938265;     bTlUPVxoTM55938265 = bTlUPVxoTM49604356;     bTlUPVxoTM49604356 = bTlUPVxoTM84910190;     bTlUPVxoTM84910190 = bTlUPVxoTM80323272;     bTlUPVxoTM80323272 = bTlUPVxoTM69060892;     bTlUPVxoTM69060892 = bTlUPVxoTM4932299;     bTlUPVxoTM4932299 = bTlUPVxoTM36764205;     bTlUPVxoTM36764205 = bTlUPVxoTM16770676;     bTlUPVxoTM16770676 = bTlUPVxoTM94818319;     bTlUPVxoTM94818319 = bTlUPVxoTM77220001;     bTlUPVxoTM77220001 = bTlUPVxoTM80745777;     bTlUPVxoTM80745777 = bTlUPVxoTM76046835;     bTlUPVxoTM76046835 = bTlUPVxoTM89898786;     bTlUPVxoTM89898786 = bTlUPVxoTM90048452;     bTlUPVxoTM90048452 = bTlUPVxoTM72287585;     bTlUPVxoTM72287585 = bTlUPVxoTM67895595;     bTlUPVxoTM67895595 = bTlUPVxoTM69760453;     bTlUPVxoTM69760453 = bTlUPVxoTM47461179;     bTlUPVxoTM47461179 = bTlUPVxoTM77446079;     bTlUPVxoTM77446079 = bTlUPVxoTM60419303;     bTlUPVxoTM60419303 = bTlUPVxoTM25970122;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BOMyHRevjG11946570() {     int ZdcIcArdTM48357239 = -603328314;    int ZdcIcArdTM87843299 = -110381322;    int ZdcIcArdTM77405271 = -144723889;    int ZdcIcArdTM69361096 = -220382812;    int ZdcIcArdTM85239604 = -785991022;    int ZdcIcArdTM59711979 = -720549403;    int ZdcIcArdTM83242449 = -32980941;    int ZdcIcArdTM62315952 = -647639242;    int ZdcIcArdTM16579627 = -467283967;    int ZdcIcArdTM69471302 = -831713920;    int ZdcIcArdTM79220035 = 68680173;    int ZdcIcArdTM62323169 = -562615894;    int ZdcIcArdTM8226506 = -373244614;    int ZdcIcArdTM72671711 = -619996030;    int ZdcIcArdTM26871525 = -286969972;    int ZdcIcArdTM26298756 = -556039015;    int ZdcIcArdTM496612 = -728400121;    int ZdcIcArdTM14317189 = -787303751;    int ZdcIcArdTM93992522 = -665755736;    int ZdcIcArdTM68680482 = -540753348;    int ZdcIcArdTM97821745 = -578177215;    int ZdcIcArdTM7972307 = -982073679;    int ZdcIcArdTM97310034 = -370674560;    int ZdcIcArdTM61583964 = -532149702;    int ZdcIcArdTM55014405 = -410737475;    int ZdcIcArdTM88474830 = -163523302;    int ZdcIcArdTM67256787 = -361636673;    int ZdcIcArdTM62189815 = -333600578;    int ZdcIcArdTM52847842 = -426040624;    int ZdcIcArdTM16844930 = -705084572;    int ZdcIcArdTM52349760 = -897716028;    int ZdcIcArdTM24881517 = -139505529;    int ZdcIcArdTM23170703 = -765109801;    int ZdcIcArdTM32569963 = -885141921;    int ZdcIcArdTM35262400 = -518862056;    int ZdcIcArdTM31917735 = -438236194;    int ZdcIcArdTM83038363 = -182229994;    int ZdcIcArdTM78405136 = -719638096;    int ZdcIcArdTM25250099 = -222556013;    int ZdcIcArdTM17134795 = -516613194;    int ZdcIcArdTM31461312 = -37928749;    int ZdcIcArdTM98995656 = -76459841;    int ZdcIcArdTM64740117 = -19965222;    int ZdcIcArdTM46538961 = -965389550;    int ZdcIcArdTM2740420 = 74971961;    int ZdcIcArdTM75213529 = -982154943;    int ZdcIcArdTM56530968 = -887367768;    int ZdcIcArdTM15161686 = -675361072;    int ZdcIcArdTM65828080 = -299691461;    int ZdcIcArdTM62715754 = -72485353;    int ZdcIcArdTM47496075 = -57104428;    int ZdcIcArdTM76977913 = -227893242;    int ZdcIcArdTM8890525 = -619184649;    int ZdcIcArdTM85647901 = -35655043;    int ZdcIcArdTM14124824 = -889401874;    int ZdcIcArdTM40384933 = -621254636;    int ZdcIcArdTM90533265 = -739706763;    int ZdcIcArdTM15821307 = -612574187;    int ZdcIcArdTM14346691 = -809645338;    int ZdcIcArdTM96764773 = -522467721;    int ZdcIcArdTM92455191 = -258912730;    int ZdcIcArdTM21052634 = -699380363;    int ZdcIcArdTM9468111 = -121598618;    int ZdcIcArdTM99734697 = -762199395;    int ZdcIcArdTM17121542 = -933997893;    int ZdcIcArdTM54338518 = -791814299;    int ZdcIcArdTM39152467 = -797506093;    int ZdcIcArdTM75656543 = -488102693;    int ZdcIcArdTM37409312 = -1133974;    int ZdcIcArdTM94953790 = -848733778;    int ZdcIcArdTM43260393 = -273809022;    int ZdcIcArdTM22091475 = 91237975;    int ZdcIcArdTM89067090 = -464747738;    int ZdcIcArdTM76857728 = -49142542;    int ZdcIcArdTM37219171 = -402824600;    int ZdcIcArdTM98826089 = -401717374;    int ZdcIcArdTM43232190 = -862108458;    int ZdcIcArdTM50771074 = -405285011;    int ZdcIcArdTM58843545 = -507121664;    int ZdcIcArdTM79800875 = -428582532;    int ZdcIcArdTM31943862 = -276155535;    int ZdcIcArdTM52095102 = -686275602;    int ZdcIcArdTM96361735 = 66090882;    int ZdcIcArdTM90132087 = -253555272;    int ZdcIcArdTM69348855 = -547980145;    int ZdcIcArdTM75371846 = -569822787;    int ZdcIcArdTM15990993 = -520320881;    int ZdcIcArdTM37522801 = -629454759;    int ZdcIcArdTM18445140 = -995740047;    int ZdcIcArdTM94877466 = -897607421;    int ZdcIcArdTM41384470 = -698529432;    int ZdcIcArdTM67217056 = -569655807;    int ZdcIcArdTM64058445 = -909992758;    int ZdcIcArdTM28485325 = -700088293;    int ZdcIcArdTM24679603 = -157700465;    int ZdcIcArdTM10408678 = -338548386;    int ZdcIcArdTM89527546 = -954861224;    int ZdcIcArdTM65005419 = -257765827;    int ZdcIcArdTM29417419 = 68608343;    int ZdcIcArdTM48401901 = -603328314;     ZdcIcArdTM48357239 = ZdcIcArdTM87843299;     ZdcIcArdTM87843299 = ZdcIcArdTM77405271;     ZdcIcArdTM77405271 = ZdcIcArdTM69361096;     ZdcIcArdTM69361096 = ZdcIcArdTM85239604;     ZdcIcArdTM85239604 = ZdcIcArdTM59711979;     ZdcIcArdTM59711979 = ZdcIcArdTM83242449;     ZdcIcArdTM83242449 = ZdcIcArdTM62315952;     ZdcIcArdTM62315952 = ZdcIcArdTM16579627;     ZdcIcArdTM16579627 = ZdcIcArdTM69471302;     ZdcIcArdTM69471302 = ZdcIcArdTM79220035;     ZdcIcArdTM79220035 = ZdcIcArdTM62323169;     ZdcIcArdTM62323169 = ZdcIcArdTM8226506;     ZdcIcArdTM8226506 = ZdcIcArdTM72671711;     ZdcIcArdTM72671711 = ZdcIcArdTM26871525;     ZdcIcArdTM26871525 = ZdcIcArdTM26298756;     ZdcIcArdTM26298756 = ZdcIcArdTM496612;     ZdcIcArdTM496612 = ZdcIcArdTM14317189;     ZdcIcArdTM14317189 = ZdcIcArdTM93992522;     ZdcIcArdTM93992522 = ZdcIcArdTM68680482;     ZdcIcArdTM68680482 = ZdcIcArdTM97821745;     ZdcIcArdTM97821745 = ZdcIcArdTM7972307;     ZdcIcArdTM7972307 = ZdcIcArdTM97310034;     ZdcIcArdTM97310034 = ZdcIcArdTM61583964;     ZdcIcArdTM61583964 = ZdcIcArdTM55014405;     ZdcIcArdTM55014405 = ZdcIcArdTM88474830;     ZdcIcArdTM88474830 = ZdcIcArdTM67256787;     ZdcIcArdTM67256787 = ZdcIcArdTM62189815;     ZdcIcArdTM62189815 = ZdcIcArdTM52847842;     ZdcIcArdTM52847842 = ZdcIcArdTM16844930;     ZdcIcArdTM16844930 = ZdcIcArdTM52349760;     ZdcIcArdTM52349760 = ZdcIcArdTM24881517;     ZdcIcArdTM24881517 = ZdcIcArdTM23170703;     ZdcIcArdTM23170703 = ZdcIcArdTM32569963;     ZdcIcArdTM32569963 = ZdcIcArdTM35262400;     ZdcIcArdTM35262400 = ZdcIcArdTM31917735;     ZdcIcArdTM31917735 = ZdcIcArdTM83038363;     ZdcIcArdTM83038363 = ZdcIcArdTM78405136;     ZdcIcArdTM78405136 = ZdcIcArdTM25250099;     ZdcIcArdTM25250099 = ZdcIcArdTM17134795;     ZdcIcArdTM17134795 = ZdcIcArdTM31461312;     ZdcIcArdTM31461312 = ZdcIcArdTM98995656;     ZdcIcArdTM98995656 = ZdcIcArdTM64740117;     ZdcIcArdTM64740117 = ZdcIcArdTM46538961;     ZdcIcArdTM46538961 = ZdcIcArdTM2740420;     ZdcIcArdTM2740420 = ZdcIcArdTM75213529;     ZdcIcArdTM75213529 = ZdcIcArdTM56530968;     ZdcIcArdTM56530968 = ZdcIcArdTM15161686;     ZdcIcArdTM15161686 = ZdcIcArdTM65828080;     ZdcIcArdTM65828080 = ZdcIcArdTM62715754;     ZdcIcArdTM62715754 = ZdcIcArdTM47496075;     ZdcIcArdTM47496075 = ZdcIcArdTM76977913;     ZdcIcArdTM76977913 = ZdcIcArdTM8890525;     ZdcIcArdTM8890525 = ZdcIcArdTM85647901;     ZdcIcArdTM85647901 = ZdcIcArdTM14124824;     ZdcIcArdTM14124824 = ZdcIcArdTM40384933;     ZdcIcArdTM40384933 = ZdcIcArdTM90533265;     ZdcIcArdTM90533265 = ZdcIcArdTM15821307;     ZdcIcArdTM15821307 = ZdcIcArdTM14346691;     ZdcIcArdTM14346691 = ZdcIcArdTM96764773;     ZdcIcArdTM96764773 = ZdcIcArdTM92455191;     ZdcIcArdTM92455191 = ZdcIcArdTM21052634;     ZdcIcArdTM21052634 = ZdcIcArdTM9468111;     ZdcIcArdTM9468111 = ZdcIcArdTM99734697;     ZdcIcArdTM99734697 = ZdcIcArdTM17121542;     ZdcIcArdTM17121542 = ZdcIcArdTM54338518;     ZdcIcArdTM54338518 = ZdcIcArdTM39152467;     ZdcIcArdTM39152467 = ZdcIcArdTM75656543;     ZdcIcArdTM75656543 = ZdcIcArdTM37409312;     ZdcIcArdTM37409312 = ZdcIcArdTM94953790;     ZdcIcArdTM94953790 = ZdcIcArdTM43260393;     ZdcIcArdTM43260393 = ZdcIcArdTM22091475;     ZdcIcArdTM22091475 = ZdcIcArdTM89067090;     ZdcIcArdTM89067090 = ZdcIcArdTM76857728;     ZdcIcArdTM76857728 = ZdcIcArdTM37219171;     ZdcIcArdTM37219171 = ZdcIcArdTM98826089;     ZdcIcArdTM98826089 = ZdcIcArdTM43232190;     ZdcIcArdTM43232190 = ZdcIcArdTM50771074;     ZdcIcArdTM50771074 = ZdcIcArdTM58843545;     ZdcIcArdTM58843545 = ZdcIcArdTM79800875;     ZdcIcArdTM79800875 = ZdcIcArdTM31943862;     ZdcIcArdTM31943862 = ZdcIcArdTM52095102;     ZdcIcArdTM52095102 = ZdcIcArdTM96361735;     ZdcIcArdTM96361735 = ZdcIcArdTM90132087;     ZdcIcArdTM90132087 = ZdcIcArdTM69348855;     ZdcIcArdTM69348855 = ZdcIcArdTM75371846;     ZdcIcArdTM75371846 = ZdcIcArdTM15990993;     ZdcIcArdTM15990993 = ZdcIcArdTM37522801;     ZdcIcArdTM37522801 = ZdcIcArdTM18445140;     ZdcIcArdTM18445140 = ZdcIcArdTM94877466;     ZdcIcArdTM94877466 = ZdcIcArdTM41384470;     ZdcIcArdTM41384470 = ZdcIcArdTM67217056;     ZdcIcArdTM67217056 = ZdcIcArdTM64058445;     ZdcIcArdTM64058445 = ZdcIcArdTM28485325;     ZdcIcArdTM28485325 = ZdcIcArdTM24679603;     ZdcIcArdTM24679603 = ZdcIcArdTM10408678;     ZdcIcArdTM10408678 = ZdcIcArdTM89527546;     ZdcIcArdTM89527546 = ZdcIcArdTM65005419;     ZdcIcArdTM65005419 = ZdcIcArdTM29417419;     ZdcIcArdTM29417419 = ZdcIcArdTM48401901;     ZdcIcArdTM48401901 = ZdcIcArdTM48357239;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void lcBATYeVCG48591544() {     int oueEOOaned70744356 = -237732277;    int oueEOOaned72190800 = -58924377;    int oueEOOaned6174550 = -384377734;    int oueEOOaned84367784 = -226790344;    int oueEOOaned83413858 = -277048024;    int oueEOOaned74595095 = -581157781;    int oueEOOaned75398188 = -954968247;    int oueEOOaned37949811 = -18235886;    int oueEOOaned13742758 = -351480006;    int oueEOOaned61234887 = -574455253;    int oueEOOaned16761895 = 72055772;    int oueEOOaned18226119 = -813361187;    int oueEOOaned75234686 = -991961933;    int oueEOOaned15286464 = -47704787;    int oueEOOaned63606801 = 85552315;    int oueEOOaned70554668 = -937706557;    int oueEOOaned50319963 = 94320059;    int oueEOOaned20293605 = -139046441;    int oueEOOaned35274153 = -800857862;    int oueEOOaned47196766 = -793387802;    int oueEOOaned98704853 = -569664523;    int oueEOOaned71787525 = -386839992;    int oueEOOaned603358 = -546755152;    int oueEOOaned88083751 = 3857440;    int oueEOOaned90554651 = -32296599;    int oueEOOaned76777825 = -607417526;    int oueEOOaned14346208 = -119223166;    int oueEOOaned68101265 = -122055032;    int oueEOOaned59740403 = -770297743;    int oueEOOaned26002110 = -625380439;    int oueEOOaned96640565 = -979032180;    int oueEOOaned82422708 = -791576884;    int oueEOOaned4209373 = -194453296;    int oueEOOaned91958650 = -109268771;    int oueEOOaned11491765 = -582618479;    int oueEOOaned73610603 = -753631571;    int oueEOOaned76478075 = 84822055;    int oueEOOaned32031238 = -587263568;    int oueEOOaned73558229 = -623602045;    int oueEOOaned37320509 = -652837224;    int oueEOOaned60218682 = -541106415;    int oueEOOaned87504048 = -934368532;    int oueEOOaned70290309 = 92504195;    int oueEOOaned45983081 = -962753198;    int oueEOOaned70723686 = -834171516;    int oueEOOaned83313811 = -64384038;    int oueEOOaned68828365 = -365778021;    int oueEOOaned59760360 = -431896062;    int oueEOOaned60287985 = -249408159;    int oueEOOaned59799500 = -214505612;    int oueEOOaned56365291 = -412180705;    int oueEOOaned50829171 = -658712524;    int oueEOOaned87204927 = -291095879;    int oueEOOaned45934447 = -311335852;    int oueEOOaned49886691 = 69894010;    int oueEOOaned98956831 = -850892285;    int oueEOOaned71587442 = -512169225;    int oueEOOaned18090799 = -288235174;    int oueEOOaned93813132 = -94493745;    int oueEOOaned6636034 = -669630498;    int oueEOOaned60248888 = -361934615;    int oueEOOaned7296923 = -732913216;    int oueEOOaned78209408 = -247938143;    int oueEOOaned87740648 = -726099567;    int oueEOOaned64594321 = -595423074;    int oueEOOaned34339187 = -136367345;    int oueEOOaned14016746 = -518907892;    int oueEOOaned83276036 = -782693163;    int oueEOOaned3794700 = -465086309;    int oueEOOaned89996197 = -160816115;    int oueEOOaned94076592 = -922528613;    int oueEOOaned18288726 = -318416373;    int oueEOOaned46735375 = -515444397;    int oueEOOaned97953644 = -48020638;    int oueEOOaned86978083 = -152281388;    int oueEOOaned11200805 = -635295992;    int oueEOOaned1497217 = -379344187;    int oueEOOaned54620277 = -584001954;    int oueEOOaned17360066 = -161971044;    int oueEOOaned7240841 = -967912562;    int oueEOOaned7949460 = -141639506;    int oueEOOaned54585847 = -687327104;    int oueEOOaned7813281 = -872646873;    int oueEOOaned99940902 = -455792132;    int oueEOOaned69636818 = -113199735;    int oueEOOaned45811395 = -220319657;    int oueEOOaned95217780 = -400481005;    int oueEOOaned58274926 = -883117445;    int oueEOOaned42071959 = -79162782;    int oueEOOaned12534933 = -731726194;    int oueEOOaned2023162 = -141462346;    int oueEOOaned58387277 = -626942771;    int oueEOOaned38218105 = -392769823;    int oueEOOaned66922196 = -953971547;    int oueEOOaned77071621 = -190902609;    int oueEOOaned52921760 = -808193200;    int oueEOOaned9294640 = -586430390;    int oueEOOaned82549660 = -181396239;    int oueEOOaned81388759 = -267330125;    int oueEOOaned36384499 = -237732277;     oueEOOaned70744356 = oueEOOaned72190800;     oueEOOaned72190800 = oueEOOaned6174550;     oueEOOaned6174550 = oueEOOaned84367784;     oueEOOaned84367784 = oueEOOaned83413858;     oueEOOaned83413858 = oueEOOaned74595095;     oueEOOaned74595095 = oueEOOaned75398188;     oueEOOaned75398188 = oueEOOaned37949811;     oueEOOaned37949811 = oueEOOaned13742758;     oueEOOaned13742758 = oueEOOaned61234887;     oueEOOaned61234887 = oueEOOaned16761895;     oueEOOaned16761895 = oueEOOaned18226119;     oueEOOaned18226119 = oueEOOaned75234686;     oueEOOaned75234686 = oueEOOaned15286464;     oueEOOaned15286464 = oueEOOaned63606801;     oueEOOaned63606801 = oueEOOaned70554668;     oueEOOaned70554668 = oueEOOaned50319963;     oueEOOaned50319963 = oueEOOaned20293605;     oueEOOaned20293605 = oueEOOaned35274153;     oueEOOaned35274153 = oueEOOaned47196766;     oueEOOaned47196766 = oueEOOaned98704853;     oueEOOaned98704853 = oueEOOaned71787525;     oueEOOaned71787525 = oueEOOaned603358;     oueEOOaned603358 = oueEOOaned88083751;     oueEOOaned88083751 = oueEOOaned90554651;     oueEOOaned90554651 = oueEOOaned76777825;     oueEOOaned76777825 = oueEOOaned14346208;     oueEOOaned14346208 = oueEOOaned68101265;     oueEOOaned68101265 = oueEOOaned59740403;     oueEOOaned59740403 = oueEOOaned26002110;     oueEOOaned26002110 = oueEOOaned96640565;     oueEOOaned96640565 = oueEOOaned82422708;     oueEOOaned82422708 = oueEOOaned4209373;     oueEOOaned4209373 = oueEOOaned91958650;     oueEOOaned91958650 = oueEOOaned11491765;     oueEOOaned11491765 = oueEOOaned73610603;     oueEOOaned73610603 = oueEOOaned76478075;     oueEOOaned76478075 = oueEOOaned32031238;     oueEOOaned32031238 = oueEOOaned73558229;     oueEOOaned73558229 = oueEOOaned37320509;     oueEOOaned37320509 = oueEOOaned60218682;     oueEOOaned60218682 = oueEOOaned87504048;     oueEOOaned87504048 = oueEOOaned70290309;     oueEOOaned70290309 = oueEOOaned45983081;     oueEOOaned45983081 = oueEOOaned70723686;     oueEOOaned70723686 = oueEOOaned83313811;     oueEOOaned83313811 = oueEOOaned68828365;     oueEOOaned68828365 = oueEOOaned59760360;     oueEOOaned59760360 = oueEOOaned60287985;     oueEOOaned60287985 = oueEOOaned59799500;     oueEOOaned59799500 = oueEOOaned56365291;     oueEOOaned56365291 = oueEOOaned50829171;     oueEOOaned50829171 = oueEOOaned87204927;     oueEOOaned87204927 = oueEOOaned45934447;     oueEOOaned45934447 = oueEOOaned49886691;     oueEOOaned49886691 = oueEOOaned98956831;     oueEOOaned98956831 = oueEOOaned71587442;     oueEOOaned71587442 = oueEOOaned18090799;     oueEOOaned18090799 = oueEOOaned93813132;     oueEOOaned93813132 = oueEOOaned6636034;     oueEOOaned6636034 = oueEOOaned60248888;     oueEOOaned60248888 = oueEOOaned7296923;     oueEOOaned7296923 = oueEOOaned78209408;     oueEOOaned78209408 = oueEOOaned87740648;     oueEOOaned87740648 = oueEOOaned64594321;     oueEOOaned64594321 = oueEOOaned34339187;     oueEOOaned34339187 = oueEOOaned14016746;     oueEOOaned14016746 = oueEOOaned83276036;     oueEOOaned83276036 = oueEOOaned3794700;     oueEOOaned3794700 = oueEOOaned89996197;     oueEOOaned89996197 = oueEOOaned94076592;     oueEOOaned94076592 = oueEOOaned18288726;     oueEOOaned18288726 = oueEOOaned46735375;     oueEOOaned46735375 = oueEOOaned97953644;     oueEOOaned97953644 = oueEOOaned86978083;     oueEOOaned86978083 = oueEOOaned11200805;     oueEOOaned11200805 = oueEOOaned1497217;     oueEOOaned1497217 = oueEOOaned54620277;     oueEOOaned54620277 = oueEOOaned17360066;     oueEOOaned17360066 = oueEOOaned7240841;     oueEOOaned7240841 = oueEOOaned7949460;     oueEOOaned7949460 = oueEOOaned54585847;     oueEOOaned54585847 = oueEOOaned7813281;     oueEOOaned7813281 = oueEOOaned99940902;     oueEOOaned99940902 = oueEOOaned69636818;     oueEOOaned69636818 = oueEOOaned45811395;     oueEOOaned45811395 = oueEOOaned95217780;     oueEOOaned95217780 = oueEOOaned58274926;     oueEOOaned58274926 = oueEOOaned42071959;     oueEOOaned42071959 = oueEOOaned12534933;     oueEOOaned12534933 = oueEOOaned2023162;     oueEOOaned2023162 = oueEOOaned58387277;     oueEOOaned58387277 = oueEOOaned38218105;     oueEOOaned38218105 = oueEOOaned66922196;     oueEOOaned66922196 = oueEOOaned77071621;     oueEOOaned77071621 = oueEOOaned52921760;     oueEOOaned52921760 = oueEOOaned9294640;     oueEOOaned9294640 = oueEOOaned82549660;     oueEOOaned82549660 = oueEOOaned81388759;     oueEOOaned81388759 = oueEOOaned36384499;     oueEOOaned36384499 = oueEOOaned70744356;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void rbzgubwYGl85236519() {     int KEFoyWytKf93131474 = -972136240;    int KEFoyWytKf56538301 = -7467432;    int KEFoyWytKf34943829 = -624031578;    int KEFoyWytKf99374472 = -233197876;    int KEFoyWytKf81588112 = -868105025;    int KEFoyWytKf89478212 = -441766159;    int KEFoyWytKf67553927 = -776955553;    int KEFoyWytKf13583670 = -488832530;    int KEFoyWytKf10905889 = -235676045;    int KEFoyWytKf52998472 = -317196586;    int KEFoyWytKf54303755 = 75431371;    int KEFoyWytKf74129067 = 35893519;    int KEFoyWytKf42242867 = -510679253;    int KEFoyWytKf57901216 = -575413544;    int KEFoyWytKf342079 = -641925398;    int KEFoyWytKf14810581 = -219374100;    int KEFoyWytKf143316 = -182959761;    int KEFoyWytKf26270021 = -590789132;    int KEFoyWytKf76555784 = -935959987;    int KEFoyWytKf25713049 = 53977745;    int KEFoyWytKf99587960 = -561151832;    int KEFoyWytKf35602743 = -891606306;    int KEFoyWytKf3896682 = -722835744;    int KEFoyWytKf14583539 = -560135417;    int KEFoyWytKf26094899 = -753855724;    int KEFoyWytKf65080819 = 48688250;    int KEFoyWytKf61435628 = -976809659;    int KEFoyWytKf74012716 = 89490515;    int KEFoyWytKf66632964 = -14554863;    int KEFoyWytKf35159289 = -545676305;    int KEFoyWytKf40931371 = 39651668;    int KEFoyWytKf39963899 = -343648238;    int KEFoyWytKf85248043 = -723796791;    int KEFoyWytKf51347338 = -433395621;    int KEFoyWytKf87721128 = -646374901;    int KEFoyWytKf15303473 = 30973053;    int KEFoyWytKf69917788 = -748125897;    int KEFoyWytKf85657338 = -454889040;    int KEFoyWytKf21866361 = 75351924;    int KEFoyWytKf57506223 = -789061253;    int KEFoyWytKf88976053 = 55715919;    int KEFoyWytKf76012439 = -692277223;    int KEFoyWytKf75840500 = -895026389;    int KEFoyWytKf45427200 = -960116847;    int KEFoyWytKf38706953 = -643314994;    int KEFoyWytKf91414093 = -246613134;    int KEFoyWytKf81125763 = -944188273;    int KEFoyWytKf4359035 = -188431053;    int KEFoyWytKf54747890 = -199124857;    int KEFoyWytKf56883245 = -356525871;    int KEFoyWytKf65234508 = -767256982;    int KEFoyWytKf24680428 = 10468194;    int KEFoyWytKf65519331 = 36992891;    int KEFoyWytKf6220992 = -587016660;    int KEFoyWytKf85648558 = -70810105;    int KEFoyWytKf57528731 = 19470065;    int KEFoyWytKf52641619 = -284631688;    int KEFoyWytKf20360290 = 36103839;    int KEFoyWytKf73279574 = -479342152;    int KEFoyWytKf16507293 = -816793276;    int KEFoyWytKf28042585 = -464956500;    int KEFoyWytKf93541211 = -766446068;    int KEFoyWytKf46950706 = -374277668;    int KEFoyWytKf75746600 = -689999740;    int KEFoyWytKf12067101 = -256848254;    int KEFoyWytKf14339856 = -580920391;    int KEFoyWytKf88881023 = -240309690;    int KEFoyWytKf90895529 = 22716368;    int KEFoyWytKf70180087 = -929038643;    int KEFoyWytKf85038605 = -572898452;    int KEFoyWytKf44892793 = -471248204;    int KEFoyWytKf14485977 = -728070721;    int KEFoyWytKf4403661 = -566141056;    int KEFoyWytKf19049561 = -46898734;    int KEFoyWytKf36736996 = 98261825;    int KEFoyWytKf23575521 = -868874609;    int KEFoyWytKf59762242 = -996579917;    int KEFoyWytKf58469481 = -762718897;    int KEFoyWytKf75876586 = -916820424;    int KEFoyWytKf34680806 = -407242591;    int KEFoyWytKf83955056 = -7123477;    int KEFoyWytKf57076593 = -688378606;    int KEFoyWytKf19264826 = -711384628;    int KEFoyWytKf9749719 = -658028992;    int KEFoyWytKf69924781 = -778419324;    int KEFoyWytKf16250943 = -970816527;    int KEFoyWytKf74444568 = -280641130;    int KEFoyWytKf79027051 = -36780131;    int KEFoyWytKf65698779 = -262585517;    int KEFoyWytKf30192398 = -565844966;    int KEFoyWytKf62661853 = -684395260;    int KEFoyWytKf49557498 = -684229736;    int KEFoyWytKf12377764 = -975546888;    int KEFoyWytKf5359068 = -107854801;    int KEFoyWytKf29463639 = -224104753;    int KEFoyWytKf95434841 = -177838013;    int KEFoyWytKf29061733 = -217999555;    int KEFoyWytKf93901 = -105026650;    int KEFoyWytKf33360099 = -603268593;    int KEFoyWytKf24367097 = -972136240;     KEFoyWytKf93131474 = KEFoyWytKf56538301;     KEFoyWytKf56538301 = KEFoyWytKf34943829;     KEFoyWytKf34943829 = KEFoyWytKf99374472;     KEFoyWytKf99374472 = KEFoyWytKf81588112;     KEFoyWytKf81588112 = KEFoyWytKf89478212;     KEFoyWytKf89478212 = KEFoyWytKf67553927;     KEFoyWytKf67553927 = KEFoyWytKf13583670;     KEFoyWytKf13583670 = KEFoyWytKf10905889;     KEFoyWytKf10905889 = KEFoyWytKf52998472;     KEFoyWytKf52998472 = KEFoyWytKf54303755;     KEFoyWytKf54303755 = KEFoyWytKf74129067;     KEFoyWytKf74129067 = KEFoyWytKf42242867;     KEFoyWytKf42242867 = KEFoyWytKf57901216;     KEFoyWytKf57901216 = KEFoyWytKf342079;     KEFoyWytKf342079 = KEFoyWytKf14810581;     KEFoyWytKf14810581 = KEFoyWytKf143316;     KEFoyWytKf143316 = KEFoyWytKf26270021;     KEFoyWytKf26270021 = KEFoyWytKf76555784;     KEFoyWytKf76555784 = KEFoyWytKf25713049;     KEFoyWytKf25713049 = KEFoyWytKf99587960;     KEFoyWytKf99587960 = KEFoyWytKf35602743;     KEFoyWytKf35602743 = KEFoyWytKf3896682;     KEFoyWytKf3896682 = KEFoyWytKf14583539;     KEFoyWytKf14583539 = KEFoyWytKf26094899;     KEFoyWytKf26094899 = KEFoyWytKf65080819;     KEFoyWytKf65080819 = KEFoyWytKf61435628;     KEFoyWytKf61435628 = KEFoyWytKf74012716;     KEFoyWytKf74012716 = KEFoyWytKf66632964;     KEFoyWytKf66632964 = KEFoyWytKf35159289;     KEFoyWytKf35159289 = KEFoyWytKf40931371;     KEFoyWytKf40931371 = KEFoyWytKf39963899;     KEFoyWytKf39963899 = KEFoyWytKf85248043;     KEFoyWytKf85248043 = KEFoyWytKf51347338;     KEFoyWytKf51347338 = KEFoyWytKf87721128;     KEFoyWytKf87721128 = KEFoyWytKf15303473;     KEFoyWytKf15303473 = KEFoyWytKf69917788;     KEFoyWytKf69917788 = KEFoyWytKf85657338;     KEFoyWytKf85657338 = KEFoyWytKf21866361;     KEFoyWytKf21866361 = KEFoyWytKf57506223;     KEFoyWytKf57506223 = KEFoyWytKf88976053;     KEFoyWytKf88976053 = KEFoyWytKf76012439;     KEFoyWytKf76012439 = KEFoyWytKf75840500;     KEFoyWytKf75840500 = KEFoyWytKf45427200;     KEFoyWytKf45427200 = KEFoyWytKf38706953;     KEFoyWytKf38706953 = KEFoyWytKf91414093;     KEFoyWytKf91414093 = KEFoyWytKf81125763;     KEFoyWytKf81125763 = KEFoyWytKf4359035;     KEFoyWytKf4359035 = KEFoyWytKf54747890;     KEFoyWytKf54747890 = KEFoyWytKf56883245;     KEFoyWytKf56883245 = KEFoyWytKf65234508;     KEFoyWytKf65234508 = KEFoyWytKf24680428;     KEFoyWytKf24680428 = KEFoyWytKf65519331;     KEFoyWytKf65519331 = KEFoyWytKf6220992;     KEFoyWytKf6220992 = KEFoyWytKf85648558;     KEFoyWytKf85648558 = KEFoyWytKf57528731;     KEFoyWytKf57528731 = KEFoyWytKf52641619;     KEFoyWytKf52641619 = KEFoyWytKf20360290;     KEFoyWytKf20360290 = KEFoyWytKf73279574;     KEFoyWytKf73279574 = KEFoyWytKf16507293;     KEFoyWytKf16507293 = KEFoyWytKf28042585;     KEFoyWytKf28042585 = KEFoyWytKf93541211;     KEFoyWytKf93541211 = KEFoyWytKf46950706;     KEFoyWytKf46950706 = KEFoyWytKf75746600;     KEFoyWytKf75746600 = KEFoyWytKf12067101;     KEFoyWytKf12067101 = KEFoyWytKf14339856;     KEFoyWytKf14339856 = KEFoyWytKf88881023;     KEFoyWytKf88881023 = KEFoyWytKf90895529;     KEFoyWytKf90895529 = KEFoyWytKf70180087;     KEFoyWytKf70180087 = KEFoyWytKf85038605;     KEFoyWytKf85038605 = KEFoyWytKf44892793;     KEFoyWytKf44892793 = KEFoyWytKf14485977;     KEFoyWytKf14485977 = KEFoyWytKf4403661;     KEFoyWytKf4403661 = KEFoyWytKf19049561;     KEFoyWytKf19049561 = KEFoyWytKf36736996;     KEFoyWytKf36736996 = KEFoyWytKf23575521;     KEFoyWytKf23575521 = KEFoyWytKf59762242;     KEFoyWytKf59762242 = KEFoyWytKf58469481;     KEFoyWytKf58469481 = KEFoyWytKf75876586;     KEFoyWytKf75876586 = KEFoyWytKf34680806;     KEFoyWytKf34680806 = KEFoyWytKf83955056;     KEFoyWytKf83955056 = KEFoyWytKf57076593;     KEFoyWytKf57076593 = KEFoyWytKf19264826;     KEFoyWytKf19264826 = KEFoyWytKf9749719;     KEFoyWytKf9749719 = KEFoyWytKf69924781;     KEFoyWytKf69924781 = KEFoyWytKf16250943;     KEFoyWytKf16250943 = KEFoyWytKf74444568;     KEFoyWytKf74444568 = KEFoyWytKf79027051;     KEFoyWytKf79027051 = KEFoyWytKf65698779;     KEFoyWytKf65698779 = KEFoyWytKf30192398;     KEFoyWytKf30192398 = KEFoyWytKf62661853;     KEFoyWytKf62661853 = KEFoyWytKf49557498;     KEFoyWytKf49557498 = KEFoyWytKf12377764;     KEFoyWytKf12377764 = KEFoyWytKf5359068;     KEFoyWytKf5359068 = KEFoyWytKf29463639;     KEFoyWytKf29463639 = KEFoyWytKf95434841;     KEFoyWytKf95434841 = KEFoyWytKf29061733;     KEFoyWytKf29061733 = KEFoyWytKf93901;     KEFoyWytKf93901 = KEFoyWytKf33360099;     KEFoyWytKf33360099 = KEFoyWytKf24367097;     KEFoyWytKf24367097 = KEFoyWytKf93131474;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xvDTZcpxZB21881494() {     int XzNVQKZCUO15518592 = -606540203;    int XzNVQKZCUO40885801 = 43989514;    int XzNVQKZCUO63713107 = -863685422;    int XzNVQKZCUO14381161 = -239605407;    int XzNVQKZCUO79762367 = -359162026;    int XzNVQKZCUO4361330 = -302374537;    int XzNVQKZCUO59709666 = -598942859;    int XzNVQKZCUO89217528 = -959429174;    int XzNVQKZCUO8069021 = -119872084;    int XzNVQKZCUO44762056 = -59937919;    int XzNVQKZCUO91845614 = 78806970;    int XzNVQKZCUO30032016 = -214851774;    int XzNVQKZCUO9251048 = -29396573;    int XzNVQKZCUO515969 = -3122301;    int XzNVQKZCUO37077356 = -269403111;    int XzNVQKZCUO59066493 = -601041642;    int XzNVQKZCUO49966667 = -460239580;    int XzNVQKZCUO32246437 = 57468177;    int XzNVQKZCUO17837415 = 28937888;    int XzNVQKZCUO4229333 = -198656709;    int XzNVQKZCUO471068 = -552639140;    int XzNVQKZCUO99417961 = -296372619;    int XzNVQKZCUO7190005 = -898916336;    int XzNVQKZCUO41083326 = -24128274;    int XzNVQKZCUO61635145 = -375414848;    int XzNVQKZCUO53383814 = -395205974;    int XzNVQKZCUO8525049 = -734396152;    int XzNVQKZCUO79924166 = -798963939;    int XzNVQKZCUO73525524 = -358811982;    int XzNVQKZCUO44316468 = -465972172;    int XzNVQKZCUO85222175 = -41664484;    int XzNVQKZCUO97505089 = -995719593;    int XzNVQKZCUO66286714 = -153140286;    int XzNVQKZCUO10736025 = -757522471;    int XzNVQKZCUO63950493 = -710131324;    int XzNVQKZCUO56996342 = -284422324;    int XzNVQKZCUO63357500 = -481073848;    int XzNVQKZCUO39283439 = -322514512;    int XzNVQKZCUO70174492 = -325694108;    int XzNVQKZCUO77691938 = -925285282;    int XzNVQKZCUO17733424 = -447461747;    int XzNVQKZCUO64520830 = -450185914;    int XzNVQKZCUO81390692 = -782556973;    int XzNVQKZCUO44871319 = -957480496;    int XzNVQKZCUO6690220 = -452458472;    int XzNVQKZCUO99514374 = -428842229;    int XzNVQKZCUO93423160 = -422598526;    int XzNVQKZCUO48957710 = 55033956;    int XzNVQKZCUO49207795 = -148841556;    int XzNVQKZCUO53966991 = -498546130;    int XzNVQKZCUO74103724 = -22333259;    int XzNVQKZCUO98531685 = -420351088;    int XzNVQKZCUO43833734 = -734918339;    int XzNVQKZCUO66507537 = -862697469;    int XzNVQKZCUO21410426 = -211514220;    int XzNVQKZCUO16100630 = -210167585;    int XzNVQKZCUO33695797 = -57094150;    int XzNVQKZCUO22629782 = -739557148;    int XzNVQKZCUO52746016 = -864190559;    int XzNVQKZCUO26378553 = -963956053;    int XzNVQKZCUO95836280 = -567978385;    int XzNVQKZCUO79785500 = -799978921;    int XzNVQKZCUO15692004 = -500617193;    int XzNVQKZCUO63752552 = -653899912;    int XzNVQKZCUO59539880 = 81726565;    int XzNVQKZCUO94340524 = 74526563;    int XzNVQKZCUO63745302 = 38288511;    int XzNVQKZCUO98515022 = -271874102;    int XzNVQKZCUO36565475 = -292990977;    int XzNVQKZCUO80081013 = -984980788;    int XzNVQKZCUO95708993 = -19967795;    int XzNVQKZCUO10683228 = -37725069;    int XzNVQKZCUO62071945 = -616837715;    int XzNVQKZCUO40145477 = -45776830;    int XzNVQKZCUO86495908 = -751194963;    int XzNVQKZCUO35950237 = -2453227;    int XzNVQKZCUO18027269 = -513815646;    int XzNVQKZCUO62318685 = -941435840;    int XzNVQKZCUO34393107 = -571669803;    int XzNVQKZCUO62120770 = -946572620;    int XzNVQKZCUO59960654 = -972607448;    int XzNVQKZCUO59567339 = -689430108;    int XzNVQKZCUO30716371 = -550122384;    int XzNVQKZCUO19558534 = -860265852;    int XzNVQKZCUO70212744 = -343638914;    int XzNVQKZCUO86690490 = -621313397;    int XzNVQKZCUO53671356 = -160801254;    int XzNVQKZCUO99779176 = -290442817;    int XzNVQKZCUO89325598 = -446008251;    int XzNVQKZCUO47849864 = -399963739;    int XzNVQKZCUO23300546 = -127328174;    int XzNVQKZCUO40727718 = -741516701;    int XzNVQKZCUO86537423 = -458323953;    int XzNVQKZCUO43795940 = -361738056;    int XzNVQKZCUO81855657 = -257306897;    int XzNVQKZCUO37947924 = -647482827;    int XzNVQKZCUO48828826 = -949568721;    int XzNVQKZCUO17638141 = -28657061;    int XzNVQKZCUO85331438 = -939207062;    int XzNVQKZCUO12349695 = -606540203;     XzNVQKZCUO15518592 = XzNVQKZCUO40885801;     XzNVQKZCUO40885801 = XzNVQKZCUO63713107;     XzNVQKZCUO63713107 = XzNVQKZCUO14381161;     XzNVQKZCUO14381161 = XzNVQKZCUO79762367;     XzNVQKZCUO79762367 = XzNVQKZCUO4361330;     XzNVQKZCUO4361330 = XzNVQKZCUO59709666;     XzNVQKZCUO59709666 = XzNVQKZCUO89217528;     XzNVQKZCUO89217528 = XzNVQKZCUO8069021;     XzNVQKZCUO8069021 = XzNVQKZCUO44762056;     XzNVQKZCUO44762056 = XzNVQKZCUO91845614;     XzNVQKZCUO91845614 = XzNVQKZCUO30032016;     XzNVQKZCUO30032016 = XzNVQKZCUO9251048;     XzNVQKZCUO9251048 = XzNVQKZCUO515969;     XzNVQKZCUO515969 = XzNVQKZCUO37077356;     XzNVQKZCUO37077356 = XzNVQKZCUO59066493;     XzNVQKZCUO59066493 = XzNVQKZCUO49966667;     XzNVQKZCUO49966667 = XzNVQKZCUO32246437;     XzNVQKZCUO32246437 = XzNVQKZCUO17837415;     XzNVQKZCUO17837415 = XzNVQKZCUO4229333;     XzNVQKZCUO4229333 = XzNVQKZCUO471068;     XzNVQKZCUO471068 = XzNVQKZCUO99417961;     XzNVQKZCUO99417961 = XzNVQKZCUO7190005;     XzNVQKZCUO7190005 = XzNVQKZCUO41083326;     XzNVQKZCUO41083326 = XzNVQKZCUO61635145;     XzNVQKZCUO61635145 = XzNVQKZCUO53383814;     XzNVQKZCUO53383814 = XzNVQKZCUO8525049;     XzNVQKZCUO8525049 = XzNVQKZCUO79924166;     XzNVQKZCUO79924166 = XzNVQKZCUO73525524;     XzNVQKZCUO73525524 = XzNVQKZCUO44316468;     XzNVQKZCUO44316468 = XzNVQKZCUO85222175;     XzNVQKZCUO85222175 = XzNVQKZCUO97505089;     XzNVQKZCUO97505089 = XzNVQKZCUO66286714;     XzNVQKZCUO66286714 = XzNVQKZCUO10736025;     XzNVQKZCUO10736025 = XzNVQKZCUO63950493;     XzNVQKZCUO63950493 = XzNVQKZCUO56996342;     XzNVQKZCUO56996342 = XzNVQKZCUO63357500;     XzNVQKZCUO63357500 = XzNVQKZCUO39283439;     XzNVQKZCUO39283439 = XzNVQKZCUO70174492;     XzNVQKZCUO70174492 = XzNVQKZCUO77691938;     XzNVQKZCUO77691938 = XzNVQKZCUO17733424;     XzNVQKZCUO17733424 = XzNVQKZCUO64520830;     XzNVQKZCUO64520830 = XzNVQKZCUO81390692;     XzNVQKZCUO81390692 = XzNVQKZCUO44871319;     XzNVQKZCUO44871319 = XzNVQKZCUO6690220;     XzNVQKZCUO6690220 = XzNVQKZCUO99514374;     XzNVQKZCUO99514374 = XzNVQKZCUO93423160;     XzNVQKZCUO93423160 = XzNVQKZCUO48957710;     XzNVQKZCUO48957710 = XzNVQKZCUO49207795;     XzNVQKZCUO49207795 = XzNVQKZCUO53966991;     XzNVQKZCUO53966991 = XzNVQKZCUO74103724;     XzNVQKZCUO74103724 = XzNVQKZCUO98531685;     XzNVQKZCUO98531685 = XzNVQKZCUO43833734;     XzNVQKZCUO43833734 = XzNVQKZCUO66507537;     XzNVQKZCUO66507537 = XzNVQKZCUO21410426;     XzNVQKZCUO21410426 = XzNVQKZCUO16100630;     XzNVQKZCUO16100630 = XzNVQKZCUO33695797;     XzNVQKZCUO33695797 = XzNVQKZCUO22629782;     XzNVQKZCUO22629782 = XzNVQKZCUO52746016;     XzNVQKZCUO52746016 = XzNVQKZCUO26378553;     XzNVQKZCUO26378553 = XzNVQKZCUO95836280;     XzNVQKZCUO95836280 = XzNVQKZCUO79785500;     XzNVQKZCUO79785500 = XzNVQKZCUO15692004;     XzNVQKZCUO15692004 = XzNVQKZCUO63752552;     XzNVQKZCUO63752552 = XzNVQKZCUO59539880;     XzNVQKZCUO59539880 = XzNVQKZCUO94340524;     XzNVQKZCUO94340524 = XzNVQKZCUO63745302;     XzNVQKZCUO63745302 = XzNVQKZCUO98515022;     XzNVQKZCUO98515022 = XzNVQKZCUO36565475;     XzNVQKZCUO36565475 = XzNVQKZCUO80081013;     XzNVQKZCUO80081013 = XzNVQKZCUO95708993;     XzNVQKZCUO95708993 = XzNVQKZCUO10683228;     XzNVQKZCUO10683228 = XzNVQKZCUO62071945;     XzNVQKZCUO62071945 = XzNVQKZCUO40145477;     XzNVQKZCUO40145477 = XzNVQKZCUO86495908;     XzNVQKZCUO86495908 = XzNVQKZCUO35950237;     XzNVQKZCUO35950237 = XzNVQKZCUO18027269;     XzNVQKZCUO18027269 = XzNVQKZCUO62318685;     XzNVQKZCUO62318685 = XzNVQKZCUO34393107;     XzNVQKZCUO34393107 = XzNVQKZCUO62120770;     XzNVQKZCUO62120770 = XzNVQKZCUO59960654;     XzNVQKZCUO59960654 = XzNVQKZCUO59567339;     XzNVQKZCUO59567339 = XzNVQKZCUO30716371;     XzNVQKZCUO30716371 = XzNVQKZCUO19558534;     XzNVQKZCUO19558534 = XzNVQKZCUO70212744;     XzNVQKZCUO70212744 = XzNVQKZCUO86690490;     XzNVQKZCUO86690490 = XzNVQKZCUO53671356;     XzNVQKZCUO53671356 = XzNVQKZCUO99779176;     XzNVQKZCUO99779176 = XzNVQKZCUO89325598;     XzNVQKZCUO89325598 = XzNVQKZCUO47849864;     XzNVQKZCUO47849864 = XzNVQKZCUO23300546;     XzNVQKZCUO23300546 = XzNVQKZCUO40727718;     XzNVQKZCUO40727718 = XzNVQKZCUO86537423;     XzNVQKZCUO86537423 = XzNVQKZCUO43795940;     XzNVQKZCUO43795940 = XzNVQKZCUO81855657;     XzNVQKZCUO81855657 = XzNVQKZCUO37947924;     XzNVQKZCUO37947924 = XzNVQKZCUO48828826;     XzNVQKZCUO48828826 = XzNVQKZCUO17638141;     XzNVQKZCUO17638141 = XzNVQKZCUO85331438;     XzNVQKZCUO85331438 = XzNVQKZCUO12349695;     XzNVQKZCUO12349695 = XzNVQKZCUO15518592;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xPwpvyaopc58526468() {     int wgzxAqmmxm37905709 = -240944166;    int wgzxAqmmxm25233302 = 95446459;    int wgzxAqmmxm92482386 = -3339266;    int wgzxAqmmxm29387850 = -246012939;    int wgzxAqmmxm77936621 = -950219027;    int wgzxAqmmxm19244447 = -162982914;    int wgzxAqmmxm51865405 = -420930165;    int wgzxAqmmxm64851387 = -330025819;    int wgzxAqmmxm5232152 = -4068123;    int wgzxAqmmxm36525641 = -902679252;    int wgzxAqmmxm29387474 = 82182569;    int wgzxAqmmxm85934965 = -465597067;    int wgzxAqmmxm76259228 = -648113892;    int wgzxAqmmxm43130721 = -530831058;    int wgzxAqmmxm73812632 = -996880824;    int wgzxAqmmxm3322407 = -982709185;    int wgzxAqmmxm99790018 = -737519400;    int wgzxAqmmxm38222854 = -394274513;    int wgzxAqmmxm59119046 = -106164237;    int wgzxAqmmxm82745615 = -451291163;    int wgzxAqmmxm1354175 = -544126449;    int wgzxAqmmxm63233180 = -801138932;    int wgzxAqmmxm10483328 = 25003072;    int wgzxAqmmxm67583113 = -588121131;    int wgzxAqmmxm97175392 = 3026027;    int wgzxAqmmxm41686809 = -839100198;    int wgzxAqmmxm55614469 = -491982644;    int wgzxAqmmxm85835616 = -587418392;    int wgzxAqmmxm80418085 = -703069101;    int wgzxAqmmxm53473648 = -386268039;    int wgzxAqmmxm29512981 = -122980636;    int wgzxAqmmxm55046280 = -547790948;    int wgzxAqmmxm47325384 = -682483781;    int wgzxAqmmxm70124712 = 18350679;    int wgzxAqmmxm40179858 = -773887746;    int wgzxAqmmxm98689211 = -599817700;    int wgzxAqmmxm56797212 = -214021800;    int wgzxAqmmxm92909540 = -190139984;    int wgzxAqmmxm18482624 = -726740140;    int wgzxAqmmxm97877652 = 38490688;    int wgzxAqmmxm46490795 = -950639413;    int wgzxAqmmxm53029222 = -208094605;    int wgzxAqmmxm86940884 = -670087557;    int wgzxAqmmxm44315438 = -954844145;    int wgzxAqmmxm74673486 = -261601949;    int wgzxAqmmxm7614657 = -611071325;    int wgzxAqmmxm5720558 = 98991221;    int wgzxAqmmxm93556384 = -801501035;    int wgzxAqmmxm43667700 = -98558254;    int wgzxAqmmxm51050737 = -640566389;    int wgzxAqmmxm82972941 = -377409536;    int wgzxAqmmxm72382942 = -851170370;    int wgzxAqmmxm22148137 = -406829569;    int wgzxAqmmxm26794082 = -38378278;    int wgzxAqmmxm57172294 = -352218335;    int wgzxAqmmxm74672529 = -439805235;    int wgzxAqmmxm14749974 = -929556613;    int wgzxAqmmxm24899273 = -415218135;    int wgzxAqmmxm32212458 = -149038966;    int wgzxAqmmxm36249813 = -11118830;    int wgzxAqmmxm63629977 = -671000271;    int wgzxAqmmxm66029789 = -833511773;    int wgzxAqmmxm84433302 = -626956718;    int wgzxAqmmxm51758503 = -617800085;    int wgzxAqmmxm7012660 = -679698616;    int wgzxAqmmxm74341193 = -370026483;    int wgzxAqmmxm38609581 = -783113287;    int wgzxAqmmxm6134516 = -566464572;    int wgzxAqmmxm2950863 = -756943312;    int wgzxAqmmxm75123421 = -297063125;    int wgzxAqmmxm46525194 = -668687386;    int wgzxAqmmxm6880479 = -447379417;    int wgzxAqmmxm19740230 = -667534374;    int wgzxAqmmxm61241393 = -44654926;    int wgzxAqmmxm36254821 = -500651750;    int wgzxAqmmxm48324953 = -236031844;    int wgzxAqmmxm76292295 = -31051376;    int wgzxAqmmxm66167889 = -20152783;    int wgzxAqmmxm92909627 = -226519183;    int wgzxAqmmxm89560735 = -385902649;    int wgzxAqmmxm35966251 = -838091419;    int wgzxAqmmxm62058085 = -690481610;    int wgzxAqmmxm42167916 = -388860139;    int wgzxAqmmxm29367349 = 37497287;    int wgzxAqmmxm70500707 = 91141497;    int wgzxAqmmxm57130038 = -271810267;    int wgzxAqmmxm32898144 = -40961379;    int wgzxAqmmxm20531302 = -544105503;    int wgzxAqmmxm12952419 = -629430986;    int wgzxAqmmxm65507329 = -234082512;    int wgzxAqmmxm83939237 = -670261087;    int wgzxAqmmxm31897939 = -798803665;    int wgzxAqmmxm60697082 = 58898982;    int wgzxAqmmxm82232811 = -615621310;    int wgzxAqmmxm34247675 = -290509041;    int wgzxAqmmxm80461006 = -17127640;    int wgzxAqmmxm68595920 = -581137887;    int wgzxAqmmxm35182381 = 47712528;    int wgzxAqmmxm37302778 = -175145530;    int wgzxAqmmxm332293 = -240944166;     wgzxAqmmxm37905709 = wgzxAqmmxm25233302;     wgzxAqmmxm25233302 = wgzxAqmmxm92482386;     wgzxAqmmxm92482386 = wgzxAqmmxm29387850;     wgzxAqmmxm29387850 = wgzxAqmmxm77936621;     wgzxAqmmxm77936621 = wgzxAqmmxm19244447;     wgzxAqmmxm19244447 = wgzxAqmmxm51865405;     wgzxAqmmxm51865405 = wgzxAqmmxm64851387;     wgzxAqmmxm64851387 = wgzxAqmmxm5232152;     wgzxAqmmxm5232152 = wgzxAqmmxm36525641;     wgzxAqmmxm36525641 = wgzxAqmmxm29387474;     wgzxAqmmxm29387474 = wgzxAqmmxm85934965;     wgzxAqmmxm85934965 = wgzxAqmmxm76259228;     wgzxAqmmxm76259228 = wgzxAqmmxm43130721;     wgzxAqmmxm43130721 = wgzxAqmmxm73812632;     wgzxAqmmxm73812632 = wgzxAqmmxm3322407;     wgzxAqmmxm3322407 = wgzxAqmmxm99790018;     wgzxAqmmxm99790018 = wgzxAqmmxm38222854;     wgzxAqmmxm38222854 = wgzxAqmmxm59119046;     wgzxAqmmxm59119046 = wgzxAqmmxm82745615;     wgzxAqmmxm82745615 = wgzxAqmmxm1354175;     wgzxAqmmxm1354175 = wgzxAqmmxm63233180;     wgzxAqmmxm63233180 = wgzxAqmmxm10483328;     wgzxAqmmxm10483328 = wgzxAqmmxm67583113;     wgzxAqmmxm67583113 = wgzxAqmmxm97175392;     wgzxAqmmxm97175392 = wgzxAqmmxm41686809;     wgzxAqmmxm41686809 = wgzxAqmmxm55614469;     wgzxAqmmxm55614469 = wgzxAqmmxm85835616;     wgzxAqmmxm85835616 = wgzxAqmmxm80418085;     wgzxAqmmxm80418085 = wgzxAqmmxm53473648;     wgzxAqmmxm53473648 = wgzxAqmmxm29512981;     wgzxAqmmxm29512981 = wgzxAqmmxm55046280;     wgzxAqmmxm55046280 = wgzxAqmmxm47325384;     wgzxAqmmxm47325384 = wgzxAqmmxm70124712;     wgzxAqmmxm70124712 = wgzxAqmmxm40179858;     wgzxAqmmxm40179858 = wgzxAqmmxm98689211;     wgzxAqmmxm98689211 = wgzxAqmmxm56797212;     wgzxAqmmxm56797212 = wgzxAqmmxm92909540;     wgzxAqmmxm92909540 = wgzxAqmmxm18482624;     wgzxAqmmxm18482624 = wgzxAqmmxm97877652;     wgzxAqmmxm97877652 = wgzxAqmmxm46490795;     wgzxAqmmxm46490795 = wgzxAqmmxm53029222;     wgzxAqmmxm53029222 = wgzxAqmmxm86940884;     wgzxAqmmxm86940884 = wgzxAqmmxm44315438;     wgzxAqmmxm44315438 = wgzxAqmmxm74673486;     wgzxAqmmxm74673486 = wgzxAqmmxm7614657;     wgzxAqmmxm7614657 = wgzxAqmmxm5720558;     wgzxAqmmxm5720558 = wgzxAqmmxm93556384;     wgzxAqmmxm93556384 = wgzxAqmmxm43667700;     wgzxAqmmxm43667700 = wgzxAqmmxm51050737;     wgzxAqmmxm51050737 = wgzxAqmmxm82972941;     wgzxAqmmxm82972941 = wgzxAqmmxm72382942;     wgzxAqmmxm72382942 = wgzxAqmmxm22148137;     wgzxAqmmxm22148137 = wgzxAqmmxm26794082;     wgzxAqmmxm26794082 = wgzxAqmmxm57172294;     wgzxAqmmxm57172294 = wgzxAqmmxm74672529;     wgzxAqmmxm74672529 = wgzxAqmmxm14749974;     wgzxAqmmxm14749974 = wgzxAqmmxm24899273;     wgzxAqmmxm24899273 = wgzxAqmmxm32212458;     wgzxAqmmxm32212458 = wgzxAqmmxm36249813;     wgzxAqmmxm36249813 = wgzxAqmmxm63629977;     wgzxAqmmxm63629977 = wgzxAqmmxm66029789;     wgzxAqmmxm66029789 = wgzxAqmmxm84433302;     wgzxAqmmxm84433302 = wgzxAqmmxm51758503;     wgzxAqmmxm51758503 = wgzxAqmmxm7012660;     wgzxAqmmxm7012660 = wgzxAqmmxm74341193;     wgzxAqmmxm74341193 = wgzxAqmmxm38609581;     wgzxAqmmxm38609581 = wgzxAqmmxm6134516;     wgzxAqmmxm6134516 = wgzxAqmmxm2950863;     wgzxAqmmxm2950863 = wgzxAqmmxm75123421;     wgzxAqmmxm75123421 = wgzxAqmmxm46525194;     wgzxAqmmxm46525194 = wgzxAqmmxm6880479;     wgzxAqmmxm6880479 = wgzxAqmmxm19740230;     wgzxAqmmxm19740230 = wgzxAqmmxm61241393;     wgzxAqmmxm61241393 = wgzxAqmmxm36254821;     wgzxAqmmxm36254821 = wgzxAqmmxm48324953;     wgzxAqmmxm48324953 = wgzxAqmmxm76292295;     wgzxAqmmxm76292295 = wgzxAqmmxm66167889;     wgzxAqmmxm66167889 = wgzxAqmmxm92909627;     wgzxAqmmxm92909627 = wgzxAqmmxm89560735;     wgzxAqmmxm89560735 = wgzxAqmmxm35966251;     wgzxAqmmxm35966251 = wgzxAqmmxm62058085;     wgzxAqmmxm62058085 = wgzxAqmmxm42167916;     wgzxAqmmxm42167916 = wgzxAqmmxm29367349;     wgzxAqmmxm29367349 = wgzxAqmmxm70500707;     wgzxAqmmxm70500707 = wgzxAqmmxm57130038;     wgzxAqmmxm57130038 = wgzxAqmmxm32898144;     wgzxAqmmxm32898144 = wgzxAqmmxm20531302;     wgzxAqmmxm20531302 = wgzxAqmmxm12952419;     wgzxAqmmxm12952419 = wgzxAqmmxm65507329;     wgzxAqmmxm65507329 = wgzxAqmmxm83939237;     wgzxAqmmxm83939237 = wgzxAqmmxm31897939;     wgzxAqmmxm31897939 = wgzxAqmmxm60697082;     wgzxAqmmxm60697082 = wgzxAqmmxm82232811;     wgzxAqmmxm82232811 = wgzxAqmmxm34247675;     wgzxAqmmxm34247675 = wgzxAqmmxm80461006;     wgzxAqmmxm80461006 = wgzxAqmmxm68595920;     wgzxAqmmxm68595920 = wgzxAqmmxm35182381;     wgzxAqmmxm35182381 = wgzxAqmmxm37302778;     wgzxAqmmxm37302778 = wgzxAqmmxm332293;     wgzxAqmmxm332293 = wgzxAqmmxm37905709;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void QGdgLGxdcM95171442() {     int OEYCPSpJOj60292826 = -975348129;    int OEYCPSpJOj9580802 = -953096596;    int OEYCPSpJOj21251665 = -242993110;    int OEYCPSpJOj44394538 = -252420471;    int OEYCPSpJOj76110875 = -441276029;    int OEYCPSpJOj34127564 = -23591292;    int OEYCPSpJOj44021144 = -242917471;    int OEYCPSpJOj40485246 = -800622463;    int OEYCPSpJOj2395283 = -988264162;    int OEYCPSpJOj28289226 = -645420585;    int OEYCPSpJOj66929334 = 85558169;    int OEYCPSpJOj41837914 = -716342361;    int OEYCPSpJOj43267409 = -166831212;    int OEYCPSpJOj85745473 = 41460186;    int OEYCPSpJOj10547910 = -624358538;    int OEYCPSpJOj47578319 = -264376727;    int OEYCPSpJOj49613370 = 85200780;    int OEYCPSpJOj44199270 = -846017204;    int OEYCPSpJOj400677 = -241266363;    int OEYCPSpJOj61261899 = -703925616;    int OEYCPSpJOj2237282 = -535613757;    int OEYCPSpJOj27048399 = -205905245;    int OEYCPSpJOj13776651 = -151077520;    int OEYCPSpJOj94082900 = -52113989;    int OEYCPSpJOj32715639 = -718533097;    int OEYCPSpJOj29989803 = -182994422;    int OEYCPSpJOj2703890 = -249569137;    int OEYCPSpJOj91747066 = -375872846;    int OEYCPSpJOj87310646 = 52673780;    int OEYCPSpJOj62630827 = -306563905;    int OEYCPSpJOj73803786 = -204296788;    int OEYCPSpJOj12587472 = -99862302;    int OEYCPSpJOj28364055 = -111827276;    int OEYCPSpJOj29513400 = -305776171;    int OEYCPSpJOj16409223 = -837644169;    int OEYCPSpJOj40382080 = -915213076;    int OEYCPSpJOj50236924 = 53030249;    int OEYCPSpJOj46535641 = -57765456;    int OEYCPSpJOj66790755 = -27786172;    int OEYCPSpJOj18063368 = -97733341;    int OEYCPSpJOj75248165 = -353817079;    int OEYCPSpJOj41537613 = 33996704;    int OEYCPSpJOj92491076 = -557618140;    int OEYCPSpJOj43759558 = -952207794;    int OEYCPSpJOj42656753 = -70745427;    int OEYCPSpJOj15714939 = -793300420;    int OEYCPSpJOj18017955 = -479419032;    int OEYCPSpJOj38155059 = -558036026;    int OEYCPSpJOj38127605 = -48274952;    int OEYCPSpJOj48134483 = -782586648;    int OEYCPSpJOj91842157 = -732485813;    int OEYCPSpJOj46234200 = -181989652;    int OEYCPSpJOj462541 = -78740800;    int OEYCPSpJOj87080627 = -314059087;    int OEYCPSpJOj92934161 = -492922451;    int OEYCPSpJOj33244428 = -669442885;    int OEYCPSpJOj95804151 = -702019076;    int OEYCPSpJOj27168765 = -90879122;    int OEYCPSpJOj11678899 = -533887374;    int OEYCPSpJOj46121072 = -158281608;    int OEYCPSpJOj31423674 = -774022156;    int OEYCPSpJOj52274078 = -867044626;    int OEYCPSpJOj53174600 = -753296243;    int OEYCPSpJOj39764455 = -581700257;    int OEYCPSpJOj54485439 = -341123797;    int OEYCPSpJOj54341862 = -814579530;    int OEYCPSpJOj13473860 = -504515086;    int OEYCPSpJOj13754010 = -861055041;    int OEYCPSpJOj69336250 = -120895646;    int OEYCPSpJOj70165829 = -709145462;    int OEYCPSpJOj97341394 = -217406976;    int OEYCPSpJOj3077730 = -857033765;    int OEYCPSpJOj77408515 = -718231033;    int OEYCPSpJOj82337309 = -43533022;    int OEYCPSpJOj86013733 = -250108538;    int OEYCPSpJOj60699669 = -469610461;    int OEYCPSpJOj34557322 = -648287106;    int OEYCPSpJOj70017093 = -198869727;    int OEYCPSpJOj51426147 = -981368562;    int OEYCPSpJOj17000701 = -925232678;    int OEYCPSpJOj11971848 = -703575390;    int OEYCPSpJOj64548831 = -691533112;    int OEYCPSpJOj53619461 = -227597894;    int OEYCPSpJOj39176164 = -164739573;    int OEYCPSpJOj70788670 = -574078093;    int OEYCPSpJOj27569587 = 77692863;    int OEYCPSpJOj12124931 = 78878497;    int OEYCPSpJOj41283427 = -797768189;    int OEYCPSpJOj36579238 = -812853721;    int OEYCPSpJOj83164794 = -68201285;    int OEYCPSpJOj44577929 = -113194001;    int OEYCPSpJOj23068160 = -856090630;    int OEYCPSpJOj34856742 = -523878083;    int OEYCPSpJOj20669683 = -869504564;    int OEYCPSpJOj86639693 = -323711186;    int OEYCPSpJOj22974088 = -486772454;    int OEYCPSpJOj88363013 = -212707053;    int OEYCPSpJOj52726622 = -975917884;    int OEYCPSpJOj89274118 = -511083998;    int OEYCPSpJOj88314890 = -975348129;     OEYCPSpJOj60292826 = OEYCPSpJOj9580802;     OEYCPSpJOj9580802 = OEYCPSpJOj21251665;     OEYCPSpJOj21251665 = OEYCPSpJOj44394538;     OEYCPSpJOj44394538 = OEYCPSpJOj76110875;     OEYCPSpJOj76110875 = OEYCPSpJOj34127564;     OEYCPSpJOj34127564 = OEYCPSpJOj44021144;     OEYCPSpJOj44021144 = OEYCPSpJOj40485246;     OEYCPSpJOj40485246 = OEYCPSpJOj2395283;     OEYCPSpJOj2395283 = OEYCPSpJOj28289226;     OEYCPSpJOj28289226 = OEYCPSpJOj66929334;     OEYCPSpJOj66929334 = OEYCPSpJOj41837914;     OEYCPSpJOj41837914 = OEYCPSpJOj43267409;     OEYCPSpJOj43267409 = OEYCPSpJOj85745473;     OEYCPSpJOj85745473 = OEYCPSpJOj10547910;     OEYCPSpJOj10547910 = OEYCPSpJOj47578319;     OEYCPSpJOj47578319 = OEYCPSpJOj49613370;     OEYCPSpJOj49613370 = OEYCPSpJOj44199270;     OEYCPSpJOj44199270 = OEYCPSpJOj400677;     OEYCPSpJOj400677 = OEYCPSpJOj61261899;     OEYCPSpJOj61261899 = OEYCPSpJOj2237282;     OEYCPSpJOj2237282 = OEYCPSpJOj27048399;     OEYCPSpJOj27048399 = OEYCPSpJOj13776651;     OEYCPSpJOj13776651 = OEYCPSpJOj94082900;     OEYCPSpJOj94082900 = OEYCPSpJOj32715639;     OEYCPSpJOj32715639 = OEYCPSpJOj29989803;     OEYCPSpJOj29989803 = OEYCPSpJOj2703890;     OEYCPSpJOj2703890 = OEYCPSpJOj91747066;     OEYCPSpJOj91747066 = OEYCPSpJOj87310646;     OEYCPSpJOj87310646 = OEYCPSpJOj62630827;     OEYCPSpJOj62630827 = OEYCPSpJOj73803786;     OEYCPSpJOj73803786 = OEYCPSpJOj12587472;     OEYCPSpJOj12587472 = OEYCPSpJOj28364055;     OEYCPSpJOj28364055 = OEYCPSpJOj29513400;     OEYCPSpJOj29513400 = OEYCPSpJOj16409223;     OEYCPSpJOj16409223 = OEYCPSpJOj40382080;     OEYCPSpJOj40382080 = OEYCPSpJOj50236924;     OEYCPSpJOj50236924 = OEYCPSpJOj46535641;     OEYCPSpJOj46535641 = OEYCPSpJOj66790755;     OEYCPSpJOj66790755 = OEYCPSpJOj18063368;     OEYCPSpJOj18063368 = OEYCPSpJOj75248165;     OEYCPSpJOj75248165 = OEYCPSpJOj41537613;     OEYCPSpJOj41537613 = OEYCPSpJOj92491076;     OEYCPSpJOj92491076 = OEYCPSpJOj43759558;     OEYCPSpJOj43759558 = OEYCPSpJOj42656753;     OEYCPSpJOj42656753 = OEYCPSpJOj15714939;     OEYCPSpJOj15714939 = OEYCPSpJOj18017955;     OEYCPSpJOj18017955 = OEYCPSpJOj38155059;     OEYCPSpJOj38155059 = OEYCPSpJOj38127605;     OEYCPSpJOj38127605 = OEYCPSpJOj48134483;     OEYCPSpJOj48134483 = OEYCPSpJOj91842157;     OEYCPSpJOj91842157 = OEYCPSpJOj46234200;     OEYCPSpJOj46234200 = OEYCPSpJOj462541;     OEYCPSpJOj462541 = OEYCPSpJOj87080627;     OEYCPSpJOj87080627 = OEYCPSpJOj92934161;     OEYCPSpJOj92934161 = OEYCPSpJOj33244428;     OEYCPSpJOj33244428 = OEYCPSpJOj95804151;     OEYCPSpJOj95804151 = OEYCPSpJOj27168765;     OEYCPSpJOj27168765 = OEYCPSpJOj11678899;     OEYCPSpJOj11678899 = OEYCPSpJOj46121072;     OEYCPSpJOj46121072 = OEYCPSpJOj31423674;     OEYCPSpJOj31423674 = OEYCPSpJOj52274078;     OEYCPSpJOj52274078 = OEYCPSpJOj53174600;     OEYCPSpJOj53174600 = OEYCPSpJOj39764455;     OEYCPSpJOj39764455 = OEYCPSpJOj54485439;     OEYCPSpJOj54485439 = OEYCPSpJOj54341862;     OEYCPSpJOj54341862 = OEYCPSpJOj13473860;     OEYCPSpJOj13473860 = OEYCPSpJOj13754010;     OEYCPSpJOj13754010 = OEYCPSpJOj69336250;     OEYCPSpJOj69336250 = OEYCPSpJOj70165829;     OEYCPSpJOj70165829 = OEYCPSpJOj97341394;     OEYCPSpJOj97341394 = OEYCPSpJOj3077730;     OEYCPSpJOj3077730 = OEYCPSpJOj77408515;     OEYCPSpJOj77408515 = OEYCPSpJOj82337309;     OEYCPSpJOj82337309 = OEYCPSpJOj86013733;     OEYCPSpJOj86013733 = OEYCPSpJOj60699669;     OEYCPSpJOj60699669 = OEYCPSpJOj34557322;     OEYCPSpJOj34557322 = OEYCPSpJOj70017093;     OEYCPSpJOj70017093 = OEYCPSpJOj51426147;     OEYCPSpJOj51426147 = OEYCPSpJOj17000701;     OEYCPSpJOj17000701 = OEYCPSpJOj11971848;     OEYCPSpJOj11971848 = OEYCPSpJOj64548831;     OEYCPSpJOj64548831 = OEYCPSpJOj53619461;     OEYCPSpJOj53619461 = OEYCPSpJOj39176164;     OEYCPSpJOj39176164 = OEYCPSpJOj70788670;     OEYCPSpJOj70788670 = OEYCPSpJOj27569587;     OEYCPSpJOj27569587 = OEYCPSpJOj12124931;     OEYCPSpJOj12124931 = OEYCPSpJOj41283427;     OEYCPSpJOj41283427 = OEYCPSpJOj36579238;     OEYCPSpJOj36579238 = OEYCPSpJOj83164794;     OEYCPSpJOj83164794 = OEYCPSpJOj44577929;     OEYCPSpJOj44577929 = OEYCPSpJOj23068160;     OEYCPSpJOj23068160 = OEYCPSpJOj34856742;     OEYCPSpJOj34856742 = OEYCPSpJOj20669683;     OEYCPSpJOj20669683 = OEYCPSpJOj86639693;     OEYCPSpJOj86639693 = OEYCPSpJOj22974088;     OEYCPSpJOj22974088 = OEYCPSpJOj88363013;     OEYCPSpJOj88363013 = OEYCPSpJOj52726622;     OEYCPSpJOj52726622 = OEYCPSpJOj89274118;     OEYCPSpJOj89274118 = OEYCPSpJOj88314890;     OEYCPSpJOj88314890 = OEYCPSpJOj60292826;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void vvbmiSyqYw31816418() {     int GOZygfjueg82679943 = -609752093;    int GOZygfjueg93928302 = -901639650;    int GOZygfjueg50020943 = -482646954;    int GOZygfjueg59401226 = -258828002;    int GOZygfjueg74285129 = 67666970;    int GOZygfjueg49010680 = -984199670;    int GOZygfjueg36176883 = -64904777;    int GOZygfjueg16119105 = -171219107;    int GOZygfjueg99558413 = -872460201;    int GOZygfjueg20052810 = -388161918;    int GOZygfjueg4471194 = 88933768;    int GOZygfjueg97740863 = -967087654;    int GOZygfjueg10275590 = -785548532;    int GOZygfjueg28360225 = -486248571;    int GOZygfjueg47283187 = -251836251;    int GOZygfjueg91834231 = -646044270;    int GOZygfjueg99436722 = -192079040;    int GOZygfjueg50175686 = -197759895;    int GOZygfjueg41682308 = -376368488;    int GOZygfjueg39778182 = -956560070;    int GOZygfjueg3120389 = -527101066;    int GOZygfjueg90863616 = -710671559;    int GOZygfjueg17069974 = -327158112;    int GOZygfjueg20582688 = -616106846;    int GOZygfjueg68255886 = -340092222;    int GOZygfjueg18292798 = -626888645;    int GOZygfjueg49793310 = -7155630;    int GOZygfjueg97658516 = -164327299;    int GOZygfjueg94203207 = -291583339;    int GOZygfjueg71788007 = -226859772;    int GOZygfjueg18094591 = -285612940;    int GOZygfjueg70128662 = -751933657;    int GOZygfjueg9402725 = -641170770;    int GOZygfjueg88902086 = -629903021;    int GOZygfjueg92638587 = -901400591;    int GOZygfjueg82074949 = -130608453;    int GOZygfjueg43676637 = -779917703;    int GOZygfjueg161742 = 74609072;    int GOZygfjueg15098887 = -428832203;    int GOZygfjueg38249082 = -233957370;    int GOZygfjueg4005537 = -856994745;    int GOZygfjueg30046004 = -823911987;    int GOZygfjueg98041268 = -445148724;    int GOZygfjueg43203677 = -949571443;    int GOZygfjueg10640020 = -979888905;    int GOZygfjueg23815221 = -975529515;    int GOZygfjueg30315353 = 42170715;    int GOZygfjueg82753733 = -314571017;    int GOZygfjueg32587510 = 2008350;    int GOZygfjueg45218228 = -924606907;    int GOZygfjueg711374 = 12437910;    int GOZygfjueg20085457 = -612808934;    int GOZygfjueg78776943 = -850652030;    int GOZygfjueg47367173 = -589739896;    int GOZygfjueg28696029 = -633626566;    int GOZygfjueg91816326 = -899080534;    int GOZygfjueg76858328 = -474481538;    int GOZygfjueg29438256 = -866540109;    int GOZygfjueg91145340 = -918735781;    int GOZygfjueg55992332 = -305444385;    int GOZygfjueg99217370 = -877044041;    int GOZygfjueg38518367 = -900577479;    int GOZygfjueg21915898 = -879635769;    int GOZygfjueg27770407 = -545600430;    int GOZygfjueg1958219 = -2548978;    int GOZygfjueg34342531 = -159132576;    int GOZygfjueg88338138 = -225916884;    int GOZygfjueg21373503 = -55645511;    int GOZygfjueg35721638 = -584847981;    int GOZygfjueg65208237 = -21227798;    int GOZygfjueg48157595 = -866126567;    int GOZygfjueg99274980 = -166688113;    int GOZygfjueg35076800 = -768927692;    int GOZygfjueg3433226 = -42411118;    int GOZygfjueg35772646 = 434675;    int GOZygfjueg73074385 = -703189079;    int GOZygfjueg92822347 = -165522835;    int GOZygfjueg73866297 = -377586670;    int GOZygfjueg9942668 = -636217942;    int GOZygfjueg44440665 = -364562707;    int GOZygfjueg87977445 = -569059361;    int GOZygfjueg67039577 = -692584614;    int GOZygfjueg65071006 = -66335649;    int GOZygfjueg48984979 = -366976433;    int GOZygfjueg71076633 = -139297683;    int GOZygfjueg98009134 = -672804007;    int GOZygfjueg91351718 = -901281628;    int GOZygfjueg62035552 = 48569125;    int GOZygfjueg60206058 = -996276456;    int GOZygfjueg822261 = 97679943;    int GOZygfjueg5216621 = -656126915;    int GOZygfjueg14238381 = -913377595;    int GOZygfjueg9016402 = -6655147;    int GOZygfjueg59106554 = -23387819;    int GOZygfjueg39031711 = -356913330;    int GOZygfjueg65487170 = -956417267;    int GOZygfjueg8130107 = -944276219;    int GOZygfjueg70270862 = -899548295;    int GOZygfjueg41245458 = -847022466;    int GOZygfjueg76297488 = -609752093;     GOZygfjueg82679943 = GOZygfjueg93928302;     GOZygfjueg93928302 = GOZygfjueg50020943;     GOZygfjueg50020943 = GOZygfjueg59401226;     GOZygfjueg59401226 = GOZygfjueg74285129;     GOZygfjueg74285129 = GOZygfjueg49010680;     GOZygfjueg49010680 = GOZygfjueg36176883;     GOZygfjueg36176883 = GOZygfjueg16119105;     GOZygfjueg16119105 = GOZygfjueg99558413;     GOZygfjueg99558413 = GOZygfjueg20052810;     GOZygfjueg20052810 = GOZygfjueg4471194;     GOZygfjueg4471194 = GOZygfjueg97740863;     GOZygfjueg97740863 = GOZygfjueg10275590;     GOZygfjueg10275590 = GOZygfjueg28360225;     GOZygfjueg28360225 = GOZygfjueg47283187;     GOZygfjueg47283187 = GOZygfjueg91834231;     GOZygfjueg91834231 = GOZygfjueg99436722;     GOZygfjueg99436722 = GOZygfjueg50175686;     GOZygfjueg50175686 = GOZygfjueg41682308;     GOZygfjueg41682308 = GOZygfjueg39778182;     GOZygfjueg39778182 = GOZygfjueg3120389;     GOZygfjueg3120389 = GOZygfjueg90863616;     GOZygfjueg90863616 = GOZygfjueg17069974;     GOZygfjueg17069974 = GOZygfjueg20582688;     GOZygfjueg20582688 = GOZygfjueg68255886;     GOZygfjueg68255886 = GOZygfjueg18292798;     GOZygfjueg18292798 = GOZygfjueg49793310;     GOZygfjueg49793310 = GOZygfjueg97658516;     GOZygfjueg97658516 = GOZygfjueg94203207;     GOZygfjueg94203207 = GOZygfjueg71788007;     GOZygfjueg71788007 = GOZygfjueg18094591;     GOZygfjueg18094591 = GOZygfjueg70128662;     GOZygfjueg70128662 = GOZygfjueg9402725;     GOZygfjueg9402725 = GOZygfjueg88902086;     GOZygfjueg88902086 = GOZygfjueg92638587;     GOZygfjueg92638587 = GOZygfjueg82074949;     GOZygfjueg82074949 = GOZygfjueg43676637;     GOZygfjueg43676637 = GOZygfjueg161742;     GOZygfjueg161742 = GOZygfjueg15098887;     GOZygfjueg15098887 = GOZygfjueg38249082;     GOZygfjueg38249082 = GOZygfjueg4005537;     GOZygfjueg4005537 = GOZygfjueg30046004;     GOZygfjueg30046004 = GOZygfjueg98041268;     GOZygfjueg98041268 = GOZygfjueg43203677;     GOZygfjueg43203677 = GOZygfjueg10640020;     GOZygfjueg10640020 = GOZygfjueg23815221;     GOZygfjueg23815221 = GOZygfjueg30315353;     GOZygfjueg30315353 = GOZygfjueg82753733;     GOZygfjueg82753733 = GOZygfjueg32587510;     GOZygfjueg32587510 = GOZygfjueg45218228;     GOZygfjueg45218228 = GOZygfjueg711374;     GOZygfjueg711374 = GOZygfjueg20085457;     GOZygfjueg20085457 = GOZygfjueg78776943;     GOZygfjueg78776943 = GOZygfjueg47367173;     GOZygfjueg47367173 = GOZygfjueg28696029;     GOZygfjueg28696029 = GOZygfjueg91816326;     GOZygfjueg91816326 = GOZygfjueg76858328;     GOZygfjueg76858328 = GOZygfjueg29438256;     GOZygfjueg29438256 = GOZygfjueg91145340;     GOZygfjueg91145340 = GOZygfjueg55992332;     GOZygfjueg55992332 = GOZygfjueg99217370;     GOZygfjueg99217370 = GOZygfjueg38518367;     GOZygfjueg38518367 = GOZygfjueg21915898;     GOZygfjueg21915898 = GOZygfjueg27770407;     GOZygfjueg27770407 = GOZygfjueg1958219;     GOZygfjueg1958219 = GOZygfjueg34342531;     GOZygfjueg34342531 = GOZygfjueg88338138;     GOZygfjueg88338138 = GOZygfjueg21373503;     GOZygfjueg21373503 = GOZygfjueg35721638;     GOZygfjueg35721638 = GOZygfjueg65208237;     GOZygfjueg65208237 = GOZygfjueg48157595;     GOZygfjueg48157595 = GOZygfjueg99274980;     GOZygfjueg99274980 = GOZygfjueg35076800;     GOZygfjueg35076800 = GOZygfjueg3433226;     GOZygfjueg3433226 = GOZygfjueg35772646;     GOZygfjueg35772646 = GOZygfjueg73074385;     GOZygfjueg73074385 = GOZygfjueg92822347;     GOZygfjueg92822347 = GOZygfjueg73866297;     GOZygfjueg73866297 = GOZygfjueg9942668;     GOZygfjueg9942668 = GOZygfjueg44440665;     GOZygfjueg44440665 = GOZygfjueg87977445;     GOZygfjueg87977445 = GOZygfjueg67039577;     GOZygfjueg67039577 = GOZygfjueg65071006;     GOZygfjueg65071006 = GOZygfjueg48984979;     GOZygfjueg48984979 = GOZygfjueg71076633;     GOZygfjueg71076633 = GOZygfjueg98009134;     GOZygfjueg98009134 = GOZygfjueg91351718;     GOZygfjueg91351718 = GOZygfjueg62035552;     GOZygfjueg62035552 = GOZygfjueg60206058;     GOZygfjueg60206058 = GOZygfjueg822261;     GOZygfjueg822261 = GOZygfjueg5216621;     GOZygfjueg5216621 = GOZygfjueg14238381;     GOZygfjueg14238381 = GOZygfjueg9016402;     GOZygfjueg9016402 = GOZygfjueg59106554;     GOZygfjueg59106554 = GOZygfjueg39031711;     GOZygfjueg39031711 = GOZygfjueg65487170;     GOZygfjueg65487170 = GOZygfjueg8130107;     GOZygfjueg8130107 = GOZygfjueg70270862;     GOZygfjueg70270862 = GOZygfjueg41245458;     GOZygfjueg41245458 = GOZygfjueg76297488;     GOZygfjueg76297488 = GOZygfjueg82679943;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void UkrzEpofZG48612721() {     int IdBjdmcFqi50130215 = -823534156;    int IdBjdmcFqi19838335 = -487645790;    int IdBjdmcFqi76536571 = -790698029;    int IdBjdmcFqi61157202 = -908321445;    int IdBjdmcFqi28987752 = -494051752;    int IdBjdmcFqi54287000 = -241124602;    int IdBjdmcFqi25664341 = -805664820;    int IdBjdmcFqi68192245 = -460039621;    int IdBjdmcFqi58627388 = -760602559;    int IdBjdmcFqi13944230 = -465840514;    int IdBjdmcFqi80088962 = -711900712;    int IdBjdmcFqi71018566 = -957627015;    int IdBjdmcFqi96456836 = 89616219;    int IdBjdmcFqi40184404 = 86962229;    int IdBjdmcFqi33055650 = -121874435;    int IdBjdmcFqi48180015 = -877901747;    int IdBjdmcFqi71241164 = -600031481;    int IdBjdmcFqi16829959 = -870927231;    int IdBjdmcFqi63421134 = -744675831;    int IdBjdmcFqi51956589 = -983133678;    int IdBjdmcFqi97964697 = -920800920;    int IdBjdmcFqi3719627 = -831986190;    int IdBjdmcFqi36982506 = -706468665;    int IdBjdmcFqi1043024 = -854111891;    int IdBjdmcFqi77238726 = -833406819;    int IdBjdmcFqi65991275 = -458719737;    int IdBjdmcFqi7866398 = -744506620;    int IdBjdmcFqi65155154 = -532726782;    int IdBjdmcFqi31700267 = -466939982;    int IdBjdmcFqi54839726 = -634356834;    int IdBjdmcFqi64117098 = -530262364;    int IdBjdmcFqi27353310 = -554417221;    int IdBjdmcFqi94925916 = -220626845;    int IdBjdmcFqi60210096 = -949755450;    int IdBjdmcFqi86666219 = -240324844;    int IdBjdmcFqi383322 = -597518534;    int IdBjdmcFqi87233133 = -820773606;    int IdBjdmcFqi65966484 = -826046021;    int IdBjdmcFqi18673526 = -133609258;    int IdBjdmcFqi30690867 = -872535286;    int IdBjdmcFqi39612744 = -663103049;    int IdBjdmcFqi95876627 = 60932715;    int IdBjdmcFqi83499881 = -570392106;    int IdBjdmcFqi57978766 = -328976687;    int IdBjdmcFqi24605495 = -845287360;    int IdBjdmcFqi92134999 = -849570563;    int IdBjdmcFqi82824395 = -860480788;    int IdBjdmcFqi72545637 = -84434134;    int IdBjdmcFqi41006742 = -165678022;    int IdBjdmcFqi6942177 = -195125334;    int IdBjdmcFqi74263352 = -529829389;    int IdBjdmcFqi15823268 = -475841633;    int IdBjdmcFqi81817574 = -378489757;    int IdBjdmcFqi34107249 = -927198345;    int IdBjdmcFqi50648024 = -288630817;    int IdBjdmcFqi46410589 = -991547967;    int IdBjdmcFqi82855829 = -781177125;    int IdBjdmcFqi75493548 = -936586138;    int IdBjdmcFqi83918475 = 25085373;    int IdBjdmcFqi62996476 = 64667984;    int IdBjdmcFqi46420602 = -496617983;    int IdBjdmcFqi60509187 = -172938038;    int IdBjdmcFqi36491978 = -993099639;    int IdBjdmcFqi3787663 = -26245725;    int IdBjdmcFqi49827132 = -935578150;    int IdBjdmcFqi52735652 = -57483491;    int IdBjdmcFqi76092649 = -637000170;    int IdBjdmcFqi36246740 = 39371668;    int IdBjdmcFqi53518185 = -672712927;    int IdBjdmcFqi32672328 = -524355901;    int IdBjdmcFqi60946881 = 42871859;    int IdBjdmcFqi5274680 = -773985460;    int IdBjdmcFqi98156432 = -637317974;    int IdBjdmcFqi32730267 = -872140546;    int IdBjdmcFqi12343846 = -220030630;    int IdBjdmcFqi2088071 = -881733636;    int IdBjdmcFqi20219745 = -161594085;    int IdBjdmcFqi79003739 = -277491979;    int IdBjdmcFqi76437528 = 91175469;    int IdBjdmcFqi85103727 = -983836256;    int IdBjdmcFqi83166879 = -598238950;    int IdBjdmcFqi35320761 = -560072486;    int IdBjdmcFqi24148413 = -267048760;    int IdBjdmcFqi24758090 = -171814648;    int IdBjdmcFqi80576374 = -4527445;    int IdBjdmcFqi48293830 = 45579268;    int IdBjdmcFqi45535736 = -75927465;    int IdBjdmcFqi60818667 = -293428501;    int IdBjdmcFqi9562073 = -561124634;    int IdBjdmcFqi40255630 = -248776878;    int IdBjdmcFqi17527493 = -816341410;    int IdBjdmcFqi11739585 = -884187468;    int IdBjdmcFqi82048008 = -751131395;    int IdBjdmcFqi55677049 = -98277242;    int IdBjdmcFqi84270265 = -275917304;    int IdBjdmcFqi79103557 = -390165011;    int IdBjdmcFqi59384649 = 54032354;    int IdBjdmcFqi79712219 = -444146381;    int IdBjdmcFqi8151635 = -393398537;    int IdBjdmcFqi71869842 = -823534156;     IdBjdmcFqi50130215 = IdBjdmcFqi19838335;     IdBjdmcFqi19838335 = IdBjdmcFqi76536571;     IdBjdmcFqi76536571 = IdBjdmcFqi61157202;     IdBjdmcFqi61157202 = IdBjdmcFqi28987752;     IdBjdmcFqi28987752 = IdBjdmcFqi54287000;     IdBjdmcFqi54287000 = IdBjdmcFqi25664341;     IdBjdmcFqi25664341 = IdBjdmcFqi68192245;     IdBjdmcFqi68192245 = IdBjdmcFqi58627388;     IdBjdmcFqi58627388 = IdBjdmcFqi13944230;     IdBjdmcFqi13944230 = IdBjdmcFqi80088962;     IdBjdmcFqi80088962 = IdBjdmcFqi71018566;     IdBjdmcFqi71018566 = IdBjdmcFqi96456836;     IdBjdmcFqi96456836 = IdBjdmcFqi40184404;     IdBjdmcFqi40184404 = IdBjdmcFqi33055650;     IdBjdmcFqi33055650 = IdBjdmcFqi48180015;     IdBjdmcFqi48180015 = IdBjdmcFqi71241164;     IdBjdmcFqi71241164 = IdBjdmcFqi16829959;     IdBjdmcFqi16829959 = IdBjdmcFqi63421134;     IdBjdmcFqi63421134 = IdBjdmcFqi51956589;     IdBjdmcFqi51956589 = IdBjdmcFqi97964697;     IdBjdmcFqi97964697 = IdBjdmcFqi3719627;     IdBjdmcFqi3719627 = IdBjdmcFqi36982506;     IdBjdmcFqi36982506 = IdBjdmcFqi1043024;     IdBjdmcFqi1043024 = IdBjdmcFqi77238726;     IdBjdmcFqi77238726 = IdBjdmcFqi65991275;     IdBjdmcFqi65991275 = IdBjdmcFqi7866398;     IdBjdmcFqi7866398 = IdBjdmcFqi65155154;     IdBjdmcFqi65155154 = IdBjdmcFqi31700267;     IdBjdmcFqi31700267 = IdBjdmcFqi54839726;     IdBjdmcFqi54839726 = IdBjdmcFqi64117098;     IdBjdmcFqi64117098 = IdBjdmcFqi27353310;     IdBjdmcFqi27353310 = IdBjdmcFqi94925916;     IdBjdmcFqi94925916 = IdBjdmcFqi60210096;     IdBjdmcFqi60210096 = IdBjdmcFqi86666219;     IdBjdmcFqi86666219 = IdBjdmcFqi383322;     IdBjdmcFqi383322 = IdBjdmcFqi87233133;     IdBjdmcFqi87233133 = IdBjdmcFqi65966484;     IdBjdmcFqi65966484 = IdBjdmcFqi18673526;     IdBjdmcFqi18673526 = IdBjdmcFqi30690867;     IdBjdmcFqi30690867 = IdBjdmcFqi39612744;     IdBjdmcFqi39612744 = IdBjdmcFqi95876627;     IdBjdmcFqi95876627 = IdBjdmcFqi83499881;     IdBjdmcFqi83499881 = IdBjdmcFqi57978766;     IdBjdmcFqi57978766 = IdBjdmcFqi24605495;     IdBjdmcFqi24605495 = IdBjdmcFqi92134999;     IdBjdmcFqi92134999 = IdBjdmcFqi82824395;     IdBjdmcFqi82824395 = IdBjdmcFqi72545637;     IdBjdmcFqi72545637 = IdBjdmcFqi41006742;     IdBjdmcFqi41006742 = IdBjdmcFqi6942177;     IdBjdmcFqi6942177 = IdBjdmcFqi74263352;     IdBjdmcFqi74263352 = IdBjdmcFqi15823268;     IdBjdmcFqi15823268 = IdBjdmcFqi81817574;     IdBjdmcFqi81817574 = IdBjdmcFqi34107249;     IdBjdmcFqi34107249 = IdBjdmcFqi50648024;     IdBjdmcFqi50648024 = IdBjdmcFqi46410589;     IdBjdmcFqi46410589 = IdBjdmcFqi82855829;     IdBjdmcFqi82855829 = IdBjdmcFqi75493548;     IdBjdmcFqi75493548 = IdBjdmcFqi83918475;     IdBjdmcFqi83918475 = IdBjdmcFqi62996476;     IdBjdmcFqi62996476 = IdBjdmcFqi46420602;     IdBjdmcFqi46420602 = IdBjdmcFqi60509187;     IdBjdmcFqi60509187 = IdBjdmcFqi36491978;     IdBjdmcFqi36491978 = IdBjdmcFqi3787663;     IdBjdmcFqi3787663 = IdBjdmcFqi49827132;     IdBjdmcFqi49827132 = IdBjdmcFqi52735652;     IdBjdmcFqi52735652 = IdBjdmcFqi76092649;     IdBjdmcFqi76092649 = IdBjdmcFqi36246740;     IdBjdmcFqi36246740 = IdBjdmcFqi53518185;     IdBjdmcFqi53518185 = IdBjdmcFqi32672328;     IdBjdmcFqi32672328 = IdBjdmcFqi60946881;     IdBjdmcFqi60946881 = IdBjdmcFqi5274680;     IdBjdmcFqi5274680 = IdBjdmcFqi98156432;     IdBjdmcFqi98156432 = IdBjdmcFqi32730267;     IdBjdmcFqi32730267 = IdBjdmcFqi12343846;     IdBjdmcFqi12343846 = IdBjdmcFqi2088071;     IdBjdmcFqi2088071 = IdBjdmcFqi20219745;     IdBjdmcFqi20219745 = IdBjdmcFqi79003739;     IdBjdmcFqi79003739 = IdBjdmcFqi76437528;     IdBjdmcFqi76437528 = IdBjdmcFqi85103727;     IdBjdmcFqi85103727 = IdBjdmcFqi83166879;     IdBjdmcFqi83166879 = IdBjdmcFqi35320761;     IdBjdmcFqi35320761 = IdBjdmcFqi24148413;     IdBjdmcFqi24148413 = IdBjdmcFqi24758090;     IdBjdmcFqi24758090 = IdBjdmcFqi80576374;     IdBjdmcFqi80576374 = IdBjdmcFqi48293830;     IdBjdmcFqi48293830 = IdBjdmcFqi45535736;     IdBjdmcFqi45535736 = IdBjdmcFqi60818667;     IdBjdmcFqi60818667 = IdBjdmcFqi9562073;     IdBjdmcFqi9562073 = IdBjdmcFqi40255630;     IdBjdmcFqi40255630 = IdBjdmcFqi17527493;     IdBjdmcFqi17527493 = IdBjdmcFqi11739585;     IdBjdmcFqi11739585 = IdBjdmcFqi82048008;     IdBjdmcFqi82048008 = IdBjdmcFqi55677049;     IdBjdmcFqi55677049 = IdBjdmcFqi84270265;     IdBjdmcFqi84270265 = IdBjdmcFqi79103557;     IdBjdmcFqi79103557 = IdBjdmcFqi59384649;     IdBjdmcFqi59384649 = IdBjdmcFqi79712219;     IdBjdmcFqi79712219 = IdBjdmcFqi8151635;     IdBjdmcFqi8151635 = IdBjdmcFqi71869842;     IdBjdmcFqi71869842 = IdBjdmcFqi50130215;}
// Junk Finished
