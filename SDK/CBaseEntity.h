#pragma once

#include "../UTILS/offsets.h"
#include "../UTILS/interfaces.h"
#include "../SDK/ModelInfo.h"
#include "../UTILS/qangle.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CClientEntityList.h"

#include "AnimLayer.h"
#include "RecvData.h"
enum DataUpdateType_t
{
	DATA_UPDATE_CREATED = 0,
	DATA_UPDATE_DATATABLE_CHANGED,
};
typedef unsigned long CBaseHandle;
namespace SDK
{
	class CBaseAnimState;
	class Collideable;
	
	struct studiohdr_t;
	struct model_t;

	enum ItemDefinitionIndex : int
	{
		WEAPON_DEAGLE = 1,
		WEAPON_ELITE = 2,
		WEAPON_FIVESEVEN = 3,
		WEAPON_GLOCK = 4,
		WEAPON_AK47 = 7,
		WEAPON_AUG = 8,
		WEAPON_AWP = 9,
		WEAPON_FAMAS = 10,
		WEAPON_G3SG1 = 11,
		WEAPON_GALILAR = 13,
		WEAPON_M249 = 14,
		WEAPON_M4A1 = 16,
		WEAPON_MAC10 = 17,
		WEAPON_P90 = 19,
		WEAPON_UMP45 = 24,
		WEAPON_XM1014 = 25,
		WEAPON_BIZON = 26,
		WEAPON_MAG7 = 27,
		WEAPON_NEGEV = 28,
		WEAPON_SAWEDOFF = 29,
		WEAPON_TEC9 = 30,
		WEAPON_TASER = 31,
		WEAPON_HKP2000 = 32,
		WEAPON_MP7 = 33,
		WEAPON_MP9 = 34,
		WEAPON_NOVA = 35,
		WEAPON_P250 = 36,
		WEAPON_SCAR20 = 38,
		WEAPON_SG556 = 39,
		WEAPON_SSG08 = 40,
		WEAPON_KNIFE_CT = 42,
		WEAPON_FLASHBANG = 43,
		WEAPON_HEGRENADE = 44,
		WEAPON_SMOKEGRENADE = 45,
		WEAPON_MOLOTOV = 46,
		WEAPON_DECOY = 47,
		WEAPON_INCGRENADE = 48,
		WEAPON_C4 = 49,
		WEAPON_KNIFE_T = 59,
		WEAPON_M4A1_SILENCER = 60,
		WEAPON_USP_SILENCER = 61,
		WEAPON_CZ75A = 63,
		WEAPON_REVOLVER = 64,
		WEAPON_KNIFE_BAYONET = 500,
		WEAPON_KNIFE_FLIP = 505,
		WEAPON_KNIFE_GUT = 506,
		WEAPON_KNIFE_KARAMBIT = 507,
		WEAPON_KNIFE_M9_BAYONET = 508,
		WEAPON_KNIFE_TACTICAL = 509,
		WEAPON_KNIFE_FALCHION = 512,
		WEAPON_KNIFE_BOWIE = 514,
		WEAPON_KNIFE_BUTTERFLY = 515,
		WEAPON_KNIFE_PUSH = 516
	};

	enum MoveType_t
	{
		MOVETYPE_NONE = 0,
		MOVETYPE_ISOMETRIC,
		MOVETYPE_WALK,
		MOVETYPE_STEP,
		MOVETYPE_FLY,
		MOVETYPE_FLYGRAVITY,
		MOVETYPE_VPHYSICS,
		MOVETYPE_PUSH,
		MOVETYPE_NOCLIP,
		MOVETYPE_LADDER,
		MOVETYPE_OBSERVER,
		MOVETYPE_CUSTOM,
		MOVETYPE_LAST = MOVETYPE_CUSTOM,
		MOVETYPE_MAX_BITS = 4
	};
	class CBaseViewModel : public CModelInfo
	{
	public:

