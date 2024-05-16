#include "core/data/language_codes.hpp"
#include "core/data/region_codes.hpp"
#include "fiber_pool.hpp"
#include "pointers.hpp"
#include "script.hpp"
#include "services/matchmaking/matchmaking_service.hpp"
#include "util/session.hpp"
#include "views/view.hpp"

#include <network/Network.hpp>

namespace big
{
	void view::session_protection()
	{
		// TODO: Block detected modders

		// TODO: Kick all modders
	}
}