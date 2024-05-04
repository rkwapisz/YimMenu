#include "backend/looped_command.hpp"
#include "natives.hpp"
#include "util/globals.hpp"

namespace big
{
	class refill_ability : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			PLAYER::SPECIAL_ABILITY_FILL_METER(self::id, 1, 0);
		};
	};

	refill_ability g_refill_ability("refillability", "REFILL_ABILITY", "REFILL_ABILITY_DESC", g.self.refill_ability);
}