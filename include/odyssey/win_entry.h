#pragma once

#include "game.h"

extern Game* CreateGame();

#if IS_WINDOWS_PLATFORM

int main(int argc, char* argv[])
{
	Game* game = CreateGame();
	Engine engine = Engine(game);
	engine.Run();
	return 0;
}

#endif