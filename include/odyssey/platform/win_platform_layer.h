#pragma once

#include "odyssey/platform/platform_layer.h"

struct GLFWwindow;

namespace Odyssey
{
	class WindowsPlatform final : public PlatformLayerImpl
	{
	public:
		~WindowsPlatform() override = default;
		bool Initialize(const char* title, int x, int y, int width, int height) override;
		void Shutdown() override;
		bool PumpMessages() override;
		double GetTimeSinceStartup() override;
		void Sleep(long ms) override;
		int GetCoreCount() override;

	private:
		GLFWwindow* myWindow{};
	};
}