#pragma once
#include "util/pools.hpp"

namespace big::train
{
	inline CVehicle* get_nearest_train()
	{
		CVehicle* nearest_train = nullptr;
		float nearest_distance  = 2000.0f; // Not too big

		auto player_pos = *g_local_player->get_position();

		for (auto vehicle : pools::get_all_vehicles())
		{
			if (vehicle->m_model_info && vehicle->m_model_info->m_hash == "freight"_J)
			{
				auto train_pos = *vehicle->get_position();
				auto distance = std::sqrt(std::pow(train_pos.x - player_pos.x, 2) + std::pow(train_pos.y - player_pos.y, 2)
				    + std::pow(train_pos.z - player_pos.z, 2));

				if (distance < nearest_distance)
				{
					nearest_train    = static_cast<CVehicle*>(vehicle);
					nearest_distance = distance;
				}
			}
		}

		return nearest_train;
	}

	inline bool is_in_train()
	{
		auto vehicle = g_local_player->m_vehicle;

		return vehicle && (vehicle->m_driver == g_local_player) && vehicle->m_model_info && vehicle->m_model_info->m_hash == "freight"_J;
	}
}
