#pragma once

#include <SFML/Graphics.hpp>
#include <queue>

using namespace std;

#include "GameContext.hpp"

class GameStage {
public:
    void run(sf::RenderWindow& window, GameContext& context);
};

