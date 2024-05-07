#pragma once
#include "services/players/player_service.hpp"

namespace big
{
	class esp
	{
	public:
		static void draw();
		static void draw_player(const player_ptr& plyr, ImDrawList* const draw_list);
		static void draw_npc(const Entity ped, ImDrawList* const draw_list);
		//static bool bone_to_screen(const player_ptr& plyr, int boneID, ImVec2& boneVec);
		static bool bone_to_screen(const player_ptr& plyr, ePedBoneType bone_type, rage::fvector2& bone_vec); // Overload for Players
		static bool bone_to_screen(CPed* cped, ePedBoneType bone_type, rage::fvector2& bone_vec); // Overload for NPCs
	};
}