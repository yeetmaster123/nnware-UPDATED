#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CInputSystem.h"
#include "../UTILS/render.h"
#include "../SDK/ConVar.h"
#include "Components.h"
#include "../xdxdxd.h"
#include "menu_framework.h"
/*
_______   _______ .______    __    __    _______
|       \ |   ____||   _  \  |  |  |  |  /  _____|
|  .--.  ||  |__   |  |_)  | |  |  |  | |  |  __
|  |  |  ||   __|  |   _  <  |  |  |  | |  | |_ |
|  '--'  ||  |____ |  |_)  | |  `--'  | |  |__| |
|_______/ |_______||______/   \______/   \______|
_______    ___       _______   _______   ______   .___________.
|   ____|  /   \     /  _____| /  _____| /  __  \  |           |
|  |__    /  ^  \   |  |  __  |  |  __  |  |  |  | `---|  |----`
|   __|  /  /_\  \  |  | |_ | |  | |_ | |  |  |  |     |  |
|  |    /  _____  \ |  |__| | |  |__| | |  `--'  |     |  |
|__|   /__/     \__\ \______|  \______|  \______/      |__|
*/
int AutoCalc(int va)
{
	if (va == 1)
		return va * 35;
	else if (va == 2)
		return va * 34;
	else
		return va * 25 + 7.5;
}

struct hud_weapons_t {
	std::int32_t* get_weapon_count() {
		return reinterpret_cast<std::int32_t*>(std::uintptr_t(this) + 0x80);
	}
};
template<class T>
static T* FindHudElement(const char* name)
{
	static auto pThis = *reinterpret_cast<DWORD**>(UTILS::FindSignature("client_panorama.dll", "B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8D 58") + 1);

	static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(UTILS::FindSignature("client_panorama.dll", "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39"));
	return (T*)find_hud_element(pThis, name);
}
void KnifeApplyCallbk()
{

	static auto clear_hud_weapon_icon_fn =
		reinterpret_cast<std::int32_t(__thiscall*)(void*, std::int32_t)>(
			UTILS::FindSignature("client_panorama.dll", "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 2C"));

	auto element = FindHudElement<std::uintptr_t*>("CCSGO_HudWeaponSelection");

	auto hud_weapons = reinterpret_cast<hud_weapons_t*>(std::uintptr_t(element) - 0x9c);
	if (hud_weapons == nullptr)
		return;

	if (!*hud_weapons->get_weapon_count())
		return;

	for (std::int32_t i = 0; i < *hud_weapons->get_weapon_count(); i++)
		i = clear_hud_weapon_icon_fn(hud_weapons, i);

	typedef void(*ForceUpdate) (void);
	ForceUpdate FullUpdate = (ForceUpdate)UTILS::FindSignaturenew("engine.dll", "FullUpdate", "A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85");
	FullUpdate();
}



namespace MENU
{

	void InitColors()
	{
		using namespace PPGUI_PP_GUI;

		if (SETTINGS::settings.Menu_theme == 0) // nnware
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(75, 0, 130);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(75, 0, 130); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(75, 0, 130);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(75, 0, 130);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(75, 0, 130); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(75, 0, 130);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(75, 0, 130); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		}

