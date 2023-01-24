#pragma once

#include "odyssey/types.h"

namespace PlatformLayer
{
	bool Initialize(const char* title, int x, int y, int width, int height);
	void Shutdown();
	bool PumpMessages();
	double GetTimeSinceStartup();
	void Sleep(long ms);
	int GetCoreCount();
	void SetArgs(int argc, char* argv[]);
	Vector<std::string> GetArgs();
	std::string GetBinPath();
}
