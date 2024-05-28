#include "view_esp.hpp"
#include "gta/enums.hpp"
#include "gta_util.hpp"
#include "pointers.hpp"
#include "services/gta_data/gta_data_service.hpp"
#include "services/players/player_service.hpp"
#include "util/entity.hpp"
#include "util/ped.hpp"
#include "util/math.hpp"
#include "util/misc.hpp"
#include "util/world_to_screen.hpp"

namespace big
{
	static const ImColor death_bg         = ImColor(0.117f, 0.113f, 0.172f, .75f);
	static const ImColor armor_blue_bg    = ImColor(0.36f, 0.71f, 0.89f, .75f);
	static const ImColor armor_blue       = ImColor(0.36f, 0.71f, 0.89f, 1.f);
	static const ImColor health_green_bg  = ImColor(0.29f, 0.69f, 0.34f, .75f);
	static const ImColor health_green     = ImColor(0.29f, 0.69f, 0.34f, 1.f);
	static const ImColor health_yellow_bg = ImColor(0.69f, 0.49f, 0.29f, .75f);
	static const ImColor health_yellow    = ImColor(0.69f, 0.49f, 0.29f, 1.f);
	static const ImColor health_red_bg    = ImColor(0.69f, 0.29f, 0.29f, .75f);
	static const ImColor health_red       = ImColor(0.69f, 0.29f, 0.29f, 1.f);

	static std::mutex g_peds_mutex;
	std::vector<CPed*> esp_ped_pool;

	// Since rendering is asyncronous, we're going to use the script thread to create a mutex-protected local copy of our ped pool which we will then iterate through in draw_esp
	void looped::esp_get_peds()
	{
		// Since this is looping all the time, just make sure we're not calling get_all_peds() unless the NPC ESP feature is enabled
		if (!g.esp_npc.enabled)
			return;

		std::scoped_lock lock(g_peds_mutex);
		esp_ped_pool.clear(); // Make sure we've got a clean vector every loop

		for (auto ped : pools::get_all_peds())
		{
			if (!ped || ped == g_local_player)
				continue;

			esp_ped_pool.push_back(static_cast<CPed*>(ped));
		}
	}

