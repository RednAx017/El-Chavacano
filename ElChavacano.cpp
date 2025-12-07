#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>

#include "CharacterSelectionScene.hpp"
#include "GameContext.hpp"
#include "GameStage.hpp"
#include "IntroductionScene.hpp"

using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode({960u, 540u}), "El Chavacano", sf::Style::Resize | sf::Style::Close);
    window.setFramerateLimit(60);

    GameContext context;
    const string fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    if (!context.font.openFromFile(fontPath)) {
        cerr << "Unable to load font from: " << fontPath << '\n';
        return 1;
    }

    const string backgroundPath = "Background.png";
    if (context.backgroundTexture.loadFromFile(backgroundPath)) {
        context.backgroundSprite = make_unique<sf::Sprite>(context.backgroundTexture);
        const auto bounds = context.backgroundSprite->getLocalBounds();
        const float scaleX = static_cast<float>(window.getSize().x) / bounds.size.x;
        const float scaleY = static_cast<float>(window.getSize().y) / bounds.size.y;
        context.backgroundSprite->setScale(sf::Vector2f{scaleX, scaleY});
        context.hasBackground = true;
    } else {
        cerr << "Warning: Could not load background image at " << backgroundPath << '\n';
    }

    IntroductionScene intro;
    intro.run(window, context);
    if (!window.isOpen()) {
        return 0;
    }

    // Main game loop - allows replaying
    while (window.isOpen()) {
        CharacterSelectionScene selection;
        selection.run(window, context);
        if (!window.isOpen()) {
            return 0;
        }

        GameStage stage;
        stage.run(window, context);
        
        // If window is still open after game ends, loop back to character selection
        // (GameStage will handle PlayAgain screen and exit if ESC is pressed)
        if (!window.isOpen()) {
            return 0;
        }
    }

    return 0;
}
