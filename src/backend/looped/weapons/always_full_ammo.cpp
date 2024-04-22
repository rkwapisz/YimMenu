#include "backend/looped_command.hpp"
#include "natives.hpp"

namespace big
{
	class always_full_ammo : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			int max_ammo;
			Hash weapon_hash;
			WEAPON::GET_CURRENT_PED_WEAPON(self::ped, &weapon_hash, NULL);
			if (WEAPON::IS_WEAPON_VALID(weapon_hash) && WEAPON::GET_MAX_AMMO(self::ped, weapon_hash, &max_ammo))
				WEAPON::SET_PED_AMMO(self::ped, weapon_hash, max_ammo, 0);

			Vehicle player_vehicle = PED::GET_VEHICLE_PED_IS_IN(self::ped, false);
			if (player_vehicle && !g.weapons.infinite_ammo) // If infinite ammo is enabled, don't overwrite the -1 it's writing with these natives
			{
				std::array<uint32_t, 12> weaponHashes = {0x7CBE304C, 0x44A56189, 0xA247D03E, 0xB4F96934, 0x753A78F1, 0x8BB7C63E, 0x6C88E47D, 0xBCE908DB, 0x504DA665, 0x1EF01D8A, 0x9E5840A2, 0x3C83C410};

				for (auto hash : weaponHashes)
				{
					VEHICLE::SET_VEHICLE_WEAPON_RESTRICTED_AMMO(player_vehicle, hash, 20);
				} // Setting to -1 will disable the used missiles count

				VEHICLE::SET_VEHICLE_BOMB_AMMO(player_vehicle, 20);
				VEHICLE::SET_VEHICLE_COUNTERMEASURE_AMMO(player_vehicle, 20);
			}
		}
	};

	always_full_ammo g_always_full_ammo("alwaysfullammo", "BACKEND_LOOPED_WEAPONS_ALWAYS_FULL_AMMO", "BACKEND_LOOPED_WEAPONS_ALWAYS_FULL_AMMO_DESC", g.weapons.always_full_ammo);
}