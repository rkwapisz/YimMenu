#include "backend/looped_command.hpp"
#include "natives.hpp"
#include "util/vehicle.hpp"

namespace big
{
	class keep_vehicle_repaired : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			const auto veh = self::veh;
			vehicle::repair(veh);
		}
	};

	keep_vehicle_repaired
	    g_keep_vehicle_repaired("keepfixed", "KEEP_VEHICLE_FIXED", "KEEP_VEHICLE_FIXED_DESC", g.vehicle.keep_vehicle_repaired);
}