	void esp::draw_npc(CPed* cped, ImDrawList* const draw_list)
	{
		// Ignore nulls, players, and animals (maybe we can make animals configurable later)

		if (!cped || cped->m_player_info || cped->get_ped_type() == 28)
			return;
		
		// We can preserve essentially all of draw_player's functionality, just apply it to peds with maybe some additional ped-type classifications

		if (g.esp_npc.only_armed && cped->m_weapon_manager && cped->m_weapon_manager->m_weapon_info)
		{
			// Checking for firetype none lets us skip over a lot of things that have weapon hashes but aren't weapons (phones, food, etc.)
			// Also check for Unarmed (bare fists) since it's still considered melee
			if (cped->m_weapon_manager->m_selected_weapon_hash == 0xA2719263 || cped->m_weapon_manager->m_weapon_info->m_fire_type == eFireType::None)
			{
				return;
			}
		}

		if (cped->m_health <= 0)
			return;

		rage::fvector3 ped_position_fvec = *cped->get_position();
		
		rage::fvector2 screen;

		const float distance = math::calculate_distance_from_game_cam(ped_position_fvec);
		const float multplr  = distance > g.esp_npc.global_render_distance[1] ? -1.f : 6.17757f / distance;

		// Use the same global rendering distances, maybe break them out in the future to prevent ESP overload (probably don't need to draw as many peds from as far away as we do players)
		if (multplr == -1.f || g.esp_npc.global_render_distance[0] > distance)
			return;

		if (world_to_screen::w2s(ped_position_fvec, screen))
		{
			const auto esp_x = screen.x;
			const auto esp_y = screen.y;

			std::string name_str;
			ImVec2 name_pos = {esp_x - (62.5f * multplr), esp_y - (175.f * multplr) - 20.f};
			ImU32 esp_color = g.esp_npc.npc_unarmed_color;

            if (cped->m_weapon_manager && cped->m_weapon_manager->m_weapon_info && cped->m_weapon_manager->m_selected_weapon_hash != "weapon_unarmed"_J && cped->m_weapon_manager->m_weapon_info->m_fire_type != eFireType::None)
            {
                esp_color = g.esp_npc.npc_armed_color;
            }

			const auto armor_perc  = cped->m_armor / 50.f;
			const auto health_perc = cped->m_health / (cped->m_maxhealth + 0.001f);

			if (distance < g.esp_npc.tracer_render_distance[1] && distance > g.esp_npc.tracer_render_distance[0]
			    && g.esp_npc.tracer)
				draw_list->AddLine({(float)*g_pointers->m_gta.m_resolution_x * g.esp_npc.tracer_draw_position[0],
				                       (float)*g_pointers->m_gta.m_resolution_y * g.esp_npc.tracer_draw_position[1]},
					{esp_x, esp_y},
					esp_color);

			if (distance < g.esp_npc.box_render_distance[1] && distance > g.esp_npc.box_render_distance[0] && g.esp_npc.box)
				draw_list->AddRect({esp_x - (62.5f * multplr), esp_y - (175.f * multplr)}, {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y - (175.f * multplr) + (350.f * multplr)}, esp_color);

			if (distance < g.esp_npc.bone_render_distance[1] && distance > g.esp_npc.bone_render_distance[0]
			    && g.esp_npc.bone)
			{
				draw_skeleton(cped, draw_list, multplr, esp_color, g.esp_npc.only_draw_head);
			}

			if (g.esp_npc.name)
				name_str = g_gta_data_service->ped_by_hash(cped->m_model_info->m_hash).m_name;

			if (g.esp_npc.distance)
			{
				if (g.esp_npc.name)
					name_str += " | ";
				name_str += std::to_string((int)distance);
				name_str += "m";
			}

			std::string extra_info = "";

			if (g.esp_npc.weapon && cped->m_weapon_manager)
			{
				uint32_t weapon_hash = cped->m_weapon_manager->m_selected_weapon_hash;

				if (weapon_hash)
				{
					extra_info += g_gta_data_service->weapon_by_hash(weapon_hash).m_display_name;
				}

			if (g.esp_npc.vehicle && cped->m_vehicle && cped->m_vehicle->m_model_info)
			{
				int32_t vehicle_hash = cped->m_vehicle->m_model_info->m_hash;
				bool in_vehicle = false;

				if (cped == cped->m_vehicle->m_driver)
				{
					in_vehicle = true;
				}
				else
				{
					for (int i = 0; i < 15; i++)
					{
						if (cped->m_vehicle->m_passengers[i] == cped)
						{
							in_vehicle = true;
							break;
						}
					}
				}

				if (vehicle_hash && in_vehicle)
				{
					if (!extra_info.empty())
						extra_info += " | ";

					extra_info += g_gta_data_service->vehicle_by_hash(vehicle_hash).m_display_name;
				}
			}

			draw_list->AddText(name_pos, esp_color, name_str.c_str());

			if (!extra_info.empty())
				draw_list->AddText({esp_x - (62.5f * multplr), esp_y - (175.f * multplr) + 20.f}, esp_color, extra_info.c_str());

			if (g.esp_npc.health)
			{
				if (g.esp_npc.scale_health_from_dist)
				{
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
					    {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y + (175.f * multplr) + 5.f},
					    health_perc == 0.f      ? death_bg :
					        health_perc < 0.25f ? health_red_bg :
					        health_perc < 0.65f ? health_yellow_bg :
					                              health_green_bg,
					    4);
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
					    {esp_x - (62.5f * multplr) + (125.f * multplr) * health_perc, esp_y + (175.f * multplr) + 5.f},
					    health_perc < 0.25f     ? health_red :
					        health_perc < 0.65f ? health_yellow :
					                              health_green,
					    4);
				}
				else
				{
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
					    {esp_x - (62.5f * multplr) + (100.f), esp_y + (175.f * multplr) + 5.f},
					    health_perc == 0.f      ? death_bg :
					        health_perc < 0.25f ? health_red_bg :
					        health_perc < 0.65f ? health_yellow_bg :
					                              health_green_bg,
					    4);
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
					    {esp_x - (62.5f * multplr) + (100.f * health_perc), esp_y + (175.f * multplr) + 5.f},
					    health_perc < 0.25f     ? health_red :
					        health_perc < 0.65f ? health_yellow :
					                              health_green,
					    4);
				}
			}

			if (g.esp_npc.armor && cped->m_armor > 0)
			{
				float offset = 5.f;
				offset       = g.esp_npc.health ? 10.f : 5.f;
				if (g.esp_npc.scale_armor_from_dist)
				{
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y + (175.f * multplr) + offset}, armor_blue_bg, 4);
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (125.f * multplr) * armor_perc, esp_y + (175.f * multplr) + offset}, armor_blue, 4);
				}
				else
				{
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (100.f), esp_y + (175.f * multplr) + offset}, armor_blue_bg, 4);
					draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (100.f * armor_perc), esp_y + (175.f * multplr) + offset}, armor_blue, 4);
				}
			}

		}
	}

	void esp::draw_player(const player_ptr& plyr, ImDrawList* const draw_list)
	{
		if (!plyr || !plyr->is_valid() || !plyr->get_ped() || !plyr->get_ped()->m_navigation)
			return;

		auto& player_pos = *plyr->get_ped()->m_navigation->get_position();

		rage::fvector2 screen;

		const float distance = math::calculate_distance_from_game_cam(player_pos);
		const float multplr  = distance > g.esp_player.global_render_distance[1] ? -1.f : 6.17757f / distance;

		if (multplr == -1.f || g.esp_player.global_render_distance[0] > distance)
			return;

		uint32_t ped_damage_bits = plyr->get_ped()->m_damage_bits;

		if (world_to_screen::w2s(player_pos, screen))
		//if (g_pointers->m_gta.m_get_screen_coords_for_world_coords(player_pos.data, &screen_x, &screen_y))
		{
			const auto esp_x = screen.x;
			const auto esp_y = screen.y;

			std::string name_str;
			ImVec2 name_pos = {esp_x - (62.5f * multplr), esp_y - (175.f * multplr) - 20.f};
			ImU32 esp_color = g.esp_player.default_color;

			if (plyr->is_friend())
			{
				esp_color = g.esp_player.friend_color;
			}

			const auto armor_perc  = plyr->get_ped()->m_armor / 50.f;
			const auto health_perc = plyr->get_ped()->m_health / (plyr->get_ped()->m_maxhealth + 0.001f);

			if (distance < g.esp_player.tracer_render_distance[1] && distance > g.esp_player.tracer_render_distance[0]
			    && g.esp_player.tracer)
				draw_list->AddLine({(float)*g_pointers->m_gta.m_resolution_x * g.esp_player.tracer_draw_position[0],
				                       (float)*g_pointers->m_gta.m_resolution_y * g.esp_player.tracer_draw_position[1]},
				    {esp_x, esp_y},
				    esp_color);

			if (distance < g.esp_player.box_render_distance[1] && distance > g.esp_player.box_render_distance[0]
			    && g.esp_player.box)
				draw_list->AddRect({esp_x - (62.5f * multplr), esp_y - (175.f * multplr)}, {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y - (175.f * multplr) + (350.f * multplr)}, esp_color);

			if (distance < g.esp_player.bone_render_distance[1] && distance > g.esp_player.bone_render_distance[0]
			    && g.esp_player.bone)
			{
				draw_skeleton(plyr->get_ped(), draw_list, multplr, esp_color, g.esp_player.only_draw_head);
			}

			if (g.esp_player.name)
				name_str = plyr->get_name();

			if (g.esp_player.distance)
			{
				if (g.esp_player.name)
					name_str += " | ";
				name_str += std::to_string((int)distance);
				name_str += "m";
			}

			std::string extra_info = "";

			if (g.esp_player.weapon && plyr->get_ped()->m_weapon_manager)
			{
				uint32_t weapon_hash = plyr->get_ped()->m_weapon_manager->m_selected_weapon_hash;

				if (weapon_hash)
				{
					extra_info += g_gta_data_service->weapon_by_hash(weapon_hash).m_display_name;
				}
			}

			if (g.esp_player.vehicle && plyr->get_ped()->m_vehicle && plyr->get_ped()->m_vehicle->m_model_info)
			{
				int32_t vehicle_hash = plyr->get_ped()->m_vehicle->m_model_info->m_hash;
				bool in_vehicle = false;

				if (plyr->get_ped() == plyr->get_ped()->m_vehicle->m_driver)
				{
					in_vehicle = true;
				}
				else
				{
					for (int i = 0; i < 15; i++)
					{
						if (plyr->get_ped()->m_vehicle->m_passengers[i] == plyr->get_ped())
						{
							in_vehicle = true;
							break;
						}
					}
				}

				if (vehicle_hash && in_vehicle)
				{
					if (!extra_info.empty())
						extra_info += " | ";

					extra_info += g_gta_data_service->vehicle_by_hash(vehicle_hash).m_display_name;
				}
			}

			draw_list->AddText(name_pos, esp_color, name_str.c_str());

			if (!extra_info.empty())
				draw_list->AddText({esp_x - (62.5f * multplr), esp_y - (175.f * multplr) + 20.f}, esp_color, extra_info.c_str());

			bool player_god      = false;

			std::string mode_str = "";
			if (g.esp_player.god)
			{
				if (ped_damage_bits & (uint32_t)eEntityProofs::GOD)
				{
					player_god = true;
					mode_str += "ESP_GOD"_T.data();
				}
				else
				{
					bool bullet = false;
					bool explosion = false;

					if (ped_damage_bits & (uint32_t)eEntityProofs::BULLET)
					{
						bullet = true;
					}
					if (ped_damage_bits & (uint32_t)eEntityProofs::EXPLOSION)
					{
						explosion = true;
					}

					if (bullet)
						mode_str += "ESP_BULLET"_T.data();
					if (bullet && explosion)
						mode_str += " ";
					if (explosion)
						mode_str += "ESP_EXPLOSION"_T.data();
				}
			}

			if (auto player_vehicle = plyr->get_current_vehicle(); player_vehicle && (plyr->get_ped()->m_ped_task_flag & (uint32_t)ePedTask::TASK_DRIVING)
			    && (player_vehicle->m_damage_bits & (uint32_t)eEntityProofs::GOD))
			{
				// Add a spacer between "God" and "Vehicle God" if both are enabled
				if (player_god)
				{
					mode_str += " | ";
				}

				mode_str += "ESP_VEH_GOD"_T.data();
			}

			if (!mode_str.empty())
			{
				draw_list->AddText({esp_x - (62.5f * multplr), esp_y - (175.f * multplr) - 40.f},
				    ImColor(1.f, 0.f, 0.f, 1.f),
				    mode_str.c_str());
			}

			if (!in_god)
			{
				if (g.esp_player.health)
				{
					if (g.esp_player.scale_health_from_dist)
					{
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
						    {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y + (175.f * multplr) + 5.f},
						    health_perc == 0.f      ? death_bg :
						        health_perc < 0.25f ? health_red_bg :
						        health_perc < 0.65f ? health_yellow_bg :
						                              health_green_bg,
						    4);
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
						    {esp_x - (62.5f * multplr) + (125.f * multplr) * health_perc, esp_y + (175.f * multplr) + 5.f},
						    health_perc < 0.25f     ? health_red :
						        health_perc < 0.65f ? health_yellow :
						                              health_green,
						    4);
					}
					else
					{
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
						    {esp_x - (62.5f * multplr) + (100.f), esp_y + (175.f * multplr) + 5.f},
						    health_perc == 0.f      ? death_bg :
						        health_perc < 0.25f ? health_red_bg :
						        health_perc < 0.65f ? health_yellow_bg :
						                              health_green_bg,
						    4);
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + 5.f},
						    {esp_x - (62.5f * multplr) + (100.f * health_perc), esp_y + (175.f * multplr) + 5.f},
						    health_perc < 0.25f     ? health_red :
						        health_perc < 0.65f ? health_yellow :
						                              health_green,
						    4);
					}
				}
				if (g.esp_player.armor && plyr->get_ped()->m_armor > 0)
				{
					float offset = 5.f;
					offset       = g.esp_player.health ? 10.f : 5.f;
					if (g.esp_player.scale_armor_from_dist)
					{
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (125.f * multplr), esp_y + (175.f * multplr) + offset}, armor_blue_bg, 4);
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (125.f * multplr) * armor_perc, esp_y + (175.f * multplr) + offset}, armor_blue, 4);
					}
					else
					{
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (100.f), esp_y + (175.f * multplr) + offset}, armor_blue_bg, 4);
						draw_list->AddLine({esp_x - (62.5f * multplr), esp_y + (175.f * multplr) + offset}, {esp_x - (62.5f * multplr) + (100.f * armor_perc), esp_y + (175.f * multplr) + offset}, armor_blue, 4);
					}
				}
			}
		}
	}

    bool esp::bone_to_screen(CPed* cped, ePedBoneType bone_type, rage::fvector2& bone_vec)
    {
        if (cped == nullptr)
            return false;

        // Validate stability of get_bone_coords
		auto ped_bones = cped->get_bone_coords(bone_type);

        if (world_to_screen::w2s(ped_bones, bone_vec))
            return true;

        return false;
    }

	void esp::draw_skeleton(CPed* cped, ImDrawList* const draw_list, float multplr, ImColor esp_color, bool only_draw_head)
	{
		// Map bone locations to x/y on screen
		rage::fvector2 head_pos;
		bool head_valid = esp::bone_to_screen(cped, ePedBoneType::HEAD, head_pos);
		if (head_valid)
		{
			// Draw circle around head
			draw_list->AddCircle(ImVec2(head_pos.x, head_pos.y), 20.f * multplr, esp_color, 0, 2.0f);
		}

		if (!only_draw_head)
		{
			rage::fvector2 neck_pos;
			bool neck_valid = bone_to_screen(cped, ePedBoneType::NECK, neck_pos);
			if (head_valid && neck_valid)
			{
				// Head to neck
				draw_list->AddLine(ImVec2(head_pos.x, head_pos.y), ImVec2(neck_pos.x, neck_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 abdomen_pos;
			bool abdomen_valid = bone_to_screen(cped, ePedBoneType::ABDOMEN, abdomen_pos);
			if (neck_valid && abdomen_valid)
			{
				// Neck to abdomen
				draw_list->AddLine(ImVec2(neck_pos.x, neck_pos.y), ImVec2(abdomen_pos.x, abdomen_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 l_hand_pos;
			bool l_hand_valid = bone_to_screen(cped, ePedBoneType::L_HAND, l_hand_pos);
			if (neck_valid && l_hand_valid)
			{
				// Neck to left hand
				draw_list->AddLine(ImVec2(neck_pos.x, neck_pos.y), ImVec2(l_hand_pos.x, l_hand_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 r_hand_pos;
			bool r_hand_valid = bone_to_screen(cped, ePedBoneType::R_HAND, r_hand_pos);
			if (neck_valid && r_hand_valid)
			{
				// Neck to right hand
				draw_list->AddLine(ImVec2(neck_pos.x, neck_pos.y), ImVec2(r_hand_pos.x, r_hand_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 l_ankle_pos;
			bool l_ankle_valid = bone_to_screen(cped, ePedBoneType::L_ANKLE, l_ankle_pos);
			if (abdomen_valid && l_ankle_valid)
			{
				// Abdomen to left ankle
				draw_list->AddLine(ImVec2(abdomen_pos.x, abdomen_pos.y), ImVec2(l_ankle_pos.x, l_ankle_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 r_ankle_pos;
			bool r_ankle_valid = bone_to_screen(cped, ePedBoneType::R_ANKLE, r_ankle_pos);
			if (abdomen_valid && r_ankle_valid)
			{
				// Abdomen to right ankle
				draw_list->AddLine(ImVec2(abdomen_pos.x, abdomen_pos.y), ImVec2(r_ankle_pos.x, r_ankle_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 l_foot_pos;
			bool l_foot_valid = bone_to_screen(cped, ePedBoneType::L_FOOT, l_foot_pos);
			if (l_ankle_valid && l_foot_valid)
			{
				// Left ankle to left foot
				draw_list->AddLine(ImVec2(l_ankle_pos.x, l_ankle_pos.y), ImVec2(l_foot_pos.x, l_foot_pos.y), esp_color, 2.0f);
			}

			rage::fvector2 r_foot_pos;
			bool r_foot_valid = bone_to_screen(cped, ePedBoneType::R_FOOT, r_foot_pos);
			if (r_ankle_valid && r_foot_valid)
			{
				// Right ankle to right foot
				draw_list->AddLine(ImVec2(r_ankle_pos.x, r_ankle_pos.y), ImVec2(r_foot_pos.x, r_foot_pos.y), esp_color, 2.0f);
			}
		}
	}

	void esp::draw()
	{
		if (!g.esp_player.enabled && !g.esp_npc.enabled)
			return;

		if (const auto draw_list = ImGui::GetBackgroundDrawList(); draw_list)
		{
			// draw_player(g_player_service->get_self(), draw_list); // Draw ESP on self, useful for debugging

			if (g.esp_player.enabled)
			{
				g_player_service->iterate([draw_list](const player_entry& entry) {
					draw_player(entry.second, draw_list);
				});
			}

            if (g.esp_npc.enabled)
            {
                std::vector<CPed*> local_ped_pool; // Create a local copy of esp_ped_pool

                {
                    std::scoped_lock lock(g_peds_mutex); // Use the scoped lock just for the vector local copy so we don't hold it for the entire for loop
                    local_ped_pool = esp_ped_pool;
                }

                for (auto ped : local_ped_pool)
                {
                    // We've already verified that the peds in local_ped_pool are valid, just draw them
                    draw_npc(ped, draw_list);
                }
            }
		}
	}
}