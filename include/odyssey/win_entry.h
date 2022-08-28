#pragma once

#include "game.h"
using namespace Odyssey;

extern Game* CreateGame();

#if IS_WINDOWS_PLATFORM

int main()
{
	Game* game = CreateGame();
	Engine engine = Engine(game);
	engine.Run();
	return 0;
}

#endif