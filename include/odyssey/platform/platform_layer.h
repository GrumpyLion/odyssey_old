#pragma once

namespace Odyssey
{
	class PlatformLayerImpl
	{
	public:
		virtual ~PlatformLayerImpl() = default;
		virtual bool Initialize(const char* title, int x, int y, int width, int height) = 0;
		virtual void Shutdown() = 0;
		virtual bool PumpMessages() = 0;
		virtual double GetTimeSinceStartup() = 0;
		virtual void Sleep(long ms) = 0;
		virtual int GetCoreCount() = 0;
	};

	class PlatformLayer
	{
	public:
		static void SetImpl(PlatformLayerImpl* impl);
		static bool Initialize(const char* title, int x, int y, int width, int height);
		static void Shutdown();
		static bool PumpMessages();
		static double GetTimeSinceStartup();
		static void Sleep(long ms);
		static int GetCoreCount();
	private:
		static PlatformLayerImpl* myImpl;
	};
}
