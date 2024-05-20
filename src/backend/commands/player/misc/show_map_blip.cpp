#include "backend/looped/looped.hpp"
#include "backend/player_command.hpp"
#include "natives.hpp"
#include "pointers.hpp"
#include "services/pickups/pickup_service.hpp"
#include "util/globals.hpp"
#include "util/player_blips.hpp"

namespace big
{
	static std::vector<std::unique_ptr<blip_object>> active_blips;

	int max_blips = 10;

	class show_map_blip : player_command
	{
		using player_command::player_command;

		virtual void execute(player_ptr player, const command_arguments& _args, const std::shared_ptr<command_context> ctx) override
		{
			if (!player || !player->get_ped())
				return;

			if (active_blips.size() >= max_blips) // That's enough blips
				return;

			Entity player_handle = static_cast<Entity>(g_pointers->m_gta.m_ptr_to_handle(player->get_ped()));

			active_blips.push_back(std::make_unique<blip_object>(player_handle, 161, 0, std::chrono::seconds(120))); // Sprite 161 is animated, so it looks good without any flashing
		}
	};

	show_map_blip g_show_map_blip("showmapblip", "SHOW_MAP_BLIP", "SHOW_MAP_BLIP_DESC", 0);

	void looped::player_blip_maintenance()
	{
		active_blips.erase(std::remove_if(active_blips.begin(),
		                       active_blips.end(),
		                       [](const std::unique_ptr<blip_object>& blip) {
			                       if (blip->is_new_blip_expired() || !blip->is_player_blip_valid())
			                       {
				                       return true;
			                       }

			                       return false;
		                       }),
		    active_blips.end());
	}
}