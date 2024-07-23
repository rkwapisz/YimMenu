#include "backend/looped_command.hpp"
#include "gta/enums.hpp"
#include "natives.hpp"
#include "hooking/hooking.hpp"
#include "util/entity.hpp"
#include "util/ped.hpp"
#include "util/pools.hpp"
#include "util/world_to_screen.hpp"
#include <chrono>
#include <cmath>
#include "util/math.hpp"
#include <numbers>

namespace big
{
	static inline CPed* target_cped = nullptr;

	class aimbot : looped_command
	{
		// Stage 1: Target Acquisition
		// Stage 2: Target Tracking
		// Stage 3: Target Reset

		using looped_command::looped_command;

		public:

			virtual void on_tick() override
			{
				if (!PLAYER::IS_PLAYER_FREE_AIMING(self::id))
				{
					return;
				}

				CAM::STOP_SCRIPT_GLOBAL_SHAKING(true);
				CAM::SET_GAMEPLAY_CAM_SHAKE_AMPLITUDE(0);
			}

			static rage::fvector3 get_camera_position()
			{
				uintptr_t cam_gameplay_director = *g_pointers->m_gta.m_cam_gameplay_director;
				uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);
				return *reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x60);
			}

			static rage::fvector3 get_camera_aim_direction()
			{
				uintptr_t cam_gameplay_director = *g_pointers->m_gta.m_cam_gameplay_director;
				uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);

				uintptr_t cam_follow_ped_camera_metadata = *reinterpret_cast<uintptr_t*>(cam_follow_ped_camera + 0x10);
				bool is_first_person = *reinterpret_cast<float*>(cam_follow_ped_camera_metadata + 0x30) == 0.0f;
				// We only use 0x40 in first person on foot
				if (is_first_person)
				{
					// This is our first-person camera direction
					return reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x40)->normalize();
				}
				else
				{
					// This is our third-person camera direction, and yes it is different from the first-person
					return reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x3'D0)->normalize();
				}
			}

			static void reset_target()
			{
				target_cped = nullptr;
			}

			static void acquire_target()
			{
				// Only acquire a target if we don't already have one
			    if (target_cped)
				    return;

			    ePedBoneType aim_bone = g.weapons.aimbot.aim_bone_ped;

			    // Stage 1: Target Acquisition
			    rage::fvector2 resolution = {(float)*g_pointers->m_gta.m_resolution_x, (float)*g_pointers->m_gta.m_resolution_y};

			    int plyr_team = PLAYER::GET_PLAYER_TEAM(self::id);

			    for (auto ped : pools::get_all_peds())
			    {
				    if (!ped || ped == g_local_player)
					    continue;

				    CPed* cped = static_cast<CPed*>(ped);

				    // Don't trying acquiring a target if we're already locked onto one or if the cped is invalid
				    if (cped == nullptr)
					    continue;

				    // First, filter out all dead peds
				    if (cped->m_health <= 0.0f)
					    continue;

				    // Ignore peds that are in the vehicle with the player
				    if (g_local_player->m_vehicle && g_local_player->m_vehicle == cped->m_vehicle)
					    continue;

				    CVehicle* target_vehicle = cped->m_vehicle;
				    bool in_vehicle          = false;

				    if (target_vehicle)
				    {
					    if (cped == target_vehicle->m_driver)
					    {
						    in_vehicle = true;
					    }
					    else
					    {
						    for (int i = 0; i < 15; i++)
						    {
							    if (cped == target_vehicle->m_passengers[i])
							    {
								    in_vehicle = true;
								    break;
							    }
						    }
					    }
				    }

				    if (in_vehicle)
					    aim_bone = g.weapons.aimbot.aim_bone_veh; // If we're in a vehicle, use the in-vehicle preferred bone

				    rage::fvector3 camera_position = aimbot::get_camera_position();
				    rage::fvector3 ped_position    = cped->get_bone_coords(aim_bone);

				    float ped_to_cam_distance = math::calculate_distance_from_game_cam(ped_position);

				    // Don't flip out if an enemy is on top of us
				    if (ped_to_cam_distance < 0.5f || ped_to_cam_distance > 1000.0f)
					    continue;

				    rage::fvector2 screen = {0.f, 0.f};
				    world_to_screen::w2s({ped_position.x, ped_position.y, ped_position.z}, screen);

				    float xDelta = screen.x - (resolution.x * 0.5f); // How far from center (crosshair) is X?
				    float yDelta = screen.y - (resolution.y * 0.5f); // How far from center (crosshair) is Y?

				    // Note that the values returned into xScreen and yScreen by the W2S function range from [0,0] (top left) to [1,1] (bottom right)
				    // Largest supported magnitude will obviously be sqrt(0.5^2 + 0.5^2) = 0.707 which is what we'll use in the GUI for now
				    // TODO: Create a more easily understandable mapping between GUI value and actual implementation (maybe pixels? maybe draw a box on-screen that shows the valid area?)
				    float crosshairMag = sqrtf(xDelta * xDelta + yDelta * yDelta);

				    if (crosshairMag > g.weapons.aimbot.fov)
					    continue;

				    // Save the LOS check for last since it's the most expensive
				    // LOS check on the bone... if we can't get a clear shot then find a new target

				    Entity target_ped = g_pointers->m_gta.m_ptr_to_handle(cped);

				    auto shape_test_handle = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(camera_position.x,
				        camera_position.y,
				        camera_position.z,
				        ped_position.x,
				        ped_position.y,
				        ped_position.z,
				        ST_INCLUDE_ALL,
				        self::ped,
				        ST_OPTION_IGNORE_GLASS | ST_OPTION_IGNORE_NOTHING | ST_OPTION_IGNORE_TRANSPARENT);

				    int did_shapetest_hit;
				    Vector3 hit_coords;
				    Entity entity_hit;

				    if (SHAPETEST::GET_SHAPE_TEST_RESULT(shape_test_handle, &did_shapetest_hit, &hit_coords, &hit_coords, &entity_hit))
				    {
					    if (!((did_shapetest_hit && entity_hit == target_ped) || !did_shapetest_hit))
					    {
						    continue;
					    }
				    }

				    // Now that we've filtered out most of what we want to ignore, our remaining peds are all alive, within our scan area, and targetable
				    // From this list of potentially valid targets, let's pick one!

				    uint32_t type = cped->get_ped_type();

				    // If target is a player and we're aiming at players
				    player_ptr target_plyr = ped::get_player_from_ped(target_ped);
				    if (g.weapons.aimbot.on_player && target_plyr)
				    {
					    // Don't aim at our own teammates
					    if (plyr_team != -1 && target_plyr)
					    {
						    int target_team = PLAYER::GET_PLAYER_TEAM(target_plyr->id());

						    if (plyr_team == target_team)
							    continue;
					    }

					    target_cped = cped;
					    break;
				    }
				    // If target is armed and we're aiming at armed NPCs
				    // Note that we check !target_plyr since player targeting is a separate option
				    // Type == 28 check exists because animals are armed (lol)
				    else if (!target_plyr && g.weapons.aimbot.on_armed && type != 28
				        && !(cped->m_weapon_manager->m_selected_weapon_hash == 0xA2719263
				            || cped->m_weapon_manager->m_weapon_info->m_fire_type == eFireType::None))
				    {
					    target_cped = cped;
					    break;
				    }
				    // If target is an NPC and we're aiming at all NPCs
				    // TODO: Maybe filter out animals (type 28)?
				    else if (!target_plyr && g.weapons.aimbot.on_npc && !target_plyr && type != 28)
				    {
					    target_cped = cped;
					    break;
				    }
			    }
		    }

			static void track_target()
		    {
			    ePedBoneType aim_bone = g.weapons.aimbot.aim_bone_ped;

			    if (target_cped->m_health <= 0.0f) // Dead?
			    {
				    // Reset the target entity, and don't bother with the camera stuff since next tick we're scanning for a new target
				    reset_target();
				    return;
			    }

			    // Note that target_vehicle will return the player's current AND/OR last vehicle, so we need to check if the target is actually in the vehicle as a driver or passenger
			    CVehicle* target_vehicle = target_cped->m_vehicle;
			    bool in_vehicle          = false;

			    if (target_vehicle)
			    {
				    if (target_cped == target_vehicle->m_driver)
				    {
					    in_vehicle = true;
				    }
				    else
				    {
					    for (int i = 0; i < 15; i++)
					    {
						    if (target_cped == target_vehicle->m_passengers[i])
						    {
							    in_vehicle = true;
							    break;
						    }
					    }
				    }
			    }

			    if (in_vehicle)
			    {
				    // If we're in a vehicle, use the in-vehicle preferred bone
				    aim_bone = g.weapons.aimbot.aim_bone_veh;
			    }

			    // Check for bastard vehicles like the RC Bandito, I&P Mini Tank, and Oppressors
			    if (in_vehicle && target_vehicle && target_vehicle->m_model_info)
			    {
				    if (target_vehicle->m_model_info->m_hash == "rcbandito"_J || // RCBANDITO
				        target_vehicle->m_model_info->m_hash == "minitank"_J)    // MINITANK
				    {
					    // Peds are technically inside these vehicles which makes headshots shoot WAY over the vehicle
					    // In this case we want to overwrite the user's preferred bone
					    aim_bone = ePedBoneType::ABDOMEN;
				    }
			    }

			    Entity target_ped = g_pointers->m_gta.m_ptr_to_handle(target_cped);

			    // Convert fvector3 to Vector3 for use in is_target_in_los
			    Vector3 target_position = target_cped->get_bone_coords(aim_bone);

			    Vector3 camera_position = aimbot::get_camera_position();

			    Vector3 target_velocity = ENTITY::GET_ENTITY_VELOCITY(target_ped);
			    Vector3 player_velocity = ENTITY::GET_ENTITY_VELOCITY(self::ped);

			    auto shape_test_handle = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(camera_position.x,
			        camera_position.y,
			        camera_position.z,
			        target_position.x,
			        target_position.y,
			        target_position.z,
			        ST_INCLUDE_ALL,
			        self::ped,
			        ST_OPTION_IGNORE_GLASS | ST_OPTION_IGNORE_NOTHING | ST_OPTION_IGNORE_TRANSPARENT);

			    int did_shapetest_hit;
			    Vector3 hit_coords;
			    Entity entity_hit;

			    if (SHAPETEST::GET_SHAPE_TEST_RESULT(shape_test_handle, &did_shapetest_hit, &hit_coords, &hit_coords, &entity_hit))
			    {
				    if (!((did_shapetest_hit && entity_hit == target_ped) || !did_shapetest_hit))
				    {
					    target_cped = nullptr;
					    return;
				    }
			    }

			    rage::fvector3 target_position_fvec = {target_position.x, target_position.y, target_position.z};
			    rage::fvector3 target_velocity_fvec = {target_velocity.x, target_velocity.y, target_velocity.z};

			    // We use get_camera_position() later to get the player's camera position, so no need for natives
			    rage::fvector3 player_position_fvec = *g_local_player->m_navigation->get_position();
			    rage::fvector3 player_velocity_fvec = {player_velocity.x, player_velocity.y, player_velocity.z};

			    // Apply a compensating factor
			    float velocity_comp_factor = g.weapons.aimbot.pred_comp;

			    // Since weapons aren't hitscan, we need some prediction based on bullet speed and target velocity
			    float bullet_speed = g_local_player->m_weapon_manager->m_weapon_info->m_speed;

			    // Calculate the distance between the player and the target
			    float distance = target_position_fvec.distance(player_position_fvec);

			    // Calculate the time it takes for the bullet to reach the target
			    float time = distance / bullet_speed;

			    // Calculate the predicted target position based on the target's velocity, plus our comp factor for all cases
			    target_position_fvec = target_position_fvec + (target_velocity_fvec * time) + (target_velocity_fvec * velocity_comp_factor);

			    if (in_vehicle && target_vehicle)
			    {
				    target_position_fvec.z += g.weapons.aimbot.z_veh_comp;
			    }
			    else
			    {
				    target_position_fvec.z += g.weapons.aimbot.z_foot_comp;
			    }

			    uintptr_t cam_gameplay_director = *g_pointers->m_gta.m_cam_gameplay_director;

			    // Good info here about the layout of the gameplay camera director
			    // https://www.unknowncheats.me/forum/grand-theft-auto-v/144028-grand-theft-auto-reversal-structs-offsets-625.html#post3249805

			    uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);

			    //uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C8);
			    //uintptr_t cam_follow_ped_camera2 = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);
			    //uintptr_t cam_follow_ped_camera3 = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x3'C0);

			    // Convert target_position from Vector3 to rage::fvector3
			    rage::fvector3 camera_position_fvec = aimbot::get_camera_position();

			    // Compensate for player velocity
			    camera_position_fvec = camera_position_fvec + (player_velocity_fvec * velocity_comp_factor);

			    // Finally calculate the vector we write into memory
			    rage::fvector3 camera_target_fvec = (target_position_fvec - camera_position_fvec).normalize();

			    // Game uses different cameras when on-foot vs. in vehicle, which is why using the gameplay cam is such a PITA, but for the aimbot it's fine to write to both locations
			    aimbot::reset_aim_vectors(cam_follow_ped_camera);
			    *reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x40) = camera_target_fvec; // First person & sniper (on foot)
			    *reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x3'D0) = camera_target_fvec; // Third person

				if (g.weapons.aimbot.triggerbot)
					PED::SET_PED_RESET_FLAG(self::ped, 65, TRUE);
		    }

			// This is a total mystery but we need to call it to correct our in-vehicle aim
			static void reset_aim_vectors(uintptr_t camera)
			{
				uintptr_t camera_params = *(uintptr_t*)(camera + 0x10);
				{
					// So far it seems that 0x2AC is only -2.0f when free aiming in a vehicle
					if (*(float*)(camera_params + 0x2'AC) == -2.0f)
					{
						*(float*)(camera_params + 0x2'AC) = 0.0f;
						*(float*)(camera_params + 0x2'B0) = 0.0f;
						*(float*)(camera_params + 0x2'C0) = 111.0f;
						*(float*)(camera_params + 0x2'C4) = 111.0f;
					}

					if (*(float*)(camera_params + 0x1'30) == 8.0f)
					{
						*(float*)(camera_params + 0x1'30) = 111.0f; // def 8.0f
						*(float*)(camera_params + 0x1'34) = 111.0f; // def 10.0f
						*(float*)(camera_params + 0x4'CC) = 0.0f;   // def 4.0f

						if (*(float*)(camera_params + 0x4'9C) == 1.0f)
						{
							*(float*)(camera_params + 0x4'9C) = 0.0f; // def 1.0f
						}

						*(float*)(camera_params + 0x2'AC) = 0.0f; // def -3.0f
						*(float*)(camera_params + 0x2'B0) = 0.0f; // def -8.0f
					}
				}
			}
	};

	aimbot g_aimbot("aimbot", "VIEW_OVERLAY_AIMBOT", "BACKEND_LOOPED_WEAPONS_AIMBOT_DESC", g.weapons.aimbot.enable);

	bool hooks::aimbot_cam_gameplay_director_update(uintptr_t this_)
	{
		const auto result = big::hooking::get_original<hooks::aimbot_cam_gameplay_director_update>()(this_);

		// Reset aim target when we're not aiming
		if (!PLAYER::IS_PLAYER_FREE_AIMING(self::id))
		{
			aimbot::reset_target();
			return result;
		}

		if (!target_cped)
		{
			aimbot::acquire_target();
		}
		else
		{
			aimbot::track_target();
		}

		return result;
	}

	bool_command
		g_aimbot_nonhitscan("nonhitscan", "BACKEND_LOOPED_WEAPONS_AIMBOT_NONHITSCAN", "BACKEND_LOOPED_WEAPONS_AIMBOT_NONHITSCAN_DESC", g.weapons.aimbot.nonhitscan);
	bool_command
		g_aimbot_triggerbot("triggerbot", "BACKEND_LOOPED_WEAPONS_TRIGGERBOT", "BACKEND_LOOPED_WEAPONS_TRIGGERBOT_DESC", g.weapons.aimbot.triggerbot);
	bool_command
	    g_aimbot_on_player("aimatplayer", "PLAYER", "BACKEND_LOOPED_WEAPONS_AIM_AT_PLAYER_DESC", g.weapons.aimbot.on_player);
	bool_command
		g_aimbot_on_npc("aimatnpc", "BACKEND_LOOPED_WEAPONS_AIM_AT_NPC", "BACKEND_LOOPED_WEAPONS_AIM_AT_NPC_DESC", g.weapons.aimbot.on_npc);
	bool_command
		g_aimbot_on_enemy("aimatenemy", "BACKEND_LOOPED_WEAPONS_AIM_AT_ARMED", "BACKEND_LOOPED_WEAPONS_AIM_AT_ARMED_DESC", g.weapons.aimbot.on_armed);
}
