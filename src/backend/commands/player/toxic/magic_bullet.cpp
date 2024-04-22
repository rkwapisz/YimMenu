#include "backend/command.hpp"
#include "gta/enums.hpp"
#include "natives.hpp"
#include "util/entity.hpp"
#include "util/ped.hpp"
#include "util/world_to_screen.hpp"
#include "services/players/player_service.hpp"

#include <numbers>

namespace big
{
	class magic_bullet : command
	{
		using command::command;

		virtual void execute(const command_arguments&, const std::shared_ptr<command_context> ctx) override
		{
			uint8_t target_player = 0;
			float minimum_distance = *g_pointers->m_gta.m_resolution_x;

			player_ptr best_player = nullptr;

			// Only use magic bullet if we're free aiming

			if (!PLAYER::IS_PLAYER_FREE_AIMING(self::id))
				return;

			rage::fvector2 center = {*g_pointers->m_gta.m_resolution_x * 0.5f, *g_pointers->m_gta.m_resolution_y * 0.5f};

			// Get player closest to the crosshair
			g_player_service->iterate([&](auto& plyr) {

				player_ptr target_plyr = plyr.second;

				if (target_plyr == nullptr)
					return;

				rage::fvector3 target_position_fvec = *target_plyr->get_ped()->m_navigation->get_position();
				rage::fvector2 screen_pos;

				world_to_screen::w2s(target_position_fvec, screen_pos);

				// Filter out players that are not on the screen
				if (screen_pos.x < 0 || screen_pos.y < 0 || screen_pos.x > *g_pointers->m_gta.m_resolution_x || screen_pos.y > *g_pointers->m_gta.m_resolution_y)
					return;

				// Calculate the distance from the center of the screen
				float player_distance_from_center = sqrt((screen_pos.x - center.x) * (screen_pos.x - center.x) + (screen_pos.y - center.y) * (screen_pos.y - center.y));

				if (player_distance_from_center < minimum_distance)
				{
					minimum_distance = player_distance_from_center;
					best_player      = target_plyr;
				}
			});

			if (best_player == nullptr)
				return;

			// Calculate from where to fire the bullet
			//rage::fvector3 target_position_fvec = *best_player->get_ped()->m_navigation->get_position();

			Entity target_entity_handle = g_pointers->m_gta.m_ptr_to_handle(best_player->get_ped());

			Vector3 target_position = ENTITY::GET_ENTITY_BONE_POSTION(target_entity_handle, PED::GET_PED_BONE_INDEX(target_entity_handle, static_cast<uint16_t>(PedBones::SKEL_Head)));
			rage::fvector3 target_position_fvec = {target_position.x, target_position.y, target_position.z};

			rage::fvector3 player_position_fvec = *g_local_player->m_navigation->get_position();

			rage::fvector3 magic_vector = target_position_fvec - player_position_fvec;
			magic_vector                = magic_vector.normalize();
			
			rage::fvector3 shoot_from_fvec = target_position_fvec - (magic_vector * 0.005f);
			rage::fvector3 shoot_to_fvec = target_position_fvec + (magic_vector * 0.005f);

			MISC::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(
				shoot_from_fvec.x,
			    shoot_from_fvec.y,
			    shoot_from_fvec.z,
			    shoot_to_fvec.x,
			    shoot_to_fvec.y,
			    shoot_to_fvec.z,
			    g_local_player->m_weapon_manager->m_weapon_info->m_damage, // damage
			    false,                                                     // pureAccuracy
			    g_local_player->m_weapon_manager->m_selected_weapon_hash,  //weaponHash
			    self::ped,                                                 //ownerPed
			    true,                                                      // isAudible
			    true,                                                      // isInvisible
			    -1);
		}
	};

	magic_bullet g_magicbullet("magicbullet", "VIEW_HOTKEY_SETTINGS_MAGIC_BULLET", "VIEW_HOTKEY_SETTINGS_MAGIC_BULLET_DESC", 0);
}
