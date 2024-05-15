#include "util/train.hpp"

#include "backend/command.hpp"
#include "backend/looped/looped.hpp"
#include "gta/enums.hpp"
#include "natives.hpp"
#include "util/blip.hpp"
#include "util/entity.hpp"

#include <cmath>

namespace big
{
	CVehicle* previous_train;

	void looped::drive_train()
	{
		if (!g.vehicle.train.enabled)
			return;

		if (!train::is_in_train())
		{
			return;
		}
			
		auto vehicle = g_local_player->m_vehicle;

		if (!vehicle || vehicle != previous_train)
		{
			// We've changed trains, so reset the cruise speed
			g.vehicle.train.target_speed = 0.f;
			previous_train               = vehicle;
		}

		if (vehicle && vehicle->m_model_info && vehicle->m_model_info->m_hash == "freight"_J)
		{
			if (PAD::IS_CONTROL_PRESSED(0, 71))
			{
				g.vehicle.train.target_speed = std::min(g.vehicle.train.target_speed + 0.25f, 80.0f);
			}

			if (PAD::IS_CONTROL_PRESSED(0, 72))
			{
				g.vehicle.train.target_speed = std::max(g.vehicle.train.target_speed - 0.25f, -80.0f);
			}

			VEHICLE::SET_TRAIN_CRUISE_SPEED(self::veh, g.vehicle.train.target_speed);

			Vector3 speed_vector = ENTITY::GET_ENTITY_SPEED_VECTOR(self::veh, true);
			float speed          = std::sqrt(
                speed_vector.x * speed_vector.x + speed_vector.y * speed_vector.y + speed_vector.z * speed_vector.z);

			if (speed_vector.y < 0.0f)
				speed = -speed;

			g.vehicle.train.current_speed = speed;
		}
	}

	class hijack_nearest_train : command
	{
		using command::command;

		virtual void execute(const command_arguments&, const std::shared_ptr<command_context> ctx) override
		{
			if (auto vehicle = train::get_nearest_train())
			{
				Vehicle veh_handle = static_cast<Vehicle>(g_pointers->m_gta.m_ptr_to_handle(vehicle));

				if (auto ped = VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh_handle, -1, true))
				{
					TASK::CLEAR_PED_TASKS_IMMEDIATELY(ped);
				}

				PED::SET_PED_INTO_VEHICLE(self::ped, veh_handle, -1);
				g_notification_service.push_success("HIJACK_NEAREST_TRAIN"_T.data(), "HIJACK_TRAIN_SUCCESS"_T.data());
			}
			else
			{
				g_notification_service.push_warning("HIJACK_NEAREST_TRAIN"_T.data(), "HIJACK_NEAREST_TRAIN_NOT_FOUND"_T.data());
			}
		}
	};

	hijack_nearest_train g_hijack_nearest_train("hijacknearesttrain", "HIJACK_NEAREST_TRAIN", "HIJACK_NEAREST_TRAIN_DESC", 0);

	class exit_train : command
	{
		using command::command;

		virtual void execute(const command_arguments&, const std::shared_ptr<command_context> ctx) override
		{
			auto vehicle = g_local_player->m_vehicle;

			if (vehicle && vehicle->m_model_info && vehicle->m_model_info->m_hash == "freight"_J)
			{
				TASK::CLEAR_PED_TASKS_IMMEDIATELY(self::ped);
			}
		}
	};

	exit_train g_exit_train("exittrain", "EXIT_TRAIN", "EXIT_TRAIN_DESC", 0);
}