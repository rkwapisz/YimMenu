#include "backend/player_command.hpp"
#include "natives.hpp"
#include "pointers.hpp"
#include "util/teleport.hpp"

#include <chrono>
#include <thread>

namespace big
{
	class gift_vehicle : player_command
	{
		using player_command::player_command;

		virtual void execute(player_ptr player, const command_arguments& _args, const std::shared_ptr<command_context> ctx) override
		{
			Ped ped = PLAYER::GET_PLAYER_PED_SCRIPT_INDEX(player->id());

			if (!PED::IS_PED_IN_ANY_VEHICLE(ped, true))
			{
				g_notification_service.push_warning("VEHICLE"_T.data(), "ERROR_PLAYER_IS_NOT_IN_VEHICLE"_T.data());
				return;
			}

			Vehicle vehicle = PED::GET_VEHICLE_PED_IS_USING(ped);

			for (int i = 0; i < 20; i++)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				if (entity::take_control_of(vehicle))
				{
					auto net_hash = NETWORK::NETWORK_HASH_FROM_PLAYER_HANDLE(player->id());

					DECORATOR::DECOR_SET_INT(vehicle, "Previous_Owner", net_hash);
					DECORATOR::DECOR_SET_INT(vehicle, "Veh_Modded_By_Player", net_hash);
					DECORATOR::DECOR_SET_INT(vehicle, "Not_Allow_As_Saved_Veh", 0);
					DECORATOR::DECOR_SET_INT(vehicle, "Player_Vehicle", net_hash);
					DECORATOR::DECOR_SET_INT(vehicle, "MPBitset", 8);

					g_notification_service.push_success("VEHICLE"_T.data(), "GIFT_VEHICLE_SUCCESS"_T.data());

					return;
				}
			}

			g_notification_service.push_warning("VEHICLE"_T.data(), "GIFT_VEHICLE_FAIL"_T.data());
		}
	};

	gift_vehicle g_gift_vehicle("giftvehicle", "BACKEND_GIFT_VEHICLE", "BACKEND_GIFT_VEHICLE_DESC", 0);
}
