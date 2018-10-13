#pragma once

#include "../UTILS/offsets.h"
#include "../UTILS/interfaces.h"


namespace SDK
{
	class CSWeaponInfo
	{
	public:
		char _0x0000[20];
		__int32 max_clip;	//0x0014 
		char _0x0018[12];
		__int32 max_reserved_ammo;	//0x0024 
		char _0x0028[96];
		char* hud_name;//0x0088 
		char* weapon_name;	//0x008C 
		char _0x0090[60];
		__int32 type;//0x00CC 
		__int32 price;//0x00D0 
		__int32 reward;//0x00D4 
		char _0x00D8[20];
		BYTE full_auto;//0x00EC 
		char _0x00ED[3];
		__int32 damage;//0x00F0 
		float armor_ratio;	//0x00F4 
		__int32 bullets;	//0x00F8 
		float penetration;	//0x00FC 
		char _0x0100[8];
		float range;//0x0108 
		float range_modifier;//0x010C 
		char _0x0110[16];
		BYTE silencer;//0x0120 
		char _0x0121[15];
		float max_speed;	//0x0130 
		float max_speed_alt;//0x0134 
		char _0x0138[76];
		__int32 recoil_seed;//0x0184 
		char _0x0188[32];
	};

	struct Weapon_Info
	{
	public:
		char _0x0000[20];
		__int32 max_clip;	//0x0014 
		char _0x0018[12];
		__int32 max_reserved_ammo;	//0x0024 
		char _0x0028[96];
		char* hud_name;//0x0088 
		char* weapon_name;	//0x008C 
		char _0x0090[60];
		__int32 type;//0x00CC 
		__int32 price;//0x00D0 
		__int32 reward;//0x00D4 
		char _0x00D8[20];
		BYTE full_auto;//0x00EC 
		char _0x00ED[3];
		__int32 damage;//0x00F0 
		float armor_ratio;	//0x00F4 
		__int32 bullets;	//0x00F8 
		float penetration;	//0x00FC 
		char _0x0100[8];
		float range;//0x0108 
		float range_modifier;//0x010C 
		char _0x0110[16];
		BYTE silencer;//0x0120 
		char _0x0121[15];
		float max_speed;	//0x0130 
		float max_speed_alt;//0x0134 
		char _0x0138[76];
		__int32 recoil_seed;//0x0184 
		char _0x0188[32];
	};
	class CBaseWeapon
	{
	public:
		float GetNextPrimaryAttack()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flNextPrimaryAttack);
		}
		short GetItemDefenitionIndex()
		{
			return *reinterpret_cast<short*>(uintptr_t(this) + OFFSETS::m_iItemDefinitionIndex);
		}
		short GetItemDefinitionIndexbam()
		{
			return *reinterpret_cast<short*>(uintptr_t(this) + OFFSETS::m_iItemDefinitionIndex);
		}
		
		int* ModelIndex()
		{
			return (int*)((uintptr_t)this + OFFSETS::m_nModelIndex);
		}
		int* ViewModelIndex()
		{
			return (int*)((uintptr_t)this + OFFSETS::m_iViewModelIndex);
		}
		int* fixItemIDHigh()
		{
			return (int*)((uintptr_t)this + 0x2FB0);
		}
		HANDLE m_hWeaponWorldModel()
		{
			return *(HANDLE*)((uintptr_t)this + OFFSETS::m_hWeaponWorldModel);
		}
		int* GetEntityQuality()
		{
			return (int*)((uintptr_t)this + 0x2F9C);
		}
		short* fixskins()
		{
			return (short*)((uintptr_t)this + OFFSETS::m_iItemDefinitionIndex);
		}
		short fix()
		{
			return *(short*)((uintptr_t)this + OFFSETS::m_iItemDefinitionIndex);
		}
		int* FallbackPaintKit()
		{
			return (int*)((uintptr_t)this + 0x31A8);
		}

		int* FallbackSeed()
		{
			return (int*)((uintptr_t)this + 0x31AC);
		}

		float* FallbackWear()
		{
			return (float*)((uintptr_t)this + 0x31B0);
		}
		char* GetCustomName() {
			return reinterpret_cast<char*>(uintptr_t(this) + 0x302C);
		}
		int* FallbackStatTrak()
		{
			return (int*)((uintptr_t)this + 0x31B4);
		}

		int* OwnerXuidLow()
		{
			return (int*)((uintptr_t)this + 0x31A0);
		}
		int* OwnerXuidHigh()
		{
			return (int*)((uintptr_t)this + 0x31A4);
		}
		float GetPostponeFireReadyTime()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_flPostponeFireReadyTime);
		}
		float GetSpreadCone()
		{
			typedef float(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 436)(this);
		}
		float GetInaccuracy()
		{
			typedef float(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 467)(this);
		}
		HANDLE GetOwnerHandle()
		{
			return *(HANDLE*)((uintptr_t)this + 0x31D0);
		}
		Vector GetOrigin()
		{
			return *(Vector*)((uintptr_t)this + OFFSETS::m_vecOrigin);
		}
		bool is_revolver()
		{
			int iWeaponID = this->GetItemDefenitionIndex();
			return (iWeaponID == 64);
		}

		bool is_knife()
		{
			int iWeaponID = this->GetItemDefenitionIndex();
			return (iWeaponID == 42 || iWeaponID == 59
				|| iWeaponID == 59 || iWeaponID == 41
				|| iWeaponID == 500 || iWeaponID == 505 || iWeaponID == 506
				|| iWeaponID == 507 || iWeaponID == 508 || iWeaponID == 509
				|| iWeaponID == 515);
		}

		bool is_grenade()
		{
			if (!this)
				return false;

			int WeaponId = this->GetItemDefenitionIndex();

			if (!WeaponId)
				return false;

			if (WeaponId == 43 || WeaponId == 44 || WeaponId == 45 || WeaponId == 46 || WeaponId == 47 || WeaponId == 48)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		int GetLoadedAmmo() 
		{
			return *(int*)((DWORD)this + OFFSETS::m_iClip1);
		}
		void UpdateAccuracyPenalty()
		{
			typedef void(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 468)(this); //470
		}
		Weapon_Info* GetWeaponInfo()
		{
			if (!this) return nullptr;
			typedef Weapon_Info*(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 444)(this);
		}
		CSWeaponInfo* get_full_info()
		{
			if (!this) return nullptr;
			typedef CSWeaponInfo*(__thiscall* Fn)(void*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 444)(this);
		}
	};

	class CCSBomb
	{
	public:
		HANDLE GetOwnerHandle()
		{
			return *(HANDLE*)((uintptr_t)this + 0x31D0);
		}

		float GetC4BlowTime()
		{
			return *(float*)((uintptr_t)this + 0x297C);
		}

		float GetC4DefuseCountDown()
		{
			return *(float*)((uintptr_t)this + 0x2994);
		}

		int GetBombDefuser()
		{
			return *(int*)((uintptr_t)this + 0x17B8);
		}

		bool IsBombDefused()
		{
			return *(bool*)((uintptr_t)this + 0x2998);
		}
	};

	class CBaseCSGrenade : CBaseWeapon
	{
	public:
		float pin_pulled()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_bPinPulled);
		}

		float throw_time()
		{
			return *reinterpret_cast<float*>(uintptr_t(this) + OFFSETS::m_fThrowTime);
		}
	};
}