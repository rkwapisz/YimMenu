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
		static void draw_skeleton(CPed* cped, ImDrawList* const draw_list, float multplr, ImColor esp_color, bool only_draw_head);
		static bool bone_to_screen(CPed* cped, ePedBoneType bone_type, rage::fvector2& bone_vec);
	};
}