#include "backend/looped/looped.hpp"
#include "gta_pointers.hpp"
#include "services/players/player_service.hpp"
#include "util/globals.hpp"
#include "util/misc.hpp"

namespace big
{
	// Use a static vector to store all of our custom player blips, since it lets us keep track better after a player leaves the session
	std::vector<Blip> blips;

	void clear_all_player_blips()
	{
		// If a player leaves, their object can be destroyed before we have a chance to remove the blip which will leave "floating" blips, so it's better to store blips outside of the player object
		for (auto blip : blips)
		{
			if (HUD::DOES_BLIP_EXIST(blip))
				HUD::REMOVE_BLIP(&blip);

		}

		// Clear the entire vector
		blips.clear();

		// Reset each player's blip
		g_player_service->iterate([](const player_entry& entry) {
			entry.second->highlight_blip = 0;
		});
	}

	void looped::player_highlight_blip()
	{
		if (!*g_pointers->m_gta.m_is_session_started)
			return;

		static int previous_player_count = 0;
		int current_player_count  = g_player_service->players().size(); // Don't include us in the count

		// Blips are handled kinda stupidly by the game... blips added via ADD_BLIP_FOR_ENTITY don't disappear when the entity disappears
		// Clear everyone's blips when the session player count changes so we don't have to individually clean up blips
		if (current_player_count != previous_player_count)
		{
			clear_all_player_blips();

			previous_player_count = current_player_count;
		}

		g_player_service->iterate([](const player_entry& entry) {

			if (!entry.second || !entry.second->is_valid() || !entry.second->get_ped())
				return;

			Entity plyr_entity = g_pointers->m_gta.m_ptr_to_handle(entry.second->get_ped());

			// First time around for a player (or after a player count change), give them a blip if they don't have one
			if (entry.second->show_highlight_blip && !entry.second->highlight_blip)
			{
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
				HUD::SET_BLIP_ALPHA(entry.second->highlight_blip, 255);

				// Add the blip to the vector
				blips.push_back(entry.second->highlight_blip);
			}

			// If the player already has a blip but isn't supposed to be showing it, turn it off
			if ((!entry.second->show_highlight_blip && entry.second->highlight_blip)
				|| INTERIOR::IS_VALID_INTERIOR(INTERIOR::GET_INTERIOR_FROM_ENTITY(plyr_entity)) // Turn off blips for players inside interiors
				|| entry.second->get_ped()->m_health <= 0.0f) // Turn off blips for dead players
			{
				if (HUD::DOES_BLIP_EXIST(entry.second->highlight_blip))
					HUD::REMOVE_BLIP(&entry.second->highlight_blip);

				entry.second->highlight_blip = 0;

				// Remove blip from the vector
				auto it = std::find(blips.begin(), blips.end(), entry.second->highlight_blip);
				if (it != blips.end())
					blips.erase(it);
			}
		});
	}
}
