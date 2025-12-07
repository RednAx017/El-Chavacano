#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <stack>
#include <string>

using namespace std;

enum class CharacterChoice {
    Gangster1,
    Gangster3
};

struct GameContext {
    sf::Font font;
    sf::Texture backgroundTexture;
    unique_ptr<sf::Sprite> backgroundSprite;
    bool hasBackground = false;
    string selectedCharacterName = "Gangster 1";
    CharacterChoice selectedCharacter = CharacterChoice::Gangster1;
    stack<string> actionHistory;
};

