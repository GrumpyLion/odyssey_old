#include "odyssey.h"
using namespace Odyssey;

#include "odyssey/platform/platform_layer.h"
#include "odyssey/core/logger.h"
#include "renderer/renderer_frontend.h"

Engine::Engine(Game* game)
	: myGame(game)
{
	myGame->SetEngine(this);

	Logger::Initialize();
	Logger::Log("Odyssey Engine warming up.....");

	PlatformLayer::Initialize(myGame->GetName(), 100, 100, 1024, 600);
	PlatformLayer::GetCoreCount();
	
	RendererFrontend::Initialize(1024, 600);
}

Engine::~Engine()
{
	delete myGame;

	RendererFrontend::Shutdown();
}

void Engine::Run()
{
	while(PlatformLayer::PumpMessages())
	{
		myGame->Update(0.0f);

		RendererFrontend::Render();

		PlatformLayer::Sleep(5);
	}
}