		inline DWORD GetOwner() {

			return *(PDWORD)((DWORD)this + 0x29BC);
		}

		inline int GetModelIndex() {

			return *(int*)((DWORD)this + OFFSETS::m_nModelIndex);
		}
	};
	class CBaseAnimating
	{
	public:
		std::array<float, 24>* m_flPoseParameter()
		{
			static int offset = 0;
			if (!offset)
				offset = OFFSETS::m_flPoseParameter;
			return (std::array<float, 24>*)((uintptr_t)this + offset);
		}
		model_t* GetModel()
		{
			void* pRenderable = reinterpret_cast<void*>(uintptr_t(this) + 0x4);
			typedef model_t* (__thiscall* fnGetModel)(void*);

			return VMT::VMTHookManager::GetFunction<fnGetModel>(pRenderable, 8)(pRenderable);
		}
		
		void SetBoneMatrix(matrix3x4_t* boneMatrix)
		{
			//Offset found in C_BaseAnimating::GetBoneTransform, string search ankle_L and a function below is the right one
			const auto model = this->GetModel();
			if (!model)
				return;

			matrix3x4_t* matrix = *(matrix3x4_t**)((DWORD)this + 9880);
			studiohdr_t *hdr = INTERFACES::ModelInfo->GetStudioModel(model);
			if (!hdr)
				return;
			int size = hdr->numbones;
			if (matrix) {
				for (int i = 0; i < size; i++)
					memcpy(matrix + i, boneMatrix + i, sizeof(matrix3x4_t));
			}
		}
		void GetDirectBoneMatrix(matrix3x4_t* boneMatrix)
		{
			const auto model = this->GetModel();
			if (!model)
				return;

			matrix3x4_t* matrix = *(matrix3x4_t**)((DWORD)this + 9880);
			studiohdr_t *hdr = INTERFACES::ModelInfo->GetStudioModel(model);
			if (!hdr)
				return;
			int size = hdr->numbones;
			if (matrix) {
				for (int i = 0; i < size; i++)
					memcpy(boneMatrix + i, matrix + i, sizeof(matrix3x4_t));
			}
		}
	};

