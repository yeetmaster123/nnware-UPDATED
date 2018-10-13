
#include "includes.h"
#include "UTILS/interfaces.h"
#include "UTILS/offsets.h"
#include "SDK/CBaseAnimState.h"
#include "SDK/CInput.h"
#include "SDK/IClient.h"
#include "SDK/CPanel.h"
#include "UTILS/render.h"
#include "SDK/ConVar.h"
#include "SDK/CGlowObjectManager.h"
#include "SDK/IEngine.h"
#include "SDK/CTrace.h"
#include "SDK/CClientEntityList.h"
#include "SDK/CBaseWeapon.h"
#include "SDK/ModelInfo.h"
#include "SDK/ModelRender.h"
#include "SDK/RenderView.h"
#include "SDK/CTrace.h"
#include "SDK/CViewSetup.h"
#include "SDK/CGlobalVars.h"
#include "UTILS/NetvarHookManager.h"
#include "SDK/CBaseEntity.h"
#include "SDK/RecvData.h"


class Weapon_t
{
public:
	bool ChangerEnabled = false;
	int ChangerSkin = 0;
	char ChangerName[32] = "";
	int ChangerStatTrak = 0;
	int ChangerSeed = 0;
	float ChangerWear = 0;
};
extern Weapon_t* weapons;
extern void skinchanger();