#include "backend/looped_command.hpp"
#include "gta/enums.hpp"
#include "natives.hpp"
#include "util/entity.hpp"
#include "util/world_to_screen.hpp"

#include <numbers>
namespace big
{
	class aimbot : looped_command
	{
		static inline float playerToPedDistance;

		static inline Entity target_entity = 0;
		static inline uint16_t aimBone = (uint16_t)PedBones::SKEL_Head;

		// Stage 1: Target Acquisition
		// Stage 2: Target Tracking
		// Stage 3: Target Reset

		using looped_command::looped_command;
		virtual void on_tick() override
		{
			// Reset aim target when we're not aiming
			if (!PLAYER::IS_PLAYER_FREE_AIMING(self::id)) {
				target_entity = 0;
			}

			// Only process aim targets while we're actually free aiming
			if (PLAYER::IS_PLAYER_FREE_AIMING(self::id) && !target_entity)
			{
				Hash weapon_hash = WEAPON::GET_SELECTED_PED_WEAPON(self::ped);
				
				if (!g.weapons.aimbot.nonhitscan)
				{
					// Don't aimbot with throwables
					if (weapon_hash == 0x93E220BD || // Grenade
						weapon_hash == 0xA0973D5E || // BZ Gas
						weapon_hash == 0x24B17070 || // Molotov
						weapon_hash == 0xAB564B93 || // Prox Mines
						weapon_hash == 0xBA45E8B8 || // Pipe Bomb
						weapon_hash == 0x2C3731D9 || // Sticky Bomb
						weapon_hash == 0x497FACC3 || // Flare
						weapon_hash == 0xFDBC8A50)   // Tear Gas
					{
						return;
					}

					if (weapon_hash == 0xB1CA77B1 || // RPG
						weapon_hash == 0xA284510B || // Grenade Launcher
						weapon_hash == 0x4DD2DC56 || // Smoke Grenade Launcher
						weapon_hash == 0x7F7497E5 || // Firework Launcher
						weapon_hash == 0x63AB0442 || // Homing Launcher
						weapon_hash == 0x0781FE4A || // Compact Launcher
						weapon_hash == 0xDB26713A)   // EMP Launcher
					{
						return;
					}
				}

				// Stage 1: Target Acquisition
				rage::fvector2 resolution = {(float)*g_pointers->m_gta.m_resolution_x, (float)*g_pointers->m_gta.m_resolution_y};

				for (auto ped : entity::get_entities(false, true))
				{
					// Don't trying acquiring a target if we're already locked onto one
					if (target_entity)
						continue;

					// First, filter out all dead peds
					if (ENTITY::IS_ENTITY_DEAD(ped, 0))
						continue;

					Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(self::ped, 0);
					Vehicle pedVehicle    = PED::GET_VEHICLE_PED_IS_IN(ped, 0);

					// Ignore peds that are in the vehicle with the player
					if (playerVehicle && (playerVehicle == pedVehicle))
						continue;

					// Next, filter out peds outside of the scan area
					Vector3 pedWorldPosition    = ENTITY::GET_ENTITY_COORDS(ped, false);
					Vector3 playerWorldPosition = ENTITY::GET_ENTITY_COORDS(self::ped, false);

					rage::fvector3 ped_world_position_fvec = {pedWorldPosition.x, pedWorldPosition.y, pedWorldPosition.z};
					float player_to_cam_distance = math::calculate_distance_from_game_cam(ped_world_position_fvec);

					// Don't flip out if an enemy is on top of us
					if (player_to_cam_distance < 2.0f || player_to_cam_distance > 1000.0f)
						continue;

					rage::fvector2 screen = {0.f, 0.f};
					//GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(pedWorldPosition.x, pedWorldPosition.y, pedWorldPosition.z, &xScreen, &yScreen);
					world_to_screen::w2s({pedWorldPosition.x, pedWorldPosition.y, pedWorldPosition.z}, screen);

					float xDelta = screen.x - (resolution.x * 0.5f); // How far from center (crosshair) is X?
					float yDelta = screen.y - (resolution.y * 0.5f); // How far from center (crosshair) is Y?

					// Note that the values returned into xScreen and yScreen by the W2S function range from [0,0] (top left) to [1,1] (bottom right)
					// Largest supported magnitude will obviously be sqrt(0.5^2 + 0.5^2) = 0.707 which is what we'll use in the GUI for now
					// TODO: Create a more easily understandable mapping between GUI value and actual implementation (maybe pixels? maybe draw a box on-screen that shows the valid area?)
					float crosshairMag = sqrtf(xDelta * xDelta + yDelta * yDelta);

					if (crosshairMag > g.weapons.aimbot.fov)
						continue;

					// Filter out peds that are alive and in the scan area, but are behind some sort of cover (we don't want to aim through walls)
					if (!ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY_ADJUST_FOR_COVER(self::ped, ped, 17))
						continue;

					// Now that we've filtered out most of what we want to ignore, our remaining peds are all alive, within our scan area, and targetable
					// From this list of potentially valid targets, let's pick one!
					int relation = PED::GET_RELATIONSHIP_BETWEEN_PEDS(self::ped, ped); // relation for enemy check
					int type     = PED::GET_PED_TYPE(ped); // for police check, cop types are 6, swat is 27

					// If target is a player and we're aiming at players
					if (PED::IS_PED_A_PLAYER(ped) && g.weapons.aimbot.on_player)
					{
						goto set_target;
					}
					// If target is an enemy and we're aiming at enemies
					else if ((relation == 4 || relation == 5 || PED::IS_PED_IN_COMBAT(ped, self::ped))
					    && g.weapons.aimbot.on_enemy) // relation 4 and 5 are for enemies
					{
						goto set_target;
					}
					// If target is law enforcement and we're aiming at law enforcement
					else if (((type == 6 && !PED::IS_PED_MODEL(ped, rage::joaat("s_m_y_uscg_01"))) || type == 27 || // s_m_y_uscg_01 = us coast guard 1 (technically military)
					             PED::IS_PED_MODEL(ped, rage::joaat("s_m_y_ranger_01")) || PED::IS_PED_MODEL(ped, rage::joaat("s_f_y_ranger_01"))) // ranger models
					    && g.weapons.aimbot.on_police)
					{
						goto set_target;
					}
					// If target is an NPC and we're aiming at all NPCs
					// TODO: Maybe filter out animals (type 28)?
					else if (g.weapons.aimbot.on_npc && !PED::IS_PED_A_PLAYER(ped) && type != 28)
					{
						goto set_target;
					}

					// Nothing found, keep going
					else
					{
						continue;
					}

					set_target:
					{
					    // At this point, we've verified this ped is something we want to aim at
						target_entity = ped;
					}
				}
			}

			// Stage 2: Target Tracking
			if (PLAYER::IS_PLAYER_FREE_AIMING(self::id) && target_entity)
			{
				// We're now actively checking against the target entity each tick, not the entire pedlist
				// So now we need to verify that the target entity is still valid (alive, not behind cover, etc.) and break the lock-on DURING free aim
				if (ENTITY::IS_ENTITY_DEAD(target_entity, 0) || !ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY_ADJUST_FOR_COVER(self::ped, target_entity, 17))
				{
					// Reset the target entity, and don't bother with the camera stuff since next tick we're scanning for a new target
					target_entity = 0;
				}
				else
				{
					// We have a valid ped, now do bone stuff

					// Set the bone to aim at
					// Default bone will be the head
					aimBone = static_cast<uint16_t>(PedBones::SKEL_Head); // Head

					// Some heavy weapons should be aimed at the body instead of the head
					Hash weapon_hash = WEAPON::GET_SELECTED_PED_WEAPON(self::ped);

					if (weapon_hash == 0x42BF8A85 || // Minigun
					    weapon_hash == 0xFEA23564 || // Railgun
					    weapon_hash == 0xB62D1F67)   // Widowmaker
					{
						aimBone = static_cast<uint16_t>(PedBones::SKEL_Spine_Root); // Spine0
					}

					// Check if the target is in a vehicle, since we may want to aim a little differently
					int pedVehicleClass = VEHICLE::GET_VEHICLE_CLASS(PED::GET_VEHICLE_PED_IS_IN(target_entity, 0));
					if (PED::IS_PED_IN_ANY_VEHICLE(target_entity, 0))
					{
						// If the target is on a motorcycle or bike, aim at their neck since a headshot is going to be difficult and most shots will miss
						if (pedVehicleClass == 8 || pedVehicleClass == 13)
							aimBone = static_cast<uint16_t>(PedBones::SKEL_R_Clavicle); // Claivcle
						else
							// In most vehicles, we want even our heavy weapons to aim at the head since body shots will hit the vehicle anyway
							aimBone = static_cast<uint16_t>(PedBones::SKEL_Head); // Head
					}

					Vector3 target_position = ENTITY::GET_ENTITY_BONE_POSTION(target_entity, PED::GET_PED_BONE_INDEX(target_entity, aimBone));
					Vector3 target_velocity = ENTITY::GET_ENTITY_VELOCITY(target_entity);

					Vector3 player_position = ENTITY::GET_ENTITY_COORDS(self::ped, false);
					Vector3 player_velocity = ENTITY::GET_ENTITY_VELOCITY(self::ped);

					rage::fvector3 target_position_fvec = {target_position.x, target_position.y, target_position.z};
					rage::fvector3 target_velocity_fvec = {target_velocity.x, target_velocity.y, target_velocity.z};

					// We use get_camera_position() later to get the player's camera position, so no need for natives
					rage::fvector3 player_velocity_fvec = {player_velocity.x, player_velocity.y, player_velocity.z};

					// Apply a compensating factor for velocity
					float velocity_comp_factor = 0.0125f;

					target_position_fvec       = target_position_fvec + (target_velocity_fvec * velocity_comp_factor);

					target_position_fvec.z += 0.075f;

					uintptr_t cam_gameplay_director = *g_pointers->m_gta.m_cam_gameplay_director;

					// Good info here about the layout of the gameplay camera director
					// https://www.unknowncheats.me/forum/grand-theft-auto-v/144028-grand-theft-auto-reversal-structs-offsets-625.html#post3249805

					uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);

					//uintptr_t cam_follow_ped_camera = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C8);
					//uintptr_t cam_follow_ped_camera2 = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x2'C0);
					//uintptr_t cam_follow_ped_camera3 = *reinterpret_cast<uintptr_t*>(cam_gameplay_director + 0x3'C0);

					// Camera Handling
					// Note we HAVE to normalize this vector

                    // Convert target_position from Vector3 to rage::fvector3
					rage::fvector3 camera_position_fvec = get_camera_position();

					// Compensate for player velocity
					camera_position_fvec = camera_position_fvec + (player_velocity_fvec * velocity_comp_factor);

					// Finally calculate the vector we write into memory
					rage::fvector3 camera_target_fvec   = (target_position_fvec - camera_position_fvec).normalize();

					// Game uses different cameras when on-foot vs. in vehicle, which is why using the gameplay cam is such a PITA, but for the aimbot it's fine to write to both locations
					reset_aim_vectors(cam_follow_ped_camera);
					*reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x40) = camera_target_fvec; // First person & sniper (on foot)
					*reinterpret_cast<rage::fvector3*>(cam_follow_ped_camera + 0x3'D0) = camera_target_fvec; // Third person
				}
			}
		}

