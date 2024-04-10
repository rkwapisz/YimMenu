#pragma once
#include "rage/vector.hpp"
#include <cstdint>

namespace rage
{
	class matrix4x4
	{
	public:
		union {
			struct
			{
				fvector4 _1;
				fvector4 _2;
				fvector4 _3;
				fvector4 _4;
			};

			float raw[4 * 4] = {};
		};
	};

	struct grcViewport
	{
		fmatrix44 m_world;
		fmatrix44 m_worldView;
		fmatrix44 m_worldViewProj;
		fmatrix44 m_inverseView;
		fmatrix44 m_view;
		fmatrix44 m_projection;
	};

	struct CViewportGame
	{
	public:
		virtual ~CViewportGame() = 0;

	private:
		char m_pad[8];

	public:
		grcViewport viewport;
	};
}
