#include "backend/looped/looped.hpp"
#include "services/players/player_service.hpp"
#include "util/toxic.hpp"
#include "util/cages.hpp"
#include <cmath>
#include <chrono>

#pragma warning(disable : 4244) 

namespace big
{
	static rage::fvector3 previous_position = {0.0f, 0.0f, 0.0f};

	void looped::player_toxic_options()
	{
		if (!*g_pointers->m_gta.m_is_session_started)
			return;

		int rotate_cam_bits = 0;

		static std::chrono::steady_clock::time_point lastExecutionTime;

		g_player_service->iterate([&rotate_cam_bits](const player_entry& entry) {
			if (g_player_service->get_self()->get_ped() && entry.second->get_ped() && entry.second->get_ped()->m_health > 0)
			{
				if (entry.second->kill_loop && !(entry.second->get_ped()->m_damage_bits & (1 << 8)))
					g_pointers->m_gta.m_send_network_damage(g_player_service->get_self()->get_ped(),
					    entry.second->get_ped(),
					    entry.second->get_ped()->m_navigation->get_position(),
					    0,
					    true,
					    "weapon_explosion"_J,
					    10000.0f,
					    2,
					    0,
					    (1 << 4),
					    0,
					    0,
					    0,
					    false,
					    false,
					    true,
					    true,
					    nullptr);

				if (entry.second->explosion_loop)
					toxic::blame_explode_player(entry.second, entry.second, EXP_TAG_SUBMARINE_BIG, 9999.0f, true, false, 9999.0f);

				if (entry.second->ragdoll_loop && entry.second->get_ped()->m_net_object)
					g_pointers->m_gta.m_request_ragdoll(entry.second->get_ped()->m_net_object->m_object_id);

				if (entry.second->rotate_cam_loop)
					rotate_cam_bits |= (1 << entry.second->id());

                if (entry.second->cage_loop)
                {
					rage::fvector3 current_position = *entry.second->get_ped()->m_navigation->get_position();

                    // Calculate the distance between current_position and previous_position
                    float distance = std::sqrt(std::pow(current_position.x - previous_position.x, 2) +
                                               std::pow(current_position.y - previous_position.y, 2) +
                                               std::pow(current_position.z - previous_position.z, 2));

					if (distance >= 100.0f) // Spawn a new cage if the victim moves too far (like if they teleported)
					{
						Ped pedHandle = g_pointers->m_gta.m_ptr_to_handle(entry.second->get_ped());
						float pedHeading = ENTITY::GET_ENTITY_HEADING(pedHandle);

						Vector3 spawnLocation = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(pedHandle, 0.f, 0.0f, -21.5f);
						spawnedCages.push_back(std::make_unique<cageObject>(779277682, spawnLocation.x, spawnLocation.y, spawnLocation.z, pedHeading + 90.0, std::chrono::seconds(20))); // Stunt Tube End
						spawnedCages.push_back(std::make_unique<cageObject>(779277682, spawnLocation.x, spawnLocation.y, spawnLocation.z, pedHeading - 90.0, std::chrono::seconds(20))); // Stunt Tube End

						previous_position = current_position;
					}
                }
			}

			if (rotate_cam_bits)
			{
				const size_t arg_count = 4;
				int64_t args[arg_count] = {(int64_t)eRemoteEvent::TSECommand, (int64_t)self::id, (int64_t)rotate_cam_bits, (int64_t)eRemoteEvent::TSECommandRotateCam};

				g_pointers->m_gta.m_trigger_script_event(1, args, arg_count, rotate_cam_bits, (int)eRemoteEvent::TSECommand);
			}
		});
	}
}
