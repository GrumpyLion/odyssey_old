#pragma once

namespace Odyssey
{
	class Game
	{
	public:
		virtual ~Game() = default;
		virtual void Update(float delta) = 0;
		virtual void Render(float delta) = 0;
		virtual const char* getName() = 0;
	};
}