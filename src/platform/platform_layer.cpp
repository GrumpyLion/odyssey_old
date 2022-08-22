#include "odyssey/platform/platform_layer.h"

using namespace Odyssey;

PlatformLayerImpl* PlatformLayer::myImpl = nullptr;

void PlatformLayer::SetImpl(PlatformLayerImpl* impl)
{
	myImpl = impl;
}

bool PlatformLayer::Initialize(const char* title, int x, int y, int width, int height) // TODO add assert
{
	return myImpl->Initialize(title, x, y, width, height);
}

void PlatformLayer::Shutdown()
{
	myImpl->Shutdown();
	delete myImpl;
}

bool PlatformLayer::PumpMessages()
{
	return myImpl->PumpMessages();
}

double PlatformLayer::GetTimeSinceStartup()
{
	return myImpl->GetTimeSinceStartup();
}

void PlatformLayer::Sleep(long ms)
{
	myImpl->Sleep(ms);
}

int PlatformLayer::GetCoreCount()
{
	return myImpl->GetCoreCount();
}
