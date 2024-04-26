#include "util/globals.hpp"
#include "backend/looped_command.hpp"
#include "gta/pickup_rewards.hpp"
#include "natives.hpp"

namespace big
{
	class auto_heal : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			if (g_local_player->m_health < (0.5f * g_local_player->m_maxhealth))
				ENTITY::SET_ENTITY_HEALTH(self::ped, g_local_player->m_maxhealth, self::ped, 0);

			if (g_local_player->m_armor <= 0.0f)
				PED::SET_PED_ARMOUR(self::ped, PLAYER::GET_PLAYER_MAX_ARMOUR(self::id));
		};
	};

	auto_heal g_auto_heal("autoheal", "AUTO_HEAL", "AUTO_HEAL_DESC", g.self.auto_heal);
}