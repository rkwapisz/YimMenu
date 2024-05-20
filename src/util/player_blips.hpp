#pragma once

#include <chrono>
#include <future>
#include <vector>


namespace big
{
	class blip_object
	{
	public:
		blip_object(Entity entity, int blip_sprite = 0, int flash_duration = 30, std::chrono::seconds blip_duration = std::chrono::seconds(30));

		bool is_new_blip_expired();
		bool is_player_blip_valid();

		void delete_blip();

		~blip_object();

	private:
		Entity player_entity;
		Blip new_blip;
		Blip current_blip;
		std::chrono::seconds blip_duration;
		std::future<void> blip_timer;

		bool new_blip_expired = false;

		void start_blip_timer(std::chrono::seconds blip_duration);
	};
}