		virtual void on_disable() override
		{
			target_entity      = 0;
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

	bool_command
	    g_aimbot_nonhitscan("nonhitscan", "BACKEND_LOOPED_WEAPONS_AIMBOT_NONHITSCAN", "BACKEND_LOOPED_WEAPONS_AIMBOT_NONHITSCAN_DESC", g.weapons.aimbot.nonhitscan);
	bool_command
	    g_aimbot_on_player("aimatplayer", "PLAYER", "BACKEND_LOOPED_WEAPONS_AIM_AT_PLAYER_DESC", g.weapons.aimbot.on_player);
	bool_command
		g_aimbot_on_npc("aimatnpc", "NPC", "BACKEND_LOOPED_WEAPONS_AIM_AT_NPC_DESC", g.weapons.aimbot.on_npc);
	bool_command
	    g_aimbot_on_police("aimatpolice", "POLICE", "BACKEND_LOOPED_WEAPONS_AIM_AT_POLICE_DESC", g.weapons.aimbot.on_police);
	bool_command
		g_aimbot_on_enemy("aimatenemy", "BACKEND_LOOPED_WEAPONS_AIM_AT_ENEMY", "BACKEND_LOOPED_WEAPONS_AIM_AT_ENEMY_DESC", g.weapons.aimbot.on_enemy);
}
