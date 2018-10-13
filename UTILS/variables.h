#pragma once

namespace GLOBAL
{
	extern HWND csgo_hwnd;

	extern bool should_send_packet;
	extern bool is_fakewalking;
	extern int choke_amount;

	extern Vector real_angles;
	extern Vector fake_angles;
	extern Vector strafe_angle;

	extern int randomnumber;
	extern float flHurtTime;
	extern bool DisableAA;
	extern bool Aimbotting;

	using msg_t = void(__cdecl*)(const char*, ...);
	extern msg_t		Msg;

	extern Vector FakePosition;
	extern int ground_tickz;
	extern bool CircleStraferActive;
	extern SDK::CUserCmd originalCMD;
}
namespace FONTS
{
	extern unsigned int menu_tab_font;
	extern unsigned int menu_checkbox_font;
	extern unsigned int menu_slider_font;
	extern unsigned int menu_groupbox_font;
	extern unsigned int menu_combobox_font;
	extern unsigned int menu_window_font;
	extern unsigned int TABS_font;
	extern unsigned int numpad_menu_font;
	extern unsigned int visuals_esp_font;
	extern unsigned int visuals_xhair_font;
	extern unsigned int visuals_side_font;
	extern unsigned int visuals_name_font;
	extern unsigned int visuals_lby_font;
	extern unsigned int visuals_grenade_pred_font;
	extern unsigned int visuals_icon_font;
	bool ShouldReloadFonts();
	void InitFonts();
}
class Weapon_tTT
{
public:
	int SkinsWeapon;
	int SkinsKnife;
	int VremennyiWeapon;
	int VremennyiKnife;
	int Stikers1;
	int Stikers2;
	int Stikers3;
	int Stikers4;
	float ChangerWear = 0;
	int ChangerStatTrak = 0;
	char ChangerName[32] = "";
	bool ChangerEnabled;
};

namespace SETTINGS
{
	class CSettings
	{
	public:
		bool Save(std::string file_name);
		bool Load(std::string file_name);

		void CreateConfig();

		std::vector<std::string> GetConfigs();
		Weapon_tTT weapons[520];
		bool skinenabled;
		int Knife;
		int gloves;
		int skingloves;
		float glovewear;
		bool glovesenabled;
		bool rankchanger;
		int KnifeSkin;
		int rank_id = 9;
		int wins = 100;
		int level = 40;
		int friendly = 999;
		int teaching = 999;
		int leader = 999;
		int AK47Skin;
		int GalilSkin;
		int M4A1SSkin;
		int M4A4Skin;
		int AUGSkin;
		int FAMASSkin;
		int AWPSkin;
		int SSG08Skin;
		int Menu_theme;
		bool fakelatency_enabled;
		bool fixshit;
		float fakelatency_amount;
		bool knifes;

		bool lagcomepsator;
		float ragepoints;
		int positionadjustment;
		bool rage_smart_aim;
		int rage_hitscan;
		float positionadjustment_max_history;
		int rage_multipoint;

		bool killfeed;
		int SCAR20Skin;
		int P90Skin;
		int Mp7Skin;
		int NovaSkin;
		int UMP45Skin;
		int GlockSkin;
		int SawedSkin;
		int USPSkin;
		int MagSkin;
		int XmSkin;
		int DeagleSkin;
		int DualSkin;
		int FiveSkin;
		int RevolverSkin;
		int Mac10Skin;
		int tec9Skin;
		int Cz75Skin;
		int NegevSkin;
		int M249Skin;
		int Mp9Skin;
		int P2000Skin;
		int BizonSkin;
		int Sg553Skin;
		int P250Skin;
		int G3sg1Skin;
		bool bhop_bool;
		bool strafe_bool;
		bool esp_bool;
		int chams_type;
		int xhair_type;
		bool tp_bool;
		bool aim_bool;
		bool henade;
		bool molly;
		bool smoke;
		int aim_type;
		bool aa_bool;
		int aa_pitch;
		int aa_type;
		int acc_type;
		bool wolrd_enabled;
		bool up_bool;
		bool misc_bool;
		int config_sel;
		bool beam_bool;
		bool impacts;
		bool stop_bool;
		bool forcehair;
		CColor awcolor;
		bool night_bool;
		bool trashtalk = false;
		bool trashtalk2 = false;
		bool trashtalk3 = false;
		bool box_bool;
		bool name_bool;

