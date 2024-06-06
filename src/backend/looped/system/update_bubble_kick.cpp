#include "backend/looped/looped.hpp"
#include "natives.hpp"
#include "packet.hpp"
#include "pointers.hpp"

namespace big
{
	void looped::system_update_bubble_kick()
	{
		for (auto& player : g_player_service->players())
		{
			if (player.second && player.second->is_valid() && player.second->trigger_bubble_kick)
			{
				std::srand(static_cast<unsigned int>(std::time(nullptr)));
				int bubble_id = std::rand() % 11;

				packet pack;
				pack.write_message(rage::eNetMessage::MsgRoamingJoinBubbleAck);
				pack.write(0, 2);         // ACKCODE_SUCCESS
				pack.write(bubble_id, 4); // Bubble ID (10 is MAX_BUBBLES and also INVALID_BUBBLE_ID)
				pack.write(0, 6);
				pack.send(player.second->get_net_game_player()->m_msg_id);
			}
		}
	}
}