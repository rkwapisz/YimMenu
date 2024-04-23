#pragma once

#include "backend/player_command.hpp"
#include "natives.hpp"
#include "common.hpp"
#include <future>
#include <chrono>
#include <vector>

namespace big
{
	Object CREATE_OBJECT_WITH_ROTATION(DWORD model, float posX, float posY, float posZ, float rotX, float rotY, float rotZ, float rotW, bool dynamic, bool visible);
	Object CREATE_OBJECT_WITH_HEADING(DWORD model, float posX, float posY, float posZ, float heading, bool dynamic, bool visible);

	class cageObject
	{
	public:
		cageObject(DWORD model, float xLoc, float yLoc, float zLoc, float pedHeading, std::chrono::seconds cageDuration);
		bool cageExpired();
		void deleteCage();
		~cageObject();

	private:
		Object cage;
		std::chrono::seconds cageDuration;
		std::future<void> cageTimer;

		bool isExpired = false;
		bool isVisible = false;

		void startTimer(std::chrono::seconds cageDuration);
		Object createTimedCage(DWORD model, float xLoc, float yLoc, float zLoc, float pedHeading, std::chrono::seconds cageDuration);
	};

	extern std::vector<cageObject*> spawnedCages;
}	