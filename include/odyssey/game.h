#pragma once

#include "glm/glm.hpp"

namespace Odyssey
{
	class Engine;

	class Game
	{
	public:
		virtual ~Game() = default;

		void SetEngine(Engine* engine)
		{
			myEngine = engine;
		}

		virtual void Update(float delta) = 0;
		virtual void Render(float delta) = 0;
		virtual const char* GetName() = 0;

	protected:
		Engine* myEngine{};
		glm::ivec2 myWindowPosition{};
	};
}