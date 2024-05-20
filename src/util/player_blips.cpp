#include "util/player_blips.hpp"

#include "backend/looped/looped.hpp"
#include "common.hpp"
#include "natives.hpp"

namespace big
{
	void blip_object::start_blip_timer(std::chrono::seconds blip_duration)
	{
		blip_timer = std::async(std::launch::async, [blip_duration, this]() {
			std::this_thread::sleep_for(blip_duration);

			new_blip_expired = true;
		});
	}

	blip_object::blip_object(Entity entity, int blip_sprite, int flash_duration, std::chrono::seconds blip_duration)
	{
		if (blip_duration.count() == 0) // Someone did an oopsie
			return;

		player_entity = entity;
		current_blip  = HUD::GET_BLIP_FROM_ENTITY(entity);

		// Players inside interiors won't appear correctly (just like with ESP), easy to just check for low Z-values
		Vector3 entity_coords = ENTITY::GET_ENTITY_COORDS(entity, true);

		if (entity_coords.z < -40.0f)
			g_notification_service.push_warning("WARNING"_T.data(), "BLIP_INTERIOR_WARNING"_T.data());

		new_blip = HUD::ADD_BLIP_FOR_ENTITY(entity);

		// More blip info https://docs.fivem.net/docs/game-references/blips/
		// We can do all sorts of neat colorful stuff with these blips
		HUD::SET_BLIP_SPRITE(new_blip, blip_sprite); // radar_mp_noise
		HUD::SET_BLIP_SCALE(new_blip, 0.5f);

		if (flash_duration)
		{
			HUD::SET_BLIP_FLASHES(new_blip, true);                      // Flash it
			HUD::SET_BLIP_FLASH_TIMER(new_blip, flash_duration * 1000); // Flash it long (convert seconds to ms)
			HUD::SET_BLIP_FLASH_INTERVAL(new_blip, 100);                // Flash it fast
		}

		HUD::SET_BLIP_DISPLAY(new_blip, 8); // Show it on the minimap and the main map, but not selectable

		start_blip_timer(blip_duration);
	}

	bool blip_object::is_new_blip_expired()
	{
		return new_blip_expired;
	}

	bool blip_object::is_player_blip_valid()
	{
		// Check both if the blip AND entity are valid because we don't want blips to remain if the player leaves
		return (HUD::DOES_BLIP_EXIST(current_blip) || ENTITY::DOES_ENTITY_EXIST(player_entity));
	}

	void blip_object::delete_blip()
	{
		//MISC::SET_THIS_SCRIPT_CAN_REMOVE_BLIPS_CREATED_BY_ANY_SCRIPT(true);
		//HUD::REMOVE_BLIP(&blip); // This is how we're supposed to call REMOVE_BLIP but this causes a crash
		HUD::SET_BLIP_ALPHA(new_blip, 0); // Workaround: Hide the blip instead of trying to delete it
	}

	blip_object::~blip_object()
	{
		delete_blip();
	}
}