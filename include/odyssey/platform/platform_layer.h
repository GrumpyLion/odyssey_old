#pragma once

namespace Odyssey
{
	namespace PlatformLayer
	{
		bool Initialize(const char* title, int x, int y, int width, int height);
		void Shutdown();
		bool PumpMessages();
		double GetTimeSinceStartup();
		void Sleep(long ms);
		int GetCoreCount();
	}
}