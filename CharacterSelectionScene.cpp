#include "CharacterSelectionScene.hpp"

#include <algorithm>
#include <array>
#include <iostream>

using namespace std;

namespace {
bool applyFirstFrame(sf::Sprite& sprite, const sf::Texture& texture) {
    const auto size = texture.getSize();
    if (size.y == 0 || size.x == 0) {
        return false;
    }
    const int frameHeight = static_cast<int>(size.y);
    int frameWidth = static_cast<int>(size.x);
    if (frameHeight > 0) {
        const int frames = max(1u, size.x / size.y);
        frameWidth = static_cast<int>(size.x / frames);
    }
    sprite.setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{frameWidth, frameHeight}));
    return true;
}
}

bool CharacterSelectionScene::loadCharacterTexture(sf::Texture& texture, const string& path) {
    if (!texture.loadFromFile(path)) {
        cerr << "Failed to load texture: " << path << '\n';
        return false;
    }
    texture.setSmooth(true);
    return true;
}

void CharacterSelectionScene::run(sf::RenderWindow& window, GameContext& context) {
    sf::Texture gangster1Texture;
    sf::Texture gangster3Texture;

    if (!loadCharacterTexture(gangster1Texture, kGangster1Idle) ||
        !loadCharacterTexture(gangster3Texture, kGangster3Idle)) {
        return;
    }

    sf::Sprite gangster1Sprite(gangster1Texture);
    sf::Sprite gangster3Sprite(gangster3Texture);
    applyFirstFrame(gangster1Sprite, gangster1Texture);
    applyFirstFrame(gangster3Sprite, gangster3Texture);

    gangster1Sprite.setScale(sf::Vector2f{2.3f, 2.3f});
    gangster3Sprite.setScale(sf::Vector2f{2.3f, 2.3f});
    gangster1Sprite.setPosition(sf::Vector2f{120.f, 150.f});
    gangster3Sprite.setPosition(sf::Vector2f{520.f, 150.f});

    // Load CharacterSelect.png background
    sf::Texture characterSelectTexture;
    unique_ptr<sf::Sprite> characterSelectSprite;
    bool hasCharacterSelect = false;
    if (characterSelectTexture.loadFromFile("CharacterSelect.png")) {
        characterSelectSprite = make_unique<sf::Sprite>(characterSelectTexture);
        const auto windowSize = window.getSize();
        const auto textureSize = characterSelectTexture.getSize();
        const float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x);
        const float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
        characterSelectSprite->setScale(sf::Vector2f{scaleX, scaleY});
        hasCharacterSelect = true;
    }

    sf::Text selectionLabel(context.font, "");
    selectionLabel.setCharacterSize(28);
    selectionLabel.setFillColor(sf::Color(220, 220, 220));
    selectionLabel.setPosition(sf::Vector2f{80.f, 460.f});

    bool selectionMade = false;

    while (window.isOpen() && !selectionMade) {
        while (auto eventOpt = window.pollEvent()) {
            const auto& event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            if (const auto keyEvent = event.getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Num1) {
                    context.selectedCharacterName = "Gangster 1";
                    context.selectedCharacter = CharacterChoice::Gangster1;
                    context.actionHistory.push("Chose Gangster 1");
                    selectionLabel.setString("Selected: Gangster 1");
                    selectionMade = true;
                } else if (keyEvent->code == sf::Keyboard::Key::Num3) {
                    context.selectedCharacterName = "Gangster 3";
                    context.selectedCharacter = CharacterChoice::Gangster3;
                    context.actionHistory.push("Chose Gangster 3");
                    selectionLabel.setString("Selected: Gangster 3");
                    selectionMade = true;
                }
            }
        }

        // Draw background
        if (hasCharacterSelect && characterSelectSprite) {
            window.clear();
            window.draw(*characterSelectSprite);
        } else if (context.hasBackground && context.backgroundSprite) {
            window.clear();
            window.draw(*context.backgroundSprite);
        } else {
            window.clear(sf::Color(12, 12, 30));
        }
        window.draw(gangster1Sprite);
        window.draw(gangster3Sprite);
        window.draw(selectionLabel);
        window.display();
    }
}

