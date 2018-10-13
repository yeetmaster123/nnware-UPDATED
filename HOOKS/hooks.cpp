#include "..\includes.h"
#include "../xdxdxd.h"
#include "../AWHitmarkers.h"
#include "hooks.h"
#include "../UTILS/interfaces.h"
#include "../UTILS/offsets.h"
#include "../NetVar.h"
#include "../UTILS/render.h"
#include "../Skinchanger.h"
#include "../Glovechanger.h"
#include "../SDK/CInput.h"
#include "../SDK/IClient.h"
#include "../SDK/CPanel.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlowObjectManager.h"
#include "../SDK/IEngine.h"
#include "../SDK/CTrace.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/RecvData.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/CTrace.h"
#include "../SDK/CViewSetup.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/CPrediction.h"

#include "../FEATURES/Movement.h"
#include "../FEATURES/Visuals.h"
#include "../FEATURES/Chams.h"
#include "../FEATURES/AntiAim.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Resolver.h"
#include "../FEATURES/Backtracking.h"
#include "../FEATURES/FakeWalk.h"
#include "../FEATURES/FakeLag.h"
#include "../FEATURES/EnginePred.h"
#include "../FEATURES/EventListener.h"
#include "../FEATURES/GrenadePrediction.h"
#include "../FEATURES/Legitbot.h"
#include "../FEATURES/Flashlight.h"

#include "../MENU/menu_framework.h"

#include <intrin.h>

static bool tick = false;
static int ground_tick;
Vector vecAimPunch, vecViewPunch;
Vector* pAimPunch = nullptr;
Vector* pViewPunch = nullptr;

static auto CAM_THINK = UTILS::FindSignature("client_panorama.dll", "85 C0 75 30 38 86");
static auto linegoesthrusmoke = UTILS::FindPattern("client_panorama.dll", (PBYTE)"\x55\x8B\xEC\x83\xEC\x08\x8B\x15\x00\x00\x00\x00\x0F\x57\xC0", "xxxxxxxx????xxx");

void ground_ticks()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		ground_tick++;
	else
		ground_tick = 0;
}

namespace HOOKS
{
	CreateMoveFn original_create_move;
	PaintTraverseFn original_paint_traverse;
	PaintFn original_paint;
	FrameStageNotifyFn original_frame_stage_notify;
	DrawModelExecuteFn original_draw_model_execute;
	SceneEndFn original_scene_end;
	TraceRayFn original_trace_ray;
	OverrideViewFn original_override_view;
	RenderViewFn original_render_view;
	SvCheatsGetBoolFn original_get_bool;
	GetViewmodelFOVFn original_viewmodel_fov;

	vfunc_hook client;
	vfunc_hook panel;
	vfunc_hook paint;
	vfunc_hook modelrender;
	vfunc_hook sceneend;
	vfunc_hook renderview;
	vfunc_hook trace;
	vfunc_hook netchannel;
	vfunc_hook overrideview;
	vfunc_hook input;
	vfunc_hook getbool;

	VMT::VMTHookManager override_view_hook_manager;

	CSX::Hook::VTable SurfaceTable;



	void AutoRevolver(SDK::CUserCmd* cmd) {
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		auto weapon = reinterpret_cast< SDK::CBaseWeapon* >(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

		if (!weapon)
			return;

		if (local_player->GetHealth() <= 0)
			return;

		if (!weapon->GetItemDefenitionIndex())
			return;

		if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_REVOLVER) {
			static int delay = 0;
			delay++;
			if (delay <= 15)
				cmd->buttons |= IN_ATTACK;
			else
				delay = 0;
		}
	}


	bool __stdcall HookedCreateMove(float sample_input_frametime, SDK::CUserCmd* cmd)
	{ 
		static auto ofunc = overrideview.get_original<CreateMoveFn>(24);
		ofunc(sample_input_frametime, cmd);
		if (!cmd || cmd->command_number == 0)
			return false;

		uintptr_t* FPointer; __asm { MOV FPointer, EBP }
		byte* SendPacket = (byte*)(*FPointer - 0x1C);
		if (!SendPacket) return false;

		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!local_player) return false;

		GLOBAL::should_send_packet = *SendPacket;
		GLOBAL::originalCMD = *cmd;
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			GrenadePrediction::instance().Tick(cmd->buttons);
			if (SETTINGS::settings.aim_type == 0)
				slidebitch->do_fakewalk(cmd);

			if (SETTINGS::settings.fakewalk)
			{
				if (GetAsyncKeyState(VK_SHIFT))
				{
					static int choked = 0;
					choked = choked > 7 ? 0 : choked + 1;
					GLOBAL::originalCMD.move.x = choked < 2 || choked > SETTINGS::settings.fakewalkspeed ? 0 : GLOBAL::originalCMD.move.x;
					GLOBAL::originalCMD.move.y = choked < 2 || choked > SETTINGS::settings.fakewalkspeed ? 0 : GLOBAL::originalCMD.move.y;
					GLOBAL::should_send_packet = choked < 1;
				}
			}

			if (SETTINGS::settings.autozeus_bool)
			{
				aimbot->autozeus(cmd);
			}
			if (SETTINGS::settings.autoknife_bool)
			{
				aimbot->autoknife(cmd);
			}
			if (SETTINGS::settings.novis_bool)
			{
				pAimPunch = (Vector*)((DWORD)local_player + OFFSETS::m_aimPunchAngle);
				pViewPunch = (Vector*)((DWORD)local_player + OFFSETS::m_viewPunchAngle);

				vecAimPunch = *pAimPunch;
				vecViewPunch = *pViewPunch;

				*pAimPunch = Vector(0, 0, 0);
				*pViewPunch = Vector(0, 0, 0);
			}

			for (int i = 1; i <= 65; i++)
			{
				auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

				if (!entity)
					continue;

				auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

				if (!local_player)
					return;

				bool is_local_player = entity == local_player;
				bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

				if (is_local_player)
					continue;

				if (is_teammate)
					continue;

				if (entity->GetHealth() <= 0)
					continue;

				if (entity->GetIsDormant())
					continue;

				if (SETTINGS::settings.stop_bool)
					movement->quick_stop(entity, cmd);
			}


			if (!GetAsyncKeyState(0x56))
				fakelag->do_fakelag(cmd);

			if (SETTINGS::settings.astro)
			{
				if (GLOBAL::originalCMD.move.x > 0)
				{
					cmd->buttons |= IN_BACK;
					cmd->buttons &= ~IN_FORWARD;
				}

				if (GLOBAL::originalCMD.move.x < 0)
				{
					cmd->buttons |= IN_FORWARD;
					cmd->buttons &= ~IN_BACK;
				}

				if (GLOBAL::originalCMD.move.y < 0)
				{
					cmd->buttons |= IN_MOVERIGHT;
					cmd->buttons &= ~IN_MOVELEFT;
				}

				if (GLOBAL::originalCMD.move.y > 0)
				{
					cmd->buttons |= IN_MOVELEFT;
					cmd->buttons &= ~IN_MOVERIGHT;
				}
			}
			static SDK::ConVar* ragdoll = INTERFACES::cvar->FindVar("cl_ragdoll_gravity");
			if (SETTINGS::settings.ragdoll_bool)
			{
				ragdoll->SetValue(-900);
			}
			else {
				ragdoll->SetValue(800);
			}

			static SDK::ConVar* impacts = INTERFACES::cvar->FindVar("sv_showimpacts");
			if (SETTINGS::settings.impacts)
			{
				impacts->SetValue(1);
			}
			else {
				impacts->SetValue(0);
			}


			if (SETTINGS::settings.bhop_bool) movement->bunnyhop(cmd);
			if (SETTINGS::settings.duck_bool) movement->duckinair(cmd);
			if (SETTINGS::settings.misc_clantag) visuals->Clantag();
			if (SETTINGS::settings.GameSenseClan) visuals->GameSense();
			if (SETTINGS::settings.Beta_AA) antiaim->BetaAA(cmd);

			if (SETTINGS::settings.prediction)
			{
				prediction->run_prediction(cmd);
				//movement->full_stop(cmd);
				if (SETTINGS::settings.BacktrackChams) backtracking->run_legit(cmd);
				prediction->end_prediction(cmd);
			}

			prediction->run_prediction(cmd);
			{
				if (SETTINGS::settings.auto_revolver) AutoRevolver(cmd);

				if (SETTINGS::settings.strafe_bool) movement->autostrafer(cmd);

				if (SETTINGS::settings.aim_type == 0 && SETTINGS::settings.aim_bool)
				{
					aimbot->run_aimbot(cmd);
					backtracking->run_legit(cmd);
				}

				if (SETTINGS::settings.aim_type == 1 && SETTINGS::settings.aim_bool)
				{
					if (GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.legittrigger_key)) && SETTINGS::settings.legittrigger_bool)
						legitbot->triggerbot(cmd);
					backtracking->run_legit(cmd);
				}

				if (SETTINGS::settings.aa_bool)
				{
					antiaim->do_antiaim(cmd);
					antiaim->fix_movement(cmd);
					//ground_ticks();
				}
			}
			prediction->end_prediction(cmd);

			if (!GLOBAL::should_send_packet)
				GLOBAL::real_angles = cmd->viewangles;
			else
			{
				GLOBAL::FakePosition = local_player->GetAbsOrigin();
				GLOBAL::fake_angles = cmd->viewangles;
			}

		}
		*SendPacket = GLOBAL::should_send_packet;
		cmd->move = antiaim->fix_movement(cmd, GLOBAL::originalCMD);
		if (SETTINGS::settings.aa_pitch < 2 || SETTINGS::settings.aa_pitch1_type < 2 || SETTINGS::settings.aa_pitch2_type < 2)
			UTILS::ClampLemon(cmd->viewangles);
		return false;
	}

	


	void __stdcall HookedPaintTraverse(int VGUIPanel, bool ForceRepaint, bool AllowForce)
	{
		static auto ofunc = panel.get_original<PaintTraverseFn>(41);
		std::string panel_name = INTERFACES::Panel->GetName(VGUIPanel);
		if (panel_name == "HudZoom" && SETTINGS::settings.scope_bool) return;
		if (panel_name == "FocusOverlayPanel")
		{
			if (FONTS::ShouldReloadFonts())
				FONTS::InitFonts();

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				GrenadePrediction::instance().Paint();
				visuals->DrawDamageIndicator();
				visuals->AsusProps();


				auto matpostprocess = INTERFACES::cvar->FindVar("mat_postprocess_enable");
				matpostprocess->fnChangeCallback = 0;
				matpostprocess->SetValue(SETTINGS::settings.matpostprocessenable);

				visuals->ModulateWorld();

				if (SETTINGS::settings.esp_bool)
				{
					visuals->Draw();
					visuals->ClientDraw();
				}
				if (SETTINGS::settings.hitmarker)
				{
					pHitmarker->Paint();
				}
				Flashlight.RunFrame();
			}

			MENU::PPGUI_PP_GUI::Begin();
			MENU::Do();
			MENU::PPGUI_PP_GUI::End();

			UTILS::INPUT::input_handler.Update();

			visuals->LogEvents();
		}
		ofunc(INTERFACES::Panel, VGUIPanel, ForceRepaint, AllowForce);
	}
	void __fastcall HookedFrameStageNotify(void* ecx, void* edx, int stage)
	{
		static auto ofunc = client.get_original<FrameStageNotifyFn>(37);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!local_player) return;
		Vector vecAimPunch, vecViewPunch;
		Vector* pAimPunch = nullptr; Vector* pViewPunch = nullptr;
		visuals->ModulateSky();
		visuals->DoFSN();
		switch (stage)
		{
			case FRAME_NET_UPDATE_POSTDATAUPDATE_START:

				skinchanger();
				GloveChanger();

			    if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
				{
					for (int i = 1; i <= 65; i++)
					{
						auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
						if (!entity) continue;

						bool is_local_player = entity == local_player;
						bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

						if (is_local_player) continue;
						if (is_teammate) continue;
						if (entity->GetHealth() <= 0) continue;
						if (entity->GetIsDormant()) continue;

						if (SETTINGS::settings.ResolverEnable == 0) return;
						if (SETTINGS::settings.ResolverEnable == 1) {
							resolver->resolve(entity);
						}
						if (SETTINGS::settings.ResolverEnable == 2) {
							resolver2->resolve2(entity);
						}
						if (SETTINGS::settings.ResolverEnable == 3) {
							resolver3->resolve3(entity);
						}
					}
				} break;
			case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
				break;
			case FRAME_RENDER_START:
				if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
				{
					if (in_tp)
					{
						*(Vector*)((DWORD)local_player + 0x31C8) = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0.f);

						INTERFACES::pPrediction->SetLocalViewAngles(GLOBAL::real_angles);
						local_player->UpdateClientSideAnimation();
						INTERFACES::pPrediction->SetLocalViewAngles(GLOBAL::fake_angles);

					}
					for (int i = 1; i <= 65; i++)
					{
						auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
						if (!entity) continue;
						if (entity == local_player) continue;

						*(int*)((uintptr_t)entity + 0xA30) = INTERFACES::Globals->framecount;
						*(int*)((uintptr_t)entity + 0xA28) = 0;
					}
				} break;

			case FRAME_NET_UPDATE_START:
				if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
				{
					if (SETTINGS::settings.beam_bool)
						visuals->DrawBulletBeams();
				} break;
			case FRAME_NET_UPDATE_END:
				if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
				{
					for (int i = 1; i < 65; i++)
					{
						auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

						if (!entity)
							continue;

						if (!local_player)
							continue;

						bool is_local_player = entity == local_player;
						bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

						if (is_local_player)
							continue;

						if (is_teammate)
							continue;

						if (entity->GetHealth() <= 0)
							continue;
					}
				}
				break;
		}
		ofunc(ecx, stage);
	}
	void __fastcall HookedDrawModelExecute(void* ecx, void* edx, SDK::IMatRenderContext* context, const SDK::DrawModelState_t& state, const SDK::ModelRenderInfo_t& render_info, matrix3x4_t* matrix)
	{
		static auto ofunc = modelrender.get_original<DrawModelExecuteFn>(21);
		if (SETTINGS::settings.wiresleeve_bool)
		{
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

				if (SETTINGS::settings.noflash)
				{
					if (true)
					{
						*(float*)((DWORD)local_player + 0xA2FA) = 0;
					}
				}
				std::string ModelName = INTERFACES::ModelInfo->GetModelName(render_info.pModel);

				if (ModelName.find("v_sleeve") != std::string::npos)
				{
					if (ModelName.find("v_sleeve") != std::string::npos)
					{
						SDK::IMaterial* material = INTERFACES::MaterialSystem->FindMaterial(ModelName.c_str(), TEXTURE_GROUP_MODEL);
						if (!material) return;
						material->SetMaterialVarFlag(SDK::MATERIAL_VAR_WIREFRAME, true);
						INTERFACES::ModelRender->ForcedMaterialOverride(material);
					}
				}
			}
		}
		std::string strModelName = INTERFACES::ModelInfo->GetModelName(render_info.pModel);
		if (SETTINGS::settings.wirehand_bool)
		{
			if (strModelName.find("arms") != std::string::npos && SETTINGS::settings.wirehand_bool)
			{
				SDK::IMaterial* WireHands = INTERFACES::MaterialSystem->FindMaterial(strModelName.c_str(), TEXTURE_GROUP_MODEL);
				WireHands->SetMaterialVarFlag(SDK::MATERIAL_VAR_WIREFRAME, true);
				INTERFACES::ModelRender->ForcedMaterialOverride(WireHands);
			}
		}
		ofunc(ecx, context, state, render_info, matrix);
	}

	void __fastcall HookedSceneEnd(void* ecx, void* edx)
	{
		static auto ofunc = sceneend.get_original<SceneEndFn>(9);
		ofunc(ecx);
		static SDK::IMaterial* ignorez = chams->CreateMaterialBasic(true, true, false);
		static SDK::IMaterial* notignorez = chams->CreateMaterialBasic(false, true, false);
		static SDK::IMaterial* ignorez_metallic = chams->CreateMaterialMetallic(true, true, false);
		static SDK::IMaterial* notignorez_metallic = chams->CreateMaterialMetallic(false, true, false);

		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
			if (!local_player) return;
			CColor color = SETTINGS::settings.glow_col, colorTeam = SETTINGS::settings.teamglow_color, colorlocal = SETTINGS::settings.glowlocal_col;
			for (int i = 1; i < 65; i++)
			{
				auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

				if (!entity) continue;
				if (!local_player) continue;

				bool is_local_player = entity == local_player;
				bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;
				auto ignorezmaterial = SETTINGS::settings.chamstype == 0 ? ignorez_metallic : ignorez;
				auto notignorezmaterial = SETTINGS::settings.chamstype == 0 ? notignorez_metallic : notignorez;
				if (is_local_player)
				{
					switch (SETTINGS::settings.localchams)
					{
						case 0: continue; break;
						case 1:
							local_player->SetAbsOrigin(GLOBAL::FakePosition);
							local_player->DrawModel(0x1, 255);
							local_player->SetAbsOrigin(local_player->GetAbsOrigin());
							continue; break;
						case 2:
							notignorezmaterial->ColorModulate(SETTINGS::settings.localchams_col);
							INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
							local_player->DrawModel(0x1, 255);
							INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
							continue;  break;
						case 3:
							notignorezmaterial->ColorModulate(SETTINGS::settings.localchams_col);
							INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
							local_player->SetAbsOrigin(GLOBAL::FakePosition);
							local_player->DrawModel(0x1, 255);
							local_player->SetAbsOrigin(local_player->GetAbsOrigin());
							INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
							continue; break;
					}
				}

				if (entity->GetHealth() <= 0) continue;
				if (entity->GetIsDormant())	continue;
				if (entity->GetClientClass()->m_ClassID != 35) continue;
				if (SETTINGS::settings.draw_fake)
				{
					if (local_player)
					{
						if (notignorez)
						{
							Vector OrigAng;
							OrigAng = local_player->GetEyeAngles();
							local_player->SetAngle2(Vector(0, local_player->GetLowerBodyYaw(), 0));
							bool LbyColor = false;
							float NormalColor[3] = { 1,1,1 };
							float lbyupdatecolor[3] = { 0,1,0 };
							INTERFACES::RenderView->SetColorModulation(SETTINGS::settings.fake_darw_col);
							INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
							local_player->DrawModel(0x1, 255);
							INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
							local_player->SetAngle2(OrigAng);
						}
					}
				}

				if (is_teammate)
				{
					if (entity && SETTINGS::settings.chamsteam == 2)
					{
						ignorezmaterial->ColorModulate(SETTINGS::settings.teaminvis_color);
						INTERFACES::ModelRender->ForcedMaterialOverride(ignorezmaterial);
						entity->DrawModel(0x1, 255);

						notignorezmaterial->ColorModulate(SETTINGS::settings.teamvis_color);
						INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
						entity->DrawModel(0x1, 255);

						INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
					}
					else if (entity && SETTINGS::settings.chamsteam == 1)
					{
						notignorezmaterial->ColorModulate(SETTINGS::settings.teamvis_color);
						INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
						entity->DrawModel(0x1, 255);

						INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
					} continue;
				}
				else if (is_teammate && SETTINGS::settings.chamsteam)
					continue;

				if (entity && SETTINGS::settings.chams_type == 2)
				{
					ignorezmaterial->ColorModulate(SETTINGS::settings.imodel_col);
					INTERFACES::ModelRender->ForcedMaterialOverride(ignorezmaterial);
					entity->DrawModel(0x1, 255);

					notignorezmaterial->ColorModulate(SETTINGS::settings.vmodel_col);
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
					entity->DrawModel(0x1, 255);

					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
				else if (entity && SETTINGS::settings.chams_type == 1)
				{
					notignorezmaterial->ColorModulate(SETTINGS::settings.vmodel_col);
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorezmaterial);
					entity->DrawModel(0x1, 255);

					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
			}

			for (auto i = 0; i < INTERFACES::GlowObjManager->GetSize(); i++)
			{
				auto &glowObject = INTERFACES::GlowObjManager->m_GlowObjectDefinitions[i];
				auto entity = reinterpret_cast<SDK::CBaseEntity*>(glowObject.m_pEntity);

				if (!entity) continue;
				if (!local_player) continue;

				if (glowObject.IsUnused()) continue;

				bool is_local_player = entity == local_player;
				bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

				if (is_local_player && in_tp && SETTINGS::settings.glowlocal)
				{
					glowObject.m_nGlowStyle = SETTINGS::settings.glowstylelocal;
					glowObject.m_flRed = colorlocal.RGBA[0] / 255.0f;
					glowObject.m_flGreen = colorlocal.RGBA[1] / 255.0f;
					glowObject.m_flBlue = colorlocal.RGBA[2] / 255.0f;
					glowObject.m_flAlpha = colorlocal.RGBA[3] / 255.0f;
					glowObject.m_bRenderWhenOccluded = true;
					glowObject.m_bRenderWhenUnoccluded = false;
					continue;
				}
				else if (!SETTINGS::settings.glowlocal && is_local_player)
					continue;

				if (entity->GetHealth() <= 0) continue;
				if (entity->GetIsDormant())	continue;
				if (entity->GetClientClass()->m_ClassID != 35) continue;

				if (is_teammate && SETTINGS::settings.glowteam)
				{
					glowObject.m_nGlowStyle = SETTINGS::settings.glowstyle;
					glowObject.m_flRed = colorTeam.RGBA[0] / 255.0f;
					glowObject.m_flGreen = colorTeam.RGBA[1] / 255.0f;
					glowObject.m_flBlue = colorTeam.RGBA[2] / 255.0f;
					glowObject.m_flAlpha = colorTeam.RGBA[3] / 255.0f;
					glowObject.m_bRenderWhenOccluded = true;
					glowObject.m_bRenderWhenUnoccluded = false;
					continue;
				}
				else if (is_teammate && !SETTINGS::settings.glowteam)
					continue;

				if (SETTINGS::settings.glowenable)
				{
					glowObject.m_nGlowStyle = SETTINGS::settings.glowstyle;
					glowObject.m_flRed = color.RGBA[0] / 255.0f;
					glowObject.m_flGreen = color.RGBA[1] / 255.0f;
					glowObject.m_flBlue = color.RGBA[2] / 255.0f;
					glowObject.m_flAlpha = color.RGBA[3] / 255.0f;
					glowObject.m_bRenderWhenOccluded = true;
					glowObject.m_bRenderWhenUnoccluded = false;
				}
			}

			if (SETTINGS::settings.smoke_bool)
			{
				std::vector<const char*> vistasmoke_wireframe =  { "particle/vistasmokev1/vistasmokev1_smokegrenade" };

				std::vector<const char*> vistasmoke_nodraw =
				{
					"particle/vistasmokev1/vistasmokev1_fire",
					"particle/vistasmokev1/vistasmokev1_emods",
					"particle/vistasmokev1/vistasmokev1_emods_impactdust",
				};

				for (auto mat_s : vistasmoke_wireframe)
				{
					SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_s, TEXTURE_GROUP_OTHER);
					mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_WIREFRAME, true);
				}

				for (auto mat_n : vistasmoke_nodraw)
				{
					SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_n, TEXTURE_GROUP_OTHER);
					mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_NO_DRAW, true);
				}

				static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);
				*(int*)(smokecout) = 0;
			}
		}
	}


	void __fastcall HookedOverrideView(void* ecx, void* edx, SDK::CViewSetup* pSetup)
	{
		static auto ofunc = overrideview.get_original<OverrideViewFn>(18);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!local_player) return;

		auto animstate = local_player->GetAnimState();
		if (!animstate) return;

		if (GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.thirdperson_int)) & 1)
			in_tp = !in_tp;

		if (SETTINGS::settings.novisualrecoil_bool)
		{
			pSetup->angles -= *local_player->GetAimPunchAngle() * 0.9f + *local_player->GetViewPunchAngle();
		}

		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			GrenadePrediction::instance().View(pSetup);
			auto GetCorrectDistance = [&local_player](float ideal_distance) -> float
			{
				Vector inverse_angles;
				INTERFACES::Engine->GetViewAngles(inverse_angles);

				inverse_angles.x *= -1.f, inverse_angles.y += 180.f;

				Vector direction;
				MATH::AngleVectors(inverse_angles, &direction);

				SDK::CTraceWorldOnly filter;
				SDK::trace_t trace;
				SDK::Ray_t ray;

				ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), (local_player->GetVecOrigin() + local_player->GetViewOffset()) + (direction * (ideal_distance + 5.f)));
				INTERFACES::Trace->TraceRay(ray, MASK_ALL, &filter, &trace);

				return ideal_distance * trace.flFraction;
			};

			if (SETTINGS::settings.tp_bool && in_tp)
			{
				if (local_player->GetHealth() <= 0)
					local_player->SetObserverMode(5);

				if (!INTERFACES::Input->m_fCameraInThirdPerson)
				{
					INTERFACES::Input->m_fCameraInThirdPerson = true;
					if (animstate->m_bInHitGroundAnimation && ground_tick > 1)
						INTERFACES::Input->m_vecCameraOffset = Vector(0.0f, GLOBAL::real_angles.y, GetCorrectDistance(100));
					else
						INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, GetCorrectDistance(100));
					Vector camForward;

					MATH::AngleVectors(Vector(INTERFACES::Input->m_vecCameraOffset.x, INTERFACES::Input->m_vecCameraOffset.y, 0), &camForward);
				}
			}
			else
			{
				INTERFACES::Input->m_fCameraInThirdPerson = false;
				INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0);
			}

			if (!local_player->GetIsScoped())
			{
				if (SETTINGS::settings.vfov_val == 0)
					pSetup->fov = 90;
				else
					pSetup->fov = SETTINGS::settings.vfov_val; //110, 90 default
			}

			auto zoomsensration = INTERFACES::cvar->FindVar("zoom_sensitivity_ratio_mouse");
			if (SETTINGS::settings.fixscopesens)
				zoomsensration->SetValue("0");
			else
				zoomsensration->SetValue("1");

			if (SETTINGS::settings.aim_type == 0)
			{
				if (!local_player->GetIsScoped())
					pSetup->fov = SETTINGS::settings.fov_val;
				else if (local_player->GetIsScoped() && SETTINGS::settings.removescoping)
					pSetup->fov = SETTINGS::settings.fov_val;
			}
			else if (!(SETTINGS::settings.aim_type == 0) && !local_player->GetIsScoped())
				pSetup->fov = 90;
		}
		ofunc(ecx, pSetup);
	}
	void __fastcall HookedRenderView(void* thisptr, void*, SDK::CViewSetup& setup, SDK::CViewSetup& hudViewSetup, unsigned int nClearFlags, int whatToDraw)
	{
		if (INTERFACES::Engine->IsInGame())
		{
			auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

			if (!local_player)
				return;

			if (!local_player->IsAlive())
				return;

			if (!local_player->GetIsScoped())
				setup.fov += SETTINGS::settings.fov_val;

			setup.fovViewmodel += SETTINGS::settings.vfov_val;
		}
		original_render_view(thisptr, setup, hudViewSetup, nClearFlags, whatToDraw);
	}
	void __fastcall HookedTraceRay(void *thisptr, void*, const SDK::Ray_t &ray, unsigned int fMask, SDK::ITraceFilter *pTraceFilter, SDK::trace_t *pTrace)
	{
		static auto ofunc = trace.get_original<TraceRayFn>(5);
		ofunc(thisptr, ray, fMask, pTraceFilter, pTrace);
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			pTrace->surface.flags |= SURF_SKY;
	}
	bool __fastcall HookedGetBool(void* pConVar, void* edx)
	{
		static auto ofunc = getbool.get_original<SvCheatsGetBoolFn>(13);
		if ((uintptr_t)_ReturnAddress() == CAM_THINK)
			return true;

		return ofunc(pConVar);
	}
	float __fastcall GetViewmodelFOV()
	{
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			float player_fov = original_viewmodel_fov();

			if (SETTINGS::settings.esp_bool)
				player_fov = SETTINGS::settings.viewfov_val;

			return player_fov;
		}
	}

	void Hook_LockCursor(void* xd)
	{
		SurfaceTable.UnHook();
		INTERFACES::Surface->lockcursor();
		SurfaceTable.ReHook();
		if (menu_open)
			INTERFACES::Surface->unlockcursor();
	}

	void InitHooks()
	{
		client.setup(INTERFACES::Client);
		client.hook_index(37, HookedFrameStageNotify);

		panel.setup(INTERFACES::Panel);
		panel.hook_index(41, HookedPaintTraverse);

		modelrender.setup(INTERFACES::ModelRender);
		modelrender.hook_index(21, HookedDrawModelExecute);
		
		sceneend.setup(INTERFACES::RenderView);
		sceneend.hook_index(9, HookedSceneEnd);

		trace.setup(INTERFACES::Trace);
		trace.hook_index(5, HookedTraceRay);

		overrideview.setup(INTERFACES::ClientMode);
		overrideview.hook_index(18, HookedOverrideView);
		overrideview.hook_index(24, HookedCreateMove);

		override_view_hook_manager.Init(INTERFACES::ClientMode);
		original_override_view = reinterpret_cast<OverrideViewFn>(override_view_hook_manager.HookFunction<OverrideViewFn>(18, HookedOverrideView));
		original_create_move = reinterpret_cast<CreateMoveFn>(override_view_hook_manager.HookFunction<CreateMoveFn>(24, HookedCreateMove));
		original_viewmodel_fov = reinterpret_cast<GetViewmodelFOVFn>(override_view_hook_manager.HookFunction<GetViewmodelFOVFn>(35, GetViewmodelFOV));

		getbool.setup(reinterpret_cast<DWORD**>(INTERFACES::cvar->FindVar("sv_cheats")));
		getbool.hook_index(13, HookedGetBool);



		SurfaceTable.InitTable(INTERFACES::Surface);
		SurfaceTable.HookIndex(67, Hook_LockCursor);

	}
	void EyeAnglesPitchHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;

	}
	void EyeAnglesYawHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;
	}
	void InitNetvarHooks()
	{
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[0]", EyeAnglesPitchHook);
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[1]", EyeAnglesYawHook);
	}
}