		bool onlyhs;
		bool bomb;
		bool bomb_beep;
		bool cs_win_panel_round;
		bool achievement_earned;

		CColor spread_Col;
		bool weap_bool;
		bool health_bool;
		bool info_bool;
		bool back_bool;
		bool lag_bool;
		bool autozeus_bool;
		int box_type;
		bool reverse_bool;
		bool multi_bool;
		bool fakefix_bool;
		bool prediction;
		bool vecvelocityprediction;
		bool angle_bool;
		bool tp_angle_bool;
		bool glow_bool;
		bool dist_bool;
		bool fov_bool;
		bool smoke_bool;
		bool wirehand_bool;
		bool scope_bool;
		bool predict_bool;
		bool fake_bool;
		int media_type;
		bool novis_bool;
		bool RemoveFlashBang;
		bool fakelatency;
		float fakelatency_amt;
		bool localglow_bool;
		bool duck_bool;
		bool money_bool;
		int delay_shot;
		int lag_type;
		bool auto_revolver;
		bool auto_revolver_enabled;
		bool cham_bool;
		bool ammo_bool;
		bool spread_bool;
		bool Watermark;
		float stand_lag;
		float move_lag;
		float jump_lag;

		// new night mode

		bool night_mode;

		// sky color changer

		bool sky_enabled;
		CColor skycolor;
		CColor night_col;

		bool debug_bool;

		CColor vmodel_col;
		CColor imodel_col;
		CColor box_col;
		CColor name_col;
		CColor weapon_col;
		CColor distance_col;
		CColor localchams_col;
		CColor grenadepredline_col;
		CColor grenadepredbox_col;

		CColor bulletlocal_col;
		CColor bulletenemy_col;
		CColor bulletteam_col;

		CColor menu_col = CColor(30, 30, 30);
		CColor checkbox_col = CColor(200, 0, 100);
		CColor slider_col = CColor(200, 0, 100);
		CColor tab_col = CColor(50, 50, 50);
		CColor glow_col;
		CColor glowlocal_col;
		CColor fov_col;
		bool wiresleeve_bool;
		float chance_val;
		float damage_val;
		float delta_val;
		float point_val;
		float body_val;
		float baimafterhp;
		float baimaftershot;
		bool misc_clantag;
		bool GameSenseClan;

		bool baiminair;

		bool Skins;
		bool Beta_AA;
		bool BacktrackChams;
		int stop_mode;

		bool localesp;
		int localchams;
		float fov_val = 90;
		float viewfov_val = 68;

		bool flip_bool;
		int aa_side;

		bool legit_bool;
		int legit_key;
		bool rcs_bool;
		float legitfov_val;
		int legitbone_int;
		float rcsamount_min;
		float rcsamount_max;
		float legitaim_val;
		bool legittrigger_bool;
		int legittrigger_key;

		int thirdperson_int;
		int flip_int;

		bool glowenable;
		int glowstyle;
		bool glowlocal;
		int glowstylelocal;
		int hitmarker_val;

		int aa_mode;

		int aa_real_type;
		int aa_real1_type;
		int aa_real2_type;

		int aa_fake_type;
		int aa_fake1_type;
		int aa_fake2_type;

		int aa_pitch_type;
		int aa_pitch1_type;
		int aa_pitch2_type;

		float aa_realadditive_val;
		float aa_fakeadditive_val;

		float aa_realadditive1_val;
		float aa_fakeadditive1_val;
		float delta1_val;

		float aa_realadditive2_val;
		float aa_fakeadditive2_val;
		float delta2_val;

		float spinangle;
		float spinspeed;

		float spinangle1;
		float spinspeed1;

		float spinangle2;
		float spinspeed2;

		float spinanglefake;
		float spinspeedfake;

