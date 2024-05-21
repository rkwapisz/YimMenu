#include "backend/looped/looped.hpp"
#include "services/players/player_service.hpp"
#include "gta_pointers.hpp"
#include "util/globals.hpp"
#include "util/misc.hpp"

namespace big
{
	void looped::player_highlight_blip()
	{
		if (!*g_pointers->m_gta.m_is_session_started)
			return;

		g_player_service->iterate([](const player_entry& entry) {
			// First time around for a player, give them a blip if they don't have one
			if (!entry.second->highlight_blip)
			{
				if (!entry.second->get_ped())
					return;

				// Only give blips to those who don't have one
				if (entry.second->highlight_blip)
					return;

				Entity plyr_entity = g_pointers->m_gta.m_ptr_to_handle(entry.second->get_ped());

				entry.second->highlight_blip = HUD::ADD_BLIP_FOR_ENTITY(plyr_entity);

				// More blip info https://docs.fivem.net/docs/game-references/blips/
				// We can do all sorts of neat colorful stuff with these blips
				HUD::SET_BLIP_SPRITE(entry.second->highlight_blip, 161); // radar_mp_noise
				HUD::SET_BLIP_SCALE(entry.second->highlight_blip, 0.5f);

				/*
				HUD::SET_BLIP_FLASHES(entry.second->highlight_blip, true);                      // Flash it
				HUD::SET_BLIP_FLASH_TIMER(entry.second->highlight_blip, flash_duration * 1000); // Flash it long (convert seconds to ms)
				HUD::SET_BLIP_FLASH_INTERVAL(entry.second->highlight_blip, 100); // Flash it fast
				*/

				HUD::SET_BLIP_DISPLAY(entry.second->highlight_blip, 8); // Show it on the minimap and the main map, but not selectable

				HUD::SET_BLIP_ALPHA(entry.second->highlight_blip, 0); // Make it invisible to start
			}

			// If the player is supposed to be showing their blip, turn it on
			if (entry.second->show_highlight_blip && entry.second->highlight_blip)
			{
				auto entity_coords = entry.second->get_ped()->get_position();

				// Don't draw blips for entities way under the game world (e.g., interiors)
				if (entity_coords && entity_coords->z < -40.0f)
					HUD::SET_BLIP_ALPHA(entry.second->highlight_blip, 0); // Make it visible
				else
					HUD::SET_BLIP_ALPHA(entry.second->highlight_blip, 255); // Make it visible
			}

			// If the player already has a blip but isn't supposed to be showing it, turn it off
			if (!entry.second->show_highlight_blip && entry.second->highlight_blip)
			{
				// Note: I'm trying to stay away from REMOVE_BLIP because it's fairly unstable. I'm having much better luck just initializing every player with a blip and then controlling its visibility.
				HUD::SET_BLIP_ALPHA(entry.second->highlight_blip, 0); // Make it invisible
			}
		});
	}
}
