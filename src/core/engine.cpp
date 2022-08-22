#include "odyssey.h"

#include "odyssey/platform/platform_layer.h"
#include "odyssey/core/logger.h"

namespace Odyssey
{
	Engine::Engine(Game* game)
		: myGame(game)
	{
		myGame->SetEngine(this);

		Logger::Initialize();
		Logger::Log("Odyssey Engine warming up.....");

		PlatformLayer::Initialize(myGame->GetName(), 100, 100, 1024, 600); // TODO
		PlatformLayer::GetCoreCount();
	}

	Engine::~Engine()
	{
		delete myGame;
	}

	void Engine::Run()
	{
		while(PlatformLayer::PumpMessages())
		{
			myGame->Update(0.0f);

			PlatformLayer::Sleep(5);
		}
	}
}
