#pragma once

#include "game.h"
using namespace Odyssey;

extern Game* CreateGame();

#if IS_WINDOWS_PLATFORM

int main()
{
	Game* game = CreateGame();

	return 0;
}

#endif