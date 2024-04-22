#include "backend/looped_command.hpp"
#include "natives.hpp"
#include "common.hpp"

namespace big
{
	class infinite_ammo : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			WEAPON::SET_PED_INFINITE_AMMO(self::ped, TRUE, NULL);

			Vehicle player_vehicle = PED::GET_VEHICLE_PED_IS_IN(self::ped, false);
			if (player_vehicle)
			{
				std::array<uint32_t, 12> weaponHashes = {0x7CBE304C, 0x44A56189, 0xA247D03E, 0xB4F96934, 0x753A78F1, 0x8BB7C63E, 0x6C88E47D, 0xBCE908DB, 0x504DA665, 0x1EF01D8A, 0x9E5840A2, 0x3C83C410};

				for (auto hash : weaponHashes)
				{
					VEHICLE::SET_VEHICLE_WEAPON_RESTRICTED_AMMO(player_vehicle, hash, -1);
				} // Setting to -1 will disable the used missiles count

				VEHICLE::SET_VEHICLE_BOMB_AMMO(player_vehicle, -1);
				VEHICLE::SET_VEHICLE_COUNTERMEASURE_AMMO(player_vehicle, -1);
			}
		}

		virtual void on_disable() override
		{
			WEAPON::SET_PED_INFINITE_AMMO(self::ped, FALSE, NULL);

			Vehicle player_vehicle = PED::GET_VEHICLE_PED_IS_IN(self::ped, false);
			if (player_vehicle)
			{
				std::array<uint32_t, 12> weaponHashes = {0x7CBE304C, 0x44A56189, 0xA247D03E, 0xB4F96934, 0x753A78F1, 0x8BB7C63E, 0x6C88E47D, 0xBCE908DB, 0x504DA665, 0x1EF01D8A, 0x9E5840A2, 0x3C83C410};

				for (auto hash : weaponHashes)
				{
					VEHICLE::SET_VEHICLE_WEAPON_RESTRICTED_AMMO(player_vehicle, hash, VEHICLE::GET_VEHICLE_WEAPON_RESTRICTED_AMMO(player_vehicle, hash));
				}

				VEHICLE::SET_VEHICLE_BOMB_AMMO(player_vehicle, 20);
				VEHICLE::SET_VEHICLE_COUNTERMEASURE_AMMO(player_vehicle, 20);
			}
		}
	};

	infinite_ammo g_infinite_ammo("infammo", "VIEW_OVERLAY_INFINITE_AMMO", "", g.weapons.infinite_ammo);
}