// Junk Code By Troll Face & Thaisen's Gen
void hLZnRcdUJz75048778() {     int OIddXLbEbd39301456 = -691761276;    int OIddXLbEbd15751462 = -580615176;    int OIddXLbEbd70488527 = -600147297;    int OIddXLbEbd70236453 = -836644140;    int OIddXLbEbd27281883 = -662169356;    int OIddXLbEbd13597128 = -850941353;    int OIddXLbEbd43279401 = -103387320;    int OIddXLbEbd74748983 = -558868204;    int OIddXLbEbd47175662 = 96749933;    int OIddXLbEbd47972315 = -196883649;    int OIddXLbEbd74333256 = -332841529;    int OIddXLbEbd40450661 = -684171986;    int OIddXLbEbd6735816 = -576473770;    int OIddXLbEbd73512743 = -36802345;    int OIddXLbEbd28528115 = -12476788;    int OIddXLbEbd66546607 = -514197047;    int OIddXLbEbd94430254 = -448873133;    int OIddXLbEbd30269057 = -532799189;    int OIddXLbEbd99483424 = -901632130;    int OIddXLbEbd78475955 = -983573009;    int OIddXLbEbd47355003 = -223816712;    int OIddXLbEbd88975550 = -827271681;    int OIddXLbEbd86860378 = -680960376;    int OIddXLbEbd51966865 = -909494175;    int OIddXLbEbd32952137 = -434859423;    int OIddXLbEbd39827880 = -379616015;    int OIddXLbEbd85745394 = -720878470;    int OIddXLbEbd91767107 = -802495714;    int OIddXLbEbd32599902 = -30618125;    int OIddXLbEbd21522113 = -283948789;    int OIddXLbEbd79187095 = -954234766;    int OIddXLbEbd61876344 = -823689032;    int OIddXLbEbd85241119 = -94878951;    int OIddXLbEbd2712995 = -774185384;    int OIddXLbEbd7805424 = -761308897;    int OIddXLbEbd53021391 = -344914005;    int OIddXLbEbd64334862 = -479286589;    int OIddXLbEbd3169943 = -47847501;    int OIddXLbEbd99884062 = -241391565;    int OIddXLbEbd19604767 = -965237477;    int OIddXLbEbd32446342 = -56402251;    int OIddXLbEbd92671803 = -980289050;    int OIddXLbEbd99300414 = -562173300;    int OIddXLbEbd86680933 = -977904944;    int OIddXLbEbd71947588 = 65116223;    int OIddXLbEbd83231243 = 49605372;    int OIddXLbEbd42148714 = -919581022;    int OIddXLbEbd2057566 = -138455766;    int OIddXLbEbd90357256 = -750792562;    int OIddXLbEbd32923622 = -476519592;    int OIddXLbEbd4811098 = -660852335;    int OIddXLbEbd11895238 = -911450861;    int OIddXLbEbd19981549 = -288252349;    int OIddXLbEbd45734153 = -819574144;    int OIddXLbEbd27693776 = -21785857;    int OIddXLbEbd50325906 = -864489596;    int OIddXLbEbd28891083 = -899654800;    int OIddXLbEbd18521662 = -690653122;    int OIddXLbEbd37284317 = -301784717;    int OIddXLbEbd87454003 = -182553341;    int OIddXLbEbd27851734 = -30062884;    int OIddXLbEbd51512293 = -300891607;    int OIddXLbEbd42149081 = -428250079;    int OIddXLbEbd25653549 = -619301278;    int OIddXLbEbd68785220 = -242648883;    int OIddXLbEbd12456913 = -509152497;    int OIddXLbEbd55209541 = -489293036;    int OIddXLbEbd4022822 = -802288387;    int OIddXLbEbd65707320 = -275493449;    int OIddXLbEbd75506724 = -667562784;    int OIddXLbEbd2211745 = 65089541;    int OIddXLbEbd91260311 = -301025632;    int OIddXLbEbd30384995 = -191407624;    int OIddXLbEbd79878658 = -936394653;    int OIddXLbEbd46029613 = -827170759;    int OIddXLbEbd54683199 = -243527663;    int OIddXLbEbd89675135 = -165098381;    int OIddXLbEbd179446 = -703055433;    int OIddXLbEbd80019277 = -874610399;    int OIddXLbEbd49720894 = -384464796;    int OIddXLbEbd97679165 = -460034993;    int OIddXLbEbd83687828 = -482422704;    int OIddXLbEbd1409851 = 48296847;    int OIddXLbEbd99676280 = -554098533;    int OIddXLbEbd16711016 = -623096455;    int OIddXLbEbd67291857 = 57216094;    int OIddXLbEbd41894795 = -435436684;    int OIddXLbEbd39506967 = -275304807;    int OIddXLbEbd75019219 = -652399527;    int OIddXLbEbd57479518 = -896819301;    int OIddXLbEbd24130308 = -445259205;    int OIddXLbEbd45813201 = -788633467;    int OIddXLbEbd65885626 = -746062785;    int OIddXLbEbd12430060 = 41161776;    int OIddXLbEbd91753032 = -835174594;    int OIddXLbEbd80934048 = -755510645;    int OIddXLbEbd50522723 = -452038971;    int OIddXLbEbd73646865 = -942872023;    int OIddXLbEbd17895714 = -635256061;    int OIddXLbEbd59490676 = -691761276;     OIddXLbEbd39301456 = OIddXLbEbd15751462;     OIddXLbEbd15751462 = OIddXLbEbd70488527;     OIddXLbEbd70488527 = OIddXLbEbd70236453;     OIddXLbEbd70236453 = OIddXLbEbd27281883;     OIddXLbEbd27281883 = OIddXLbEbd13597128;     OIddXLbEbd13597128 = OIddXLbEbd43279401;     OIddXLbEbd43279401 = OIddXLbEbd74748983;     OIddXLbEbd74748983 = OIddXLbEbd47175662;     OIddXLbEbd47175662 = OIddXLbEbd47972315;     OIddXLbEbd47972315 = OIddXLbEbd74333256;     OIddXLbEbd74333256 = OIddXLbEbd40450661;     OIddXLbEbd40450661 = OIddXLbEbd6735816;     OIddXLbEbd6735816 = OIddXLbEbd73512743;     OIddXLbEbd73512743 = OIddXLbEbd28528115;     OIddXLbEbd28528115 = OIddXLbEbd66546607;     OIddXLbEbd66546607 = OIddXLbEbd94430254;     OIddXLbEbd94430254 = OIddXLbEbd30269057;     OIddXLbEbd30269057 = OIddXLbEbd99483424;     OIddXLbEbd99483424 = OIddXLbEbd78475955;     OIddXLbEbd78475955 = OIddXLbEbd47355003;     OIddXLbEbd47355003 = OIddXLbEbd88975550;     OIddXLbEbd88975550 = OIddXLbEbd86860378;     OIddXLbEbd86860378 = OIddXLbEbd51966865;     OIddXLbEbd51966865 = OIddXLbEbd32952137;     OIddXLbEbd32952137 = OIddXLbEbd39827880;     OIddXLbEbd39827880 = OIddXLbEbd85745394;     OIddXLbEbd85745394 = OIddXLbEbd91767107;     OIddXLbEbd91767107 = OIddXLbEbd32599902;     OIddXLbEbd32599902 = OIddXLbEbd21522113;     OIddXLbEbd21522113 = OIddXLbEbd79187095;     OIddXLbEbd79187095 = OIddXLbEbd61876344;     OIddXLbEbd61876344 = OIddXLbEbd85241119;     OIddXLbEbd85241119 = OIddXLbEbd2712995;     OIddXLbEbd2712995 = OIddXLbEbd7805424;     OIddXLbEbd7805424 = OIddXLbEbd53021391;     OIddXLbEbd53021391 = OIddXLbEbd64334862;     OIddXLbEbd64334862 = OIddXLbEbd3169943;     OIddXLbEbd3169943 = OIddXLbEbd99884062;     OIddXLbEbd99884062 = OIddXLbEbd19604767;     OIddXLbEbd19604767 = OIddXLbEbd32446342;     OIddXLbEbd32446342 = OIddXLbEbd92671803;     OIddXLbEbd92671803 = OIddXLbEbd99300414;     OIddXLbEbd99300414 = OIddXLbEbd86680933;     OIddXLbEbd86680933 = OIddXLbEbd71947588;     OIddXLbEbd71947588 = OIddXLbEbd83231243;     OIddXLbEbd83231243 = OIddXLbEbd42148714;     OIddXLbEbd42148714 = OIddXLbEbd2057566;     OIddXLbEbd2057566 = OIddXLbEbd90357256;     OIddXLbEbd90357256 = OIddXLbEbd32923622;     OIddXLbEbd32923622 = OIddXLbEbd4811098;     OIddXLbEbd4811098 = OIddXLbEbd11895238;     OIddXLbEbd11895238 = OIddXLbEbd19981549;     OIddXLbEbd19981549 = OIddXLbEbd45734153;     OIddXLbEbd45734153 = OIddXLbEbd27693776;     OIddXLbEbd27693776 = OIddXLbEbd50325906;     OIddXLbEbd50325906 = OIddXLbEbd28891083;     OIddXLbEbd28891083 = OIddXLbEbd18521662;     OIddXLbEbd18521662 = OIddXLbEbd37284317;     OIddXLbEbd37284317 = OIddXLbEbd87454003;     OIddXLbEbd87454003 = OIddXLbEbd27851734;     OIddXLbEbd27851734 = OIddXLbEbd51512293;     OIddXLbEbd51512293 = OIddXLbEbd42149081;     OIddXLbEbd42149081 = OIddXLbEbd25653549;     OIddXLbEbd25653549 = OIddXLbEbd68785220;     OIddXLbEbd68785220 = OIddXLbEbd12456913;     OIddXLbEbd12456913 = OIddXLbEbd55209541;     OIddXLbEbd55209541 = OIddXLbEbd4022822;     OIddXLbEbd4022822 = OIddXLbEbd65707320;     OIddXLbEbd65707320 = OIddXLbEbd75506724;     OIddXLbEbd75506724 = OIddXLbEbd2211745;     OIddXLbEbd2211745 = OIddXLbEbd91260311;     OIddXLbEbd91260311 = OIddXLbEbd30384995;     OIddXLbEbd30384995 = OIddXLbEbd79878658;     OIddXLbEbd79878658 = OIddXLbEbd46029613;     OIddXLbEbd46029613 = OIddXLbEbd54683199;     OIddXLbEbd54683199 = OIddXLbEbd89675135;     OIddXLbEbd89675135 = OIddXLbEbd179446;     OIddXLbEbd179446 = OIddXLbEbd80019277;     OIddXLbEbd80019277 = OIddXLbEbd49720894;     OIddXLbEbd49720894 = OIddXLbEbd97679165;     OIddXLbEbd97679165 = OIddXLbEbd83687828;     OIddXLbEbd83687828 = OIddXLbEbd1409851;     OIddXLbEbd1409851 = OIddXLbEbd99676280;     OIddXLbEbd99676280 = OIddXLbEbd16711016;     OIddXLbEbd16711016 = OIddXLbEbd67291857;     OIddXLbEbd67291857 = OIddXLbEbd41894795;     OIddXLbEbd41894795 = OIddXLbEbd39506967;     OIddXLbEbd39506967 = OIddXLbEbd75019219;     OIddXLbEbd75019219 = OIddXLbEbd57479518;     OIddXLbEbd57479518 = OIddXLbEbd24130308;     OIddXLbEbd24130308 = OIddXLbEbd45813201;     OIddXLbEbd45813201 = OIddXLbEbd65885626;     OIddXLbEbd65885626 = OIddXLbEbd12430060;     OIddXLbEbd12430060 = OIddXLbEbd91753032;     OIddXLbEbd91753032 = OIddXLbEbd80934048;     OIddXLbEbd80934048 = OIddXLbEbd50522723;     OIddXLbEbd50522723 = OIddXLbEbd73646865;     OIddXLbEbd73646865 = OIddXLbEbd17895714;     OIddXLbEbd17895714 = OIddXLbEbd59490676;     OIddXLbEbd59490676 = OIddXLbEbd39301456;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ENJUBUUykc81047128() {     int exkMwwxdSr48616722 = -594292302;    int exkMwwxdSr71174384 = -597149679;    int exkMwwxdSr90640607 = -313787910;    int exkMwwxdSr28026476 = 71691868;    int exkMwwxdSr67345122 = -638684513;    int exkMwwxdSr7140490 = -309810214;    int exkMwwxdSr16933473 = 97363802;    int exkMwwxdSr63885139 = -755827352;    int exkMwwxdSr39984319 = -283310738;    int exkMwwxdSr16596773 = -17679317;    int exkMwwxdSr11235241 = -524082601;    int exkMwwxdSr51688410 = -221541801;    int exkMwwxdSr2517532 = -161895905;    int exkMwwxdSr5601385 = -15856450;    int exkMwwxdSr43804153 = -964007199;    int exkMwwxdSr71517131 = -541720525;    int exkMwwxdSr38241022 = -476458220;    int exkMwwxdSr42461698 = -262856502;    int exkMwwxdSr81496356 = -855272959;    int exkMwwxdSr41671303 = 37719149;    int exkMwwxdSr36034354 = -549516640;    int exkMwwxdSr13184463 = -650168013;    int exkMwwxdSr22921646 = -295489376;    int exkMwwxdSr11173495 = -426655285;    int exkMwwxdSr96014679 = -753992474;    int exkMwwxdSr5735698 = -478827656;    int exkMwwxdSr19503662 = -429767314;    int exkMwwxdSr62861583 = -209632447;    int exkMwwxdSr99716808 = -145037561;    int exkMwwxdSr34303061 = -90033352;    int exkMwwxdSr49185474 = -564886583;    int exkMwwxdSr39135707 = -154860327;    int exkMwwxdSr35100449 = 94030037;    int exkMwwxdSr91741943 = -257745270;    int exkMwwxdSr94455699 = -937721491;    int exkMwwxdSr84846285 = -761139452;    int exkMwwxdSr1802471 = -80486452;    int exkMwwxdSr59206741 = -900442864;    int exkMwwxdSr66109763 = -292607352;    int exkMwwxdSr95704663 = -672422990;    int exkMwwxdSr56509476 = -193558669;    int exkMwwxdSr80323459 = -901561141;    int exkMwwxdSr21017421 = -784053682;    int exkMwwxdSr45437834 = -368299832;    int exkMwwxdSr7897663 = -826659571;    int exkMwwxdSr53470479 = -275929186;    int exkMwwxdSr47398345 = -492415168;    int exkMwwxdSr73528603 = -584999154;    int exkMwwxdSr56384401 = -986489078;    int exkMwwxdSr2561524 = -269533294;    int exkMwwxdSr41778806 = -396728332;    int exkMwwxdSr76112280 = -459437078;    int exkMwwxdSr7989207 = -976029255;    int exkMwwxdSr50315396 = -79427262;    int exkMwwxdSr45012775 = 43296059;    int exkMwwxdSr35432260 = -944124290;    int exkMwwxdSr48252739 = -201660303;    int exkMwwxdSr79467113 = -887132625;    int exkMwwxdSr32011796 = -174315659;    int exkMwwxdSr61609425 = -59856858;    int exkMwwxdSr87636828 = -880042900;    int exkMwwxdSr54071890 = -693003752;    int exkMwwxdSr64168330 = -510789792;    int exkMwwxdSr5681259 = -93277386;    int exkMwwxdSr67411298 = -452792734;    int exkMwwxdSr72099533 = -269222275;    int exkMwwxdSr16587961 = -215571838;    int exkMwwxdSr10775589 = -904150635;    int exkMwwxdSr11145686 = -78134959;    int exkMwwxdSr58957867 = -102867748;    int exkMwwxdSr69714661 = -361234074;    int exkMwwxdSr79034280 = -576015356;    int exkMwwxdSr76351934 = -970249150;    int exkMwwxdSr85791692 = -82849970;    int exkMwwxdSr85161827 = -768722182;    int exkMwwxdSr55710894 = -647955500;    int exkMwwxdSr92167041 = -866114332;    int exkMwwxdSr77483811 = -927189545;    int exkMwwxdSr3275832 = -599995715;    int exkMwwxdSr42544201 = -378063288;    int exkMwwxdSr58337352 = -986412489;    int exkMwwxdSr45975058 = -844768160;    int exkMwwxdSr6477183 = -223143369;    int exkMwwxdSr97155285 = -875504267;    int exkMwwxdSr92524254 = -693305021;    int exkMwwxdSr73073194 = -5449505;    int exkMwwxdSr31146501 = -178831072;    int exkMwwxdSr84785052 = -826542702;    int exkMwwxdSr46729168 = -201041330;    int exkMwwxdSr59023439 = -993597202;    int exkMwwxdSr36593547 = -459479149;    int exkMwwxdSr22335357 = -193353827;    int exkMwwxdSr27194945 = -626127206;    int exkMwwxdSr4500339 = -132750495;    int exkMwwxdSr8067835 = -792380091;    int exkMwwxdSr2437587 = -500554917;    int exkMwwxdSr16155130 = -290771350;    int exkMwwxdSr15336163 = -590776296;    int exkMwwxdSr78026535 = -915507098;    int exkMwwxdSr35798129 = -594292302;     exkMwwxdSr48616722 = exkMwwxdSr71174384;     exkMwwxdSr71174384 = exkMwwxdSr90640607;     exkMwwxdSr90640607 = exkMwwxdSr28026476;     exkMwwxdSr28026476 = exkMwwxdSr67345122;     exkMwwxdSr67345122 = exkMwwxdSr7140490;     exkMwwxdSr7140490 = exkMwwxdSr16933473;     exkMwwxdSr16933473 = exkMwwxdSr63885139;     exkMwwxdSr63885139 = exkMwwxdSr39984319;     exkMwwxdSr39984319 = exkMwwxdSr16596773;     exkMwwxdSr16596773 = exkMwwxdSr11235241;     exkMwwxdSr11235241 = exkMwwxdSr51688410;     exkMwwxdSr51688410 = exkMwwxdSr2517532;     exkMwwxdSr2517532 = exkMwwxdSr5601385;     exkMwwxdSr5601385 = exkMwwxdSr43804153;     exkMwwxdSr43804153 = exkMwwxdSr71517131;     exkMwwxdSr71517131 = exkMwwxdSr38241022;     exkMwwxdSr38241022 = exkMwwxdSr42461698;     exkMwwxdSr42461698 = exkMwwxdSr81496356;     exkMwwxdSr81496356 = exkMwwxdSr41671303;     exkMwwxdSr41671303 = exkMwwxdSr36034354;     exkMwwxdSr36034354 = exkMwwxdSr13184463;     exkMwwxdSr13184463 = exkMwwxdSr22921646;     exkMwwxdSr22921646 = exkMwwxdSr11173495;     exkMwwxdSr11173495 = exkMwwxdSr96014679;     exkMwwxdSr96014679 = exkMwwxdSr5735698;     exkMwwxdSr5735698 = exkMwwxdSr19503662;     exkMwwxdSr19503662 = exkMwwxdSr62861583;     exkMwwxdSr62861583 = exkMwwxdSr99716808;     exkMwwxdSr99716808 = exkMwwxdSr34303061;     exkMwwxdSr34303061 = exkMwwxdSr49185474;     exkMwwxdSr49185474 = exkMwwxdSr39135707;     exkMwwxdSr39135707 = exkMwwxdSr35100449;     exkMwwxdSr35100449 = exkMwwxdSr91741943;     exkMwwxdSr91741943 = exkMwwxdSr94455699;     exkMwwxdSr94455699 = exkMwwxdSr84846285;     exkMwwxdSr84846285 = exkMwwxdSr1802471;     exkMwwxdSr1802471 = exkMwwxdSr59206741;     exkMwwxdSr59206741 = exkMwwxdSr66109763;     exkMwwxdSr66109763 = exkMwwxdSr95704663;     exkMwwxdSr95704663 = exkMwwxdSr56509476;     exkMwwxdSr56509476 = exkMwwxdSr80323459;     exkMwwxdSr80323459 = exkMwwxdSr21017421;     exkMwwxdSr21017421 = exkMwwxdSr45437834;     exkMwwxdSr45437834 = exkMwwxdSr7897663;     exkMwwxdSr7897663 = exkMwwxdSr53470479;     exkMwwxdSr53470479 = exkMwwxdSr47398345;     exkMwwxdSr47398345 = exkMwwxdSr73528603;     exkMwwxdSr73528603 = exkMwwxdSr56384401;     exkMwwxdSr56384401 = exkMwwxdSr2561524;     exkMwwxdSr2561524 = exkMwwxdSr41778806;     exkMwwxdSr41778806 = exkMwwxdSr76112280;     exkMwwxdSr76112280 = exkMwwxdSr7989207;     exkMwwxdSr7989207 = exkMwwxdSr50315396;     exkMwwxdSr50315396 = exkMwwxdSr45012775;     exkMwwxdSr45012775 = exkMwwxdSr35432260;     exkMwwxdSr35432260 = exkMwwxdSr48252739;     exkMwwxdSr48252739 = exkMwwxdSr79467113;     exkMwwxdSr79467113 = exkMwwxdSr32011796;     exkMwwxdSr32011796 = exkMwwxdSr61609425;     exkMwwxdSr61609425 = exkMwwxdSr87636828;     exkMwwxdSr87636828 = exkMwwxdSr54071890;     exkMwwxdSr54071890 = exkMwwxdSr64168330;     exkMwwxdSr64168330 = exkMwwxdSr5681259;     exkMwwxdSr5681259 = exkMwwxdSr67411298;     exkMwwxdSr67411298 = exkMwwxdSr72099533;     exkMwwxdSr72099533 = exkMwwxdSr16587961;     exkMwwxdSr16587961 = exkMwwxdSr10775589;     exkMwwxdSr10775589 = exkMwwxdSr11145686;     exkMwwxdSr11145686 = exkMwwxdSr58957867;     exkMwwxdSr58957867 = exkMwwxdSr69714661;     exkMwwxdSr69714661 = exkMwwxdSr79034280;     exkMwwxdSr79034280 = exkMwwxdSr76351934;     exkMwwxdSr76351934 = exkMwwxdSr85791692;     exkMwwxdSr85791692 = exkMwwxdSr85161827;     exkMwwxdSr85161827 = exkMwwxdSr55710894;     exkMwwxdSr55710894 = exkMwwxdSr92167041;     exkMwwxdSr92167041 = exkMwwxdSr77483811;     exkMwwxdSr77483811 = exkMwwxdSr3275832;     exkMwwxdSr3275832 = exkMwwxdSr42544201;     exkMwwxdSr42544201 = exkMwwxdSr58337352;     exkMwwxdSr58337352 = exkMwwxdSr45975058;     exkMwwxdSr45975058 = exkMwwxdSr6477183;     exkMwwxdSr6477183 = exkMwwxdSr97155285;     exkMwwxdSr97155285 = exkMwwxdSr92524254;     exkMwwxdSr92524254 = exkMwwxdSr73073194;     exkMwwxdSr73073194 = exkMwwxdSr31146501;     exkMwwxdSr31146501 = exkMwwxdSr84785052;     exkMwwxdSr84785052 = exkMwwxdSr46729168;     exkMwwxdSr46729168 = exkMwwxdSr59023439;     exkMwwxdSr59023439 = exkMwwxdSr36593547;     exkMwwxdSr36593547 = exkMwwxdSr22335357;     exkMwwxdSr22335357 = exkMwwxdSr27194945;     exkMwwxdSr27194945 = exkMwwxdSr4500339;     exkMwwxdSr4500339 = exkMwwxdSr8067835;     exkMwwxdSr8067835 = exkMwwxdSr2437587;     exkMwwxdSr2437587 = exkMwwxdSr16155130;     exkMwwxdSr16155130 = exkMwwxdSr15336163;     exkMwwxdSr15336163 = exkMwwxdSr78026535;     exkMwwxdSr78026535 = exkMwwxdSr35798129;     exkMwwxdSr35798129 = exkMwwxdSr48616722;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BJiMcJSTLf17692104() {     float HwDgWpTJVp71003839 = -228696265;    float HwDgWpTJVp55521884 = -545692734;    float HwDgWpTJVp19409887 = -553441754;    float HwDgWpTJVp43033164 = 65284336;    float HwDgWpTJVp65519376 = -129741514;    float HwDgWpTJVp22023607 = -170418591;    float HwDgWpTJVp9089212 = -824623504;    float HwDgWpTJVp39518998 = -126423996;    float HwDgWpTJVp37147450 = -167506777;    float HwDgWpTJVp8360357 = -860420650;    float HwDgWpTJVp48777100 = -520707002;    float HwDgWpTJVp7591359 = -472287095;    float HwDgWpTJVp69525712 = -780613225;    float HwDgWpTJVp48216137 = -543565207;    float HwDgWpTJVp80539429 = -591484912;    float HwDgWpTJVp15773044 = -923388068;    float HwDgWpTJVp88064373 = -753738040;    float HwDgWpTJVp48438114 = -714599192;    float HwDgWpTJVp22777987 = -990375084;    float HwDgWpTJVp20187587 = -214915304;    float HwDgWpTJVp36917461 = -541003948;    float HwDgWpTJVp76999680 = -54934327;    float HwDgWpTJVp26214969 = -471569968;    float HwDgWpTJVp37673282 = -990648143;    float HwDgWpTJVp31554927 = -375551599;    float HwDgWpTJVp94038691 = -922721880;    float HwDgWpTJVp66593082 = -187353807;    float HwDgWpTJVp68773033 = 1913100;    float HwDgWpTJVp6609370 = -489294680;    float HwDgWpTJVp43460240 = -10329218;    float HwDgWpTJVp93476279 = -646202735;    float HwDgWpTJVp96676898 = -806931682;    float HwDgWpTJVp16139120 = -435313458;    float HwDgWpTJVp51130630 = -581872120;    float HwDgWpTJVp70685063 = 98522087;    float HwDgWpTJVp26539155 = 23465172;    float HwDgWpTJVp95242182 = -913434403;    float HwDgWpTJVp12832843 = -768068336;    float HwDgWpTJVp14417895 = -693653384;    float HwDgWpTJVp15890378 = -808647019;    float HwDgWpTJVp85266846 = -696736335;    float HwDgWpTJVp68831851 = -659469832;    float HwDgWpTJVp26567613 = -671584266;    float HwDgWpTJVp44881953 = -365663481;    float HwDgWpTJVp75880929 = -635803048;    float HwDgWpTJVp61570761 = -458158282;    float HwDgWpTJVp59695743 = 29174579;    float HwDgWpTJVp18127278 = -341534145;    float HwDgWpTJVp50844306 = -936205777;    float HwDgWpTJVp99645269 = -411553553;    float HwDgWpTJVp50648023 = -751804609;    float HwDgWpTJVp49963537 = -890256360;    float HwDgWpTJVp86303609 = -647940485;    float HwDgWpTJVp10601942 = -355108071;    float HwDgWpTJVp80774642 = -97408056;    float HwDgWpTJVp94004159 = -73761939;    float HwDgWpTJVp29306916 = 25877234;    float HwDgWpTJVp81736604 = -562793612;    float HwDgWpTJVp11478238 = -559164066;    float HwDgWpTJVp71480684 = -207019635;    float HwDgWpTJVp55430525 = -983064785;    float HwDgWpTJVp40316178 = -726536605;    float HwDgWpTJVp32909628 = -637129317;    float HwDgWpTJVp93687209 = -57177559;    float HwDgWpTJVp14884078 = -114217915;    float HwDgWpTJVp52100202 = -713775321;    float HwDgWpTJVp91452239 = 63026363;    float HwDgWpTJVp18395082 = -98741105;    float HwDgWpTJVp77531073 = -542087294;    float HwDgWpTJVp54000275 = -514950085;    float HwDgWpTJVp20530862 = 90046335;    float HwDgWpTJVp75231531 = -985669704;    float HwDgWpTJVp34020219 = 79054191;    float HwDgWpTJVp6887610 = -81728066;    float HwDgWpTJVp34920740 = -518178970;    float HwDgWpTJVp68085610 = -881534117;    float HwDgWpTJVp50432068 = -383350061;    float HwDgWpTJVp81333015 = -5906488;    float HwDgWpTJVp61792352 = -254845095;    float HwDgWpTJVp69984165 = -917393318;    float HwDgWpTJVp34342949 = -851896460;    float HwDgWpTJVp48465804 = -845819662;    float HwDgWpTJVp17928728 = -61881124;    float HwDgWpTJVp6964101 = 22258873;    float HwDgWpTJVp92812217 = -258524610;    float HwDgWpTJVp43512742 = -755946375;    float HwDgWpTJVp10373289 = -58991197;    float HwDgWpTJVp5537178 = 19794612;    float HwDgWpTJVp70355988 = -384464064;    float HwDgWpTJVp76680904 = -827715975;    float HwDgWpTJVp97232238 = 97587937;    float HwDgWpTJVp13505578 = -250640792;    float HwDgWpTJVp1354605 = -108904271;    float HwDgWpTJVp42937210 = -386633750;    float HwDgWpTJVp60459853 = -825582235;    float HwDgWpTJVp44950668 = -970199731;    float HwDgWpTJVp35922223 = 77659485;    float HwDgWpTJVp32880403 = -514406707;    float HwDgWpTJVp29997875 = -151445566;    float HwDgWpTJVp23780727 = -228696265;     HwDgWpTJVp71003839 = HwDgWpTJVp55521884;     HwDgWpTJVp55521884 = HwDgWpTJVp19409887;     HwDgWpTJVp19409887 = HwDgWpTJVp43033164;     HwDgWpTJVp43033164 = HwDgWpTJVp65519376;     HwDgWpTJVp65519376 = HwDgWpTJVp22023607;     HwDgWpTJVp22023607 = HwDgWpTJVp9089212;     HwDgWpTJVp9089212 = HwDgWpTJVp39518998;     HwDgWpTJVp39518998 = HwDgWpTJVp37147450;     HwDgWpTJVp37147450 = HwDgWpTJVp8360357;     HwDgWpTJVp8360357 = HwDgWpTJVp48777100;     HwDgWpTJVp48777100 = HwDgWpTJVp7591359;     HwDgWpTJVp7591359 = HwDgWpTJVp69525712;     HwDgWpTJVp69525712 = HwDgWpTJVp48216137;     HwDgWpTJVp48216137 = HwDgWpTJVp80539429;     HwDgWpTJVp80539429 = HwDgWpTJVp15773044;     HwDgWpTJVp15773044 = HwDgWpTJVp88064373;     HwDgWpTJVp88064373 = HwDgWpTJVp48438114;     HwDgWpTJVp48438114 = HwDgWpTJVp22777987;     HwDgWpTJVp22777987 = HwDgWpTJVp20187587;     HwDgWpTJVp20187587 = HwDgWpTJVp36917461;     HwDgWpTJVp36917461 = HwDgWpTJVp76999680;     HwDgWpTJVp76999680 = HwDgWpTJVp26214969;     HwDgWpTJVp26214969 = HwDgWpTJVp37673282;     HwDgWpTJVp37673282 = HwDgWpTJVp31554927;     HwDgWpTJVp31554927 = HwDgWpTJVp94038691;     HwDgWpTJVp94038691 = HwDgWpTJVp66593082;     HwDgWpTJVp66593082 = HwDgWpTJVp68773033;     HwDgWpTJVp68773033 = HwDgWpTJVp6609370;     HwDgWpTJVp6609370 = HwDgWpTJVp43460240;     HwDgWpTJVp43460240 = HwDgWpTJVp93476279;     HwDgWpTJVp93476279 = HwDgWpTJVp96676898;     HwDgWpTJVp96676898 = HwDgWpTJVp16139120;     HwDgWpTJVp16139120 = HwDgWpTJVp51130630;     HwDgWpTJVp51130630 = HwDgWpTJVp70685063;     HwDgWpTJVp70685063 = HwDgWpTJVp26539155;     HwDgWpTJVp26539155 = HwDgWpTJVp95242182;     HwDgWpTJVp95242182 = HwDgWpTJVp12832843;     HwDgWpTJVp12832843 = HwDgWpTJVp14417895;     HwDgWpTJVp14417895 = HwDgWpTJVp15890378;     HwDgWpTJVp15890378 = HwDgWpTJVp85266846;     HwDgWpTJVp85266846 = HwDgWpTJVp68831851;     HwDgWpTJVp68831851 = HwDgWpTJVp26567613;     HwDgWpTJVp26567613 = HwDgWpTJVp44881953;     HwDgWpTJVp44881953 = HwDgWpTJVp75880929;     HwDgWpTJVp75880929 = HwDgWpTJVp61570761;     HwDgWpTJVp61570761 = HwDgWpTJVp59695743;     HwDgWpTJVp59695743 = HwDgWpTJVp18127278;     HwDgWpTJVp18127278 = HwDgWpTJVp50844306;     HwDgWpTJVp50844306 = HwDgWpTJVp99645269;     HwDgWpTJVp99645269 = HwDgWpTJVp50648023;     HwDgWpTJVp50648023 = HwDgWpTJVp49963537;     HwDgWpTJVp49963537 = HwDgWpTJVp86303609;     HwDgWpTJVp86303609 = HwDgWpTJVp10601942;     HwDgWpTJVp10601942 = HwDgWpTJVp80774642;     HwDgWpTJVp80774642 = HwDgWpTJVp94004159;     HwDgWpTJVp94004159 = HwDgWpTJVp29306916;     HwDgWpTJVp29306916 = HwDgWpTJVp81736604;     HwDgWpTJVp81736604 = HwDgWpTJVp11478238;     HwDgWpTJVp11478238 = HwDgWpTJVp71480684;     HwDgWpTJVp71480684 = HwDgWpTJVp55430525;     HwDgWpTJVp55430525 = HwDgWpTJVp40316178;     HwDgWpTJVp40316178 = HwDgWpTJVp32909628;     HwDgWpTJVp32909628 = HwDgWpTJVp93687209;     HwDgWpTJVp93687209 = HwDgWpTJVp14884078;     HwDgWpTJVp14884078 = HwDgWpTJVp52100202;     HwDgWpTJVp52100202 = HwDgWpTJVp91452239;     HwDgWpTJVp91452239 = HwDgWpTJVp18395082;     HwDgWpTJVp18395082 = HwDgWpTJVp77531073;     HwDgWpTJVp77531073 = HwDgWpTJVp54000275;     HwDgWpTJVp54000275 = HwDgWpTJVp20530862;     HwDgWpTJVp20530862 = HwDgWpTJVp75231531;     HwDgWpTJVp75231531 = HwDgWpTJVp34020219;     HwDgWpTJVp34020219 = HwDgWpTJVp6887610;     HwDgWpTJVp6887610 = HwDgWpTJVp34920740;     HwDgWpTJVp34920740 = HwDgWpTJVp68085610;     HwDgWpTJVp68085610 = HwDgWpTJVp50432068;     HwDgWpTJVp50432068 = HwDgWpTJVp81333015;     HwDgWpTJVp81333015 = HwDgWpTJVp61792352;     HwDgWpTJVp61792352 = HwDgWpTJVp69984165;     HwDgWpTJVp69984165 = HwDgWpTJVp34342949;     HwDgWpTJVp34342949 = HwDgWpTJVp48465804;     HwDgWpTJVp48465804 = HwDgWpTJVp17928728;     HwDgWpTJVp17928728 = HwDgWpTJVp6964101;     HwDgWpTJVp6964101 = HwDgWpTJVp92812217;     HwDgWpTJVp92812217 = HwDgWpTJVp43512742;     HwDgWpTJVp43512742 = HwDgWpTJVp10373289;     HwDgWpTJVp10373289 = HwDgWpTJVp5537178;     HwDgWpTJVp5537178 = HwDgWpTJVp70355988;     HwDgWpTJVp70355988 = HwDgWpTJVp76680904;     HwDgWpTJVp76680904 = HwDgWpTJVp97232238;     HwDgWpTJVp97232238 = HwDgWpTJVp13505578;     HwDgWpTJVp13505578 = HwDgWpTJVp1354605;     HwDgWpTJVp1354605 = HwDgWpTJVp42937210;     HwDgWpTJVp42937210 = HwDgWpTJVp60459853;     HwDgWpTJVp60459853 = HwDgWpTJVp44950668;     HwDgWpTJVp44950668 = HwDgWpTJVp35922223;     HwDgWpTJVp35922223 = HwDgWpTJVp32880403;     HwDgWpTJVp32880403 = HwDgWpTJVp29997875;     HwDgWpTJVp29997875 = HwDgWpTJVp23780727;     HwDgWpTJVp23780727 = HwDgWpTJVp71003839;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void yESrCEFasD84653505() {     float YHtUMnXkvo61123677 = -159976608;    float YHtUMnXkvo77219265 = -963214029;    float YHtUMnXkvo16215798 = -647364829;    float YHtUMnXkvo1611600 = 58571684;    float YHtUMnXkvo87416213 = -539420277;    float YHtUMnXkvo37615444 = -862484511;    float YHtUMnXkvo72299985 = -166705444;    float YHtUMnXkvo61611612 = -567049052;    float YHtUMnXkvo43699301 = -989045484;    float YHtUMnXkvo9255541 = -67102047;    float YHtUMnXkvo45249525 = -988599231;    float YHtUMnXkvo56632544 = -787353592;    float YHtUMnXkvo39724759 = -14507559;    float YHtUMnXkvo40479211 = -886879143;    float YHtUMnXkvo23785911 = 60681293;    float YHtUMnXkvo14517334 = -956563588;    float YHtUMnXkvo87879313 = -834697851;    float YHtUMnXkvo92794359 = -454520107;    float YHtUMnXkvo4120648 = -974767787;    float YHtUMnXkvo59585597 = -374818065;    float YHtUMnXkvo4509288 = 44104586;    float YHtUMnXkvo53377528 = -898022845;    float YHtUMnXkvo96331783 = -236987732;    float YHtUMnXkvo89244487 = -219593041;    float YHtUMnXkvo6882804 = -345756396;    float YHtUMnXkvo29403734 = -706801543;    float YHtUMnXkvo58781998 = -247682514;    float YHtUMnXkvo27346933 = -666943947;    float YHtUMnXkvo80496814 = -797564043;    float YHtUMnXkvo67339190 = -660162983;    float YHtUMnXkvo30352361 = -259962514;    float YHtUMnXkvo80767669 = -599577863;    float YHtUMnXkvo10560584 = -937482834;    float YHtUMnXkvo75252112 = -450005011;    float YHtUMnXkvo88639636 = -911127499;    float YHtUMnXkvo22598351 = -359329984;    float YHtUMnXkvo7417120 = -528903686;    float YHtUMnXkvo7107806 = -629390259;    float YHtUMnXkvo60264508 = -13796846;    float YHtUMnXkvo32275412 = -322786478;    float YHtUMnXkvo86822187 = -490541509;    float YHtUMnXkvo9173975 = -667755127;    float YHtUMnXkvo46667814 = -606140115;    float YHtUMnXkvo34775792 = -886711113;    float YHtUMnXkvo51863399 = -121572406;    float YHtUMnXkvo22437723 = -753826858;    float YHtUMnXkvo67816825 = -588543;    float YHtUMnXkvo83897317 = -976951755;    float YHtUMnXkvo97421349 = -935908984;    float YHtUMnXkvo48971098 = -455574777;    float YHtUMnXkvo93272916 = -704741661;    float YHtUMnXkvo8283902 = -136828942;    float YHtUMnXkvo11204413 = -251847488;    float YHtUMnXkvo73759275 = 89416796;    float YHtUMnXkvo80144217 = 17092394;    float YHtUMnXkvo7746149 = -261953763;    float YHtUMnXkvo80887482 = -626226298;    float YHtUMnXkvo26971310 = -327771789;    float YHtUMnXkvo94728795 = -595671921;    float YHtUMnXkvo58012480 = -832618735;    float YHtUMnXkvo78833445 = -514801998;    float YHtUMnXkvo44953052 = -499761498;    float YHtUMnXkvo81114797 = -769485010;    float YHtUMnXkvo76360111 = -228882502;    float YHtUMnXkvo78903180 = -807139533;    float YHtUMnXkvo64481855 = -289021369;    float YHtUMnXkvo46071960 = -849870759;    float YHtUMnXkvo64472646 = -564502549;    float YHtUMnXkvo51839575 = -975751644;    float YHtUMnXkvo1187560 = -579988723;    float YHtUMnXkvo7100215 = -327659903;    float YHtUMnXkvo80771508 = -105307592;    float YHtUMnXkvo32529851 = -340723261;    float YHtUMnXkvo71845236 = -551981309;    float YHtUMnXkvo72763409 = -884276557;    float YHtUMnXkvo95335312 = -288140288;    float YHtUMnXkvo6709715 = -191882730;    float YHtUMnXkvo61555991 = -350276619;    float YHtUMnXkvo37381088 = 1979365;    float YHtUMnXkvo84445081 = -591929539;    float YHtUMnXkvo61586908 = -606213001;    float YHtUMnXkvo74884680 = -270730760;    float YHtUMnXkvo29925584 = -731034963;    float YHtUMnXkvo31525716 = -241989266;    float YHtUMnXkvo74066274 = -955421323;    float YHtUMnXkvo22068459 = -23133573;    float YHtUMnXkvo69563256 = -247730375;    float YHtUMnXkvo36801309 = -926899630;    float YHtUMnXkvo95107894 = -367097406;    float YHtUMnXkvo80893487 = -549173736;    float YHtUMnXkvo41710868 = -733103687;    float YHtUMnXkvo80445809 = -101131898;    float YHtUMnXkvo12379010 = 66281661;    float YHtUMnXkvo2252029 = -181178111;    float YHtUMnXkvo53441967 = -807984481;    float YHtUMnXkvo41869135 = -990780012;    float YHtUMnXkvo28059178 = -898270118;    float YHtUMnXkvo70307702 = -277257614;    float YHtUMnXkvo55872612 = 20428420;    float YHtUMnXkvo87381543 = -159976608;     YHtUMnXkvo61123677 = YHtUMnXkvo77219265;     YHtUMnXkvo77219265 = YHtUMnXkvo16215798;     YHtUMnXkvo16215798 = YHtUMnXkvo1611600;     YHtUMnXkvo1611600 = YHtUMnXkvo87416213;     YHtUMnXkvo87416213 = YHtUMnXkvo37615444;     YHtUMnXkvo37615444 = YHtUMnXkvo72299985;     YHtUMnXkvo72299985 = YHtUMnXkvo61611612;     YHtUMnXkvo61611612 = YHtUMnXkvo43699301;     YHtUMnXkvo43699301 = YHtUMnXkvo9255541;     YHtUMnXkvo9255541 = YHtUMnXkvo45249525;     YHtUMnXkvo45249525 = YHtUMnXkvo56632544;     YHtUMnXkvo56632544 = YHtUMnXkvo39724759;     YHtUMnXkvo39724759 = YHtUMnXkvo40479211;     YHtUMnXkvo40479211 = YHtUMnXkvo23785911;     YHtUMnXkvo23785911 = YHtUMnXkvo14517334;     YHtUMnXkvo14517334 = YHtUMnXkvo87879313;     YHtUMnXkvo87879313 = YHtUMnXkvo92794359;     YHtUMnXkvo92794359 = YHtUMnXkvo4120648;     YHtUMnXkvo4120648 = YHtUMnXkvo59585597;     YHtUMnXkvo59585597 = YHtUMnXkvo4509288;     YHtUMnXkvo4509288 = YHtUMnXkvo53377528;     YHtUMnXkvo53377528 = YHtUMnXkvo96331783;     YHtUMnXkvo96331783 = YHtUMnXkvo89244487;     YHtUMnXkvo89244487 = YHtUMnXkvo6882804;     YHtUMnXkvo6882804 = YHtUMnXkvo29403734;     YHtUMnXkvo29403734 = YHtUMnXkvo58781998;     YHtUMnXkvo58781998 = YHtUMnXkvo27346933;     YHtUMnXkvo27346933 = YHtUMnXkvo80496814;     YHtUMnXkvo80496814 = YHtUMnXkvo67339190;     YHtUMnXkvo67339190 = YHtUMnXkvo30352361;     YHtUMnXkvo30352361 = YHtUMnXkvo80767669;     YHtUMnXkvo80767669 = YHtUMnXkvo10560584;     YHtUMnXkvo10560584 = YHtUMnXkvo75252112;     YHtUMnXkvo75252112 = YHtUMnXkvo88639636;     YHtUMnXkvo88639636 = YHtUMnXkvo22598351;     YHtUMnXkvo22598351 = YHtUMnXkvo7417120;     YHtUMnXkvo7417120 = YHtUMnXkvo7107806;     YHtUMnXkvo7107806 = YHtUMnXkvo60264508;     YHtUMnXkvo60264508 = YHtUMnXkvo32275412;     YHtUMnXkvo32275412 = YHtUMnXkvo86822187;     YHtUMnXkvo86822187 = YHtUMnXkvo9173975;     YHtUMnXkvo9173975 = YHtUMnXkvo46667814;     YHtUMnXkvo46667814 = YHtUMnXkvo34775792;     YHtUMnXkvo34775792 = YHtUMnXkvo51863399;     YHtUMnXkvo51863399 = YHtUMnXkvo22437723;     YHtUMnXkvo22437723 = YHtUMnXkvo67816825;     YHtUMnXkvo67816825 = YHtUMnXkvo83897317;     YHtUMnXkvo83897317 = YHtUMnXkvo97421349;     YHtUMnXkvo97421349 = YHtUMnXkvo48971098;     YHtUMnXkvo48971098 = YHtUMnXkvo93272916;     YHtUMnXkvo93272916 = YHtUMnXkvo8283902;     YHtUMnXkvo8283902 = YHtUMnXkvo11204413;     YHtUMnXkvo11204413 = YHtUMnXkvo73759275;     YHtUMnXkvo73759275 = YHtUMnXkvo80144217;     YHtUMnXkvo80144217 = YHtUMnXkvo7746149;     YHtUMnXkvo7746149 = YHtUMnXkvo80887482;     YHtUMnXkvo80887482 = YHtUMnXkvo26971310;     YHtUMnXkvo26971310 = YHtUMnXkvo94728795;     YHtUMnXkvo94728795 = YHtUMnXkvo58012480;     YHtUMnXkvo58012480 = YHtUMnXkvo78833445;     YHtUMnXkvo78833445 = YHtUMnXkvo44953052;     YHtUMnXkvo44953052 = YHtUMnXkvo81114797;     YHtUMnXkvo81114797 = YHtUMnXkvo76360111;     YHtUMnXkvo76360111 = YHtUMnXkvo78903180;     YHtUMnXkvo78903180 = YHtUMnXkvo64481855;     YHtUMnXkvo64481855 = YHtUMnXkvo46071960;     YHtUMnXkvo46071960 = YHtUMnXkvo64472646;     YHtUMnXkvo64472646 = YHtUMnXkvo51839575;     YHtUMnXkvo51839575 = YHtUMnXkvo1187560;     YHtUMnXkvo1187560 = YHtUMnXkvo7100215;     YHtUMnXkvo7100215 = YHtUMnXkvo80771508;     YHtUMnXkvo80771508 = YHtUMnXkvo32529851;     YHtUMnXkvo32529851 = YHtUMnXkvo71845236;     YHtUMnXkvo71845236 = YHtUMnXkvo72763409;     YHtUMnXkvo72763409 = YHtUMnXkvo95335312;     YHtUMnXkvo95335312 = YHtUMnXkvo6709715;     YHtUMnXkvo6709715 = YHtUMnXkvo61555991;     YHtUMnXkvo61555991 = YHtUMnXkvo37381088;     YHtUMnXkvo37381088 = YHtUMnXkvo84445081;     YHtUMnXkvo84445081 = YHtUMnXkvo61586908;     YHtUMnXkvo61586908 = YHtUMnXkvo74884680;     YHtUMnXkvo74884680 = YHtUMnXkvo29925584;     YHtUMnXkvo29925584 = YHtUMnXkvo31525716;     YHtUMnXkvo31525716 = YHtUMnXkvo74066274;     YHtUMnXkvo74066274 = YHtUMnXkvo22068459;     YHtUMnXkvo22068459 = YHtUMnXkvo69563256;     YHtUMnXkvo69563256 = YHtUMnXkvo36801309;     YHtUMnXkvo36801309 = YHtUMnXkvo95107894;     YHtUMnXkvo95107894 = YHtUMnXkvo80893487;     YHtUMnXkvo80893487 = YHtUMnXkvo41710868;     YHtUMnXkvo41710868 = YHtUMnXkvo80445809;     YHtUMnXkvo80445809 = YHtUMnXkvo12379010;     YHtUMnXkvo12379010 = YHtUMnXkvo2252029;     YHtUMnXkvo2252029 = YHtUMnXkvo53441967;     YHtUMnXkvo53441967 = YHtUMnXkvo41869135;     YHtUMnXkvo41869135 = YHtUMnXkvo28059178;     YHtUMnXkvo28059178 = YHtUMnXkvo70307702;     YHtUMnXkvo70307702 = YHtUMnXkvo55872612;     YHtUMnXkvo55872612 = YHtUMnXkvo87381543;     YHtUMnXkvo87381543 = YHtUMnXkvo61123677;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void KiCeQYmIOm12577960() {     long ceHebEdeWi32048085 = -120006266;    long ceHebEdeWi65191106 = -681722117;    long ceHebEdeWi89675538 = -21570365;    long ceHebEdeWi60978448 = -863189629;    long ceHebEdeWi91146650 = -282262647;    long ceHebEdeWi75255755 = -587747490;    long ceHebEdeWi25067462 = -151620446;    long ceHebEdeWi16660683 = -151340015;    long ceHebEdeWi63994348 = -52062229;    long ceHebEdeWi42421452 = -859669172;    long ceHebEdeWi1292390 = -633142618;    long ceHebEdeWi43477163 = -780116773;    long ceHebEdeWi84341135 = 3125905;    long ceHebEdeWi92916717 = -494452910;    long ceHebEdeWi95002834 = -983455885;    long ceHebEdeWi7035388 = -995391152;    long ceHebEdeWi43698426 = -969032386;    long ceHebEdeWi69314210 = -204304622;    long ceHebEdeWi84793038 = -989912363;    long ceHebEdeWi75186271 = -615915746;    long ceHebEdeWi51013589 = -659978418;    long ceHebEdeWi81924312 = -561303550;    long ceHebEdeWi504146 = -153294258;    long ceHebEdeWi33180270 = -260321726;    long ceHebEdeWi94476016 = 32967061;    long ceHebEdeWi34226000 = -175749228;    long ceHebEdeWi9401565 = -659451083;    long ceHebEdeWi73400258 = -397521307;    long ceHebEdeWi61154797 = -199683333;    long ceHebEdeWi2316142 = 46254049;    long ceHebEdeWi34106145 = -976830253;    long ceHebEdeWi71689849 = -853698930;    long ceHebEdeWi49544182 = 69269427;    long ceHebEdeWi34466128 = -702710905;    long ceHebEdeWi37898505 = -554014075;    long ceHebEdeWi82891848 = -708694850;    long ceHebEdeWi94299384 = -158642388;    long ceHebEdeWi39620933 = -599438742;    long ceHebEdeWi85732034 = -802867982;    long ceHebEdeWi88945584 = -743879884;    long ceHebEdeWi65869735 = 59004561;    long ceHebEdeWi2206568 = -763053627;    long ceHebEdeWi65151210 = -253371433;    long ceHebEdeWi55806570 = -338411490;    long ceHebEdeWi67878264 = -401335327;    long ceHebEdeWi73932411 = 80370549;    long ceHebEdeWi78809360 = -487280642;    long ceHebEdeWi43966361 = -701243585;    long ceHebEdeWi24548292 = -699618883;    long ceHebEdeWi77984853 = -750603522;    long ceHebEdeWi41554994 = -874739768;    long ceHebEdeWi60707589 = -181987886;    long ceHebEdeWi72998362 = -971884588;    long ceHebEdeWi95492697 = -861680352;    long ceHebEdeWi61564370 = -918988620;    long ceHebEdeWi50123773 = -558702716;    long ceHebEdeWi64686960 = -428427860;    long ceHebEdeWi56495269 = -761248639;    long ceHebEdeWi66502432 = -796156690;    long ceHebEdeWi56920650 = -6513419;    long ceHebEdeWi65854190 = -928296408;    long ceHebEdeWi51667204 = -754099139;    long ceHebEdeWi55505886 = -951656683;    long ceHebEdeWi61678206 = 1683722;    long ceHebEdeWi8315307 = -882838919;    long ceHebEdeWi29602540 = -779443688;    long ceHebEdeWi93932980 = -749386201;    long ceHebEdeWi49875008 = -294163190;    long ceHebEdeWi55018212 = -940438835;    long ceHebEdeWi12110986 = -174761036;    long ceHebEdeWi12736004 = -736748764;    long ceHebEdeWi4077494 = -269593645;    long ceHebEdeWi83582175 = -401436640;    long ceHebEdeWi95847454 = -146032479;    long ceHebEdeWi9316536 = -574920307;    long ceHebEdeWi48807022 = -896924792;    long ceHebEdeWi16773102 = -207932117;    long ceHebEdeWi44697576 = -814882769;    long ceHebEdeWi65302005 = -858986400;    long ceHebEdeWi20543606 = 52596512;    long ceHebEdeWi55416639 = -688468587;    long ceHebEdeWi65435204 = -958207498;    long ceHebEdeWi48851966 = -697902424;    long ceHebEdeWi83169943 = -449079811;    long ceHebEdeWi60761148 = -79006183;    long ceHebEdeWi73398556 = -694842367;    long ceHebEdeWi98691487 = -881814342;    long ceHebEdeWi54051485 = -69050221;    long ceHebEdeWi72901758 = -783722286;    long ceHebEdeWi87774732 = -995311360;    long ceHebEdeWi18204889 = -180266991;    long ceHebEdeWi37804115 = -397393749;    long ceHebEdeWi73118500 = -803282053;    long ceHebEdeWi28811385 = -696354564;    long ceHebEdeWi23091394 = -815583477;    long ceHebEdeWi14202532 = -186896301;    long ceHebEdeWi46700681 = -811396944;    long ceHebEdeWi3473005 = -155055155;    long ceHebEdeWi47491264 = -455572571;    long ceHebEdeWi38275724 = -120006266;     ceHebEdeWi32048085 = ceHebEdeWi65191106;     ceHebEdeWi65191106 = ceHebEdeWi89675538;     ceHebEdeWi89675538 = ceHebEdeWi60978448;     ceHebEdeWi60978448 = ceHebEdeWi91146650;     ceHebEdeWi91146650 = ceHebEdeWi75255755;     ceHebEdeWi75255755 = ceHebEdeWi25067462;     ceHebEdeWi25067462 = ceHebEdeWi16660683;     ceHebEdeWi16660683 = ceHebEdeWi63994348;     ceHebEdeWi63994348 = ceHebEdeWi42421452;     ceHebEdeWi42421452 = ceHebEdeWi1292390;     ceHebEdeWi1292390 = ceHebEdeWi43477163;     ceHebEdeWi43477163 = ceHebEdeWi84341135;     ceHebEdeWi84341135 = ceHebEdeWi92916717;     ceHebEdeWi92916717 = ceHebEdeWi95002834;     ceHebEdeWi95002834 = ceHebEdeWi7035388;     ceHebEdeWi7035388 = ceHebEdeWi43698426;     ceHebEdeWi43698426 = ceHebEdeWi69314210;     ceHebEdeWi69314210 = ceHebEdeWi84793038;     ceHebEdeWi84793038 = ceHebEdeWi75186271;     ceHebEdeWi75186271 = ceHebEdeWi51013589;     ceHebEdeWi51013589 = ceHebEdeWi81924312;     ceHebEdeWi81924312 = ceHebEdeWi504146;     ceHebEdeWi504146 = ceHebEdeWi33180270;     ceHebEdeWi33180270 = ceHebEdeWi94476016;     ceHebEdeWi94476016 = ceHebEdeWi34226000;     ceHebEdeWi34226000 = ceHebEdeWi9401565;     ceHebEdeWi9401565 = ceHebEdeWi73400258;     ceHebEdeWi73400258 = ceHebEdeWi61154797;     ceHebEdeWi61154797 = ceHebEdeWi2316142;     ceHebEdeWi2316142 = ceHebEdeWi34106145;     ceHebEdeWi34106145 = ceHebEdeWi71689849;     ceHebEdeWi71689849 = ceHebEdeWi49544182;     ceHebEdeWi49544182 = ceHebEdeWi34466128;     ceHebEdeWi34466128 = ceHebEdeWi37898505;     ceHebEdeWi37898505 = ceHebEdeWi82891848;     ceHebEdeWi82891848 = ceHebEdeWi94299384;     ceHebEdeWi94299384 = ceHebEdeWi39620933;     ceHebEdeWi39620933 = ceHebEdeWi85732034;     ceHebEdeWi85732034 = ceHebEdeWi88945584;     ceHebEdeWi88945584 = ceHebEdeWi65869735;     ceHebEdeWi65869735 = ceHebEdeWi2206568;     ceHebEdeWi2206568 = ceHebEdeWi65151210;     ceHebEdeWi65151210 = ceHebEdeWi55806570;     ceHebEdeWi55806570 = ceHebEdeWi67878264;     ceHebEdeWi67878264 = ceHebEdeWi73932411;     ceHebEdeWi73932411 = ceHebEdeWi78809360;     ceHebEdeWi78809360 = ceHebEdeWi43966361;     ceHebEdeWi43966361 = ceHebEdeWi24548292;     ceHebEdeWi24548292 = ceHebEdeWi77984853;     ceHebEdeWi77984853 = ceHebEdeWi41554994;     ceHebEdeWi41554994 = ceHebEdeWi60707589;     ceHebEdeWi60707589 = ceHebEdeWi72998362;     ceHebEdeWi72998362 = ceHebEdeWi95492697;     ceHebEdeWi95492697 = ceHebEdeWi61564370;     ceHebEdeWi61564370 = ceHebEdeWi50123773;     ceHebEdeWi50123773 = ceHebEdeWi64686960;     ceHebEdeWi64686960 = ceHebEdeWi56495269;     ceHebEdeWi56495269 = ceHebEdeWi66502432;     ceHebEdeWi66502432 = ceHebEdeWi56920650;     ceHebEdeWi56920650 = ceHebEdeWi65854190;     ceHebEdeWi65854190 = ceHebEdeWi51667204;     ceHebEdeWi51667204 = ceHebEdeWi55505886;     ceHebEdeWi55505886 = ceHebEdeWi61678206;     ceHebEdeWi61678206 = ceHebEdeWi8315307;     ceHebEdeWi8315307 = ceHebEdeWi29602540;     ceHebEdeWi29602540 = ceHebEdeWi93932980;     ceHebEdeWi93932980 = ceHebEdeWi49875008;     ceHebEdeWi49875008 = ceHebEdeWi55018212;     ceHebEdeWi55018212 = ceHebEdeWi12110986;     ceHebEdeWi12110986 = ceHebEdeWi12736004;     ceHebEdeWi12736004 = ceHebEdeWi4077494;     ceHebEdeWi4077494 = ceHebEdeWi83582175;     ceHebEdeWi83582175 = ceHebEdeWi95847454;     ceHebEdeWi95847454 = ceHebEdeWi9316536;     ceHebEdeWi9316536 = ceHebEdeWi48807022;     ceHebEdeWi48807022 = ceHebEdeWi16773102;     ceHebEdeWi16773102 = ceHebEdeWi44697576;     ceHebEdeWi44697576 = ceHebEdeWi65302005;     ceHebEdeWi65302005 = ceHebEdeWi20543606;     ceHebEdeWi20543606 = ceHebEdeWi55416639;     ceHebEdeWi55416639 = ceHebEdeWi65435204;     ceHebEdeWi65435204 = ceHebEdeWi48851966;     ceHebEdeWi48851966 = ceHebEdeWi83169943;     ceHebEdeWi83169943 = ceHebEdeWi60761148;     ceHebEdeWi60761148 = ceHebEdeWi73398556;     ceHebEdeWi73398556 = ceHebEdeWi98691487;     ceHebEdeWi98691487 = ceHebEdeWi54051485;     ceHebEdeWi54051485 = ceHebEdeWi72901758;     ceHebEdeWi72901758 = ceHebEdeWi87774732;     ceHebEdeWi87774732 = ceHebEdeWi18204889;     ceHebEdeWi18204889 = ceHebEdeWi37804115;     ceHebEdeWi37804115 = ceHebEdeWi73118500;     ceHebEdeWi73118500 = ceHebEdeWi28811385;     ceHebEdeWi28811385 = ceHebEdeWi23091394;     ceHebEdeWi23091394 = ceHebEdeWi14202532;     ceHebEdeWi14202532 = ceHebEdeWi46700681;     ceHebEdeWi46700681 = ceHebEdeWi3473005;     ceHebEdeWi3473005 = ceHebEdeWi47491264;     ceHebEdeWi47491264 = ceHebEdeWi38275724;     ceHebEdeWi38275724 = ceHebEdeWi32048085;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ZIGKIWtJJo79539361() {     int FtVRpYBFCg22167923 = -51286608;    int FtVRpYBFCg86888487 = 756588;    int FtVRpYBFCg86481449 = -115493440;    int FtVRpYBFCg19556884 = -869902281;    int FtVRpYBFCg13043488 = -691941410;    int FtVRpYBFCg90847592 = -179813410;    int FtVRpYBFCg88278236 = -593702385;    int FtVRpYBFCg38753297 = -591965071;    int FtVRpYBFCg70546199 = -873600936;    int FtVRpYBFCg43316636 = -66350568;    int FtVRpYBFCg97764813 = -1034847;    int FtVRpYBFCg92518348 = 4816730;    int FtVRpYBFCg54540182 = -330768430;    int FtVRpYBFCg85179791 = -837766846;    int FtVRpYBFCg38249315 = -331289680;    int FtVRpYBFCg5779678 = 71433327;    int FtVRpYBFCg43513365 = 50007802;    int FtVRpYBFCg13670456 = 55774464;    int FtVRpYBFCg66135699 = -974305066;    int FtVRpYBFCg14584283 = -775818506;    int FtVRpYBFCg18605416 = -74869884;    int FtVRpYBFCg58302160 = -304392069;    int FtVRpYBFCg70620961 = 81287979;    int FtVRpYBFCg84751475 = -589266625;    int FtVRpYBFCg69803894 = 62762263;    int FtVRpYBFCg69591042 = 40171109;    int FtVRpYBFCg1590482 = -719779789;    int FtVRpYBFCg31974158 = 33621646;    int FtVRpYBFCg35042241 = -507952696;    int FtVRpYBFCg26195092 = -603579716;    int FtVRpYBFCg70982227 = -590590032;    int FtVRpYBFCg55780620 = -646345111;    int FtVRpYBFCg43965647 = -432899948;    int FtVRpYBFCg58587610 = -570843796;    int FtVRpYBFCg55853078 = -463663661;    int FtVRpYBFCg78951045 = 8509994;    int FtVRpYBFCg6474321 = -874111670;    int FtVRpYBFCg33895895 = -460760665;    int FtVRpYBFCg31578649 = -123011444;    int FtVRpYBFCg5330619 = -258019344;    int FtVRpYBFCg67425076 = -834800613;    int FtVRpYBFCg42548692 = -771338922;    int FtVRpYBFCg85251411 = -187927283;    int FtVRpYBFCg45700410 = -859459122;    int FtVRpYBFCg43860734 = -987104685;    int FtVRpYBFCg34799373 = -215298028;    int FtVRpYBFCg86930443 = -517043764;    int FtVRpYBFCg9736401 = -236661195;    int FtVRpYBFCg71125335 = -699322091;    int FtVRpYBFCg27310683 = -794624746;    int FtVRpYBFCg84179887 = -827676820;    int FtVRpYBFCg19027954 = -528560468;    int FtVRpYBFCg97899165 = -575791591;    int FtVRpYBFCg58650030 = -417155485;    int FtVRpYBFCg60933945 = -804488169;    int FtVRpYBFCg63865762 = -746894540;    int FtVRpYBFCg16267527 = 19468608;    int FtVRpYBFCg1729975 = -526226816;    int FtVRpYBFCg49752990 = -832664545;    int FtVRpYBFCg43452446 = -632112519;    int FtVRpYBFCg89257110 = -460033621;    int FtVRpYBFCg56304078 = -527324032;    int FtVRpYBFCg3711056 = 15987624;    int FtVRpYBFCg44351108 = -170021221;    int FtVRpYBFCg72334409 = -475760537;    int FtVRpYBFCg41984193 = -354689737;    int FtVRpYBFCg48552701 = -562283323;    int FtVRpYBFCg95952571 = -759924634;    int FtVRpYBFCg29326713 = -274103185;    int FtVRpYBFCg59298270 = -239799674;    int FtVRpYBFCg99305356 = -54455003;    int FtVRpYBFCg9617471 = -489231533;    int FtVRpYBFCg82091807 = -821214092;    int FtVRpYBFCg60805081 = -616285723;    int FtVRpYBFCg47159206 = -941017894;    int FtVRpYBFCg76056724 = -303530963;    int FtVRpYBFCg73050748 = -16464787;    int FtVRpYBFCg24920551 = -59252900;    int FtVRpYBFCg40890741 = -602161941;    int FtVRpYBFCg35004521 = -721939709;    int FtVRpYBFCg82660598 = -442785128;    int FtVRpYBFCg91854081 = -383118595;    int FtVRpYBFCg60848823 = -267056263;    int FtVRpYBFCg7731559 = -713327950;    int FtVRpYBFCg42015204 = -775902896;    int FtVRpYBFCg51954273 = 37970435;    int FtVRpYBFCg57881455 = 29446480;    int FtVRpYBFCg85315616 = 84255536;    int FtVRpYBFCg97653664 = -766355627;    int FtVRpYBFCg91987315 = -716769121;    int FtVRpYBFCg62683518 = 89041385;    int FtVRpYBFCg4744347 = -247884855;    int FtVRpYBFCg84142905 = -628096121;    int FtVRpYBFCg88126202 = -490898925;    int FtVRpYBFCg16073508 = -797985723;    int FtVRpYBFCg11120999 = -207476581;    int FtVRpYBFCg38837636 = -687326546;    int FtVRpYBFCg40900304 = 82093938;    int FtVRpYBFCg73366001 = -283698586;    int FtVRpYBFCg1876541 = -51286608;     FtVRpYBFCg22167923 = FtVRpYBFCg86888487;     FtVRpYBFCg86888487 = FtVRpYBFCg86481449;     FtVRpYBFCg86481449 = FtVRpYBFCg19556884;     FtVRpYBFCg19556884 = FtVRpYBFCg13043488;     FtVRpYBFCg13043488 = FtVRpYBFCg90847592;     FtVRpYBFCg90847592 = FtVRpYBFCg88278236;     FtVRpYBFCg88278236 = FtVRpYBFCg38753297;     FtVRpYBFCg38753297 = FtVRpYBFCg70546199;     FtVRpYBFCg70546199 = FtVRpYBFCg43316636;     FtVRpYBFCg43316636 = FtVRpYBFCg97764813;     FtVRpYBFCg97764813 = FtVRpYBFCg92518348;     FtVRpYBFCg92518348 = FtVRpYBFCg54540182;     FtVRpYBFCg54540182 = FtVRpYBFCg85179791;     FtVRpYBFCg85179791 = FtVRpYBFCg38249315;     FtVRpYBFCg38249315 = FtVRpYBFCg5779678;     FtVRpYBFCg5779678 = FtVRpYBFCg43513365;     FtVRpYBFCg43513365 = FtVRpYBFCg13670456;     FtVRpYBFCg13670456 = FtVRpYBFCg66135699;     FtVRpYBFCg66135699 = FtVRpYBFCg14584283;     FtVRpYBFCg14584283 = FtVRpYBFCg18605416;     FtVRpYBFCg18605416 = FtVRpYBFCg58302160;     FtVRpYBFCg58302160 = FtVRpYBFCg70620961;     FtVRpYBFCg70620961 = FtVRpYBFCg84751475;     FtVRpYBFCg84751475 = FtVRpYBFCg69803894;     FtVRpYBFCg69803894 = FtVRpYBFCg69591042;     FtVRpYBFCg69591042 = FtVRpYBFCg1590482;     FtVRpYBFCg1590482 = FtVRpYBFCg31974158;     FtVRpYBFCg31974158 = FtVRpYBFCg35042241;     FtVRpYBFCg35042241 = FtVRpYBFCg26195092;     FtVRpYBFCg26195092 = FtVRpYBFCg70982227;     FtVRpYBFCg70982227 = FtVRpYBFCg55780620;     FtVRpYBFCg55780620 = FtVRpYBFCg43965647;     FtVRpYBFCg43965647 = FtVRpYBFCg58587610;     FtVRpYBFCg58587610 = FtVRpYBFCg55853078;     FtVRpYBFCg55853078 = FtVRpYBFCg78951045;     FtVRpYBFCg78951045 = FtVRpYBFCg6474321;     FtVRpYBFCg6474321 = FtVRpYBFCg33895895;     FtVRpYBFCg33895895 = FtVRpYBFCg31578649;     FtVRpYBFCg31578649 = FtVRpYBFCg5330619;     FtVRpYBFCg5330619 = FtVRpYBFCg67425076;     FtVRpYBFCg67425076 = FtVRpYBFCg42548692;     FtVRpYBFCg42548692 = FtVRpYBFCg85251411;     FtVRpYBFCg85251411 = FtVRpYBFCg45700410;     FtVRpYBFCg45700410 = FtVRpYBFCg43860734;     FtVRpYBFCg43860734 = FtVRpYBFCg34799373;     FtVRpYBFCg34799373 = FtVRpYBFCg86930443;     FtVRpYBFCg86930443 = FtVRpYBFCg9736401;     FtVRpYBFCg9736401 = FtVRpYBFCg71125335;     FtVRpYBFCg71125335 = FtVRpYBFCg27310683;     FtVRpYBFCg27310683 = FtVRpYBFCg84179887;     FtVRpYBFCg84179887 = FtVRpYBFCg19027954;     FtVRpYBFCg19027954 = FtVRpYBFCg97899165;     FtVRpYBFCg97899165 = FtVRpYBFCg58650030;     FtVRpYBFCg58650030 = FtVRpYBFCg60933945;     FtVRpYBFCg60933945 = FtVRpYBFCg63865762;     FtVRpYBFCg63865762 = FtVRpYBFCg16267527;     FtVRpYBFCg16267527 = FtVRpYBFCg1729975;     FtVRpYBFCg1729975 = FtVRpYBFCg49752990;     FtVRpYBFCg49752990 = FtVRpYBFCg43452446;     FtVRpYBFCg43452446 = FtVRpYBFCg89257110;     FtVRpYBFCg89257110 = FtVRpYBFCg56304078;     FtVRpYBFCg56304078 = FtVRpYBFCg3711056;     FtVRpYBFCg3711056 = FtVRpYBFCg44351108;     FtVRpYBFCg44351108 = FtVRpYBFCg72334409;     FtVRpYBFCg72334409 = FtVRpYBFCg41984193;     FtVRpYBFCg41984193 = FtVRpYBFCg48552701;     FtVRpYBFCg48552701 = FtVRpYBFCg95952571;     FtVRpYBFCg95952571 = FtVRpYBFCg29326713;     FtVRpYBFCg29326713 = FtVRpYBFCg59298270;     FtVRpYBFCg59298270 = FtVRpYBFCg99305356;     FtVRpYBFCg99305356 = FtVRpYBFCg9617471;     FtVRpYBFCg9617471 = FtVRpYBFCg82091807;     FtVRpYBFCg82091807 = FtVRpYBFCg60805081;     FtVRpYBFCg60805081 = FtVRpYBFCg47159206;     FtVRpYBFCg47159206 = FtVRpYBFCg76056724;     FtVRpYBFCg76056724 = FtVRpYBFCg73050748;     FtVRpYBFCg73050748 = FtVRpYBFCg24920551;     FtVRpYBFCg24920551 = FtVRpYBFCg40890741;     FtVRpYBFCg40890741 = FtVRpYBFCg35004521;     FtVRpYBFCg35004521 = FtVRpYBFCg82660598;     FtVRpYBFCg82660598 = FtVRpYBFCg91854081;     FtVRpYBFCg91854081 = FtVRpYBFCg60848823;     FtVRpYBFCg60848823 = FtVRpYBFCg7731559;     FtVRpYBFCg7731559 = FtVRpYBFCg42015204;     FtVRpYBFCg42015204 = FtVRpYBFCg51954273;     FtVRpYBFCg51954273 = FtVRpYBFCg57881455;     FtVRpYBFCg57881455 = FtVRpYBFCg85315616;     FtVRpYBFCg85315616 = FtVRpYBFCg97653664;     FtVRpYBFCg97653664 = FtVRpYBFCg91987315;     FtVRpYBFCg91987315 = FtVRpYBFCg62683518;     FtVRpYBFCg62683518 = FtVRpYBFCg4744347;     FtVRpYBFCg4744347 = FtVRpYBFCg84142905;     FtVRpYBFCg84142905 = FtVRpYBFCg88126202;     FtVRpYBFCg88126202 = FtVRpYBFCg16073508;     FtVRpYBFCg16073508 = FtVRpYBFCg11120999;     FtVRpYBFCg11120999 = FtVRpYBFCg38837636;     FtVRpYBFCg38837636 = FtVRpYBFCg40900304;     FtVRpYBFCg40900304 = FtVRpYBFCg73366001;     FtVRpYBFCg73366001 = FtVRpYBFCg1876541;     FtVRpYBFCg1876541 = FtVRpYBFCg22167923;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void KZdAUHTIIK16184336() {     int wtDcwNJRWh44555040 = -785690571;    int wtDcwNJRWh71235987 = 52213533;    int wtDcwNJRWh15250729 = -355147284;    int wtDcwNJRWh34563573 = -876309812;    int wtDcwNJRWh11217742 = -182998411;    int wtDcwNJRWh5730710 = -40421788;    int wtDcwNJRWh80433975 = -415689691;    int wtDcwNJRWh14387156 = 37438284;    int wtDcwNJRWh67709330 = -757796975;    int wtDcwNJRWh35080220 = -909091901;    int wtDcwNJRWh35306673 = 2340752;    int wtDcwNJRWh48421297 = -245928564;    int wtDcwNJRWh21548363 = -949485749;    int wtDcwNJRWh27794543 = -265475603;    int wtDcwNJRWh74984591 = 41232607;    int wtDcwNJRWh50035590 = -310234215;    int wtDcwNJRWh93336717 = -227272018;    int wtDcwNJRWh19646872 = -395968227;    int wtDcwNJRWh7417330 = -9407191;    int wtDcwNJRWh93100565 = 71547040;    int wtDcwNJRWh19488523 = -66357193;    int wtDcwNJRWh22117379 = -809158382;    int wtDcwNJRWh73914284 = -94792613;    int wtDcwNJRWh11251263 = -53259482;    int wtDcwNJRWh5344142 = -658796861;    int wtDcwNJRWh57894036 = -403723115;    int wtDcwNJRWh48679902 = -477366282;    int wtDcwNJRWh37885609 = -854832807;    int wtDcwNJRWh41934802 = -852209815;    int wtDcwNJRWh35352271 = -523875582;    int wtDcwNJRWh15273032 = -671906184;    int wtDcwNJRWh13321812 = -198416466;    int wtDcwNJRWh25004317 = -962243443;    int wtDcwNJRWh17976298 = -894970646;    int wtDcwNJRWh32082443 = -527420083;    int wtDcwNJRWh20643914 = -306885383;    int wtDcwNJRWh99914033 = -607059622;    int wtDcwNJRWh87521996 = -328386137;    int wtDcwNJRWh79886779 = -524057476;    int wtDcwNJRWh25516333 = -394243373;    int wtDcwNJRWh96182447 = -237978279;    int wtDcwNJRWh31057083 = -529247613;    int wtDcwNJRWh90801603 = -75457866;    int wtDcwNJRWh45144529 = -856822771;    int wtDcwNJRWh11844001 = -796248162;    int wtDcwNJRWh42899655 = -397527123;    int wtDcwNJRWh99227840 = 4545984;    int wtDcwNJRWh54335075 = 6803814;    int wtDcwNJRWh65585240 = -649038790;    int wtDcwNJRWh24394428 = -936645005;    int wtDcwNJRWh93049104 = -82753097;    int wtDcwNJRWh92879211 = -959379750;    int wtDcwNJRWh76213568 = -247702821;    int wtDcwNJRWh18936576 = -692836294;    int wtDcwNJRWh96695812 = -945192284;    int wtDcwNJRWh22437662 = -976532190;    int wtDcwNJRWh97321703 = -852993855;    int wtDcwNJRWh3999467 = -201887803;    int wtDcwNJRWh29219432 = -117512952;    int wtDcwNJRWh53323705 = -779275297;    int wtDcwNJRWh57050807 = -563055506;    int wtDcwNJRWh42548367 = -560856885;    int wtDcwNJRWh72452353 = -110351901;    int wtDcwNJRWh32357059 = -133921393;    int wtDcwNJRWh19807189 = -137185718;    int wtDcwNJRWh21984862 = -799242783;    int wtDcwNJRWh23416980 = -283685121;    int wtDcwNJRWh3572066 = 45484896;    int wtDcwNJRWh95712100 = -738055520;    int wtDcwNJRWh54340678 = -651882011;    int wtDcwNJRWh50121556 = -703174594;    int wtDcwNJRWh5814721 = -898885881;    int wtDcwNJRWh39760092 = -871910751;    int wtDcwNJRWh81900997 = -615163819;    int wtDcwNJRWh96918118 = -690474682;    int wtDcwNJRWh88431440 = -537109580;    int wtDcwNJRWh31315775 = -633700516;    int wtDcwNJRWh28769755 = -237969843;    int wtDcwNJRWh99407261 = -257011320;    int wtDcwNJRWh62444486 = -161269739;    int wtDcwNJRWh58666195 = -308269099;    int wtDcwNJRWh94344826 = -384170097;    int wtDcwNJRWh72300368 = -105794018;    int wtDcwNJRWh17540374 = -915564810;    int wtDcwNJRWh42303167 = -341122486;    int wtDcwNJRWh22393821 = -712526435;    int wtDcwNJRWh37108243 = -950713645;    int wtDcwNJRWh6067742 = -169407150;    int wtDcwNJRWh21280485 = -949778362;    int wtDcwNJRWh9644781 = -550887894;    int wtDcwNJRWh23322211 = -453891529;    int wtDcwNJRWh95914566 = -305171820;    int wtDcwNJRWh58302565 = -110873186;    int wtDcwNJRWh26563075 = -744782180;    int wtDcwNJRWh68465525 = -831187867;    int wtDcwNJRWh53634081 = -677121395;    int wtDcwNJRWh58604729 = -318895712;    int wtDcwNJRWh58444544 = -941536474;    int wtDcwNJRWh25337341 = -619637054;    int wtDcwNJRWh89859138 = -785690571;     wtDcwNJRWh44555040 = wtDcwNJRWh71235987;     wtDcwNJRWh71235987 = wtDcwNJRWh15250729;     wtDcwNJRWh15250729 = wtDcwNJRWh34563573;     wtDcwNJRWh34563573 = wtDcwNJRWh11217742;     wtDcwNJRWh11217742 = wtDcwNJRWh5730710;     wtDcwNJRWh5730710 = wtDcwNJRWh80433975;     wtDcwNJRWh80433975 = wtDcwNJRWh14387156;     wtDcwNJRWh14387156 = wtDcwNJRWh67709330;     wtDcwNJRWh67709330 = wtDcwNJRWh35080220;     wtDcwNJRWh35080220 = wtDcwNJRWh35306673;     wtDcwNJRWh35306673 = wtDcwNJRWh48421297;     wtDcwNJRWh48421297 = wtDcwNJRWh21548363;     wtDcwNJRWh21548363 = wtDcwNJRWh27794543;     wtDcwNJRWh27794543 = wtDcwNJRWh74984591;     wtDcwNJRWh74984591 = wtDcwNJRWh50035590;     wtDcwNJRWh50035590 = wtDcwNJRWh93336717;     wtDcwNJRWh93336717 = wtDcwNJRWh19646872;     wtDcwNJRWh19646872 = wtDcwNJRWh7417330;     wtDcwNJRWh7417330 = wtDcwNJRWh93100565;     wtDcwNJRWh93100565 = wtDcwNJRWh19488523;     wtDcwNJRWh19488523 = wtDcwNJRWh22117379;     wtDcwNJRWh22117379 = wtDcwNJRWh73914284;     wtDcwNJRWh73914284 = wtDcwNJRWh11251263;     wtDcwNJRWh11251263 = wtDcwNJRWh5344142;     wtDcwNJRWh5344142 = wtDcwNJRWh57894036;     wtDcwNJRWh57894036 = wtDcwNJRWh48679902;     wtDcwNJRWh48679902 = wtDcwNJRWh37885609;     wtDcwNJRWh37885609 = wtDcwNJRWh41934802;     wtDcwNJRWh41934802 = wtDcwNJRWh35352271;     wtDcwNJRWh35352271 = wtDcwNJRWh15273032;     wtDcwNJRWh15273032 = wtDcwNJRWh13321812;     wtDcwNJRWh13321812 = wtDcwNJRWh25004317;     wtDcwNJRWh25004317 = wtDcwNJRWh17976298;     wtDcwNJRWh17976298 = wtDcwNJRWh32082443;     wtDcwNJRWh32082443 = wtDcwNJRWh20643914;     wtDcwNJRWh20643914 = wtDcwNJRWh99914033;     wtDcwNJRWh99914033 = wtDcwNJRWh87521996;     wtDcwNJRWh87521996 = wtDcwNJRWh79886779;     wtDcwNJRWh79886779 = wtDcwNJRWh25516333;     wtDcwNJRWh25516333 = wtDcwNJRWh96182447;     wtDcwNJRWh96182447 = wtDcwNJRWh31057083;     wtDcwNJRWh31057083 = wtDcwNJRWh90801603;     wtDcwNJRWh90801603 = wtDcwNJRWh45144529;     wtDcwNJRWh45144529 = wtDcwNJRWh11844001;     wtDcwNJRWh11844001 = wtDcwNJRWh42899655;     wtDcwNJRWh42899655 = wtDcwNJRWh99227840;     wtDcwNJRWh99227840 = wtDcwNJRWh54335075;     wtDcwNJRWh54335075 = wtDcwNJRWh65585240;     wtDcwNJRWh65585240 = wtDcwNJRWh24394428;     wtDcwNJRWh24394428 = wtDcwNJRWh93049104;     wtDcwNJRWh93049104 = wtDcwNJRWh92879211;     wtDcwNJRWh92879211 = wtDcwNJRWh76213568;     wtDcwNJRWh76213568 = wtDcwNJRWh18936576;     wtDcwNJRWh18936576 = wtDcwNJRWh96695812;     wtDcwNJRWh96695812 = wtDcwNJRWh22437662;     wtDcwNJRWh22437662 = wtDcwNJRWh97321703;     wtDcwNJRWh97321703 = wtDcwNJRWh3999467;     wtDcwNJRWh3999467 = wtDcwNJRWh29219432;     wtDcwNJRWh29219432 = wtDcwNJRWh53323705;     wtDcwNJRWh53323705 = wtDcwNJRWh57050807;     wtDcwNJRWh57050807 = wtDcwNJRWh42548367;     wtDcwNJRWh42548367 = wtDcwNJRWh72452353;     wtDcwNJRWh72452353 = wtDcwNJRWh32357059;     wtDcwNJRWh32357059 = wtDcwNJRWh19807189;     wtDcwNJRWh19807189 = wtDcwNJRWh21984862;     wtDcwNJRWh21984862 = wtDcwNJRWh23416980;     wtDcwNJRWh23416980 = wtDcwNJRWh3572066;     wtDcwNJRWh3572066 = wtDcwNJRWh95712100;     wtDcwNJRWh95712100 = wtDcwNJRWh54340678;     wtDcwNJRWh54340678 = wtDcwNJRWh50121556;     wtDcwNJRWh50121556 = wtDcwNJRWh5814721;     wtDcwNJRWh5814721 = wtDcwNJRWh39760092;     wtDcwNJRWh39760092 = wtDcwNJRWh81900997;     wtDcwNJRWh81900997 = wtDcwNJRWh96918118;     wtDcwNJRWh96918118 = wtDcwNJRWh88431440;     wtDcwNJRWh88431440 = wtDcwNJRWh31315775;     wtDcwNJRWh31315775 = wtDcwNJRWh28769755;     wtDcwNJRWh28769755 = wtDcwNJRWh99407261;     wtDcwNJRWh99407261 = wtDcwNJRWh62444486;     wtDcwNJRWh62444486 = wtDcwNJRWh58666195;     wtDcwNJRWh58666195 = wtDcwNJRWh94344826;     wtDcwNJRWh94344826 = wtDcwNJRWh72300368;     wtDcwNJRWh72300368 = wtDcwNJRWh17540374;     wtDcwNJRWh17540374 = wtDcwNJRWh42303167;     wtDcwNJRWh42303167 = wtDcwNJRWh22393821;     wtDcwNJRWh22393821 = wtDcwNJRWh37108243;     wtDcwNJRWh37108243 = wtDcwNJRWh6067742;     wtDcwNJRWh6067742 = wtDcwNJRWh21280485;     wtDcwNJRWh21280485 = wtDcwNJRWh9644781;     wtDcwNJRWh9644781 = wtDcwNJRWh23322211;     wtDcwNJRWh23322211 = wtDcwNJRWh95914566;     wtDcwNJRWh95914566 = wtDcwNJRWh58302565;     wtDcwNJRWh58302565 = wtDcwNJRWh26563075;     wtDcwNJRWh26563075 = wtDcwNJRWh68465525;     wtDcwNJRWh68465525 = wtDcwNJRWh53634081;     wtDcwNJRWh53634081 = wtDcwNJRWh58604729;     wtDcwNJRWh58604729 = wtDcwNJRWh58444544;     wtDcwNJRWh58444544 = wtDcwNJRWh25337341;     wtDcwNJRWh25337341 = wtDcwNJRWh89859138;     wtDcwNJRWh89859138 = wtDcwNJRWh44555040;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mwOohMCJMh68861142() {     int EVdLVjQFim81475099 = -69472811;    int EVdLVjQFim15526806 = -69021822;    int EVdLVjQFim83275045 = -600544820;    int EVdLVjQFim58696732 = -12242310;    int EVdLVjQFim64156854 = -703900719;    int EVdLVjQFim27897745 = -362973285;    int EVdLVjQFim72616477 = -848621847;    int EVdLVjQFim64407016 = -969145866;    int EVdLVjQFim16780062 = -70719195;    int EVdLVjQFim7740485 = -258154825;    int EVdLVjQFim60067555 = -691616010;    int EVdLVjQFim21620694 = -851901039;    int EVdLVjQFim40138040 = -768263657;    int EVdLVjQFim59573707 = -698262265;    int EVdLVjQFim59990489 = -136211156;    int EVdLVjQFim78955885 = -647084714;    int EVdLVjQFim57140082 = -302095497;    int EVdLVjQFim29613763 = -995622376;    int EVdLVjQFim48683968 = -50423728;    int EVdLVjQFim17871835 = 30443088;    int EVdLVjQFim6871548 = -588652779;    int EVdLVjQFim5381405 = -489778255;    int EVdLVjQFim52780223 = -931938323;    int EVdLVjQFim48520740 = -507578388;    int EVdLVjQFim43419799 = 35261896;    int EVdLVjQFim81216455 = -494297943;    int EVdLVjQFim98125725 = -804668275;    int EVdLVjQFim47323040 = -684723822;    int EVdLVjQFim68007207 = -54960025;    int EVdLVjQFim9088378 = -3953496;    int EVdLVjQFim82746454 = -135108392;    int EVdLVjQFim13944806 = 82706392;    int EVdLVjQFim50640506 = -221421231;    int EVdLVjQFim47970105 = -725255848;    int EVdLVjQFim95450900 = -964218661;    int EVdLVjQFim64311176 = -782422636;    int EVdLVjQFim33868639 = -324878256;    int EVdLVjQFim27329793 = -207434377;    int EVdLVjQFim91946345 = -240079001;    int EVdLVjQFim46271631 = -301994464;    int EVdLVjQFim76307795 = -806886904;    int EVdLVjQFim12322424 = -934851116;    int EVdLVjQFim57322617 = -558720482;    int EVdLVjQFim3497183 = -796059564;    int EVdLVjQFim34833374 = 4545270;    int EVdLVjQFim55573569 = -30888425;    int EVdLVjQFim76802315 = -141478687;    int EVdLVjQFim436914 = -69156278;    int EVdLVjQFim85475465 = -664723434;    int EVdLVjQFim72005363 = -71419556;    int EVdLVjQFim87721809 = -36746576;    int EVdLVjQFim51429068 = -829228870;    int EVdLVjQFim5217259 = -303636008;    int EVdLVjQFim26044113 = -66908417;    int EVdLVjQFim61989594 = -435530351;    int EVdLVjQFim76093695 = -579694557;    int EVdLVjQFim62746582 = -137083499;    int EVdLVjQFim34754306 = 7033567;    int EVdLVjQFim15276933 = 52495793;    int EVdLVjQFim82940399 = -109602777;    int EVdLVjQFim29772019 = -558305010;    int EVdLVjQFim25293438 = -63898026;    int EVdLVjQFim96399809 = -814185841;    int EVdLVjQFim7691684 = 33234300;    int EVdLVjQFim24994031 = -23046433;    int EVdLVjQFim46122749 = -674322402;    int EVdLVjQFim70980188 = -530479809;    int EVdLVjQFim92167935 = 56992191;    int EVdLVjQFim64122807 = -734043604;    int EVdLVjQFim95679312 = -353788520;    int EVdLVjQFim45087247 = -222206458;    int EVdLVjQFim29810290 = 5338879;    int EVdLVjQFim37667417 = -655543375;    int EVdLVjQFim2412337 = -748429265;    int EVdLVjQFim41564039 = -162670009;    int EVdLVjQFim94549123 = -653801664;    int EVdLVjQFim48058787 = -931057774;    int EVdLVjQFim49283040 = -35878760;    int EVdLVjQFim13687366 = -412123658;    int EVdLVjQFim87846229 = -933849680;    int EVdLVjQFim4414141 = -252819256;    int EVdLVjQFim97688811 = -635511998;    int EVdLVjQFim61847574 = 79999612;    int EVdLVjQFim96001843 = -983540469;    int EVdLVjQFim21366569 = -967206920;    int EVdLVjQFim31317387 = -305879522;    int EVdLVjQFim8727548 = -613657601;    int EVdLVjQFim24596393 = -54512814;    int EVdLVjQFim85980510 = -189725498;    int EVdLVjQFim19357206 = -284524105;    int EVdLVjQFim1564595 = -545339138;    int EVdLVjQFim99114332 = -231911824;    int EVdLVjQFim12052860 = -159930170;    int EVdLVjQFim9005947 = -30476225;    int EVdLVjQFim16499612 = -743689454;    int EVdLVjQFim51014357 = -642988879;    int EVdLVjQFim15922614 = -20665275;    int EVdLVjQFim49630933 = -491954783;    int EVdLVjQFim78503152 = -673013131;    int EVdLVjQFim88710624 = -69472811;     EVdLVjQFim81475099 = EVdLVjQFim15526806;     EVdLVjQFim15526806 = EVdLVjQFim83275045;     EVdLVjQFim83275045 = EVdLVjQFim58696732;     EVdLVjQFim58696732 = EVdLVjQFim64156854;     EVdLVjQFim64156854 = EVdLVjQFim27897745;     EVdLVjQFim27897745 = EVdLVjQFim72616477;     EVdLVjQFim72616477 = EVdLVjQFim64407016;     EVdLVjQFim64407016 = EVdLVjQFim16780062;     EVdLVjQFim16780062 = EVdLVjQFim7740485;     EVdLVjQFim7740485 = EVdLVjQFim60067555;     EVdLVjQFim60067555 = EVdLVjQFim21620694;     EVdLVjQFim21620694 = EVdLVjQFim40138040;     EVdLVjQFim40138040 = EVdLVjQFim59573707;     EVdLVjQFim59573707 = EVdLVjQFim59990489;     EVdLVjQFim59990489 = EVdLVjQFim78955885;     EVdLVjQFim78955885 = EVdLVjQFim57140082;     EVdLVjQFim57140082 = EVdLVjQFim29613763;     EVdLVjQFim29613763 = EVdLVjQFim48683968;     EVdLVjQFim48683968 = EVdLVjQFim17871835;     EVdLVjQFim17871835 = EVdLVjQFim6871548;     EVdLVjQFim6871548 = EVdLVjQFim5381405;     EVdLVjQFim5381405 = EVdLVjQFim52780223;     EVdLVjQFim52780223 = EVdLVjQFim48520740;     EVdLVjQFim48520740 = EVdLVjQFim43419799;     EVdLVjQFim43419799 = EVdLVjQFim81216455;     EVdLVjQFim81216455 = EVdLVjQFim98125725;     EVdLVjQFim98125725 = EVdLVjQFim47323040;     EVdLVjQFim47323040 = EVdLVjQFim68007207;     EVdLVjQFim68007207 = EVdLVjQFim9088378;     EVdLVjQFim9088378 = EVdLVjQFim82746454;     EVdLVjQFim82746454 = EVdLVjQFim13944806;     EVdLVjQFim13944806 = EVdLVjQFim50640506;     EVdLVjQFim50640506 = EVdLVjQFim47970105;     EVdLVjQFim47970105 = EVdLVjQFim95450900;     EVdLVjQFim95450900 = EVdLVjQFim64311176;     EVdLVjQFim64311176 = EVdLVjQFim33868639;     EVdLVjQFim33868639 = EVdLVjQFim27329793;     EVdLVjQFim27329793 = EVdLVjQFim91946345;     EVdLVjQFim91946345 = EVdLVjQFim46271631;     EVdLVjQFim46271631 = EVdLVjQFim76307795;     EVdLVjQFim76307795 = EVdLVjQFim12322424;     EVdLVjQFim12322424 = EVdLVjQFim57322617;     EVdLVjQFim57322617 = EVdLVjQFim3497183;     EVdLVjQFim3497183 = EVdLVjQFim34833374;     EVdLVjQFim34833374 = EVdLVjQFim55573569;     EVdLVjQFim55573569 = EVdLVjQFim76802315;     EVdLVjQFim76802315 = EVdLVjQFim436914;     EVdLVjQFim436914 = EVdLVjQFim85475465;     EVdLVjQFim85475465 = EVdLVjQFim72005363;     EVdLVjQFim72005363 = EVdLVjQFim87721809;     EVdLVjQFim87721809 = EVdLVjQFim51429068;     EVdLVjQFim51429068 = EVdLVjQFim5217259;     EVdLVjQFim5217259 = EVdLVjQFim26044113;     EVdLVjQFim26044113 = EVdLVjQFim61989594;     EVdLVjQFim61989594 = EVdLVjQFim76093695;     EVdLVjQFim76093695 = EVdLVjQFim62746582;     EVdLVjQFim62746582 = EVdLVjQFim34754306;     EVdLVjQFim34754306 = EVdLVjQFim15276933;     EVdLVjQFim15276933 = EVdLVjQFim82940399;     EVdLVjQFim82940399 = EVdLVjQFim29772019;     EVdLVjQFim29772019 = EVdLVjQFim25293438;     EVdLVjQFim25293438 = EVdLVjQFim96399809;     EVdLVjQFim96399809 = EVdLVjQFim7691684;     EVdLVjQFim7691684 = EVdLVjQFim24994031;     EVdLVjQFim24994031 = EVdLVjQFim46122749;     EVdLVjQFim46122749 = EVdLVjQFim70980188;     EVdLVjQFim70980188 = EVdLVjQFim92167935;     EVdLVjQFim92167935 = EVdLVjQFim64122807;     EVdLVjQFim64122807 = EVdLVjQFim95679312;     EVdLVjQFim95679312 = EVdLVjQFim45087247;     EVdLVjQFim45087247 = EVdLVjQFim29810290;     EVdLVjQFim29810290 = EVdLVjQFim37667417;     EVdLVjQFim37667417 = EVdLVjQFim2412337;     EVdLVjQFim2412337 = EVdLVjQFim41564039;     EVdLVjQFim41564039 = EVdLVjQFim94549123;     EVdLVjQFim94549123 = EVdLVjQFim48058787;     EVdLVjQFim48058787 = EVdLVjQFim49283040;     EVdLVjQFim49283040 = EVdLVjQFim13687366;     EVdLVjQFim13687366 = EVdLVjQFim87846229;     EVdLVjQFim87846229 = EVdLVjQFim4414141;     EVdLVjQFim4414141 = EVdLVjQFim97688811;     EVdLVjQFim97688811 = EVdLVjQFim61847574;     EVdLVjQFim61847574 = EVdLVjQFim96001843;     EVdLVjQFim96001843 = EVdLVjQFim21366569;     EVdLVjQFim21366569 = EVdLVjQFim31317387;     EVdLVjQFim31317387 = EVdLVjQFim8727548;     EVdLVjQFim8727548 = EVdLVjQFim24596393;     EVdLVjQFim24596393 = EVdLVjQFim85980510;     EVdLVjQFim85980510 = EVdLVjQFim19357206;     EVdLVjQFim19357206 = EVdLVjQFim1564595;     EVdLVjQFim1564595 = EVdLVjQFim99114332;     EVdLVjQFim99114332 = EVdLVjQFim12052860;     EVdLVjQFim12052860 = EVdLVjQFim9005947;     EVdLVjQFim9005947 = EVdLVjQFim16499612;     EVdLVjQFim16499612 = EVdLVjQFim51014357;     EVdLVjQFim51014357 = EVdLVjQFim15922614;     EVdLVjQFim15922614 = EVdLVjQFim49630933;     EVdLVjQFim49630933 = EVdLVjQFim78503152;     EVdLVjQFim78503152 = EVdLVjQFim88710624;     EVdLVjQFim88710624 = EVdLVjQFim81475099;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void aYVcpjwixZ5506117() {     int ijdigNxQfk3862217 = -803876774;    int ijdigNxQfk99874305 = -17564876;    int ijdigNxQfk12044325 = -840198664;    int ijdigNxQfk73703420 = -18649842;    int ijdigNxQfk62331109 = -194957720;    int ijdigNxQfk42780862 = -223581663;    int ijdigNxQfk64772216 = -670609153;    int ijdigNxQfk40040875 = -339742510;    int ijdigNxQfk13943193 = 45084766;    int ijdigNxQfk99504069 = -896158;    int ijdigNxQfk97609414 = -688240411;    int ijdigNxQfk77523643 = -2646332;    int ijdigNxQfk7146221 = -286980977;    int ijdigNxQfk2188460 = -125971021;    int ijdigNxQfk96725765 = -863688869;    int ijdigNxQfk23211798 = 71247744;    int ijdigNxQfk6963435 = -579375317;    int ijdigNxQfk35590179 = -347365067;    int ijdigNxQfk89965599 = -185525853;    int ijdigNxQfk96388117 = -222191365;    int ijdigNxQfk7754655 = -580140088;    int ijdigNxQfk69196622 = -994544569;    int ijdigNxQfk56073546 = -8018916;    int ijdigNxQfk75020526 = 28428755;    int ijdigNxQfk78960045 = -686297228;    int ijdigNxQfk69519450 = -938192166;    int ijdigNxQfk45215146 = -562254768;    int ijdigNxQfk53234490 = -473178275;    int ijdigNxQfk74899768 = -399217144;    int ijdigNxQfk18245557 = 75750638;    int ijdigNxQfk27037259 = -216424544;    int ijdigNxQfk71485996 = -569364963;    int ijdigNxQfk31679176 = -750764725;    int ijdigNxQfk7358792 = 50617302;    int ijdigNxQfk71680265 = 72024916;    int ijdigNxQfk6004046 = 2181987;    int ijdigNxQfk27308351 = -57826207;    int ijdigNxQfk80955894 = -75059849;    int ijdigNxQfk40254477 = -641125033;    int ijdigNxQfk66457346 = -438218493;    int ijdigNxQfk5065166 = -210064570;    int ijdigNxQfk830815 = -692759807;    int ijdigNxQfk62872809 = -446251066;    int ijdigNxQfk2941303 = -793423213;    int ijdigNxQfk2816641 = -904598208;    int ijdigNxQfk63673851 = -213117520;    int ijdigNxQfk89099712 = -719888939;    int ijdigNxQfk45035589 = -925691269;    int ijdigNxQfk79935371 = -614440133;    int ijdigNxQfk69089109 = -213439815;    int ijdigNxQfk96591025 = -391822853;    int ijdigNxQfk25280325 = -160048153;    int ijdigNxQfk83531661 = 24452762;    int ijdigNxQfk86330657 = -342589226;    int ijdigNxQfk97751461 = -576234466;    int ijdigNxQfk34665594 = -809332206;    int ijdigNxQfk43800759 = 90454039;    int ijdigNxQfk37023798 = -768627420;    int ijdigNxQfk94743374 = -332352614;    int ijdigNxQfk92811658 = -256765554;    int ijdigNxQfk97565715 = -661326895;    int ijdigNxQfk11537727 = -97430878;    int ijdigNxQfk65141107 = -940525366;    int ijdigNxQfk95697635 = 69334128;    int ijdigNxQfk72466810 = -784471614;    int ijdigNxQfk26123418 = -18875449;    int ijdigNxQfk45844467 = -251881607;    int ijdigNxQfk99787428 = -237598279;    int ijdigNxQfk30508195 = -97995938;    int ijdigNxQfk90721720 = -765870857;    int ijdigNxQfk95903447 = -870926049;    int ijdigNxQfk26007540 = -404315469;    int ijdigNxQfk95335702 = -706240034;    int ijdigNxQfk23508254 = -747307361;    int ijdigNxQfk91322951 = 87873204;    int ijdigNxQfk6923840 = -887380281;    int ijdigNxQfk6323814 = -448293503;    int ijdigNxQfk53132244 = -214595703;    int ijdigNxQfk72203886 = -66973037;    int ijdigNxQfk15286195 = -373179709;    int ijdigNxQfk80419737 = -118303228;    int ijdigNxQfk179558 = -636563500;    int ijdigNxQfk73299119 = -858738143;    int ijdigNxQfk5810659 = -85777330;    int ijdigNxQfk21654532 = -532426510;    int ijdigNxQfk1756935 = 43623608;    int ijdigNxQfk87954335 = -493817725;    int ijdigNxQfk45348518 = -308175500;    int ijdigNxQfk9607330 = -373148233;    int ijdigNxQfk37014671 = -118642878;    int ijdigNxQfk62203286 = 11727948;    int ijdigNxQfk90284553 = -289198788;    int ijdigNxQfk86212519 = -742707235;    int ijdigNxQfk47442818 = -284359479;    int ijdigNxQfk68891630 = -776891598;    int ijdigNxQfk93527439 = -12633692;    int ijdigNxQfk35689708 = -752234441;    int ijdigNxQfk67175173 = -415585194;    int ijdigNxQfk30474492 = 91048401;    int ijdigNxQfk76693222 = -803876774;     ijdigNxQfk3862217 = ijdigNxQfk99874305;     ijdigNxQfk99874305 = ijdigNxQfk12044325;     ijdigNxQfk12044325 = ijdigNxQfk73703420;     ijdigNxQfk73703420 = ijdigNxQfk62331109;     ijdigNxQfk62331109 = ijdigNxQfk42780862;     ijdigNxQfk42780862 = ijdigNxQfk64772216;     ijdigNxQfk64772216 = ijdigNxQfk40040875;     ijdigNxQfk40040875 = ijdigNxQfk13943193;     ijdigNxQfk13943193 = ijdigNxQfk99504069;     ijdigNxQfk99504069 = ijdigNxQfk97609414;     ijdigNxQfk97609414 = ijdigNxQfk77523643;     ijdigNxQfk77523643 = ijdigNxQfk7146221;     ijdigNxQfk7146221 = ijdigNxQfk2188460;     ijdigNxQfk2188460 = ijdigNxQfk96725765;     ijdigNxQfk96725765 = ijdigNxQfk23211798;     ijdigNxQfk23211798 = ijdigNxQfk6963435;     ijdigNxQfk6963435 = ijdigNxQfk35590179;     ijdigNxQfk35590179 = ijdigNxQfk89965599;     ijdigNxQfk89965599 = ijdigNxQfk96388117;     ijdigNxQfk96388117 = ijdigNxQfk7754655;     ijdigNxQfk7754655 = ijdigNxQfk69196622;     ijdigNxQfk69196622 = ijdigNxQfk56073546;     ijdigNxQfk56073546 = ijdigNxQfk75020526;     ijdigNxQfk75020526 = ijdigNxQfk78960045;     ijdigNxQfk78960045 = ijdigNxQfk69519450;     ijdigNxQfk69519450 = ijdigNxQfk45215146;     ijdigNxQfk45215146 = ijdigNxQfk53234490;     ijdigNxQfk53234490 = ijdigNxQfk74899768;     ijdigNxQfk74899768 = ijdigNxQfk18245557;     ijdigNxQfk18245557 = ijdigNxQfk27037259;     ijdigNxQfk27037259 = ijdigNxQfk71485996;     ijdigNxQfk71485996 = ijdigNxQfk31679176;     ijdigNxQfk31679176 = ijdigNxQfk7358792;     ijdigNxQfk7358792 = ijdigNxQfk71680265;     ijdigNxQfk71680265 = ijdigNxQfk6004046;     ijdigNxQfk6004046 = ijdigNxQfk27308351;     ijdigNxQfk27308351 = ijdigNxQfk80955894;     ijdigNxQfk80955894 = ijdigNxQfk40254477;     ijdigNxQfk40254477 = ijdigNxQfk66457346;     ijdigNxQfk66457346 = ijdigNxQfk5065166;     ijdigNxQfk5065166 = ijdigNxQfk830815;     ijdigNxQfk830815 = ijdigNxQfk62872809;     ijdigNxQfk62872809 = ijdigNxQfk2941303;     ijdigNxQfk2941303 = ijdigNxQfk2816641;     ijdigNxQfk2816641 = ijdigNxQfk63673851;     ijdigNxQfk63673851 = ijdigNxQfk89099712;     ijdigNxQfk89099712 = ijdigNxQfk45035589;     ijdigNxQfk45035589 = ijdigNxQfk79935371;     ijdigNxQfk79935371 = ijdigNxQfk69089109;     ijdigNxQfk69089109 = ijdigNxQfk96591025;     ijdigNxQfk96591025 = ijdigNxQfk25280325;     ijdigNxQfk25280325 = ijdigNxQfk83531661;     ijdigNxQfk83531661 = ijdigNxQfk86330657;     ijdigNxQfk86330657 = ijdigNxQfk97751461;     ijdigNxQfk97751461 = ijdigNxQfk34665594;     ijdigNxQfk34665594 = ijdigNxQfk43800759;     ijdigNxQfk43800759 = ijdigNxQfk37023798;     ijdigNxQfk37023798 = ijdigNxQfk94743374;     ijdigNxQfk94743374 = ijdigNxQfk92811658;     ijdigNxQfk92811658 = ijdigNxQfk97565715;     ijdigNxQfk97565715 = ijdigNxQfk11537727;     ijdigNxQfk11537727 = ijdigNxQfk65141107;     ijdigNxQfk65141107 = ijdigNxQfk95697635;     ijdigNxQfk95697635 = ijdigNxQfk72466810;     ijdigNxQfk72466810 = ijdigNxQfk26123418;     ijdigNxQfk26123418 = ijdigNxQfk45844467;     ijdigNxQfk45844467 = ijdigNxQfk99787428;     ijdigNxQfk99787428 = ijdigNxQfk30508195;     ijdigNxQfk30508195 = ijdigNxQfk90721720;     ijdigNxQfk90721720 = ijdigNxQfk95903447;     ijdigNxQfk95903447 = ijdigNxQfk26007540;     ijdigNxQfk26007540 = ijdigNxQfk95335702;     ijdigNxQfk95335702 = ijdigNxQfk23508254;     ijdigNxQfk23508254 = ijdigNxQfk91322951;     ijdigNxQfk91322951 = ijdigNxQfk6923840;     ijdigNxQfk6923840 = ijdigNxQfk6323814;     ijdigNxQfk6323814 = ijdigNxQfk53132244;     ijdigNxQfk53132244 = ijdigNxQfk72203886;     ijdigNxQfk72203886 = ijdigNxQfk15286195;     ijdigNxQfk15286195 = ijdigNxQfk80419737;     ijdigNxQfk80419737 = ijdigNxQfk179558;     ijdigNxQfk179558 = ijdigNxQfk73299119;     ijdigNxQfk73299119 = ijdigNxQfk5810659;     ijdigNxQfk5810659 = ijdigNxQfk21654532;     ijdigNxQfk21654532 = ijdigNxQfk1756935;     ijdigNxQfk1756935 = ijdigNxQfk87954335;     ijdigNxQfk87954335 = ijdigNxQfk45348518;     ijdigNxQfk45348518 = ijdigNxQfk9607330;     ijdigNxQfk9607330 = ijdigNxQfk37014671;     ijdigNxQfk37014671 = ijdigNxQfk62203286;     ijdigNxQfk62203286 = ijdigNxQfk90284553;     ijdigNxQfk90284553 = ijdigNxQfk86212519;     ijdigNxQfk86212519 = ijdigNxQfk47442818;     ijdigNxQfk47442818 = ijdigNxQfk68891630;     ijdigNxQfk68891630 = ijdigNxQfk93527439;     ijdigNxQfk93527439 = ijdigNxQfk35689708;     ijdigNxQfk35689708 = ijdigNxQfk67175173;     ijdigNxQfk67175173 = ijdigNxQfk30474492;     ijdigNxQfk30474492 = ijdigNxQfk76693222;     ijdigNxQfk76693222 = ijdigNxQfk3862217;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void IDqQkxcsFp42151091() {     int dlbbzjqMJv26249335 = -438280737;    int dlbbzjqMJv84221806 = 33892069;    int dlbbzjqMJv40813603 = 20147492;    int dlbbzjqMJv88710108 = -25057374;    int dlbbzjqMJv60505363 = -786014722;    int dlbbzjqMJv57663978 = -84190041;    int dlbbzjqMJv56927955 = -492596459;    int dlbbzjqMJv15674734 = -810339154;    int dlbbzjqMJv11106324 = -939111273;    int dlbbzjqMJv91267654 = -843637491;    int dlbbzjqMJv35151274 = -684864811;    int dlbbzjqMJv33426592 = -253391626;    int dlbbzjqMJv74154401 = -905698296;    int dlbbzjqMJv44803212 = -653679778;    int dlbbzjqMJv33461043 = -491166582;    int dlbbzjqMJv67467710 = -310419799;    int dlbbzjqMJv56786786 = -856655137;    int dlbbzjqMJv41566595 = -799107757;    int dlbbzjqMJv31247230 = -320627979;    int dlbbzjqMJv74904401 = -474825819;    int dlbbzjqMJv8637762 = -571627396;    int dlbbzjqMJv33011841 = -399310882;    int dlbbzjqMJv59366869 = -184099508;    int dlbbzjqMJv1520314 = -535564102;    int dlbbzjqMJv14500293 = -307856353;    int dlbbzjqMJv57822445 = -282086390;    int dlbbzjqMJv92304566 = -319841261;    int dlbbzjqMJv59145940 = -261632729;    int dlbbzjqMJv81792328 = -743474263;    int dlbbzjqMJv27402737 = -944545229;    int dlbbzjqMJv71328064 = -297740696;    int dlbbzjqMJv29027188 = -121436317;    int dlbbzjqMJv12717847 = -180108220;    int dlbbzjqMJv66747479 = -273509548;    int dlbbzjqMJv47909630 = 8268494;    int dlbbzjqMJv47696915 = -313213389;    int dlbbzjqMJv20748063 = -890774159;    int dlbbzjqMJv34581995 = 57314679;    int dlbbzjqMJv88562608 = 57828935;    int dlbbzjqMJv86643060 = -574442522;    int dlbbzjqMJv33822537 = -713242236;    int dlbbzjqMJv89339206 = -450668498;    int dlbbzjqMJv68423001 = -333781649;    int dlbbzjqMJv2385422 = -790786862;    int dlbbzjqMJv70799907 = -713741686;    int dlbbzjqMJv71774133 = -395346615;    int dlbbzjqMJv1397111 = -198299192;    int dlbbzjqMJv89634263 = -682226260;    int dlbbzjqMJv74395276 = -564156831;    int dlbbzjqMJv66172855 = -355460074;    int dlbbzjqMJv5460242 = -746899130;    int dlbbzjqMJv99131581 = -590867435;    int dlbbzjqMJv61846065 = -747458468;    int dlbbzjqMJv46617203 = -618270034;    int dlbbzjqMJv33513330 = -716938581;    int dlbbzjqMJv93237493 = 61030144;    int dlbbzjqMJv24854937 = -782008424;    int dlbbzjqMJv39293289 = -444288406;    int dlbbzjqMJv74209816 = -717201021;    int dlbbzjqMJv2682919 = -403928332;    int dlbbzjqMJv65359412 = -764348780;    int dlbbzjqMJv97782015 = -130963731;    int dlbbzjqMJv33882405 = 33135109;    int dlbbzjqMJv83703587 = -994566045;    int dlbbzjqMJv19939590 = -445896795;    int dlbbzjqMJv6124087 = -463428495;    int dlbbzjqMJv20708746 = 26716594;    int dlbbzjqMJv7406922 = -532188749;    int dlbbzjqMJv96893582 = -561948273;    int dlbbzjqMJv85764128 = -77953194;    int dlbbzjqMJv46719648 = -419645640;    int dlbbzjqMJv22204791 = -813969817;    int dlbbzjqMJv53003987 = -756936693;    int dlbbzjqMJv44604170 = -746185457;    int dlbbzjqMJv41081864 = -761583584;    int dlbbzjqMJv19298556 = -20958899;    int dlbbzjqMJv64588840 = 34470767;    int dlbbzjqMJv56981448 = -393312646;    int dlbbzjqMJv30720407 = -821822417;    int dlbbzjqMJv42726160 = -912509738;    int dlbbzjqMJv56425335 = 16212801;    int dlbbzjqMJv2670304 = -637615001;    int dlbbzjqMJv84750664 = -697475899;    int dlbbzjqMJv15619474 = -288014190;    int dlbbzjqMJv21942495 = -97646099;    int dlbbzjqMJv72196482 = -706873262;    int dlbbzjqMJv67181122 = -373977850;    int dlbbzjqMJv66100643 = -561838186;    int dlbbzjqMJv33234150 = -556570968;    int dlbbzjqMJv54672137 = 47238349;    int dlbbzjqMJv22841978 = -531204966;    int dlbbzjqMJv81454774 = -346485753;    int dlbbzjqMJv60372178 = -225484300;    int dlbbzjqMJv85879689 = -538242734;    int dlbbzjqMJv21283648 = -810093743;    int dlbbzjqMJv36040521 = -482278506;    int dlbbzjqMJv55456801 = -383803607;    int dlbbzjqMJv84719414 = -339215605;    int dlbbzjqMJv82445831 = -244890067;    int dlbbzjqMJv64675820 = -438280737;     dlbbzjqMJv26249335 = dlbbzjqMJv84221806;     dlbbzjqMJv84221806 = dlbbzjqMJv40813603;     dlbbzjqMJv40813603 = dlbbzjqMJv88710108;     dlbbzjqMJv88710108 = dlbbzjqMJv60505363;     dlbbzjqMJv60505363 = dlbbzjqMJv57663978;     dlbbzjqMJv57663978 = dlbbzjqMJv56927955;     dlbbzjqMJv56927955 = dlbbzjqMJv15674734;     dlbbzjqMJv15674734 = dlbbzjqMJv11106324;     dlbbzjqMJv11106324 = dlbbzjqMJv91267654;     dlbbzjqMJv91267654 = dlbbzjqMJv35151274;     dlbbzjqMJv35151274 = dlbbzjqMJv33426592;     dlbbzjqMJv33426592 = dlbbzjqMJv74154401;     dlbbzjqMJv74154401 = dlbbzjqMJv44803212;     dlbbzjqMJv44803212 = dlbbzjqMJv33461043;     dlbbzjqMJv33461043 = dlbbzjqMJv67467710;     dlbbzjqMJv67467710 = dlbbzjqMJv56786786;     dlbbzjqMJv56786786 = dlbbzjqMJv41566595;     dlbbzjqMJv41566595 = dlbbzjqMJv31247230;     dlbbzjqMJv31247230 = dlbbzjqMJv74904401;     dlbbzjqMJv74904401 = dlbbzjqMJv8637762;     dlbbzjqMJv8637762 = dlbbzjqMJv33011841;     dlbbzjqMJv33011841 = dlbbzjqMJv59366869;     dlbbzjqMJv59366869 = dlbbzjqMJv1520314;     dlbbzjqMJv1520314 = dlbbzjqMJv14500293;     dlbbzjqMJv14500293 = dlbbzjqMJv57822445;     dlbbzjqMJv57822445 = dlbbzjqMJv92304566;     dlbbzjqMJv92304566 = dlbbzjqMJv59145940;     dlbbzjqMJv59145940 = dlbbzjqMJv81792328;     dlbbzjqMJv81792328 = dlbbzjqMJv27402737;     dlbbzjqMJv27402737 = dlbbzjqMJv71328064;     dlbbzjqMJv71328064 = dlbbzjqMJv29027188;     dlbbzjqMJv29027188 = dlbbzjqMJv12717847;     dlbbzjqMJv12717847 = dlbbzjqMJv66747479;     dlbbzjqMJv66747479 = dlbbzjqMJv47909630;     dlbbzjqMJv47909630 = dlbbzjqMJv47696915;     dlbbzjqMJv47696915 = dlbbzjqMJv20748063;     dlbbzjqMJv20748063 = dlbbzjqMJv34581995;     dlbbzjqMJv34581995 = dlbbzjqMJv88562608;     dlbbzjqMJv88562608 = dlbbzjqMJv86643060;     dlbbzjqMJv86643060 = dlbbzjqMJv33822537;     dlbbzjqMJv33822537 = dlbbzjqMJv89339206;     dlbbzjqMJv89339206 = dlbbzjqMJv68423001;     dlbbzjqMJv68423001 = dlbbzjqMJv2385422;     dlbbzjqMJv2385422 = dlbbzjqMJv70799907;     dlbbzjqMJv70799907 = dlbbzjqMJv71774133;     dlbbzjqMJv71774133 = dlbbzjqMJv1397111;     dlbbzjqMJv1397111 = dlbbzjqMJv89634263;     dlbbzjqMJv89634263 = dlbbzjqMJv74395276;     dlbbzjqMJv74395276 = dlbbzjqMJv66172855;     dlbbzjqMJv66172855 = dlbbzjqMJv5460242;     dlbbzjqMJv5460242 = dlbbzjqMJv99131581;     dlbbzjqMJv99131581 = dlbbzjqMJv61846065;     dlbbzjqMJv61846065 = dlbbzjqMJv46617203;     dlbbzjqMJv46617203 = dlbbzjqMJv33513330;     dlbbzjqMJv33513330 = dlbbzjqMJv93237493;     dlbbzjqMJv93237493 = dlbbzjqMJv24854937;     dlbbzjqMJv24854937 = dlbbzjqMJv39293289;     dlbbzjqMJv39293289 = dlbbzjqMJv74209816;     dlbbzjqMJv74209816 = dlbbzjqMJv2682919;     dlbbzjqMJv2682919 = dlbbzjqMJv65359412;     dlbbzjqMJv65359412 = dlbbzjqMJv97782015;     dlbbzjqMJv97782015 = dlbbzjqMJv33882405;     dlbbzjqMJv33882405 = dlbbzjqMJv83703587;     dlbbzjqMJv83703587 = dlbbzjqMJv19939590;     dlbbzjqMJv19939590 = dlbbzjqMJv6124087;     dlbbzjqMJv6124087 = dlbbzjqMJv20708746;     dlbbzjqMJv20708746 = dlbbzjqMJv7406922;     dlbbzjqMJv7406922 = dlbbzjqMJv96893582;     dlbbzjqMJv96893582 = dlbbzjqMJv85764128;     dlbbzjqMJv85764128 = dlbbzjqMJv46719648;     dlbbzjqMJv46719648 = dlbbzjqMJv22204791;     dlbbzjqMJv22204791 = dlbbzjqMJv53003987;     dlbbzjqMJv53003987 = dlbbzjqMJv44604170;     dlbbzjqMJv44604170 = dlbbzjqMJv41081864;     dlbbzjqMJv41081864 = dlbbzjqMJv19298556;     dlbbzjqMJv19298556 = dlbbzjqMJv64588840;     dlbbzjqMJv64588840 = dlbbzjqMJv56981448;     dlbbzjqMJv56981448 = dlbbzjqMJv30720407;     dlbbzjqMJv30720407 = dlbbzjqMJv42726160;     dlbbzjqMJv42726160 = dlbbzjqMJv56425335;     dlbbzjqMJv56425335 = dlbbzjqMJv2670304;     dlbbzjqMJv2670304 = dlbbzjqMJv84750664;     dlbbzjqMJv84750664 = dlbbzjqMJv15619474;     dlbbzjqMJv15619474 = dlbbzjqMJv21942495;     dlbbzjqMJv21942495 = dlbbzjqMJv72196482;     dlbbzjqMJv72196482 = dlbbzjqMJv67181122;     dlbbzjqMJv67181122 = dlbbzjqMJv66100643;     dlbbzjqMJv66100643 = dlbbzjqMJv33234150;     dlbbzjqMJv33234150 = dlbbzjqMJv54672137;     dlbbzjqMJv54672137 = dlbbzjqMJv22841978;     dlbbzjqMJv22841978 = dlbbzjqMJv81454774;     dlbbzjqMJv81454774 = dlbbzjqMJv60372178;     dlbbzjqMJv60372178 = dlbbzjqMJv85879689;     dlbbzjqMJv85879689 = dlbbzjqMJv21283648;     dlbbzjqMJv21283648 = dlbbzjqMJv36040521;     dlbbzjqMJv36040521 = dlbbzjqMJv55456801;     dlbbzjqMJv55456801 = dlbbzjqMJv84719414;     dlbbzjqMJv84719414 = dlbbzjqMJv82445831;     dlbbzjqMJv82445831 = dlbbzjqMJv64675820;     dlbbzjqMJv64675820 = dlbbzjqMJv26249335;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void kifwmNqteV78796066() {     int lRUBldtmID48636452 = -72684700;    int lRUBldtmID68569306 = 85349014;    int lRUBldtmID69582881 = -219506352;    int lRUBldtmID3716798 = -31464905;    int lRUBldtmID58679617 = -277071723;    int lRUBldtmID72547095 = 55201581;    int lRUBldtmID49083694 = -314583765;    int lRUBldtmID91308592 = -180935798;    int lRUBldtmID8269455 = -823307312;    int lRUBldtmID83031238 = -586378824;    int lRUBldtmID72693133 = -681489212;    int lRUBldtmID89329540 = -504136919;    int lRUBldtmID41162582 = -424415616;    int lRUBldtmID87417964 = -81388535;    int lRUBldtmID70196320 = -118644295;    int lRUBldtmID11723623 = -692087341;    int lRUBldtmID6610138 = -33934957;    int lRUBldtmID47543012 = -150850448;    int lRUBldtmID72528861 = -455730104;    int lRUBldtmID53420684 = -727460273;    int lRUBldtmID9520869 = -563114705;    int lRUBldtmID96827059 = -904077195;    int lRUBldtmID62660192 = -360180100;    int lRUBldtmID28020101 = 443040;    int lRUBldtmID50040539 = 70584523;    int lRUBldtmID46125439 = -725980614;    int lRUBldtmID39393987 = -77427754;    int lRUBldtmID65057390 = -50087183;    int lRUBldtmID88684889 = 12268618;    int lRUBldtmID36559916 = -864841095;    int lRUBldtmID15618870 = -379056848;    int lRUBldtmID86568378 = -773507672;    int lRUBldtmID93756517 = -709451715;    int lRUBldtmID26136167 = -597636398;    int lRUBldtmID24138995 = -55487929;    int lRUBldtmID89389784 = -628608766;    int lRUBldtmID14187775 = -623722110;    int lRUBldtmID88208095 = -910310793;    int lRUBldtmID36870739 = -343217097;    int lRUBldtmID6828775 = -710666552;    int lRUBldtmID62579908 = -116419902;    int lRUBldtmID77847597 = -208577189;    int lRUBldtmID73973193 = -221312233;    int lRUBldtmID1829541 = -788150511;    int lRUBldtmID38783174 = -522885164;    int lRUBldtmID79874414 = -577575711;    int lRUBldtmID13694508 = -776709445;    int lRUBldtmID34232938 = -438761251;    int lRUBldtmID68855181 = -513873529;    int lRUBldtmID63256600 = -497480333;    int lRUBldtmID14329459 = -1975407;    int lRUBldtmID72982839 = 78313283;    int lRUBldtmID40160468 = -419369698;    int lRUBldtmID6903749 = -893950843;    int lRUBldtmID69275197 = -857642696;    int lRUBldtmID51809392 = -168607506;    int lRUBldtmID5909114 = -554470887;    int lRUBldtmID41562781 = -119949393;    int lRUBldtmID53676258 = -2049428;    int lRUBldtmID12554178 = -551091109;    int lRUBldtmID33153109 = -867370665;    int lRUBldtmID84026304 = -164496583;    int lRUBldtmID2623704 = -93204417;    int lRUBldtmID71709539 = -958466217;    int lRUBldtmID67412369 = -107321976;    int lRUBldtmID86124755 = -907981541;    int lRUBldtmID95573023 = -794685204;    int lRUBldtmID15026416 = -826779218;    int lRUBldtmID63278970 = 74099393;    int lRUBldtmID80806536 = -490035530;    int lRUBldtmID97535847 = 31634769;    int lRUBldtmID18402042 = -123624164;    int lRUBldtmID10672273 = -807633352;    int lRUBldtmID65700086 = -745063553;    int lRUBldtmID90840776 = -511040371;    int lRUBldtmID31673272 = -254537516;    int lRUBldtmID22853866 = -582764962;    int lRUBldtmID60830652 = -572029589;    int lRUBldtmID89236927 = -476671797;    int lRUBldtmID70166125 = -351839767;    int lRUBldtmID32430932 = -949271170;    int lRUBldtmID5161049 = -638666503;    int lRUBldtmID96202209 = -536213654;    int lRUBldtmID25428289 = -490251050;    int lRUBldtmID22230458 = -762865689;    int lRUBldtmID42636030 = -357370132;    int lRUBldtmID46407910 = -254137974;    int lRUBldtmID86852768 = -815500872;    int lRUBldtmID56860970 = -739993703;    int lRUBldtmID72329602 = -886880423;    int lRUBldtmID83480670 = 25862121;    int lRUBldtmID72624994 = -403772718;    int lRUBldtmID34531838 = -808261365;    int lRUBldtmID24316562 = -792125988;    int lRUBldtmID73675666 = -843295887;    int lRUBldtmID78553603 = -951923319;    int lRUBldtmID75223894 = -15372773;    int lRUBldtmID2263655 = -262846016;    int lRUBldtmID34417172 = -580828535;    int lRUBldtmID52658418 = -72684700;     lRUBldtmID48636452 = lRUBldtmID68569306;     lRUBldtmID68569306 = lRUBldtmID69582881;     lRUBldtmID69582881 = lRUBldtmID3716798;     lRUBldtmID3716798 = lRUBldtmID58679617;     lRUBldtmID58679617 = lRUBldtmID72547095;     lRUBldtmID72547095 = lRUBldtmID49083694;     lRUBldtmID49083694 = lRUBldtmID91308592;     lRUBldtmID91308592 = lRUBldtmID8269455;     lRUBldtmID8269455 = lRUBldtmID83031238;     lRUBldtmID83031238 = lRUBldtmID72693133;     lRUBldtmID72693133 = lRUBldtmID89329540;     lRUBldtmID89329540 = lRUBldtmID41162582;     lRUBldtmID41162582 = lRUBldtmID87417964;     lRUBldtmID87417964 = lRUBldtmID70196320;     lRUBldtmID70196320 = lRUBldtmID11723623;     lRUBldtmID11723623 = lRUBldtmID6610138;     lRUBldtmID6610138 = lRUBldtmID47543012;     lRUBldtmID47543012 = lRUBldtmID72528861;     lRUBldtmID72528861 = lRUBldtmID53420684;     lRUBldtmID53420684 = lRUBldtmID9520869;     lRUBldtmID9520869 = lRUBldtmID96827059;     lRUBldtmID96827059 = lRUBldtmID62660192;     lRUBldtmID62660192 = lRUBldtmID28020101;     lRUBldtmID28020101 = lRUBldtmID50040539;     lRUBldtmID50040539 = lRUBldtmID46125439;     lRUBldtmID46125439 = lRUBldtmID39393987;     lRUBldtmID39393987 = lRUBldtmID65057390;     lRUBldtmID65057390 = lRUBldtmID88684889;     lRUBldtmID88684889 = lRUBldtmID36559916;     lRUBldtmID36559916 = lRUBldtmID15618870;     lRUBldtmID15618870 = lRUBldtmID86568378;     lRUBldtmID86568378 = lRUBldtmID93756517;     lRUBldtmID93756517 = lRUBldtmID26136167;     lRUBldtmID26136167 = lRUBldtmID24138995;     lRUBldtmID24138995 = lRUBldtmID89389784;     lRUBldtmID89389784 = lRUBldtmID14187775;     lRUBldtmID14187775 = lRUBldtmID88208095;     lRUBldtmID88208095 = lRUBldtmID36870739;     lRUBldtmID36870739 = lRUBldtmID6828775;     lRUBldtmID6828775 = lRUBldtmID62579908;     lRUBldtmID62579908 = lRUBldtmID77847597;     lRUBldtmID77847597 = lRUBldtmID73973193;     lRUBldtmID73973193 = lRUBldtmID1829541;     lRUBldtmID1829541 = lRUBldtmID38783174;     lRUBldtmID38783174 = lRUBldtmID79874414;     lRUBldtmID79874414 = lRUBldtmID13694508;     lRUBldtmID13694508 = lRUBldtmID34232938;     lRUBldtmID34232938 = lRUBldtmID68855181;     lRUBldtmID68855181 = lRUBldtmID63256600;     lRUBldtmID63256600 = lRUBldtmID14329459;     lRUBldtmID14329459 = lRUBldtmID72982839;     lRUBldtmID72982839 = lRUBldtmID40160468;     lRUBldtmID40160468 = lRUBldtmID6903749;     lRUBldtmID6903749 = lRUBldtmID69275197;     lRUBldtmID69275197 = lRUBldtmID51809392;     lRUBldtmID51809392 = lRUBldtmID5909114;     lRUBldtmID5909114 = lRUBldtmID41562781;     lRUBldtmID41562781 = lRUBldtmID53676258;     lRUBldtmID53676258 = lRUBldtmID12554178;     lRUBldtmID12554178 = lRUBldtmID33153109;     lRUBldtmID33153109 = lRUBldtmID84026304;     lRUBldtmID84026304 = lRUBldtmID2623704;     lRUBldtmID2623704 = lRUBldtmID71709539;     lRUBldtmID71709539 = lRUBldtmID67412369;     lRUBldtmID67412369 = lRUBldtmID86124755;     lRUBldtmID86124755 = lRUBldtmID95573023;     lRUBldtmID95573023 = lRUBldtmID15026416;     lRUBldtmID15026416 = lRUBldtmID63278970;     lRUBldtmID63278970 = lRUBldtmID80806536;     lRUBldtmID80806536 = lRUBldtmID97535847;     lRUBldtmID97535847 = lRUBldtmID18402042;     lRUBldtmID18402042 = lRUBldtmID10672273;     lRUBldtmID10672273 = lRUBldtmID65700086;     lRUBldtmID65700086 = lRUBldtmID90840776;     lRUBldtmID90840776 = lRUBldtmID31673272;     lRUBldtmID31673272 = lRUBldtmID22853866;     lRUBldtmID22853866 = lRUBldtmID60830652;     lRUBldtmID60830652 = lRUBldtmID89236927;     lRUBldtmID89236927 = lRUBldtmID70166125;     lRUBldtmID70166125 = lRUBldtmID32430932;     lRUBldtmID32430932 = lRUBldtmID5161049;     lRUBldtmID5161049 = lRUBldtmID96202209;     lRUBldtmID96202209 = lRUBldtmID25428289;     lRUBldtmID25428289 = lRUBldtmID22230458;     lRUBldtmID22230458 = lRUBldtmID42636030;     lRUBldtmID42636030 = lRUBldtmID46407910;     lRUBldtmID46407910 = lRUBldtmID86852768;     lRUBldtmID86852768 = lRUBldtmID56860970;     lRUBldtmID56860970 = lRUBldtmID72329602;     lRUBldtmID72329602 = lRUBldtmID83480670;     lRUBldtmID83480670 = lRUBldtmID72624994;     lRUBldtmID72624994 = lRUBldtmID34531838;     lRUBldtmID34531838 = lRUBldtmID24316562;     lRUBldtmID24316562 = lRUBldtmID73675666;     lRUBldtmID73675666 = lRUBldtmID78553603;     lRUBldtmID78553603 = lRUBldtmID75223894;     lRUBldtmID75223894 = lRUBldtmID2263655;     lRUBldtmID2263655 = lRUBldtmID34417172;     lRUBldtmID34417172 = lRUBldtmID52658418;     lRUBldtmID52658418 = lRUBldtmID48636452;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void geYBoqgCMP15441041() {     int PrceeGZaTy71023569 = -807088664;    int PrceeGZaTy52916807 = -963194040;    int PrceeGZaTy98352160 = -459160196;    int PrceeGZaTy18723486 = -37872437;    int PrceeGZaTy56853871 = -868128724;    int PrceeGZaTy87430212 = -905406797;    int PrceeGZaTy41239433 = -136571071;    int PrceeGZaTy66942451 = -651532443;    int PrceeGZaTy5432586 = -707503351;    int PrceeGZaTy74794823 = -329120157;    int PrceeGZaTy10234994 = -678113613;    int PrceeGZaTy45232490 = -754882212;    int PrceeGZaTy8170763 = 56867064;    int PrceeGZaTy30032717 = -609097292;    int PrceeGZaTy6931597 = -846122008;    int PrceeGZaTy55979535 = 26245117;    int PrceeGZaTy56433489 = -311214777;    int PrceeGZaTy53519428 = -602593139;    int PrceeGZaTy13810492 = -590832229;    int PrceeGZaTy31936968 = -980094726;    int PrceeGZaTy10403977 = -554602013;    int PrceeGZaTy60642278 = -308843508;    int PrceeGZaTy65953515 = -536260692;    int PrceeGZaTy54519888 = -563549817;    int PrceeGZaTy85580786 = -650974602;    int PrceeGZaTy34428434 = -69874838;    int PrceeGZaTy86483407 = -935014247;    int PrceeGZaTy70968840 = -938541636;    int PrceeGZaTy95577450 = -331988502;    int PrceeGZaTy45717095 = -785136962;    int PrceeGZaTy59909674 = -460373000;    int PrceeGZaTy44109569 = -325579027;    int PrceeGZaTy74795187 = -138795210;    int PrceeGZaTy85524854 = -921763249;    int PrceeGZaTy368360 = -119244351;    int PrceeGZaTy31082653 = -944004142;    int PrceeGZaTy7627488 = -356670062;    int PrceeGZaTy41834197 = -777936265;    int PrceeGZaTy85178870 = -744263128;    int PrceeGZaTy27014490 = -846890581;    int PrceeGZaTy91337278 = -619597568;    int PrceeGZaTy66355988 = 33514120;    int PrceeGZaTy79523385 = -108842817;    int PrceeGZaTy1273660 = -785514160;    int PrceeGZaTy6766441 = -332028641;    int PrceeGZaTy87974696 = -759804806;    int PrceeGZaTy25991905 = -255119698;    int PrceeGZaTy78831612 = -195296242;    int PrceeGZaTy63315086 = -463590228;    int PrceeGZaTy60340346 = -639500592;    int PrceeGZaTy23198675 = -357051684;    int PrceeGZaTy46834096 = -352505999;    int PrceeGZaTy18474871 = -91280928;    int PrceeGZaTy67190293 = -69631652;    int PrceeGZaTy5037065 = -998346812;    int PrceeGZaTy10381292 = -398245156;    int PrceeGZaTy86963291 = -326933349;    int PrceeGZaTy43832272 = -895610380;    int PrceeGZaTy33142700 = -386897836;    int PrceeGZaTy22425438 = -698253887;    int PrceeGZaTy946806 = -970392551;    int PrceeGZaTy70270593 = -198029436;    int PrceeGZaTy71365001 = -219543942;    int PrceeGZaTy59715490 = -922366390;    int PrceeGZaTy14885149 = -868747157;    int PrceeGZaTy66125424 = -252534587;    int PrceeGZaTy70437302 = -516087003;    int PrceeGZaTy22645909 = -21369688;    int PrceeGZaTy29664358 = -389852942;    int PrceeGZaTy75848943 = -902117867;    int PrceeGZaTy48352048 = -617084822;    int PrceeGZaTy14599293 = -533278512;    int PrceeGZaTy68340557 = -858330011;    int PrceeGZaTy86796002 = -743941649;    int PrceeGZaTy40599689 = -260497159;    int PrceeGZaTy44047988 = -488116134;    int PrceeGZaTy81118892 = -100000692;    int PrceeGZaTy64679856 = -750746532;    int PrceeGZaTy47753448 = -131521176;    int PrceeGZaTy97606089 = -891169796;    int PrceeGZaTy8436529 = -814755141;    int PrceeGZaTy7651795 = -639718005;    int PrceeGZaTy7653755 = -374951409;    int PrceeGZaTy35237105 = -692487910;    int PrceeGZaTy22518421 = -328085278;    int PrceeGZaTy13075579 = -7867002;    int PrceeGZaTy25634698 = -134298099;    int PrceeGZaTy7604894 = 30836441;    int PrceeGZaTy80487789 = -923416437;    int PrceeGZaTy89987067 = -720999196;    int PrceeGZaTy44119362 = -517070793;    int PrceeGZaTy63795215 = -461059682;    int PrceeGZaTy8691497 = -291038430;    int PrceeGZaTy62753433 = 53990758;    int PrceeGZaTy26067684 = -876498031;    int PrceeGZaTy21066686 = -321568133;    int PrceeGZaTy94990987 = -746941939;    int PrceeGZaTy19807895 = -186476428;    int PrceeGZaTy86388511 = -916767003;    int PrceeGZaTy40641016 = -807088664;     PrceeGZaTy71023569 = PrceeGZaTy52916807;     PrceeGZaTy52916807 = PrceeGZaTy98352160;     PrceeGZaTy98352160 = PrceeGZaTy18723486;     PrceeGZaTy18723486 = PrceeGZaTy56853871;     PrceeGZaTy56853871 = PrceeGZaTy87430212;     PrceeGZaTy87430212 = PrceeGZaTy41239433;     PrceeGZaTy41239433 = PrceeGZaTy66942451;     PrceeGZaTy66942451 = PrceeGZaTy5432586;     PrceeGZaTy5432586 = PrceeGZaTy74794823;     PrceeGZaTy74794823 = PrceeGZaTy10234994;     PrceeGZaTy10234994 = PrceeGZaTy45232490;     PrceeGZaTy45232490 = PrceeGZaTy8170763;     PrceeGZaTy8170763 = PrceeGZaTy30032717;     PrceeGZaTy30032717 = PrceeGZaTy6931597;     PrceeGZaTy6931597 = PrceeGZaTy55979535;     PrceeGZaTy55979535 = PrceeGZaTy56433489;     PrceeGZaTy56433489 = PrceeGZaTy53519428;     PrceeGZaTy53519428 = PrceeGZaTy13810492;     PrceeGZaTy13810492 = PrceeGZaTy31936968;     PrceeGZaTy31936968 = PrceeGZaTy10403977;     PrceeGZaTy10403977 = PrceeGZaTy60642278;     PrceeGZaTy60642278 = PrceeGZaTy65953515;     PrceeGZaTy65953515 = PrceeGZaTy54519888;     PrceeGZaTy54519888 = PrceeGZaTy85580786;     PrceeGZaTy85580786 = PrceeGZaTy34428434;     PrceeGZaTy34428434 = PrceeGZaTy86483407;     PrceeGZaTy86483407 = PrceeGZaTy70968840;     PrceeGZaTy70968840 = PrceeGZaTy95577450;     PrceeGZaTy95577450 = PrceeGZaTy45717095;     PrceeGZaTy45717095 = PrceeGZaTy59909674;     PrceeGZaTy59909674 = PrceeGZaTy44109569;     PrceeGZaTy44109569 = PrceeGZaTy74795187;     PrceeGZaTy74795187 = PrceeGZaTy85524854;     PrceeGZaTy85524854 = PrceeGZaTy368360;     PrceeGZaTy368360 = PrceeGZaTy31082653;     PrceeGZaTy31082653 = PrceeGZaTy7627488;     PrceeGZaTy7627488 = PrceeGZaTy41834197;     PrceeGZaTy41834197 = PrceeGZaTy85178870;     PrceeGZaTy85178870 = PrceeGZaTy27014490;     PrceeGZaTy27014490 = PrceeGZaTy91337278;     PrceeGZaTy91337278 = PrceeGZaTy66355988;     PrceeGZaTy66355988 = PrceeGZaTy79523385;     PrceeGZaTy79523385 = PrceeGZaTy1273660;     PrceeGZaTy1273660 = PrceeGZaTy6766441;     PrceeGZaTy6766441 = PrceeGZaTy87974696;     PrceeGZaTy87974696 = PrceeGZaTy25991905;     PrceeGZaTy25991905 = PrceeGZaTy78831612;     PrceeGZaTy78831612 = PrceeGZaTy63315086;     PrceeGZaTy63315086 = PrceeGZaTy60340346;     PrceeGZaTy60340346 = PrceeGZaTy23198675;     PrceeGZaTy23198675 = PrceeGZaTy46834096;     PrceeGZaTy46834096 = PrceeGZaTy18474871;     PrceeGZaTy18474871 = PrceeGZaTy67190293;     PrceeGZaTy67190293 = PrceeGZaTy5037065;     PrceeGZaTy5037065 = PrceeGZaTy10381292;     PrceeGZaTy10381292 = PrceeGZaTy86963291;     PrceeGZaTy86963291 = PrceeGZaTy43832272;     PrceeGZaTy43832272 = PrceeGZaTy33142700;     PrceeGZaTy33142700 = PrceeGZaTy22425438;     PrceeGZaTy22425438 = PrceeGZaTy946806;     PrceeGZaTy946806 = PrceeGZaTy70270593;     PrceeGZaTy70270593 = PrceeGZaTy71365001;     PrceeGZaTy71365001 = PrceeGZaTy59715490;     PrceeGZaTy59715490 = PrceeGZaTy14885149;     PrceeGZaTy14885149 = PrceeGZaTy66125424;     PrceeGZaTy66125424 = PrceeGZaTy70437302;     PrceeGZaTy70437302 = PrceeGZaTy22645909;     PrceeGZaTy22645909 = PrceeGZaTy29664358;     PrceeGZaTy29664358 = PrceeGZaTy75848943;     PrceeGZaTy75848943 = PrceeGZaTy48352048;     PrceeGZaTy48352048 = PrceeGZaTy14599293;     PrceeGZaTy14599293 = PrceeGZaTy68340557;     PrceeGZaTy68340557 = PrceeGZaTy86796002;     PrceeGZaTy86796002 = PrceeGZaTy40599689;     PrceeGZaTy40599689 = PrceeGZaTy44047988;     PrceeGZaTy44047988 = PrceeGZaTy81118892;     PrceeGZaTy81118892 = PrceeGZaTy64679856;     PrceeGZaTy64679856 = PrceeGZaTy47753448;     PrceeGZaTy47753448 = PrceeGZaTy97606089;     PrceeGZaTy97606089 = PrceeGZaTy8436529;     PrceeGZaTy8436529 = PrceeGZaTy7651795;     PrceeGZaTy7651795 = PrceeGZaTy7653755;     PrceeGZaTy7653755 = PrceeGZaTy35237105;     PrceeGZaTy35237105 = PrceeGZaTy22518421;     PrceeGZaTy22518421 = PrceeGZaTy13075579;     PrceeGZaTy13075579 = PrceeGZaTy25634698;     PrceeGZaTy25634698 = PrceeGZaTy7604894;     PrceeGZaTy7604894 = PrceeGZaTy80487789;     PrceeGZaTy80487789 = PrceeGZaTy89987067;     PrceeGZaTy89987067 = PrceeGZaTy44119362;     PrceeGZaTy44119362 = PrceeGZaTy63795215;     PrceeGZaTy63795215 = PrceeGZaTy8691497;     PrceeGZaTy8691497 = PrceeGZaTy62753433;     PrceeGZaTy62753433 = PrceeGZaTy26067684;     PrceeGZaTy26067684 = PrceeGZaTy21066686;     PrceeGZaTy21066686 = PrceeGZaTy94990987;     PrceeGZaTy94990987 = PrceeGZaTy19807895;     PrceeGZaTy19807895 = PrceeGZaTy86388511;     PrceeGZaTy86388511 = PrceeGZaTy40641016;     PrceeGZaTy40641016 = PrceeGZaTy71023569;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void KVRwfVmbpl52086015() {     int VRtssOpYje93410686 = -441492627;    int VRtssOpYje37264307 = -911737095;    int VRtssOpYje27121439 = -698814041;    int VRtssOpYje33730174 = -44279969;    int VRtssOpYje55028125 = -359185725;    int VRtssOpYje2313330 = -766015174;    int VRtssOpYje33395172 = 41441622;    int VRtssOpYje42576310 = -22129087;    int VRtssOpYje2595717 = -591699390;    int VRtssOpYje66558408 = -71861490;    int VRtssOpYje47776853 = -674738014;    int VRtssOpYje1135439 = 94372494;    int VRtssOpYje75178943 = -561850255;    int VRtssOpYje72647469 = -36806049;    int VRtssOpYje43666874 = -473599721;    int VRtssOpYje235448 = -355422426;    int VRtssOpYje6256842 = -588494596;    int VRtssOpYje59495844 = 45664171;    int VRtssOpYje55092123 = -725934354;    int VRtssOpYje10453251 = -132729180;    int VRtssOpYje11287084 = -546089322;    int VRtssOpYje24457496 = -813609822;    int VRtssOpYje69246839 = -712341284;    int VRtssOpYje81019675 = -27542674;    int VRtssOpYje21121033 = -272533726;    int VRtssOpYje22731429 = -513769062;    int VRtssOpYje33572828 = -692600739;    int VRtssOpYje76880290 = -726996090;    int VRtssOpYje2470012 = -676245621;    int VRtssOpYje54874275 = -705432829;    int VRtssOpYje4200480 = -541689152;    int VRtssOpYje1650761 = -977650381;    int VRtssOpYje55833858 = -668138705;    int VRtssOpYje44913541 = -145890099;    int VRtssOpYje76597724 = -183000774;    int VRtssOpYje72775522 = -159399518;    int VRtssOpYje1067200 = -89618013;    int VRtssOpYje95460297 = -645561737;    int VRtssOpYje33487002 = -45309160;    int VRtssOpYje47200204 = -983114610;    int VRtssOpYje20094650 = -22775234;    int VRtssOpYje54864380 = -824394571;    int VRtssOpYje85073577 = 3626599;    int VRtssOpYje717780 = -782877809;    int VRtssOpYje74749707 = -141172119;    int VRtssOpYje96074978 = -942033902;    int VRtssOpYje38289302 = -833529951;    int VRtssOpYje23430287 = 48168768;    int VRtssOpYje57774991 = -413306926;    int VRtssOpYje57424092 = -781520851;    int VRtssOpYje32067892 = -712127961;    int VRtssOpYje20685354 = -783325281;    int VRtssOpYje96789274 = -863192158;    int VRtssOpYje27476839 = -345312461;    int VRtssOpYje40798932 = -39050927;    int VRtssOpYje68953190 = -627882806;    int VRtssOpYje68017468 = -99395812;    int VRtssOpYje46101764 = -571271367;    int VRtssOpYje12609141 = -771746243;    int VRtssOpYje32296697 = -845416664;    int VRtssOpYje68740502 = 26585564;    int VRtssOpYje56514882 = -231562288;    int VRtssOpYje40106299 = -345883467;    int VRtssOpYje47721442 = -886266562;    int VRtssOpYje62357928 = -530172338;    int VRtssOpYje46126093 = -697087633;    int VRtssOpYje45301581 = -237488801;    int VRtssOpYje30265402 = -315960157;    int VRtssOpYje96049744 = -853805276;    int VRtssOpYje70891351 = -214200203;    int VRtssOpYje99168248 = -165804413;    int VRtssOpYje10796544 = -942932860;    int VRtssOpYje26008842 = -909026670;    int VRtssOpYje7891919 = -742819745;    int VRtssOpYje90358601 = -9953946;    int VRtssOpYje56422704 = -721694751;    int VRtssOpYje39383919 = -717236421;    int VRtssOpYje68529059 = -929463475;    int VRtssOpYje6269969 = -886370556;    int VRtssOpYje25046055 = -330499825;    int VRtssOpYje84442126 = -680239112;    int VRtssOpYje10142541 = -640769507;    int VRtssOpYje19105300 = -213689164;    int VRtssOpYje45045920 = -894724770;    int VRtssOpYje22806384 = -993304868;    int VRtssOpYje83515126 = -758363872;    int VRtssOpYje4861486 = -14458223;    int VRtssOpYje28357019 = -222826245;    int VRtssOpYje4114610 = -6839172;    int VRtssOpYje7644534 = -555117969;    int VRtssOpYje4758054 = 39996293;    int VRtssOpYje54965436 = -518346647;    int VRtssOpYje82851156 = -873815494;    int VRtssOpYje1190305 = -199892497;    int VRtssOpYje78459702 = -909700175;    int VRtssOpYje63579767 = -791212946;    int VRtssOpYje14758081 = -378511105;    int VRtssOpYje37352135 = -110106839;    int VRtssOpYje38359851 = -152705471;    int VRtssOpYje28623614 = -441492627;     VRtssOpYje93410686 = VRtssOpYje37264307;     VRtssOpYje37264307 = VRtssOpYje27121439;     VRtssOpYje27121439 = VRtssOpYje33730174;     VRtssOpYje33730174 = VRtssOpYje55028125;     VRtssOpYje55028125 = VRtssOpYje2313330;     VRtssOpYje2313330 = VRtssOpYje33395172;     VRtssOpYje33395172 = VRtssOpYje42576310;     VRtssOpYje42576310 = VRtssOpYje2595717;     VRtssOpYje2595717 = VRtssOpYje66558408;     VRtssOpYje66558408 = VRtssOpYje47776853;     VRtssOpYje47776853 = VRtssOpYje1135439;     VRtssOpYje1135439 = VRtssOpYje75178943;     VRtssOpYje75178943 = VRtssOpYje72647469;     VRtssOpYje72647469 = VRtssOpYje43666874;     VRtssOpYje43666874 = VRtssOpYje235448;     VRtssOpYje235448 = VRtssOpYje6256842;     VRtssOpYje6256842 = VRtssOpYje59495844;     VRtssOpYje59495844 = VRtssOpYje55092123;     VRtssOpYje55092123 = VRtssOpYje10453251;     VRtssOpYje10453251 = VRtssOpYje11287084;     VRtssOpYje11287084 = VRtssOpYje24457496;     VRtssOpYje24457496 = VRtssOpYje69246839;     VRtssOpYje69246839 = VRtssOpYje81019675;     VRtssOpYje81019675 = VRtssOpYje21121033;     VRtssOpYje21121033 = VRtssOpYje22731429;     VRtssOpYje22731429 = VRtssOpYje33572828;     VRtssOpYje33572828 = VRtssOpYje76880290;     VRtssOpYje76880290 = VRtssOpYje2470012;     VRtssOpYje2470012 = VRtssOpYje54874275;     VRtssOpYje54874275 = VRtssOpYje4200480;     VRtssOpYje4200480 = VRtssOpYje1650761;     VRtssOpYje1650761 = VRtssOpYje55833858;     VRtssOpYje55833858 = VRtssOpYje44913541;     VRtssOpYje44913541 = VRtssOpYje76597724;     VRtssOpYje76597724 = VRtssOpYje72775522;     VRtssOpYje72775522 = VRtssOpYje1067200;     VRtssOpYje1067200 = VRtssOpYje95460297;     VRtssOpYje95460297 = VRtssOpYje33487002;     VRtssOpYje33487002 = VRtssOpYje47200204;     VRtssOpYje47200204 = VRtssOpYje20094650;     VRtssOpYje20094650 = VRtssOpYje54864380;     VRtssOpYje54864380 = VRtssOpYje85073577;     VRtssOpYje85073577 = VRtssOpYje717780;     VRtssOpYje717780 = VRtssOpYje74749707;     VRtssOpYje74749707 = VRtssOpYje96074978;     VRtssOpYje96074978 = VRtssOpYje38289302;     VRtssOpYje38289302 = VRtssOpYje23430287;     VRtssOpYje23430287 = VRtssOpYje57774991;     VRtssOpYje57774991 = VRtssOpYje57424092;     VRtssOpYje57424092 = VRtssOpYje32067892;     VRtssOpYje32067892 = VRtssOpYje20685354;     VRtssOpYje20685354 = VRtssOpYje96789274;     VRtssOpYje96789274 = VRtssOpYje27476839;     VRtssOpYje27476839 = VRtssOpYje40798932;     VRtssOpYje40798932 = VRtssOpYje68953190;     VRtssOpYje68953190 = VRtssOpYje68017468;     VRtssOpYje68017468 = VRtssOpYje46101764;     VRtssOpYje46101764 = VRtssOpYje12609141;     VRtssOpYje12609141 = VRtssOpYje32296697;     VRtssOpYje32296697 = VRtssOpYje68740502;     VRtssOpYje68740502 = VRtssOpYje56514882;     VRtssOpYje56514882 = VRtssOpYje40106299;     VRtssOpYje40106299 = VRtssOpYje47721442;     VRtssOpYje47721442 = VRtssOpYje62357928;     VRtssOpYje62357928 = VRtssOpYje46126093;     VRtssOpYje46126093 = VRtssOpYje45301581;     VRtssOpYje45301581 = VRtssOpYje30265402;     VRtssOpYje30265402 = VRtssOpYje96049744;     VRtssOpYje96049744 = VRtssOpYje70891351;     VRtssOpYje70891351 = VRtssOpYje99168248;     VRtssOpYje99168248 = VRtssOpYje10796544;     VRtssOpYje10796544 = VRtssOpYje26008842;     VRtssOpYje26008842 = VRtssOpYje7891919;     VRtssOpYje7891919 = VRtssOpYje90358601;     VRtssOpYje90358601 = VRtssOpYje56422704;     VRtssOpYje56422704 = VRtssOpYje39383919;     VRtssOpYje39383919 = VRtssOpYje68529059;     VRtssOpYje68529059 = VRtssOpYje6269969;     VRtssOpYje6269969 = VRtssOpYje25046055;     VRtssOpYje25046055 = VRtssOpYje84442126;     VRtssOpYje84442126 = VRtssOpYje10142541;     VRtssOpYje10142541 = VRtssOpYje19105300;     VRtssOpYje19105300 = VRtssOpYje45045920;     VRtssOpYje45045920 = VRtssOpYje22806384;     VRtssOpYje22806384 = VRtssOpYje83515126;     VRtssOpYje83515126 = VRtssOpYje4861486;     VRtssOpYje4861486 = VRtssOpYje28357019;     VRtssOpYje28357019 = VRtssOpYje4114610;     VRtssOpYje4114610 = VRtssOpYje7644534;     VRtssOpYje7644534 = VRtssOpYje4758054;     VRtssOpYje4758054 = VRtssOpYje54965436;     VRtssOpYje54965436 = VRtssOpYje82851156;     VRtssOpYje82851156 = VRtssOpYje1190305;     VRtssOpYje1190305 = VRtssOpYje78459702;     VRtssOpYje78459702 = VRtssOpYje63579767;     VRtssOpYje63579767 = VRtssOpYje14758081;     VRtssOpYje14758081 = VRtssOpYje37352135;     VRtssOpYje37352135 = VRtssOpYje38359851;     VRtssOpYje38359851 = VRtssOpYje28623614;     VRtssOpYje28623614 = VRtssOpYje93410686;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void aFkDrWnbbT88730990() {     int QuOAOhMCwM15797804 = -75896590;    int QuOAOhMCwM21611808 = -860280150;    int QuOAOhMCwM55890718 = -938467885;    int QuOAOhMCwM48736863 = -50687500;    int QuOAOhMCwM53202379 = -950242727;    int QuOAOhMCwM17196447 = -626623552;    int QuOAOhMCwM25550911 = -880545684;    int QuOAOhMCwM18210169 = -492725731;    int QuOAOhMCwM99758847 = -475895429;    int QuOAOhMCwM58321992 = -914602823;    int QuOAOhMCwM85318712 = -671362415;    int QuOAOhMCwM57038388 = -156372799;    int QuOAOhMCwM42187124 = -80567575;    int QuOAOhMCwM15262221 = -564514806;    int QuOAOhMCwM80402151 = -101077434;    int QuOAOhMCwM44491361 = -737089968;    int QuOAOhMCwM56080193 = -865774416;    int QuOAOhMCwM65472260 = -406078520;    int QuOAOhMCwM96373753 = -861036480;    int QuOAOhMCwM88969534 = -385363634;    int QuOAOhMCwM12170191 = -537576630;    int QuOAOhMCwM88272714 = -218376135;    int QuOAOhMCwM72540162 = -888421876;    int QuOAOhMCwM7519463 = -591535531;    int QuOAOhMCwM56661280 = -994092851;    int QuOAOhMCwM11034423 = -957663286;    int QuOAOhMCwM80662248 = -450187232;    int QuOAOhMCwM82791740 = -515450543;    int QuOAOhMCwM9362573 = 79497260;    int QuOAOhMCwM64031454 = -625728695;    int QuOAOhMCwM48491285 = -623005304;    int QuOAOhMCwM59191951 = -529721736;    int QuOAOhMCwM36872528 = -97482200;    int QuOAOhMCwM4302229 = -470016949;    int QuOAOhMCwM52827088 = -246757196;    int QuOAOhMCwM14468392 = -474794895;    int QuOAOhMCwM94506911 = -922565965;    int QuOAOhMCwM49086398 = -513187208;    int QuOAOhMCwM81795133 = -446355192;    int QuOAOhMCwM67385919 = -19338639;    int QuOAOhMCwM48852020 = -525952900;    int QuOAOhMCwM43372771 = -582303262;    int QuOAOhMCwM90623769 = -983903984;    int QuOAOhMCwM161899 = -780241458;    int QuOAOhMCwM42732973 = 49684403;    int QuOAOhMCwM4175261 = -24262997;    int QuOAOhMCwM50586700 = -311940204;    int QuOAOhMCwM68028961 = -808366223;    int QuOAOhMCwM52234896 = -363023624;    int QuOAOhMCwM54507837 = -923541110;    int QuOAOhMCwM40937108 = 32795762;    int QuOAOhMCwM94536610 = -114144563;    int QuOAOhMCwM75103677 = -535103389;    int QuOAOhMCwM87763384 = -620993269;    int QuOAOhMCwM76560799 = -179755042;    int QuOAOhMCwM27525090 = -857520455;    int QuOAOhMCwM49071645 = -971858274;    int QuOAOhMCwM48371255 = -246932354;    int QuOAOhMCwM92075582 = -56594650;    int QuOAOhMCwM42167957 = -992579441;    int QuOAOhMCwM36534199 = -76436321;    int QuOAOhMCwM42759171 = -265095141;    int QuOAOhMCwM8847597 = -472222992;    int QuOAOhMCwM35727394 = -850166735;    int QuOAOhMCwM9830708 = -191597519;    int QuOAOhMCwM26126762 = -41640679;    int QuOAOhMCwM20165860 = 41109400;    int QuOAOhMCwM37884895 = -610550627;    int QuOAOhMCwM62435132 = -217757611;    int QuOAOhMCwM65933759 = -626282540;    int QuOAOhMCwM49984449 = -814524004;    int QuOAOhMCwM6993795 = -252587208;    int QuOAOhMCwM83677127 = -959723329;    int QuOAOhMCwM28987835 = -741697841;    int QuOAOhMCwM40117514 = -859410734;    int QuOAOhMCwM68797419 = -955273368;    int QuOAOhMCwM97648945 = -234472151;    int QuOAOhMCwM72378263 = -8180418;    int QuOAOhMCwM64786489 = -541219935;    int QuOAOhMCwM52486020 = -869829854;    int QuOAOhMCwM60447723 = -545723083;    int QuOAOhMCwM12633287 = -641821009;    int QuOAOhMCwM30556845 = -52426920;    int QuOAOhMCwM54854735 = 3038370;    int QuOAOhMCwM23094347 = -558524458;    int QuOAOhMCwM53954674 = -408860742;    int QuOAOhMCwM84088273 = -994618348;    int QuOAOhMCwM49109144 = -476488931;    int QuOAOhMCwM27741429 = -190261907;    int QuOAOhMCwM25301999 = -389236741;    int QuOAOhMCwM65396746 = -502936621;    int QuOAOhMCwM46135657 = -575633611;    int QuOAOhMCwM57010816 = -356592559;    int QuOAOhMCwM39627177 = -453775751;    int QuOAOhMCwM30851720 = -942902319;    int QuOAOhMCwM6092850 = -160857760;    int QuOAOhMCwM34525175 = -10080271;    int QuOAOhMCwM54896376 = -33737250;    int QuOAOhMCwM90331190 = -488643939;    int QuOAOhMCwM16606212 = -75896590;     QuOAOhMCwM15797804 = QuOAOhMCwM21611808;     QuOAOhMCwM21611808 = QuOAOhMCwM55890718;     QuOAOhMCwM55890718 = QuOAOhMCwM48736863;     QuOAOhMCwM48736863 = QuOAOhMCwM53202379;     QuOAOhMCwM53202379 = QuOAOhMCwM17196447;     QuOAOhMCwM17196447 = QuOAOhMCwM25550911;     QuOAOhMCwM25550911 = QuOAOhMCwM18210169;     QuOAOhMCwM18210169 = QuOAOhMCwM99758847;     QuOAOhMCwM99758847 = QuOAOhMCwM58321992;     QuOAOhMCwM58321992 = QuOAOhMCwM85318712;     QuOAOhMCwM85318712 = QuOAOhMCwM57038388;     QuOAOhMCwM57038388 = QuOAOhMCwM42187124;     QuOAOhMCwM42187124 = QuOAOhMCwM15262221;     QuOAOhMCwM15262221 = QuOAOhMCwM80402151;     QuOAOhMCwM80402151 = QuOAOhMCwM44491361;     QuOAOhMCwM44491361 = QuOAOhMCwM56080193;     QuOAOhMCwM56080193 = QuOAOhMCwM65472260;     QuOAOhMCwM65472260 = QuOAOhMCwM96373753;     QuOAOhMCwM96373753 = QuOAOhMCwM88969534;     QuOAOhMCwM88969534 = QuOAOhMCwM12170191;     QuOAOhMCwM12170191 = QuOAOhMCwM88272714;     QuOAOhMCwM88272714 = QuOAOhMCwM72540162;     QuOAOhMCwM72540162 = QuOAOhMCwM7519463;     QuOAOhMCwM7519463 = QuOAOhMCwM56661280;     QuOAOhMCwM56661280 = QuOAOhMCwM11034423;     QuOAOhMCwM11034423 = QuOAOhMCwM80662248;     QuOAOhMCwM80662248 = QuOAOhMCwM82791740;     QuOAOhMCwM82791740 = QuOAOhMCwM9362573;     QuOAOhMCwM9362573 = QuOAOhMCwM64031454;     QuOAOhMCwM64031454 = QuOAOhMCwM48491285;     QuOAOhMCwM48491285 = QuOAOhMCwM59191951;     QuOAOhMCwM59191951 = QuOAOhMCwM36872528;     QuOAOhMCwM36872528 = QuOAOhMCwM4302229;     QuOAOhMCwM4302229 = QuOAOhMCwM52827088;     QuOAOhMCwM52827088 = QuOAOhMCwM14468392;     QuOAOhMCwM14468392 = QuOAOhMCwM94506911;     QuOAOhMCwM94506911 = QuOAOhMCwM49086398;     QuOAOhMCwM49086398 = QuOAOhMCwM81795133;     QuOAOhMCwM81795133 = QuOAOhMCwM67385919;     QuOAOhMCwM67385919 = QuOAOhMCwM48852020;     QuOAOhMCwM48852020 = QuOAOhMCwM43372771;     QuOAOhMCwM43372771 = QuOAOhMCwM90623769;     QuOAOhMCwM90623769 = QuOAOhMCwM161899;     QuOAOhMCwM161899 = QuOAOhMCwM42732973;     QuOAOhMCwM42732973 = QuOAOhMCwM4175261;     QuOAOhMCwM4175261 = QuOAOhMCwM50586700;     QuOAOhMCwM50586700 = QuOAOhMCwM68028961;     QuOAOhMCwM68028961 = QuOAOhMCwM52234896;     QuOAOhMCwM52234896 = QuOAOhMCwM54507837;     QuOAOhMCwM54507837 = QuOAOhMCwM40937108;     QuOAOhMCwM40937108 = QuOAOhMCwM94536610;     QuOAOhMCwM94536610 = QuOAOhMCwM75103677;     QuOAOhMCwM75103677 = QuOAOhMCwM87763384;     QuOAOhMCwM87763384 = QuOAOhMCwM76560799;     QuOAOhMCwM76560799 = QuOAOhMCwM27525090;     QuOAOhMCwM27525090 = QuOAOhMCwM49071645;     QuOAOhMCwM49071645 = QuOAOhMCwM48371255;     QuOAOhMCwM48371255 = QuOAOhMCwM92075582;     QuOAOhMCwM92075582 = QuOAOhMCwM42167957;     QuOAOhMCwM42167957 = QuOAOhMCwM36534199;     QuOAOhMCwM36534199 = QuOAOhMCwM42759171;     QuOAOhMCwM42759171 = QuOAOhMCwM8847597;     QuOAOhMCwM8847597 = QuOAOhMCwM35727394;     QuOAOhMCwM35727394 = QuOAOhMCwM9830708;     QuOAOhMCwM9830708 = QuOAOhMCwM26126762;     QuOAOhMCwM26126762 = QuOAOhMCwM20165860;     QuOAOhMCwM20165860 = QuOAOhMCwM37884895;     QuOAOhMCwM37884895 = QuOAOhMCwM62435132;     QuOAOhMCwM62435132 = QuOAOhMCwM65933759;     QuOAOhMCwM65933759 = QuOAOhMCwM49984449;     QuOAOhMCwM49984449 = QuOAOhMCwM6993795;     QuOAOhMCwM6993795 = QuOAOhMCwM83677127;     QuOAOhMCwM83677127 = QuOAOhMCwM28987835;     QuOAOhMCwM28987835 = QuOAOhMCwM40117514;     QuOAOhMCwM40117514 = QuOAOhMCwM68797419;     QuOAOhMCwM68797419 = QuOAOhMCwM97648945;     QuOAOhMCwM97648945 = QuOAOhMCwM72378263;     QuOAOhMCwM72378263 = QuOAOhMCwM64786489;     QuOAOhMCwM64786489 = QuOAOhMCwM52486020;     QuOAOhMCwM52486020 = QuOAOhMCwM60447723;     QuOAOhMCwM60447723 = QuOAOhMCwM12633287;     QuOAOhMCwM12633287 = QuOAOhMCwM30556845;     QuOAOhMCwM30556845 = QuOAOhMCwM54854735;     QuOAOhMCwM54854735 = QuOAOhMCwM23094347;     QuOAOhMCwM23094347 = QuOAOhMCwM53954674;     QuOAOhMCwM53954674 = QuOAOhMCwM84088273;     QuOAOhMCwM84088273 = QuOAOhMCwM49109144;     QuOAOhMCwM49109144 = QuOAOhMCwM27741429;     QuOAOhMCwM27741429 = QuOAOhMCwM25301999;     QuOAOhMCwM25301999 = QuOAOhMCwM65396746;     QuOAOhMCwM65396746 = QuOAOhMCwM46135657;     QuOAOhMCwM46135657 = QuOAOhMCwM57010816;     QuOAOhMCwM57010816 = QuOAOhMCwM39627177;     QuOAOhMCwM39627177 = QuOAOhMCwM30851720;     QuOAOhMCwM30851720 = QuOAOhMCwM6092850;     QuOAOhMCwM6092850 = QuOAOhMCwM34525175;     QuOAOhMCwM34525175 = QuOAOhMCwM54896376;     QuOAOhMCwM54896376 = QuOAOhMCwM90331190;     QuOAOhMCwM90331190 = QuOAOhMCwM16606212;     QuOAOhMCwM16606212 = QuOAOhMCwM15797804;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void pWwZlTAMAv25375965() {     int GWHBovLmdY38184921 = -810300553;    int GWHBovLmdY5959308 = -808823204;    int GWHBovLmdY84659996 = -78121729;    int GWHBovLmdY63743551 = -57095032;    int GWHBovLmdY51376634 = -441299728;    int GWHBovLmdY32079564 = -487231930;    int GWHBovLmdY17706650 = -702532990;    int GWHBovLmdY93844027 = -963322375;    int GWHBovLmdY96921979 = -360091468;    int GWHBovLmdY50085577 = -657344155;    int GWHBovLmdY22860573 = -667986816;    int GWHBovLmdY12941337 = -407118092;    int GWHBovLmdY9195304 = -699284895;    int GWHBovLmdY57876973 = 7776437;    int GWHBovLmdY17137428 = -828555147;    int GWHBovLmdY88747273 = -18757511;    int GWHBovLmdY5903545 = -43054236;    int GWHBovLmdY71448677 = -857821211;    int GWHBovLmdY37655385 = -996138605;    int GWHBovLmdY67485817 = -637998087;    int GWHBovLmdY13053298 = -529063939;    int GWHBovLmdY52087933 = -723142448;    int GWHBovLmdY75833485 = 35497532;    int GWHBovLmdY34019250 = -55528388;    int GWHBovLmdY92201526 = -615651976;    int GWHBovLmdY99337417 = -301557510;    int GWHBovLmdY27751668 = -207773725;    int GWHBovLmdY88703190 = -303904997;    int GWHBovLmdY16255133 = -264759859;    int GWHBovLmdY73188634 = -546024562;    int GWHBovLmdY92782090 = -704321456;    int GWHBovLmdY16733142 = -81793090;    int GWHBovLmdY17911199 = -626825695;    int GWHBovLmdY63690916 = -794143799;    int GWHBovLmdY29056453 = -310513619;    int GWHBovLmdY56161260 = -790190271;    int GWHBovLmdY87946623 = -655513916;    int GWHBovLmdY2712500 = -380812680;    int GWHBovLmdY30103265 = -847401223;    int GWHBovLmdY87571633 = -155562669;    int GWHBovLmdY77609391 = 70869434;    int GWHBovLmdY31881162 = -340211953;    int GWHBovLmdY96173961 = -871434568;    int GWHBovLmdY99606017 = -777605107;    int GWHBovLmdY10716240 = -859459074;    int GWHBovLmdY12275543 = -206492093;    int GWHBovLmdY62884097 = -890350457;    int GWHBovLmdY12627636 = -564901214;    int GWHBovLmdY46694801 = -312740322;    int GWHBovLmdY51591583 = 34438631;    int GWHBovLmdY49806325 = -322280515;    int GWHBovLmdY68387868 = -544963845;    int GWHBovLmdY53418081 = -207014619;    int GWHBovLmdY48049929 = -896674078;    int GWHBovLmdY12322667 = -320459157;    int GWHBovLmdY86096988 = 12841895;    int GWHBovLmdY30125823 = -744320737;    int GWHBovLmdY50640747 = 77406659;    int GWHBovLmdY71542024 = -441443057;    int GWHBovLmdY52039216 = -39742219;    int GWHBovLmdY4327896 = -179458206;    int GWHBovLmdY29003460 = -298627994;    int GWHBovLmdY77588894 = -598562517;    int GWHBovLmdY23733346 = -814066907;    int GWHBovLmdY57303487 = -953022700;    int GWHBovLmdY6127431 = -486193726;    int GWHBovLmdY95030137 = -780292398;    int GWHBovLmdY45504388 = -905141097;    int GWHBovLmdY28820520 = -681709945;    int GWHBovLmdY60976167 = 61635123;    int GWHBovLmdY800650 = -363243595;    int GWHBovLmdY3191046 = -662241556;    int GWHBovLmdY41345412 = 89580012;    int GWHBovLmdY50083751 = -740575937;    int GWHBovLmdY89876426 = -608867521;    int GWHBovLmdY81172135 = -88851986;    int GWHBovLmdY55913972 = -851707880;    int GWHBovLmdY76227467 = -186897361;    int GWHBovLmdY23303010 = -196069315;    int GWHBovLmdY79925984 = -309159884;    int GWHBovLmdY36453321 = -411207054;    int GWHBovLmdY15124033 = -642872511;    int GWHBovLmdY42008390 = -991164675;    int GWHBovLmdY64663550 = -199198490;    int GWHBovLmdY23382310 = -123744047;    int GWHBovLmdY24394223 = -59357612;    int GWHBovLmdY63315061 = -874778472;    int GWHBovLmdY69861269 = -730151617;    int GWHBovLmdY51368249 = -373684642;    int GWHBovLmdY42959465 = -223355514;    int GWHBovLmdY26035438 = 54130465;    int GWHBovLmdY37305877 = -632920576;    int GWHBovLmdY31170475 = -939369624;    int GWHBovLmdY78064048 = -707659005;    int GWHBovLmdY83243738 = -976104463;    int GWHBovLmdY48605932 = -630502573;    int GWHBovLmdY54292268 = -741649437;    int GWHBovLmdY72440616 = 42632339;    int GWHBovLmdY42302531 = -824582407;    int GWHBovLmdY4588810 = -810300553;     GWHBovLmdY38184921 = GWHBovLmdY5959308;     GWHBovLmdY5959308 = GWHBovLmdY84659996;     GWHBovLmdY84659996 = GWHBovLmdY63743551;     GWHBovLmdY63743551 = GWHBovLmdY51376634;     GWHBovLmdY51376634 = GWHBovLmdY32079564;     GWHBovLmdY32079564 = GWHBovLmdY17706650;     GWHBovLmdY17706650 = GWHBovLmdY93844027;     GWHBovLmdY93844027 = GWHBovLmdY96921979;     GWHBovLmdY96921979 = GWHBovLmdY50085577;     GWHBovLmdY50085577 = GWHBovLmdY22860573;     GWHBovLmdY22860573 = GWHBovLmdY12941337;     GWHBovLmdY12941337 = GWHBovLmdY9195304;     GWHBovLmdY9195304 = GWHBovLmdY57876973;     GWHBovLmdY57876973 = GWHBovLmdY17137428;     GWHBovLmdY17137428 = GWHBovLmdY88747273;     GWHBovLmdY88747273 = GWHBovLmdY5903545;     GWHBovLmdY5903545 = GWHBovLmdY71448677;     GWHBovLmdY71448677 = GWHBovLmdY37655385;     GWHBovLmdY37655385 = GWHBovLmdY67485817;     GWHBovLmdY67485817 = GWHBovLmdY13053298;     GWHBovLmdY13053298 = GWHBovLmdY52087933;     GWHBovLmdY52087933 = GWHBovLmdY75833485;     GWHBovLmdY75833485 = GWHBovLmdY34019250;     GWHBovLmdY34019250 = GWHBovLmdY92201526;     GWHBovLmdY92201526 = GWHBovLmdY99337417;     GWHBovLmdY99337417 = GWHBovLmdY27751668;     GWHBovLmdY27751668 = GWHBovLmdY88703190;     GWHBovLmdY88703190 = GWHBovLmdY16255133;     GWHBovLmdY16255133 = GWHBovLmdY73188634;     GWHBovLmdY73188634 = GWHBovLmdY92782090;     GWHBovLmdY92782090 = GWHBovLmdY16733142;     GWHBovLmdY16733142 = GWHBovLmdY17911199;     GWHBovLmdY17911199 = GWHBovLmdY63690916;     GWHBovLmdY63690916 = GWHBovLmdY29056453;     GWHBovLmdY29056453 = GWHBovLmdY56161260;     GWHBovLmdY56161260 = GWHBovLmdY87946623;     GWHBovLmdY87946623 = GWHBovLmdY2712500;     GWHBovLmdY2712500 = GWHBovLmdY30103265;     GWHBovLmdY30103265 = GWHBovLmdY87571633;     GWHBovLmdY87571633 = GWHBovLmdY77609391;     GWHBovLmdY77609391 = GWHBovLmdY31881162;     GWHBovLmdY31881162 = GWHBovLmdY96173961;     GWHBovLmdY96173961 = GWHBovLmdY99606017;     GWHBovLmdY99606017 = GWHBovLmdY10716240;     GWHBovLmdY10716240 = GWHBovLmdY12275543;     GWHBovLmdY12275543 = GWHBovLmdY62884097;     GWHBovLmdY62884097 = GWHBovLmdY12627636;     GWHBovLmdY12627636 = GWHBovLmdY46694801;     GWHBovLmdY46694801 = GWHBovLmdY51591583;     GWHBovLmdY51591583 = GWHBovLmdY49806325;     GWHBovLmdY49806325 = GWHBovLmdY68387868;     GWHBovLmdY68387868 = GWHBovLmdY53418081;     GWHBovLmdY53418081 = GWHBovLmdY48049929;     GWHBovLmdY48049929 = GWHBovLmdY12322667;     GWHBovLmdY12322667 = GWHBovLmdY86096988;     GWHBovLmdY86096988 = GWHBovLmdY30125823;     GWHBovLmdY30125823 = GWHBovLmdY50640747;     GWHBovLmdY50640747 = GWHBovLmdY71542024;     GWHBovLmdY71542024 = GWHBovLmdY52039216;     GWHBovLmdY52039216 = GWHBovLmdY4327896;     GWHBovLmdY4327896 = GWHBovLmdY29003460;     GWHBovLmdY29003460 = GWHBovLmdY77588894;     GWHBovLmdY77588894 = GWHBovLmdY23733346;     GWHBovLmdY23733346 = GWHBovLmdY57303487;     GWHBovLmdY57303487 = GWHBovLmdY6127431;     GWHBovLmdY6127431 = GWHBovLmdY95030137;     GWHBovLmdY95030137 = GWHBovLmdY45504388;     GWHBovLmdY45504388 = GWHBovLmdY28820520;     GWHBovLmdY28820520 = GWHBovLmdY60976167;     GWHBovLmdY60976167 = GWHBovLmdY800650;     GWHBovLmdY800650 = GWHBovLmdY3191046;     GWHBovLmdY3191046 = GWHBovLmdY41345412;     GWHBovLmdY41345412 = GWHBovLmdY50083751;     GWHBovLmdY50083751 = GWHBovLmdY89876426;     GWHBovLmdY89876426 = GWHBovLmdY81172135;     GWHBovLmdY81172135 = GWHBovLmdY55913972;     GWHBovLmdY55913972 = GWHBovLmdY76227467;     GWHBovLmdY76227467 = GWHBovLmdY23303010;     GWHBovLmdY23303010 = GWHBovLmdY79925984;     GWHBovLmdY79925984 = GWHBovLmdY36453321;     GWHBovLmdY36453321 = GWHBovLmdY15124033;     GWHBovLmdY15124033 = GWHBovLmdY42008390;     GWHBovLmdY42008390 = GWHBovLmdY64663550;     GWHBovLmdY64663550 = GWHBovLmdY23382310;     GWHBovLmdY23382310 = GWHBovLmdY24394223;     GWHBovLmdY24394223 = GWHBovLmdY63315061;     GWHBovLmdY63315061 = GWHBovLmdY69861269;     GWHBovLmdY69861269 = GWHBovLmdY51368249;     GWHBovLmdY51368249 = GWHBovLmdY42959465;     GWHBovLmdY42959465 = GWHBovLmdY26035438;     GWHBovLmdY26035438 = GWHBovLmdY37305877;     GWHBovLmdY37305877 = GWHBovLmdY31170475;     GWHBovLmdY31170475 = GWHBovLmdY78064048;     GWHBovLmdY78064048 = GWHBovLmdY83243738;     GWHBovLmdY83243738 = GWHBovLmdY48605932;     GWHBovLmdY48605932 = GWHBovLmdY54292268;     GWHBovLmdY54292268 = GWHBovLmdY72440616;     GWHBovLmdY72440616 = GWHBovLmdY42302531;     GWHBovLmdY42302531 = GWHBovLmdY4588810;     GWHBovLmdY4588810 = GWHBovLmdY38184921;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void WWbcELmegx62020939() {     int XOGHkmIbfc60572039 = -444704516;    int XOGHkmIbfc90306808 = -757366259;    int XOGHkmIbfc13429275 = -317775573;    int XOGHkmIbfc78750239 = -63502564;    int XOGHkmIbfc49550888 = 67643271;    int XOGHkmIbfc46962680 = -347840308;    int XOGHkmIbfc9862389 = -524520296;    int XOGHkmIbfc69477886 = -333919020;    int XOGHkmIbfc94085110 = -244287508;    int XOGHkmIbfc41849162 = -400085488;    int XOGHkmIbfc60402432 = -664611217;    int XOGHkmIbfc68844285 = -657863386;    int XOGHkmIbfc76203484 = -218002214;    int XOGHkmIbfc491726 = -519932320;    int XOGHkmIbfc53872705 = -456032861;    int XOGHkmIbfc33003186 = -400425053;    int XOGHkmIbfc55726896 = -320334056;    int XOGHkmIbfc77425093 = -209563901;    int XOGHkmIbfc78937015 = -31240730;    int XOGHkmIbfc46002101 = -890632541;    int XOGHkmIbfc13936405 = -520551247;    int XOGHkmIbfc15903152 = -127908761;    int XOGHkmIbfc79126808 = -140583060;    int XOGHkmIbfc60519037 = -619521246;    int XOGHkmIbfc27741774 = -237211100;    int XOGHkmIbfc87640412 = -745451734;    int XOGHkmIbfc74841088 = 34639782;    int XOGHkmIbfc94614640 = -92359450;    int XOGHkmIbfc23147694 = -609016978;    int XOGHkmIbfc82345813 = -466320428;    int XOGHkmIbfc37072895 = -785637608;    int XOGHkmIbfc74274333 = -733864445;    int XOGHkmIbfc98949869 = -56169190;    int XOGHkmIbfc23079603 = -18270649;    int XOGHkmIbfc5285818 = -374270041;    int XOGHkmIbfc97854129 = -5585648;    int XOGHkmIbfc81386336 = -388461868;    int XOGHkmIbfc56338600 = -248438152;    int XOGHkmIbfc78411396 = -148447255;    int XOGHkmIbfc7757348 = -291786698;    int XOGHkmIbfc6366762 = -432308232;    int XOGHkmIbfc20389554 = -98120644;    int XOGHkmIbfc1724154 = -758965152;    int XOGHkmIbfc99050136 = -774968756;    int XOGHkmIbfc78699506 = -668602552;    int XOGHkmIbfc20375824 = -388721188;    int XOGHkmIbfc75181494 = -368760709;    int XOGHkmIbfc57226310 = -321436205;    int XOGHkmIbfc41154706 = -262457021;    int XOGHkmIbfc48675329 = -107581628;    int XOGHkmIbfc58675541 = -677356792;    int XOGHkmIbfc42239125 = -975783127;    int XOGHkmIbfc31732484 = -978925849;    int XOGHkmIbfc8336475 = -72354887;    int XOGHkmIbfc48084534 = -461163272;    int XOGHkmIbfc44668887 = -216795755;    int XOGHkmIbfc11180000 = -516783200;    int XOGHkmIbfc52910238 = -698254328;    int XOGHkmIbfc51008466 = -826291464;    int XOGHkmIbfc61910476 = -186904996;    int XOGHkmIbfc72121591 = -282480091;    int XOGHkmIbfc15247748 = -332160846;    int XOGHkmIbfc46330192 = -724902042;    int XOGHkmIbfc11739297 = -777967080;    int XOGHkmIbfc4776267 = -614447881;    int XOGHkmIbfc86128099 = -930746772;    int XOGHkmIbfc69894416 = -501694196;    int XOGHkmIbfc53123882 = -99731566;    int XOGHkmIbfc95205907 = -45662279;    int XOGHkmIbfc56018575 = -350447213;    int XOGHkmIbfc51616849 = 88036814;    int XOGHkmIbfc99388296 = 28104096;    int XOGHkmIbfc99013697 = 38883353;    int XOGHkmIbfc71179667 = -739454033;    int XOGHkmIbfc39635339 = -358324309;    int XOGHkmIbfc93546851 = -322430603;    int XOGHkmIbfc14178998 = -368943610;    int XOGHkmIbfc80076671 = -365614304;    int XOGHkmIbfc81819530 = -950918694;    int XOGHkmIbfc7365950 = -848489913;    int XOGHkmIbfc12458918 = -276691025;    int XOGHkmIbfc17614779 = -643924013;    int XOGHkmIbfc53459935 = -829902430;    int XOGHkmIbfc74472365 = -401435351;    int XOGHkmIbfc23670273 = -788963637;    int XOGHkmIbfc94833770 = -809854482;    int XOGHkmIbfc42541849 = -754938597;    int XOGHkmIbfc90613394 = -983814303;    int XOGHkmIbfc74995069 = -557107377;    int XOGHkmIbfc60616930 = -57474287;    int XOGHkmIbfc86674129 = -488802449;    int XOGHkmIbfc28476098 = -690207541;    int XOGHkmIbfc5330135 = -422146689;    int XOGHkmIbfc16500920 = -961542260;    int XOGHkmIbfc35635756 = 90693392;    int XOGHkmIbfc91119013 = -147387;    int XOGHkmIbfc74059361 = -373218603;    int XOGHkmIbfc89984856 = -980998073;    int XOGHkmIbfc94273870 = -60520875;    int XOGHkmIbfc92571407 = -444704516;     XOGHkmIbfc60572039 = XOGHkmIbfc90306808;     XOGHkmIbfc90306808 = XOGHkmIbfc13429275;     XOGHkmIbfc13429275 = XOGHkmIbfc78750239;     XOGHkmIbfc78750239 = XOGHkmIbfc49550888;     XOGHkmIbfc49550888 = XOGHkmIbfc46962680;     XOGHkmIbfc46962680 = XOGHkmIbfc9862389;     XOGHkmIbfc9862389 = XOGHkmIbfc69477886;     XOGHkmIbfc69477886 = XOGHkmIbfc94085110;     XOGHkmIbfc94085110 = XOGHkmIbfc41849162;     XOGHkmIbfc41849162 = XOGHkmIbfc60402432;     XOGHkmIbfc60402432 = XOGHkmIbfc68844285;     XOGHkmIbfc68844285 = XOGHkmIbfc76203484;     XOGHkmIbfc76203484 = XOGHkmIbfc491726;     XOGHkmIbfc491726 = XOGHkmIbfc53872705;     XOGHkmIbfc53872705 = XOGHkmIbfc33003186;     XOGHkmIbfc33003186 = XOGHkmIbfc55726896;     XOGHkmIbfc55726896 = XOGHkmIbfc77425093;     XOGHkmIbfc77425093 = XOGHkmIbfc78937015;     XOGHkmIbfc78937015 = XOGHkmIbfc46002101;     XOGHkmIbfc46002101 = XOGHkmIbfc13936405;     XOGHkmIbfc13936405 = XOGHkmIbfc15903152;     XOGHkmIbfc15903152 = XOGHkmIbfc79126808;     XOGHkmIbfc79126808 = XOGHkmIbfc60519037;     XOGHkmIbfc60519037 = XOGHkmIbfc27741774;     XOGHkmIbfc27741774 = XOGHkmIbfc87640412;     XOGHkmIbfc87640412 = XOGHkmIbfc74841088;     XOGHkmIbfc74841088 = XOGHkmIbfc94614640;     XOGHkmIbfc94614640 = XOGHkmIbfc23147694;     XOGHkmIbfc23147694 = XOGHkmIbfc82345813;     XOGHkmIbfc82345813 = XOGHkmIbfc37072895;     XOGHkmIbfc37072895 = XOGHkmIbfc74274333;     XOGHkmIbfc74274333 = XOGHkmIbfc98949869;     XOGHkmIbfc98949869 = XOGHkmIbfc23079603;     XOGHkmIbfc23079603 = XOGHkmIbfc5285818;     XOGHkmIbfc5285818 = XOGHkmIbfc97854129;     XOGHkmIbfc97854129 = XOGHkmIbfc81386336;     XOGHkmIbfc81386336 = XOGHkmIbfc56338600;     XOGHkmIbfc56338600 = XOGHkmIbfc78411396;     XOGHkmIbfc78411396 = XOGHkmIbfc7757348;     XOGHkmIbfc7757348 = XOGHkmIbfc6366762;     XOGHkmIbfc6366762 = XOGHkmIbfc20389554;     XOGHkmIbfc20389554 = XOGHkmIbfc1724154;     XOGHkmIbfc1724154 = XOGHkmIbfc99050136;     XOGHkmIbfc99050136 = XOGHkmIbfc78699506;     XOGHkmIbfc78699506 = XOGHkmIbfc20375824;     XOGHkmIbfc20375824 = XOGHkmIbfc75181494;     XOGHkmIbfc75181494 = XOGHkmIbfc57226310;     XOGHkmIbfc57226310 = XOGHkmIbfc41154706;     XOGHkmIbfc41154706 = XOGHkmIbfc48675329;     XOGHkmIbfc48675329 = XOGHkmIbfc58675541;     XOGHkmIbfc58675541 = XOGHkmIbfc42239125;     XOGHkmIbfc42239125 = XOGHkmIbfc31732484;     XOGHkmIbfc31732484 = XOGHkmIbfc8336475;     XOGHkmIbfc8336475 = XOGHkmIbfc48084534;     XOGHkmIbfc48084534 = XOGHkmIbfc44668887;     XOGHkmIbfc44668887 = XOGHkmIbfc11180000;     XOGHkmIbfc11180000 = XOGHkmIbfc52910238;     XOGHkmIbfc52910238 = XOGHkmIbfc51008466;     XOGHkmIbfc51008466 = XOGHkmIbfc61910476;     XOGHkmIbfc61910476 = XOGHkmIbfc72121591;     XOGHkmIbfc72121591 = XOGHkmIbfc15247748;     XOGHkmIbfc15247748 = XOGHkmIbfc46330192;     XOGHkmIbfc46330192 = XOGHkmIbfc11739297;     XOGHkmIbfc11739297 = XOGHkmIbfc4776267;     XOGHkmIbfc4776267 = XOGHkmIbfc86128099;     XOGHkmIbfc86128099 = XOGHkmIbfc69894416;     XOGHkmIbfc69894416 = XOGHkmIbfc53123882;     XOGHkmIbfc53123882 = XOGHkmIbfc95205907;     XOGHkmIbfc95205907 = XOGHkmIbfc56018575;     XOGHkmIbfc56018575 = XOGHkmIbfc51616849;     XOGHkmIbfc51616849 = XOGHkmIbfc99388296;     XOGHkmIbfc99388296 = XOGHkmIbfc99013697;     XOGHkmIbfc99013697 = XOGHkmIbfc71179667;     XOGHkmIbfc71179667 = XOGHkmIbfc39635339;     XOGHkmIbfc39635339 = XOGHkmIbfc93546851;     XOGHkmIbfc93546851 = XOGHkmIbfc14178998;     XOGHkmIbfc14178998 = XOGHkmIbfc80076671;     XOGHkmIbfc80076671 = XOGHkmIbfc81819530;     XOGHkmIbfc81819530 = XOGHkmIbfc7365950;     XOGHkmIbfc7365950 = XOGHkmIbfc12458918;     XOGHkmIbfc12458918 = XOGHkmIbfc17614779;     XOGHkmIbfc17614779 = XOGHkmIbfc53459935;     XOGHkmIbfc53459935 = XOGHkmIbfc74472365;     XOGHkmIbfc74472365 = XOGHkmIbfc23670273;     XOGHkmIbfc23670273 = XOGHkmIbfc94833770;     XOGHkmIbfc94833770 = XOGHkmIbfc42541849;     XOGHkmIbfc42541849 = XOGHkmIbfc90613394;     XOGHkmIbfc90613394 = XOGHkmIbfc74995069;     XOGHkmIbfc74995069 = XOGHkmIbfc60616930;     XOGHkmIbfc60616930 = XOGHkmIbfc86674129;     XOGHkmIbfc86674129 = XOGHkmIbfc28476098;     XOGHkmIbfc28476098 = XOGHkmIbfc5330135;     XOGHkmIbfc5330135 = XOGHkmIbfc16500920;     XOGHkmIbfc16500920 = XOGHkmIbfc35635756;     XOGHkmIbfc35635756 = XOGHkmIbfc91119013;     XOGHkmIbfc91119013 = XOGHkmIbfc74059361;     XOGHkmIbfc74059361 = XOGHkmIbfc89984856;     XOGHkmIbfc89984856 = XOGHkmIbfc94273870;     XOGHkmIbfc94273870 = XOGHkmIbfc92571407;     XOGHkmIbfc92571407 = XOGHkmIbfc60572039;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void QRBqxSmGbc98665913() {     int VjVhhyjTjL82959156 = -79108479;    int VjVhhyjTjL74654308 = -705909314;    int VjVhhyjTjL42198554 = -557429417;    int VjVhhyjTjL93756927 = -69910095;    int VjVhhyjTjL47725142 = -523413731;    int VjVhhyjTjL61845797 = -208448686;    int VjVhhyjTjL2018128 = -346507602;    int VjVhhyjTjL45111745 = -804515664;    int VjVhhyjTjL91248241 = -128483547;    int VjVhhyjTjL33612747 = -142826821;    int VjVhhyjTjL97944291 = -661235617;    int VjVhhyjTjL24747235 = -908608679;    int VjVhhyjTjL43211665 = -836719534;    int VjVhhyjTjL43106478 = 52358923;    int VjVhhyjTjL90607982 = -83510574;    int VjVhhyjTjL77259098 = -782092596;    int VjVhhyjTjL5550248 = -597613876;    int VjVhhyjTjL83401509 = -661306592;    int VjVhhyjTjL20218647 = -166342855;    int VjVhhyjTjL24518384 = -43266994;    int VjVhhyjTjL14819512 = -512038555;    int VjVhhyjTjL79718369 = -632675075;    int VjVhhyjTjL82420131 = -316663652;    int VjVhhyjTjL87018824 = -83514103;    int VjVhhyjTjL63282020 = -958770225;    int VjVhhyjTjL75943406 = -89345958;    int VjVhhyjTjL21930509 = -822946711;    int VjVhhyjTjL526092 = -980813904;    int VjVhhyjTjL30040255 = -953274097;    int VjVhhyjTjL91502992 = -386616295;    int VjVhhyjTjL81363700 = -866953760;    int VjVhhyjTjL31815524 = -285935800;    int VjVhhyjTjL79988539 = -585512684;    int VjVhhyjTjL82468290 = -342397499;    int VjVhhyjTjL81515182 = -438026464;    int VjVhhyjTjL39546999 = -320981024;    int VjVhhyjTjL74826048 = -121409819;    int VjVhhyjTjL9964701 = -116063624;    int VjVhhyjTjL26719527 = -549493287;    int VjVhhyjTjL27943063 = -428010727;    int VjVhhyjTjL35124133 = -935485898;    int VjVhhyjTjL8897945 = -956029335;    int VjVhhyjTjL7274346 = -646495736;    int VjVhhyjTjL98494256 = -772332405;    int VjVhhyjTjL46682773 = -477746030;    int VjVhhyjTjL28476106 = -570950283;    int VjVhhyjTjL87478891 = -947170962;    int VjVhhyjTjL1824986 = -77971196;    int VjVhhyjTjL35614611 = -212173719;    int VjVhhyjTjL45759074 = -249601887;    int VjVhhyjTjL67544757 = 67566931;    int VjVhhyjTjL16090382 = -306602409;    int VjVhhyjTjL10046888 = -650837079;    int VjVhhyjTjL68623019 = -348035696;    int VjVhhyjTjL83846401 = -601867388;    int VjVhhyjTjL3240787 = -446433405;    int VjVhhyjTjL92234177 = -289245662;    int VjVhhyjTjL55179730 = -373915314;    int VjVhhyjTjL30474908 = -111139871;    int VjVhhyjTjL71781735 = -334067773;    int VjVhhyjTjL39915288 = -385501976;    int VjVhhyjTjL1492037 = -365693699;    int VjVhhyjTjL15071491 = -851241567;    int VjVhhyjTjL99745248 = -741867252;    int VjVhhyjTjL52249046 = -275873062;    int VjVhhyjTjL66128768 = -275299818;    int VjVhhyjTjL44758695 = -223095995;    int VjVhhyjTjL60743375 = -394322036;    int VjVhhyjTjL61591295 = -509614614;    int VjVhhyjTjL51060983 = -762529550;    int VjVhhyjTjL2433050 = -560682777;    int VjVhhyjTjL95585547 = -381550252;    int VjVhhyjTjL56681982 = -11813306;    int VjVhhyjTjL92275583 = -738332129;    int VjVhhyjTjL89394251 = -107781096;    int VjVhhyjTjL5921568 = -556009221;    int VjVhhyjTjL72444024 = -986179340;    int VjVhhyjTjL83925875 = -544331248;    int VjVhhyjTjL40336051 = -605768074;    int VjVhhyjTjL34805915 = -287819942;    int VjVhhyjTjL88464514 = -142174996;    int VjVhhyjTjL20105524 = -644975515;    int VjVhhyjTjL64911480 = -668640185;    int VjVhhyjTjL84281180 = -603672211;    int VjVhhyjTjL23958236 = -354183226;    int VjVhhyjTjL65273318 = -460351352;    int VjVhhyjTjL21768637 = -635098721;    int VjVhhyjTjL11365520 = -137476989;    int VjVhhyjTjL98621888 = -740530112;    int VjVhhyjTjL78274396 = -991593059;    int VjVhhyjTjL47312822 = 68264637;    int VjVhhyjTjL19646319 = -747494505;    int VjVhhyjTjL79489793 = 95076246;    int VjVhhyjTjL54937792 = -115425514;    int VjVhhyjTjL88027774 = 57491248;    int VjVhhyjTjL33632096 = -469792200;    int VjVhhyjTjL93826454 = -4787769;    int VjVhhyjTjL7529097 = -904628484;    int VjVhhyjTjL46245210 = -396459344;    int VjVhhyjTjL80554005 = -79108479;     VjVhhyjTjL82959156 = VjVhhyjTjL74654308;     VjVhhyjTjL74654308 = VjVhhyjTjL42198554;     VjVhhyjTjL42198554 = VjVhhyjTjL93756927;     VjVhhyjTjL93756927 = VjVhhyjTjL47725142;     VjVhhyjTjL47725142 = VjVhhyjTjL61845797;     VjVhhyjTjL61845797 = VjVhhyjTjL2018128;     VjVhhyjTjL2018128 = VjVhhyjTjL45111745;     VjVhhyjTjL45111745 = VjVhhyjTjL91248241;     VjVhhyjTjL91248241 = VjVhhyjTjL33612747;     VjVhhyjTjL33612747 = VjVhhyjTjL97944291;     VjVhhyjTjL97944291 = VjVhhyjTjL24747235;     VjVhhyjTjL24747235 = VjVhhyjTjL43211665;     VjVhhyjTjL43211665 = VjVhhyjTjL43106478;     VjVhhyjTjL43106478 = VjVhhyjTjL90607982;     VjVhhyjTjL90607982 = VjVhhyjTjL77259098;     VjVhhyjTjL77259098 = VjVhhyjTjL5550248;     VjVhhyjTjL5550248 = VjVhhyjTjL83401509;     VjVhhyjTjL83401509 = VjVhhyjTjL20218647;     VjVhhyjTjL20218647 = VjVhhyjTjL24518384;     VjVhhyjTjL24518384 = VjVhhyjTjL14819512;     VjVhhyjTjL14819512 = VjVhhyjTjL79718369;     VjVhhyjTjL79718369 = VjVhhyjTjL82420131;     VjVhhyjTjL82420131 = VjVhhyjTjL87018824;     VjVhhyjTjL87018824 = VjVhhyjTjL63282020;     VjVhhyjTjL63282020 = VjVhhyjTjL75943406;     VjVhhyjTjL75943406 = VjVhhyjTjL21930509;     VjVhhyjTjL21930509 = VjVhhyjTjL526092;     VjVhhyjTjL526092 = VjVhhyjTjL30040255;     VjVhhyjTjL30040255 = VjVhhyjTjL91502992;     VjVhhyjTjL91502992 = VjVhhyjTjL81363700;     VjVhhyjTjL81363700 = VjVhhyjTjL31815524;     VjVhhyjTjL31815524 = VjVhhyjTjL79988539;     VjVhhyjTjL79988539 = VjVhhyjTjL82468290;     VjVhhyjTjL82468290 = VjVhhyjTjL81515182;     VjVhhyjTjL81515182 = VjVhhyjTjL39546999;     VjVhhyjTjL39546999 = VjVhhyjTjL74826048;     VjVhhyjTjL74826048 = VjVhhyjTjL9964701;     VjVhhyjTjL9964701 = VjVhhyjTjL26719527;     VjVhhyjTjL26719527 = VjVhhyjTjL27943063;     VjVhhyjTjL27943063 = VjVhhyjTjL35124133;     VjVhhyjTjL35124133 = VjVhhyjTjL8897945;     VjVhhyjTjL8897945 = VjVhhyjTjL7274346;     VjVhhyjTjL7274346 = VjVhhyjTjL98494256;     VjVhhyjTjL98494256 = VjVhhyjTjL46682773;     VjVhhyjTjL46682773 = VjVhhyjTjL28476106;     VjVhhyjTjL28476106 = VjVhhyjTjL87478891;     VjVhhyjTjL87478891 = VjVhhyjTjL1824986;     VjVhhyjTjL1824986 = VjVhhyjTjL35614611;     VjVhhyjTjL35614611 = VjVhhyjTjL45759074;     VjVhhyjTjL45759074 = VjVhhyjTjL67544757;     VjVhhyjTjL67544757 = VjVhhyjTjL16090382;     VjVhhyjTjL16090382 = VjVhhyjTjL10046888;     VjVhhyjTjL10046888 = VjVhhyjTjL68623019;     VjVhhyjTjL68623019 = VjVhhyjTjL83846401;     VjVhhyjTjL83846401 = VjVhhyjTjL3240787;     VjVhhyjTjL3240787 = VjVhhyjTjL92234177;     VjVhhyjTjL92234177 = VjVhhyjTjL55179730;     VjVhhyjTjL55179730 = VjVhhyjTjL30474908;     VjVhhyjTjL30474908 = VjVhhyjTjL71781735;     VjVhhyjTjL71781735 = VjVhhyjTjL39915288;     VjVhhyjTjL39915288 = VjVhhyjTjL1492037;     VjVhhyjTjL1492037 = VjVhhyjTjL15071491;     VjVhhyjTjL15071491 = VjVhhyjTjL99745248;     VjVhhyjTjL99745248 = VjVhhyjTjL52249046;     VjVhhyjTjL52249046 = VjVhhyjTjL66128768;     VjVhhyjTjL66128768 = VjVhhyjTjL44758695;     VjVhhyjTjL44758695 = VjVhhyjTjL60743375;     VjVhhyjTjL60743375 = VjVhhyjTjL61591295;     VjVhhyjTjL61591295 = VjVhhyjTjL51060983;     VjVhhyjTjL51060983 = VjVhhyjTjL2433050;     VjVhhyjTjL2433050 = VjVhhyjTjL95585547;     VjVhhyjTjL95585547 = VjVhhyjTjL56681982;     VjVhhyjTjL56681982 = VjVhhyjTjL92275583;     VjVhhyjTjL92275583 = VjVhhyjTjL89394251;     VjVhhyjTjL89394251 = VjVhhyjTjL5921568;     VjVhhyjTjL5921568 = VjVhhyjTjL72444024;     VjVhhyjTjL72444024 = VjVhhyjTjL83925875;     VjVhhyjTjL83925875 = VjVhhyjTjL40336051;     VjVhhyjTjL40336051 = VjVhhyjTjL34805915;     VjVhhyjTjL34805915 = VjVhhyjTjL88464514;     VjVhhyjTjL88464514 = VjVhhyjTjL20105524;     VjVhhyjTjL20105524 = VjVhhyjTjL64911480;     VjVhhyjTjL64911480 = VjVhhyjTjL84281180;     VjVhhyjTjL84281180 = VjVhhyjTjL23958236;     VjVhhyjTjL23958236 = VjVhhyjTjL65273318;     VjVhhyjTjL65273318 = VjVhhyjTjL21768637;     VjVhhyjTjL21768637 = VjVhhyjTjL11365520;     VjVhhyjTjL11365520 = VjVhhyjTjL98621888;     VjVhhyjTjL98621888 = VjVhhyjTjL78274396;     VjVhhyjTjL78274396 = VjVhhyjTjL47312822;     VjVhhyjTjL47312822 = VjVhhyjTjL19646319;     VjVhhyjTjL19646319 = VjVhhyjTjL79489793;     VjVhhyjTjL79489793 = VjVhhyjTjL54937792;     VjVhhyjTjL54937792 = VjVhhyjTjL88027774;     VjVhhyjTjL88027774 = VjVhhyjTjL33632096;     VjVhhyjTjL33632096 = VjVhhyjTjL93826454;     VjVhhyjTjL93826454 = VjVhhyjTjL7529097;     VjVhhyjTjL7529097 = VjVhhyjTjL46245210;     VjVhhyjTjL46245210 = VjVhhyjTjL80554005;     VjVhhyjTjL80554005 = VjVhhyjTjL82959156;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BAcazlJrXF15462218() {     int jJVsesNITZ50409428 = -292890542;    int jJVsesNITZ564342 = -291915454;    int jJVsesNITZ68714182 = -865480491;    int jJVsesNITZ95512903 = -719403538;    int jJVsesNITZ2427764 = 14867547;    int jJVsesNITZ67122116 = -565373618;    int jJVsesNITZ91505586 = 12732356;    int jJVsesNITZ97184885 = 6663823;    int jJVsesNITZ50317216 = -16625905;    int jJVsesNITZ27504166 = -220505417;    int jJVsesNITZ73562061 = -362070097;    int jJVsesNITZ98024937 = -899148040;    int jJVsesNITZ29392912 = 38445216;    int jJVsesNITZ54930657 = -474430276;    int jJVsesNITZ76380445 = 46451242;    int jJVsesNITZ33604882 = 86049927;    int jJVsesNITZ77354690 = 94433684;    int jJVsesNITZ50055782 = -234473929;    int jJVsesNITZ41957473 = -534650199;    int jJVsesNITZ36696791 = -69840603;    int jJVsesNITZ9663821 = -905738410;    int jJVsesNITZ92574379 = -753989706;    int jJVsesNITZ2332664 = -695974205;    int jJVsesNITZ67479160 = -321519148;    int jJVsesNITZ72264861 = -352084821;    int jJVsesNITZ23641884 = 78822951;    int jJVsesNITZ80003597 = -460297700;    int jJVsesNITZ68022729 = -249213387;    int jJVsesNITZ67537314 = -28630740;    int jJVsesNITZ74554712 = -794113357;    int jJVsesNITZ27386207 = -11603184;    int jJVsesNITZ89040171 = -88419364;    int jJVsesNITZ65511730 = -164968759;    int jJVsesNITZ53776300 = -662249928;    int jJVsesNITZ75542814 = -876950716;    int jJVsesNITZ57855371 = -787891106;    int jJVsesNITZ18382545 = -162265723;    int jJVsesNITZ75769443 = 83281282;    int jJVsesNITZ30294167 = -254270341;    int jJVsesNITZ20384848 = 33411357;    int jJVsesNITZ70731340 = -741594202;    int jJVsesNITZ74728567 = -71184633;    int jJVsesNITZ92732958 = -771739117;    int jJVsesNITZ13269346 = -151737648;    int jJVsesNITZ60648248 = -343144485;    int jJVsesNITZ96795884 = -444991331;    int jJVsesNITZ39987934 = -749822465;    int jJVsesNITZ91616889 = -947834313;    int jJVsesNITZ44033843 = -379860091;    int jJVsesNITZ7483023 = -620120315;    int jJVsesNITZ41096736 = -474700368;    int jJVsesNITZ11828193 = -169635107;    int jJVsesNITZ13087518 = -178674807;    int jJVsesNITZ55363096 = -685494145;    int jJVsesNITZ5798397 = -256871639;    int jJVsesNITZ57835049 = -538900837;    int jJVsesNITZ98231677 = -595941249;    int jJVsesNITZ1235022 = -443961344;    int jJVsesNITZ23248043 = -267318717;    int jJVsesNITZ78785880 = 36044596;    int jJVsesNITZ87118519 = -5075918;    int jJVsesNITZ23482857 = -738054258;    int jJVsesNITZ29647571 = -964705438;    int jJVsesNITZ75762504 = -222512548;    int jJVsesNITZ117960 = -108902234;    int jJVsesNITZ84521889 = -173650733;    int jJVsesNITZ32513207 = -634179281;    int jJVsesNITZ75616612 = -299304856;    int jJVsesNITZ79387843 = -597479560;    int jJVsesNITZ18525074 = -165657653;    int jJVsesNITZ15222337 = -751684351;    int jJVsesNITZ1585247 = -988847599;    int jJVsesNITZ19761615 = -980203588;    int jJVsesNITZ21572625 = -468061556;    int jJVsesNITZ65965451 = -328246401;    int jJVsesNITZ34935253 = -734553778;    int jJVsesNITZ99841421 = -982250589;    int jJVsesNITZ89063317 = -444236557;    int jJVsesNITZ6830912 = -978374663;    int jJVsesNITZ75468976 = -907093490;    int jJVsesNITZ83653949 = -171354584;    int jJVsesNITZ88386708 = -512463388;    int jJVsesNITZ23988886 = -869353296;    int jJVsesNITZ60054291 = -408510426;    int jJVsesNITZ33457977 = -219412989;    int jJVsesNITZ15558014 = -841968077;    int jJVsesNITZ75952653 = -909744558;    int jJVsesNITZ10148635 = -479474614;    int jJVsesNITZ47977903 = -305378290;    int jJVsesNITZ17707766 = -238049880;    int jJVsesNITZ59623694 = -91949857;    int jJVsesNITZ17147523 = -718304379;    int jJVsesNITZ52521401 = -649400002;    int jJVsesNITZ51508287 = -190314938;    int jJVsesNITZ33266329 = -961512726;    int jJVsesNITZ47248483 = 96460056;    int jJVsesNITZ45080997 = -106479196;    int jJVsesNITZ16970454 = -449226570;    int jJVsesNITZ13151387 = 57164585;    int jJVsesNITZ76126359 = -292890542;     jJVsesNITZ50409428 = jJVsesNITZ564342;     jJVsesNITZ564342 = jJVsesNITZ68714182;     jJVsesNITZ68714182 = jJVsesNITZ95512903;     jJVsesNITZ95512903 = jJVsesNITZ2427764;     jJVsesNITZ2427764 = jJVsesNITZ67122116;     jJVsesNITZ67122116 = jJVsesNITZ91505586;     jJVsesNITZ91505586 = jJVsesNITZ97184885;     jJVsesNITZ97184885 = jJVsesNITZ50317216;     jJVsesNITZ50317216 = jJVsesNITZ27504166;     jJVsesNITZ27504166 = jJVsesNITZ73562061;     jJVsesNITZ73562061 = jJVsesNITZ98024937;     jJVsesNITZ98024937 = jJVsesNITZ29392912;     jJVsesNITZ29392912 = jJVsesNITZ54930657;     jJVsesNITZ54930657 = jJVsesNITZ76380445;     jJVsesNITZ76380445 = jJVsesNITZ33604882;     jJVsesNITZ33604882 = jJVsesNITZ77354690;     jJVsesNITZ77354690 = jJVsesNITZ50055782;     jJVsesNITZ50055782 = jJVsesNITZ41957473;     jJVsesNITZ41957473 = jJVsesNITZ36696791;     jJVsesNITZ36696791 = jJVsesNITZ9663821;     jJVsesNITZ9663821 = jJVsesNITZ92574379;     jJVsesNITZ92574379 = jJVsesNITZ2332664;     jJVsesNITZ2332664 = jJVsesNITZ67479160;     jJVsesNITZ67479160 = jJVsesNITZ72264861;     jJVsesNITZ72264861 = jJVsesNITZ23641884;     jJVsesNITZ23641884 = jJVsesNITZ80003597;     jJVsesNITZ80003597 = jJVsesNITZ68022729;     jJVsesNITZ68022729 = jJVsesNITZ67537314;     jJVsesNITZ67537314 = jJVsesNITZ74554712;     jJVsesNITZ74554712 = jJVsesNITZ27386207;     jJVsesNITZ27386207 = jJVsesNITZ89040171;     jJVsesNITZ89040171 = jJVsesNITZ65511730;     jJVsesNITZ65511730 = jJVsesNITZ53776300;     jJVsesNITZ53776300 = jJVsesNITZ75542814;     jJVsesNITZ75542814 = jJVsesNITZ57855371;     jJVsesNITZ57855371 = jJVsesNITZ18382545;     jJVsesNITZ18382545 = jJVsesNITZ75769443;     jJVsesNITZ75769443 = jJVsesNITZ30294167;     jJVsesNITZ30294167 = jJVsesNITZ20384848;     jJVsesNITZ20384848 = jJVsesNITZ70731340;     jJVsesNITZ70731340 = jJVsesNITZ74728567;     jJVsesNITZ74728567 = jJVsesNITZ92732958;     jJVsesNITZ92732958 = jJVsesNITZ13269346;     jJVsesNITZ13269346 = jJVsesNITZ60648248;     jJVsesNITZ60648248 = jJVsesNITZ96795884;     jJVsesNITZ96795884 = jJVsesNITZ39987934;     jJVsesNITZ39987934 = jJVsesNITZ91616889;     jJVsesNITZ91616889 = jJVsesNITZ44033843;     jJVsesNITZ44033843 = jJVsesNITZ7483023;     jJVsesNITZ7483023 = jJVsesNITZ41096736;     jJVsesNITZ41096736 = jJVsesNITZ11828193;     jJVsesNITZ11828193 = jJVsesNITZ13087518;     jJVsesNITZ13087518 = jJVsesNITZ55363096;     jJVsesNITZ55363096 = jJVsesNITZ5798397;     jJVsesNITZ5798397 = jJVsesNITZ57835049;     jJVsesNITZ57835049 = jJVsesNITZ98231677;     jJVsesNITZ98231677 = jJVsesNITZ1235022;     jJVsesNITZ1235022 = jJVsesNITZ23248043;     jJVsesNITZ23248043 = jJVsesNITZ78785880;     jJVsesNITZ78785880 = jJVsesNITZ87118519;     jJVsesNITZ87118519 = jJVsesNITZ23482857;     jJVsesNITZ23482857 = jJVsesNITZ29647571;     jJVsesNITZ29647571 = jJVsesNITZ75762504;     jJVsesNITZ75762504 = jJVsesNITZ117960;     jJVsesNITZ117960 = jJVsesNITZ84521889;     jJVsesNITZ84521889 = jJVsesNITZ32513207;     jJVsesNITZ32513207 = jJVsesNITZ75616612;     jJVsesNITZ75616612 = jJVsesNITZ79387843;     jJVsesNITZ79387843 = jJVsesNITZ18525074;     jJVsesNITZ18525074 = jJVsesNITZ15222337;     jJVsesNITZ15222337 = jJVsesNITZ1585247;     jJVsesNITZ1585247 = jJVsesNITZ19761615;     jJVsesNITZ19761615 = jJVsesNITZ21572625;     jJVsesNITZ21572625 = jJVsesNITZ65965451;     jJVsesNITZ65965451 = jJVsesNITZ34935253;     jJVsesNITZ34935253 = jJVsesNITZ99841421;     jJVsesNITZ99841421 = jJVsesNITZ89063317;     jJVsesNITZ89063317 = jJVsesNITZ6830912;     jJVsesNITZ6830912 = jJVsesNITZ75468976;     jJVsesNITZ75468976 = jJVsesNITZ83653949;     jJVsesNITZ83653949 = jJVsesNITZ88386708;     jJVsesNITZ88386708 = jJVsesNITZ23988886;     jJVsesNITZ23988886 = jJVsesNITZ60054291;     jJVsesNITZ60054291 = jJVsesNITZ33457977;     jJVsesNITZ33457977 = jJVsesNITZ15558014;     jJVsesNITZ15558014 = jJVsesNITZ75952653;     jJVsesNITZ75952653 = jJVsesNITZ10148635;     jJVsesNITZ10148635 = jJVsesNITZ47977903;     jJVsesNITZ47977903 = jJVsesNITZ17707766;     jJVsesNITZ17707766 = jJVsesNITZ59623694;     jJVsesNITZ59623694 = jJVsesNITZ17147523;     jJVsesNITZ17147523 = jJVsesNITZ52521401;     jJVsesNITZ52521401 = jJVsesNITZ51508287;     jJVsesNITZ51508287 = jJVsesNITZ33266329;     jJVsesNITZ33266329 = jJVsesNITZ47248483;     jJVsesNITZ47248483 = jJVsesNITZ45080997;     jJVsesNITZ45080997 = jJVsesNITZ16970454;     jJVsesNITZ16970454 = jJVsesNITZ13151387;     jJVsesNITZ13151387 = jJVsesNITZ76126359;     jJVsesNITZ76126359 = jJVsesNITZ50409428;}
// Junk Finished