	class CBaseEntity
	{
	public:
		int GetHealth()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iHealth);
		}
		CBaseHandle * m_hMyWeapons()
		{
			return (CBaseHandle*)((uintptr_t)this + 0x2DE8);
		}
		int GetItemDefenitionIndex()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iItemDefinitionIndex);
		}
		char* GetGunIcon() {
			int WeaponId = this->GetItemDefenitionIndex();

			switch (WeaponId) {
			case  SDK::ItemDefinitionIndex::WEAPON_KNIFE_CT:
				return "]";
			case  SDK::ItemDefinitionIndex::WEAPON_KNIFE_T:
				return "]";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_BAYONET:
				return "1";
			case  SDK::ItemDefinitionIndex::WEAPON_KNIFE_FLIP:
				return "2";
			case  SDK::ItemDefinitionIndex::WEAPON_KNIFE_GUT:
				return "3";
			case  SDK::ItemDefinitionIndex::WEAPON_KNIFE_KARAMBIT:
				return "]";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_M9_BAYONET:
				return "5";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_TACTICAL:
				return "7";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_FALCHION:
				return "0";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_BOWIE:
				return "6";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_BUTTERFLY:
				return "8";
			case SDK::ItemDefinitionIndex::WEAPON_KNIFE_PUSH:
				return "9";
			case SDK::ItemDefinitionIndex::WEAPON_DEAGLE:
				return "A";
			case SDK::ItemDefinitionIndex::WEAPON_ELITE:
				return "B";
			case SDK::ItemDefinitionIndex::WEAPON_FIVESEVEN:
				return "C";
			case SDK::ItemDefinitionIndex::WEAPON_GLOCK:
				return "D";
			case  SDK::ItemDefinitionIndex::WEAPON_HKP2000:
				return "E";
			case SDK::ItemDefinitionIndex::WEAPON_P250:
				return "F";
			case  SDK::ItemDefinitionIndex::WEAPON_USP_SILENCER:
				return "G";
			case SDK::ItemDefinitionIndex::WEAPON_TEC9:
				return "H";
			case  SDK::ItemDefinitionIndex::WEAPON_CZ75A:
				return "I";
			case  SDK::ItemDefinitionIndex::WEAPON_REVOLVER:
				return "J";
			case  SDK::ItemDefinitionIndex::WEAPON_MAC10:
				return "K";
			case  SDK::ItemDefinitionIndex::WEAPON_UMP45:
				return "L";
			case  SDK::ItemDefinitionIndex::WEAPON_BIZON:
				return "M";
			case  SDK::ItemDefinitionIndex::WEAPON_MP7:
				return "N";
			case SDK::ItemDefinitionIndex::WEAPON_MP9:
				return "O";
			case  SDK::ItemDefinitionIndex::WEAPON_P90:
				return "P";
			case  SDK::ItemDefinitionIndex::WEAPON_GALILAR:
				return "Q";
			case  SDK::ItemDefinitionIndex::WEAPON_FAMAS:
				return "R";
			case SDK::ItemDefinitionIndex::WEAPON_M4A1_SILENCER:
				return "T";
			case SDK::ItemDefinitionIndex::WEAPON_M4A1:
				return "S";
			case SDK::ItemDefinitionIndex::WEAPON_AUG:
				return "U";
			case SDK::ItemDefinitionIndex::WEAPON_SG556:
				return "V";
			case SDK::ItemDefinitionIndex::WEAPON_AK47:
				return "W";
			case SDK::ItemDefinitionIndex::WEAPON_G3SG1:
				return "X";
			case SDK::ItemDefinitionIndex::WEAPON_SCAR20:
				return "Y";
			case  SDK::ItemDefinitionIndex::WEAPON_AWP:
				return "Z";
			case  SDK::ItemDefinitionIndex::WEAPON_SSG08:
				return "a";
			case SDK::ItemDefinitionIndex::WEAPON_XM1014:
				return "b";
			case  SDK::ItemDefinitionIndex::WEAPON_SAWEDOFF:
				return "c";
			case SDK::ItemDefinitionIndex::WEAPON_MAG7:
				return "d";
			case SDK::ItemDefinitionIndex::WEAPON_NOVA:
				return "e";
			case SDK::ItemDefinitionIndex::WEAPON_NEGEV:
				return "f";
			case SDK::ItemDefinitionIndex::WEAPON_M249:
				return "g";
			case SDK::ItemDefinitionIndex::WEAPON_TASER:
				return "h";
			case SDK::ItemDefinitionIndex::WEAPON_FLASHBANG:
				return "i";
			case SDK::ItemDefinitionIndex::WEAPON_HEGRENADE:
				return "j";
			case SDK::ItemDefinitionIndex::WEAPON_SMOKEGRENADE:
				return "k";
			case SDK::ItemDefinitionIndex::WEAPON_MOLOTOV:
				return "l";
			case SDK::ItemDefinitionIndex::WEAPON_DECOY:
				return "m";
			case SDK::ItemDefinitionIndex::WEAPON_INCGRENADE:
				return "n";
			case SDK::ItemDefinitionIndex::WEAPON_C4:
				return "o";
			default:
				return " ";
			}
		}

		enum ItemDefinitionIndex : int
		{
			WEAPON_DEAGLE = 1,
			WEAPON_ELITE = 2,
			WEAPON_FIVESEVEN = 3,
			WEAPON_GLOCK = 4,
			WEAPON_AK47 = 7,
			WEAPON_AUG = 8,
			WEAPON_AWP = 9,
			WEAPON_FAMAS = 10,
			WEAPON_G3SG1 = 11,
			WEAPON_GALILAR = 13,
			WEAPON_M249 = 14,
			WEAPON_M4A1 = 16,
			WEAPON_MAC10 = 17,
			WEAPON_P90 = 19,
			WEAPON_UMP45 = 24,
			WEAPON_XM1014 = 25,
			WEAPON_BIZON = 26,
			WEAPON_MAG7 = 27,
			WEAPON_NEGEV = 28,
			WEAPON_SAWEDOFF = 29,
			WEAPON_TEC9 = 30,
			WEAPON_TASER = 31,
			WEAPON_HKP2000 = 32,
			WEAPON_MP7 = 33,
			WEAPON_MP9 = 34,
			WEAPON_NOVA = 35,
			WEAPON_P250 = 36,
			WEAPON_SCAR20 = 38,
			WEAPON_SG556 = 39,
			WEAPON_SSG08 = 40,
			WEAPON_KNIFE_CT = 42,
			WEAPON_FLASHBANG = 43,
			WEAPON_HEGRENADE = 44,
			WEAPON_SMOKEGRENADE = 45,
			WEAPON_MOLOTOV = 46,
			WEAPON_DECOY = 47,
			WEAPON_INCGRENADE = 48,
			WEAPON_C4 = 49,
			WEAPON_KNIFE_T = 59,
			WEAPON_M4A1_SILENCER = 60,
			WEAPON_USP_SILENCER = 61,
			WEAPON_CZ75A = 63,
			WEAPON_REVOLVER = 64,
			WEAPON_KNIFE_BAYONET = 500,
			WEAPON_KNIFE_FLIP = 505,
			WEAPON_KNIFE_GUT = 506,
			WEAPON_KNIFE_KARAMBIT = 507,
			WEAPON_KNIFE_M9_BAYONET = 508,
			WEAPON_KNIFE_TACTICAL = 509,
			WEAPON_KNIFE_FALCHION = 512,
			WEAPON_KNIFE_BOWIE = 514,
			WEAPON_KNIFE_BUTTERFLY = 515,
			WEAPON_KNIFE_PUSH = 516
		};
		uintptr_t* GetWeapons()
		{
			return reinterpret_cast<uintptr_t*>(uintptr_t(this) + OFFSETS::m_hMyWeapons);
		}
		int GetFlags()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_fFlags);
		}
		void SetSpotted(bool value)
		{
			*reinterpret_cast<bool*>(uintptr_t(this) + OFFSETS::m_bSpotted) = value;
		}
		void SetFlags(int flags)
		{
			*reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_fFlags) = flags;
		}
		int GetTeam()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iTeamNum);
		}
		int GetObserverMode()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iObserverMode);
		}
		int SetObserverMode(int mode)
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iObserverMode) = mode;
		}
		void SetFlashDuration(float value)
		{
			*reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flFlashDuration) = value;
		}
		bool GetIsScoped()
		{
			return *reinterpret_cast<bool*>(uintptr_t(this) + OFFSETS::m_bIsScoped);
		}
		Vector GetVelocity()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_vecVelocity);
		}
		void SetVelocity(Vector velocity)
		{
			*reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_vecVelocity) = velocity;
		}
		int GetMoney()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_iAccount);
		}
		int GetLifeState()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_lifeState);
		}
		float GetLowerBodyYaw()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flLowerBodyYawTarget);
		}
		Vector GetVecOrigin()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_vecOrigin);
		}
		Vector* GetViewPunchAngle()
		{
			return (Vector*)((uintptr_t)this + OFFSETS::m_viewPunchAngle);
		}

		Vector* GetAimPunchAngle()
		{
			return (Vector*)((uintptr_t)this + OFFSETS::m_aimPunchAngle);
		}
		bool IsAlive() 
		{ 
			return this->GetLifeState() == 0; 
		}
		Vector& GetAbsOrigin()
		{
			typedef Vector& (__thiscall* OriginalFn)(void*);
			return ((OriginalFn)VMT::VMTHookManager::GetFunction<OriginalFn>(this, 10))(this);
		}
		int GetTickBase()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_nTickBase);
		}
		bool GetIsDormant()
		{
			return *reinterpret_cast<bool*>(uintptr_t(this) + OFFSETS::m_bDormant);
		}
		CBaseAnimState* GetAnimState()
		{
			return *reinterpret_cast<CBaseAnimState**>(uintptr_t(this) + OFFSETS::animstate);
		}
		CAnimationLayer* GetAnimOverlaysModifiable() 
		{ 
			return (*reinterpret_cast< CAnimationLayer** >(reinterpret_cast< std::uintptr_t >(this) + 0x2970));
		}
		Collideable* GetCollideable()
		{
			return (Collideable*)((DWORD)this + OFFSETS::m_Collision);
		}
		void GetRenderBounds(Vector& mins, Vector& maxs)
		{
			void* pRenderable = (void*)(this + 0x4);
			typedef void(__thiscall* Fn)(void*, Vector&, Vector&);
			VMT::VMTHookManager::GetFunction<Fn>(pRenderable, 17)(pRenderable, mins, maxs);
		}
		int GetIndex()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + 0x64);
		}
		int GetMoveType()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_MoveType);
		}
		CAnimationLayer& GetAnimOverlay(int Index)
		{
			return (*(CAnimationLayer**)((DWORD)this + 0x2970))[Index];
		}
		void SetAnimOverlay(int Index, CAnimationLayer layer)
		{
			(*(CAnimationLayer**)((DWORD)this + 0x2970))[Index] = layer;
		}
		Vector* GetPunchAngles1() {
			return reinterpret_cast< Vector* >(uintptr_t(this) + OFFSETS::m_aimPunchAngle);
		}
		Vector *GetPunchAngles2()
		{
			return reinterpret_cast<Vector*>(uintptr_t(this) + 0x3010);
		}
		int CBaseEntity::GetSequenceActivity(int sequence)
		{
			const auto model = GetModel();
			if (!model)
				return -1;

			const auto hdr = INTERFACES::ModelInfo->GetStudioModel(model);
			if (!hdr)
				return -1;

			static auto offset = (DWORD)UTILS::FindSignature("client_panorama.dll", "55 8B EC 83 7D 08 FF 56 8B F1 74 3D");
			static auto GetSequenceActivity = reinterpret_cast<int(__fastcall*)(void*, SDK::studiohdr_t*, int)>(offset);
			
			return GetSequenceActivity(this, hdr, sequence);
		}
		Vector GetEyeAngles()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_angEyeAngles);
		}
		Vector GetVecMins()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + 0x320);
		}
		Vector GetVecMaxs()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + 0x32C);
		}
		QAngle* EasyEyeAngles()
		{
			return (QAngle*)((uintptr_t)this + OFFSETS::m_angEyeAngles);
		}
		void SetModelIndex(int index)
		{
			typedef void(__thiscall* OriginalFn)(PVOID, int);
			return VMT::VMTHookManager::GetFunction<OriginalFn>(this, 75)(this, index);
		}
		void PreDataUpdate(DataUpdateType_t updateType)
		{
			PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);
			typedef void(__thiscall* OriginalFn)(PVOID, int);
			return VMT::VMTHookManager::GetFunction<OriginalFn>(pNetworkable, 6)(pNetworkable, updateType);
		}
		IClientNetworkable* CBaseEntity::GetNetworkable()
		{
			return reinterpret_cast<IClientNetworkable*>(reinterpret_cast<uintptr_t>(this) + 0x8);
		}
		void SetEyeAngles(Vector angles)
		{
			*reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_angEyeAngles) = angles;
		}
		float GetSimTime()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flSimulationTime);
		}
		Vector GetViewOffset()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_vecViewOffset);
		}
		model_t* GetModel()
		{
			void* pRenderable = reinterpret_cast<void*>(uintptr_t(this) + 0x4);
			typedef model_t* (__thiscall* fnGetModel)(void*);

			return VMT::VMTHookManager::GetFunction<fnGetModel>(pRenderable, 8)(pRenderable);
		}
		matrix3x4_t GetBoneMatrix(int BoneID)
		{
			matrix3x4_t matrix;

			auto offset = *reinterpret_cast<uintptr_t*>(uintptr_t(this) + OFFSETS::m_dwBoneMatrix);
			if (offset)
				matrix = *reinterpret_cast<matrix3x4_t*>(offset + 0x30 * BoneID);

			return matrix;
		}
		Vector GetBonePosition(int i)
		{
			VMatrix matrix[128];
			if (this->SetupBones(matrix, 128, BONE_USED_BY_HITBOX, GetTickCount64()))
			{
				return Vector(matrix[i][0][3], matrix[i][1][3], matrix[i][2][3]);
			}
			return Vector(0, 0, 0);
		}
		Vector GetEyePosition(void)
		{
			return GetVecOrigin() + *(Vector*)((DWORD)this + OFFSETS::m_vecViewOffset);
		}
		Vector GetPunchAngles()
		{
			return *reinterpret_cast<Vector*>(uintptr_t(this) + OFFSETS::m_aimPunchAngle);
		}
		bool GetImmunity()
		{
			return *reinterpret_cast<bool*>(uintptr_t(this) + OFFSETS::m_bGunGameImmunity);
		}
		bool SetupBones(VMatrix *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
		{
			/*void* pRenderable = reinterpret_cast<void*>(uintptr_t(this) + 0x4);
			if (!pRenderable)
				false;

			typedef bool(__thiscall* Fn)(void*, matrix3x4_t*, int, int, float);
			return VMT::VMTHookManager::GetFunction<Fn>(pRenderable, 13)(pRenderable, pBoneToWorldOut, nMaxBones, boneMask, currentTime);*/
			__asm
			{
				mov edi, this
				lea ecx, dword ptr ds : [edi + 0x4]
				mov edx, dword ptr ds : [ecx]
				push currentTime
				push boneMask
				push nMaxBones
				push pBoneToWorldOut
				call dword ptr ds : [edx + 0x34]
			}
		}
		void UpdateClientSideAnimation()
		{
			typedef void(__thiscall* original)(void*);
			VMT::VMTHookManager::GetFunction<original>(this, 218)(this);
		}
		float GetSpread()
		{
			typedef float(__thiscall* original)(void*);
			return VMT::VMTHookManager::GetFunction<original>(this, 439)(this);
		}
		float GetInaccuracy()
		{
			typedef float(__thiscall* oInaccuracy)(PVOID);
			return VMT::VMTHookManager::GetFunction<oInaccuracy>(this, 469)(this);
		}
		void SetAbsOrigin(Vector ArgOrigin)
		{
			using Fn = void(__thiscall*)(CBaseEntity*, const Vector &origin);
			static Fn func;

			if (!func)
				func = (Fn)(UTILS::FindPattern("client_panorama.dll", (PBYTE)"\x55\x8B\xEC\x83\xE4\xF8\x51\x53\x56\x57\x8B\xF1\xE8\x00\x00", "xxxxxxxxxxxxx??"));

			func(this, ArgOrigin);
		}
		void SetOriginz(Vector wantedpos)
		{
			typedef void(__thiscall* SetOriginFn)(void*, const Vector &);
			static SetOriginFn SetOriginze = (SetOriginFn)(UTILS::FindSignature("client_panorama.dll", "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8"));
			SetOriginze(this, wantedpos);
		}
		Vector& GetAbsAnglesVec()
		{
			typedef Vector& (__thiscall* OriginalFn)(void*);
			return ((OriginalFn)VMT::VMTHookManager::GetFunction<OriginalFn>(this, 11))(this);
		}
		Vector& GetAbsAngles()
		{
			typedef Vector& (__thiscall* OriginalFn)(void*);
			return ((OriginalFn)VMT::VMTHookManager::GetFunction<OriginalFn>(this, 11))(this);
		}
		void SetAbsAngles(Vector angles)
		{
			using Fn = void(__thiscall*)(CBaseEntity*, const Vector &angles);
			static Fn func;

			if (!func)
				func = (Fn)(UTILS::FindPattern("client_panorama.dll", (BYTE*)"\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x64\x53\x56\x57\x8B\xF1\xE8", "xxxxxxxxxxxxxxx"));

			func(this, angles);
		}
		float* GetPoseParamaters()
		{
			return reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flPoseParameter);
		}
		Vector GetRenderAngles()
		{
			return *(Vector*)((DWORD)this + 0x128);
		}
		void SetRenderAngles(Vector angles)
		{
			*(Vector*)((DWORD)this + 0x128) = angles;
		}
		int* GetWearables()
		{
			return (int*)((DWORD)this + 0x2EF4);
		}
		int DrawModel(int flags, uint8_t alpha)
		{
			void* pRenderable = (void*)(this + 0x4);

			using fn = int(__thiscall*)(void*, int, uint8_t);
			return VMT::VMTHookManager::GetFunction< fn >(pRenderable, 9)(pRenderable, flags, alpha);
		}
		ClientClass* GetClientClass()
		{
			void* Networkable = (void*)(this + 0x8);
			typedef ClientClass*(__thiscall* OriginalFn)(void*);
			return VMT::VMTHookManager::GetFunction<OriginalFn>(Networkable, 2)(Networkable);
		}
		float GetNextAttack()
		{
			return *reinterpret_cast<float*>(uint32_t(this) + OFFSETS::m_flNextAttack);
		}
		int GetActiveWeaponIndex()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_hActiveWeapon) & 0xFFF;
		}
		int GetArmor()
		{
			return *reinterpret_cast<int*>(uintptr_t(this) + OFFSETS::m_ArmorValue);
		}
		bool HasHelmet()
		{
			return *reinterpret_cast<bool*>(uintptr_t(this) + OFFSETS::m_bHasHelmet);
		}
		char* GetArmorName()
		{
			if (GetArmor() > 0)
			{
				if (HasHelmet())
					return "hk";
				else
					return "k";
			}
			else
				return " ";
		}
		bool HasC4()
		{
			int iBombIndex = *(int*)(*(DWORD*)(OFFSETS::dwPlayerResource) + OFFSETS::m_iPlayerC4);
			if (iBombIndex == this->GetIndex())
				return true;
			else
				return false;
		}
		void SetAngle2(Vector wantedang) {
			typedef void(__thiscall* oSetAngle)(void*, const Vector &);
			static oSetAngle _SetAngle = (oSetAngle)((uintptr_t)UTILS::FindSignature("client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1"));
			_SetAngle(this, wantedang);
		}
	};

	class CBaseAttributableItem : public CBaseEntity {
	public:
		int* GetAccountID2() {
			return (int*)((DWORD)this + 0x00002D70 + 0x00000040 + 0x000001F8);
		}

		int* GetItemDefinitionIndex2() {
			return (int*)((DWORD)this + 0x00002D70 + 0x00000040 + 0x000001D8);
		}

		int* GetItemIDHigh2() {
			return (int*)((DWORD)this + 0x00002D70 + 0x00000040 + 0x000001F0);
		}

		int* GetEntityQuality2() {
			return (int*)((DWORD)this + 0x00002D70 + 0x00000040 + 0x000001DC);
		}

		//char* GetCustomName() {
		//	return ((DWORD)this + offsets.m_szCustomName2);
		//}

		int* GetFallbackPaintKit() {
			return (int*)((DWORD)this + 0x3170);
		}

		int* GetFallbackSeed() {
			return (int*)((DWORD)this + 0x3174);
		}

		float* GetFallbackWear() {
			return (float*)((DWORD)this + 0x3178);
		}

		int* GetFallbackStatTrak() {
			return (int*)((DWORD)this + 0x317C);
		}

	};
}
