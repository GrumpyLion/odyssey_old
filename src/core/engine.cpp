#include "odyssey.h"

namespace Odyssey
{
	Engine::Engine(Game* game)
		: myGame(game)
	{
		myGame->SetEngine(this);
	}

	Engine::~Engine()
	{
		delete myGame;
	}

	void Engine::Run()
	{

	}
}
