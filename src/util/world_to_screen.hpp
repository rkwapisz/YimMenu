#pragma once
#include "pointers.hpp"
#include "gta/matrix.hpp"
#include "graphics/CViewport.hpp"

namespace big::world_to_screen
{
	// w2w will return real pixel coordinates into screen
	bool w2s(const rage::fvector3 entity_position, rage::fvector2 &screen) {
	    // Get the viewport matrix
	   
		CViewportGame** g_viewportGame = g_pointers->m_gta.m_viewport;

		if (g_viewportGame == nullptr)
			return false;

		const auto viewport    = (*g_viewportGame)->viewport;
		const auto view_matrix = viewport.m_worldViewProj;

		rage::fmatrix44 transposed_view_matrix;

		// Transpose the view_matrix so we can store right, up, and forward in their own vectors
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				transposed_view_matrix.data[i][j] = view_matrix.data[j][i];
			}
		}

		// Apply the transposed matrix to the entity position to get our entity to screen matrix
		auto vRight   = transposed_view_matrix.rows[0]; // Right
		auto vUp      = transposed_view_matrix.rows[1]; // Up
		auto vForward = transposed_view_matrix.rows[2]; // Forward

		Vector3 tVec = {0.f, 0.f, 0.f};

		tVec.x = (vRight.x * entity_position.x)		+ (vRight.y * entity_position.y)	+ (vRight.z * entity_position.z)	+ vRight.w;
		tVec.y = (vUp.x * entity_position.x)		+ (vUp.y * entity_position.y)		+ (vUp.z * entity_position.z)		+ vUp.w;
		tVec.z = (vForward.x * entity_position.x)	+ (vForward.y * entity_position.y)	+ (vForward.z * entity_position.z)	+ vForward.w;

		if (tVec.z < 0.001f)
			return false;

		tVec.z = 1.0f / tVec.z;
		tVec.x *= tVec.z;
		tVec.y *= tVec.z;

		// Get the resolution of the game window
		rage::fvector2 resolution = {static_cast<float>(*g_pointers->m_gta.m_resolution_x), static_cast<float>(*g_pointers->m_gta.m_resolution_y)};

		// Calculate the screen coordinates
		screen.x = ((resolution.x * 0.5f) + (0.5f * tVec.x * resolution.x + 1.0f));
		screen.y = ((resolution.y * 0.5f) - (0.5f * tVec.y * resolution.y + 1.0f));

		// Check if the screen coordinates are outside the screen boundaries
		if (screen.x > resolution.x || screen.x < 0.0f || screen.y > resolution.y || screen.y < 0.0f)
			return false;

		return true;
	}
}