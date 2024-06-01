#include "backend/looped/looped.hpp"
#include "backend/player_command.hpp"
#include "natives.hpp"
#include "pointers.hpp"

namespace big
{
	void looped::session_auto_kick_player()
	{
		if (!*g_pointers->m_gta.m_is_session_started)
			return;

		g_player_service->iterate([](auto& plyr) {
			// Don't auto kick host or trusted players
			if (plyr.second->auto_kick)
			{
				auto p_name = plyr.second->get_name();

				LOG(INFO) << "Auto kick on for kicking player: " << p_name;

				if (plyr.second->is_host() || plyr.second->is_trusted || (g.session.trust_friends && plyr.second->is_friend()))
					return;

				LOG(INFO) << "Auto kicking player: " << p_name;

				if (g_player_service->get_self()->is_host())
					// Breakup or host kick work, but breakup kick is less detectable and since it's automatic we don't need to worry about extra work if/when the player rejoins
					dynamic_cast<player_command*>(command::get("breakup"_J))->call(plyr.second, {});
				else
					dynamic_cast<player_command*>(command::get("desync"_J))->call(plyr.second, {});

				g_notification_service.push_warning("TOXIC"_T.data(), std::vformat(g_translation_service.get_translation("AUTO_KICK_PLAYER_NOTIFY"), std::make_format_args(p_name)));

				// Disable auto kick for this player for this session so we don't spam kick
				plyr.second->auto_kick = false;
			}
		});
	}
}