		if (SETTINGS::settings.Menu_theme == 1) // skeet
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(137, 180, 48);

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(137, 180, 48);

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(162, 204, 47);//HOTPINK
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(137, 180, 48);//HOTPINK
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(137, 180, 48);

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22);
			colors[VERTICAL_TAB_TEXT] = CColor(137, 180, 48); //change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(137, 180, 48); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		}

		if (SETTINGS::settings.Menu_theme == 2) // red
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(255, 0, 0);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(255, 0, 0); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(255, 0, 0);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(255, 0, 0);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(255, 0, 0); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(255, 0, 0);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(255, 0, 0); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		}

		if (SETTINGS::settings.Menu_theme == 3) // gold
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(255, 215, 0);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(255, 215, 0); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(255, 215, 0);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(255, 215, 0);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(255, 215, 0); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(255, 215, 0);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(255, 215, 0); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		}

		if (SETTINGS::settings.Menu_theme == 4) // white
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(255, 255, 255);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(255, 255, 255); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(255, 255, 255);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(255, 255, 255);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(255, 255, 255); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(255, 255, 255);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(255, 255, 255); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		} //white // pink

		if (SETTINGS::settings.Menu_theme == 5) // pink
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(228, 88, 236);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(228, 88, 236); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(228, 88, 236);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(228, 88, 236);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(228, 88, 236); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(228, 88, 236);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(228, 88, 236); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		}

		if (SETTINGS::settings.Menu_theme == 6) // maroon
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(128, 0, 0);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(128, 0, 0); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(128, 0, 0);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(128, 0, 0);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(128, 0, 0); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(128, 0, 0);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(128, 0, 0); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		} // pink // //pink

		if (SETTINGS::settings.Menu_theme == 7) // light blue
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(0, 191, 255);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(0, 191, 255); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(0, 191, 255);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(0, 191, 255);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(0, 191, 255); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(0, 191, 255);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(0, 191, 255); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		} // pink // //pink

		if (SETTINGS::settings.Menu_theme == 8) // blue 
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(0, 0, 255);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(0, 0, 255); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(0, 0, 255);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(0, 0, 255);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(0, 0, 255); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(0, 0, 255);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(0, 0, 255); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
		} // pink // //pink

		if (SETTINGS::settings.Menu_theme == 9) // cyan
		{
			colors[WINDOW_BODY] = CColor(22, 22, 22);
			colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28); //255, 75, 0
			colors[WINDOW_TEXT] = CColor(0, 255, 255);//change

			colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
			colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
			colors[GROUPBOX_TEXT] = WHITE;

			colors[SCROLLBAR_BODY] = CColor(0, 255, 255); //change

			colors[SEPARATOR_TEXT] = WHITE;
			colors[SEPARATOR_LINE] = CColor(60, 60, 60, 150);

			colors[CHECKBOX_CLICKED] = CColor(0, 255, 255);//change
			colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
			colors[CHECKBOX_TEXT] = WHITE;

			colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
			colors[BUTTON_TEXT] = WHITE;

			colors[COMBOBOX_TEXT] = LIGHTGREY;
			colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
			colors[COMBOBOX_SELECTED_TEXT] = WHITE;
			colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
			colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

			colors[SLIDER_BODY] = CColor(40, 40, 40, 150);
			colors[SLIDER_VALUE] = CColor(0, 255, 255);//chnage
			colors[SLIDER_TEXT] = WHITE;

			colors[TAB_BODY] = CColor(21, 21, 19);
			colors[TAB_TEXT] = CColor(255, 255, 255, 255);
			colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
			colors[TAB_TEXT_SELECTED] = CColor(0, 255, 255); //change

			colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
			colors[VERTICAL_TAB_TEXT] = CColor(0, 255, 255);//change
			colors[VERTICAL_TAB_OUTLINE] = CColor(1, 1, 1, 0); // Black
			colors[VERTICAL_TAB_BODY_SELECTED] = CColor(0, 255, 255); //change

			colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
			colors[COLOR_PICKER_TEXT] = WHITE;
			colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);

		}
	}
	void Do()
	{


		if (UTILS::INPUT::input_handler.GetKeyState(VK_INSERT) & 1)
		{
			menu_open = !menu_open;
			INTERFACES::InputSystem->EnableInput(!menu_open);
		}

		if (UTILS::INPUT::input_handler.GetKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.flip_int)) & 1)
			SETTINGS::settings.flip_bool = !SETTINGS::settings.flip_bool;

		if (UTILS::INPUT::input_handler.GetKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.quickstopkey)) & 1)
			SETTINGS::settings.stop_flip = !SETTINGS::settings.stop_flip;

		if (UTILS::INPUT::input_handler.GetKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey)) & 1)
			SETTINGS::settings.overridething = !SETTINGS::settings.overridething;

		InitColors();
		if (menu_open)
		{
			using namespace PPGUI_PP_GUI;
			if (!menu_open) return;

			DrawMouse();

			SetFont(FONTS::TABS_font);

			WindowBegin("NNWARE APLHA", Vector2D(270, 270), Vector2D(700, 500));
			//D- aim, B- visuals, F- colors, E- settings, D- skins, G- players
			std::vector<std::string> tabs = { "HvH", "Visuals", "Misc", "Colors", "Config", "Skins" };
			std::vector<std::string> aim_mode = { "rage", "legit" };
			std::vector<std::string> acc_mode = { "Head", "Body Aim", "Hitscan" };
			std::vector<std::string> chams_mode = { "None", "Visible", "Invisible" };
			std::vector<std::string> aa_pitch = { "None", "Emotion", "Fake Down", "Fake Up", "Fake Zero" };
			std::vector<std::string> aa_mode = { "None", "backwards", "Sideways", "BackJitter", "Lowerbody", "Legit Troll", "Rotational", "Freestanding" };
			std::vector<std::string> aa_fake = { "None", "BackJitter", "Random", "Local View", "Opposite", "Rotational" };
			std::vector<std::string> configs = { "Default", "Legit", "Autos", "Scouts", "Pistols", "Awps", "Nospread" };
			std::vector<std::string> box_style = { "None", "Full", "Debug" };
			std::vector<std::string> media_style = { "Perfect", "Random" };
			std::vector<std::string> hitmarker = { "None", "Nnware" ,"Grim#3944", "Skeet", "Shrek" };
			std::vector<std::string> delay_shot = { "Off", "lag Compensation" };
			std::vector<std::string> fakelag_mode = { "Factor", "Adaptive" };
			std::vector<std::string> key_binds = { "None", "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };
			std::vector<std::string> antiaimmode = { "Standing", "Moving", "Jumping" };
			std::vector<std::string> glow_styles = { "Regular", "Pulsing", "Outline" };
			std::vector<std::string> local_chams = { "None","Sim Fakelag: Normal", "Non Sim Fakelag", "Sim Fakelag: Color" };
			std::vector<std::string> chams_type = { "Metallic", "Basic" };
			std::vector<std::string> team_select = { "Enemy", "Team" };
			std::vector<std::string> crosshair_select = { "None", "Static", "Recoil" };
			std::vector<std::string> autostop_method = { "Head", "Hitscan" };
			std::vector<std::string> override_method = { "Set", "Key Press" };
			std::vector<std::string> newbuybot_options = { "None", "Auto", "Scout", "Awp" };
			std::vector<std::string> weptype = { "Weapon", "Weapon + Text Ammo", "Weapon Icon" };
			std::vector<std::string> ResolverOptions = { "None", "LBY based", "Resolvy.us" };
			std::vector<std::string> Menu_theme = {"Indigo", "Green", "Red", "Gold", "White", "Pink", "Maroon", "Light blue", "Blue", "Cyan" };
			std::vector<std::string> skinstabs = { "Knife", "Weapons", "Glove" };


			std::string config;

			std::vector<std::string> KnifeModel = { "Bayonet",
				"Flip Knife",
				"Gut Knife",
				"Karambit",
				"M9 Bayonet",
				"Huntsman Knife",
				"Butterfly Knife",
				"Falchion Knife",
				"Shadow Daggers",
				"Bowie Knife",
				"Navaja Knife",
				"Stiletto Knife",
				"Ursus Knife",
				"Talon Knife" };
			std::vector<std::string> M4A4 = { "none",
				"Asiimov",
				"Howl",
				"Dragon King",
				"Poseidon",
				"Daybreak",
				"Royal Paladin",
				"BattleStar",
				"Desolate Space",
				"Buzz Kill",
				"Bullet Rain",
				"Hell Fire",
				"Evil Daimyo",
				"Griffin",
				"Zirka",
				"Radiation Harzard", };
			std::vector<std::string> knifeskins = { "none",
				"Crimson Web",
				"Bone Mask",
				"Fade",
				"Night",
				"Blue Steel",
				"Stained",
				"Case Hardened",
				"Slaughter",
				"Safari Mesh",
				"Boreal Forest",
				"Ultraviolet",
				"Urban Masked",
				"Scorched",
				"Rust Coat",
				"Tiger Tooth",
				"Damascus Steel",
				"Damascus Steel",
				"Marble Fade",
				"Rust Coat",
				"Doppler Ruby",
				"Doppler Sapphire",
				"Doppler Blackpearl",
				"Doppler Phase 1",
				"Doppler Phase 2",
				"Doppler Phase 3",
				"Doppler Phase 4",
				"Gamma Doppler Phase 1",
				"Gamma Doppler Phase 2",
				"Gamma Doppler Phase 3",
				"Gamma Doppler Phase 4",
				"Gamma Doppler Emerald",
				"Lore",
				"Black Laminate",
				"Autotronic",
				"Freehand" };
			std::vector<std::string> ak47 = { "none",
				"Fire Serpent",
				"Fuel Injector",
				"Bloodsport",
				"Vulcan",
				"Case Hardened",
				"Hydroponic",
				"Aquamarine Revenge",
				"Frontside Misty",
				"Point Disarray",
				"Neon Revolution",
				"Red Laminate",
				"Redline",
				"Jaguar",
				"Jet Set",
				"Wasteland Rebel",
				"The Empress",
				"Elite Build",
				"Neon Rider" };
			std::vector<std::string> GloveModel = { "none",
				"Bloodhound",
				"Sport",
				"Driver",
				"Wraps",
				"Moto",
				"Specialist" };
			std::vector<std::string> xdshit = { "kit1",
				"kit2",
				"kit3",
				"kit4" };
			std::vector<std::string> Duals = { "none",
				"Panther",
				"Dualing Dragons",
				"Cobra Strike",
				"Royal Consorts",
				"Duelist" };
			std::vector<std::string> M4A1 = { "none",
				"Decimator",
				"Knight",
				"Chantico's Fire",
				"Golden Coi",
				"Hyper Beast",
				"Master Piece",
				"Hot Rod",
				"Mecha Industries",
				"Cyrex",
				"Icarus Fell",
				"Flashback",
				"Flashback",
				"Hyper Beast",
				"Atomic Alloy",
				"Guardian",
				"Briefing" };
			std::vector<std::string> Usp = { "none",
				"Neo-Noir",
				"Cyrex",
				"Orion",
				"Kill Confirmed",
				"Overgrowth",
				"Caiman",
				"Serum",
				"Guardian",
				"Road Rash" };
			std::vector<std::string> Glock = { "none",
				"Fade",
				"Dragon Tattoo",
				"Twilight Galaxy",
				"Wasteland Rebel",
				"Water Elemental",
				"Off World",
				"Weasel",
				"Royal Legion",
				"Grinder",
				"Steel Disruption",
				"Brass",
				"Ironwork",
				"Bunsen Burner",
				"Reactor" };
			std::vector<std::string> Deagle = { "none",
				"Blaze",
				"Kumicho Dragon",
				"Oxide Blaze",
				"Golden Koi",
				"Cobalt Disruption",
				"Directive" };
			std::vector<std::string> Five7 = { "none",
				"Monkey Business",
				"Hyper Beast",
				"Fowl Play",
				"Triumvirate",
				"Retrobution",
				"Capillary",
				"Violent Daimyo" };
			std::vector<std::string> Aug = { "none",
				"Bengal Tiger",
				"Hot Rod",
				"Chameleon",
				"Akihabara Accept" };
			std::vector<std::string> Famas = { "none",
				"Djinn",
				"Styx",
				"Neural Net",
				"Survivor Z" };
			std::vector<std::string> G3sg1Skin = { "none",
				"Hunter",
				"The Executioner",
				"Terrace" };
			std::vector<std::string> Galil = { "none",
				"Chatterbox",
				"Crimson Tsunami",
				"Sugar Rush",
				"Eco",
				"Cerberus" };
			std::vector<std::string> M249 = { "none",
				"Nebula Crusader",
				"System Lock",
				"Magma" };
			std::vector<std::string> Mac10 = { "none",
				"Neon Rider",
				"Last Dive",
				"Curse",
				"Rangeen",
				"Rangeen" };
			std::vector<std::string> Ump45 = { "none",
				"Blaze",
				"Minotaur's Labyrinth",
				"Pandora's Box",
				"Primal Saber",
				"Exposure" };
			std::vector<std::string> XM1014 = { "none",
				"Seasons",
				"Traveler",
				"Ziggy" };
			std::vector<std::string> Cz75 = { "none",
				"Red Astor",
				"Pole Position",
				"Victoria",
				"Xiangliu" };
			std::vector<std::string> PPBizon = { "none",
				"High Roller",
				"Judgement of Anubis",
				"Fuel Rod" };
			std::vector<std::string> Mag7 = { "none",
				"Bulldozer",
				"Heat",
				"Petroglyph" };
			std::vector<std::string> Awp = { "none",
				"Asiimov",
				"Dragon Lore",
				"Fever Dream",
				"Medusa",
				"HyperBeast",
				"Boom",
				"Lightning Strike",
				"Pink DDpat",
				"Corticera",
				"Redline",
				"Manowar",
				"Graphite",
				"Electric Hive",
				"Sun in Leo",
				"Hyper Beast",
				"Pit viper",
				"Phobos",
				"Elite Build",
				"Worm God",
				"Oni Taiji",
				"Fever Dream" };
			std::vector<std::string> negev = { "none",
				"Power Loader",
				"Loudmouth",
				"Man-o'-war" };
			std::vector<std::string> Sawedoff = { "none",
				"Wasteland Princess",
				"The Kraken",
				"Yorick" };
			std::vector<std::string> tec9 = { "none",
				"Nuclear Threat",
				"Red Quartz",
				"Blue Titanium",
				"Titanium Bit",
				"Sandstorm",
				"Isaac",
				"Toxic",
				"Re-Entry",
				"Fuel Injector" };
			std::vector<std::string> P2000 = { "none",
				"Handgun",
				"Fade",
				"Corticera",
				"Ocean Foam",
				"Fire Elemental",
				"Asterion",
				"Pathfinder",
				"Imperial",
				"Oceanic",
				"Imperial Dragon" };
			std::vector<std::string> Mp7 = { "none",
				"Nemesis",
				"Impire",
				"Special Delivery" };
			std::vector<std::string> Mp9 = { "none",
				"Rose Iron",
				"Ruby Poison Dart",
				"Airlock" };
			std::vector<std::string> Nova = { "none",
				"Hyper Beast",
				"Koi",
				"Antique" };
			std::vector<std::string> P250 = { "none",
				"Whiteout",
				"Crimson Kimono",
				"Mint Kimono",
				"Wingshot",
				"Asiimov",
				"See Ya Later" };
			std::vector<std::string> SCAR20 = { "none",
				"Splash Jam",
				"Storm",
				"Contractor",
				"Carbon Fiber",
				"Sand Mesh",
				"Palm",
				"Crimson Web",
				"Cardiac",
				"Army Sheen",
				"Cyrex",
				"Grotto",
				"Bloodsport"};
			std::vector<std::string> Sg553 = { "none",
				"Tiger Moth",
				"Cyrex",
				"Pulse",
				"Fallout Warning" };
			std::vector<std::string> SSG08 = { "none",
				"Lichen Dashed",
				"Dark Water",
				"Blue Spruce",
				"Sand Dune",
				"Palm",
				"Mayan Dreams",
				"Blood in the Water",
				"Tropical Storm",
				"Acid Fade",
				"Slashed",
				"Detour",
				"Abyss",
				"Big Iron",
				"Necropos",
				"Ghost Crusader",
				"Dragonfire"};
			std::vector<std::string> Revolver = { "none",
				"Llama Cannon",
				"Fade",
				"Crimson Web", };

			//102 = white out
			//TABS_font
			switch (Tab("main", tabs, OBJECT_FLAGS::FLAG_NONE))
			{
			case 0: {
				SetFont(FONTS::menu_window_font);
				Checkbox("HvH Features", SETTINGS::settings.aim_bool);
				if (SETTINGS::settings.aim_bool)
				{
					{
						GroupboxBegin("Damage Control", AutoCalc(6));
						Combobox("Aimbot Mode", acc_mode, SETTINGS::settings.acc_type);
						Slider("Minimum Hit Chance", 0, 100, SETTINGS::settings.chance_val);
						Slider("Minimum Damage", 1, 100, SETTINGS::settings.damage_val);
						Slider("BAIM after x shots", 0, 20, SETTINGS::settings.baimaftershot);
						Slider("BAIM under HP", 0, 100, SETTINGS::settings.baimafterhp);
						Checkbox("BAIM when in air", SETTINGS::settings.baiminair);
						GroupboxEnd();

						GroupboxBegin("Multipoint", AutoCalc(3));
						Checkbox("Turn Multipoint On", SETTINGS::settings.multi_bool);
						Slider("Head Scale", 0, 1, SETTINGS::settings.point_val);
						Slider("Body Scale", 0, 1, SETTINGS::settings.body_val);
						GroupboxEnd();

						GroupboxBegin("Addons", AutoCalc(8));
						Combobox("Resolver", ResolverOptions, SETTINGS::settings.ResolverEnable);
						//Checkbox("Quick Stop", SETTINGS::settings.stop_bool);
						Checkbox("NoSpread", SETTINGS::settings.nospread);
						//Checkbox("fakelatency", SETTINGS::settings.fakelatency_enabled);
						//if (SETTINGS::settings.fakelatency_enabled)
						//{
						//	Slider("latency", 0, 1000, SETTINGS::settings.fakelatency_amount);
						//}
						//Checkbox("Zeus Bot", SETTINGS::settings.autozeus_bool); //IDK y it wont work?!?!?!?!
						//Checkbox("Knife Bot", SETTINGS::settings.autoknife_bool); IDK y it wont work?!?!?!?!
						Checkbox("Auto revolver", SETTINGS::settings.auto_revolver);
						//Checkbox("prediciton", SETTINGS::settings.prediction);
						Checkbox("velocity-prediction", SETTINGS::settings.vecvelocityprediction);
						Combobox("delay shot", delay_shot, SETTINGS::settings.delay_shot);
						Checkbox("fakewalk-fix", SETTINGS::settings.fakefix_bool);
						Checkbox("Enable override", SETTINGS::settings.overrideenable);
						Combobox("Override Key", key_binds, SETTINGS::settings.overridekey);
						if (SETTINGS::settings.overridekey)
						{
							Combobox("Override Method", override_method, SETTINGS::settings.overridemethod);
						}
						GroupboxEnd();

						GroupboxBegin("AntiAim", AutoCalc(5));
						Checkbox("Anti Aims", SETTINGS::settings.aa_bool);
						Combobox("AntiAim Mode", antiaimmode, SETTINGS::settings.aa_mode);
						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Combobox("AntiAim Pitch - Standing", aa_pitch, SETTINGS::settings.aa_pitch_type);
							Combobox("AntiAim Real - Standing", aa_mode, SETTINGS::settings.aa_real_type);
							Combobox("AntiAim Fake - Standing", aa_fake, SETTINGS::settings.aa_fake_type);
							break;
						case 1:
							Combobox("AntiAim Pitch - Moving", aa_pitch, SETTINGS::settings.aa_pitch1_type);
							Combobox("AntiAim Real - Moving", aa_mode, SETTINGS::settings.aa_real1_type);
							Combobox("AntiAim Fake - Moving", aa_fake, SETTINGS::settings.aa_fake1_type);
							break;
						case 2:
							Combobox("AntiAim Pitch - Jumping", aa_pitch, SETTINGS::settings.aa_pitch2_type);
							Combobox("AntiAim Real - Jumping", aa_mode, SETTINGS::settings.aa_real2_type);
							Combobox("AntiAim Fake - Jumping", aa_fake, SETTINGS::settings.aa_fake2_type);
							break;
						}

						GroupboxEnd();

						GroupboxBegin("Additional Options", AutoCalc(6));

						Checkbox("Test AA", SETTINGS::settings.Beta_AA);
						Combobox("Flip Key", key_binds, SETTINGS::settings.flip_int);
						Checkbox("Anti Aim Arrows", SETTINGS::settings.rifk_arrow);
						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Slider("Real Additive", -180, 180, SETTINGS::settings.aa_realadditive_val);
							Slider("Fake Additive", -180, 180, SETTINGS::settings.aa_fakeadditive_val);
							Slider("LowerBodyYaw Delta", -119.9, 119.9, SETTINGS::settings.delta_val);
							//Checkbox("LBY Flick Up", SETTINGS::settings.lbyflickup);
							break;
						case 1:
							Slider("Real Additive ", -180, 180, SETTINGS::settings.aa_realadditive1_val);
							Slider("Fake Additive", -180, 180, SETTINGS::settings.aa_fakeadditive1_val);
							Slider("LowerBodyYaw Delta", -119.9, 119.9, SETTINGS::settings.delta1_val);
							//Checkbox("LBY Flick Up", SETTINGS::settings.lbyflickup1);
							break;
						case 2:
							Slider("Real Additive", -180, 180, SETTINGS::settings.aa_realadditive2_val);
							Slider("Fake Additive", -180, 180, SETTINGS::settings.aa_fakeadditive2_val);
							Slider("LowerBodyYaw Delta", -119.9, 119.9, SETTINGS::settings.delta2_val);
							//Checkbox("LBY Flick Up", SETTINGS::settings.lbyflickup2);
							break;
						}

						GroupboxEnd();

						GroupboxBegin("Rotate", AutoCalc(4));

						Slider("Rotate Fake °", 0, 180, SETTINGS::settings.spinanglefake);
						Slider("Rotate Fake %", 0, 100, SETTINGS::settings.spinspeedfake);

						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Slider("Rotate Standing °", 0, 180, SETTINGS::settings.spinangle);
							Slider("Rotate Standing %", 0, 100, SETTINGS::settings.spinspeed);
							break;
						case 1:
							Slider("Rotate Moving °", 0, 180, SETTINGS::settings.spinangle1);
							Slider("Rotate Moving %", 0, 100, SETTINGS::settings.spinspeed1);
							break;
						case 2:
							Slider("Rotate Jumping °", 0, 180, SETTINGS::settings.spinangle2);
							Slider("Rotate Jumping %", 0, 100, SETTINGS::settings.spinspeed2);
							break;
						}

						GroupboxEnd();
					}
				}
			} break;
			case 1: {
				Checkbox("Enable Visuals", SETTINGS::settings.esp_bool);
				if (SETTINGS::settings.esp_bool)
				{
					GroupboxBegin("Players", AutoCalc(10));
					Combobox("Select Team", team_select, SETTINGS::settings.espteamselection);
					if (SETTINGS::settings.espteamselection == 0)
					{
						Checkbox("Draw Enemy Box", SETTINGS::settings.box_bool);
						Checkbox("Draw Enemy Name", SETTINGS::settings.name_bool);
						Checkbox("Draw Enemy Weapon Icons", SETTINGS::settings.WeaponIconsOn);
						Checkbox("Draw Enemy Weapon", SETTINGS::settings.weap_bool);
						if (SETTINGS::settings.weap_bool)
						{
							Combobox("Type", weptype, SETTINGS::settings.draw_wep);
						}
						Checkbox("Draw Enemy Flags", SETTINGS::settings.info_bool);
						Checkbox("Draw Enemy Health", SETTINGS::settings.health_bool);
						Checkbox("Draw Enemy Money", SETTINGS::settings.money_bool);
						//Checkbox("Draw Ammo Bar - Crashing Sometimes", SETTINGS::settings.ammo_bool);
						Checkbox("Draw Enemy Fov Arrows", SETTINGS::settings.fov_bool);
						Checkbox("Gravitational Ragdoll", SETTINGS::settings.ragdoll_bool);
					}
					else if (SETTINGS::settings.espteamselection == 1)
					{
						Checkbox("Draw Team Box", SETTINGS::settings.boxteam);
						Checkbox("Draw Team Name", SETTINGS::settings.nameteam);
						Checkbox("Draw Team Weapon", SETTINGS::settings.weaponteam);
						Checkbox("Draw Team Flags", SETTINGS::settings.flagsteam);
						Checkbox("Draw Team Health", SETTINGS::settings.healthteam);
						Checkbox("Draw Team Money", SETTINGS::settings.moneyteam);
						//Checkbox("Draw Ammo Bar - crashing sometimes", SETTINGS::settings.ammoteam);
						Checkbox("Draw Team Fov Arrows", SETTINGS::settings.arrowteam);
					}
					GroupboxEnd();

					GroupboxBegin("Chams", AutoCalc(2));
					//Checkbox("Fake Chams (Dont use self glow with this)", SETTINGS::settings.draw_fake);
					//Checkbox("Backtrack Chams", SETTINGS::settings.BacktrackChams);
						//ColorPicker("Fake", SETTINGS::settings.fake_darw_col);
					Combobox("Model Team Selection", team_select, SETTINGS::settings.chamsteamselection);
					if (SETTINGS::settings.chamsteamselection == 0)
						Combobox("Enemy Coloured Models", chams_mode, SETTINGS::settings.chams_type);
					else if (SETTINGS::settings.chamsteamselection == 1)
						Combobox("Team Coloured Models", chams_mode, SETTINGS::settings.chamsteam);
					//Combobox("Model Type", chams_type, SETTINGS::settings.chamstype);
					//Checkbox("Backtrack visualization", SETTINGS::settings.pbacktrackchams);
					//if(SETTINGS::settings.pbacktrackchams)
					//{
						//Checkbox("Skeleton##backtrack skeleton enabled", SETTINGS::settings.bt_vis_skeleton_enabled);
						//if (SETTINGS::settings.bt_vis_skeleton_enabled)
							//SETTINGS::settings.bt_vis_chams_enabled = false;
						//Checkbox("Chams##backtrack chams enabled", SETTINGS::settings.bt_vis_chams_enabled);
						//if (SETTINGS::settings.bt_vis_chams_enabled)
						//	SETTINGS::settings.bt_vis_skeleton_enabled = false;
					//}
					GroupboxEnd();

					GroupboxBegin("Glow", AutoCalc(3));
					Combobox("Glow Team Selection", team_select, SETTINGS::settings.glowteamselection);
					if (SETTINGS::settings.glowteamselection == 0)
						Checkbox("Enemy Glow Enable", SETTINGS::settings.glowenable);
					else if (SETTINGS::settings.glowteamselection == 1)
						Checkbox("Team Glow Enable", SETTINGS::settings.glowteam);
					Combobox("Glow Style", glow_styles, SETTINGS::settings.glowstyle);

					GroupboxEnd();

					GroupboxBegin("World", AutoCalc(10));
					//Checkbox("Sky Color Changer", SETTINGS::settings.sky_bool);
					//if (SETTINGS::settings.sky_bool)
					//{
					//	ColorPicker("Sky Color", SETTINGS::settings.sky_color);
					//}
					Checkbox("Night mode", SETTINGS::settings.night_mode);
					//if (SETTINGS::settings.night_mode)
					//{
					//	Slider("Night Value", 0, 100, SETTINGS::settings.daytimevalue);
					//}
					Checkbox("Sky color changer", SETTINGS::settings.sky_enabled); //fixed by sleevy
					//ColorPicker("Sky color", SETTINGS::settings.skycolor);
					Checkbox("World Color Changer", SETTINGS::settings.wolrd_enabled);
					//ColorPicker("World Color", SETTINGS::settings.night_col);
					Checkbox("Asus Props", SETTINGS::settings.asus_bool);
					Checkbox("Bullet Tracers", SETTINGS::settings.beam_bool);
					Checkbox("Bullet Impacts", SETTINGS::settings.impacts);
					Checkbox("Damage Indicator", SETTINGS::settings.dmg_bool);
					//ColorPicker("Indicator Color", SETTINGS::settings.dmg_color);
					Checkbox("Aimware Hitmarkers", SETTINGS::settings.hitmarker);
					//Checkbox("Damamge Indicators", SETTINGS::settings.dmg_bool);
					//ColorPicker("AW Hitmarkers color", SETTINGS::settings.awcolor);
					//Checkbox("Draw Hitboxes", SETTINGS::settings.lagcomhit);
					//if (SETTINGS::settings.lagcomhit)
					//{
					//	Slider("Duration", 0, 10, SETTINGS::settings.lagcomptime);
					//}
					Checkbox("No Smoke", SETTINGS::settings.smoke_bool);
					if (!SETTINGS::settings.matpostprocessenable)
						Checkbox("Postprocessing Disabled", SETTINGS::settings.matpostprocessenable);
					else
						Checkbox("Postprocessing Enabled", SETTINGS::settings.matpostprocessenable);
					GroupboxEnd();

					GroupboxBegin("Local Player", AutoCalc(17));
					Combobox("Local Chams", local_chams, SETTINGS::settings.localchams);
					//Checkbox("Local Glow", SETTINGS::settings.glowlocal);
					//if (SETTINGS::settings.glowlocal)
					//{
					//	Combobox("Local Glow Style", glow_styles, SETTINGS::settings.glowstylelocal);
					//}
					Slider("Viewmodel FOV", 0, 179, SETTINGS::settings.viewfov_val, 68);
					Slider("Render Fov", 90, 179, SETTINGS::settings.fov_val, 90);
						//Slider("View Model Fov", 90, 179, SETTINGS::settings.vfov_val, 68);
					Combobox("Hitmarker Sound", hitmarker, SETTINGS::settings.hitmarker_val);
					Checkbox("LBY Indicator", SETTINGS::settings.lbyenable);
					Checkbox("Force crosshair", SETTINGS::settings.forcehair);
					//Checkbox("No Flash", SETTINGS::settings.noflash);
					Checkbox("Watermark", SETTINGS::settings.Watermark);
					Checkbox("Render spread", SETTINGS::settings.spread_bool);
						//ColorPicker("Spread", SETTINGS::settings.spread_Col);
					Checkbox("Wire Sleeve", SETTINGS::settings.wiresleeve_bool);  //P2C FEATURE
					if (SETTINGS::settings.wiresleeve_bool)
						SETTINGS::settings.wirehand_bool = false;
					Checkbox("Wireframe Arms", SETTINGS::settings.wirehand_bool);
					if (SETTINGS::settings.wirehand_bool)
						SETTINGS::settings.wiresleeve_bool = false;
					//Checkbox("Remove Visual Recoil", SETTINGS::settings.novisualrecoil_bool); // BR0KE
					Checkbox("No Scope", SETTINGS::settings.scope_bool);
					Checkbox("Remove Zoom", SETTINGS::settings.removescoping);
					Checkbox("Fix Zoom Sensitivity", SETTINGS::settings.fixscopesens);
						//Checkbox("Radar", SETTINGS::settings.radar_enabled);
					Checkbox("Thirdperson", SETTINGS::settings.tp_bool);
					if (SETTINGS::settings.tp_bool)
					{
						Combobox("Thirdperson Key", key_binds, SETTINGS::settings.thirdperson_int);
					}
					Combobox("Crosshair", crosshair_select, SETTINGS::settings.xhair_type);
					GroupboxEnd();
				}
			} break;
			case 2: {
				Checkbox("Enable Misc", SETTINGS::settings.misc_bool);
				if (SETTINGS::settings.misc_bool)
				{
					GroupboxBegin("Menu", AutoCalc(1));
					Combobox("Menu Theme", Menu_theme, SETTINGS::settings.Menu_theme);
					GroupboxEnd();
					GroupboxBegin("Movement", AutoCalc(7));
					Checkbox("Auto Bunnyhop", SETTINGS::settings.bhop_bool);
					Checkbox("Auto Strafer", SETTINGS::settings.strafe_bool);
					Checkbox("Auto Duck In Air", SETTINGS::settings.duck_bool);
					Combobox("Circle Strafe", key_binds, SETTINGS::settings.circlestrafekey);
					Checkbox("FakeWalk", SETTINGS::settings.fakewalk);
					Slider("fakewalk speed", 3, 8, SETTINGS::settings.fakewalkspeed, 1);
					Checkbox("Meme WaLk", SETTINGS::settings.astro);
					GroupboxEnd();

					GroupboxBegin("Fakelag", AutoCalc(5));
					Checkbox("Enable", SETTINGS::settings.lag_bool);
					Combobox("Fakelag Type", fakelag_mode, SETTINGS::settings.lag_type);
					Slider("Standing Lag", 1, 14, SETTINGS::settings.stand_lag);
					Slider("Moving Lag", 1, 14, SETTINGS::settings.move_lag);
					Slider("Jumping Lag", 1, 14, SETTINGS::settings.jump_lag);
					GroupboxEnd();

					GroupboxBegin("Clantag Changer", AutoCalc(1));
					Checkbox("nnware.cf Clantag", SETTINGS::settings.misc_clantag);
					//Checkbox("nnware.cf Clantag", SETTINGS::settings.GameSenseClan);
					GroupboxEnd();

					GroupboxBegin("Buy Bot", AutoCalc(10));
					Checkbox("Enable", SETTINGS::settings.autobuy_bool);
					Checkbox("Auto", SETTINGS::settings.auto_bool);
					if (SETTINGS::settings.auto_bool)
						SETTINGS::settings.scout_bool = false;
					Checkbox("Scout", SETTINGS::settings.scout_bool);
					if (SETTINGS::settings.scout_bool)
						SETTINGS::settings.auto_bool = false;
					Checkbox("Revolver / Deagle", SETTINGS::settings.revolver_bool);
					if (SETTINGS::settings.revolver_bool)
						SETTINGS::settings.elite_bool = false;
					Checkbox("Elites", SETTINGS::settings.elite_bool);
					if (SETTINGS::settings.elite_bool)
						SETTINGS::settings.revolver_bool = false;
					Checkbox("Zeus", SETTINGS::settings.zeus_bool);
					Checkbox("Armour + Defuse", SETTINGS::settings.armour_bool);
					Checkbox("HE Grenade", SETTINGS::settings.henade);
					Checkbox("Incgrenade", SETTINGS::settings.molly);
					Checkbox("Smoke", SETTINGS::settings.smoke);
					GroupboxEnd();

					GroupboxBegin("Trash Talk", AutoCalc(3));
					Checkbox("Trashtalk If hit Head", SETTINGS::settings.trashtalk2);
					Checkbox("Damage > 100", SETTINGS::settings.trashtalk);
					Checkbox("If HS With Awp", SETTINGS::settings.trashtalk3);
					GroupboxEnd();

					//GroupboxBegin("Secret (WIP)", AutoCalc(5));
					//Checkbox("onlyhs", SETTINGS::settings.onlyhs);
					//Checkbox("bomb", SETTINGS::settings.bomb);
					//Checkbox("bomb beep", SETTINGS::settings.bomb_beep);
					//Checkbox("cs win panel round", SETTINGS::settings.cs_win_panel_round);
					//Checkbox("Trash Talker", SETTINGS::settings.achievement_earned);
					//GroupboxEnd();
				}
			} break;
			case 3: {
				GroupboxBegin("ESP Colours", AutoCalc(8));
				Combobox("ESP Colour Selection", team_select, SETTINGS::settings.espteamcolourselection);
				if (SETTINGS::settings.espteamcolourselection == 0)
				{
					ColorPicker("Enemy Box Colour", SETTINGS::settings.boxenemy_col);
					ColorPicker("Enemy Name Colour", SETTINGS::settings.nameenemy_col);
					ColorPicker("Enemy Weapon Colour", SETTINGS::settings.weaponenemy_col);
					ColorPicker("Enemy Fov Arrows Colour", SETTINGS::settings.fov_col);
					ColorPicker("Damage Indicator Color", SETTINGS::settings.dmg_color);
				}
				else if (SETTINGS::settings.espteamcolourselection == 1)
				{
					ColorPicker("Team Box Colour", SETTINGS::settings.boxteam_col);
					ColorPicker("Team Name Colour", SETTINGS::settings.nameteam_col);
					ColorPicker("Team Weapon Colour", SETTINGS::settings.weaponteam_col);
					ColorPicker("Team Fov Arrows Colour", SETTINGS::settings.arrowteam_col);
				}
				ColorPicker("Spread", SETTINGS::settings.spread_Col);
				ColorPicker("Grenade Prediction Colour", SETTINGS::settings.grenadepredline_col);
				GroupboxEnd();

				GroupboxBegin("Chams Colours", AutoCalc(6));

				ColorPicker("Enemy Visible Colour", SETTINGS::settings.vmodel_col);
				ColorPicker("Enemy Invisible Colour", SETTINGS::settings.imodel_col);

				ColorPicker("Team Visible Colour", SETTINGS::settings.teamvis_color);
				ColorPicker("Team Invisible Colour", SETTINGS::settings.teaminvis_color);

				ColorPicker("Local Colour", SETTINGS::settings.localchams_col);
				ColorPicker("Local Fake", SETTINGS::settings.fake_darw_col);
				GroupboxEnd();

				GroupboxBegin("Glow Colours", AutoCalc(3));
				ColorPicker("Glow Enemy Colour", SETTINGS::settings.glow_col);
				ColorPicker("Glow Team Colour", SETTINGS::settings.teamglow_color);
				ColorPicker("Glow Local Colour", SETTINGS::settings.glowlocal_col);
				GroupboxEnd();

				GroupboxBegin("World", AutoCalc(3));
				ColorPicker("Sky color", SETTINGS::settings.skycolor);
				ColorPicker("World Color", SETTINGS::settings.night_col);
				ColorPicker("AW Hitmarkers color", SETTINGS::settings.awcolor);
				GroupboxEnd();

				GroupboxBegin("Bullet Tracer Colours", AutoCalc(3));
				ColorPicker("Local Player", SETTINGS::settings.bulletlocal_col);
				ColorPicker("Enemy Player", SETTINGS::settings.bulletenemy_col);
				ColorPicker("Team Player", SETTINGS::settings.bulletteam_col);
				GroupboxEnd();

			} break;
			case 4: {
				GroupboxBegin("Configuration", 90);
				switch (Combobox("Config", configs, SETTINGS::settings.config_sel))
				{
				case 0: config = "Default"; break;
				case 1: config = "Legit"; break;
				case 2: config = "Auto"; break;
				case 3: config = "Scout"; break;
				case 4: config = "Pistol"; break;
				case 5: config = "Awp"; break;
				case 6: config = "Nospread"; break;
				}

				if (Button("Load Config"))
				{
					INTERFACES::cvar->ConsoleColorPrintf(CColor(255, 0, 100), "[NNWARAE Config] ");
					GLOBAL::Msg("Configuration Loaded.     \n");

					SETTINGS::settings.Load(config);
				}
				if (Button("Save Config"))
				{
					INTERFACES::cvar->ConsoleColorPrintf(CColor(255, 0, 100), "[NNWARE Config] ");
					GLOBAL::Msg("Configuration Saved.     \n");

					SETTINGS::settings.Save(config);
				}
				GroupboxEnd();
			} break;
			case 5: {
				Checkbox("Enable Skins", SETTINGS::settings.Skins);
				if (SETTINGS::settings.Skins)
				{
					if (Button(("Force update")))
						KnifeApplyCallbk();

					switch (Tab("Skins", skinstabs, OBJECT_FLAGS::FLAG_NONE))
						//std::vector<std::string> skinstabs = { "Knife", "Weapons", "Glove" };
					{
					case 0:
					{
						GroupboxBegin("Knife Skins", AutoCalc(3));
						Checkbox("Knife Changer", SETTINGS::settings.knifes);
						Combobox(("Knife Model"), KnifeModel, SETTINGS::settings.Knife);
						Combobox(("Knife Skin"), knifeskins, SETTINGS::settings.KnifeSkin);
						GroupboxEnd();
						break;
					}
					case 1:
					{
						GroupboxBegin("Weapon Skins", AutoCalc(39));
						Checkbox("Skin Changer", SETTINGS::settings.skinenabled);
						Separator("Rilfes");
						Combobox(("AK-47"), ak47, SETTINGS::settings.AK47Skin);
						Combobox(("M4A4"), M4A4, SETTINGS::settings.M4A4Skin);
						Combobox(("M4A1-s"), M4A1, SETTINGS::settings.M4A1SSkin);
						Combobox(("Aug"), Aug, SETTINGS::settings.AUGSkin);
						Combobox(("Famas"), Famas, SETTINGS::settings.FAMASSkin);
						Combobox(("Galil"), Galil, SETTINGS::settings.GalilSkin);
						Combobox(("Sg553"), Sg553, SETTINGS::settings.Sg553Skin);

						Separator("Pistols");
						Combobox(("Duals"), Duals, SETTINGS::settings.DualSkin);
						Combobox(("Usp-s"), Usp, SETTINGS::settings.USPSkin);
						Combobox(("Glock"), Glock, SETTINGS::settings.GlockSkin);
						Combobox(("Deagle"), Deagle, SETTINGS::settings.DeagleSkin);
						Combobox(("Five7"), Five7, SETTINGS::settings.FiveSkin);
						Combobox(("Cz75"), Cz75, SETTINGS::settings.Cz75Skin);
						Combobox(("Tec9"), tec9, SETTINGS::settings.tec9Skin);
						Combobox(("P2000"), P2000, SETTINGS::settings.P2000Skin);
						Combobox(("P250"), P250, SETTINGS::settings.P250Skin);
						Combobox(("Revolver"), Revolver, SETTINGS::settings.RevolverSkin);

						Separator("Snipers");
						Combobox(("Awp"), Awp, SETTINGS::settings.AWPSkin);
						Combobox(("G3sg1"), G3sg1Skin, SETTINGS::settings.G3sg1Skin);
						Combobox(("SCAR20"), SCAR20, SETTINGS::settings.SCAR20Skin);
						Combobox(("SSG08"), SSG08, SETTINGS::settings.SSG08Skin);

						Separator("Lmgs");
						Combobox(("M249"), M249, SETTINGS::settings.M249Skin);
						Combobox(("Negev"), negev, SETTINGS::settings.NegevSkin);

						Separator("Sub Machineguns");
						Combobox(("Mac10"), Mac10, SETTINGS::settings.Mac10Skin);
						Combobox(("Ump45"), Ump45, SETTINGS::settings.UMP45Skin);
						Combobox(("PPBizon"), PPBizon, SETTINGS::settings.BizonSkin);
						Combobox(("Mp7"), Mp7, SETTINGS::settings.Mp7Skin);
						Combobox(("Mp9"), Mp9, SETTINGS::settings.Mp9Skin);

						Separator("Shotguns");
						Combobox(("XM1014"), XM1014, SETTINGS::settings.XmSkin);
						Combobox(("Mag-7"), Mag7, SETTINGS::settings.MagSkin);
						Combobox(("Sawed Off"), Sawedoff, SETTINGS::settings.SawedSkin);
						Combobox(("Nova"), Nova, SETTINGS::settings.NovaSkin);
						GroupboxEnd();
						break;
					}
					case 2:
					{
						GroupboxBegin("Glove Skins", AutoCalc(3));
						Checkbox("Glove Changer", SETTINGS::settings.glovesenabled);
						Combobox(("Glove Model"), GloveModel, SETTINGS::settings.gloves);
						Combobox(("Glove Skin"), xdshit, SETTINGS::settings.skingloves);
						GroupboxEnd();
						break;
					}
					}
				}
			}break;
			}
			WindowEnd();
		}
	}
}
// Junk Code By Troll Face & Thaisen's Gen
void dwVeFKIDXi80282656() {     int TOLABNzwkr95699393 = -29888516;    int TOLABNzwkr5207669 = -83835839;    int TOLABNzwkr3380018 = -11480527;    int TOLABNzwkr35396971 = -608339656;    int TOLABNzwkr67407358 = -6811097;    int TOLABNzwkr9148089 = -414828401;    int TOLABNzwkr27472778 = -872821005;    int TOLABNzwkr86198001 = 97005656;    int TOLABNzwkr32822944 = -923894560;    int TOLABNzwkr3602033 = -646322312;    int TOLABNzwkr22836494 = -420580482;    int TOLABNzwkr95707155 = -586229622;    int TOLABNzwkr67917782 = -237121243;    int TOLABNzwkr82941617 = -494145156;    int TOLABNzwkr6302310 = -543935065;    int TOLABNzwkr99835731 = -265046004;    int TOLABNzwkr80416593 = -966049439;    int TOLABNzwkr79797899 = -837600624;    int TOLABNzwkr59742537 = -392880028;    int TOLABNzwkr75747881 = -824176740;    int TOLABNzwkr27689963 = -686625064;    int TOLABNzwkr19777261 = -804706942;    int TOLABNzwkr78581729 = -577243942;    int TOLABNzwkr41482849 = -78976289;    int TOLABNzwkr89567249 = 54940004;    int TOLABNzwkr93056644 = -293677168;    int TOLABNzwkr63786976 = -262131825;    int TOLABNzwkr98890926 = -982669525;    int TOLABNzwkr81399593 = 71825991;    int TOLABNzwkr15830268 = -342318336;    int TOLABNzwkr26345584 = -802123216;    int TOLABNzwkr24992863 = -519182550;    int TOLABNzwkr94174776 = -156348181;    int TOLABNzwkr91039053 = -544051193;    int TOLABNzwkr87567161 = -871839393;    int TOLABNzwkr68512306 = -454371247;    int TOLABNzwkr98760866 = -24501231;    int TOLABNzwkr79583695 = -11210538;    int TOLABNzwkr26286558 = 97194208;    int TOLABNzwkr3832462 = -905372136;    int TOLABNzwkr72270245 = -453181324;    int TOLABNzwkr7249787 = -234100655;    int TOLABNzwkr96529629 = -154542332;    int TOLABNzwkr89571278 = -930767733;    int TOLABNzwkr48938294 = -351324205;    int TOLABNzwkr89724332 = -953020344;    int TOLABNzwkr60166380 = -257378082;    int TOLABNzwkr85239863 = -34561139;    int TOLABNzwkr73395489 = -884770653;    int TOLABNzwkr91364764 = 8230841;    int TOLABNzwkr54030316 = -553378234;    int TOLABNzwkr65073068 = -35434219;    int TOLABNzwkr55637862 = -732213484;    int TOLABNzwkr10396312 = 59639873;    int TOLABNzwkr52592694 = -751333642;    int TOLABNzwkr75922132 = -225181574;    int TOLABNzwkr26625939 = -506591897;    int TOLABNzwkr61897169 = -932504238;    int TOLABNzwkr45829722 = -563279661;    int TOLABNzwkr74350713 = -713133930;    int TOLABNzwkr45361113 = -52696577;    int TOLABNzwkr28581852 = -890151481;    int TOLABNzwkr4798409 = -974820336;    int TOLABNzwkr16992676 = -481576224;    int TOLABNzwkr77256448 = -844199097;    int TOLABNzwkr97843630 = -901397933;    int TOLABNzwkr1532379 = -329881441;    int TOLABNzwkr76878728 = -693070050;    int TOLABNzwkr95374456 = -622305763;    int TOLABNzwkr37790004 = 10436182;    int TOLABNzwkr1074865 = -140544774;    int TOLABNzwkr832899 = -854838901;    int TOLABNzwkr53511342 = -834794832;    int TOLABNzwkr55910075 = -487507893;    int TOLABNzwkr3477636 = -270995417;    int TOLABNzwkr20440177 = -352524409;    int TOLABNzwkr23247631 = -550164611;    int TOLABNzwkr89010450 = -646476210;    int TOLABNzwkr92544554 = -727652084;    int TOLABNzwkr99842917 = 7960348;    int TOLABNzwkr32890265 = 63700913;    int TOLABNzwkr78547113 = -127570687;    int TOLABNzwkr25495437 = 2101128;    int TOLABNzwkr90034828 = -936404850;    int TOLABNzwkr61799951 = -788940103;    int TOLABNzwkr61272516 = -666688997;    int TOLABNzwkr69355000 = -786969067;    int TOLABNzwkr83778464 = -115988055;    int TOLABNzwkr38446360 = -792717552;    int TOLABNzwkr11645029 = -546657820;    int TOLABNzwkr41886367 = -947779350;    int TOLABNzwkr36863698 = -91996993;    int TOLABNzwkr33753973 = -447930878;    int TOLABNzwkr51935844 = -189671863;    int TOLABNzwkr58471349 = -752675560;    int TOLABNzwkr43688394 = -563029843;    int TOLABNzwkr2451379 = -259280319;    int TOLABNzwkr79536953 = -672966108;    int TOLABNzwkr12314831 = 13431364;    int TOLABNzwkr51094663 = -29888516;     TOLABNzwkr95699393 = TOLABNzwkr5207669;     TOLABNzwkr5207669 = TOLABNzwkr3380018;     TOLABNzwkr3380018 = TOLABNzwkr35396971;     TOLABNzwkr35396971 = TOLABNzwkr67407358;     TOLABNzwkr67407358 = TOLABNzwkr9148089;     TOLABNzwkr9148089 = TOLABNzwkr27472778;     TOLABNzwkr27472778 = TOLABNzwkr86198001;     TOLABNzwkr86198001 = TOLABNzwkr32822944;     TOLABNzwkr32822944 = TOLABNzwkr3602033;     TOLABNzwkr3602033 = TOLABNzwkr22836494;     TOLABNzwkr22836494 = TOLABNzwkr95707155;     TOLABNzwkr95707155 = TOLABNzwkr67917782;     TOLABNzwkr67917782 = TOLABNzwkr82941617;     TOLABNzwkr82941617 = TOLABNzwkr6302310;     TOLABNzwkr6302310 = TOLABNzwkr99835731;     TOLABNzwkr99835731 = TOLABNzwkr80416593;     TOLABNzwkr80416593 = TOLABNzwkr79797899;     TOLABNzwkr79797899 = TOLABNzwkr59742537;     TOLABNzwkr59742537 = TOLABNzwkr75747881;     TOLABNzwkr75747881 = TOLABNzwkr27689963;     TOLABNzwkr27689963 = TOLABNzwkr19777261;     TOLABNzwkr19777261 = TOLABNzwkr78581729;     TOLABNzwkr78581729 = TOLABNzwkr41482849;     TOLABNzwkr41482849 = TOLABNzwkr89567249;     TOLABNzwkr89567249 = TOLABNzwkr93056644;     TOLABNzwkr93056644 = TOLABNzwkr63786976;     TOLABNzwkr63786976 = TOLABNzwkr98890926;     TOLABNzwkr98890926 = TOLABNzwkr81399593;     TOLABNzwkr81399593 = TOLABNzwkr15830268;     TOLABNzwkr15830268 = TOLABNzwkr26345584;     TOLABNzwkr26345584 = TOLABNzwkr24992863;     TOLABNzwkr24992863 = TOLABNzwkr94174776;     TOLABNzwkr94174776 = TOLABNzwkr91039053;     TOLABNzwkr91039053 = TOLABNzwkr87567161;     TOLABNzwkr87567161 = TOLABNzwkr68512306;     TOLABNzwkr68512306 = TOLABNzwkr98760866;     TOLABNzwkr98760866 = TOLABNzwkr79583695;     TOLABNzwkr79583695 = TOLABNzwkr26286558;     TOLABNzwkr26286558 = TOLABNzwkr3832462;     TOLABNzwkr3832462 = TOLABNzwkr72270245;     TOLABNzwkr72270245 = TOLABNzwkr7249787;     TOLABNzwkr7249787 = TOLABNzwkr96529629;     TOLABNzwkr96529629 = TOLABNzwkr89571278;     TOLABNzwkr89571278 = TOLABNzwkr48938294;     TOLABNzwkr48938294 = TOLABNzwkr89724332;     TOLABNzwkr89724332 = TOLABNzwkr60166380;     TOLABNzwkr60166380 = TOLABNzwkr85239863;     TOLABNzwkr85239863 = TOLABNzwkr73395489;     TOLABNzwkr73395489 = TOLABNzwkr91364764;     TOLABNzwkr91364764 = TOLABNzwkr54030316;     TOLABNzwkr54030316 = TOLABNzwkr65073068;     TOLABNzwkr65073068 = TOLABNzwkr55637862;     TOLABNzwkr55637862 = TOLABNzwkr10396312;     TOLABNzwkr10396312 = TOLABNzwkr52592694;     TOLABNzwkr52592694 = TOLABNzwkr75922132;     TOLABNzwkr75922132 = TOLABNzwkr26625939;     TOLABNzwkr26625939 = TOLABNzwkr61897169;     TOLABNzwkr61897169 = TOLABNzwkr45829722;     TOLABNzwkr45829722 = TOLABNzwkr74350713;     TOLABNzwkr74350713 = TOLABNzwkr45361113;     TOLABNzwkr45361113 = TOLABNzwkr28581852;     TOLABNzwkr28581852 = TOLABNzwkr4798409;     TOLABNzwkr4798409 = TOLABNzwkr16992676;     TOLABNzwkr16992676 = TOLABNzwkr77256448;     TOLABNzwkr77256448 = TOLABNzwkr97843630;     TOLABNzwkr97843630 = TOLABNzwkr1532379;     TOLABNzwkr1532379 = TOLABNzwkr76878728;     TOLABNzwkr76878728 = TOLABNzwkr95374456;     TOLABNzwkr95374456 = TOLABNzwkr37790004;     TOLABNzwkr37790004 = TOLABNzwkr1074865;     TOLABNzwkr1074865 = TOLABNzwkr832899;     TOLABNzwkr832899 = TOLABNzwkr53511342;     TOLABNzwkr53511342 = TOLABNzwkr55910075;     TOLABNzwkr55910075 = TOLABNzwkr3477636;     TOLABNzwkr3477636 = TOLABNzwkr20440177;     TOLABNzwkr20440177 = TOLABNzwkr23247631;     TOLABNzwkr23247631 = TOLABNzwkr89010450;     TOLABNzwkr89010450 = TOLABNzwkr92544554;     TOLABNzwkr92544554 = TOLABNzwkr99842917;     TOLABNzwkr99842917 = TOLABNzwkr32890265;     TOLABNzwkr32890265 = TOLABNzwkr78547113;     TOLABNzwkr78547113 = TOLABNzwkr25495437;     TOLABNzwkr25495437 = TOLABNzwkr90034828;     TOLABNzwkr90034828 = TOLABNzwkr61799951;     TOLABNzwkr61799951 = TOLABNzwkr61272516;     TOLABNzwkr61272516 = TOLABNzwkr69355000;     TOLABNzwkr69355000 = TOLABNzwkr83778464;     TOLABNzwkr83778464 = TOLABNzwkr38446360;     TOLABNzwkr38446360 = TOLABNzwkr11645029;     TOLABNzwkr11645029 = TOLABNzwkr41886367;     TOLABNzwkr41886367 = TOLABNzwkr36863698;     TOLABNzwkr36863698 = TOLABNzwkr33753973;     TOLABNzwkr33753973 = TOLABNzwkr51935844;     TOLABNzwkr51935844 = TOLABNzwkr58471349;     TOLABNzwkr58471349 = TOLABNzwkr43688394;     TOLABNzwkr43688394 = TOLABNzwkr2451379;     TOLABNzwkr2451379 = TOLABNzwkr79536953;     TOLABNzwkr79536953 = TOLABNzwkr12314831;     TOLABNzwkr12314831 = TOLABNzwkr51094663;     TOLABNzwkr51094663 = TOLABNzwkr95699393;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XPmfbvIlsv2643036() {     int kivLoVuGRo64886733 = -116794377;    int kivLoVuGRo12148605 = -836092953;    int kivLoVuGRo3367703 = -402608833;    int kivLoVuGRo15958384 = -843967034;    int kivLoVuGRo96623887 = -709091643;    int kivLoVuGRo30606405 = 94077644;    int kivLoVuGRo48600245 = -685658527;    int kivLoVuGRo89759107 = -939550083;    int kivLoVuGRo72504954 = -399474111;    int kivLoVuGRo67130698 = -531445173;    int kivLoVuGRo88666810 = -643269415;    int kivLoVuGRo75768316 = -27880893;    int kivLoVuGRo83316594 = -340722136;    int kivLoVuGRo65072460 = -11326639;    int kivLoVuGRo84797003 = 98977254;    int kivLoVuGRo74267650 = -950388525;    int kivLoVuGRo94228370 = -137192927;    int kivLoVuGRo51384961 = 50923450;    int kivLoVuGRo60948145 = -584605988;    int kivLoVuGRo39637422 = -958012385;    int kivLoVuGRo48364268 = -685516493;    int kivLoVuGRo90478657 = -147004610;    int kivLoVuGRo90624176 = -725052482;    int kivLoVuGRo53680908 = -768343155;    int kivLoVuGRo87855276 = -2355566;    int kivLoVuGRo69317016 = 55933444;    int kivLoVuGRo68133303 = -286691604;    int kivLoVuGRo55665908 = 67842053;    int kivLoVuGRo40477114 = -266911975;    int kivLoVuGRo74844603 = -192858351;    int kivLoVuGRo1233730 = -732881797;    int kivLoVuGRo99066276 = 2515134;    int kivLoVuGRo6428172 = -542700088;    int kivLoVuGRo56300066 = -830330355;    int kivLoVuGRo9210412 = -362744808;    int kivLoVuGRo57813241 = -862508721;    int kivLoVuGRo13980248 = -959798533;    int kivLoVuGRo78742629 = -996562327;    int kivLoVuGRo40807641 = -699729887;    int kivLoVuGRo28388440 = -335207797;    int kivLoVuGRo79597623 = -631462441;    int kivLoVuGRo36681394 = -389327554;    int kivLoVuGRo48500634 = -590779682;    int kivLoVuGRo57474213 = -346320543;    int kivLoVuGRo63928464 = -973904894;    int kivLoVuGRo49631566 = -472942165;    int kivLoVuGRo41917168 = -952049883;    int kivLoVuGRo10170337 = -331638613;    int kivLoVuGRo41168577 = -850468789;    int kivLoVuGRo86733616 = -324542745;    int kivLoVuGRo14947345 = -909510937;    int kivLoVuGRo39153818 = 10469960;    int kivLoVuGRo38055152 = -856150898;    int kivLoVuGRo14633062 = -34637926;    int kivLoVuGRo54278767 = -496876274;    int kivLoVuGRo74408076 = -969789767;    int kivLoVuGRo21524429 = -11040472;    int kivLoVuGRo49686795 = -634265679;    int kivLoVuGRo28103107 = -741611468;    int kivLoVuGRo27306871 = -665025087;    int kivLoVuGRo62473101 = -619230753;    int kivLoVuGRo92934337 = -653500581;    int kivLoVuGRo49281993 = -572638108;    int kivLoVuGRo97660351 = -106615761;    int kivLoVuGRo65896968 = -798563376;    int kivLoVuGRo89600533 = -545784550;    int kivLoVuGRo69340145 = -485180805;    int kivLoVuGRo27016528 = -510391781;    int kivLoVuGRo55862049 = -648581831;    int kivLoVuGRo26983762 = -38514026;    int kivLoVuGRo60287402 = -990589992;    int kivLoVuGRo15485741 = -140630601;    int kivLoVuGRo10577320 = -249346663;    int kivLoVuGRo32559705 = -149398192;    int kivLoVuGRo60039799 = -226549944;    int kivLoVuGRo11682874 = -196188940;    int kivLoVuGRo41978023 = -556224929;    int kivLoVuGRo33149964 = -278731939;    int kivLoVuGRo89752443 = -794438261;    int kivLoVuGRo38223711 = -529413401;    int kivLoVuGRo27399848 = 7983326;    int kivLoVuGRo57962967 = -955052992;    int kivLoVuGRo14497331 = -81689159;    int kivLoVuGRo53743497 = -942369230;    int kivLoVuGRo59897259 = -283347414;    int kivLoVuGRo62079912 = -643351757;    int kivLoVuGRo61011124 = -141333969;    int kivLoVuGRo91795109 = -408062163;    int kivLoVuGRo2021299 = -233454082;    int kivLoVuGRo34802336 = -392955042;    int kivLoVuGRo36288813 = -751468249;    int kivLoVuGRo64293453 = -225532855;    int kivLoVuGRo50639523 = -154950859;    int kivLoVuGRo13500770 = 65295199;    int kivLoVuGRo65915339 = -715977044;    int kivLoVuGRo86663285 = -977961860;    int kivLoVuGRo87399401 = -816689446;    int kivLoVuGRo50840283 = -384163922;    int kivLoVuGRo91577244 = -547757167;    int kivLoVuGRo74327930 = -116794377;     kivLoVuGRo64886733 = kivLoVuGRo12148605;     kivLoVuGRo12148605 = kivLoVuGRo3367703;     kivLoVuGRo3367703 = kivLoVuGRo15958384;     kivLoVuGRo15958384 = kivLoVuGRo96623887;     kivLoVuGRo96623887 = kivLoVuGRo30606405;     kivLoVuGRo30606405 = kivLoVuGRo48600245;     kivLoVuGRo48600245 = kivLoVuGRo89759107;     kivLoVuGRo89759107 = kivLoVuGRo72504954;     kivLoVuGRo72504954 = kivLoVuGRo67130698;     kivLoVuGRo67130698 = kivLoVuGRo88666810;     kivLoVuGRo88666810 = kivLoVuGRo75768316;     kivLoVuGRo75768316 = kivLoVuGRo83316594;     kivLoVuGRo83316594 = kivLoVuGRo65072460;     kivLoVuGRo65072460 = kivLoVuGRo84797003;     kivLoVuGRo84797003 = kivLoVuGRo74267650;     kivLoVuGRo74267650 = kivLoVuGRo94228370;     kivLoVuGRo94228370 = kivLoVuGRo51384961;     kivLoVuGRo51384961 = kivLoVuGRo60948145;     kivLoVuGRo60948145 = kivLoVuGRo39637422;     kivLoVuGRo39637422 = kivLoVuGRo48364268;     kivLoVuGRo48364268 = kivLoVuGRo90478657;     kivLoVuGRo90478657 = kivLoVuGRo90624176;     kivLoVuGRo90624176 = kivLoVuGRo53680908;     kivLoVuGRo53680908 = kivLoVuGRo87855276;     kivLoVuGRo87855276 = kivLoVuGRo69317016;     kivLoVuGRo69317016 = kivLoVuGRo68133303;     kivLoVuGRo68133303 = kivLoVuGRo55665908;     kivLoVuGRo55665908 = kivLoVuGRo40477114;     kivLoVuGRo40477114 = kivLoVuGRo74844603;     kivLoVuGRo74844603 = kivLoVuGRo1233730;     kivLoVuGRo1233730 = kivLoVuGRo99066276;     kivLoVuGRo99066276 = kivLoVuGRo6428172;     kivLoVuGRo6428172 = kivLoVuGRo56300066;     kivLoVuGRo56300066 = kivLoVuGRo9210412;     kivLoVuGRo9210412 = kivLoVuGRo57813241;     kivLoVuGRo57813241 = kivLoVuGRo13980248;     kivLoVuGRo13980248 = kivLoVuGRo78742629;     kivLoVuGRo78742629 = kivLoVuGRo40807641;     kivLoVuGRo40807641 = kivLoVuGRo28388440;     kivLoVuGRo28388440 = kivLoVuGRo79597623;     kivLoVuGRo79597623 = kivLoVuGRo36681394;     kivLoVuGRo36681394 = kivLoVuGRo48500634;     kivLoVuGRo48500634 = kivLoVuGRo57474213;     kivLoVuGRo57474213 = kivLoVuGRo63928464;     kivLoVuGRo63928464 = kivLoVuGRo49631566;     kivLoVuGRo49631566 = kivLoVuGRo41917168;     kivLoVuGRo41917168 = kivLoVuGRo10170337;     kivLoVuGRo10170337 = kivLoVuGRo41168577;     kivLoVuGRo41168577 = kivLoVuGRo86733616;     kivLoVuGRo86733616 = kivLoVuGRo14947345;     kivLoVuGRo14947345 = kivLoVuGRo39153818;     kivLoVuGRo39153818 = kivLoVuGRo38055152;     kivLoVuGRo38055152 = kivLoVuGRo14633062;     kivLoVuGRo14633062 = kivLoVuGRo54278767;     kivLoVuGRo54278767 = kivLoVuGRo74408076;     kivLoVuGRo74408076 = kivLoVuGRo21524429;     kivLoVuGRo21524429 = kivLoVuGRo49686795;     kivLoVuGRo49686795 = kivLoVuGRo28103107;     kivLoVuGRo28103107 = kivLoVuGRo27306871;     kivLoVuGRo27306871 = kivLoVuGRo62473101;     kivLoVuGRo62473101 = kivLoVuGRo92934337;     kivLoVuGRo92934337 = kivLoVuGRo49281993;     kivLoVuGRo49281993 = kivLoVuGRo97660351;     kivLoVuGRo97660351 = kivLoVuGRo65896968;     kivLoVuGRo65896968 = kivLoVuGRo89600533;     kivLoVuGRo89600533 = kivLoVuGRo69340145;     kivLoVuGRo69340145 = kivLoVuGRo27016528;     kivLoVuGRo27016528 = kivLoVuGRo55862049;     kivLoVuGRo55862049 = kivLoVuGRo26983762;     kivLoVuGRo26983762 = kivLoVuGRo60287402;     kivLoVuGRo60287402 = kivLoVuGRo15485741;     kivLoVuGRo15485741 = kivLoVuGRo10577320;     kivLoVuGRo10577320 = kivLoVuGRo32559705;     kivLoVuGRo32559705 = kivLoVuGRo60039799;     kivLoVuGRo60039799 = kivLoVuGRo11682874;     kivLoVuGRo11682874 = kivLoVuGRo41978023;     kivLoVuGRo41978023 = kivLoVuGRo33149964;     kivLoVuGRo33149964 = kivLoVuGRo89752443;     kivLoVuGRo89752443 = kivLoVuGRo38223711;     kivLoVuGRo38223711 = kivLoVuGRo27399848;     kivLoVuGRo27399848 = kivLoVuGRo57962967;     kivLoVuGRo57962967 = kivLoVuGRo14497331;     kivLoVuGRo14497331 = kivLoVuGRo53743497;     kivLoVuGRo53743497 = kivLoVuGRo59897259;     kivLoVuGRo59897259 = kivLoVuGRo62079912;     kivLoVuGRo62079912 = kivLoVuGRo61011124;     kivLoVuGRo61011124 = kivLoVuGRo91795109;     kivLoVuGRo91795109 = kivLoVuGRo2021299;     kivLoVuGRo2021299 = kivLoVuGRo34802336;     kivLoVuGRo34802336 = kivLoVuGRo36288813;     kivLoVuGRo36288813 = kivLoVuGRo64293453;     kivLoVuGRo64293453 = kivLoVuGRo50639523;     kivLoVuGRo50639523 = kivLoVuGRo13500770;     kivLoVuGRo13500770 = kivLoVuGRo65915339;     kivLoVuGRo65915339 = kivLoVuGRo86663285;     kivLoVuGRo86663285 = kivLoVuGRo87399401;     kivLoVuGRo87399401 = kivLoVuGRo50840283;     kivLoVuGRo50840283 = kivLoVuGRo91577244;     kivLoVuGRo91577244 = kivLoVuGRo74327930;     kivLoVuGRo74327930 = kivLoVuGRo64886733;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CkJyzRLLoC39288010() {     float VffrgDGdEK87273850 = -851198340;    float VffrgDGdEK96496105 = -784636008;    float VffrgDGdEK32136982 = -642262677;    float VffrgDGdEK30965072 = -850374565;    float VffrgDGdEK94798141 = -200148644;    float VffrgDGdEK45489522 = -866530734;    float VffrgDGdEK40755984 = -507645833;    float VffrgDGdEK65392966 = -310146727;    float VffrgDGdEK69668085 = -283670150;    float VffrgDGdEK58894282 = -274186506;    float VffrgDGdEK26208670 = -639893816;    float VffrgDGdEK31671266 = -278626186;    float VffrgDGdEK50324774 = -959439455;    float VffrgDGdEK7687213 = -539035396;    float VffrgDGdEK21532280 = -628500459;    float VffrgDGdEK18523563 = -232056067;    float VffrgDGdEK44051722 = -414472747;    float VffrgDGdEK57361377 = -400819240;    float VffrgDGdEK2229777 = -719708113;    float VffrgDGdEK18153705 = -110646838;    float VffrgDGdEK49247375 = -677003801;    float VffrgDGdEK54293875 = -651770923;    float VffrgDGdEK93917499 = -901133074;    float VffrgDGdEK80180695 = -232336012;    float VffrgDGdEK23395523 = -723914690;    float VffrgDGdEK57620011 = -387960780;    float VffrgDGdEK15222724 = -44278097;    float VffrgDGdEK61577358 = -820612400;    float VffrgDGdEK47369675 = -611169094;    float VffrgDGdEK84001783 = -113154218;    float VffrgDGdEK45524535 = -814197949;    float VffrgDGdEK56607467 = -649556221;    float VffrgDGdEK87466841 = 27956417;    float VffrgDGdEK15688754 = -54457205;    float VffrgDGdEK85439776 = -426501230;    float VffrgDGdEK99506110 = -77904097;    float VffrgDGdEK7419960 = -692746485;    float VffrgDGdEK32368731 = -864187798;    float VffrgDGdEK89115772 = -775919;    float VffrgDGdEK48574155 = -471431826;    float VffrgDGdEK8354994 = -34640107;    float VffrgDGdEK25189785 = -147236245;    float VffrgDGdEK54050826 = -478310266;    float VffrgDGdEK56918332 = -343684192;    float VffrgDGdEK31911731 = -783048372;    float VffrgDGdEK57731847 = -655171261;    float VffrgDGdEK54214566 = -430460136;    float VffrgDGdEK54769011 = -88173604;    float VffrgDGdEK35628482 = -800185487;    float VffrgDGdEK83817362 = -466563004;    float VffrgDGdEK23816561 = -164587214;    float VffrgDGdEK13005075 = -420349322;    float VffrgDGdEK16369556 = -528062128;    float VffrgDGdEK74919606 = -310318735;    float VffrgDGdEK90040635 = -637580389;    float VffrgDGdEK32979975 = -99427417;    float VffrgDGdEK2578606 = -883502934;    float VffrgDGdEK51956286 = -309926665;    float VffrgDGdEK7569549 = -26459875;    float VffrgDGdEK37178131 = -812187864;    float VffrgDGdEK30266798 = -722252638;    float VffrgDGdEK79178626 = -687033434;    float VffrgDGdEK18023291 = -698977633;    float VffrgDGdEK85666302 = -70515933;    float VffrgDGdEK13369748 = -459988557;    float VffrgDGdEK69601202 = -990337596;    float VffrgDGdEK44204424 = -206582604;    float VffrgDGdEK34636021 = -804982251;    float VffrgDGdEK22247437 = -12534166;    float VffrgDGdEK22026170 = -450596363;    float VffrgDGdEK11103603 = -539309582;    float VffrgDGdEK11682992 = -550284949;    float VffrgDGdEK68245605 = -300043322;    float VffrgDGdEK53655621 = -148276288;    float VffrgDGdEK9798712 = 23993268;    float VffrgDGdEK24057590 = -429767557;    float VffrgDGdEK243050 = -73460658;    float VffrgDGdEK36999168 = -457448883;    float VffrgDGdEK48268964 = -449287641;    float VffrgDGdEK65663675 = 31256570;    float VffrgDGdEK3405446 = -957500645;    float VffrgDGdEK60453713 = -956104494;    float VffrgDGdEK25948876 = 79573086;    float VffrgDGdEK63552312 = -44606091;    float VffrgDGdEK60185222 = -948567004;    float VffrgDGdEK32519460 = -293848627;    float VffrgDGdEK40237912 = -21494093;    float VffrgDGdEK12547235 = -661724849;    float VffrgDGdEK25648119 = -416876816;    float VffrgDGdEK52459801 = -227073814;    float VffrgDGdEK96927504 = -194401163;    float VffrgDGdEK55463674 = -282819820;    float VffrgDGdEK24799182 = -737727924;    float VffrgDGdEK51937641 = -188588055;    float VffrgDGdEK18307358 = -749179188;    float VffrgDGdEK29176367 = -347606674;    float VffrgDGdEK7166495 = -448258612;    float VffrgDGdEK68384523 = -307794333;    float VffrgDGdEK43548584 = -883695635;    float VffrgDGdEK62310528 = -851198340;     VffrgDGdEK87273850 = VffrgDGdEK96496105;     VffrgDGdEK96496105 = VffrgDGdEK32136982;     VffrgDGdEK32136982 = VffrgDGdEK30965072;     VffrgDGdEK30965072 = VffrgDGdEK94798141;     VffrgDGdEK94798141 = VffrgDGdEK45489522;     VffrgDGdEK45489522 = VffrgDGdEK40755984;     VffrgDGdEK40755984 = VffrgDGdEK65392966;     VffrgDGdEK65392966 = VffrgDGdEK69668085;     VffrgDGdEK69668085 = VffrgDGdEK58894282;     VffrgDGdEK58894282 = VffrgDGdEK26208670;     VffrgDGdEK26208670 = VffrgDGdEK31671266;     VffrgDGdEK31671266 = VffrgDGdEK50324774;     VffrgDGdEK50324774 = VffrgDGdEK7687213;     VffrgDGdEK7687213 = VffrgDGdEK21532280;     VffrgDGdEK21532280 = VffrgDGdEK18523563;     VffrgDGdEK18523563 = VffrgDGdEK44051722;     VffrgDGdEK44051722 = VffrgDGdEK57361377;     VffrgDGdEK57361377 = VffrgDGdEK2229777;     VffrgDGdEK2229777 = VffrgDGdEK18153705;     VffrgDGdEK18153705 = VffrgDGdEK49247375;     VffrgDGdEK49247375 = VffrgDGdEK54293875;     VffrgDGdEK54293875 = VffrgDGdEK93917499;     VffrgDGdEK93917499 = VffrgDGdEK80180695;     VffrgDGdEK80180695 = VffrgDGdEK23395523;     VffrgDGdEK23395523 = VffrgDGdEK57620011;     VffrgDGdEK57620011 = VffrgDGdEK15222724;     VffrgDGdEK15222724 = VffrgDGdEK61577358;     VffrgDGdEK61577358 = VffrgDGdEK47369675;     VffrgDGdEK47369675 = VffrgDGdEK84001783;     VffrgDGdEK84001783 = VffrgDGdEK45524535;     VffrgDGdEK45524535 = VffrgDGdEK56607467;     VffrgDGdEK56607467 = VffrgDGdEK87466841;     VffrgDGdEK87466841 = VffrgDGdEK15688754;     VffrgDGdEK15688754 = VffrgDGdEK85439776;     VffrgDGdEK85439776 = VffrgDGdEK99506110;     VffrgDGdEK99506110 = VffrgDGdEK7419960;     VffrgDGdEK7419960 = VffrgDGdEK32368731;     VffrgDGdEK32368731 = VffrgDGdEK89115772;     VffrgDGdEK89115772 = VffrgDGdEK48574155;     VffrgDGdEK48574155 = VffrgDGdEK8354994;     VffrgDGdEK8354994 = VffrgDGdEK25189785;     VffrgDGdEK25189785 = VffrgDGdEK54050826;     VffrgDGdEK54050826 = VffrgDGdEK56918332;     VffrgDGdEK56918332 = VffrgDGdEK31911731;     VffrgDGdEK31911731 = VffrgDGdEK57731847;     VffrgDGdEK57731847 = VffrgDGdEK54214566;     VffrgDGdEK54214566 = VffrgDGdEK54769011;     VffrgDGdEK54769011 = VffrgDGdEK35628482;     VffrgDGdEK35628482 = VffrgDGdEK83817362;     VffrgDGdEK83817362 = VffrgDGdEK23816561;     VffrgDGdEK23816561 = VffrgDGdEK13005075;     VffrgDGdEK13005075 = VffrgDGdEK16369556;     VffrgDGdEK16369556 = VffrgDGdEK74919606;     VffrgDGdEK74919606 = VffrgDGdEK90040635;     VffrgDGdEK90040635 = VffrgDGdEK32979975;     VffrgDGdEK32979975 = VffrgDGdEK2578606;     VffrgDGdEK2578606 = VffrgDGdEK51956286;     VffrgDGdEK51956286 = VffrgDGdEK7569549;     VffrgDGdEK7569549 = VffrgDGdEK37178131;     VffrgDGdEK37178131 = VffrgDGdEK30266798;     VffrgDGdEK30266798 = VffrgDGdEK79178626;     VffrgDGdEK79178626 = VffrgDGdEK18023291;     VffrgDGdEK18023291 = VffrgDGdEK85666302;     VffrgDGdEK85666302 = VffrgDGdEK13369748;     VffrgDGdEK13369748 = VffrgDGdEK69601202;     VffrgDGdEK69601202 = VffrgDGdEK44204424;     VffrgDGdEK44204424 = VffrgDGdEK34636021;     VffrgDGdEK34636021 = VffrgDGdEK22247437;     VffrgDGdEK22247437 = VffrgDGdEK22026170;     VffrgDGdEK22026170 = VffrgDGdEK11103603;     VffrgDGdEK11103603 = VffrgDGdEK11682992;     VffrgDGdEK11682992 = VffrgDGdEK68245605;     VffrgDGdEK68245605 = VffrgDGdEK53655621;     VffrgDGdEK53655621 = VffrgDGdEK9798712;     VffrgDGdEK9798712 = VffrgDGdEK24057590;     VffrgDGdEK24057590 = VffrgDGdEK243050;     VffrgDGdEK243050 = VffrgDGdEK36999168;     VffrgDGdEK36999168 = VffrgDGdEK48268964;     VffrgDGdEK48268964 = VffrgDGdEK65663675;     VffrgDGdEK65663675 = VffrgDGdEK3405446;     VffrgDGdEK3405446 = VffrgDGdEK60453713;     VffrgDGdEK60453713 = VffrgDGdEK25948876;     VffrgDGdEK25948876 = VffrgDGdEK63552312;     VffrgDGdEK63552312 = VffrgDGdEK60185222;     VffrgDGdEK60185222 = VffrgDGdEK32519460;     VffrgDGdEK32519460 = VffrgDGdEK40237912;     VffrgDGdEK40237912 = VffrgDGdEK12547235;     VffrgDGdEK12547235 = VffrgDGdEK25648119;     VffrgDGdEK25648119 = VffrgDGdEK52459801;     VffrgDGdEK52459801 = VffrgDGdEK96927504;     VffrgDGdEK96927504 = VffrgDGdEK55463674;     VffrgDGdEK55463674 = VffrgDGdEK24799182;     VffrgDGdEK24799182 = VffrgDGdEK51937641;     VffrgDGdEK51937641 = VffrgDGdEK18307358;     VffrgDGdEK18307358 = VffrgDGdEK29176367;     VffrgDGdEK29176367 = VffrgDGdEK7166495;     VffrgDGdEK7166495 = VffrgDGdEK68384523;     VffrgDGdEK68384523 = VffrgDGdEK43548584;     VffrgDGdEK43548584 = VffrgDGdEK62310528;     VffrgDGdEK62310528 = VffrgDGdEK87273850;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void aKBRxlZmfj45286360() {     float tSQUaoWpyO96589116 = -753729366;    float tSQUaoWpyO51919027 = -801170510;    float tSQUaoWpyO52289062 = -355903290;    float tSQUaoWpyO88755093 = 57961443;    float tSQUaoWpyO34861381 = -176663801;    float tSQUaoWpyO39032884 = -325399595;    float tSQUaoWpyO14410056 = -306894711;    float tSQUaoWpyO54529122 = -507105875;    float tSQUaoWpyO62476742 = -663730822;    float tSQUaoWpyO27518740 = -94982174;    float tSQUaoWpyO63110654 = -831134889;    float tSQUaoWpyO42909015 = -915996001;    float tSQUaoWpyO46106490 = -544861590;    float tSQUaoWpyO39775854 = -518089500;    float tSQUaoWpyO36808318 = -480030870;    float tSQUaoWpyO23494087 = -259579545;    float tSQUaoWpyO87862489 = -442057834;    float tSQUaoWpyO69554018 = -130876553;    float tSQUaoWpyO84242708 = -673348942;    float tSQUaoWpyO81349053 = -189354680;    float tSQUaoWpyO37926727 = 97296271;    float tSQUaoWpyO78502787 = -474667256;    float tSQUaoWpyO29978766 = -515662074;    float tSQUaoWpyO39387324 = -849497122;    float tSQUaoWpyO86458065 = 56952259;    float tSQUaoWpyO23527829 = -487172421;    float tSQUaoWpyO48980991 = -853166942;    float tSQUaoWpyO32671833 = -227749133;    float tSQUaoWpyO14486582 = -725588530;    float tSQUaoWpyO96782731 = 80761220;    float tSQUaoWpyO15522914 = -424849766;    float tSQUaoWpyO33866831 = 19272484;    float tSQUaoWpyO37326172 = -883134595;    float tSQUaoWpyO4717702 = -638017092;    float tSQUaoWpyO72090051 = -602913825;    float tSQUaoWpyO31331005 = -494129544;    float tSQUaoWpyO44887568 = -293946348;    float tSQUaoWpyO88405529 = -616783161;    float tSQUaoWpyO55341473 = -51991706;    float tSQUaoWpyO24674052 = -178617338;    float tSQUaoWpyO32418128 = -171796525;    float tSQUaoWpyO12841441 = -68508336;    float tSQUaoWpyO75767832 = -700190647;    float tSQUaoWpyO15675233 = -834079080;    float tSQUaoWpyO67861805 = -574824166;    float tSQUaoWpyO27971083 = -980705819;    float tSQUaoWpyO59464197 = -3294281;    float tSQUaoWpyO26240049 = -534716992;    float tSQUaoWpyO1655626 = 64117997;    float tSQUaoWpyO53455264 = -259576707;    float tSQUaoWpyO60784270 = 99536789;    float tSQUaoWpyO77222116 = 31664460;    float tSQUaoWpyO4377214 = -115839034;    float tSQUaoWpyO79500850 = -670171852;    float tSQUaoWpyO7359634 = -572498474;    float tSQUaoWpyO18086330 = -179062111;    float tSQUaoWpyO21940262 = -185508437;    float tSQUaoWpyO12901738 = -506406168;    float tSQUaoWpyO2297029 = -998990817;    float tSQUaoWpyO11333553 = -689491381;    float tSQUaoWpyO90051892 = -472232653;    float tSQUaoWpyO81738222 = 20854421;    float tSQUaoWpyO40042540 = -781517345;    float tSQUaoWpyO65694011 = -644492042;    float tSQUaoWpyO11995826 = -670132408;    float tSQUaoWpyO29243824 = -750407374;    float tSQUaoWpyO5582844 = 67138594;    float tSQUaoWpyO41388789 = -906844499;    float tSQUaoWpyO67685802 = -915175676;    float tSQUaoWpyO5477313 = -985901327;    float tSQUaoWpyO78606519 = -965633198;    float tSQUaoWpyO99456960 = -825274673;    float tSQUaoWpyO14212545 = 21115152;    float tSQUaoWpyO59568656 = -394731604;    float tSQUaoWpyO48930925 = 82441845;    float tSQUaoWpyO25085286 = -834195394;    float tSQUaoWpyO2734956 = -774476609;    float tSQUaoWpyO14303534 = -681582995;    float tSQUaoWpyO71525519 = -174672957;    float tSQUaoWpyO58486983 = 37658078;    float tSQUaoWpyO64063631 = -383878141;    float tSQUaoWpyO22740942 = -218449950;    float tSQUaoWpyO31016208 = -191867130;    float tSQUaoWpyO61031317 = -366011824;    float tSQUaoWpyO35998461 = 81224430;    float tSQUaoWpyO38300797 = -356514227;    float tSQUaoWpyO29489618 = -864888482;    float tSQUaoWpyO57825321 = -112962743;    float tSQUaoWpyO97358068 = 34481382;    float tSQUaoWpyO54003722 = -323851715;    float tSQUaoWpyO9390744 = -208621107;    float tSQUaoWpyO31985830 = -787540180;    float tSQUaoWpyO86108500 = -617792345;    float tSQUaoWpyO44007921 = -362500326;    float tSQUaoWpyO34622159 = -706384685;    float tSQUaoWpyO50679905 = -92650946;    float tSQUaoWpyO72798900 = -286990991;    float tSQUaoWpyO10073821 = 44301394;    float tSQUaoWpyO3679407 = -63946672;    float tSQUaoWpyO38617982 = -753729366;     tSQUaoWpyO96589116 = tSQUaoWpyO51919027;     tSQUaoWpyO51919027 = tSQUaoWpyO52289062;     tSQUaoWpyO52289062 = tSQUaoWpyO88755093;     tSQUaoWpyO88755093 = tSQUaoWpyO34861381;     tSQUaoWpyO34861381 = tSQUaoWpyO39032884;     tSQUaoWpyO39032884 = tSQUaoWpyO14410056;     tSQUaoWpyO14410056 = tSQUaoWpyO54529122;     tSQUaoWpyO54529122 = tSQUaoWpyO62476742;     tSQUaoWpyO62476742 = tSQUaoWpyO27518740;     tSQUaoWpyO27518740 = tSQUaoWpyO63110654;     tSQUaoWpyO63110654 = tSQUaoWpyO42909015;     tSQUaoWpyO42909015 = tSQUaoWpyO46106490;     tSQUaoWpyO46106490 = tSQUaoWpyO39775854;     tSQUaoWpyO39775854 = tSQUaoWpyO36808318;     tSQUaoWpyO36808318 = tSQUaoWpyO23494087;     tSQUaoWpyO23494087 = tSQUaoWpyO87862489;     tSQUaoWpyO87862489 = tSQUaoWpyO69554018;     tSQUaoWpyO69554018 = tSQUaoWpyO84242708;     tSQUaoWpyO84242708 = tSQUaoWpyO81349053;     tSQUaoWpyO81349053 = tSQUaoWpyO37926727;     tSQUaoWpyO37926727 = tSQUaoWpyO78502787;     tSQUaoWpyO78502787 = tSQUaoWpyO29978766;     tSQUaoWpyO29978766 = tSQUaoWpyO39387324;     tSQUaoWpyO39387324 = tSQUaoWpyO86458065;     tSQUaoWpyO86458065 = tSQUaoWpyO23527829;     tSQUaoWpyO23527829 = tSQUaoWpyO48980991;     tSQUaoWpyO48980991 = tSQUaoWpyO32671833;     tSQUaoWpyO32671833 = tSQUaoWpyO14486582;     tSQUaoWpyO14486582 = tSQUaoWpyO96782731;     tSQUaoWpyO96782731 = tSQUaoWpyO15522914;     tSQUaoWpyO15522914 = tSQUaoWpyO33866831;     tSQUaoWpyO33866831 = tSQUaoWpyO37326172;     tSQUaoWpyO37326172 = tSQUaoWpyO4717702;     tSQUaoWpyO4717702 = tSQUaoWpyO72090051;     tSQUaoWpyO72090051 = tSQUaoWpyO31331005;     tSQUaoWpyO31331005 = tSQUaoWpyO44887568;     tSQUaoWpyO44887568 = tSQUaoWpyO88405529;     tSQUaoWpyO88405529 = tSQUaoWpyO55341473;     tSQUaoWpyO55341473 = tSQUaoWpyO24674052;     tSQUaoWpyO24674052 = tSQUaoWpyO32418128;     tSQUaoWpyO32418128 = tSQUaoWpyO12841441;     tSQUaoWpyO12841441 = tSQUaoWpyO75767832;     tSQUaoWpyO75767832 = tSQUaoWpyO15675233;     tSQUaoWpyO15675233 = tSQUaoWpyO67861805;     tSQUaoWpyO67861805 = tSQUaoWpyO27971083;     tSQUaoWpyO27971083 = tSQUaoWpyO59464197;     tSQUaoWpyO59464197 = tSQUaoWpyO26240049;     tSQUaoWpyO26240049 = tSQUaoWpyO1655626;     tSQUaoWpyO1655626 = tSQUaoWpyO53455264;     tSQUaoWpyO53455264 = tSQUaoWpyO60784270;     tSQUaoWpyO60784270 = tSQUaoWpyO77222116;     tSQUaoWpyO77222116 = tSQUaoWpyO4377214;     tSQUaoWpyO4377214 = tSQUaoWpyO79500850;     tSQUaoWpyO79500850 = tSQUaoWpyO7359634;     tSQUaoWpyO7359634 = tSQUaoWpyO18086330;     tSQUaoWpyO18086330 = tSQUaoWpyO21940262;     tSQUaoWpyO21940262 = tSQUaoWpyO12901738;     tSQUaoWpyO12901738 = tSQUaoWpyO2297029;     tSQUaoWpyO2297029 = tSQUaoWpyO11333553;     tSQUaoWpyO11333553 = tSQUaoWpyO90051892;     tSQUaoWpyO90051892 = tSQUaoWpyO81738222;     tSQUaoWpyO81738222 = tSQUaoWpyO40042540;     tSQUaoWpyO40042540 = tSQUaoWpyO65694011;     tSQUaoWpyO65694011 = tSQUaoWpyO11995826;     tSQUaoWpyO11995826 = tSQUaoWpyO29243824;     tSQUaoWpyO29243824 = tSQUaoWpyO5582844;     tSQUaoWpyO5582844 = tSQUaoWpyO41388789;     tSQUaoWpyO41388789 = tSQUaoWpyO67685802;     tSQUaoWpyO67685802 = tSQUaoWpyO5477313;     tSQUaoWpyO5477313 = tSQUaoWpyO78606519;     tSQUaoWpyO78606519 = tSQUaoWpyO99456960;     tSQUaoWpyO99456960 = tSQUaoWpyO14212545;     tSQUaoWpyO14212545 = tSQUaoWpyO59568656;     tSQUaoWpyO59568656 = tSQUaoWpyO48930925;     tSQUaoWpyO48930925 = tSQUaoWpyO25085286;     tSQUaoWpyO25085286 = tSQUaoWpyO2734956;     tSQUaoWpyO2734956 = tSQUaoWpyO14303534;     tSQUaoWpyO14303534 = tSQUaoWpyO71525519;     tSQUaoWpyO71525519 = tSQUaoWpyO58486983;     tSQUaoWpyO58486983 = tSQUaoWpyO64063631;     tSQUaoWpyO64063631 = tSQUaoWpyO22740942;     tSQUaoWpyO22740942 = tSQUaoWpyO31016208;     tSQUaoWpyO31016208 = tSQUaoWpyO61031317;     tSQUaoWpyO61031317 = tSQUaoWpyO35998461;     tSQUaoWpyO35998461 = tSQUaoWpyO38300797;     tSQUaoWpyO38300797 = tSQUaoWpyO29489618;     tSQUaoWpyO29489618 = tSQUaoWpyO57825321;     tSQUaoWpyO57825321 = tSQUaoWpyO97358068;     tSQUaoWpyO97358068 = tSQUaoWpyO54003722;     tSQUaoWpyO54003722 = tSQUaoWpyO9390744;     tSQUaoWpyO9390744 = tSQUaoWpyO31985830;     tSQUaoWpyO31985830 = tSQUaoWpyO86108500;     tSQUaoWpyO86108500 = tSQUaoWpyO44007921;     tSQUaoWpyO44007921 = tSQUaoWpyO34622159;     tSQUaoWpyO34622159 = tSQUaoWpyO50679905;     tSQUaoWpyO50679905 = tSQUaoWpyO72798900;     tSQUaoWpyO72798900 = tSQUaoWpyO10073821;     tSQUaoWpyO10073821 = tSQUaoWpyO3679407;     tSQUaoWpyO3679407 = tSQUaoWpyO38617982;     tSQUaoWpyO38617982 = tSQUaoWpyO96589116;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ZCZBJqfVqe81931334() {     long fUvollddIj18976234 = -388133329;    long fUvollddIj36266528 = -749713565;    long fUvollddIj81058340 = -595557134;    long fUvollddIj3761783 = 51553911;    long fUvollddIj33035635 = -767720803;    long fUvollddIj53916000 = -186007973;    long fUvollddIj6565795 = -128882018;    long fUvollddIj30162981 = -977702519;    long fUvollddIj59639873 = -547926861;    long fUvollddIj19282324 = -937723506;    long fUvollddIj652514 = -827759290;    long fUvollddIj98811963 = -66741295;    long fUvollddIj13114671 = -63578910;    long fUvollddIj82390606 = 54201743;    long fUvollddIj73543594 = -107508583;    long fUvollddIj67749999 = -641247087;    long fUvollddIj37685842 = -719337654;    long fUvollddIj75530434 = -582619244;    long fUvollddIj25524339 = -808451067;    long fUvollddIj59865336 = -441989133;    long fUvollddIj38809834 = -994191038;    long fUvollddIj42318006 = -979433569;    long fUvollddIj33272090 = -691742666;    long fUvollddIj65887111 = -313489980;    long fUvollddIj21998313 = -664606866;    long fUvollddIj11830823 = -931066645;    long fUvollddIj96070411 = -610753435;    long fUvollddIj38583283 = -16203586;    long fUvollddIj21379143 = 30154351;    long fUvollddIj5939911 = -939534647;    long fUvollddIj59813719 = -506165918;    long fUvollddIj91408021 = -632798870;    long fUvollddIj18364842 = -312478090;    long fUvollddIj64106389 = -962143942;    long fUvollddIj48319416 = -666670247;    long fUvollddIj73023874 = -809524921;    long fUvollddIj38327280 = -26894299;    long fUvollddIj42031630 = -484408633;    long fUvollddIj3649605 = -453037738;    long fUvollddIj44859766 = -314841368;    long fUvollddIj61175499 = -674974191;    long fUvollddIj1349833 = -926417027;    long fUvollddIj81318024 = -587721231;    long fUvollddIj15119352 = -831442729;    long fUvollddIj35845072 = -383967643;    long fUvollddIj36071365 = -62934915;    long fUvollddIj71761594 = -581704534;    long fUvollddIj70838723 = -291251983;    long fUvollddIj96115530 = -985598702;    long fUvollddIj50539010 = -401596966;    long fUvollddIj69653487 = -255539488;    long fUvollddIj51073374 = -399154822;    long fUvollddIj82691616 = -887750264;    long fUvollddIj39787396 = -945852661;    long fUvollddIj43121501 = -713202589;    long fUvollddIj76658228 = -408699760;    long fUvollddIj2994439 = 42029100;    long fUvollddIj15171230 = -182067155;    long fUvollddIj81763469 = -283839224;    long fUvollddIj21204812 = -836654158;    long fUvollddIj57845589 = -575254539;    long fUvollddIj67982511 = -12678432;    long fUvollddIj8783838 = -907856870;    long fUvollddIj53699963 = -608392214;    long fUvollddIj59468605 = -331557589;    long fUvollddIj9244493 = -94960420;    long fUvollddIj80447122 = -754263205;    long fUvollddIj49008282 = -101434968;    long fUvollddIj34071190 = -279128011;    long fUvollddIj519721 = -297983663;    long fUvollddIj29422720 = -514352789;    long fUvollddIj95654211 = -134929021;    long fUvollddIj71880830 = -29581507;    long fUvollddIj80664572 = -393609700;    long fUvollddIj98689837 = -767014943;    long fUvollddIj37460002 = 32225988;    long fUvollddIj60999982 = -291712339;    long fUvollddIj18152738 = -860299938;    long fUvollddIj30042040 = -929522337;    long fUvollddIj85926947 = -501671951;    long fUvollddIj40069229 = -249362112;    long fUvollddIj25231688 = -219501452;    long fUvollddIj42467753 = -30604885;    long fUvollddIj70840133 = -568248684;    long fUvollddIj36286424 = -583995160;    long fUvollddIj8740345 = -7011097;    long fUvollddIj8716405 = -745048607;    long fUvollddIj78577446 = -366625430;    long fUvollddIj20984888 = -148941353;    long fUvollddIj71661188 = -157970487;    long fUvollddIj70029435 = -751554021;    long fUvollddIj23156051 = -844827144;    long fUvollddIj60268160 = -100569410;    long fUvollddIj82444792 = -616383580;    long fUvollddIj87014177 = -739586830;    long fUvollddIj93192987 = -562295760;    long fUvollddIj92565994 = 81439843;    long fUvollddIj27618061 = -979329017;    long fUvollddIj55650746 = -399885140;    long fUvollddIj26600580 = -388133329;     fUvollddIj18976234 = fUvollddIj36266528;     fUvollddIj36266528 = fUvollddIj81058340;     fUvollddIj81058340 = fUvollddIj3761783;     fUvollddIj3761783 = fUvollddIj33035635;     fUvollddIj33035635 = fUvollddIj53916000;     fUvollddIj53916000 = fUvollddIj6565795;     fUvollddIj6565795 = fUvollddIj30162981;     fUvollddIj30162981 = fUvollddIj59639873;     fUvollddIj59639873 = fUvollddIj19282324;     fUvollddIj19282324 = fUvollddIj652514;     fUvollddIj652514 = fUvollddIj98811963;     fUvollddIj98811963 = fUvollddIj13114671;     fUvollddIj13114671 = fUvollddIj82390606;     fUvollddIj82390606 = fUvollddIj73543594;     fUvollddIj73543594 = fUvollddIj67749999;     fUvollddIj67749999 = fUvollddIj37685842;     fUvollddIj37685842 = fUvollddIj75530434;     fUvollddIj75530434 = fUvollddIj25524339;     fUvollddIj25524339 = fUvollddIj59865336;     fUvollddIj59865336 = fUvollddIj38809834;     fUvollddIj38809834 = fUvollddIj42318006;     fUvollddIj42318006 = fUvollddIj33272090;     fUvollddIj33272090 = fUvollddIj65887111;     fUvollddIj65887111 = fUvollddIj21998313;     fUvollddIj21998313 = fUvollddIj11830823;     fUvollddIj11830823 = fUvollddIj96070411;     fUvollddIj96070411 = fUvollddIj38583283;     fUvollddIj38583283 = fUvollddIj21379143;     fUvollddIj21379143 = fUvollddIj5939911;     fUvollddIj5939911 = fUvollddIj59813719;     fUvollddIj59813719 = fUvollddIj91408021;     fUvollddIj91408021 = fUvollddIj18364842;     fUvollddIj18364842 = fUvollddIj64106389;     fUvollddIj64106389 = fUvollddIj48319416;     fUvollddIj48319416 = fUvollddIj73023874;     fUvollddIj73023874 = fUvollddIj38327280;     fUvollddIj38327280 = fUvollddIj42031630;     fUvollddIj42031630 = fUvollddIj3649605;     fUvollddIj3649605 = fUvollddIj44859766;     fUvollddIj44859766 = fUvollddIj61175499;     fUvollddIj61175499 = fUvollddIj1349833;     fUvollddIj1349833 = fUvollddIj81318024;     fUvollddIj81318024 = fUvollddIj15119352;     fUvollddIj15119352 = fUvollddIj35845072;     fUvollddIj35845072 = fUvollddIj36071365;     fUvollddIj36071365 = fUvollddIj71761594;     fUvollddIj71761594 = fUvollddIj70838723;     fUvollddIj70838723 = fUvollddIj96115530;     fUvollddIj96115530 = fUvollddIj50539010;     fUvollddIj50539010 = fUvollddIj69653487;     fUvollddIj69653487 = fUvollddIj51073374;     fUvollddIj51073374 = fUvollddIj82691616;     fUvollddIj82691616 = fUvollddIj39787396;     fUvollddIj39787396 = fUvollddIj43121501;     fUvollddIj43121501 = fUvollddIj76658228;     fUvollddIj76658228 = fUvollddIj2994439;     fUvollddIj2994439 = fUvollddIj15171230;     fUvollddIj15171230 = fUvollddIj81763469;     fUvollddIj81763469 = fUvollddIj21204812;     fUvollddIj21204812 = fUvollddIj57845589;     fUvollddIj57845589 = fUvollddIj67982511;     fUvollddIj67982511 = fUvollddIj8783838;     fUvollddIj8783838 = fUvollddIj53699963;     fUvollddIj53699963 = fUvollddIj59468605;     fUvollddIj59468605 = fUvollddIj9244493;     fUvollddIj9244493 = fUvollddIj80447122;     fUvollddIj80447122 = fUvollddIj49008282;     fUvollddIj49008282 = fUvollddIj34071190;     fUvollddIj34071190 = fUvollddIj519721;     fUvollddIj519721 = fUvollddIj29422720;     fUvollddIj29422720 = fUvollddIj95654211;     fUvollddIj95654211 = fUvollddIj71880830;     fUvollddIj71880830 = fUvollddIj80664572;     fUvollddIj80664572 = fUvollddIj98689837;     fUvollddIj98689837 = fUvollddIj37460002;     fUvollddIj37460002 = fUvollddIj60999982;     fUvollddIj60999982 = fUvollddIj18152738;     fUvollddIj18152738 = fUvollddIj30042040;     fUvollddIj30042040 = fUvollddIj85926947;     fUvollddIj85926947 = fUvollddIj40069229;     fUvollddIj40069229 = fUvollddIj25231688;     fUvollddIj25231688 = fUvollddIj42467753;     fUvollddIj42467753 = fUvollddIj70840133;     fUvollddIj70840133 = fUvollddIj36286424;     fUvollddIj36286424 = fUvollddIj8740345;     fUvollddIj8740345 = fUvollddIj8716405;     fUvollddIj8716405 = fUvollddIj78577446;     fUvollddIj78577446 = fUvollddIj20984888;     fUvollddIj20984888 = fUvollddIj71661188;     fUvollddIj71661188 = fUvollddIj70029435;     fUvollddIj70029435 = fUvollddIj23156051;     fUvollddIj23156051 = fUvollddIj60268160;     fUvollddIj60268160 = fUvollddIj82444792;     fUvollddIj82444792 = fUvollddIj87014177;     fUvollddIj87014177 = fUvollddIj93192987;     fUvollddIj93192987 = fUvollddIj92565994;     fUvollddIj92565994 = fUvollddIj27618061;     fUvollddIj27618061 = fUvollddIj55650746;     fUvollddIj55650746 = fUvollddIj26600580;     fUvollddIj26600580 = fUvollddIj18976234;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void VUmwYGrmnl48892737() {     int fTCKdTyGnk9096072 = -319413671;    int fTCKdTyGnk57963909 = -67234860;    int fTCKdTyGnk77864251 = -689480209;    int fTCKdTyGnk62340218 = 44841259;    int fTCKdTyGnk54932473 = -77399566;    int fTCKdTyGnk69507837 = -878073892;    int fTCKdTyGnk69776569 = -570963957;    int fTCKdTyGnk52255595 = -318327575;    int fTCKdTyGnk66191725 = -269465568;    int fTCKdTyGnk20177508 = -144404903;    int fTCKdTyGnk97124938 = -195651519;    int fTCKdTyGnk47853149 = -381807792;    int fTCKdTyGnk83313717 = -397473244;    int fTCKdTyGnk74653680 = -289112193;    int fTCKdTyGnk16790076 = -555342378;    int fTCKdTyGnk66494289 = -674422608;    int fTCKdTyGnk37500781 = -800297465;    int fTCKdTyGnk19886680 = -322540158;    int fTCKdTyGnk6867000 = -792843770;    int fTCKdTyGnk99263347 = -601891894;    int fTCKdTyGnk6401661 = -409082504;    int fTCKdTyGnk18695854 = -722522088;    int fTCKdTyGnk3388905 = -457160429;    int fTCKdTyGnk17458317 = -642434878;    int fTCKdTyGnk97326190 = -634811663;    int fTCKdTyGnk47195865 = -715146308;    int fTCKdTyGnk88259327 = -671082141;    int fTCKdTyGnk97157183 = -685060633;    int fTCKdTyGnk95266587 = -278115012;    int fTCKdTyGnk29818861 = -489368412;    int fTCKdTyGnk96689800 = -119925697;    int fTCKdTyGnk75498793 = -425445051;    int fTCKdTyGnk12786307 = -814647466;    int fTCKdTyGnk88227870 = -830276832;    int fTCKdTyGnk66273989 = -576319833;    int fTCKdTyGnk69083070 = -92320077;    int fTCKdTyGnk50502217 = -742363582;    int fTCKdTyGnk36306593 = -345730556;    int fTCKdTyGnk49496218 = -873181200;    int fTCKdTyGnk61244801 = -928980827;    int fTCKdTyGnk62730839 = -468779365;    int fTCKdTyGnk41691956 = -934702322;    int fTCKdTyGnk1418226 = -522277080;    int fTCKdTyGnk5013191 = -252490361;    int fTCKdTyGnk11827542 = -969737001;    int fTCKdTyGnk96938326 = -358603491;    int fTCKdTyGnk79882677 = -611467656;    int fTCKdTyGnk36608763 = -926669592;    int fTCKdTyGnk42692574 = -985301909;    int fTCKdTyGnk99864838 = -445618189;    int fTCKdTyGnk12278381 = -208476540;    int fTCKdTyGnk9393739 = -745727403;    int fTCKdTyGnk7592420 = -491657267;    int fTCKdTyGnk2944729 = -501327794;    int fTCKdTyGnk42491077 = -598702138;    int fTCKdTyGnk90400217 = -596891584;    int fTCKdTyGnk54575005 = -610074432;    int fTCKdTyGnk60405935 = 52954668;    int fTCKdTyGnk65014027 = -320347079;    int fTCKdTyGnk7736608 = -362253258;    int fTCKdTyGnk81248509 = -106991751;    int fTCKdTyGnk72619385 = -885903325;    int fTCKdTyGnk56989007 = 59787437;    int fTCKdTyGnk36372865 = -780097157;    int fTCKdTyGnk23487708 = 75520793;    int fTCKdTyGnk21626146 = -770206468;    int fTCKdTyGnk35066842 = -567160327;    int fTCKdTyGnk95085846 = -567196412;    int fTCKdTyGnk8379692 = -712792361;    int fTCKdTyGnk47707005 = -363022302;    int fTCKdTyGnk15992072 = -932059027;    int fTCKdTyGnk1194189 = -354566909;    int fTCKdTyGnk70390462 = -449358959;    int fTCKdTyGnk45622199 = -863862943;    int fTCKdTyGnk36532508 = -33112530;    int fTCKdTyGnk64709704 = -474380182;    int fTCKdTyGnk17277629 = -100245008;    int fTCKdTyGnk98375713 = -104670068;    int fTCKdTyGnk5630776 = -672697877;    int fTCKdTyGnk387864 = -176208172;    int fTCKdTyGnk67313187 = -3678653;    int fTCKdTyGnk51650565 = -744412550;    int fTCKdTyGnk54464609 = -699758724;    int fTCKdTyGnk95401748 = -832496824;    int fTCKdTyGnk17540480 = -180891872;    int fTCKdTyGnk87296061 = -374198294;    int fTCKdTyGnk67906373 = -933787785;    int fTCKdTyGnk9841578 = -213319672;    int fTCKdTyGnk45736794 = -131574695;    int fTCKdTyGnk75873771 = -979428249;    int fTCKdTyGnk14508066 = -482245645;    int fTCKdTyGnk90096281 = -695318250;    int fTCKdTyGnk71292565 = 74616522;    int fTCKdTyGnk41759610 = -410927942;    int fTCKdTyGnk79996291 = -721989076;    int fTCKdTyGnk90111454 = -582876040;    int fTCKdTyGnk84702949 = -894489759;    int fTCKdTyGnk65045360 = -742179924;    int fTCKdTyGnk81525483 = -228011155;    int fTCKdTyGnk90201396 = -319413671;     fTCKdTyGnk9096072 = fTCKdTyGnk57963909;     fTCKdTyGnk57963909 = fTCKdTyGnk77864251;     fTCKdTyGnk77864251 = fTCKdTyGnk62340218;     fTCKdTyGnk62340218 = fTCKdTyGnk54932473;     fTCKdTyGnk54932473 = fTCKdTyGnk69507837;     fTCKdTyGnk69507837 = fTCKdTyGnk69776569;     fTCKdTyGnk69776569 = fTCKdTyGnk52255595;     fTCKdTyGnk52255595 = fTCKdTyGnk66191725;     fTCKdTyGnk66191725 = fTCKdTyGnk20177508;     fTCKdTyGnk20177508 = fTCKdTyGnk97124938;     fTCKdTyGnk97124938 = fTCKdTyGnk47853149;     fTCKdTyGnk47853149 = fTCKdTyGnk83313717;     fTCKdTyGnk83313717 = fTCKdTyGnk74653680;     fTCKdTyGnk74653680 = fTCKdTyGnk16790076;     fTCKdTyGnk16790076 = fTCKdTyGnk66494289;     fTCKdTyGnk66494289 = fTCKdTyGnk37500781;     fTCKdTyGnk37500781 = fTCKdTyGnk19886680;     fTCKdTyGnk19886680 = fTCKdTyGnk6867000;     fTCKdTyGnk6867000 = fTCKdTyGnk99263347;     fTCKdTyGnk99263347 = fTCKdTyGnk6401661;     fTCKdTyGnk6401661 = fTCKdTyGnk18695854;     fTCKdTyGnk18695854 = fTCKdTyGnk3388905;     fTCKdTyGnk3388905 = fTCKdTyGnk17458317;     fTCKdTyGnk17458317 = fTCKdTyGnk97326190;     fTCKdTyGnk97326190 = fTCKdTyGnk47195865;     fTCKdTyGnk47195865 = fTCKdTyGnk88259327;     fTCKdTyGnk88259327 = fTCKdTyGnk97157183;     fTCKdTyGnk97157183 = fTCKdTyGnk95266587;     fTCKdTyGnk95266587 = fTCKdTyGnk29818861;     fTCKdTyGnk29818861 = fTCKdTyGnk96689800;     fTCKdTyGnk96689800 = fTCKdTyGnk75498793;     fTCKdTyGnk75498793 = fTCKdTyGnk12786307;     fTCKdTyGnk12786307 = fTCKdTyGnk88227870;     fTCKdTyGnk88227870 = fTCKdTyGnk66273989;     fTCKdTyGnk66273989 = fTCKdTyGnk69083070;     fTCKdTyGnk69083070 = fTCKdTyGnk50502217;     fTCKdTyGnk50502217 = fTCKdTyGnk36306593;     fTCKdTyGnk36306593 = fTCKdTyGnk49496218;     fTCKdTyGnk49496218 = fTCKdTyGnk61244801;     fTCKdTyGnk61244801 = fTCKdTyGnk62730839;     fTCKdTyGnk62730839 = fTCKdTyGnk41691956;     fTCKdTyGnk41691956 = fTCKdTyGnk1418226;     fTCKdTyGnk1418226 = fTCKdTyGnk5013191;     fTCKdTyGnk5013191 = fTCKdTyGnk11827542;     fTCKdTyGnk11827542 = fTCKdTyGnk96938326;     fTCKdTyGnk96938326 = fTCKdTyGnk79882677;     fTCKdTyGnk79882677 = fTCKdTyGnk36608763;     fTCKdTyGnk36608763 = fTCKdTyGnk42692574;     fTCKdTyGnk42692574 = fTCKdTyGnk99864838;     fTCKdTyGnk99864838 = fTCKdTyGnk12278381;     fTCKdTyGnk12278381 = fTCKdTyGnk9393739;     fTCKdTyGnk9393739 = fTCKdTyGnk7592420;     fTCKdTyGnk7592420 = fTCKdTyGnk2944729;     fTCKdTyGnk2944729 = fTCKdTyGnk42491077;     fTCKdTyGnk42491077 = fTCKdTyGnk90400217;     fTCKdTyGnk90400217 = fTCKdTyGnk54575005;     fTCKdTyGnk54575005 = fTCKdTyGnk60405935;     fTCKdTyGnk60405935 = fTCKdTyGnk65014027;     fTCKdTyGnk65014027 = fTCKdTyGnk7736608;     fTCKdTyGnk7736608 = fTCKdTyGnk81248509;     fTCKdTyGnk81248509 = fTCKdTyGnk72619385;     fTCKdTyGnk72619385 = fTCKdTyGnk56989007;     fTCKdTyGnk56989007 = fTCKdTyGnk36372865;     fTCKdTyGnk36372865 = fTCKdTyGnk23487708;     fTCKdTyGnk23487708 = fTCKdTyGnk21626146;     fTCKdTyGnk21626146 = fTCKdTyGnk35066842;     fTCKdTyGnk35066842 = fTCKdTyGnk95085846;     fTCKdTyGnk95085846 = fTCKdTyGnk8379692;     fTCKdTyGnk8379692 = fTCKdTyGnk47707005;     fTCKdTyGnk47707005 = fTCKdTyGnk15992072;     fTCKdTyGnk15992072 = fTCKdTyGnk1194189;     fTCKdTyGnk1194189 = fTCKdTyGnk70390462;     fTCKdTyGnk70390462 = fTCKdTyGnk45622199;     fTCKdTyGnk45622199 = fTCKdTyGnk36532508;     fTCKdTyGnk36532508 = fTCKdTyGnk64709704;     fTCKdTyGnk64709704 = fTCKdTyGnk17277629;     fTCKdTyGnk17277629 = fTCKdTyGnk98375713;     fTCKdTyGnk98375713 = fTCKdTyGnk5630776;     fTCKdTyGnk5630776 = fTCKdTyGnk387864;     fTCKdTyGnk387864 = fTCKdTyGnk67313187;     fTCKdTyGnk67313187 = fTCKdTyGnk51650565;     fTCKdTyGnk51650565 = fTCKdTyGnk54464609;     fTCKdTyGnk54464609 = fTCKdTyGnk95401748;     fTCKdTyGnk95401748 = fTCKdTyGnk17540480;     fTCKdTyGnk17540480 = fTCKdTyGnk87296061;     fTCKdTyGnk87296061 = fTCKdTyGnk67906373;     fTCKdTyGnk67906373 = fTCKdTyGnk9841578;     fTCKdTyGnk9841578 = fTCKdTyGnk45736794;     fTCKdTyGnk45736794 = fTCKdTyGnk75873771;     fTCKdTyGnk75873771 = fTCKdTyGnk14508066;     fTCKdTyGnk14508066 = fTCKdTyGnk90096281;     fTCKdTyGnk90096281 = fTCKdTyGnk71292565;     fTCKdTyGnk71292565 = fTCKdTyGnk41759610;     fTCKdTyGnk41759610 = fTCKdTyGnk79996291;     fTCKdTyGnk79996291 = fTCKdTyGnk90111454;     fTCKdTyGnk90111454 = fTCKdTyGnk84702949;     fTCKdTyGnk84702949 = fTCKdTyGnk65045360;     fTCKdTyGnk65045360 = fTCKdTyGnk81525483;     fTCKdTyGnk81525483 = fTCKdTyGnk90201396;     fTCKdTyGnk90201396 = fTCKdTyGnk9096072;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void TcGrMXDWmN85537711() {     int XpAAXTJUmm31483189 = 46182366;    int XpAAXTJUmm42311410 = -15777915;    int XpAAXTJUmm6633531 = -929134053;    int XpAAXTJUmm77346906 = 38433727;    int XpAAXTJUmm53106727 = -668456567;    int XpAAXTJUmm84390954 = -738682270;    int XpAAXTJUmm61932308 = -392951263;    int XpAAXTJUmm27889454 = -788924219;    int XpAAXTJUmm63354856 = -153661607;    int XpAAXTJUmm11941093 = -987146236;    int XpAAXTJUmm34666798 = -192275920;    int XpAAXTJUmm3756098 = -632553086;    int XpAAXTJUmm50321898 = 83809436;    int XpAAXTJUmm17268433 = -816820950;    int XpAAXTJUmm53525352 = -182820091;    int XpAAXTJUmm10750202 = 43909850;    int XpAAXTJUmm87324133 = 22422715;    int XpAAXTJUmm25863097 = -774282849;    int XpAAXTJUmm48148631 = -927945895;    int XpAAXTJUmm77779630 = -854526348;    int XpAAXTJUmm7284768 = -400569812;    int XpAAXTJUmm82511072 = -127288401;    int XpAAXTJUmm6682228 = -633241021;    int XpAAXTJUmm43958104 = -106427735;    int XpAAXTJUmm32866437 = -256370787;    int XpAAXTJUmm35498859 = -59040532;    int XpAAXTJUmm35348748 = -428668634;    int XpAAXTJUmm3068634 = -473515087;    int XpAAXTJUmm2159149 = -622372131;    int XpAAXTJUmm38976040 = -409664278;    int XpAAXTJUmm40980606 = -201241849;    int XpAAXTJUmm33039984 = 22483594;    int XpAAXTJUmm93824976 = -243990961;    int XpAAXTJUmm47616558 = -54403683;    int XpAAXTJUmm42503353 = -640076255;    int XpAAXTJUmm10775940 = -407715453;    int XpAAXTJUmm43941929 = -475311533;    int XpAAXTJUmm89932693 = -213356028;    int XpAAXTJUmm97804349 = -174227231;    int XpAAXTJUmm81430515 = 34795144;    int XpAAXTJUmm91488210 = -971957031;    int XpAAXTJUmm30200348 = -692611013;    int XpAAXTJUmm6968418 = -409807664;    int XpAAXTJUmm4457310 = -249854010;    int XpAAXTJUmm79810808 = -778880479;    int XpAAXTJUmm5038609 = -540832586;    int XpAAXTJUmm92180074 = -89877909;    int XpAAXTJUmm81207437 = -683204583;    int XpAAXTJUmm37152479 = -935018608;    int XpAAXTJUmm96948584 = -587638448;    int XpAAXTJUmm21147597 = -563552817;    int XpAAXTJUmm83244995 = -76546685;    int XpAAXTJUmm85906823 = -163568497;    int XpAAXTJUmm63231274 = -777008603;    int XpAAXTJUmm78252944 = -739406254;    int XpAAXTJUmm48972117 = -826529234;    int XpAAXTJUmm35629182 = -382536895;    int XpAAXTJUmm62675426 = -722706319;    int XpAAXTJUmm44480469 = -705195486;    int XpAAXTJUmm17607868 = -509416036;    int XpAAXTJUmm49042206 = -210013637;    int XpAAXTJUmm58863674 = -919436177;    int XpAAXTJUmm25730305 = -66552088;    int XpAAXTJUmm24378817 = -743997329;    int XpAAXTJUmm70960487 = -685904388;    int XpAAXTJUmm1626815 = -114759514;    int XpAAXTJUmm9931121 = -288562126;    int XpAAXTJUmm2705340 = -861786882;    int XpAAXTJUmm74765079 = -76744695;    int XpAAXTJUmm42749413 = -775104638;    int XpAAXTJUmm66808272 = -480778618;    int XpAAXTJUmm97391439 = -764221257;    int XpAAXTJUmm28058747 = -500055618;    int XpAAXTJUmm66718115 = -862741039;    int XpAAXTJUmm86291420 = -882569317;    int XpAAXTJUmm77084420 = -707958800;    int XpAAXTJUmm75542655 = -717480738;    int XpAAXTJUmm2224918 = -283387012;    int XpAAXTJUmm64147296 = -327547257;    int XpAAXTJUmm27827829 = -715538202;    int XpAAXTJUmm43318785 = -969162624;    int XpAAXTJUmm54141310 = -745464052;    int XpAAXTJUmm65916154 = -538496479;    int XpAAXTJUmm5210564 = 65266316;    int XpAAXTJUmm17828443 = -846111462;    int XpAAXTJUmm57735610 = -24695164;    int XpAAXTJUmm47133161 = -813947909;    int XpAAXTJUmm30593703 = -466982358;    int XpAAXTJUmm69363614 = -314997429;    int XpAAXTJUmm93531236 = -813547022;    int XpAAXTJUmm75146757 = 74821441;    int XpAAXTJUmm81266502 = -752605215;    int XpAAXTJUmm45452224 = -508160543;    int XpAAXTJUmm80196482 = -664811196;    int XpAAXTJUmm32388309 = -755191220;    int XpAAXTJUmm32624536 = 47479146;    int XpAAXTJUmm4470043 = -526058925;    int XpAAXTJUmm82589600 = -665810335;    int XpAAXTJUmm33496823 = -563949623;    int XpAAXTJUmm78183994 = 46182366;     XpAAXTJUmm31483189 = XpAAXTJUmm42311410;     XpAAXTJUmm42311410 = XpAAXTJUmm6633531;     XpAAXTJUmm6633531 = XpAAXTJUmm77346906;     XpAAXTJUmm77346906 = XpAAXTJUmm53106727;     XpAAXTJUmm53106727 = XpAAXTJUmm84390954;     XpAAXTJUmm84390954 = XpAAXTJUmm61932308;     XpAAXTJUmm61932308 = XpAAXTJUmm27889454;     XpAAXTJUmm27889454 = XpAAXTJUmm63354856;     XpAAXTJUmm63354856 = XpAAXTJUmm11941093;     XpAAXTJUmm11941093 = XpAAXTJUmm34666798;     XpAAXTJUmm34666798 = XpAAXTJUmm3756098;     XpAAXTJUmm3756098 = XpAAXTJUmm50321898;     XpAAXTJUmm50321898 = XpAAXTJUmm17268433;     XpAAXTJUmm17268433 = XpAAXTJUmm53525352;     XpAAXTJUmm53525352 = XpAAXTJUmm10750202;     XpAAXTJUmm10750202 = XpAAXTJUmm87324133;     XpAAXTJUmm87324133 = XpAAXTJUmm25863097;     XpAAXTJUmm25863097 = XpAAXTJUmm48148631;     XpAAXTJUmm48148631 = XpAAXTJUmm77779630;     XpAAXTJUmm77779630 = XpAAXTJUmm7284768;     XpAAXTJUmm7284768 = XpAAXTJUmm82511072;     XpAAXTJUmm82511072 = XpAAXTJUmm6682228;     XpAAXTJUmm6682228 = XpAAXTJUmm43958104;     XpAAXTJUmm43958104 = XpAAXTJUmm32866437;     XpAAXTJUmm32866437 = XpAAXTJUmm35498859;     XpAAXTJUmm35498859 = XpAAXTJUmm35348748;     XpAAXTJUmm35348748 = XpAAXTJUmm3068634;     XpAAXTJUmm3068634 = XpAAXTJUmm2159149;     XpAAXTJUmm2159149 = XpAAXTJUmm38976040;     XpAAXTJUmm38976040 = XpAAXTJUmm40980606;     XpAAXTJUmm40980606 = XpAAXTJUmm33039984;     XpAAXTJUmm33039984 = XpAAXTJUmm93824976;     XpAAXTJUmm93824976 = XpAAXTJUmm47616558;     XpAAXTJUmm47616558 = XpAAXTJUmm42503353;     XpAAXTJUmm42503353 = XpAAXTJUmm10775940;     XpAAXTJUmm10775940 = XpAAXTJUmm43941929;     XpAAXTJUmm43941929 = XpAAXTJUmm89932693;     XpAAXTJUmm89932693 = XpAAXTJUmm97804349;     XpAAXTJUmm97804349 = XpAAXTJUmm81430515;     XpAAXTJUmm81430515 = XpAAXTJUmm91488210;     XpAAXTJUmm91488210 = XpAAXTJUmm30200348;     XpAAXTJUmm30200348 = XpAAXTJUmm6968418;     XpAAXTJUmm6968418 = XpAAXTJUmm4457310;     XpAAXTJUmm4457310 = XpAAXTJUmm79810808;     XpAAXTJUmm79810808 = XpAAXTJUmm5038609;     XpAAXTJUmm5038609 = XpAAXTJUmm92180074;     XpAAXTJUmm92180074 = XpAAXTJUmm81207437;     XpAAXTJUmm81207437 = XpAAXTJUmm37152479;     XpAAXTJUmm37152479 = XpAAXTJUmm96948584;     XpAAXTJUmm96948584 = XpAAXTJUmm21147597;     XpAAXTJUmm21147597 = XpAAXTJUmm83244995;     XpAAXTJUmm83244995 = XpAAXTJUmm85906823;     XpAAXTJUmm85906823 = XpAAXTJUmm63231274;     XpAAXTJUmm63231274 = XpAAXTJUmm78252944;     XpAAXTJUmm78252944 = XpAAXTJUmm48972117;     XpAAXTJUmm48972117 = XpAAXTJUmm35629182;     XpAAXTJUmm35629182 = XpAAXTJUmm62675426;     XpAAXTJUmm62675426 = XpAAXTJUmm44480469;     XpAAXTJUmm44480469 = XpAAXTJUmm17607868;     XpAAXTJUmm17607868 = XpAAXTJUmm49042206;     XpAAXTJUmm49042206 = XpAAXTJUmm58863674;     XpAAXTJUmm58863674 = XpAAXTJUmm25730305;     XpAAXTJUmm25730305 = XpAAXTJUmm24378817;     XpAAXTJUmm24378817 = XpAAXTJUmm70960487;     XpAAXTJUmm70960487 = XpAAXTJUmm1626815;     XpAAXTJUmm1626815 = XpAAXTJUmm9931121;     XpAAXTJUmm9931121 = XpAAXTJUmm2705340;     XpAAXTJUmm2705340 = XpAAXTJUmm74765079;     XpAAXTJUmm74765079 = XpAAXTJUmm42749413;     XpAAXTJUmm42749413 = XpAAXTJUmm66808272;     XpAAXTJUmm66808272 = XpAAXTJUmm97391439;     XpAAXTJUmm97391439 = XpAAXTJUmm28058747;     XpAAXTJUmm28058747 = XpAAXTJUmm66718115;     XpAAXTJUmm66718115 = XpAAXTJUmm86291420;     XpAAXTJUmm86291420 = XpAAXTJUmm77084420;     XpAAXTJUmm77084420 = XpAAXTJUmm75542655;     XpAAXTJUmm75542655 = XpAAXTJUmm2224918;     XpAAXTJUmm2224918 = XpAAXTJUmm64147296;     XpAAXTJUmm64147296 = XpAAXTJUmm27827829;     XpAAXTJUmm27827829 = XpAAXTJUmm43318785;     XpAAXTJUmm43318785 = XpAAXTJUmm54141310;     XpAAXTJUmm54141310 = XpAAXTJUmm65916154;     XpAAXTJUmm65916154 = XpAAXTJUmm5210564;     XpAAXTJUmm5210564 = XpAAXTJUmm17828443;     XpAAXTJUmm17828443 = XpAAXTJUmm57735610;     XpAAXTJUmm57735610 = XpAAXTJUmm47133161;     XpAAXTJUmm47133161 = XpAAXTJUmm30593703;     XpAAXTJUmm30593703 = XpAAXTJUmm69363614;     XpAAXTJUmm69363614 = XpAAXTJUmm93531236;     XpAAXTJUmm93531236 = XpAAXTJUmm75146757;     XpAAXTJUmm75146757 = XpAAXTJUmm81266502;     XpAAXTJUmm81266502 = XpAAXTJUmm45452224;     XpAAXTJUmm45452224 = XpAAXTJUmm80196482;     XpAAXTJUmm80196482 = XpAAXTJUmm32388309;     XpAAXTJUmm32388309 = XpAAXTJUmm32624536;     XpAAXTJUmm32624536 = XpAAXTJUmm4470043;     XpAAXTJUmm4470043 = XpAAXTJUmm82589600;     XpAAXTJUmm82589600 = XpAAXTJUmm33496823;     XpAAXTJUmm33496823 = XpAAXTJUmm78183994;     XpAAXTJUmm78183994 = XpAAXTJUmm31483189;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void iNHindRXxu38214517() {     int RQYhgtbdWn68403248 = -337599875;    int RQYhgtbdWn86602227 = -137013270;    int RQYhgtbdWn74657847 = -74531589;    int RQYhgtbdWn1480066 = -197498771;    int RQYhgtbdWn6045840 = -89358875;    int RQYhgtbdWn6557990 = 38766232;    int RQYhgtbdWn54114810 = -825883419;    int RQYhgtbdWn77909314 = -695508370;    int RQYhgtbdWn12425588 = -566583827;    int RQYhgtbdWn84601357 = -336209160;    int RQYhgtbdWn59427679 = -886232681;    int RQYhgtbdWn76955494 = -138525561;    int RQYhgtbdWn68911575 = -834968472;    int RQYhgtbdWn49047597 = -149607612;    int RQYhgtbdWn38531249 = -360263854;    int RQYhgtbdWn39670497 = -292940649;    int RQYhgtbdWn51127498 = -52400764;    int RQYhgtbdWn35829987 = -273936998;    int RQYhgtbdWn89415269 = -968962432;    int RQYhgtbdWn2550900 = -895630300;    int RQYhgtbdWn94667791 = -922865399;    int RQYhgtbdWn65775098 = -907908275;    int RQYhgtbdWn85548166 = -370386731;    int RQYhgtbdWn81227581 = -560746641;    int RQYhgtbdWn70942094 = -662312030;    int RQYhgtbdWn58821278 = -149615360;    int RQYhgtbdWn84794571 = -755970627;    int RQYhgtbdWn12506065 = -303406101;    int RQYhgtbdWn28231553 = -925122342;    int RQYhgtbdWn12712146 = -989742192;    int RQYhgtbdWn8454028 = -764444057;    int RQYhgtbdWn33662978 = -796393548;    int RQYhgtbdWn19461166 = -603168748;    int RQYhgtbdWn77610365 = -984688885;    int RQYhgtbdWn5871812 = 23125167;    int RQYhgtbdWn54443202 = -883252707;    int RQYhgtbdWn77896534 = -193130167;    int RQYhgtbdWn29740491 = -92404268;    int RQYhgtbdWn9863915 = -990248757;    int RQYhgtbdWn2185814 = -972955947;    int RQYhgtbdWn71613558 = -440865656;    int RQYhgtbdWn11465688 = 1785484;    int RQYhgtbdWn73489431 = -893070280;    int RQYhgtbdWn62809964 = -189090803;    int RQYhgtbdWn2800182 = 21912953;    int RQYhgtbdWn17712523 = -174193888;    int RQYhgtbdWn69754549 = -235902579;    int RQYhgtbdWn27309277 = -759164676;    int RQYhgtbdWn57042705 = -950703252;    int RQYhgtbdWn44559520 = -822412999;    int RQYhgtbdWn15820302 = -517546296;    int RQYhgtbdWn41794852 = 53604194;    int RQYhgtbdWn14910513 = -219501684;    int RQYhgtbdWn70338811 = -151080726;    int RQYhgtbdWn43546726 = -229744320;    int RQYhgtbdWn2628151 = -429691601;    int RQYhgtbdWn1054061 = -766626539;    int RQYhgtbdWn93430266 = -513784949;    int RQYhgtbdWn30537971 = -535186741;    int RQYhgtbdWn47224561 = -939743516;    int RQYhgtbdWn21763418 = -205263141;    int RQYhgtbdWn41608745 = -422477318;    int RQYhgtbdWn49677761 = -770386028;    int RQYhgtbdWn99713441 = -576841636;    int RQYhgtbdWn76147329 = -571765103;    int RQYhgtbdWn25764702 = 10160866;    int RQYhgtbdWn57494329 = -535356813;    int RQYhgtbdWn91301209 = -850279588;    int RQYhgtbdWn43175785 = -72732780;    int RQYhgtbdWn84088047 = -477011148;    int RQYhgtbdWn61773963 = 189518;    int RQYhgtbdWn21387008 = -959996497;    int RQYhgtbdWn25966072 = -283688242;    int RQYhgtbdWn87229455 = -996006486;    int RQYhgtbdWn30937341 = -354764644;    int RQYhgtbdWn83202103 = -824650884;    int RQYhgtbdWn92285667 = 85162005;    int RQYhgtbdWn22738203 = -81295929;    int RQYhgtbdWn78427399 = -482659595;    int RQYhgtbdWn53229572 = -388118143;    int RQYhgtbdWn89066729 = -913712781;    int RQYhgtbdWn57485295 = -996805952;    int RQYhgtbdWn55463360 = -352702849;    int RQYhgtbdWn83672033 = -2709343;    int RQYhgtbdWn96891844 = -372195897;    int RQYhgtbdWn66659175 = -718048251;    int RQYhgtbdWn18752465 = -476891865;    int RQYhgtbdWn49122354 = -352088023;    int RQYhgtbdWn34063640 = -654944565;    int RQYhgtbdWn3243662 = -547183233;    int RQYhgtbdWn53389141 = -16626168;    int RQYhgtbdWn84466268 = -679345219;    int RQYhgtbdWn99202519 = -557217527;    int RQYhgtbdWn62639354 = 49494758;    int RQYhgtbdWn80422395 = -667692807;    int RQYhgtbdWn30004813 = 81611662;    int RQYhgtbdWn61787927 = -227828488;    int RQYhgtbdWn73775989 = -216228644;    int RQYhgtbdWn86662634 = -617325700;    int RQYhgtbdWn77035480 = -337599875;     RQYhgtbdWn68403248 = RQYhgtbdWn86602227;     RQYhgtbdWn86602227 = RQYhgtbdWn74657847;     RQYhgtbdWn74657847 = RQYhgtbdWn1480066;     RQYhgtbdWn1480066 = RQYhgtbdWn6045840;     RQYhgtbdWn6045840 = RQYhgtbdWn6557990;     RQYhgtbdWn6557990 = RQYhgtbdWn54114810;     RQYhgtbdWn54114810 = RQYhgtbdWn77909314;     RQYhgtbdWn77909314 = RQYhgtbdWn12425588;     RQYhgtbdWn12425588 = RQYhgtbdWn84601357;     RQYhgtbdWn84601357 = RQYhgtbdWn59427679;     RQYhgtbdWn59427679 = RQYhgtbdWn76955494;     RQYhgtbdWn76955494 = RQYhgtbdWn68911575;     RQYhgtbdWn68911575 = RQYhgtbdWn49047597;     RQYhgtbdWn49047597 = RQYhgtbdWn38531249;     RQYhgtbdWn38531249 = RQYhgtbdWn39670497;     RQYhgtbdWn39670497 = RQYhgtbdWn51127498;     RQYhgtbdWn51127498 = RQYhgtbdWn35829987;     RQYhgtbdWn35829987 = RQYhgtbdWn89415269;     RQYhgtbdWn89415269 = RQYhgtbdWn2550900;     RQYhgtbdWn2550900 = RQYhgtbdWn94667791;     RQYhgtbdWn94667791 = RQYhgtbdWn65775098;     RQYhgtbdWn65775098 = RQYhgtbdWn85548166;     RQYhgtbdWn85548166 = RQYhgtbdWn81227581;     RQYhgtbdWn81227581 = RQYhgtbdWn70942094;     RQYhgtbdWn70942094 = RQYhgtbdWn58821278;     RQYhgtbdWn58821278 = RQYhgtbdWn84794571;     RQYhgtbdWn84794571 = RQYhgtbdWn12506065;     RQYhgtbdWn12506065 = RQYhgtbdWn28231553;     RQYhgtbdWn28231553 = RQYhgtbdWn12712146;     RQYhgtbdWn12712146 = RQYhgtbdWn8454028;     RQYhgtbdWn8454028 = RQYhgtbdWn33662978;     RQYhgtbdWn33662978 = RQYhgtbdWn19461166;     RQYhgtbdWn19461166 = RQYhgtbdWn77610365;     RQYhgtbdWn77610365 = RQYhgtbdWn5871812;     RQYhgtbdWn5871812 = RQYhgtbdWn54443202;     RQYhgtbdWn54443202 = RQYhgtbdWn77896534;     RQYhgtbdWn77896534 = RQYhgtbdWn29740491;     RQYhgtbdWn29740491 = RQYhgtbdWn9863915;     RQYhgtbdWn9863915 = RQYhgtbdWn2185814;     RQYhgtbdWn2185814 = RQYhgtbdWn71613558;     RQYhgtbdWn71613558 = RQYhgtbdWn11465688;     RQYhgtbdWn11465688 = RQYhgtbdWn73489431;     RQYhgtbdWn73489431 = RQYhgtbdWn62809964;     RQYhgtbdWn62809964 = RQYhgtbdWn2800182;     RQYhgtbdWn2800182 = RQYhgtbdWn17712523;     RQYhgtbdWn17712523 = RQYhgtbdWn69754549;     RQYhgtbdWn69754549 = RQYhgtbdWn27309277;     RQYhgtbdWn27309277 = RQYhgtbdWn57042705;     RQYhgtbdWn57042705 = RQYhgtbdWn44559520;     RQYhgtbdWn44559520 = RQYhgtbdWn15820302;     RQYhgtbdWn15820302 = RQYhgtbdWn41794852;     RQYhgtbdWn41794852 = RQYhgtbdWn14910513;     RQYhgtbdWn14910513 = RQYhgtbdWn70338811;     RQYhgtbdWn70338811 = RQYhgtbdWn43546726;     RQYhgtbdWn43546726 = RQYhgtbdWn2628151;     RQYhgtbdWn2628151 = RQYhgtbdWn1054061;     RQYhgtbdWn1054061 = RQYhgtbdWn93430266;     RQYhgtbdWn93430266 = RQYhgtbdWn30537971;     RQYhgtbdWn30537971 = RQYhgtbdWn47224561;     RQYhgtbdWn47224561 = RQYhgtbdWn21763418;     RQYhgtbdWn21763418 = RQYhgtbdWn41608745;     RQYhgtbdWn41608745 = RQYhgtbdWn49677761;     RQYhgtbdWn49677761 = RQYhgtbdWn99713441;     RQYhgtbdWn99713441 = RQYhgtbdWn76147329;     RQYhgtbdWn76147329 = RQYhgtbdWn25764702;     RQYhgtbdWn25764702 = RQYhgtbdWn57494329;     RQYhgtbdWn57494329 = RQYhgtbdWn91301209;     RQYhgtbdWn91301209 = RQYhgtbdWn43175785;     RQYhgtbdWn43175785 = RQYhgtbdWn84088047;     RQYhgtbdWn84088047 = RQYhgtbdWn61773963;     RQYhgtbdWn61773963 = RQYhgtbdWn21387008;     RQYhgtbdWn21387008 = RQYhgtbdWn25966072;     RQYhgtbdWn25966072 = RQYhgtbdWn87229455;     RQYhgtbdWn87229455 = RQYhgtbdWn30937341;     RQYhgtbdWn30937341 = RQYhgtbdWn83202103;     RQYhgtbdWn83202103 = RQYhgtbdWn92285667;     RQYhgtbdWn92285667 = RQYhgtbdWn22738203;     RQYhgtbdWn22738203 = RQYhgtbdWn78427399;     RQYhgtbdWn78427399 = RQYhgtbdWn53229572;     RQYhgtbdWn53229572 = RQYhgtbdWn89066729;     RQYhgtbdWn89066729 = RQYhgtbdWn57485295;     RQYhgtbdWn57485295 = RQYhgtbdWn55463360;     RQYhgtbdWn55463360 = RQYhgtbdWn83672033;     RQYhgtbdWn83672033 = RQYhgtbdWn96891844;     RQYhgtbdWn96891844 = RQYhgtbdWn66659175;     RQYhgtbdWn66659175 = RQYhgtbdWn18752465;     RQYhgtbdWn18752465 = RQYhgtbdWn49122354;     RQYhgtbdWn49122354 = RQYhgtbdWn34063640;     RQYhgtbdWn34063640 = RQYhgtbdWn3243662;     RQYhgtbdWn3243662 = RQYhgtbdWn53389141;     RQYhgtbdWn53389141 = RQYhgtbdWn84466268;     RQYhgtbdWn84466268 = RQYhgtbdWn99202519;     RQYhgtbdWn99202519 = RQYhgtbdWn62639354;     RQYhgtbdWn62639354 = RQYhgtbdWn80422395;     RQYhgtbdWn80422395 = RQYhgtbdWn30004813;     RQYhgtbdWn30004813 = RQYhgtbdWn61787927;     RQYhgtbdWn61787927 = RQYhgtbdWn73775989;     RQYhgtbdWn73775989 = RQYhgtbdWn86662634;     RQYhgtbdWn86662634 = RQYhgtbdWn77035480;     RQYhgtbdWn77035480 = RQYhgtbdWn68403248;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void TSNtBrbReD74859492() {     int ZaNJIGGDOR90790366 = 27996162;    int ZaNJIGGDOR70949727 = -85556324;    int ZaNJIGGDOR3427127 = -314185433;    int ZaNJIGGDOR16486754 = -203906302;    int ZaNJIGGDOR4220094 = -680415876;    int ZaNJIGGDOR21441107 = -921842145;    int ZaNJIGGDOR46270549 = -647870725;    int ZaNJIGGDOR53543173 = -66105014;    int ZaNJIGGDOR9588719 = -450779866;    int ZaNJIGGDOR76364942 = -78950493;    int ZaNJIGGDOR96969539 = -882857082;    int ZaNJIGGDOR32858444 = -389270854;    int ZaNJIGGDOR35919756 = -353685792;    int ZaNJIGGDOR91662348 = -677316369;    int ZaNJIGGDOR75266526 = 12258433;    int ZaNJIGGDOR83926409 = -674608191;    int ZaNJIGGDOR950851 = -329680584;    int ZaNJIGGDOR41806404 = -725679689;    int ZaNJIGGDOR30696900 = -4064557;    int ZaNJIGGDOR81067182 = -48264753;    int ZaNJIGGDOR95550898 = -914352708;    int ZaNJIGGDOR29590317 = -312674588;    int ZaNJIGGDOR88841489 = -546467323;    int ZaNJIGGDOR7727369 = -24739498;    int ZaNJIGGDOR6482342 = -283871155;    int ZaNJIGGDOR47124273 = -593509583;    int ZaNJIGGDOR31883992 = -513557120;    int ZaNJIGGDOR18417515 = -91860555;    int ZaNJIGGDOR35124114 = -169379461;    int ZaNJIGGDOR21869326 = -910038058;    int ZaNJIGGDOR52744833 = -845760209;    int ZaNJIGGDOR91204168 = -348464903;    int ZaNJIGGDOR499836 = -32512243;    int ZaNJIGGDOR36999053 = -208815735;    int ZaNJIGGDOR82101176 = -40631255;    int ZaNJIGGDOR96136071 = -98648083;    int ZaNJIGGDOR71336246 = 73921881;    int ZaNJIGGDOR83366591 = 39970260;    int ZaNJIGGDOR58172046 = -291294789;    int ZaNJIGGDOR22371528 = -9179976;    int ZaNJIGGDOR370930 = -944043322;    int ZaNJIGGDOR99974079 = -856123207;    int ZaNJIGGDOR79039623 = -780600863;    int ZaNJIGGDOR62254083 = -186454452;    int ZaNJIGGDOR70783448 = -887230524;    int ZaNJIGGDOR25812805 = -356422983;    int ZaNJIGGDOR82051946 = -814312832;    int ZaNJIGGDOR71907951 = -515699667;    int ZaNJIGGDOR51502610 = -900419951;    int ZaNJIGGDOR41643265 = -964433258;    int ZaNJIGGDOR24689519 = -872622573;    int ZaNJIGGDOR15646110 = -377215088;    int ZaNJIGGDOR93224916 = -991412914;    int ZaNJIGGDOR30625357 = -426761535;    int ZaNJIGGDOR79308593 = -370448435;    int ZaNJIGGDOR61200049 = -659329250;    int ZaNJIGGDOR82108238 = -539089001;    int ZaNJIGGDOR95699757 = -189445936;    int ZaNJIGGDOR10004413 = -920035148;    int ZaNJIGGDOR57095821 = 13093707;    int ZaNJIGGDOR89557114 = -308285026;    int ZaNJIGGDOR27853034 = -456010171;    int ZaNJIGGDOR18419059 = -896725553;    int ZaNJIGGDOR87719393 = -540741808;    int ZaNJIGGDOR23620109 = -233190284;    int ZaNJIGGDOR5765371 = -434392180;    int ZaNJIGGDOR32358608 = -256758612;    int ZaNJIGGDOR98920703 = -44870057;    int ZaNJIGGDOR9561173 = -536685114;    int ZaNJIGGDOR79130455 = -889093484;    int ZaNJIGGDOR12590164 = -648530073;    int ZaNJIGGDOR17584259 = -269650845;    int ZaNJIGGDOR83634357 = -334384901;    int ZaNJIGGDOR8325372 = -994884581;    int ZaNJIGGDOR80696253 = -104221432;    int ZaNJIGGDOR95576819 = 41770499;    int ZaNJIGGDOR50550693 = -532073725;    int ZaNJIGGDOR26587407 = -260012872;    int ZaNJIGGDOR36943920 = -137508974;    int ZaNJIGGDOR80669537 = -927448172;    int ZaNJIGGDOR65072327 = -779196752;    int ZaNJIGGDOR59976041 = -997857454;    int ZaNJIGGDOR66914905 = -191440604;    int ZaNJIGGDOR93480848 = -204946203;    int ZaNJIGGDOR97179807 = 62584514;    int ZaNJIGGDOR37098723 = -368545121;    int ZaNJIGGDOR97979252 = -357051989;    int ZaNJIGGDOR69874479 = -605750709;    int ZaNJIGGDOR57690460 = -838367300;    int ZaNJIGGDOR20901127 = -381302006;    int ZaNJIGGDOR14027833 = -559559082;    int ZaNJIGGDOR75636489 = -736632184;    int ZaNJIGGDOR73362178 = -39994592;    int ZaNJIGGDOR1076226 = -204388496;    int ZaNJIGGDOR32814414 = -700894951;    int ZaNJIGGDOR72517895 = -388033151;    int ZaNJIGGDOR81555020 = -959397654;    int ZaNJIGGDOR91320230 = -139859056;    int ZaNJIGGDOR38633974 = -953264168;    int ZaNJIGGDOR65018078 = 27996162;     ZaNJIGGDOR90790366 = ZaNJIGGDOR70949727;     ZaNJIGGDOR70949727 = ZaNJIGGDOR3427127;     ZaNJIGGDOR3427127 = ZaNJIGGDOR16486754;     ZaNJIGGDOR16486754 = ZaNJIGGDOR4220094;     ZaNJIGGDOR4220094 = ZaNJIGGDOR21441107;     ZaNJIGGDOR21441107 = ZaNJIGGDOR46270549;     ZaNJIGGDOR46270549 = ZaNJIGGDOR53543173;     ZaNJIGGDOR53543173 = ZaNJIGGDOR9588719;     ZaNJIGGDOR9588719 = ZaNJIGGDOR76364942;     ZaNJIGGDOR76364942 = ZaNJIGGDOR96969539;     ZaNJIGGDOR96969539 = ZaNJIGGDOR32858444;     ZaNJIGGDOR32858444 = ZaNJIGGDOR35919756;     ZaNJIGGDOR35919756 = ZaNJIGGDOR91662348;     ZaNJIGGDOR91662348 = ZaNJIGGDOR75266526;     ZaNJIGGDOR75266526 = ZaNJIGGDOR83926409;     ZaNJIGGDOR83926409 = ZaNJIGGDOR950851;     ZaNJIGGDOR950851 = ZaNJIGGDOR41806404;     ZaNJIGGDOR41806404 = ZaNJIGGDOR30696900;     ZaNJIGGDOR30696900 = ZaNJIGGDOR81067182;     ZaNJIGGDOR81067182 = ZaNJIGGDOR95550898;     ZaNJIGGDOR95550898 = ZaNJIGGDOR29590317;     ZaNJIGGDOR29590317 = ZaNJIGGDOR88841489;     ZaNJIGGDOR88841489 = ZaNJIGGDOR7727369;     ZaNJIGGDOR7727369 = ZaNJIGGDOR6482342;     ZaNJIGGDOR6482342 = ZaNJIGGDOR47124273;     ZaNJIGGDOR47124273 = ZaNJIGGDOR31883992;     ZaNJIGGDOR31883992 = ZaNJIGGDOR18417515;     ZaNJIGGDOR18417515 = ZaNJIGGDOR35124114;     ZaNJIGGDOR35124114 = ZaNJIGGDOR21869326;     ZaNJIGGDOR21869326 = ZaNJIGGDOR52744833;     ZaNJIGGDOR52744833 = ZaNJIGGDOR91204168;     ZaNJIGGDOR91204168 = ZaNJIGGDOR499836;     ZaNJIGGDOR499836 = ZaNJIGGDOR36999053;     ZaNJIGGDOR36999053 = ZaNJIGGDOR82101176;     ZaNJIGGDOR82101176 = ZaNJIGGDOR96136071;     ZaNJIGGDOR96136071 = ZaNJIGGDOR71336246;     ZaNJIGGDOR71336246 = ZaNJIGGDOR83366591;     ZaNJIGGDOR83366591 = ZaNJIGGDOR58172046;     ZaNJIGGDOR58172046 = ZaNJIGGDOR22371528;     ZaNJIGGDOR22371528 = ZaNJIGGDOR370930;     ZaNJIGGDOR370930 = ZaNJIGGDOR99974079;     ZaNJIGGDOR99974079 = ZaNJIGGDOR79039623;     ZaNJIGGDOR79039623 = ZaNJIGGDOR62254083;     ZaNJIGGDOR62254083 = ZaNJIGGDOR70783448;     ZaNJIGGDOR70783448 = ZaNJIGGDOR25812805;     ZaNJIGGDOR25812805 = ZaNJIGGDOR82051946;     ZaNJIGGDOR82051946 = ZaNJIGGDOR71907951;     ZaNJIGGDOR71907951 = ZaNJIGGDOR51502610;     ZaNJIGGDOR51502610 = ZaNJIGGDOR41643265;     ZaNJIGGDOR41643265 = ZaNJIGGDOR24689519;     ZaNJIGGDOR24689519 = ZaNJIGGDOR15646110;     ZaNJIGGDOR15646110 = ZaNJIGGDOR93224916;     ZaNJIGGDOR93224916 = ZaNJIGGDOR30625357;     ZaNJIGGDOR30625357 = ZaNJIGGDOR79308593;     ZaNJIGGDOR79308593 = ZaNJIGGDOR61200049;     ZaNJIGGDOR61200049 = ZaNJIGGDOR82108238;     ZaNJIGGDOR82108238 = ZaNJIGGDOR95699757;     ZaNJIGGDOR95699757 = ZaNJIGGDOR10004413;     ZaNJIGGDOR10004413 = ZaNJIGGDOR57095821;     ZaNJIGGDOR57095821 = ZaNJIGGDOR89557114;     ZaNJIGGDOR89557114 = ZaNJIGGDOR27853034;     ZaNJIGGDOR27853034 = ZaNJIGGDOR18419059;     ZaNJIGGDOR18419059 = ZaNJIGGDOR87719393;     ZaNJIGGDOR87719393 = ZaNJIGGDOR23620109;     ZaNJIGGDOR23620109 = ZaNJIGGDOR5765371;     ZaNJIGGDOR5765371 = ZaNJIGGDOR32358608;     ZaNJIGGDOR32358608 = ZaNJIGGDOR98920703;     ZaNJIGGDOR98920703 = ZaNJIGGDOR9561173;     ZaNJIGGDOR9561173 = ZaNJIGGDOR79130455;     ZaNJIGGDOR79130455 = ZaNJIGGDOR12590164;     ZaNJIGGDOR12590164 = ZaNJIGGDOR17584259;     ZaNJIGGDOR17584259 = ZaNJIGGDOR83634357;     ZaNJIGGDOR83634357 = ZaNJIGGDOR8325372;     ZaNJIGGDOR8325372 = ZaNJIGGDOR80696253;     ZaNJIGGDOR80696253 = ZaNJIGGDOR95576819;     ZaNJIGGDOR95576819 = ZaNJIGGDOR50550693;     ZaNJIGGDOR50550693 = ZaNJIGGDOR26587407;     ZaNJIGGDOR26587407 = ZaNJIGGDOR36943920;     ZaNJIGGDOR36943920 = ZaNJIGGDOR80669537;     ZaNJIGGDOR80669537 = ZaNJIGGDOR65072327;     ZaNJIGGDOR65072327 = ZaNJIGGDOR59976041;     ZaNJIGGDOR59976041 = ZaNJIGGDOR66914905;     ZaNJIGGDOR66914905 = ZaNJIGGDOR93480848;     ZaNJIGGDOR93480848 = ZaNJIGGDOR97179807;     ZaNJIGGDOR97179807 = ZaNJIGGDOR37098723;     ZaNJIGGDOR37098723 = ZaNJIGGDOR97979252;     ZaNJIGGDOR97979252 = ZaNJIGGDOR69874479;     ZaNJIGGDOR69874479 = ZaNJIGGDOR57690460;     ZaNJIGGDOR57690460 = ZaNJIGGDOR20901127;     ZaNJIGGDOR20901127 = ZaNJIGGDOR14027833;     ZaNJIGGDOR14027833 = ZaNJIGGDOR75636489;     ZaNJIGGDOR75636489 = ZaNJIGGDOR73362178;     ZaNJIGGDOR73362178 = ZaNJIGGDOR1076226;     ZaNJIGGDOR1076226 = ZaNJIGGDOR32814414;     ZaNJIGGDOR32814414 = ZaNJIGGDOR72517895;     ZaNJIGGDOR72517895 = ZaNJIGGDOR81555020;     ZaNJIGGDOR81555020 = ZaNJIGGDOR91320230;     ZaNJIGGDOR91320230 = ZaNJIGGDOR38633974;     ZaNJIGGDOR38633974 = ZaNJIGGDOR65018078;     ZaNJIGGDOR65018078 = ZaNJIGGDOR90790366;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void HLwLKKQSvA11504467() {     int EVfzIuTTyc13177484 = -706407801;    int EVfzIuTTyc55297228 = -34099379;    int EVfzIuTTyc32196405 = -553839277;    int EVfzIuTTyc31493443 = -210313834;    int EVfzIuTTyc2394348 = -171472877;    int EVfzIuTTyc36324224 = -782450523;    int EVfzIuTTyc38426288 = -469858031;    int EVfzIuTTyc29177032 = -536701658;    int EVfzIuTTyc6751850 = -334975905;    int EVfzIuTTyc68128526 = -921691826;    int EVfzIuTTyc34511399 = -879481483;    int EVfzIuTTyc88761392 = -640016147;    int EVfzIuTTyc2927937 = -972403111;    int EVfzIuTTyc34277101 = -105025126;    int EVfzIuTTyc12001804 = -715219280;    int EVfzIuTTyc28182322 = 43724266;    int EVfzIuTTyc50774202 = -606960404;    int EVfzIuTTyc47782820 = -77422379;    int EVfzIuTTyc71978531 = -139166682;    int EVfzIuTTyc59583466 = -300899207;    int EVfzIuTTyc96434006 = -905840016;    int EVfzIuTTyc93405534 = -817440901;    int EVfzIuTTyc92134812 = -722547915;    int EVfzIuTTyc34227156 = -588732355;    int EVfzIuTTyc42022588 = 94569721;    int EVfzIuTTyc35427268 = 62596193;    int EVfzIuTTyc78973412 = -271143613;    int EVfzIuTTyc24328966 = -980315008;    int EVfzIuTTyc42016675 = -513636580;    int EVfzIuTTyc31026505 = -830333925;    int EVfzIuTTyc97035637 = -927076361;    int EVfzIuTTyc48745360 = 99463742;    int EVfzIuTTyc81538506 = -561855738;    int EVfzIuTTyc96387740 = -532942585;    int EVfzIuTTyc58330541 = -104387678;    int EVfzIuTTyc37828940 = -414043460;    int EVfzIuTTyc64775958 = -759026070;    int EVfzIuTTyc36992692 = -927655212;    int EVfzIuTTyc6480178 = -692340820;    int EVfzIuTTyc42557243 = -145404006;    int EVfzIuTTyc29128300 = -347220988;    int EVfzIuTTyc88482470 = -614031898;    int EVfzIuTTyc84589815 = -668131447;    int EVfzIuTTyc61698202 = -183818101;    int EVfzIuTTyc38766715 = -696374002;    int EVfzIuTTyc33913086 = -538652079;    int EVfzIuTTyc94349343 = -292723085;    int EVfzIuTTyc16506626 = -272234657;    int EVfzIuTTyc45962515 = -850136649;    int EVfzIuTTyc38727011 = -6453517;    int EVfzIuTTyc33558735 = -127698850;    int EVfzIuTTyc89497366 = -808034370;    int EVfzIuTTyc71539319 = -663324144;    int EVfzIuTTyc90911901 = -702442343;    int EVfzIuTTyc15070461 = -511152551;    int EVfzIuTTyc19771949 = -888966900;    int EVfzIuTTyc63162415 = -311551464;    int EVfzIuTTyc97969249 = -965106922;    int EVfzIuTTyc89470854 = -204883556;    int EVfzIuTTyc66967080 = -134069071;    int EVfzIuTTyc57350811 = -411306911;    int EVfzIuTTyc14097323 = -489543024;    int EVfzIuTTyc87160356 = 76934921;    int EVfzIuTTyc75725344 = -504641981;    int EVfzIuTTyc71092888 = -994615465;    int EVfzIuTTyc85766039 = -878945226;    int EVfzIuTTyc7222887 = 21839590;    int EVfzIuTTyc6540197 = -339460527;    int EVfzIuTTyc75946560 = 99362551;    int EVfzIuTTyc74172863 = -201175821;    int EVfzIuTTyc63406363 = -197249664;    int EVfzIuTTyc13781510 = -679305193;    int EVfzIuTTyc41302642 = -385081560;    int EVfzIuTTyc29421288 = -993762677;    int EVfzIuTTyc30455166 = -953678220;    int EVfzIuTTyc7951536 = -191808118;    int EVfzIuTTyc8815720 = -49309454;    int EVfzIuTTyc30436611 = -438729815;    int EVfzIuTTyc95460440 = -892358354;    int EVfzIuTTyc8109502 = -366778201;    int EVfzIuTTyc41077924 = -644680723;    int EVfzIuTTyc62466787 = -998908956;    int EVfzIuTTyc78366450 = -30178360;    int EVfzIuTTyc3289664 = -407183063;    int EVfzIuTTyc97467770 = -602635076;    int EVfzIuTTyc7538272 = -19041991;    int EVfzIuTTyc77206040 = -237212114;    int EVfzIuTTyc90626604 = -859413395;    int EVfzIuTTyc81317279 = 78209965;    int EVfzIuTTyc38558592 = -215420778;    int EVfzIuTTyc74666525 = -2491996;    int EVfzIuTTyc66806709 = -793919148;    int EVfzIuTTyc47521838 = -622771656;    int EVfzIuTTyc39513098 = -458271750;    int EVfzIuTTyc85206431 = -734097095;    int EVfzIuTTyc15030977 = -857677965;    int EVfzIuTTyc1322114 = -590966820;    int EVfzIuTTyc8864471 = -63489467;    int EVfzIuTTyc90605313 = -189202636;    int EVfzIuTTyc53000676 = -706407801;     EVfzIuTTyc13177484 = EVfzIuTTyc55297228;     EVfzIuTTyc55297228 = EVfzIuTTyc32196405;     EVfzIuTTyc32196405 = EVfzIuTTyc31493443;     EVfzIuTTyc31493443 = EVfzIuTTyc2394348;     EVfzIuTTyc2394348 = EVfzIuTTyc36324224;     EVfzIuTTyc36324224 = EVfzIuTTyc38426288;     EVfzIuTTyc38426288 = EVfzIuTTyc29177032;     EVfzIuTTyc29177032 = EVfzIuTTyc6751850;     EVfzIuTTyc6751850 = EVfzIuTTyc68128526;     EVfzIuTTyc68128526 = EVfzIuTTyc34511399;     EVfzIuTTyc34511399 = EVfzIuTTyc88761392;     EVfzIuTTyc88761392 = EVfzIuTTyc2927937;     EVfzIuTTyc2927937 = EVfzIuTTyc34277101;     EVfzIuTTyc34277101 = EVfzIuTTyc12001804;     EVfzIuTTyc12001804 = EVfzIuTTyc28182322;     EVfzIuTTyc28182322 = EVfzIuTTyc50774202;     EVfzIuTTyc50774202 = EVfzIuTTyc47782820;     EVfzIuTTyc47782820 = EVfzIuTTyc71978531;     EVfzIuTTyc71978531 = EVfzIuTTyc59583466;     EVfzIuTTyc59583466 = EVfzIuTTyc96434006;     EVfzIuTTyc96434006 = EVfzIuTTyc93405534;     EVfzIuTTyc93405534 = EVfzIuTTyc92134812;     EVfzIuTTyc92134812 = EVfzIuTTyc34227156;     EVfzIuTTyc34227156 = EVfzIuTTyc42022588;     EVfzIuTTyc42022588 = EVfzIuTTyc35427268;     EVfzIuTTyc35427268 = EVfzIuTTyc78973412;     EVfzIuTTyc78973412 = EVfzIuTTyc24328966;     EVfzIuTTyc24328966 = EVfzIuTTyc42016675;     EVfzIuTTyc42016675 = EVfzIuTTyc31026505;     EVfzIuTTyc31026505 = EVfzIuTTyc97035637;     EVfzIuTTyc97035637 = EVfzIuTTyc48745360;     EVfzIuTTyc48745360 = EVfzIuTTyc81538506;     EVfzIuTTyc81538506 = EVfzIuTTyc96387740;     EVfzIuTTyc96387740 = EVfzIuTTyc58330541;     EVfzIuTTyc58330541 = EVfzIuTTyc37828940;     EVfzIuTTyc37828940 = EVfzIuTTyc64775958;     EVfzIuTTyc64775958 = EVfzIuTTyc36992692;     EVfzIuTTyc36992692 = EVfzIuTTyc6480178;     EVfzIuTTyc6480178 = EVfzIuTTyc42557243;     EVfzIuTTyc42557243 = EVfzIuTTyc29128300;     EVfzIuTTyc29128300 = EVfzIuTTyc88482470;     EVfzIuTTyc88482470 = EVfzIuTTyc84589815;     EVfzIuTTyc84589815 = EVfzIuTTyc61698202;     EVfzIuTTyc61698202 = EVfzIuTTyc38766715;     EVfzIuTTyc38766715 = EVfzIuTTyc33913086;     EVfzIuTTyc33913086 = EVfzIuTTyc94349343;     EVfzIuTTyc94349343 = EVfzIuTTyc16506626;     EVfzIuTTyc16506626 = EVfzIuTTyc45962515;     EVfzIuTTyc45962515 = EVfzIuTTyc38727011;     EVfzIuTTyc38727011 = EVfzIuTTyc33558735;     EVfzIuTTyc33558735 = EVfzIuTTyc89497366;     EVfzIuTTyc89497366 = EVfzIuTTyc71539319;     EVfzIuTTyc71539319 = EVfzIuTTyc90911901;     EVfzIuTTyc90911901 = EVfzIuTTyc15070461;     EVfzIuTTyc15070461 = EVfzIuTTyc19771949;     EVfzIuTTyc19771949 = EVfzIuTTyc63162415;     EVfzIuTTyc63162415 = EVfzIuTTyc97969249;     EVfzIuTTyc97969249 = EVfzIuTTyc89470854;     EVfzIuTTyc89470854 = EVfzIuTTyc66967080;     EVfzIuTTyc66967080 = EVfzIuTTyc57350811;     EVfzIuTTyc57350811 = EVfzIuTTyc14097323;     EVfzIuTTyc14097323 = EVfzIuTTyc87160356;     EVfzIuTTyc87160356 = EVfzIuTTyc75725344;     EVfzIuTTyc75725344 = EVfzIuTTyc71092888;     EVfzIuTTyc71092888 = EVfzIuTTyc85766039;     EVfzIuTTyc85766039 = EVfzIuTTyc7222887;     EVfzIuTTyc7222887 = EVfzIuTTyc6540197;     EVfzIuTTyc6540197 = EVfzIuTTyc75946560;     EVfzIuTTyc75946560 = EVfzIuTTyc74172863;     EVfzIuTTyc74172863 = EVfzIuTTyc63406363;     EVfzIuTTyc63406363 = EVfzIuTTyc13781510;     EVfzIuTTyc13781510 = EVfzIuTTyc41302642;     EVfzIuTTyc41302642 = EVfzIuTTyc29421288;     EVfzIuTTyc29421288 = EVfzIuTTyc30455166;     EVfzIuTTyc30455166 = EVfzIuTTyc7951536;     EVfzIuTTyc7951536 = EVfzIuTTyc8815720;     EVfzIuTTyc8815720 = EVfzIuTTyc30436611;     EVfzIuTTyc30436611 = EVfzIuTTyc95460440;     EVfzIuTTyc95460440 = EVfzIuTTyc8109502;     EVfzIuTTyc8109502 = EVfzIuTTyc41077924;     EVfzIuTTyc41077924 = EVfzIuTTyc62466787;     EVfzIuTTyc62466787 = EVfzIuTTyc78366450;     EVfzIuTTyc78366450 = EVfzIuTTyc3289664;     EVfzIuTTyc3289664 = EVfzIuTTyc97467770;     EVfzIuTTyc97467770 = EVfzIuTTyc7538272;     EVfzIuTTyc7538272 = EVfzIuTTyc77206040;     EVfzIuTTyc77206040 = EVfzIuTTyc90626604;     EVfzIuTTyc90626604 = EVfzIuTTyc81317279;     EVfzIuTTyc81317279 = EVfzIuTTyc38558592;     EVfzIuTTyc38558592 = EVfzIuTTyc74666525;     EVfzIuTTyc74666525 = EVfzIuTTyc66806709;     EVfzIuTTyc66806709 = EVfzIuTTyc47521838;     EVfzIuTTyc47521838 = EVfzIuTTyc39513098;     EVfzIuTTyc39513098 = EVfzIuTTyc85206431;     EVfzIuTTyc85206431 = EVfzIuTTyc15030977;     EVfzIuTTyc15030977 = EVfzIuTTyc1322114;     EVfzIuTTyc1322114 = EVfzIuTTyc8864471;     EVfzIuTTyc8864471 = EVfzIuTTyc90605313;     EVfzIuTTyc90605313 = EVfzIuTTyc53000676;     EVfzIuTTyc53000676 = EVfzIuTTyc13177484;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void kjiwvIWBYq48149441() {     int eYQgUONkOh35564601 = -340811764;    int eYQgUONkOh39644728 = 17357566;    int eYQgUONkOh60965683 = -793493121;    int eYQgUONkOh46500131 = -216721366;    int eYQgUONkOh568602 = -762529879;    int eYQgUONkOh51207340 = -643058901;    int eYQgUONkOh30582027 = -291845337;    int eYQgUONkOh4810891 = 92701698;    int eYQgUONkOh3914981 = -219171944;    int eYQgUONkOh59892111 = -664433159;    int eYQgUONkOh72053258 = -876105884;    int eYQgUONkOh44664341 = -890761441;    int eYQgUONkOh69936117 = -491120431;    int eYQgUONkOh76891853 = -632733883;    int eYQgUONkOh48737080 = -342696993;    int eYQgUONkOh72438234 = -337943276;    int eYQgUONkOh597554 = -884240224;    int eYQgUONkOh53759236 = -529165070;    int eYQgUONkOh13260162 = -274268808;    int eYQgUONkOh38099749 = -553533660;    int eYQgUONkOh97317113 = -897327324;    int eYQgUONkOh57220753 = -222207214;    int eYQgUONkOh95428135 = -898628508;    int eYQgUONkOh60726943 = -52725213;    int eYQgUONkOh77562835 = -626989404;    int eYQgUONkOh23730262 = -381298031;    int eYQgUONkOh26062833 = -28730106;    int eYQgUONkOh30240416 = -768769462;    int eYQgUONkOh48909236 = -857893699;    int eYQgUONkOh40183685 = -750629791;    int eYQgUONkOh41326443 = 91607487;    int eYQgUONkOh6286551 = -552607612;    int eYQgUONkOh62577176 = 8800767;    int eYQgUONkOh55776427 = -857069435;    int eYQgUONkOh34559906 = -168144100;    int eYQgUONkOh79521809 = -729438836;    int eYQgUONkOh58215671 = -491974022;    int eYQgUONkOh90618793 = -795280683;    int eYQgUONkOh54788309 = 6613148;    int eYQgUONkOh62742957 = -281628035;    int eYQgUONkOh57885671 = -850398654;    int eYQgUONkOh76990861 = -371940589;    int eYQgUONkOh90140007 = -555662031;    int eYQgUONkOh61142322 = -181181750;    int eYQgUONkOh6749982 = -505517480;    int eYQgUONkOh42013368 = -720881174;    int eYQgUONkOh6646742 = -871133337;    int eYQgUONkOh61105300 = -28769648;    int eYQgUONkOh40422420 = -799853347;    int eYQgUONkOh35810757 = -148473776;    int eYQgUONkOh42427951 = -482775127;    int eYQgUONkOh63348624 = -138853652;    int eYQgUONkOh49853723 = -335235374;    int eYQgUONkOh51198447 = -978123152;    int eYQgUONkOh50832328 = -651856666;    int eYQgUONkOh78343847 = -18604550;    int eYQgUONkOh44216592 = -84013927;    int eYQgUONkOh238741 = -640767909;    int eYQgUONkOh68937296 = -589731963;    int eYQgUONkOh76838340 = -281231848;    int eYQgUONkOh25144508 = -514328796;    int eYQgUONkOh341612 = -523075876;    int eYQgUONkOh55901655 = -49404604;    int eYQgUONkOh63731296 = -468542153;    int eYQgUONkOh18565668 = -656040646;    int eYQgUONkOh65766708 = -223498272;    int eYQgUONkOh82087164 = -799562209;    int eYQgUONkOh14159690 = -634050996;    int eYQgUONkOh42331948 = -364589783;    int eYQgUONkOh69215271 = -613258158;    int eYQgUONkOh14222564 = -845969255;    int eYQgUONkOh9978761 = 11040459;    int eYQgUONkOh98970927 = -435778219;    int eYQgUONkOh50517205 = -992640773;    int eYQgUONkOh80214078 = -703135007;    int eYQgUONkOh20326252 = -425386736;    int eYQgUONkOh67080746 = -666545184;    int eYQgUONkOh34285814 = -617446758;    int eYQgUONkOh53976961 = -547207733;    int eYQgUONkOh35549467 = -906108230;    int eYQgUONkOh17083521 = -510164694;    int eYQgUONkOh64957532 = -999960458;    int eYQgUONkOh89817995 = -968916115;    int eYQgUONkOh13098479 = -609419923;    int eYQgUONkOh97755733 = -167854665;    int eYQgUONkOh77977819 = -769538861;    int eYQgUONkOh56432828 = -117372238;    int eYQgUONkOh11378730 = -13076081;    int eYQgUONkOh4944100 = -105212770;    int eYQgUONkOh56216058 = -49539551;    int eYQgUONkOh35305217 = -545424910;    int eYQgUONkOh57976930 = -851206113;    int eYQgUONkOh21681498 = -105548721;    int eYQgUONkOh77949969 = -712155005;    int eYQgUONkOh37598450 = -767299239;    int eYQgUONkOh57544059 = -227322778;    int eYQgUONkOh21089207 = -222535986;    int eYQgUONkOh26408711 = 12880122;    int eYQgUONkOh42576654 = -525141104;    int eYQgUONkOh40983274 = -340811764;     eYQgUONkOh35564601 = eYQgUONkOh39644728;     eYQgUONkOh39644728 = eYQgUONkOh60965683;     eYQgUONkOh60965683 = eYQgUONkOh46500131;     eYQgUONkOh46500131 = eYQgUONkOh568602;     eYQgUONkOh568602 = eYQgUONkOh51207340;     eYQgUONkOh51207340 = eYQgUONkOh30582027;     eYQgUONkOh30582027 = eYQgUONkOh4810891;     eYQgUONkOh4810891 = eYQgUONkOh3914981;     eYQgUONkOh3914981 = eYQgUONkOh59892111;     eYQgUONkOh59892111 = eYQgUONkOh72053258;     eYQgUONkOh72053258 = eYQgUONkOh44664341;     eYQgUONkOh44664341 = eYQgUONkOh69936117;     eYQgUONkOh69936117 = eYQgUONkOh76891853;     eYQgUONkOh76891853 = eYQgUONkOh48737080;     eYQgUONkOh48737080 = eYQgUONkOh72438234;     eYQgUONkOh72438234 = eYQgUONkOh597554;     eYQgUONkOh597554 = eYQgUONkOh53759236;     eYQgUONkOh53759236 = eYQgUONkOh13260162;     eYQgUONkOh13260162 = eYQgUONkOh38099749;     eYQgUONkOh38099749 = eYQgUONkOh97317113;     eYQgUONkOh97317113 = eYQgUONkOh57220753;     eYQgUONkOh57220753 = eYQgUONkOh95428135;     eYQgUONkOh95428135 = eYQgUONkOh60726943;     eYQgUONkOh60726943 = eYQgUONkOh77562835;     eYQgUONkOh77562835 = eYQgUONkOh23730262;     eYQgUONkOh23730262 = eYQgUONkOh26062833;     eYQgUONkOh26062833 = eYQgUONkOh30240416;     eYQgUONkOh30240416 = eYQgUONkOh48909236;     eYQgUONkOh48909236 = eYQgUONkOh40183685;     eYQgUONkOh40183685 = eYQgUONkOh41326443;     eYQgUONkOh41326443 = eYQgUONkOh6286551;     eYQgUONkOh6286551 = eYQgUONkOh62577176;     eYQgUONkOh62577176 = eYQgUONkOh55776427;     eYQgUONkOh55776427 = eYQgUONkOh34559906;     eYQgUONkOh34559906 = eYQgUONkOh79521809;     eYQgUONkOh79521809 = eYQgUONkOh58215671;     eYQgUONkOh58215671 = eYQgUONkOh90618793;     eYQgUONkOh90618793 = eYQgUONkOh54788309;     eYQgUONkOh54788309 = eYQgUONkOh62742957;     eYQgUONkOh62742957 = eYQgUONkOh57885671;     eYQgUONkOh57885671 = eYQgUONkOh76990861;     eYQgUONkOh76990861 = eYQgUONkOh90140007;     eYQgUONkOh90140007 = eYQgUONkOh61142322;     eYQgUONkOh61142322 = eYQgUONkOh6749982;     eYQgUONkOh6749982 = eYQgUONkOh42013368;     eYQgUONkOh42013368 = eYQgUONkOh6646742;     eYQgUONkOh6646742 = eYQgUONkOh61105300;     eYQgUONkOh61105300 = eYQgUONkOh40422420;     eYQgUONkOh40422420 = eYQgUONkOh35810757;     eYQgUONkOh35810757 = eYQgUONkOh42427951;     eYQgUONkOh42427951 = eYQgUONkOh63348624;     eYQgUONkOh63348624 = eYQgUONkOh49853723;     eYQgUONkOh49853723 = eYQgUONkOh51198447;     eYQgUONkOh51198447 = eYQgUONkOh50832328;     eYQgUONkOh50832328 = eYQgUONkOh78343847;     eYQgUONkOh78343847 = eYQgUONkOh44216592;     eYQgUONkOh44216592 = eYQgUONkOh238741;     eYQgUONkOh238741 = eYQgUONkOh68937296;     eYQgUONkOh68937296 = eYQgUONkOh76838340;     eYQgUONkOh76838340 = eYQgUONkOh25144508;     eYQgUONkOh25144508 = eYQgUONkOh341612;     eYQgUONkOh341612 = eYQgUONkOh55901655;     eYQgUONkOh55901655 = eYQgUONkOh63731296;     eYQgUONkOh63731296 = eYQgUONkOh18565668;     eYQgUONkOh18565668 = eYQgUONkOh65766708;     eYQgUONkOh65766708 = eYQgUONkOh82087164;     eYQgUONkOh82087164 = eYQgUONkOh14159690;     eYQgUONkOh14159690 = eYQgUONkOh42331948;     eYQgUONkOh42331948 = eYQgUONkOh69215271;     eYQgUONkOh69215271 = eYQgUONkOh14222564;     eYQgUONkOh14222564 = eYQgUONkOh9978761;     eYQgUONkOh9978761 = eYQgUONkOh98970927;     eYQgUONkOh98970927 = eYQgUONkOh50517205;     eYQgUONkOh50517205 = eYQgUONkOh80214078;     eYQgUONkOh80214078 = eYQgUONkOh20326252;     eYQgUONkOh20326252 = eYQgUONkOh67080746;     eYQgUONkOh67080746 = eYQgUONkOh34285814;     eYQgUONkOh34285814 = eYQgUONkOh53976961;     eYQgUONkOh53976961 = eYQgUONkOh35549467;     eYQgUONkOh35549467 = eYQgUONkOh17083521;     eYQgUONkOh17083521 = eYQgUONkOh64957532;     eYQgUONkOh64957532 = eYQgUONkOh89817995;     eYQgUONkOh89817995 = eYQgUONkOh13098479;     eYQgUONkOh13098479 = eYQgUONkOh97755733;     eYQgUONkOh97755733 = eYQgUONkOh77977819;     eYQgUONkOh77977819 = eYQgUONkOh56432828;     eYQgUONkOh56432828 = eYQgUONkOh11378730;     eYQgUONkOh11378730 = eYQgUONkOh4944100;     eYQgUONkOh4944100 = eYQgUONkOh56216058;     eYQgUONkOh56216058 = eYQgUONkOh35305217;     eYQgUONkOh35305217 = eYQgUONkOh57976930;     eYQgUONkOh57976930 = eYQgUONkOh21681498;     eYQgUONkOh21681498 = eYQgUONkOh77949969;     eYQgUONkOh77949969 = eYQgUONkOh37598450;     eYQgUONkOh37598450 = eYQgUONkOh57544059;     eYQgUONkOh57544059 = eYQgUONkOh21089207;     eYQgUONkOh21089207 = eYQgUONkOh26408711;     eYQgUONkOh26408711 = eYQgUONkOh42576654;     eYQgUONkOh42576654 = eYQgUONkOh40983274;     eYQgUONkOh40983274 = eYQgUONkOh35564601;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XqXVumURBY84794416() {     int UQjVWcDNPV57951718 = 24784273;    int UQjVWcDNPV23992229 = 68814512;    int UQjVWcDNPV89734962 = 66853034;    int UQjVWcDNPV61506819 = -223128897;    int UQjVWcDNPV98742856 = -253586880;    int UQjVWcDNPV66090457 = -503667279;    int UQjVWcDNPV22737766 = -113832643;    int UQjVWcDNPV80444749 = -377894946;    int UQjVWcDNPV1078112 = -103367983;    int UQjVWcDNPV51655696 = -407174492;    int UQjVWcDNPV9595119 = -872730285;    int UQjVWcDNPV567291 = -41506734;    int UQjVWcDNPV36944298 = -9837751;    int UQjVWcDNPV19506606 = -60442640;    int UQjVWcDNPV85472357 = 29825294;    int UQjVWcDNPV16694147 = -719610819;    int UQjVWcDNPV50420905 = -61520044;    int UQjVWcDNPV59735652 = -980907761;    int UQjVWcDNPV54541793 = -409370933;    int UQjVWcDNPV16616033 = -806168114;    int UQjVWcDNPV98200220 = -888814633;    int UQjVWcDNPV21035972 = -726973528;    int UQjVWcDNPV98721459 = 25290900;    int UQjVWcDNPV87226730 = -616718070;    int UQjVWcDNPV13103082 = -248548528;    int UQjVWcDNPV12033257 = -825192255;    int UQjVWcDNPV73152253 = -886316599;    int UQjVWcDNPV36151866 = -557223915;    int UQjVWcDNPV55801797 = -102150818;    int UQjVWcDNPV49340864 = -670925658;    int UQjVWcDNPV85617248 = 10291335;    int UQjVWcDNPV63827741 = -104678967;    int UQjVWcDNPV43615847 = -520542728;    int UQjVWcDNPV15165115 = -81196285;    int UQjVWcDNPV10789271 = -231900523;    int UQjVWcDNPV21214679 = 55165787;    int UQjVWcDNPV51655383 = -224921973;    int UQjVWcDNPV44244894 = -662906155;    int UQjVWcDNPV3096441 = -394432884;    int UQjVWcDNPV82928672 = -417852064;    int UQjVWcDNPV86643041 = -253576320;    int UQjVWcDNPV65499253 = -129849280;    int UQjVWcDNPV95690199 = -443192615;    int UQjVWcDNPV60586441 = -178545399;    int UQjVWcDNPV74733248 = -314660958;    int UQjVWcDNPV50113650 = -903110269;    int UQjVWcDNPV18944139 = -349543590;    int UQjVWcDNPV5703975 = -885304639;    int UQjVWcDNPV34882325 = -749570046;    int UQjVWcDNPV32894503 = -290494035;    int UQjVWcDNPV51297168 = -837851404;    int UQjVWcDNPV37199881 = -569672934;    int UQjVWcDNPV28168126 = -7146605;    int UQjVWcDNPV11484993 = -153803961;    int UQjVWcDNPV86594195 = -792560781;    int UQjVWcDNPV36915747 = -248242200;    int UQjVWcDNPV25270770 = -956476389;    int UQjVWcDNPV2508233 = -316428896;    int UQjVWcDNPV48403737 = -974580370;    int UQjVWcDNPV86709599 = -428394625;    int UQjVWcDNPV92938204 = -617350681;    int UQjVWcDNPV86585900 = -556608729;    int UQjVWcDNPV24642953 = -175744129;    int UQjVWcDNPV51737248 = -432442326;    int UQjVWcDNPV66038447 = -317465827;    int UQjVWcDNPV45767377 = -668051318;    int UQjVWcDNPV56951443 = -520964007;    int UQjVWcDNPV21779183 = -928641466;    int UQjVWcDNPV8717336 = -828542117;    int UQjVWcDNPV64257679 = 74659506;    int UQjVWcDNPV65038764 = -394688846;    int UQjVWcDNPV6176012 = -398613889;    int UQjVWcDNPV56639212 = -486474878;    int UQjVWcDNPV71613121 = -991518869;    int UQjVWcDNPV29972991 = -452591795;    int UQjVWcDNPV32700968 = -658965353;    int UQjVWcDNPV25345773 = -183780913;    int UQjVWcDNPV38135018 = -796163701;    int UQjVWcDNPV12493482 = -202057113;    int UQjVWcDNPV62989432 = -345438259;    int UQjVWcDNPV93089118 = -375648665;    int UQjVWcDNPV67448278 = 98988040;    int UQjVWcDNPV1269541 = -807653870;    int UQjVWcDNPV22907295 = -811656783;    int UQjVWcDNPV98043696 = -833074255;    int UQjVWcDNPV48417367 = -420035731;    int UQjVWcDNPV35659616 = 2467637;    int UQjVWcDNPV32130855 = -266738767;    int UQjVWcDNPV28570919 = -288635505;    int UQjVWcDNPV73873523 = -983658324;    int UQjVWcDNPV95943908 = 11642176;    int UQjVWcDNPV49147151 = -908493077;    int UQjVWcDNPV95841156 = -688325786;    int UQjVWcDNPV16386841 = -966038259;    int UQjVWcDNPV89990467 = -800501384;    int UQjVWcDNPV57142 = -696967592;    int UQjVWcDNPV40856301 = -954105152;    int UQjVWcDNPV43952951 = 89249711;    int UQjVWcDNPV94547993 = -861079572;    int UQjVWcDNPV28965872 = 24784273;     UQjVWcDNPV57951718 = UQjVWcDNPV23992229;     UQjVWcDNPV23992229 = UQjVWcDNPV89734962;     UQjVWcDNPV89734962 = UQjVWcDNPV61506819;     UQjVWcDNPV61506819 = UQjVWcDNPV98742856;     UQjVWcDNPV98742856 = UQjVWcDNPV66090457;     UQjVWcDNPV66090457 = UQjVWcDNPV22737766;     UQjVWcDNPV22737766 = UQjVWcDNPV80444749;     UQjVWcDNPV80444749 = UQjVWcDNPV1078112;     UQjVWcDNPV1078112 = UQjVWcDNPV51655696;     UQjVWcDNPV51655696 = UQjVWcDNPV9595119;     UQjVWcDNPV9595119 = UQjVWcDNPV567291;     UQjVWcDNPV567291 = UQjVWcDNPV36944298;     UQjVWcDNPV36944298 = UQjVWcDNPV19506606;     UQjVWcDNPV19506606 = UQjVWcDNPV85472357;     UQjVWcDNPV85472357 = UQjVWcDNPV16694147;     UQjVWcDNPV16694147 = UQjVWcDNPV50420905;     UQjVWcDNPV50420905 = UQjVWcDNPV59735652;     UQjVWcDNPV59735652 = UQjVWcDNPV54541793;     UQjVWcDNPV54541793 = UQjVWcDNPV16616033;     UQjVWcDNPV16616033 = UQjVWcDNPV98200220;     UQjVWcDNPV98200220 = UQjVWcDNPV21035972;     UQjVWcDNPV21035972 = UQjVWcDNPV98721459;     UQjVWcDNPV98721459 = UQjVWcDNPV87226730;     UQjVWcDNPV87226730 = UQjVWcDNPV13103082;     UQjVWcDNPV13103082 = UQjVWcDNPV12033257;     UQjVWcDNPV12033257 = UQjVWcDNPV73152253;     UQjVWcDNPV73152253 = UQjVWcDNPV36151866;     UQjVWcDNPV36151866 = UQjVWcDNPV55801797;     UQjVWcDNPV55801797 = UQjVWcDNPV49340864;     UQjVWcDNPV49340864 = UQjVWcDNPV85617248;     UQjVWcDNPV85617248 = UQjVWcDNPV63827741;     UQjVWcDNPV63827741 = UQjVWcDNPV43615847;     UQjVWcDNPV43615847 = UQjVWcDNPV15165115;     UQjVWcDNPV15165115 = UQjVWcDNPV10789271;     UQjVWcDNPV10789271 = UQjVWcDNPV21214679;     UQjVWcDNPV21214679 = UQjVWcDNPV51655383;     UQjVWcDNPV51655383 = UQjVWcDNPV44244894;     UQjVWcDNPV44244894 = UQjVWcDNPV3096441;     UQjVWcDNPV3096441 = UQjVWcDNPV82928672;     UQjVWcDNPV82928672 = UQjVWcDNPV86643041;     UQjVWcDNPV86643041 = UQjVWcDNPV65499253;     UQjVWcDNPV65499253 = UQjVWcDNPV95690199;     UQjVWcDNPV95690199 = UQjVWcDNPV60586441;     UQjVWcDNPV60586441 = UQjVWcDNPV74733248;     UQjVWcDNPV74733248 = UQjVWcDNPV50113650;     UQjVWcDNPV50113650 = UQjVWcDNPV18944139;     UQjVWcDNPV18944139 = UQjVWcDNPV5703975;     UQjVWcDNPV5703975 = UQjVWcDNPV34882325;     UQjVWcDNPV34882325 = UQjVWcDNPV32894503;     UQjVWcDNPV32894503 = UQjVWcDNPV51297168;     UQjVWcDNPV51297168 = UQjVWcDNPV37199881;     UQjVWcDNPV37199881 = UQjVWcDNPV28168126;     UQjVWcDNPV28168126 = UQjVWcDNPV11484993;     UQjVWcDNPV11484993 = UQjVWcDNPV86594195;     UQjVWcDNPV86594195 = UQjVWcDNPV36915747;     UQjVWcDNPV36915747 = UQjVWcDNPV25270770;     UQjVWcDNPV25270770 = UQjVWcDNPV2508233;     UQjVWcDNPV2508233 = UQjVWcDNPV48403737;     UQjVWcDNPV48403737 = UQjVWcDNPV86709599;     UQjVWcDNPV86709599 = UQjVWcDNPV92938204;     UQjVWcDNPV92938204 = UQjVWcDNPV86585900;     UQjVWcDNPV86585900 = UQjVWcDNPV24642953;     UQjVWcDNPV24642953 = UQjVWcDNPV51737248;     UQjVWcDNPV51737248 = UQjVWcDNPV66038447;     UQjVWcDNPV66038447 = UQjVWcDNPV45767377;     UQjVWcDNPV45767377 = UQjVWcDNPV56951443;     UQjVWcDNPV56951443 = UQjVWcDNPV21779183;     UQjVWcDNPV21779183 = UQjVWcDNPV8717336;     UQjVWcDNPV8717336 = UQjVWcDNPV64257679;     UQjVWcDNPV64257679 = UQjVWcDNPV65038764;     UQjVWcDNPV65038764 = UQjVWcDNPV6176012;     UQjVWcDNPV6176012 = UQjVWcDNPV56639212;     UQjVWcDNPV56639212 = UQjVWcDNPV71613121;     UQjVWcDNPV71613121 = UQjVWcDNPV29972991;     UQjVWcDNPV29972991 = UQjVWcDNPV32700968;     UQjVWcDNPV32700968 = UQjVWcDNPV25345773;     UQjVWcDNPV25345773 = UQjVWcDNPV38135018;     UQjVWcDNPV38135018 = UQjVWcDNPV12493482;     UQjVWcDNPV12493482 = UQjVWcDNPV62989432;     UQjVWcDNPV62989432 = UQjVWcDNPV93089118;     UQjVWcDNPV93089118 = UQjVWcDNPV67448278;     UQjVWcDNPV67448278 = UQjVWcDNPV1269541;     UQjVWcDNPV1269541 = UQjVWcDNPV22907295;     UQjVWcDNPV22907295 = UQjVWcDNPV98043696;     UQjVWcDNPV98043696 = UQjVWcDNPV48417367;     UQjVWcDNPV48417367 = UQjVWcDNPV35659616;     UQjVWcDNPV35659616 = UQjVWcDNPV32130855;     UQjVWcDNPV32130855 = UQjVWcDNPV28570919;     UQjVWcDNPV28570919 = UQjVWcDNPV73873523;     UQjVWcDNPV73873523 = UQjVWcDNPV95943908;     UQjVWcDNPV95943908 = UQjVWcDNPV49147151;     UQjVWcDNPV49147151 = UQjVWcDNPV95841156;     UQjVWcDNPV95841156 = UQjVWcDNPV16386841;     UQjVWcDNPV16386841 = UQjVWcDNPV89990467;     UQjVWcDNPV89990467 = UQjVWcDNPV57142;     UQjVWcDNPV57142 = UQjVWcDNPV40856301;     UQjVWcDNPV40856301 = UQjVWcDNPV43952951;     UQjVWcDNPV43952951 = UQjVWcDNPV94547993;     UQjVWcDNPV94547993 = UQjVWcDNPV28965872;     UQjVWcDNPV28965872 = UQjVWcDNPV57951718;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zMJGUUBtqM21439391() {     int HNbQhKiLxl80338835 = -709619690;    int HNbQhKiLxl8339729 = -979728543;    int HNbQhKiLxl18504241 = -172800810;    int HNbQhKiLxl76513507 = -229536429;    int HNbQhKiLxl96917110 = -844643881;    int HNbQhKiLxl80973574 = -364275657;    int HNbQhKiLxl14893505 = 64180051;    int HNbQhKiLxl56078608 = -848491591;    int HNbQhKiLxl98241242 = 12435978;    int HNbQhKiLxl43419280 = -149915825;    int HNbQhKiLxl47136978 = -869354686;    int HNbQhKiLxl56470239 = -292252027;    int HNbQhKiLxl3952479 = -628555070;    int HNbQhKiLxl62121358 = -588151397;    int HNbQhKiLxl22207635 = -697652419;    int HNbQhKiLxl60950060 = -1278361;    int HNbQhKiLxl244257 = -338799864;    int HNbQhKiLxl65712069 = -332650452;    int HNbQhKiLxl95823423 = -544473058;    int HNbQhKiLxl95132315 = 41197432;    int HNbQhKiLxl99083327 = -880301941;    int HNbQhKiLxl84851189 = -131739841;    int HNbQhKiLxl2014783 = -150789692;    int HNbQhKiLxl13726518 = -80710927;    int HNbQhKiLxl48643329 = -970107653;    int HNbQhKiLxl336252 = -169086479;    int HNbQhKiLxl20241674 = -643903091;    int HNbQhKiLxl42063316 = -345678369;    int HNbQhKiLxl62694357 = -446407937;    int HNbQhKiLxl58498043 = -591221525;    int HNbQhKiLxl29908053 = -71024817;    int HNbQhKiLxl21368933 = -756750321;    int HNbQhKiLxl24654518 = 50113778;    int HNbQhKiLxl74553802 = -405323135;    int HNbQhKiLxl87018634 = -295656945;    int HNbQhKiLxl62907547 = -260229589;    int HNbQhKiLxl45095095 = 42130076;    int HNbQhKiLxl97870994 = -530531627;    int HNbQhKiLxl51404572 = -795478915;    int HNbQhKiLxl3114387 = -554076093;    int HNbQhKiLxl15400413 = -756753986;    int HNbQhKiLxl54007644 = -987757971;    int HNbQhKiLxl1240391 = -330723198;    int HNbQhKiLxl60030560 = -175909048;    int HNbQhKiLxl42716515 = -123804435;    int HNbQhKiLxl58213932 = 14660635;    int HNbQhKiLxl31241536 = -927953843;    int HNbQhKiLxl50302649 = -641839630;    int HNbQhKiLxl29342230 = -699286744;    int HNbQhKiLxl29978248 = -432514294;    int HNbQhKiLxl60166384 = -92927681;    int HNbQhKiLxl11051139 = 99507784;    int HNbQhKiLxl6482529 = -779057835;    int HNbQhKiLxl71771537 = -429484770;    int HNbQhKiLxl22356063 = -933264896;    int HNbQhKiLxl95487645 = -477879850;    int HNbQhKiLxl6324947 = -728938852;    int HNbQhKiLxl4777724 = 7910117;    int HNbQhKiLxl27870179 = -259428777;    int HNbQhKiLxl96580859 = -575557403;    int HNbQhKiLxl60731900 = -720372566;    int HNbQhKiLxl72830189 = -590141581;    int HNbQhKiLxl93384250 = -302083654;    int HNbQhKiLxl39743199 = -396342498;    int HNbQhKiLxl13511227 = 21108992;    int HNbQhKiLxl25768046 = -12604365;    int HNbQhKiLxl31815722 = -242365805;    int HNbQhKiLxl29398676 = -123231936;    int HNbQhKiLxl75102723 = -192494452;    int HNbQhKiLxl59300087 = -337422831;    int HNbQhKiLxl15854965 = 56591563;    int HNbQhKiLxl2373263 = -808268237;    int HNbQhKiLxl14307498 = -537171537;    int HNbQhKiLxl92709037 = -990396965;    int HNbQhKiLxl79731903 = -202048582;    int HNbQhKiLxl45075683 = -892543971;    int HNbQhKiLxl83610798 = -801016643;    int HNbQhKiLxl41984222 = -974880644;    int HNbQhKiLxl71010002 = -956906492;    int HNbQhKiLxl90429396 = -884768288;    int HNbQhKiLxl69094715 = -241132636;    int HNbQhKiLxl69939024 = 97936538;    int HNbQhKiLxl12721086 = -646391625;    int HNbQhKiLxl32716110 = 86106356;    int HNbQhKiLxl98331659 = -398293845;    int HNbQhKiLxl18856915 = -70532601;    int HNbQhKiLxl14886404 = -977692487;    int HNbQhKiLxl52882980 = -520401453;    int HNbQhKiLxl52197739 = -472058240;    int HNbQhKiLxl91530989 = -817777096;    int HNbQhKiLxl56582601 = -531290738;    int HNbQhKiLxl40317372 = -965780042;    int HNbQhKiLxl70000816 = -171102851;    int HNbQhKiLxl54823712 = -119921513;    int HNbQhKiLxl42382486 = -833703528;    int HNbQhKiLxl42570223 = -66612405;    int HNbQhKiLxl60623394 = -585674318;    int HNbQhKiLxl61497192 = -934380701;    int HNbQhKiLxl46519333 = -97018040;    int HNbQhKiLxl16948470 = -709619690;     HNbQhKiLxl80338835 = HNbQhKiLxl8339729;     HNbQhKiLxl8339729 = HNbQhKiLxl18504241;     HNbQhKiLxl18504241 = HNbQhKiLxl76513507;     HNbQhKiLxl76513507 = HNbQhKiLxl96917110;     HNbQhKiLxl96917110 = HNbQhKiLxl80973574;     HNbQhKiLxl80973574 = HNbQhKiLxl14893505;     HNbQhKiLxl14893505 = HNbQhKiLxl56078608;     HNbQhKiLxl56078608 = HNbQhKiLxl98241242;     HNbQhKiLxl98241242 = HNbQhKiLxl43419280;     HNbQhKiLxl43419280 = HNbQhKiLxl47136978;     HNbQhKiLxl47136978 = HNbQhKiLxl56470239;     HNbQhKiLxl56470239 = HNbQhKiLxl3952479;     HNbQhKiLxl3952479 = HNbQhKiLxl62121358;     HNbQhKiLxl62121358 = HNbQhKiLxl22207635;     HNbQhKiLxl22207635 = HNbQhKiLxl60950060;     HNbQhKiLxl60950060 = HNbQhKiLxl244257;     HNbQhKiLxl244257 = HNbQhKiLxl65712069;     HNbQhKiLxl65712069 = HNbQhKiLxl95823423;     HNbQhKiLxl95823423 = HNbQhKiLxl95132315;     HNbQhKiLxl95132315 = HNbQhKiLxl99083327;     HNbQhKiLxl99083327 = HNbQhKiLxl84851189;     HNbQhKiLxl84851189 = HNbQhKiLxl2014783;     HNbQhKiLxl2014783 = HNbQhKiLxl13726518;     HNbQhKiLxl13726518 = HNbQhKiLxl48643329;     HNbQhKiLxl48643329 = HNbQhKiLxl336252;     HNbQhKiLxl336252 = HNbQhKiLxl20241674;     HNbQhKiLxl20241674 = HNbQhKiLxl42063316;     HNbQhKiLxl42063316 = HNbQhKiLxl62694357;     HNbQhKiLxl62694357 = HNbQhKiLxl58498043;     HNbQhKiLxl58498043 = HNbQhKiLxl29908053;     HNbQhKiLxl29908053 = HNbQhKiLxl21368933;     HNbQhKiLxl21368933 = HNbQhKiLxl24654518;     HNbQhKiLxl24654518 = HNbQhKiLxl74553802;     HNbQhKiLxl74553802 = HNbQhKiLxl87018634;     HNbQhKiLxl87018634 = HNbQhKiLxl62907547;     HNbQhKiLxl62907547 = HNbQhKiLxl45095095;     HNbQhKiLxl45095095 = HNbQhKiLxl97870994;     HNbQhKiLxl97870994 = HNbQhKiLxl51404572;     HNbQhKiLxl51404572 = HNbQhKiLxl3114387;     HNbQhKiLxl3114387 = HNbQhKiLxl15400413;     HNbQhKiLxl15400413 = HNbQhKiLxl54007644;     HNbQhKiLxl54007644 = HNbQhKiLxl1240391;     HNbQhKiLxl1240391 = HNbQhKiLxl60030560;     HNbQhKiLxl60030560 = HNbQhKiLxl42716515;     HNbQhKiLxl42716515 = HNbQhKiLxl58213932;     HNbQhKiLxl58213932 = HNbQhKiLxl31241536;     HNbQhKiLxl31241536 = HNbQhKiLxl50302649;     HNbQhKiLxl50302649 = HNbQhKiLxl29342230;     HNbQhKiLxl29342230 = HNbQhKiLxl29978248;     HNbQhKiLxl29978248 = HNbQhKiLxl60166384;     HNbQhKiLxl60166384 = HNbQhKiLxl11051139;     HNbQhKiLxl11051139 = HNbQhKiLxl6482529;     HNbQhKiLxl6482529 = HNbQhKiLxl71771537;     HNbQhKiLxl71771537 = HNbQhKiLxl22356063;     HNbQhKiLxl22356063 = HNbQhKiLxl95487645;     HNbQhKiLxl95487645 = HNbQhKiLxl6324947;     HNbQhKiLxl6324947 = HNbQhKiLxl4777724;     HNbQhKiLxl4777724 = HNbQhKiLxl27870179;     HNbQhKiLxl27870179 = HNbQhKiLxl96580859;     HNbQhKiLxl96580859 = HNbQhKiLxl60731900;     HNbQhKiLxl60731900 = HNbQhKiLxl72830189;     HNbQhKiLxl72830189 = HNbQhKiLxl93384250;     HNbQhKiLxl93384250 = HNbQhKiLxl39743199;     HNbQhKiLxl39743199 = HNbQhKiLxl13511227;     HNbQhKiLxl13511227 = HNbQhKiLxl25768046;     HNbQhKiLxl25768046 = HNbQhKiLxl31815722;     HNbQhKiLxl31815722 = HNbQhKiLxl29398676;     HNbQhKiLxl29398676 = HNbQhKiLxl75102723;     HNbQhKiLxl75102723 = HNbQhKiLxl59300087;     HNbQhKiLxl59300087 = HNbQhKiLxl15854965;     HNbQhKiLxl15854965 = HNbQhKiLxl2373263;     HNbQhKiLxl2373263 = HNbQhKiLxl14307498;     HNbQhKiLxl14307498 = HNbQhKiLxl92709037;     HNbQhKiLxl92709037 = HNbQhKiLxl79731903;     HNbQhKiLxl79731903 = HNbQhKiLxl45075683;     HNbQhKiLxl45075683 = HNbQhKiLxl83610798;     HNbQhKiLxl83610798 = HNbQhKiLxl41984222;     HNbQhKiLxl41984222 = HNbQhKiLxl71010002;     HNbQhKiLxl71010002 = HNbQhKiLxl90429396;     HNbQhKiLxl90429396 = HNbQhKiLxl69094715;     HNbQhKiLxl69094715 = HNbQhKiLxl69939024;     HNbQhKiLxl69939024 = HNbQhKiLxl12721086;     HNbQhKiLxl12721086 = HNbQhKiLxl32716110;     HNbQhKiLxl32716110 = HNbQhKiLxl98331659;     HNbQhKiLxl98331659 = HNbQhKiLxl18856915;     HNbQhKiLxl18856915 = HNbQhKiLxl14886404;     HNbQhKiLxl14886404 = HNbQhKiLxl52882980;     HNbQhKiLxl52882980 = HNbQhKiLxl52197739;     HNbQhKiLxl52197739 = HNbQhKiLxl91530989;     HNbQhKiLxl91530989 = HNbQhKiLxl56582601;     HNbQhKiLxl56582601 = HNbQhKiLxl40317372;     HNbQhKiLxl40317372 = HNbQhKiLxl70000816;     HNbQhKiLxl70000816 = HNbQhKiLxl54823712;     HNbQhKiLxl54823712 = HNbQhKiLxl42382486;     HNbQhKiLxl42382486 = HNbQhKiLxl42570223;     HNbQhKiLxl42570223 = HNbQhKiLxl60623394;     HNbQhKiLxl60623394 = HNbQhKiLxl61497192;     HNbQhKiLxl61497192 = HNbQhKiLxl46519333;     HNbQhKiLxl46519333 = HNbQhKiLxl16948470;     HNbQhKiLxl16948470 = HNbQhKiLxl80338835;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OSjHMleZbf58084365() {     int AAPnbwJtVj2725953 = -344023653;    int AAPnbwJtVj92687229 = -928271598;    int AAPnbwJtVj47273520 = -412454654;    int AAPnbwJtVj91520196 = -235943961;    int AAPnbwJtVj95091364 = -335700883;    int AAPnbwJtVj95856691 = -224884035;    int AAPnbwJtVj7049244 = -857807256;    int AAPnbwJtVj31712467 = -219088235;    int AAPnbwJtVj95404373 = -971760061;    int AAPnbwJtVj35182865 = -992657157;    int AAPnbwJtVj84678837 = -865979086;    int AAPnbwJtVj12373189 = -542997321;    int AAPnbwJtVj70960658 = -147272390;    int AAPnbwJtVj4736111 = -15860154;    int AAPnbwJtVj58942911 = -325130132;    int AAPnbwJtVj5205973 = -382945904;    int AAPnbwJtVj50067609 = -616079683;    int AAPnbwJtVj71688485 = -784393142;    int AAPnbwJtVj37105055 = -679575183;    int AAPnbwJtVj73648599 = -211437021;    int AAPnbwJtVj99966434 = -871789250;    int AAPnbwJtVj48666408 = -636506154;    int AAPnbwJtVj5308106 = -326870284;    int AAPnbwJtVj40226305 = -644703784;    int AAPnbwJtVj84183575 = -591666777;    int AAPnbwJtVj88639245 = -612980703;    int AAPnbwJtVj67331094 = -401489584;    int AAPnbwJtVj47974766 = -134132822;    int AAPnbwJtVj69586918 = -790665056;    int AAPnbwJtVj67655223 = -511517391;    int AAPnbwJtVj74198858 = -152340969;    int AAPnbwJtVj78910123 = -308821676;    int AAPnbwJtVj5693188 = -479229717;    int AAPnbwJtVj33942489 = -729449985;    int AAPnbwJtVj63247999 = -359413368;    int AAPnbwJtVj4600417 = -575624966;    int AAPnbwJtVj38534807 = -790817876;    int AAPnbwJtVj51497096 = -398157099;    int AAPnbwJtVj99712702 = -96524947;    int AAPnbwJtVj23300101 = -690300123;    int AAPnbwJtVj44157784 = -159931652;    int AAPnbwJtVj42516036 = -745666662;    int AAPnbwJtVj6790583 = -218253782;    int AAPnbwJtVj59474679 = -173272697;    int AAPnbwJtVj10699782 = 67052087;    int AAPnbwJtVj66314214 = -167568460;    int AAPnbwJtVj43538933 = -406364096;    int AAPnbwJtVj94901324 = -398374621;    int AAPnbwJtVj23802135 = -649003442;    int AAPnbwJtVj27061994 = -574534553;    int AAPnbwJtVj69035601 = -448003958;    int AAPnbwJtVj84902395 = -331311498;    int AAPnbwJtVj84796932 = -450969065;    int AAPnbwJtVj32058083 = -705165579;    int AAPnbwJtVj58117930 = 26030989;    int AAPnbwJtVj54059545 = -707517499;    int AAPnbwJtVj87379124 = -501401315;    int AAPnbwJtVj7047215 = -767750870;    int AAPnbwJtVj7336621 = -644277184;    int AAPnbwJtVj6452119 = -722720180;    int AAPnbwJtVj28525597 = -823394451;    int AAPnbwJtVj59074478 = -623674434;    int AAPnbwJtVj62125548 = -428423179;    int AAPnbwJtVj27749151 = -360242671;    int AAPnbwJtVj60984006 = -740316189;    int AAPnbwJtVj5768714 = -457157411;    int AAPnbwJtVj6680001 = 36232396;    int AAPnbwJtVj37018170 = -417822405;    int AAPnbwJtVj41488111 = -656446786;    int AAPnbwJtVj54342495 = -749505167;    int AAPnbwJtVj66671165 = -592128028;    int AAPnbwJtVj98570512 = -117922585;    int AAPnbwJtVj71975782 = -587868196;    int AAPnbwJtVj13804954 = -989275061;    int AAPnbwJtVj29490816 = 48494630;    int AAPnbwJtVj57450399 = -26122588;    int AAPnbwJtVj41875825 = -318252372;    int AAPnbwJtVj45833426 = -53597587;    int AAPnbwJtVj29526523 = -611755872;    int AAPnbwJtVj17869362 = -324098317;    int AAPnbwJtVj45100313 = -106616607;    int AAPnbwJtVj72429770 = 96885036;    int AAPnbwJtVj24172631 = -485129381;    int AAPnbwJtVj42524925 = -116130504;    int AAPnbwJtVj98619622 = 36486566;    int AAPnbwJtVj89296463 = -821029471;    int AAPnbwJtVj94113191 = -857852612;    int AAPnbwJtVj73635105 = -774064139;    int AAPnbwJtVj75824559 = -655480974;    int AAPnbwJtVj9188455 = -651895869;    int AAPnbwJtVj17221293 = 25776349;    int AAPnbwJtVj31487592 = 76932993;    int AAPnbwJtVj44160475 = -753879916;    int AAPnbwJtVj93260584 = -373804768;    int AAPnbwJtVj94774504 = -866905672;    int AAPnbwJtVj85083305 = -536257219;    int AAPnbwJtVj80390487 = -217243484;    int AAPnbwJtVj79041432 = -858011112;    int AAPnbwJtVj98490672 = -432956508;    int AAPnbwJtVj4931068 = -344023653;     AAPnbwJtVj2725953 = AAPnbwJtVj92687229;     AAPnbwJtVj92687229 = AAPnbwJtVj47273520;     AAPnbwJtVj47273520 = AAPnbwJtVj91520196;     AAPnbwJtVj91520196 = AAPnbwJtVj95091364;     AAPnbwJtVj95091364 = AAPnbwJtVj95856691;     AAPnbwJtVj95856691 = AAPnbwJtVj7049244;     AAPnbwJtVj7049244 = AAPnbwJtVj31712467;     AAPnbwJtVj31712467 = AAPnbwJtVj95404373;     AAPnbwJtVj95404373 = AAPnbwJtVj35182865;     AAPnbwJtVj35182865 = AAPnbwJtVj84678837;     AAPnbwJtVj84678837 = AAPnbwJtVj12373189;     AAPnbwJtVj12373189 = AAPnbwJtVj70960658;     AAPnbwJtVj70960658 = AAPnbwJtVj4736111;     AAPnbwJtVj4736111 = AAPnbwJtVj58942911;     AAPnbwJtVj58942911 = AAPnbwJtVj5205973;     AAPnbwJtVj5205973 = AAPnbwJtVj50067609;     AAPnbwJtVj50067609 = AAPnbwJtVj71688485;     AAPnbwJtVj71688485 = AAPnbwJtVj37105055;     AAPnbwJtVj37105055 = AAPnbwJtVj73648599;     AAPnbwJtVj73648599 = AAPnbwJtVj99966434;     AAPnbwJtVj99966434 = AAPnbwJtVj48666408;     AAPnbwJtVj48666408 = AAPnbwJtVj5308106;     AAPnbwJtVj5308106 = AAPnbwJtVj40226305;     AAPnbwJtVj40226305 = AAPnbwJtVj84183575;     AAPnbwJtVj84183575 = AAPnbwJtVj88639245;     AAPnbwJtVj88639245 = AAPnbwJtVj67331094;     AAPnbwJtVj67331094 = AAPnbwJtVj47974766;     AAPnbwJtVj47974766 = AAPnbwJtVj69586918;     AAPnbwJtVj69586918 = AAPnbwJtVj67655223;     AAPnbwJtVj67655223 = AAPnbwJtVj74198858;     AAPnbwJtVj74198858 = AAPnbwJtVj78910123;     AAPnbwJtVj78910123 = AAPnbwJtVj5693188;     AAPnbwJtVj5693188 = AAPnbwJtVj33942489;     AAPnbwJtVj33942489 = AAPnbwJtVj63247999;     AAPnbwJtVj63247999 = AAPnbwJtVj4600417;     AAPnbwJtVj4600417 = AAPnbwJtVj38534807;     AAPnbwJtVj38534807 = AAPnbwJtVj51497096;     AAPnbwJtVj51497096 = AAPnbwJtVj99712702;     AAPnbwJtVj99712702 = AAPnbwJtVj23300101;     AAPnbwJtVj23300101 = AAPnbwJtVj44157784;     AAPnbwJtVj44157784 = AAPnbwJtVj42516036;     AAPnbwJtVj42516036 = AAPnbwJtVj6790583;     AAPnbwJtVj6790583 = AAPnbwJtVj59474679;     AAPnbwJtVj59474679 = AAPnbwJtVj10699782;     AAPnbwJtVj10699782 = AAPnbwJtVj66314214;     AAPnbwJtVj66314214 = AAPnbwJtVj43538933;     AAPnbwJtVj43538933 = AAPnbwJtVj94901324;     AAPnbwJtVj94901324 = AAPnbwJtVj23802135;     AAPnbwJtVj23802135 = AAPnbwJtVj27061994;     AAPnbwJtVj27061994 = AAPnbwJtVj69035601;     AAPnbwJtVj69035601 = AAPnbwJtVj84902395;     AAPnbwJtVj84902395 = AAPnbwJtVj84796932;     AAPnbwJtVj84796932 = AAPnbwJtVj32058083;     AAPnbwJtVj32058083 = AAPnbwJtVj58117930;     AAPnbwJtVj58117930 = AAPnbwJtVj54059545;     AAPnbwJtVj54059545 = AAPnbwJtVj87379124;     AAPnbwJtVj87379124 = AAPnbwJtVj7047215;     AAPnbwJtVj7047215 = AAPnbwJtVj7336621;     AAPnbwJtVj7336621 = AAPnbwJtVj6452119;     AAPnbwJtVj6452119 = AAPnbwJtVj28525597;     AAPnbwJtVj28525597 = AAPnbwJtVj59074478;     AAPnbwJtVj59074478 = AAPnbwJtVj62125548;     AAPnbwJtVj62125548 = AAPnbwJtVj27749151;     AAPnbwJtVj27749151 = AAPnbwJtVj60984006;     AAPnbwJtVj60984006 = AAPnbwJtVj5768714;     AAPnbwJtVj5768714 = AAPnbwJtVj6680001;     AAPnbwJtVj6680001 = AAPnbwJtVj37018170;     AAPnbwJtVj37018170 = AAPnbwJtVj41488111;     AAPnbwJtVj41488111 = AAPnbwJtVj54342495;     AAPnbwJtVj54342495 = AAPnbwJtVj66671165;     AAPnbwJtVj66671165 = AAPnbwJtVj98570512;     AAPnbwJtVj98570512 = AAPnbwJtVj71975782;     AAPnbwJtVj71975782 = AAPnbwJtVj13804954;     AAPnbwJtVj13804954 = AAPnbwJtVj29490816;     AAPnbwJtVj29490816 = AAPnbwJtVj57450399;     AAPnbwJtVj57450399 = AAPnbwJtVj41875825;     AAPnbwJtVj41875825 = AAPnbwJtVj45833426;     AAPnbwJtVj45833426 = AAPnbwJtVj29526523;     AAPnbwJtVj29526523 = AAPnbwJtVj17869362;     AAPnbwJtVj17869362 = AAPnbwJtVj45100313;     AAPnbwJtVj45100313 = AAPnbwJtVj72429770;     AAPnbwJtVj72429770 = AAPnbwJtVj24172631;     AAPnbwJtVj24172631 = AAPnbwJtVj42524925;     AAPnbwJtVj42524925 = AAPnbwJtVj98619622;     AAPnbwJtVj98619622 = AAPnbwJtVj89296463;     AAPnbwJtVj89296463 = AAPnbwJtVj94113191;     AAPnbwJtVj94113191 = AAPnbwJtVj73635105;     AAPnbwJtVj73635105 = AAPnbwJtVj75824559;     AAPnbwJtVj75824559 = AAPnbwJtVj9188455;     AAPnbwJtVj9188455 = AAPnbwJtVj17221293;     AAPnbwJtVj17221293 = AAPnbwJtVj31487592;     AAPnbwJtVj31487592 = AAPnbwJtVj44160475;     AAPnbwJtVj44160475 = AAPnbwJtVj93260584;     AAPnbwJtVj93260584 = AAPnbwJtVj94774504;     AAPnbwJtVj94774504 = AAPnbwJtVj85083305;     AAPnbwJtVj85083305 = AAPnbwJtVj80390487;     AAPnbwJtVj80390487 = AAPnbwJtVj79041432;     AAPnbwJtVj79041432 = AAPnbwJtVj98490672;     AAPnbwJtVj98490672 = AAPnbwJtVj4931068;     AAPnbwJtVj4931068 = AAPnbwJtVj2725953;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void kMJsZJAYOO94729339() {     int RRTGqBRqXy25113071 = 21572384;    int RRTGqBRqXy77034729 = -876814652;    int RRTGqBRqXy76042798 = -652108498;    int RRTGqBRqXy6526885 = -242351492;    int RRTGqBRqXy93265618 = -926757884;    int RRTGqBRqXy10739809 = -85492413;    int RRTGqBRqXy99204982 = -679794562;    int RRTGqBRqXy7346326 = -689684879;    int RRTGqBRqXy92567505 = -855956101;    int RRTGqBRqXy26946450 = -735398490;    int RRTGqBRqXy22220697 = -862603487;    int RRTGqBRqXy68276137 = -793742614;    int RRTGqBRqXy37968839 = -765989710;    int RRTGqBRqXy47350862 = -543568911;    int RRTGqBRqXy95678188 = 47392154;    int RRTGqBRqXy49461885 = -764613446;    int RRTGqBRqXy99890960 = -893359503;    int RRTGqBRqXy77664901 = -136135833;    int RRTGqBRqXy78386685 = -814677309;    int RRTGqBRqXy52164882 = -464071475;    int RRTGqBRqXy849542 = -863276558;    int RRTGqBRqXy12481627 = -41272467;    int RRTGqBRqXy8601429 = -502950876;    int RRTGqBRqXy66726092 = -108696642;    int RRTGqBRqXy19723823 = -213225902;    int RRTGqBRqXy76942240 = 43125073;    int RRTGqBRqXy14420515 = -159076077;    int RRTGqBRqXy53886216 = 77412724;    int RRTGqBRqXy76479479 = -34922176;    int RRTGqBRqXy76812402 = -431813258;    int RRTGqBRqXy18489664 = -233657121;    int RRTGqBRqXy36451314 = -960893031;    int RRTGqBRqXy86731858 = 91426788;    int RRTGqBRqXy93331176 = 46423165;    int RRTGqBRqXy39477364 = -423169790;    int RRTGqBRqXy46293286 = -891020342;    int RRTGqBRqXy31974520 = -523765827;    int RRTGqBRqXy5123197 = -265782571;    int RRTGqBRqXy48020834 = -497570979;    int RRTGqBRqXy43485816 = -826524152;    int RRTGqBRqXy72915154 = -663109318;    int RRTGqBRqXy31024427 = -503575353;    int RRTGqBRqXy12340775 = -105784366;    int RRTGqBRqXy58918799 = -170636346;    int RRTGqBRqXy78683048 = -842091391;    int RRTGqBRqXy74414495 = -349797556;    int RRTGqBRqXy55836331 = -984774349;    int RRTGqBRqXy39499999 = -154909612;    int RRTGqBRqXy18262040 = -598720141;    int RRTGqBRqXy24145740 = -716554812;    int RRTGqBRqXy77904817 = -803080235;    int RRTGqBRqXy58753652 = -762130780;    int RRTGqBRqXy63111335 = -122880295;    int RRTGqBRqXy92344627 = -980846387;    int RRTGqBRqXy93879798 = -114673127;    int RRTGqBRqXy12631444 = -937155149;    int RRTGqBRqXy68433301 = -273863777;    int RRTGqBRqXy9316707 = -443411857;    int RRTGqBRqXy86803062 = 70874409;    int RRTGqBRqXy16323379 = -869882958;    int RRTGqBRqXy96319293 = -926416336;    int RRTGqBRqXy45318767 = -657207286;    int RRTGqBRqXy30866846 = -554762704;    int RRTGqBRqXy15755103 = -324142843;    int RRTGqBRqXy8456786 = -401741370;    int RRTGqBRqXy85769382 = -901710457;    int RRTGqBRqXy81544279 = -785169402;    int RRTGqBRqXy44637663 = -712412875;    int RRTGqBRqXy7873499 = -20399121;    int RRTGqBRqXy49384903 = -61587504;    int RRTGqBRqXy17487365 = -140847619;    int RRTGqBRqXy94767763 = -527576933;    int RRTGqBRqXy29644067 = -638564854;    int RRTGqBRqXy34900870 = -988153157;    int RRTGqBRqXy79249728 = -800962157;    int RRTGqBRqXy69825115 = -259701205;    int RRTGqBRqXy140852 = -935488102;    int RRTGqBRqXy49682630 = -232314530;    int RRTGqBRqXy88043043 = -266605251;    int RRTGqBRqXy45309327 = -863428347;    int RRTGqBRqXy21105910 = 27899422;    int RRTGqBRqXy74920516 = 95833534;    int RRTGqBRqXy35624176 = -323867136;    int RRTGqBRqXy52333740 = -318367364;    int RRTGqBRqXy98907585 = -628733024;    int RRTGqBRqXy59736011 = -471526341;    int RRTGqBRqXy73339979 = -738012736;    int RRTGqBRqXy94387230 = 72273175;    int RRTGqBRqXy99451378 = -838903709;    int RRTGqBRqXy26845921 = -486014642;    int RRTGqBRqXy77859984 = -517156565;    int RRTGqBRqXy22657813 = 19646029;    int RRTGqBRqXy18320135 = -236656981;    int RRTGqBRqXy31697456 = -627688022;    int RRTGqBRqXy47166522 = -900107816;    int RRTGqBRqXy27596388 = 94097968;    int RRTGqBRqXy157581 = -948812650;    int RRTGqBRqXy96585672 = -781641523;    int RRTGqBRqXy50462013 = -768894976;    int RRTGqBRqXy92913665 = 21572384;     RRTGqBRqXy25113071 = RRTGqBRqXy77034729;     RRTGqBRqXy77034729 = RRTGqBRqXy76042798;     RRTGqBRqXy76042798 = RRTGqBRqXy6526885;     RRTGqBRqXy6526885 = RRTGqBRqXy93265618;     RRTGqBRqXy93265618 = RRTGqBRqXy10739809;     RRTGqBRqXy10739809 = RRTGqBRqXy99204982;     RRTGqBRqXy99204982 = RRTGqBRqXy7346326;     RRTGqBRqXy7346326 = RRTGqBRqXy92567505;     RRTGqBRqXy92567505 = RRTGqBRqXy26946450;     RRTGqBRqXy26946450 = RRTGqBRqXy22220697;     RRTGqBRqXy22220697 = RRTGqBRqXy68276137;     RRTGqBRqXy68276137 = RRTGqBRqXy37968839;     RRTGqBRqXy37968839 = RRTGqBRqXy47350862;     RRTGqBRqXy47350862 = RRTGqBRqXy95678188;     RRTGqBRqXy95678188 = RRTGqBRqXy49461885;     RRTGqBRqXy49461885 = RRTGqBRqXy99890960;     RRTGqBRqXy99890960 = RRTGqBRqXy77664901;     RRTGqBRqXy77664901 = RRTGqBRqXy78386685;     RRTGqBRqXy78386685 = RRTGqBRqXy52164882;     RRTGqBRqXy52164882 = RRTGqBRqXy849542;     RRTGqBRqXy849542 = RRTGqBRqXy12481627;     RRTGqBRqXy12481627 = RRTGqBRqXy8601429;     RRTGqBRqXy8601429 = RRTGqBRqXy66726092;     RRTGqBRqXy66726092 = RRTGqBRqXy19723823;     RRTGqBRqXy19723823 = RRTGqBRqXy76942240;     RRTGqBRqXy76942240 = RRTGqBRqXy14420515;     RRTGqBRqXy14420515 = RRTGqBRqXy53886216;     RRTGqBRqXy53886216 = RRTGqBRqXy76479479;     RRTGqBRqXy76479479 = RRTGqBRqXy76812402;     RRTGqBRqXy76812402 = RRTGqBRqXy18489664;     RRTGqBRqXy18489664 = RRTGqBRqXy36451314;     RRTGqBRqXy36451314 = RRTGqBRqXy86731858;     RRTGqBRqXy86731858 = RRTGqBRqXy93331176;     RRTGqBRqXy93331176 = RRTGqBRqXy39477364;     RRTGqBRqXy39477364 = RRTGqBRqXy46293286;     RRTGqBRqXy46293286 = RRTGqBRqXy31974520;     RRTGqBRqXy31974520 = RRTGqBRqXy5123197;     RRTGqBRqXy5123197 = RRTGqBRqXy48020834;     RRTGqBRqXy48020834 = RRTGqBRqXy43485816;     RRTGqBRqXy43485816 = RRTGqBRqXy72915154;     RRTGqBRqXy72915154 = RRTGqBRqXy31024427;     RRTGqBRqXy31024427 = RRTGqBRqXy12340775;     RRTGqBRqXy12340775 = RRTGqBRqXy58918799;     RRTGqBRqXy58918799 = RRTGqBRqXy78683048;     RRTGqBRqXy78683048 = RRTGqBRqXy74414495;     RRTGqBRqXy74414495 = RRTGqBRqXy55836331;     RRTGqBRqXy55836331 = RRTGqBRqXy39499999;     RRTGqBRqXy39499999 = RRTGqBRqXy18262040;     RRTGqBRqXy18262040 = RRTGqBRqXy24145740;     RRTGqBRqXy24145740 = RRTGqBRqXy77904817;     RRTGqBRqXy77904817 = RRTGqBRqXy58753652;     RRTGqBRqXy58753652 = RRTGqBRqXy63111335;     RRTGqBRqXy63111335 = RRTGqBRqXy92344627;     RRTGqBRqXy92344627 = RRTGqBRqXy93879798;     RRTGqBRqXy93879798 = RRTGqBRqXy12631444;     RRTGqBRqXy12631444 = RRTGqBRqXy68433301;     RRTGqBRqXy68433301 = RRTGqBRqXy9316707;     RRTGqBRqXy9316707 = RRTGqBRqXy86803062;     RRTGqBRqXy86803062 = RRTGqBRqXy16323379;     RRTGqBRqXy16323379 = RRTGqBRqXy96319293;     RRTGqBRqXy96319293 = RRTGqBRqXy45318767;     RRTGqBRqXy45318767 = RRTGqBRqXy30866846;     RRTGqBRqXy30866846 = RRTGqBRqXy15755103;     RRTGqBRqXy15755103 = RRTGqBRqXy8456786;     RRTGqBRqXy8456786 = RRTGqBRqXy85769382;     RRTGqBRqXy85769382 = RRTGqBRqXy81544279;     RRTGqBRqXy81544279 = RRTGqBRqXy44637663;     RRTGqBRqXy44637663 = RRTGqBRqXy7873499;     RRTGqBRqXy7873499 = RRTGqBRqXy49384903;     RRTGqBRqXy49384903 = RRTGqBRqXy17487365;     RRTGqBRqXy17487365 = RRTGqBRqXy94767763;     RRTGqBRqXy94767763 = RRTGqBRqXy29644067;     RRTGqBRqXy29644067 = RRTGqBRqXy34900870;     RRTGqBRqXy34900870 = RRTGqBRqXy79249728;     RRTGqBRqXy79249728 = RRTGqBRqXy69825115;     RRTGqBRqXy69825115 = RRTGqBRqXy140852;     RRTGqBRqXy140852 = RRTGqBRqXy49682630;     RRTGqBRqXy49682630 = RRTGqBRqXy88043043;     RRTGqBRqXy88043043 = RRTGqBRqXy45309327;     RRTGqBRqXy45309327 = RRTGqBRqXy21105910;     RRTGqBRqXy21105910 = RRTGqBRqXy74920516;     RRTGqBRqXy74920516 = RRTGqBRqXy35624176;     RRTGqBRqXy35624176 = RRTGqBRqXy52333740;     RRTGqBRqXy52333740 = RRTGqBRqXy98907585;     RRTGqBRqXy98907585 = RRTGqBRqXy59736011;     RRTGqBRqXy59736011 = RRTGqBRqXy73339979;     RRTGqBRqXy73339979 = RRTGqBRqXy94387230;     RRTGqBRqXy94387230 = RRTGqBRqXy99451378;     RRTGqBRqXy99451378 = RRTGqBRqXy26845921;     RRTGqBRqXy26845921 = RRTGqBRqXy77859984;     RRTGqBRqXy77859984 = RRTGqBRqXy22657813;     RRTGqBRqXy22657813 = RRTGqBRqXy18320135;     RRTGqBRqXy18320135 = RRTGqBRqXy31697456;     RRTGqBRqXy31697456 = RRTGqBRqXy47166522;     RRTGqBRqXy47166522 = RRTGqBRqXy27596388;     RRTGqBRqXy27596388 = RRTGqBRqXy157581;     RRTGqBRqXy157581 = RRTGqBRqXy96585672;     RRTGqBRqXy96585672 = RRTGqBRqXy50462013;     RRTGqBRqXy50462013 = RRTGqBRqXy92913665;     RRTGqBRqXy92913665 = RRTGqBRqXy25113071;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void KcmcikSXZu31374315() {     int NdWQMdhHIv47500188 = -712831579;    int NdWQMdhHIv61382230 = -825357707;    int NdWQMdhHIv4812077 = -891762342;    int NdWQMdhHIv21533573 = -248759024;    int NdWQMdhHIv91439872 = -417814885;    int NdWQMdhHIv25622925 = 53899209;    int NdWQMdhHIv91360721 = -501781868;    int NdWQMdhHIv82980184 = -60281523;    int NdWQMdhHIv89730636 = -740152140;    int NdWQMdhHIv18710035 = -478139823;    int NdWQMdhHIv59762557 = -859227888;    int NdWQMdhHIv24179086 = 55512093;    int NdWQMdhHIv4977020 = -284707029;    int NdWQMdhHIv89965614 = 28722332;    int NdWQMdhHIv32413466 = -680085559;    int NdWQMdhHIv93717797 = -46280989;    int NdWQMdhHIv49714312 = -70639323;    int NdWQMdhHIv83641317 = -587878524;    int NdWQMdhHIv19668317 = -949779434;    int NdWQMdhHIv30681166 = -716705929;    int NdWQMdhHIv1732650 = -854763867;    int NdWQMdhHIv76296845 = -546038781;    int NdWQMdhHIv11894752 = -679031468;    int NdWQMdhHIv93225878 = -672689499;    int NdWQMdhHIv55264069 = -934785026;    int NdWQMdhHIv65245235 = -400769151;    int NdWQMdhHIv61509935 = 83337430;    int NdWQMdhHIv59797666 = -811041729;    int NdWQMdhHIv83372040 = -379179295;    int NdWQMdhHIv85969582 = -352109124;    int NdWQMdhHIv62780469 = -314973273;    int NdWQMdhHIv93992505 = -512964385;    int NdWQMdhHIv67770528 = -437916707;    int NdWQMdhHIv52719864 = -277703685;    int NdWQMdhHIv15706729 = -486926213;    int NdWQMdhHIv87986155 = -106415718;    int NdWQMdhHIv25414232 = -256713779;    int NdWQMdhHIv58749297 = -133408043;    int NdWQMdhHIv96328965 = -898617011;    int NdWQMdhHIv63671530 = -962748181;    int NdWQMdhHIv1672526 = -66286984;    int NdWQMdhHIv19532818 = -261484044;    int NdWQMdhHIv17890967 = 6685050;    int NdWQMdhHIv58362918 = -167999995;    int NdWQMdhHIv46666315 = -651234868;    int NdWQMdhHIv82514777 = -532026651;    int NdWQMdhHIv68133728 = -463184602;    int NdWQMdhHIv84098673 = 88555398;    int NdWQMdhHIv12721945 = -548436839;    int NdWQMdhHIv21229485 = -858575071;    int NdWQMdhHIv86774033 = -58156512;    int NdWQMdhHIv32604910 = -92950062;    int NdWQMdhHIv41425739 = -894791525;    int NdWQMdhHIv52631173 = -156527196;    int NdWQMdhHIv29641666 = -255377242;    int NdWQMdhHIv71203342 = -66792799;    int NdWQMdhHIv49487478 = -46326240;    int NdWQMdhHIv11586198 = -119072844;    int NdWQMdhHIv66269504 = -313973998;    int NdWQMdhHIv26194638 = 82954265;    int NdWQMdhHIv64112990 = 70561779;    int NdWQMdhHIv31563056 = -690740139;    int NdWQMdhHIv99608143 = -681102229;    int NdWQMdhHIv3761055 = -288043016;    int NdWQMdhHIv55929565 = -63166551;    int NdWQMdhHIv65770051 = -246263503;    int NdWQMdhHIv56408557 = -506571201;    int NdWQMdhHIv52257156 = 92996656;    int NdWQMdhHIv74258886 = -484351455;    int NdWQMdhHIv44427311 = -473669841;    int NdWQMdhHIv68303565 = -789567210;    int NdWQMdhHIv90965014 = -937231281;    int NdWQMdhHIv87312352 = -689261513;    int NdWQMdhHIv55996786 = -987031253;    int NdWQMdhHIv29008640 = -550418945;    int NdWQMdhHIv82199831 = -493279823;    int NdWQMdhHIv58405878 = -452723832;    int NdWQMdhHIv53531834 = -411031473;    int NdWQMdhHIv46559564 = 78545369;    int NdWQMdhHIv72749291 = -302758376;    int NdWQMdhHIv97111506 = -937584549;    int NdWQMdhHIv77411262 = 94782032;    int NdWQMdhHIv47075721 = -162604891;    int NdWQMdhHIv62142555 = -520604224;    int NdWQMdhHIv99195548 = -193952613;    int NdWQMdhHIv30175559 = -122023211;    int NdWQMdhHIv52566767 = -618172861;    int NdWQMdhHIv15139356 = -181389512;    int NdWQMdhHIv23078199 = 77673556;    int NdWQMdhHIv44503386 = -320133414;    int NdWQMdhHIv38498677 = 39910521;    int NdWQMdhHIv13828034 = -37640936;    int NdWQMdhHIv92479793 = -819434046;    int NdWQMdhHIv70134327 = -881571276;    int NdWQMdhHIv99558540 = -933309960;    int NdWQMdhHIv70109469 = -375546846;    int NdWQMdhHIv19924674 = -580381816;    int NdWQMdhHIv14129913 = -705271934;    int NdWQMdhHIv2433353 = -4833444;    int NdWQMdhHIv80896263 = -712831579;     NdWQMdhHIv47500188 = NdWQMdhHIv61382230;     NdWQMdhHIv61382230 = NdWQMdhHIv4812077;     NdWQMdhHIv4812077 = NdWQMdhHIv21533573;     NdWQMdhHIv21533573 = NdWQMdhHIv91439872;     NdWQMdhHIv91439872 = NdWQMdhHIv25622925;     NdWQMdhHIv25622925 = NdWQMdhHIv91360721;     NdWQMdhHIv91360721 = NdWQMdhHIv82980184;     NdWQMdhHIv82980184 = NdWQMdhHIv89730636;     NdWQMdhHIv89730636 = NdWQMdhHIv18710035;     NdWQMdhHIv18710035 = NdWQMdhHIv59762557;     NdWQMdhHIv59762557 = NdWQMdhHIv24179086;     NdWQMdhHIv24179086 = NdWQMdhHIv4977020;     NdWQMdhHIv4977020 = NdWQMdhHIv89965614;     NdWQMdhHIv89965614 = NdWQMdhHIv32413466;     NdWQMdhHIv32413466 = NdWQMdhHIv93717797;     NdWQMdhHIv93717797 = NdWQMdhHIv49714312;     NdWQMdhHIv49714312 = NdWQMdhHIv83641317;     NdWQMdhHIv83641317 = NdWQMdhHIv19668317;     NdWQMdhHIv19668317 = NdWQMdhHIv30681166;     NdWQMdhHIv30681166 = NdWQMdhHIv1732650;     NdWQMdhHIv1732650 = NdWQMdhHIv76296845;     NdWQMdhHIv76296845 = NdWQMdhHIv11894752;     NdWQMdhHIv11894752 = NdWQMdhHIv93225878;     NdWQMdhHIv93225878 = NdWQMdhHIv55264069;     NdWQMdhHIv55264069 = NdWQMdhHIv65245235;     NdWQMdhHIv65245235 = NdWQMdhHIv61509935;     NdWQMdhHIv61509935 = NdWQMdhHIv59797666;     NdWQMdhHIv59797666 = NdWQMdhHIv83372040;     NdWQMdhHIv83372040 = NdWQMdhHIv85969582;     NdWQMdhHIv85969582 = NdWQMdhHIv62780469;     NdWQMdhHIv62780469 = NdWQMdhHIv93992505;     NdWQMdhHIv93992505 = NdWQMdhHIv67770528;     NdWQMdhHIv67770528 = NdWQMdhHIv52719864;     NdWQMdhHIv52719864 = NdWQMdhHIv15706729;     NdWQMdhHIv15706729 = NdWQMdhHIv87986155;     NdWQMdhHIv87986155 = NdWQMdhHIv25414232;     NdWQMdhHIv25414232 = NdWQMdhHIv58749297;     NdWQMdhHIv58749297 = NdWQMdhHIv96328965;     NdWQMdhHIv96328965 = NdWQMdhHIv63671530;     NdWQMdhHIv63671530 = NdWQMdhHIv1672526;     NdWQMdhHIv1672526 = NdWQMdhHIv19532818;     NdWQMdhHIv19532818 = NdWQMdhHIv17890967;     NdWQMdhHIv17890967 = NdWQMdhHIv58362918;     NdWQMdhHIv58362918 = NdWQMdhHIv46666315;     NdWQMdhHIv46666315 = NdWQMdhHIv82514777;     NdWQMdhHIv82514777 = NdWQMdhHIv68133728;     NdWQMdhHIv68133728 = NdWQMdhHIv84098673;     NdWQMdhHIv84098673 = NdWQMdhHIv12721945;     NdWQMdhHIv12721945 = NdWQMdhHIv21229485;     NdWQMdhHIv21229485 = NdWQMdhHIv86774033;     NdWQMdhHIv86774033 = NdWQMdhHIv32604910;     NdWQMdhHIv32604910 = NdWQMdhHIv41425739;     NdWQMdhHIv41425739 = NdWQMdhHIv52631173;     NdWQMdhHIv52631173 = NdWQMdhHIv29641666;     NdWQMdhHIv29641666 = NdWQMdhHIv71203342;     NdWQMdhHIv71203342 = NdWQMdhHIv49487478;     NdWQMdhHIv49487478 = NdWQMdhHIv11586198;     NdWQMdhHIv11586198 = NdWQMdhHIv66269504;     NdWQMdhHIv66269504 = NdWQMdhHIv26194638;     NdWQMdhHIv26194638 = NdWQMdhHIv64112990;     NdWQMdhHIv64112990 = NdWQMdhHIv31563056;     NdWQMdhHIv31563056 = NdWQMdhHIv99608143;     NdWQMdhHIv99608143 = NdWQMdhHIv3761055;     NdWQMdhHIv3761055 = NdWQMdhHIv55929565;     NdWQMdhHIv55929565 = NdWQMdhHIv65770051;     NdWQMdhHIv65770051 = NdWQMdhHIv56408557;     NdWQMdhHIv56408557 = NdWQMdhHIv52257156;     NdWQMdhHIv52257156 = NdWQMdhHIv74258886;     NdWQMdhHIv74258886 = NdWQMdhHIv44427311;     NdWQMdhHIv44427311 = NdWQMdhHIv68303565;     NdWQMdhHIv68303565 = NdWQMdhHIv90965014;     NdWQMdhHIv90965014 = NdWQMdhHIv87312352;     NdWQMdhHIv87312352 = NdWQMdhHIv55996786;     NdWQMdhHIv55996786 = NdWQMdhHIv29008640;     NdWQMdhHIv29008640 = NdWQMdhHIv82199831;     NdWQMdhHIv82199831 = NdWQMdhHIv58405878;     NdWQMdhHIv58405878 = NdWQMdhHIv53531834;     NdWQMdhHIv53531834 = NdWQMdhHIv46559564;     NdWQMdhHIv46559564 = NdWQMdhHIv72749291;     NdWQMdhHIv72749291 = NdWQMdhHIv97111506;     NdWQMdhHIv97111506 = NdWQMdhHIv77411262;     NdWQMdhHIv77411262 = NdWQMdhHIv47075721;     NdWQMdhHIv47075721 = NdWQMdhHIv62142555;     NdWQMdhHIv62142555 = NdWQMdhHIv99195548;     NdWQMdhHIv99195548 = NdWQMdhHIv30175559;     NdWQMdhHIv30175559 = NdWQMdhHIv52566767;     NdWQMdhHIv52566767 = NdWQMdhHIv15139356;     NdWQMdhHIv15139356 = NdWQMdhHIv23078199;     NdWQMdhHIv23078199 = NdWQMdhHIv44503386;     NdWQMdhHIv44503386 = NdWQMdhHIv38498677;     NdWQMdhHIv38498677 = NdWQMdhHIv13828034;     NdWQMdhHIv13828034 = NdWQMdhHIv92479793;     NdWQMdhHIv92479793 = NdWQMdhHIv70134327;     NdWQMdhHIv70134327 = NdWQMdhHIv99558540;     NdWQMdhHIv99558540 = NdWQMdhHIv70109469;     NdWQMdhHIv70109469 = NdWQMdhHIv19924674;     NdWQMdhHIv19924674 = NdWQMdhHIv14129913;     NdWQMdhHIv14129913 = NdWQMdhHIv2433353;     NdWQMdhHIv2433353 = NdWQMdhHIv80896263;     NdWQMdhHIv80896263 = NdWQMdhHIv47500188;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OTnoywiyXh68019289() {     int AHunUaFWdJ69887305 = -347235542;    int AHunUaFWdJ45729730 = -773900762;    int AHunUaFWdJ33581356 = -31416186;    int AHunUaFWdJ36540262 = -255166556;    int AHunUaFWdJ89614127 = 91128114;    int AHunUaFWdJ40506042 = -906709169;    int AHunUaFWdJ83516460 = -323769174;    int AHunUaFWdJ58614042 = -530878168;    int AHunUaFWdJ86893767 = -624348179;    int AHunUaFWdJ10473619 = -220881156;    int AHunUaFWdJ97304416 = -855852289;    int AHunUaFWdJ80082035 = -195233201;    int AHunUaFWdJ71985200 = -903424349;    int AHunUaFWdJ32580367 = -498986425;    int AHunUaFWdJ69148742 = -307563272;    int AHunUaFWdJ37973710 = -427948531;    int AHunUaFWdJ99537663 = -347919143;    int AHunUaFWdJ89617734 = 60378786;    int AHunUaFWdJ60949947 = 15118441;    int AHunUaFWdJ9197449 = -969340382;    int AHunUaFWdJ2615757 = -846251175;    int AHunUaFWdJ40112063 = 49194906;    int AHunUaFWdJ15188075 = -855112060;    int AHunUaFWdJ19725666 = -136682356;    int AHunUaFWdJ90804316 = -556344151;    int AHunUaFWdJ53548229 = -844663375;    int AHunUaFWdJ8599356 = -774249063;    int AHunUaFWdJ65709116 = -599496183;    int AHunUaFWdJ90264600 = -723436414;    int AHunUaFWdJ95126761 = -272404991;    int AHunUaFWdJ7071274 = -396289425;    int AHunUaFWdJ51533696 = -65035740;    int AHunUaFWdJ48809199 = -967260202;    int AHunUaFWdJ12108552 = -601830536;    int AHunUaFWdJ91936093 = -550682635;    int AHunUaFWdJ29679024 = -421811095;    int AHunUaFWdJ18853944 = 10338270;    int AHunUaFWdJ12375399 = -1033515;    int AHunUaFWdJ44637097 = -199663042;    int AHunUaFWdJ83857245 = 1027789;    int AHunUaFWdJ30429896 = -569464650;    int AHunUaFWdJ8041210 = -19392735;    int AHunUaFWdJ23441159 = -980845533;    int AHunUaFWdJ57807037 = -165363644;    int AHunUaFWdJ14649581 = -460378346;    int AHunUaFWdJ90615059 = -714255747;    int AHunUaFWdJ80431125 = 58405145;    int AHunUaFWdJ28697348 = -767979593;    int AHunUaFWdJ7181850 = -498153537;    int AHunUaFWdJ18313231 = 99404670;    int AHunUaFWdJ95643250 = -413232789;    int AHunUaFWdJ6456167 = -523769344;    int AHunUaFWdJ19740142 = -566702755;    int AHunUaFWdJ12917719 = -432208005;    int AHunUaFWdJ65403533 = -396081357;    int AHunUaFWdJ29775242 = -296430449;    int AHunUaFWdJ30541656 = -918788702;    int AHunUaFWdJ13855690 = -894733830;    int AHunUaFWdJ45735945 = -698822405;    int AHunUaFWdJ36065898 = -64208512;    int AHunUaFWdJ31906687 = -32460106;    int AHunUaFWdJ17807345 = -724272991;    int AHunUaFWdJ68349442 = -807441754;    int AHunUaFWdJ91767005 = -251943188;    int AHunUaFWdJ3402345 = -824591732;    int AHunUaFWdJ45770720 = -690816549;    int AHunUaFWdJ31272836 = -227972999;    int AHunUaFWdJ59876649 = -201593814;    int AHunUaFWdJ40644274 = -948303790;    int AHunUaFWdJ39469719 = -885752177;    int AHunUaFWdJ19119766 = -338286801;    int AHunUaFWdJ87162265 = -246885628;    int AHunUaFWdJ44980637 = -739958172;    int AHunUaFWdJ77092702 = -985909349;    int AHunUaFWdJ78767552 = -299875732;    int AHunUaFWdJ94574547 = -726858440;    int AHunUaFWdJ16670905 = 30040439;    int AHunUaFWdJ57381037 = -589748416;    int AHunUaFWdJ5076085 = -676304011;    int AHunUaFWdJ189257 = -842088405;    int AHunUaFWdJ73117104 = -803068520;    int AHunUaFWdJ79902007 = 93730530;    int AHunUaFWdJ58527266 = -1342646;    int AHunUaFWdJ71951370 = -722841084;    int AHunUaFWdJ99483511 = -859172203;    int AHunUaFWdJ615107 = -872520081;    int AHunUaFWdJ31793555 = -498332985;    int AHunUaFWdJ35891481 = -435052198;    int AHunUaFWdJ46705018 = -105749179;    int AHunUaFWdJ62160851 = -154252187;    int AHunUaFWdJ99137368 = -503022393;    int AHunUaFWdJ4998255 = -94927900;    int AHunUaFWdJ66639453 = -302211110;    int AHunUaFWdJ8571200 = -35454531;    int AHunUaFWdJ51950558 = -966512105;    int AHunUaFWdJ12622552 = -845191659;    int AHunUaFWdJ39691768 = -211950982;    int AHunUaFWdJ31674154 = -628902346;    int AHunUaFWdJ54404692 = -340771912;    int AHunUaFWdJ68878861 = -347235542;     AHunUaFWdJ69887305 = AHunUaFWdJ45729730;     AHunUaFWdJ45729730 = AHunUaFWdJ33581356;     AHunUaFWdJ33581356 = AHunUaFWdJ36540262;     AHunUaFWdJ36540262 = AHunUaFWdJ89614127;     AHunUaFWdJ89614127 = AHunUaFWdJ40506042;     AHunUaFWdJ40506042 = AHunUaFWdJ83516460;     AHunUaFWdJ83516460 = AHunUaFWdJ58614042;     AHunUaFWdJ58614042 = AHunUaFWdJ86893767;     AHunUaFWdJ86893767 = AHunUaFWdJ10473619;     AHunUaFWdJ10473619 = AHunUaFWdJ97304416;     AHunUaFWdJ97304416 = AHunUaFWdJ80082035;     AHunUaFWdJ80082035 = AHunUaFWdJ71985200;     AHunUaFWdJ71985200 = AHunUaFWdJ32580367;     AHunUaFWdJ32580367 = AHunUaFWdJ69148742;     AHunUaFWdJ69148742 = AHunUaFWdJ37973710;     AHunUaFWdJ37973710 = AHunUaFWdJ99537663;     AHunUaFWdJ99537663 = AHunUaFWdJ89617734;     AHunUaFWdJ89617734 = AHunUaFWdJ60949947;     AHunUaFWdJ60949947 = AHunUaFWdJ9197449;     AHunUaFWdJ9197449 = AHunUaFWdJ2615757;     AHunUaFWdJ2615757 = AHunUaFWdJ40112063;     AHunUaFWdJ40112063 = AHunUaFWdJ15188075;     AHunUaFWdJ15188075 = AHunUaFWdJ19725666;     AHunUaFWdJ19725666 = AHunUaFWdJ90804316;     AHunUaFWdJ90804316 = AHunUaFWdJ53548229;     AHunUaFWdJ53548229 = AHunUaFWdJ8599356;     AHunUaFWdJ8599356 = AHunUaFWdJ65709116;     AHunUaFWdJ65709116 = AHunUaFWdJ90264600;     AHunUaFWdJ90264600 = AHunUaFWdJ95126761;     AHunUaFWdJ95126761 = AHunUaFWdJ7071274;     AHunUaFWdJ7071274 = AHunUaFWdJ51533696;     AHunUaFWdJ51533696 = AHunUaFWdJ48809199;     AHunUaFWdJ48809199 = AHunUaFWdJ12108552;     AHunUaFWdJ12108552 = AHunUaFWdJ91936093;     AHunUaFWdJ91936093 = AHunUaFWdJ29679024;     AHunUaFWdJ29679024 = AHunUaFWdJ18853944;     AHunUaFWdJ18853944 = AHunUaFWdJ12375399;     AHunUaFWdJ12375399 = AHunUaFWdJ44637097;     AHunUaFWdJ44637097 = AHunUaFWdJ83857245;     AHunUaFWdJ83857245 = AHunUaFWdJ30429896;     AHunUaFWdJ30429896 = AHunUaFWdJ8041210;     AHunUaFWdJ8041210 = AHunUaFWdJ23441159;     AHunUaFWdJ23441159 = AHunUaFWdJ57807037;     AHunUaFWdJ57807037 = AHunUaFWdJ14649581;     AHunUaFWdJ14649581 = AHunUaFWdJ90615059;     AHunUaFWdJ90615059 = AHunUaFWdJ80431125;     AHunUaFWdJ80431125 = AHunUaFWdJ28697348;     AHunUaFWdJ28697348 = AHunUaFWdJ7181850;     AHunUaFWdJ7181850 = AHunUaFWdJ18313231;     AHunUaFWdJ18313231 = AHunUaFWdJ95643250;     AHunUaFWdJ95643250 = AHunUaFWdJ6456167;     AHunUaFWdJ6456167 = AHunUaFWdJ19740142;     AHunUaFWdJ19740142 = AHunUaFWdJ12917719;     AHunUaFWdJ12917719 = AHunUaFWdJ65403533;     AHunUaFWdJ65403533 = AHunUaFWdJ29775242;     AHunUaFWdJ29775242 = AHunUaFWdJ30541656;     AHunUaFWdJ30541656 = AHunUaFWdJ13855690;     AHunUaFWdJ13855690 = AHunUaFWdJ45735945;     AHunUaFWdJ45735945 = AHunUaFWdJ36065898;     AHunUaFWdJ36065898 = AHunUaFWdJ31906687;     AHunUaFWdJ31906687 = AHunUaFWdJ17807345;     AHunUaFWdJ17807345 = AHunUaFWdJ68349442;     AHunUaFWdJ68349442 = AHunUaFWdJ91767005;     AHunUaFWdJ91767005 = AHunUaFWdJ3402345;     AHunUaFWdJ3402345 = AHunUaFWdJ45770720;     AHunUaFWdJ45770720 = AHunUaFWdJ31272836;     AHunUaFWdJ31272836 = AHunUaFWdJ59876649;     AHunUaFWdJ59876649 = AHunUaFWdJ40644274;     AHunUaFWdJ40644274 = AHunUaFWdJ39469719;     AHunUaFWdJ39469719 = AHunUaFWdJ19119766;     AHunUaFWdJ19119766 = AHunUaFWdJ87162265;     AHunUaFWdJ87162265 = AHunUaFWdJ44980637;     AHunUaFWdJ44980637 = AHunUaFWdJ77092702;     AHunUaFWdJ77092702 = AHunUaFWdJ78767552;     AHunUaFWdJ78767552 = AHunUaFWdJ94574547;     AHunUaFWdJ94574547 = AHunUaFWdJ16670905;     AHunUaFWdJ16670905 = AHunUaFWdJ57381037;     AHunUaFWdJ57381037 = AHunUaFWdJ5076085;     AHunUaFWdJ5076085 = AHunUaFWdJ189257;     AHunUaFWdJ189257 = AHunUaFWdJ73117104;     AHunUaFWdJ73117104 = AHunUaFWdJ79902007;     AHunUaFWdJ79902007 = AHunUaFWdJ58527266;     AHunUaFWdJ58527266 = AHunUaFWdJ71951370;     AHunUaFWdJ71951370 = AHunUaFWdJ99483511;     AHunUaFWdJ99483511 = AHunUaFWdJ615107;     AHunUaFWdJ615107 = AHunUaFWdJ31793555;     AHunUaFWdJ31793555 = AHunUaFWdJ35891481;     AHunUaFWdJ35891481 = AHunUaFWdJ46705018;     AHunUaFWdJ46705018 = AHunUaFWdJ62160851;     AHunUaFWdJ62160851 = AHunUaFWdJ99137368;     AHunUaFWdJ99137368 = AHunUaFWdJ4998255;     AHunUaFWdJ4998255 = AHunUaFWdJ66639453;     AHunUaFWdJ66639453 = AHunUaFWdJ8571200;     AHunUaFWdJ8571200 = AHunUaFWdJ51950558;     AHunUaFWdJ51950558 = AHunUaFWdJ12622552;     AHunUaFWdJ12622552 = AHunUaFWdJ39691768;     AHunUaFWdJ39691768 = AHunUaFWdJ31674154;     AHunUaFWdJ31674154 = AHunUaFWdJ54404692;     AHunUaFWdJ54404692 = AHunUaFWdJ68878861;     AHunUaFWdJ68878861 = AHunUaFWdJ69887305;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fmypKYgJRr84815592() {     int OppfcMJblB37337577 = -561017606;    int OppfcMJblB71639763 = -359906902;    int OppfcMJblB60096984 = -339467261;    int OppfcMJblB38296237 = -904659998;    int OppfcMJblB44316749 = -470590609;    int OppfcMJblB45782361 = -163634100;    int OppfcMJblB73003918 = 35470784;    int OppfcMJblB10687183 = -819698681;    int OppfcMJblB45962742 = -512490537;    int OppfcMJblB4365039 = -298559752;    int OppfcMJblB72922185 = -556686769;    int OppfcMJblB53359738 = -185772562;    int OppfcMJblB58166447 = -28259599;    int OppfcMJblB44404546 = 74224376;    int OppfcMJblB54921206 = -177601456;    int OppfcMJblB94319493 = -659806009;    int OppfcMJblB71342106 = -755871584;    int OppfcMJblB56272007 = -612788551;    int OppfcMJblB82688773 = -353188902;    int OppfcMJblB21375856 = -995913991;    int OppfcMJblB97460064 = -139951030;    int OppfcMJblB52968073 = -72119725;    int OppfcMJblB35100607 = -134422612;    int OppfcMJblB186002 = -374687401;    int OppfcMJblB99787156 = 50341253;    int OppfcMJblB1246707 = -676494466;    int OppfcMJblB66672443 = -411600052;    int OppfcMJblB33205754 = -967895666;    int OppfcMJblB27761661 = -898793057;    int OppfcMJblB78178481 = -679902053;    int OppfcMJblB53093780 = -640938849;    int OppfcMJblB8758344 = -967519305;    int OppfcMJblB34332390 = -546716277;    int OppfcMJblB83416560 = -921682965;    int OppfcMJblB85963725 = -989606888;    int OppfcMJblB47987397 = -888721176;    int OppfcMJblB62410440 = -30517634;    int OppfcMJblB78180141 = -901688609;    int OppfcMJblB48211737 = 95559903;    int OppfcMJblB76299030 = -637550126;    int OppfcMJblB66037103 = -375572954;    int OppfcMJblB73871832 = -234548033;    int OppfcMJblB8899772 = -6088915;    int OppfcMJblB72582127 = -644768887;    int OppfcMJblB28615057 = -325776802;    int OppfcMJblB58934838 = -588296795;    int OppfcMJblB32940168 = -844246357;    int OppfcMJblB18489252 = -537842711;    int OppfcMJblB15601082 = -665839909;    int OppfcMJblB80037179 = -271113758;    int OppfcMJblB69195228 = -955500088;    int OppfcMJblB2193978 = -386802043;    int OppfcMJblB22780773 = -94540483;    int OppfcMJblB99657794 = -769666454;    int OppfcMJblB87355528 = -51085608;    int OppfcMJblB84369504 = -388897881;    int OppfcMJblB36539156 = -125484290;    int OppfcMJblB59910982 = -964779860;    int OppfcMJblB38509080 = -855001251;    int OppfcMJblB43070043 = -794096143;    int OppfcMJblB79109918 = -752034049;    int OppfcMJblB39798165 = 3366449;    int OppfcMJblB82925522 = -920905625;    int OppfcMJblB67784261 = -832588484;    int OppfcMJblB51271258 = -657620904;    int OppfcMJblB64163841 = -589167464;    int OppfcMJblB19027348 = -639056285;    int OppfcMJblB74749886 = -106576635;    int OppfcMJblB58440821 = 63831264;    int OppfcMJblB6933809 = -288880280;    int OppfcMJblB31909053 = -529288375;    int OppfcMJblB93161965 = -854182975;    int OppfcMJblB8060271 = -608348455;    int OppfcMJblB6389744 = -715638776;    int OppfcMJblB55338752 = -520341037;    int OppfcMJblB23588233 = -905402997;    int OppfcMJblB44068301 = 33969189;    int OppfcMJblB62518480 = -489653726;    int OppfcMJblB71570945 = 51089400;    int OppfcMJblB40852319 = -361361953;    int OppfcMJblB68306538 = -832248109;    int OppfcMJblB48183192 = -873757342;    int OppfcMJblB17604672 = -202055757;    int OppfcMJblB47724481 = -527679300;    int OppfcMJblB8983253 = -724401966;    int OppfcMJblB50899803 = -154136806;    int OppfcMJblB85977571 = -772978822;    int OppfcMJblB34674596 = -777049823;    int OppfcMJblB96061032 = -770597357;    int OppfcMJblB1594221 = -500709007;    int OppfcMJblB11448241 = -663236887;    int OppfcMJblB2499459 = -65737774;    int OppfcMJblB39671061 = 53312642;    int OppfcMJblB5141695 = -110343954;    int OppfcMJblB97189112 = -885516078;    int OppfcMJblB26238939 = -278939403;    int OppfcMJblB90946309 = -313642409;    int OppfcMJblB41115511 = -173500431;    int OppfcMJblB21310869 = -987147984;    int OppfcMJblB64451215 = -561017606;     OppfcMJblB37337577 = OppfcMJblB71639763;     OppfcMJblB71639763 = OppfcMJblB60096984;     OppfcMJblB60096984 = OppfcMJblB38296237;     OppfcMJblB38296237 = OppfcMJblB44316749;     OppfcMJblB44316749 = OppfcMJblB45782361;     OppfcMJblB45782361 = OppfcMJblB73003918;     OppfcMJblB73003918 = OppfcMJblB10687183;     OppfcMJblB10687183 = OppfcMJblB45962742;     OppfcMJblB45962742 = OppfcMJblB4365039;     OppfcMJblB4365039 = OppfcMJblB72922185;     OppfcMJblB72922185 = OppfcMJblB53359738;     OppfcMJblB53359738 = OppfcMJblB58166447;     OppfcMJblB58166447 = OppfcMJblB44404546;     OppfcMJblB44404546 = OppfcMJblB54921206;     OppfcMJblB54921206 = OppfcMJblB94319493;     OppfcMJblB94319493 = OppfcMJblB71342106;     OppfcMJblB71342106 = OppfcMJblB56272007;     OppfcMJblB56272007 = OppfcMJblB82688773;     OppfcMJblB82688773 = OppfcMJblB21375856;     OppfcMJblB21375856 = OppfcMJblB97460064;     OppfcMJblB97460064 = OppfcMJblB52968073;     OppfcMJblB52968073 = OppfcMJblB35100607;     OppfcMJblB35100607 = OppfcMJblB186002;     OppfcMJblB186002 = OppfcMJblB99787156;     OppfcMJblB99787156 = OppfcMJblB1246707;     OppfcMJblB1246707 = OppfcMJblB66672443;     OppfcMJblB66672443 = OppfcMJblB33205754;     OppfcMJblB33205754 = OppfcMJblB27761661;     OppfcMJblB27761661 = OppfcMJblB78178481;     OppfcMJblB78178481 = OppfcMJblB53093780;     OppfcMJblB53093780 = OppfcMJblB8758344;     OppfcMJblB8758344 = OppfcMJblB34332390;     OppfcMJblB34332390 = OppfcMJblB83416560;     OppfcMJblB83416560 = OppfcMJblB85963725;     OppfcMJblB85963725 = OppfcMJblB47987397;     OppfcMJblB47987397 = OppfcMJblB62410440;     OppfcMJblB62410440 = OppfcMJblB78180141;     OppfcMJblB78180141 = OppfcMJblB48211737;     OppfcMJblB48211737 = OppfcMJblB76299030;     OppfcMJblB76299030 = OppfcMJblB66037103;     OppfcMJblB66037103 = OppfcMJblB73871832;     OppfcMJblB73871832 = OppfcMJblB8899772;     OppfcMJblB8899772 = OppfcMJblB72582127;     OppfcMJblB72582127 = OppfcMJblB28615057;     OppfcMJblB28615057 = OppfcMJblB58934838;     OppfcMJblB58934838 = OppfcMJblB32940168;     OppfcMJblB32940168 = OppfcMJblB18489252;     OppfcMJblB18489252 = OppfcMJblB15601082;     OppfcMJblB15601082 = OppfcMJblB80037179;     OppfcMJblB80037179 = OppfcMJblB69195228;     OppfcMJblB69195228 = OppfcMJblB2193978;     OppfcMJblB2193978 = OppfcMJblB22780773;     OppfcMJblB22780773 = OppfcMJblB99657794;     OppfcMJblB99657794 = OppfcMJblB87355528;     OppfcMJblB87355528 = OppfcMJblB84369504;     OppfcMJblB84369504 = OppfcMJblB36539156;     OppfcMJblB36539156 = OppfcMJblB59910982;     OppfcMJblB59910982 = OppfcMJblB38509080;     OppfcMJblB38509080 = OppfcMJblB43070043;     OppfcMJblB43070043 = OppfcMJblB79109918;     OppfcMJblB79109918 = OppfcMJblB39798165;     OppfcMJblB39798165 = OppfcMJblB82925522;     OppfcMJblB82925522 = OppfcMJblB67784261;     OppfcMJblB67784261 = OppfcMJblB51271258;     OppfcMJblB51271258 = OppfcMJblB64163841;     OppfcMJblB64163841 = OppfcMJblB19027348;     OppfcMJblB19027348 = OppfcMJblB74749886;     OppfcMJblB74749886 = OppfcMJblB58440821;     OppfcMJblB58440821 = OppfcMJblB6933809;     OppfcMJblB6933809 = OppfcMJblB31909053;     OppfcMJblB31909053 = OppfcMJblB93161965;     OppfcMJblB93161965 = OppfcMJblB8060271;     OppfcMJblB8060271 = OppfcMJblB6389744;     OppfcMJblB6389744 = OppfcMJblB55338752;     OppfcMJblB55338752 = OppfcMJblB23588233;     OppfcMJblB23588233 = OppfcMJblB44068301;     OppfcMJblB44068301 = OppfcMJblB62518480;     OppfcMJblB62518480 = OppfcMJblB71570945;     OppfcMJblB71570945 = OppfcMJblB40852319;     OppfcMJblB40852319 = OppfcMJblB68306538;     OppfcMJblB68306538 = OppfcMJblB48183192;     OppfcMJblB48183192 = OppfcMJblB17604672;     OppfcMJblB17604672 = OppfcMJblB47724481;     OppfcMJblB47724481 = OppfcMJblB8983253;     OppfcMJblB8983253 = OppfcMJblB50899803;     OppfcMJblB50899803 = OppfcMJblB85977571;     OppfcMJblB85977571 = OppfcMJblB34674596;     OppfcMJblB34674596 = OppfcMJblB96061032;     OppfcMJblB96061032 = OppfcMJblB1594221;     OppfcMJblB1594221 = OppfcMJblB11448241;     OppfcMJblB11448241 = OppfcMJblB2499459;     OppfcMJblB2499459 = OppfcMJblB39671061;     OppfcMJblB39671061 = OppfcMJblB5141695;     OppfcMJblB5141695 = OppfcMJblB97189112;     OppfcMJblB97189112 = OppfcMJblB26238939;     OppfcMJblB26238939 = OppfcMJblB90946309;     OppfcMJblB90946309 = OppfcMJblB41115511;     OppfcMJblB41115511 = OppfcMJblB21310869;     OppfcMJblB21310869 = OppfcMJblB64451215;     OppfcMJblB64451215 = OppfcMJblB37337577;}
// Junk Finished
