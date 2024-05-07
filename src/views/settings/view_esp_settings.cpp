#include "views/view.hpp"

namespace big
{
	static int esp_radio_button_index = 0;

	void view::esp_settings()
	{
		ImGui::RadioButton("SETTINGS_ESP_PLAYER"_T.data(), &esp_radio_button_index, 0);
		ImGui::SameLine();
		ImGui::RadioButton("SETTINGS_ESP_NPC"_T.data(), &esp_radio_button_index, 1);
		ImGui::Separator();

		if (esp_radio_button_index == 0)
		{
			ImGui::Checkbox("ENABLED"_T.data(), &g.esp_player.enabled);

			if (g.esp_player.enabled)
			{
				ImGui::BeginGroup();
				ImGui::Text("SETTINGS_ESP_GLOBAL_RENDER_DISTANCE"_T.data());
				ImGui::SliderFloat2("###Global Render Distance", g.esp_player.global_render_distance, 0.f, 1500.f);

				ImGui::Checkbox("SETTINGS_ESP_TRACER"_T.data(), &g.esp_player.tracer);
				if (g.esp_player.tracer)
				{
					ImGui::Text("SETTINGS_ESP_TRACER_POSITION"_T.data());
					ImGui::SliderFloat2("###Draw Position", g.esp_player.tracer_draw_position, 0.f, 1.f);
					ImGui::Text("SETTINGS_ESP_TRACER_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Tracer Render Distance",
					    g.esp_player.tracer_render_distance,
					    g.esp_player.global_render_distance[0],
					    g.esp_player.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_BOX"_T.data(), &g.esp_player.box);
				if (g.esp_player.box)
				{
					ImGui::Text("SETTINGS_ESP_BOX_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Box Render Distance",
					    g.esp_player.box_render_distance,
					    g.esp_player.global_render_distance[0],
					    g.esp_player.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_BONE"_T.data(), &g.esp_player.bone);
				if (g.esp_player.bone)
				{
					ImGui::Text("SETTINGS_ESP_BONE_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Bone Render Distance",
					    g.esp_player.bone_render_distance,
					    g.esp_player.global_render_distance[0],
					    g.esp_player.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_NAME"_T.data(), &g.esp_player.name);
				ImGui::Checkbox("SETTINGS_ESP_WEAPON"_T.data(), &g.esp_player.weapon);
				ImGui::Checkbox("SETTINGS_ESP_VEHICLE"_T.data(), &g.esp_player.vehicle);
				ImGui::Checkbox("SETTINGS_ESP_DISTANCE"_T.data(), &g.esp_player.distance);
				ImGui::Checkbox("SETTINGS_ESP_GOD_MODE"_T.data(), &g.esp_player.god);
				ImGui::Checkbox("SETTINGS_ESP_HEALTH"_T.data(), &g.esp_player.health);
				ImGui::Checkbox("SETTINGS_ESP_ARMOR"_T.data(), &g.esp_player.armor);

				if (g.esp_player.health)
					ImGui::Checkbox("SETTINGS_ESP_SCALE_HEALTH"_T.data(), &g.esp_player.scale_health_from_dist);

				if (g.esp_player.armor)
					ImGui::Checkbox("SETTINGS_ESP_SCALE_ARMOR"_T.data(), &g.esp_player.scale_armor_from_dist);

				static ImVec4 col_enemy      = ImGui::ColorConvertU32ToFloat4(g.esp_player.enemy_color);
				static ImVec4 col_default    = ImGui::ColorConvertU32ToFloat4(g.esp_player.default_color);
				static ImVec4 col_friend     = ImGui::ColorConvertU32ToFloat4(g.esp_player.friend_color);

				if (ImGui::TreeNode("SETTINGS_ESP_COLORS"_T.data()))
				{
					ImGui::Text("SETTINGS_ESP_ENEMY_DEFAULT_COLOR"_T.data());
					if (ImGui::ColorEdit4("###Default ESP Color##esp_picker", (float*)&col_default, ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoSidePreview))
					{
						g.esp_player.default_color = ImGui::ColorConvertFloat4ToU32(col_default);
					}

					ImGui::Text("SETTINGS_ESP_FRIENDLY_COLOR"_T.data());
					if (ImGui::ColorEdit4("###Friend ESP Color##friend_picker", (float*)&col_friend, ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoSidePreview))
					{
						g.esp_player.friend_color = ImGui::ColorConvertFloat4ToU32(col_friend);
					}
					ImGui::EndGroup();
				}
			}
		}

		if (esp_radio_button_index == 1)
		{
			ImGui::Checkbox("ENABLED"_T.data(), &g.esp_npc.enabled);

			if (g.esp_npc.enabled)
			{
				ImGui::BeginGroup();
				ImGui::Checkbox("SETTINGS_ESP_ONLY_ARMED"_T.data(), &g.esp_npc.only_armed);
				ImGui::Text("SETTINGS_ESP_GLOBAL_RENDER_DISTANCE"_T.data());
				ImGui::SliderFloat2("###Global Render Distance", g.esp_npc.global_render_distance, 0.f, 1500.f);

				ImGui::Checkbox("SETTINGS_ESP_TRACER"_T.data(), &g.esp_npc.tracer);
				if (g.esp_npc.tracer)
				{
					ImGui::Text("SETTINGS_ESP_TRACER_POSITION"_T.data());
					ImGui::SliderFloat2("###Draw Position", g.esp_npc.tracer_draw_position, 0.f, 1.f);
					ImGui::Text("SETTINGS_ESP_TRACER_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Tracer Render Distance",
					    g.esp_npc.tracer_render_distance,
					    g.esp_npc.global_render_distance[0],
					    g.esp_npc.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_BOX"_T.data(), &g.esp_npc.box);
				if (g.esp_npc.box)
				{
					ImGui::Text("SETTINGS_ESP_BOX_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Box Render Distance",
					    g.esp_npc.box_render_distance,
					    g.esp_npc.global_render_distance[0],
					    g.esp_npc.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_BONE"_T.data(), &g.esp_npc.bone);
				if (g.esp_npc.bone)
				{
					ImGui::Text("SETTINGS_ESP_BONE_RENDER_DISTANCE"_T.data());
					ImGui::SliderFloat2("###Bone Render Distance",
					    g.esp_npc.bone_render_distance,
					    g.esp_npc.global_render_distance[0],
					    g.esp_npc.global_render_distance[1]);
				}

				ImGui::Checkbox("SETTINGS_ESP_NAME"_T.data(), &g.esp_npc.name);
				ImGui::Checkbox("SETTINGS_ESP_WEAPON"_T.data(), &g.esp_npc.weapon);
				ImGui::Checkbox("SETTINGS_ESP_VEHICLE"_T.data(), &g.esp_npc.vehicle);
				ImGui::Checkbox("SETTINGS_ESP_DISTANCE"_T.data(), &g.esp_npc.distance);
				ImGui::Checkbox("SETTINGS_ESP_GOD_MODE"_T.data(), &g.esp_npc.god);
				ImGui::Checkbox("SETTINGS_ESP_HEALTH"_T.data(), &g.esp_npc.health);
				ImGui::Checkbox("SETTINGS_ESP_ARMOR"_T.data(), &g.esp_npc.armor);

				if (g.esp_npc.health)
					ImGui::Checkbox("SETTINGS_ESP_SCALE_HEALTH"_T.data(), &g.esp_npc.scale_health_from_dist);

				if (g.esp_npc.armor)
					ImGui::Checkbox("SETTINGS_ESP_SCALE_ARMOR"_T.data(), &g.esp_player.scale_armor_from_dist);

				static ImVec4 col_armed    = ImGui::ColorConvertU32ToFloat4(g.esp_npc.npc_armed_color);
				static ImVec4 col_unarmed = ImGui::ColorConvertU32ToFloat4(g.esp_npc.npc_unarmed_color);

				if (ImGui::TreeNode("SETTINGS_ESP_COLORS"_T.data()))
				{

					ImGui::Text("SETTINGS_ESP_ARMED_COLOR"_T.data());
					if (ImGui::ColorEdit4("###Armed ESP Color##esp_picker", (float*)&col_armed, ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoSidePreview))
					{
						g.esp_npc.npc_armed_color = ImGui::ColorConvertFloat4ToU32(col_armed);
					}

					ImGui::Text("SETTINGS_ESP_UNARMED_COLOR"_T.data());
					if (ImGui::ColorEdit4("###Unarmed ESP Color##friend_picker", (float*)&col_unarmed, ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoSidePreview))
					{
						g.esp_npc.npc_unarmed_color = ImGui::ColorConvertFloat4ToU32(col_unarmed);
					}
					ImGui::EndGroup();
				}
			}
		}
	}
}