		float spinanglefake1;
		float spinspeedfake1;

		float spinanglefake2;
		float spinspeedfake2;

		bool lbyflickup;
		bool lbyflickup1;
		bool lbyflickup2;

		bool aa_fakeangchams_bool;

		int chamstype;
		float fov_time;
		bool rifk_arrow;

		int glowteamselection;
		bool glowteam;

		int chamsteamselection;
		int chamsteam;

		int espteamselection;
		int espteamcolourselection;
		bool boxteam;
		bool nameteam;
		bool weaponteam;
		bool flagsteam;
		bool healthteam;
		bool moneyteam;
		bool ammoteam;
		bool arrowteam;
		CColor boxenemy_col;
		CColor nameenemy_col;
		CColor weaponenemy_col;
		CColor boxteam_col;
		CColor nameteam_col;
		CColor weaponteam_col;
		CColor arrowteam_col;

		CColor teamvis_color;
		CColor teaminvis_color;
		CColor teamglow_color;
		CColor fake_darw_col;

		bool matpostprocessenable;
		bool removescoping;
		bool draw_fake;
		bool fixscopesens;
		bool forcecrosshair;

		int quickstopkey;
		bool stop_flip;
		bool chamsmetallic;
		int flashlightkey;
		int overridekey;
		int autostopmethod;
		int overridemethod;
		bool overridething;
		bool overrideenable;
		bool lbyenable;
		bool zeusbot_bool;
		int circlestrafekey;
		bool fakewalk;
		bool ragdoll_bool;
		bool astro;
		int buybotoptions;
		float circlstraferetract;
		float fakewalkspeed;
		float daytimevalue = 60;

		float circlemin;
		float circlemax;
		float circlevel;
		float circlenormalizemultiplier;

		bool skinchangerenable;

		int knifeToUse;
		int bayonetID, karambitID, flipID, gutID, m9ID, huntsmanID;
		int gloveToUse;
		int bloodhoundID, driverID, handwrapsID, motoID, specialistID, sportID, hydraID;

		int uspID, p2000ID, glockID, dualberettaID, p250ID, fivesevenID, tech9ID, r8ID, deagleID;
		int novaID, xm1014ID, mag7ID, sawedoffID, m249ID, negevID;
		int mp9ID, mac10ID, mp7ID, ump45ID, p90ID, ppbizonID;
		int famasID, galilID, ak47ID, m4a4ID, m4a1sID, ssg08ID, augID, sg553ID, awpID, scar20ID, g3sg1ID;
		bool hitmarker;

		bool lagcomhit;
		float lagcomptime;
		
		bool sky_bool;
		CColor sky_color;
		int draw_wep;

		bool dmg_bool;
		CColor dmg_color;

		bool asus_bool;

		bool novisualrecoil_bool;

		bool resolve_bool;

		int ResolverEnable;

		bool nospread;

		bool WeaponIconsOn;

		bool autobuy_bool;
		bool auto_bool;
		bool revolver_bool;
		bool scout_bool;
		bool armour_bool;
		bool zeus_bool;
		bool elite_bool;
		float vfov_val;
		bool no_flash_enabled;
		bool autoknife_bool;
		bool noflash;
		bool radar_enabled;

	private:
	}; extern CSettings settings;
}

extern bool using_fake_angles[65];
extern bool full_choke;
extern bool is_shooting;

extern bool in_tp;
extern bool fake_walk;

extern int resolve_type[65];

extern int target;
extern int shots_fired[65];
extern int shots_hit[65];
extern int shots_missed[65];
extern int backtrack_missed[65];

extern bool didShot;
extern bool didMiss;

extern float tick_to_back[65];
extern float lby_to_back[65];
extern bool backtrack_tick[65];

extern float lby_delta;
extern float update_time[65];
extern float walking_time[65];

extern float local_update;

extern int hitmarker_time;
extern int random_number;

extern bool menu_hide;

extern int oldest_tick[65];
extern float compensate[65][12];
extern Vector backtrack_hitbox[65][20][12];
extern float backtrack_simtime[65][12];
