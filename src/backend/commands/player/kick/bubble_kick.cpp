#include "backend/player_command.hpp"
#include "natives.hpp"
#include "packet.hpp"
#include "pointers.hpp"

namespace big
{
	class bubble_kick : player_command
	{
		using player_command::player_command;

		virtual CommandAccessLevel get_access_level() override
		{
			return CommandAccessLevel::AGGRESSIVE;
		}

		virtual void execute(player_ptr player, const command_arguments& _args, const std::shared_ptr<command_context> ctx) override
		{
			player->trigger_bubble_kick = true;
		}
	};

	bubble_kick g_bubble_kick("bubblekick", "BUBBLE_KICK", "BUBBLE_KICK_DESC", 0);
}