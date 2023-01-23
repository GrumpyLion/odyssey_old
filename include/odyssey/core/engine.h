#pragma once

class Game;

class Engine
{
public:
	Engine(Game* game);
	~Engine();
	void Run();

private:
	Game* myGame{};
};
