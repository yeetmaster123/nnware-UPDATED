#pragma once

#define MAX_LAYER_RECORDS 15
#define MAX_PLAYERS 65

#define MAXSTUDIOBONES 256

#define	RFL_ONGROUND	(1 << 0)	
#define	RFL_MOVING		(1 << 1)

#include <deque>
#include <array>

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
	class ConVar;
}

struct LayerRecord {
	int m_nSequence;
	float m_flCycle;
	float m_flWeight;
	int m_nOrder;

	LayerRecord() {
		m_nSequence = 0;
		m_flCycle = 0.f;
		m_flWeight = 0.f;
		m_nOrder = 0;
	}
};

struct legit_backtrackdata
{
	float simtime;
	Vector hitboxPos;
	Vector origin;
	Vector angs;
	int32_t flags;
};

struct LagRecord {
	LagRecord() {
		m_fFlags = 0;
		m_vecOrigin.Init(0, 0, 0);
		m_vecEyeAngles.Init(0, 0, 0);
		m_vecAbsAngles.Init(0, 0, 0);
		m_flSimulationTime = 0;
		m_flLowerBodyYawTarget = 0.f;
	}

	int						m_fFlags;
	Vector					m_vecOrigin;
	Vector					m_vecEyeAngles;
	Vector					m_vecAbsAngles;
	float					m_flLowerBodyYawTarget;

	float					m_flSimulationTime;
	float*					m_flPoseParameter;

	std::array<LayerRecord, MAX_LAYER_RECORDS>	m_layerRecords;

	VMatrix				m_bone_matrix[128];
};

struct resolver_info_t {
	int				flags_i;
	int				tick_count;
	LagRecord		lag_record;
	bool			resolved;
	int				sequence_activity;
	bool			valid;
};

struct resolver_player_t {
	int				index;
	SDK::CBaseEntity*	player;
	std::array<LayerRecord, MAX_LAYER_RECORDS> prevlayers;
	std::deque<resolver_info_t> storedTicks;

	bool			breakinglby = false;
	bool			breakinglbyu120 = false;
};

class CBacktrack
{
public:
	SDK::NetChannel* netchan;
	void UpdateIncomingSequences();
	void AddLatency(SDK::NetChannel* net_channel, float latency);
	void log_players();
	void anim_fix(ClientFrameStage_t stage);
	void invalidate_bonecache(SDK::CBaseEntity* ent);
	void run_legit(SDK::CUserCmd * cmd);
	resolver_player_t * get_records(int index);
	Vector angle_vector(Vector meme);
	float point_to_line(Vector Point, Vector LineOrigin, Vector Dir);
	void set(SDK::CBaseEntity * player, LagRecord record);
	void backtrack_player(SDK::CBaseEntity * player, LagRecord& record);
private:
	bool valid(SDK::CBaseEntity * player);
	resolver_player_t players[MAX_PLAYERS];
};

extern legit_backtrackdata headPositions[64][12];
extern CBacktrack* backtracking;