#pragma once

#include "platform/win_platform_layer.h"
#include "game.h"
using namespace Odyssey;

extern Game* CreateGame();

#if IS_WINDOWS_PLATFORM

int main()
{
	PlatformLayer::SetImpl(new WindowsPlatform());
	Game* game = CreateGame();
	Engine engine = Engine(game);
	engine.Run();
	return 0;
}

#endif