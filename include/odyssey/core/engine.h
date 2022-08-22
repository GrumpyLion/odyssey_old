namespace Odyssey
{
	class Game;
	class PlatformLayer;

	class Engine
	{
	public:
		Engine(Game* game);
		~Engine();
		void Run();

	private:
		Game* myGame{};
	};
}