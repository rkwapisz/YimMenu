#pragma once
#include "pointers.hpp"
#include "gta/matrix.hpp"
#include "graphics/CViewport.hpp"

namespace big
{
	class world_to_screen
	{
	public:
		static bool w2s(const rage::fvector3 entity_position, rage::fvector2& screen);
	};
}