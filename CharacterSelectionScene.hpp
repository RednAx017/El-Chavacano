#pragma once

#include <SFML/Graphics.hpp>

using namespace std;

#include "AssetPaths.hpp"
#include "GameContext.hpp"

class CharacterSelectionScene {
public:
    void run(sf::RenderWindow& window, GameContext& context);

private:
    bool loadCharacterTexture(sf::Texture& texture, const string& path);
};

