#include "GameStage.hpp"

#include <SFML/Audio.hpp>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "AssetPaths.hpp"

using namespace std;

namespace {
constexpr int kStageDurationSeconds = 60;
constexpr int kMaxAmmo = 5;
constexpr float kFrameTime = 0.12f;

enum class SpriteState {
    Walk,
    Run,
    Jump,
    Shot,
    Attack,
    Idle,
    Hurt,
    Dead
};

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    bool fromPlayer = true;
    bool active = true;
    
    explicit Bullet(const sf::Texture& texture)
        : sprite(texture) {}
};

struct AnimatedSprite {
    sf::Texture texture;
    unique_ptr<sf::Sprite> sprite;
    int frameWidth = 0;
    int frameHeight = 0;
    int frameCount = 1;
    int currentFrame = 0;
    float accumulator = 0.f;

    bool load(const string& path, bool isDeadSprite = false) {
        if (!texture.loadFromFile(path)) {
            cerr << "Failed to load sprite sheet: " << path << '\n';
            return false;
        }
        texture.setSmooth(true);
        sprite = make_unique<sf::Sprite>(texture);

        const auto size = texture.getSize();
        frameHeight = static_cast<int>(size.y);
        frameCount = max(1u, size.x / size.y);
        frameWidth = static_cast<int>(size.x / frameCount);
        // For dead sprite, always show first frame (don't animate)
        if (isDeadSprite) {
            currentFrame = 0;
        }
        sprite->setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{frameWidth, frameHeight}));
        return true;
    }

    void setPosition(const sf::Vector2f& pos) {
        if (sprite) {
            sprite->setPosition(pos);
        }
    }
    void setScale(const sf::Vector2f& scale) {
        if (sprite) {
            sprite->setScale(scale);
        }
    }

    void update(float delta) {
        accumulator += delta;
        if (accumulator >= kFrameTime) {
            accumulator = 0.f;
            currentFrame = (currentFrame + 1) % frameCount;
            if (sprite) {
                sprite->setTextureRect(sf::IntRect(
                    sf::Vector2i{currentFrame * frameWidth, 0}, sf::Vector2i{frameWidth, frameHeight}));
            }
        }
    }
};

struct CharacterSpriteManager {
    AnimatedSprite idle;
    AnimatedSprite walk;
    AnimatedSprite run;
    AnimatedSprite jump;
    AnimatedSprite shot;
    AnimatedSprite attack;
    AnimatedSprite hurt;
    AnimatedSprite dead;
    SpriteState currentState = SpriteState::Walk;
    SpriteState previousState = SpriteState::Walk;
    sf::Clock actionClock;
    float actionDuration = 0.f;
    sf::Vector2f baseScale{1.8f, 1.8f};
    bool facingLeft = true;
    
    bool isFacingLeft() const { return facingLeft; }
    
    bool loadAll(bool isGangster1) {
        if (isGangster1) {
            return idle.load(kGangster1Idle) &&
                   walk.load(kGangster1Walk) &&
                   run.load(kGangster1Run) &&
                   jump.load(kGangster1Jump) &&
                   shot.load(kGangster1Shot) &&
                   attack.load(kGangster1Attack1) &&
                   hurt.load(kGangster1Hurt) &&
                   dead.load(kGangster1Dead, true); // true = is dead sprite, don't animate
        } else {
            return idle.load(kGangster3Idle) &&
                   walk.load(kGangster3Walk) &&
                   run.load(kGangster3Run) &&
                   jump.load(kGangster3Jump) &&
                   shot.load(kGangster3Shot) &&
                   attack.load(kGangster3Attack) &&
                   hurt.load(kGangster3Hurt) &&
                   dead.load(kGangster3Dead, true); // true = is dead sprite, don't animate
        }
    }
    
    void setScale(const sf::Vector2f& scale) {
        baseScale = scale;
        updateScale();
    }
    
    void setFacingDirection(bool faceLeft) {
        facingLeft = faceLeft;
        updateScale();
    }
    
    void updateScale() {
        sf::Vector2f scale = baseScale;
        
        // Always keep origin at (0,0) - simpler and more reliable
        if (idle.sprite) {
            sf::Vector2f origin(0.f, 0.f);
            idle.sprite->setOrigin(origin);
            if (walk.sprite) walk.sprite->setOrigin(origin);
            if (run.sprite) run.sprite->setOrigin(origin);
            if (jump.sprite) jump.sprite->setOrigin(origin);
            if (shot.sprite) shot.sprite->setOrigin(origin);
            if (attack.sprite) attack.sprite->setOrigin(origin);
            if (hurt.sprite) hurt.sprite->setOrigin(origin);
            if (dead.sprite) dead.sprite->setOrigin(origin);
        }
        
        // Flip horizontally by using negative X scale when facing right
        if (!facingLeft) {
            scale.x = -std::abs(scale.x);
        } else {
            scale.x = std::abs(scale.x);
        }
        
        idle.setScale(scale);
        walk.setScale(scale);
        run.setScale(scale);
        jump.setScale(scale);
        shot.setScale(scale);
        attack.setScale(scale);
        hurt.setScale(scale);
        dead.setScale(scale);
    }
    
    void setPosition(const sf::Vector2f& pos) {
        // When using negative scale with origin at (0,0), the sprite flips around its top-left corner
        // So we need to shift the position right by the sprite width when flipped
        sf::Vector2f adjustedPos = pos;
        if (!facingLeft && walk.frameWidth > 0) {
            // When flipped right, shift position to compensate
            adjustedPos.x += static_cast<float>(walk.frameWidth) * std::abs(baseScale.x);
        }
        idle.setPosition(adjustedPos);
        walk.setPosition(adjustedPos);
        run.setPosition(adjustedPos);
        jump.setPosition(adjustedPos);
        shot.setPosition(adjustedPos);
        attack.setPosition(adjustedPos);
        hurt.setPosition(adjustedPos);
        dead.setPosition(adjustedPos);
    }
    
    bool canChangeState() const {
        // Don't allow state changes if we're in a one-time animation
        return actionDuration <= 0.f || actionClock.getElapsedTime().asSeconds() >= actionDuration;
    }
    
    void changeState(SpriteState newState, float duration = 0.f) {
        if (newState != currentState) {
            // Save previous state only if not in a one-time animation
            if (actionDuration <= 0.f) {
                previousState = currentState;
            }
            currentState = newState;
            actionClock.restart();
            actionDuration = duration;
            // Reapply facing direction after state change to maintain correct orientation
            updateScale();
        }
    }
    
    void update(float delta) {
        idle.update(delta);
        walk.update(delta);
        run.update(delta);
        jump.update(delta);
        shot.update(delta);
        attack.update(delta);
        hurt.update(delta);
        // Don't animate dead sprite - show static first frame
        // dead.update(delta); // Commented out to prevent animation
        
        // Return from one-time animations to previous state
        if (actionDuration > 0.f && actionClock.getElapsedTime().asSeconds() >= actionDuration) {
            changeState(previousState);
            actionDuration = 0.f;
        }
    }
    
    sf::Sprite* getCurrentSprite() {
        sf::Sprite* sprite = nullptr;
        switch (currentState) {
            case SpriteState::Idle: sprite = idle.sprite.get(); break;
            case SpriteState::Walk: sprite = walk.sprite.get(); break;
            case SpriteState::Run: sprite = run.sprite.get(); break;
            case SpriteState::Jump: sprite = jump.sprite.get(); break;
            case SpriteState::Shot: sprite = shot.sprite.get(); break;
            case SpriteState::Attack: sprite = attack.sprite.get(); break;
            case SpriteState::Hurt: sprite = hurt.sprite.get(); break;
            case SpriteState::Dead: sprite = dead.sprite.get(); break;
            default: sprite = idle.sprite.get(); break;
        }
        // Fallback to idle sprite if current sprite is null
        return sprite ? sprite : idle.sprite.get();
    }
};
}

// drawWinBadge function removed

void GameStage::run(sf::RenderWindow& window, GameContext& context) {
    const float windowWidth = 960.f;  // Fixed window width
    const float groundY = 300.f;

    CharacterSpriteManager playerSprites;
    CharacterSpriteManager enemySprites;
    const bool playerIsGangster1 = context.selectedCharacter == CharacterChoice::Gangster1;

    if (!playerSprites.loadAll(playerIsGangster1) || !enemySprites.loadAll(!playerIsGangster1)) {
        return;
    }

    const sf::Vector2f baseScale{1.8f, 1.8f};
    playerSprites.setScale(baseScale);
    enemySprites.setScale(baseScale);
    sf::Vector2f playerPosition{120.f, groundY};
    sf::Vector2f enemyPosition{windowWidth - 250.f, groundY};  // Move enemy away from edge
    // CRITICAL: Ensure initial positions are exactly on ground
    playerPosition.y = groundY;
    enemyPosition.y = groundY;
    playerSprites.setPosition(playerPosition);
    enemySprites.setPosition(enemyPosition);

    // Bullet rendering
    sf::Texture bulletTexture;
    {
        sf::Image bulletImage;
        if (bulletImage.loadFromFile(kBulletSprite)) {
            // Treat the top-left pixel as background and make it transparent
            const sf::Color bg = bulletImage.getPixel(sf::Vector2u{0u, 0u});
            bulletImage.createMaskFromColor(bg);
            if (!bulletTexture.loadFromImage(bulletImage)) {
                cerr << "Warning: could not create bullet texture from image " << kBulletSprite << '\n';
            }
        } else {
            cerr << "Warning: could not load bullet sprite from " << kBulletSprite << '\n';
        }
    }
    vector<Bullet> bullets;

    const sf::Vector2f barSize{220.f, 24.f};
    const sf::Vector2f leftBarPos{10.f, 30.f};
    // Position enemy UI further from edge to prevent overflow
    sf::Vector2f rightBarPos{windowWidth - barSize.x - 50.f, 30.f};

    sf::RectangleShape leftHealthBack(barSize);
    leftHealthBack.setFillColor(sf::Color(40, 40, 40));
    leftHealthBack.setOutlineThickness(2.f);
    leftHealthBack.setOutlineColor(sf::Color(15, 15, 15));
    leftHealthBack.setPosition(leftBarPos);

    sf::RectangleShape leftHealthBar(barSize);
    leftHealthBar.setFillColor(sf::Color(200, 40, 40));
    leftHealthBar.setPosition(leftBarPos);

    sf::RectangleShape rightHealthBack(barSize);
    rightHealthBack.setFillColor(sf::Color(40, 40, 40));
    rightHealthBack.setOutlineThickness(2.f);
    rightHealthBack.setOutlineColor(sf::Color(15, 15, 15));
    rightHealthBack.setPosition(rightBarPos);

    sf::RectangleShape rightHealthBar(barSize);
    rightHealthBar.setFillColor(sf::Color(200, 40, 40));
    rightHealthBar.setPosition(rightBarPos);

    float playerHealth = 100.f;
    float enemyHealth = 100.f;

    // Removed bullet symbol

    sf::Text leftAmmoText(context.font, "");
    leftAmmoText.setCharacterSize(22);
    leftAmmoText.setFillColor(sf::Color::White);
    leftAmmoText.setPosition(leftBarPos + sf::Vector2f{0.f, barSize.y + 8.f});

    sf::Text rightAmmoText(context.font, "");
    rightAmmoText.setCharacterSize(22);
    rightAmmoText.setFillColor(sf::Color::White);
    rightAmmoText.setPosition(rightBarPos + sf::Vector2f{0.f, barSize.y + 8.f});

    sf::Text timerText(context.font, "");
    timerText.setCharacterSize(30);
    timerText.setFillColor(sf::Color::White);

    sf::Text actionLabel(context.font, "");
    actionLabel.setCharacterSize(20);
    actionLabel.setFillColor(sf::Color(200, 200, 200));

    queue<int> playerAmmo;
    int playerReloads = 2;  // Start with 2 reloads
    auto reloadPlayer = [&](bool isInitialLoad = false) {
        if (isInitialLoad || playerReloads > 0) {
            queue<int> empty;
            swap(playerAmmo, empty);
            for (int i = 0; i < kMaxAmmo; ++i) {
                playerAmmo.push(i);
            }
            if (!isInitialLoad) {
                playerReloads--;
            }
            context.actionHistory.push("Reloaded ammo");
        }
    };
    reloadPlayer(true);  // Initial reload (doesn't count against the 3)

    int enemyAmmo = kMaxAmmo;
    int enemyReloads = 2;  // Enemy also has 2 reloads
    const float playerSpeed = 220.f;
    const float enemySpeed = 160.f;
    const float jumpStrength = -420.f;
    const float gravity = 1200.f;
    const float enemyFireCooldown = 0.8f;
    const float enemyAttackCooldown = 0.7f;
    const float enemyReloadTime = 2.0f;

    bool waitingForStart = true;
    bool movingLeft = false;
    bool movingRight = false;
    bool isRunning = false;
    bool playerJumping = false;
    bool enemyJumping = false;
    float playerVerticalVelocity = 0.f;
    float enemyVerticalVelocity = 0.f;

    sf::Clock stageClock;
    sf::Clock deltaClock;
    sf::Clock animationClock;
    sf::Clock enemyDecisionClock;
    sf::Clock enemyFireClock;
    sf::Clock enemyAttackClock;
    sf::Clock enemyReloadClock;
    sf::Clock playerAttackCooldown;
    sf::Clock playerShootCooldown;
    const float attackCooldownTime = 0.6f;
    const float shootCooldownTime = 0.5f;
    bool enemyIsReloading = false;
    bool enemyIsRunning = false;
    stageClock.restart();
    deltaClock.restart();
    animationClock.restart();
    enemyDecisionClock.restart();
    enemyFireClock.restart();
    enemyAttackClock.restart();
    enemyReloadClock.restart();
    playerAttackCooldown.restart();
    playerShootCooldown.restart();

    sf::Text startPrompt(context.font, "Press ENTER to start");
    startPrompt.setCharacterSize(28);
    startPrompt.setFillColor(sf::Color::White);

    bool winNoted = false;
    bool defeatNoted = false;
    int enemyDirection = -1;
    
    // Rounds system
    int currentRound = 1;
    const int maxRounds = 3;
    int playerWins = 0;
    int enemyWins = 0;
    bool roundEnded = false;
    bool gameEnded = false;
    sf::Clock roundEndClock;
    const float roundEndDisplayTime = 3.0f;
    
    // Hit stun
    sf::Clock playerHitStunClock;
    sf::Clock enemyHitStunClock;
    const float hitStunDuration = 0.5f;
    bool playerHitStunned = false;
    bool enemyHitStunned = false;
    
    // Sound effects
    sf::SoundBuffer gunBuffer, tommyGunBuffer, bodyMeleeHitBuffer, swingBuffer, deadBuffer;
    unique_ptr<sf::Sound> gunSound, tommyGunSound, bodyMeleeHitSound, swingSound, deadSound;
    
    // Load sound effects
    if (gunBuffer.loadFromFile("sfx/Gun.mp3")) {
        gunSound = make_unique<sf::Sound>(gunBuffer);
    }
    if (tommyGunBuffer.loadFromFile("sfx/TommyGun.mp3")) {
        tommyGunSound = make_unique<sf::Sound>(tommyGunBuffer);
    }
    if (bodyMeleeHitBuffer.loadFromFile("sfx/BodyMeleeHit.mp3")) {
        bodyMeleeHitSound = make_unique<sf::Sound>(bodyMeleeHitBuffer);
    }
    if (swingBuffer.loadFromFile("sfx/Swing.mp3")) {
        swingSound = make_unique<sf::Sound>(swingBuffer);
    }
    if (deadBuffer.loadFromFile("sfx/Dead.mp3")) {
        deadSound = make_unique<sf::Sound>(deadBuffer);
        deadSound->setVolume(30.f);
    }
    
    // Load and play game music (louder than sound effects)
    sf::Music gameMusic;
    bool gameMusicPlaying = false;
    if (gameMusic.openFromFile("sfx/GameMusic.mp3")) {
        gameMusic.setLooping(true);
        gameMusic.setVolume(70.f); // Louder than sound effects
        gameMusicPlaying = true;
    }

    // Helper function to update background scale
    auto updateBackgroundScale = [&]() {
        if (context.hasBackground && context.backgroundSprite) {
            const auto bounds = context.backgroundSprite->getLocalBounds();
            const float scaleX = static_cast<float>(window.getSize().x) / bounds.size.x;
            const float scaleY = static_cast<float>(window.getSize().y) / bounds.size.y;
            context.backgroundSprite->setScale(sf::Vector2f{scaleX, scaleY});
        }
    };

    while (window.isOpen()) {
        while (auto eventOpt = window.pollEvent()) {
            const auto& event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            if (const auto keyEvent = event.getIf<sf::Event::KeyPressed>()) {
                if (waitingForStart && keyEvent->code == sf::Keyboard::Key::Enter) {
                    waitingForStart = false;
                    stageClock.restart();
                    deltaClock.restart();
                    animationClock.restart();
                    enemyDecisionClock.restart();
                    enemyFireClock.restart();
                    continue;
                }
                if (waitingForStart) {
                    continue;
                }

                switch (keyEvent->code) {
                case sf::Keyboard::Key::Left:
                    movingLeft = true;
                    break;
                case sf::Keyboard::Key::Right:
                    movingRight = true;
                    break;
                case sf::Keyboard::Key::Down:
                    isRunning = true;
                    break;
                case sf::Keyboard::Key::Up:
                    if (!playerJumping && !playerHitStunned) {
                        playerJumping = true;
                        playerVerticalVelocity = jumpStrength;
                        playerSprites.changeState(SpriteState::Jump);
                        context.actionHistory.push("Player jumped");
                    }
                    break;
                case sf::Keyboard::Key::A:
                    if (!playerAmmo.empty() && 
                        playerShootCooldown.getElapsedTime().asSeconds() >= shootCooldownTime &&
                        playerSprites.canChangeState() && !playerHitStunned &&
                        playerHealth > 0.f && enemyHealth > 0.f) {
                        playerAmmo.pop();
                        // Spawn a visible bullet that will handle collision later
                        if (bulletTexture.getSize().x > 0 && bulletTexture.getSize().y > 0) {
                            Bullet b{bulletTexture};
                            b.fromPlayer = true;
                            b.active = true;
                            // Make bullet smaller than the gun tip
                            b.sprite.setScale(sf::Vector2f{0.05f, 0.05f});

                            // Determine direction based on where the player is facing.
                            // NOTE: isFacingLeft() == true means sprite is in its default (right-facing)
                            // orientation; false means flipped to face left. So bullets must use:
                            // facing right -> +1, facing left -> -1.
                            float dir = playerSprites.isFacingLeft() ? 1.f : -1.f;

                            // Place bullet at the gun tip using the current player sprite bounds
                            sf::Vector2f startPos = playerPosition;
                            if (auto* playerSprite = playerSprites.getCurrentSprite()) {
                                const auto pb = playerSprite->getGlobalBounds();
                                // Use a lower point on the sprite so the bullet leaves around the gun
                                startPos.y = pb.position.y + pb.size.y * 0.6f;
                                if (dir > 0.f) {
                                    startPos.x = pb.position.x + pb.size.x - 10.f;
                                } else {
                                    startPos.x = pb.position.x + 10.f;
                                }
                            } else {
                                // Fallback: slightly above feet, in front of player
                                startPos.y -= 28.f;
                                startPos.x += dir * 40.f;
                            }

                            b.velocity = sf::Vector2f{700.f * dir, 0.f};
                            b.sprite.setPosition(startPos);
                            bullets.push_back(b);
                        }
                        playerSprites.changeState(SpriteState::Shot, shootCooldownTime);
                        playerShootCooldown.restart();
                        context.actionHistory.push("Player fired");
                    }
                    break;
                case sf::Keyboard::Key::S:
                    if (playerAttackCooldown.getElapsedTime().asSeconds() >= attackCooldownTime &&
                        playerSprites.canChangeState() && !playerHitStunned &&
                        playerHealth > 0.f && enemyHealth > 0.f) {
                        playerSprites.changeState(SpriteState::Attack, attackCooldownTime);
                        // Check if melee hits (close range)
                        float distanceToEnemy = std::abs(enemyPosition.x - playerPosition.x);
                        if (distanceToEnemy < 120.f) {
                            enemyHealth = max(0.f, enemyHealth - 8.f);
                            enemyHitStunned = true;
                            enemyHitStunClock.restart();
                            enemySprites.changeState(SpriteState::Hurt, hitStunDuration);
                            if (bodyMeleeHitSound) bodyMeleeHitSound->play();
                        } else {
                            if (swingSound) swingSound->play();
                        }
                        playerAttackCooldown.restart();
                        context.actionHistory.push("Player melee attack");
                    }
                    break;
                case sf::Keyboard::Key::R:
                    reloadPlayer();
                    break;
                default:
                    break;
                }
            } else if (const auto keyUp = event.getIf<sf::Event::KeyReleased>()) {
                if (waitingForStart) {
                    continue;
                }
                switch (keyUp->code) {
                case sf::Keyboard::Key::Left:
                    movingLeft = false;
                    break;
                case sf::Keyboard::Key::Right:
                    movingRight = false;
                    break;
                case sf::Keyboard::Key::Down:
                    isRunning = false;
                    break;
                default:
                    break;
                }
            }
        }

        const float delta = deltaClock.restart().asSeconds();
        const float animationDelta = animationClock.restart().asSeconds();
        playerSprites.update(animationDelta);
        enemySprites.update(animationDelta);

        // Start game music when game starts
        if (waitingForStart && gameMusicPlaying) {
            gameMusic.play();
        }
        
        if (waitingForStart) {
            // Fixed window size
            sf::Vector2f rightBarPos{windowWidth - barSize.x - 50.f, 30.f};
            rightHealthBack.setPosition(rightBarPos);
            rightHealthBar.setPosition(rightBarPos);
            rightAmmoText.setPosition(rightBarPos + sf::Vector2f{0.f, barSize.y + 8.f});
            
            timerText.setString("Timer: 60s");
            sf::FloatRect timerBounds = timerText.getLocalBounds();
            // Center timer between health bars at the top, but ensure it fits fully on screen
            float timerX = windowWidth / 2.f - timerBounds.size.x / 2.f;
            float minTimerX = leftBarPos.x + barSize.x + 15.f;
            float maxTimerX = rightBarPos.x - timerBounds.size.x - 15.f;
            timerX = std::max(minTimerX, std::min(timerX, maxTimerX));
            timerText.setPosition(sf::Vector2f(timerX, leftBarPos.y));
            leftHealthBar.setSize(barSize);
            rightHealthBar.setSize(barSize);
            leftAmmoText.setString(string("Ammo: ") + to_string(playerAmmo.size()) + string(" | Reloads: ") + to_string(playerReloads));
            rightAmmoText.setString(string("Ammo: ") + to_string(enemyAmmo) + string(" | Reloads: ") + to_string(enemyReloads));
            sf::FloatRect promptBounds = startPrompt.getLocalBounds();
            startPrompt.setPosition(
                sf::Vector2f{windowWidth / 2.f - promptBounds.size.x / 2.f, leftBarPos.y + 80.f});

            if (context.hasBackground && context.backgroundSprite) {
                window.clear();
                window.draw(*context.backgroundSprite);
            } else {
                window.clear(sf::Color(10, 10, 25));
            }
            // Draw health bars first (background layer)
            window.draw(leftHealthBack);
            window.draw(rightHealthBack);
            window.draw(leftHealthBar);
            window.draw(rightHealthBar);
            // Draw text on top
            window.draw(leftAmmoText);
            window.draw(rightAmmoText);
            window.draw(timerText);
            window.draw(actionLabel);
            if (auto* sprite = playerSprites.getCurrentSprite()) {
                window.draw(*sprite);
            }
            if (auto* sprite = enemySprites.getCurrentSprite()) {
                window.draw(*sprite);
            }
            window.draw(startPrompt);
            window.display();
            continue;
        }

        // Update hit stun
        if (playerHitStunned && playerHitStunClock.getElapsedTime().asSeconds() >= hitStunDuration) {
            playerHitStunned = false;
        }
        if (enemyHitStunned && enemyHitStunClock.getElapsedTime().asSeconds() >= hitStunDuration) {
            enemyHitStunned = false;
        }
        
        // Player movement (disabled during hit stun or when round ended)
        float currentSpeed = isRunning ? playerSpeed * 1.5f : playerSpeed;
        sf::Vector2f playerMotion{0.f, 0.f};
        if (!playerHitStunned && !roundEnded && playerHealth > 0.f) {
            if (movingLeft) {
                playerMotion.x -= currentSpeed * delta;
            }
            if (movingRight) {
                playerMotion.x += currentSpeed * delta;
            }
        }
        // Jump-over functionality: allow jumping over enemy if close and jumping
        float distanceToEnemy = std::abs(enemyPosition.x - playerPosition.x);
        bool canJumpOver = distanceToEnemy < 100.f && playerJumping;
        
        playerPosition += playerMotion;
        
        // If player is dead, allow them to pass through enemy (no boundary restriction)
        if (playerHealth <= 0.f) {
            // Dead player can move anywhere
            playerPosition.x = std::clamp(playerPosition.x, 40.f, windowWidth - 60.f);
        } else if (canJumpOver) {
            // Allow player to move past enemy when jumping
            playerPosition.x = std::clamp(playerPosition.x, 40.f, windowWidth - 60.f);
        } else {
            // Normal boundary restriction
            playerPosition.x = std::clamp(playerPosition.x, 40.f, windowWidth / 2.f - 60.f);
        }

        // Update player facing direction based on movement
        // Note: facingLeft=true means normal orientation, facingLeft=false means flipped
        // Sprites default to facing RIGHT (normal), so we flip when moving left
        if (movingLeft) {
            playerSprites.setFacingDirection(false); // Face left (flip from default right)
        } else if (movingRight) {
            playerSprites.setFacingDirection(true);  // Face right (normal orientation)
        }
        // If not moving, keep current facing direction
        
        // Update sprite state based on movement
        if (playerJumping) {
            // Keep jump sprite while in air
            if (playerSprites.currentState != SpriteState::Jump) {
                playerSprites.changeState(SpriteState::Jump);
            }
            playerVerticalVelocity += gravity * delta;
            playerPosition.y += playerVerticalVelocity * delta;
            if (playerPosition.y >= groundY) {
                playerPosition.y = groundY;
                playerJumping = false;
                playerVerticalVelocity = 0.f;
                // Return to walk/run state after landing
                if (isRunning && (movingLeft || movingRight)) {
                    playerSprites.changeState(SpriteState::Run);
                } else if (movingLeft || movingRight) {
                    playerSprites.changeState(SpriteState::Walk);
                } else {
                    playerSprites.changeState(SpriteState::Walk);
                }
            }
        } else {
            // CRITICAL: If not jumping, player MUST be on ground
            // Handle dead players separately
            if (playerHealth <= 0.f) {
                // Dead character falls naturally
                playerVerticalVelocity += gravity * delta;
                playerPosition.y += playerVerticalVelocity * delta;
                if (playerPosition.y >= groundY) {
                    playerPosition.y = groundY;
                    playerVerticalVelocity = 0.f;
                }
            } else {
                // ALIVE and not jumping - FORCE to ground immediately
                playerPosition.y = groundY;
                playerVerticalVelocity = 0.f;  // Reset velocity to prevent flying
            }
            // Update sprite state when not jumping and not in a one-time animation
            if (playerSprites.currentState != SpriteState::Jump &&
                playerSprites.currentState != SpriteState::Shot && 
                playerSprites.currentState != SpriteState::Attack &&
                playerSprites.currentState != SpriteState::Hurt &&
                playerSprites.currentState != SpriteState::Dead) {
                // Show dead sprite only when health is 0
                if (playerHealth <= 0.f) {
                    playerSprites.changeState(SpriteState::Dead);
                } else if (isRunning && (movingLeft || movingRight) && !playerHitStunned) {
                    playerSprites.changeState(SpriteState::Run);
                } else if ((movingLeft || movingRight) && !playerHitStunned) {
                    playerSprites.changeState(SpriteState::Walk);
                } else if (!playerHitStunned) {
                    playerSprites.changeState(SpriteState::Idle);
                    // When changing to idle, ensure player is on ground
                    if (playerHealth > 0.f) {
                        playerPosition.y = groundY;
                        playerVerticalVelocity = 0.f;
                    }
                }
            }
        }
        
        // CRITICAL: ALWAYS ensure player is on ground when not jumping (before setting sprite position)
        // This must happen unconditionally to prevent floating - enforce it STRICTLY
        // This is the FINAL check before sprite positioning
        if (!playerJumping && playerHealth > 0.f) {
            playerPosition.y = groundY;
            playerVerticalVelocity = 0.f;
        }
        // Set sprite position - player should now be on ground
        playerSprites.setPosition(playerPosition);

        // ------------------------------------------------------------------
        // Bullet updates (movement + collision)
        // ------------------------------------------------------------------
        if (!bullets.empty()) {
            for (auto& b : bullets) {
                if (!b.active) continue;
                b.sprite.move(b.velocity * delta);
                const auto pos = b.sprite.getPosition();
                if (pos.x < -50.f || pos.x > windowWidth + 50.f) {
                    b.active = false;
                    continue;
                }
                // Player bullet hitting the enemy
                if (b.fromPlayer && enemyHealth > 0.f) {
                    if (auto* enemySprite = enemySprites.getCurrentSprite()) {
                        const auto bulletBounds = b.sprite.getGlobalBounds();
                        const auto enemyBounds = enemySprite->getGlobalBounds();
                        const bool overlap =
                            bulletBounds.position.x < enemyBounds.position.x + enemyBounds.size.x &&
                            bulletBounds.position.x + bulletBounds.size.x > enemyBounds.position.x &&
                            bulletBounds.position.y < enemyBounds.position.y + enemyBounds.size.y &&
                            bulletBounds.position.y + bulletBounds.size.y > enemyBounds.position.y;

                        // Close-range fix: if the bullet overlaps the enemy at all, it's a hit.
                        if (overlap) {
                            enemyHealth = max(0.f, enemyHealth - 6.f);
                            enemyHitStunned = true;
                            enemyHitStunClock.restart();
                            enemySprites.changeState(SpriteState::Hurt, hitStunDuration);
                            if (playerIsGangster1 && tommyGunSound) {
                                tommyGunSound->play();
                            } else if (gunSound) {
                                gunSound->play();
                            }
                            b.active = false;
                        }
                    }
                }

                // Enemy bullet hitting the player
                // Enemy bullet hitting the player
                if (!b.fromPlayer && playerHealth > 0.f) {
                    if (auto* playerSprite = playerSprites.getCurrentSprite()) {
                        const auto bulletBounds = b.sprite.getGlobalBounds();
                        const auto playerBounds = playerSprite->getGlobalBounds();
                        const bool overlap =
                            bulletBounds.position.x < playerBounds.position.x + playerBounds.size.x &&
                            bulletBounds.position.x + bulletBounds.size.x > playerBounds.position.x &&
                            bulletBounds.position.y < playerBounds.position.y + playerBounds.size.y &&
                            bulletBounds.position.y + bulletBounds.size.y > playerBounds.position.y;

                        if (overlap) {
                            playerHealth = max(0.f, playerHealth - 5.f);
                            playerHitStunned = true;
                            playerHitStunClock.restart();
                            playerSprites.changeState(SpriteState::Hurt, hitStunDuration);
                            if (!playerIsGangster1 && tommyGunSound) {
                                tommyGunSound->play();
                            } else if (gunSound) {
                                gunSound->play();
                            }
                            b.active = false;
                        }
                    }
                }
            }
            bullets.erase(remove_if(bullets.begin(), bullets.end(),
                                    [](const Bullet& b) { return !b.active; }),
                          bullets.end());
        }

        // Enemy reload logic (with reload limit)
        if (enemyAmmo <= 0 && !enemyIsReloading && enemyReloads > 0) {
            enemyIsReloading = true;
            enemyReloadClock.restart();
            context.actionHistory.push("Enemy reloading");
        }
        if (enemyIsReloading && enemyReloadClock.getElapsedTime().asSeconds() >= enemyReloadTime) {
            if (enemyReloads > 0) {
                enemyAmmo = kMaxAmmo;
                enemyReloads--;
                enemyIsReloading = false;
                context.actionHistory.push("Enemy reloaded");
            } else {
                enemyIsReloading = false;
            }
        }

        float distanceToPlayer = std::abs(enemyPosition.x - playerPosition.x);
        bool canShoot = !enemyIsReloading && enemyAmmo > 0 && 
                       enemyFireClock.getElapsedTime().asSeconds() >= enemyFireCooldown &&
                       enemySprites.canChangeState();
        bool canMelee = enemyAttackClock.getElapsedTime().asSeconds() >= enemyAttackCooldown &&
                       enemySprites.canChangeState();
        bool isClose = distanceToPlayer < 120.f;
        bool isMidRange = distanceToPlayer >= 120.f && distanceToPlayer < 300.f;

        // Enemy AI decision making - more aggressive
        if (enemyDecisionClock.getElapsedTime().asSeconds() > 0.3f) {
            enemyDecisionClock.restart();
            
            if (enemyIsReloading || !enemyJumping) {
                // Determine movement direction
                if (isClose) {
                    // Close range: move away or prepare for melee
                    if (enemyPosition.x > playerPosition.x + 60.f) {
                        enemyDirection = -1; // Move left
                        enemyIsRunning = true;
                    } else if (enemyPosition.x < playerPosition.x - 40.f) {
                        enemyDirection = 1; // Move right
                        enemyIsRunning = true;
                    } else {
                        enemyDirection = 0; // Stay for melee
                        enemyIsRunning = false;
                    }
                } else if (isMidRange) {
                    // Mid range: try to get in shooting range or closer for melee
                    if (enemyPosition.x > playerPosition.x + 180.f) {
                        enemyDirection = -1;
                        enemyIsRunning = false;
                    } else if (enemyPosition.x < playerPosition.x - 80.f) {
                        enemyDirection = 1;
                        enemyIsRunning = false;
                    } else {
                        enemyDirection = 0;
                        enemyIsRunning = false;
                    }
                } else {
                    // Far range: close the distance
                    if (enemyPosition.x > playerPosition.x + 100.f) {
                        enemyDirection = -1;
                        enemyIsRunning = true;
                    } else {
                        enemyDirection = 1;
                        enemyIsRunning = true;
                    }
                }

                // Jump if player is above or if stuck
                if (!enemyJumping && distanceToPlayer < 80.f && 
                    std::abs(enemyPosition.y - playerPosition.y) < 10.f) {
                    enemyJumping = true;
                    enemyVerticalVelocity = jumpStrength * 0.85f;
                    enemySprites.changeState(SpriteState::Jump);
                }
            }
        }

        // Enemy movement - allow it to get closer to player (disabled during hit stun, when round ended, or when dead)
        float currentEnemySpeed = enemyIsRunning ? enemySpeed * 1.4f : enemySpeed;
        sf::Vector2f enemyMotion{0.f, 0.f};
        if (!enemyHitStunned && !roundEnded && enemyHealth > 0.f && playerHealth > 0.f) {
            enemyMotion.x = static_cast<float>(enemyDirection) * currentEnemySpeed * delta;
        }
        enemyPosition += enemyMotion;
        // If enemy or player is dead, allow them to pass through each other
        float minEnemyX = 40.f;  // Default minimum
        if (enemyHealth > 0.f && playerHealth > 0.f) {
            // Both alive: keep enemy on the right side of the player
            minEnemyX = playerPosition.x + 40.f;
        }
        float maxEnemyX = windowWidth - 120.f;  // Keep enemy well within window bounds
        enemyPosition.x = std::clamp(enemyPosition.x, minEnemyX, maxEnemyX);

        // Enemy jumping physics
        if (enemyJumping) {
            if (enemySprites.currentState != SpriteState::Jump) {
                enemySprites.changeState(SpriteState::Jump);
            }
            enemyVerticalVelocity += gravity * delta;
            enemyPosition.y += enemyVerticalVelocity * delta;
            if (enemyPosition.y >= groundY) {
                enemyPosition.y = groundY;
                enemyJumping = false;
                enemyVerticalVelocity = 0.f;
                // Return to appropriate state after landing
                if (enemyIsRunning && enemyDirection != 0) {
                    enemySprites.changeState(SpriteState::Run);
                } else if (enemyDirection != 0) {
                    enemySprites.changeState(SpriteState::Walk);
                } else {
                    enemySprites.changeState(SpriteState::Walk);
                }
            }
        } else {
            // ALWAYS ensure enemy is on ground when not jumping
            if (enemyHealth > 0.f) {
                // Alive and not jumping - must be on ground
                enemyPosition.y = groundY;
                enemyVerticalVelocity = 0.f;
            } else {
                // If dead, allow falling with gravity
                enemyVerticalVelocity += gravity * delta;
                enemyPosition.y += enemyVerticalVelocity * delta;
                if (enemyPosition.y >= groundY) {
                    enemyPosition.y = groundY;
                    enemyVerticalVelocity = 0.f;
                }
            }
            // Update enemy sprite based on state (when not in action animations)
            if (enemySprites.currentState != SpriteState::Jump &&
                enemySprites.currentState != SpriteState::Shot &&
                enemySprites.currentState != SpriteState::Attack &&
                enemySprites.currentState != SpriteState::Hurt &&
                enemySprites.currentState != SpriteState::Dead) {
                // Show dead sprite only when health is 0
                if (enemyHealth <= 0.f) {
                    enemySprites.changeState(SpriteState::Dead);
                } else if (enemyIsRunning && enemyDirection != 0 && !enemyHitStunned) {
                    enemySprites.changeState(SpriteState::Run);
                } else if (enemyDirection != 0 && !enemyHitStunned) {
                    enemySprites.changeState(SpriteState::Walk);
                } else if (!enemyHitStunned) {
                    enemySprites.changeState(SpriteState::Idle);
                    // When changing to idle, ensure enemy is on ground
                    if (enemyHealth > 0.f) {
                        enemyPosition.y = groundY;
                        enemyVerticalVelocity = 0.f;
                    }
                }
            }
        }

        // Make enemy face the player
        // If enemy is to the right of player, enemy should face left (towards player)
        // If enemy is to the left of player, enemy should face right (towards player)
        // Note: facingLeft=true means normal orientation (default facing right), facingLeft=false means flipped (facing left)
        bool enemyToRightOfPlayer = enemyPosition.x > playerPosition.x;
        // If enemy is right of player, face left (flip = false)
        // If enemy is left of player, face right (normal = true)
        bool enemyShouldFaceLeft = !enemyToRightOfPlayer;
        enemySprites.setFacingDirection(enemyShouldFaceLeft);
        
        // ALWAYS ensure enemy is on ground when not jumping (before setting sprite position)
        if (!enemyJumping && enemyHealth > 0.f) {
            enemyPosition.y = groundY;
            enemyVerticalVelocity = 0.f;
        }
        // Set position AFTER ensuring Y position is correct
        enemySprites.setPosition(enemyPosition);

        // Enemy attack decision - melee when close, shoot when mid-range
        // IMPORTANT: Set facing direction BEFORE attack to ensure enemy faces player during attack
        // Stop attacking if either character is dead
        if (!enemyJumping && !enemyIsReloading && !enemyHitStunned && 
            playerHealth > 0.f && enemyHealth > 0.f) {
            if (isClose && canMelee) {
                // Melee attack when close - ensure facing player
                enemySprites.setFacingDirection(enemyShouldFaceLeft);
                enemySprites.setPosition(enemyPosition); // Re-apply position with correct facing
                enemySprites.changeState(SpriteState::Attack, enemyAttackCooldown);
                playerHealth = max(0.f, playerHealth - 7.f);
                playerHitStunned = true;
                playerHitStunClock.restart();
                playerSprites.changeState(SpriteState::Hurt, hitStunDuration);
                if (bodyMeleeHitSound) bodyMeleeHitSound->play();
                enemyAttackClock.restart();
                context.actionHistory.push("Enemy melee attack");
            } else if (isMidRange && canShoot) {
                // Shoot when in range - ensure facing player
                enemySprites.setFacingDirection(enemyShouldFaceLeft);
                enemySprites.setPosition(enemyPosition); // Re-apply position with correct facing
                --enemyAmmo;

                // Spawn a visible bullet for the enemy, similar to the player's bullet
                if (bulletTexture.getSize().x > 0 && bulletTexture.getSize().y > 0) {
                    Bullet b{bulletTexture};
                    b.fromPlayer = false;
                    b.active = true;
                    b.sprite.setScale(sf::Vector2f{0.05f, 0.05f});

                    // Direction based on actual player position
                    float dir = (playerPosition.x >= enemyPosition.x) ? 1.f : -1.f;

                    // Place bullet at the enemy's gun tip using current sprite bounds
                    sf::Vector2f startPos = enemyPosition;
                    if (auto* enemySpritePtr = enemySprites.getCurrentSprite()) {
                        const auto eb = enemySpritePtr->getGlobalBounds();
                        startPos.y = eb.position.y + eb.size.y * 0.6f;
                        if (dir > 0.f) {
                            startPos.x = eb.position.x + eb.size.x - 10.f;
                        } else {
                            startPos.x = eb.position.x + 10.f;
                        }
                    } else {
                        startPos.y -= 28.f;
                        startPos.x += dir * 40.f;
                    }

                    b.velocity = sf::Vector2f{700.f * dir, 0.f};
                    b.sprite.setPosition(startPos);
                    bullets.push_back(b);
                }

                // Play sound based on enemy character
                if (!playerIsGangster1 && tommyGunSound) {
                    tommyGunSound->play();
                } else if (gunSound) {
                    gunSound->play();
                }
                enemySprites.changeState(SpriteState::Shot, enemyFireCooldown);
                enemyFireClock.restart();
                context.actionHistory.push("Enemy fired");
            }
        }

        int elapsed = static_cast<int>(stageClock.getElapsedTime().asSeconds());
        int timeLeft = max(0, kStageDurationSeconds - elapsed);

        // Fixed window size - use consistent positioning (50px from edge)
        sf::Vector2f rightBarPos{windowWidth - barSize.x - 50.f, 30.f};
        rightHealthBack.setPosition(rightBarPos);
        rightHealthBar.setPosition(rightBarPos);
        rightAmmoText.setPosition(rightBarPos + sf::Vector2f{0.f, barSize.y + 8.f});

        ostringstream oss;
        oss << "Timer: " << timeLeft << "s";
        timerText.setString(oss.str());
        sf::FloatRect timerBounds = timerText.getLocalBounds();
        // Center timer between health bars at the top, but ensure it fits fully on screen
        float timerX = windowWidth / 2.f - timerBounds.size.x / 2.f;
        // Clamp timer to ensure it's fully visible - leave space on both sides
        float minTimerX = leftBarPos.x + barSize.x + 15.f;
        float maxTimerX = rightBarPos.x - timerBounds.size.x - 15.f;
        timerX = std::max(minTimerX, std::min(timerX, maxTimerX));
        timerText.setPosition(sf::Vector2f(timerX, leftBarPos.y));

        leftHealthBar.setSize(sf::Vector2f(barSize.x * (playerHealth / 100.f), barSize.y));
        rightHealthBar.setSize(sf::Vector2f(barSize.x * (enemyHealth / 100.f), barSize.y));

        leftAmmoText.setString(string("Ammo: ") + to_string(playerAmmo.size()) + string(" | Reloads: ") + to_string(playerReloads));
        rightAmmoText.setString(string("Ammo: ") + to_string(enemyAmmo) + string(" | Reloads: ") + to_string(enemyReloads));

        // Update action label text and keep it visually aligned under the timer
        if (!context.actionHistory.empty()) {
            string fullText = string("Last: ") + context.actionHistory.top();
            actionLabel.setString(fullText);
            sf::FloatRect actionBounds = actionLabel.getLocalBounds();

            // Show the full text (no truncation), just center it under the timer
            float actionX = timerX + (timerBounds.size.x - actionBounds.size.x) / 2.f;
            float actionY = leftBarPos.y + barSize.y + 8.f;
            actionLabel.setPosition(sf::Vector2f{actionX, actionY});
        }

        // Show dead sprites when health is 0
        static bool playerDeadSoundPlayed = false;
        static bool enemyDeadSoundPlayed = false;
        static bool playerDeadAnimating = false;
        static bool enemyDeadAnimating = false;
        
        if (playerHealth <= 0.f && playerSprites.currentState != SpriteState::Dead) {
            playerSprites.changeState(SpriteState::Dead);
            playerDeadAnimating = true;
            playerSprites.dead.currentFrame = 0;
            playerSprites.dead.accumulator = 0.f;
            // Stop all movement immediately for both characters
            movingLeft = false;
            movingRight = false;
            isRunning = false;
            playerJumping = false;
            // Start falling if not already on ground
            if (playerPosition.y < groundY) {
                playerVerticalVelocity = 0.f;  // Start from current velocity
            } else {
                playerVerticalVelocity = 0.f;  // Already on ground
            }
            enemyDirection = 0;
            enemyIsRunning = false;
            enemyJumping = false;
            if (!playerDeadSoundPlayed && deadSound) {
                deadSound->play();
                playerDeadSoundPlayed = true;
            }
        }
        
        if (enemyHealth <= 0.f && enemySprites.currentState != SpriteState::Dead) {
            enemySprites.changeState(SpriteState::Dead);
            enemyDeadAnimating = true;
            enemySprites.dead.currentFrame = 0;
            enemySprites.dead.accumulator = 0.f;
            // Stop all movement immediately for both characters
            movingLeft = false;
            movingRight = false;
            isRunning = false;
            playerJumping = false;
            enemyDirection = 0;
            enemyIsRunning = false;
            enemyJumping = false;
            // Start falling if not already on ground
            if (enemyPosition.y < groundY) {
                enemyVerticalVelocity = 0.f;  // Start from current velocity
            } else {
                enemyVerticalVelocity = 0.f;  // Already on ground
            }
            if (!enemyDeadSoundPlayed && deadSound) {
                deadSound->play();
                enemyDeadSoundPlayed = true;
            }
        }
        
        // Animate dead sprite falling, then stop at last frame
        if (playerDeadAnimating && playerSprites.dead.sprite) {
            playerSprites.dead.accumulator += delta;
            if (playerSprites.dead.accumulator >= kFrameTime) {
                playerSprites.dead.accumulator = 0.f;
                if (playerSprites.dead.currentFrame < playerSprites.dead.frameCount - 1) {
                    playerSprites.dead.currentFrame++;
                    playerSprites.dead.sprite->setTextureRect(sf::IntRect(
                        sf::Vector2i{playerSprites.dead.currentFrame * playerSprites.dead.frameWidth, 0}, 
                        sf::Vector2i{playerSprites.dead.frameWidth, playerSprites.dead.frameHeight}));
                } else {
                    // Reached last frame, stop animating
                    playerDeadAnimating = false;
                }
            }
        }
        
        if (enemyDeadAnimating && enemySprites.dead.sprite) {
            enemySprites.dead.accumulator += animationDelta;
            if (enemySprites.dead.accumulator >= kFrameTime) {
                enemySprites.dead.accumulator = 0.f;
                if (enemySprites.dead.currentFrame < enemySprites.dead.frameCount - 1) {
                    enemySprites.dead.currentFrame++;
                    enemySprites.dead.sprite->setTextureRect(sf::IntRect(
                        sf::Vector2i{enemySprites.dead.currentFrame * enemySprites.dead.frameWidth, 0}, 
                        sf::Vector2i{enemySprites.dead.frameWidth, enemySprites.dead.frameHeight}));
                } else {
                    // Reached last frame, stop animating
                    enemyDeadAnimating = false;
                }
            }
        }
        
        // Dead sprites are only shown when health is 0 (handled above)
        
        const bool playerWon = enemyHealth <= 0.f;
        const bool playerLost = playerHealth <= 0.f || (timeLeft == 0 && !playerWon);
        
        // Immediately end round when someone dies - stop all actions
        if (playerWon && !winNoted) {
            context.actionHistory.push("Player victory");
            playerWins++;
            winNoted = true;
            roundEnded = true;
            roundEndClock.restart();
            // Stop all movement and attacks
            movingLeft = false;
            movingRight = false;
            isRunning = false;
            playerHitStunned = false;
            enemyHitStunned = false;
            // Don't check for 2 wins yet - let the death animation play first
        } else if (playerLost && !defeatNoted) {
            context.actionHistory.push("Player down");
            enemyWins++;
            defeatNoted = true;
            roundEnded = true;
            roundEndClock.restart();
            // Stop all movement and attacks
            movingLeft = false;
            movingRight = false;
            isRunning = false;
            playerHitStunned = false;
            enemyHitStunned = false;
            // Don't check for 2 wins yet - let the death animation play first
        }
        
        // Disable all actions when round has ended
        if (roundEnded) {
            movingLeft = false;
            movingRight = false;
            isRunning = false;
        }

        if (context.hasBackground && context.backgroundSprite) {
            window.clear();
            window.draw(*context.backgroundSprite);
        } else {
            window.clear(sf::Color(10, 10, 25));
        }

        // Draw health bars first (background layer)
        window.draw(leftHealthBack);
        window.draw(rightHealthBack);
        window.draw(leftHealthBar);
        window.draw(rightHealthBar);
        // Draw text on top
        window.draw(leftAmmoText);
        window.draw(rightAmmoText);
        window.draw(timerText);
        window.draw(actionLabel);
        // Draw bullets
        for (const auto& b : bullets) {
            if (b.active) {
                window.draw(b.sprite);
            }
        }
        
        // Draw player and enemy sprites
        if (auto* sprite = playerSprites.getCurrentSprite()) {
            window.draw(*sprite);
        }
        if (auto* sprite = enemySprites.getCurrentSprite()) {
            window.draw(*sprite);
        }

        // Win badge removed

        window.display();

        // Handle round end
        if (roundEnded) {
            if (roundEndClock.getElapsedTime().asSeconds() >= roundEndDisplayTime) {
                // After death animation has played, check if someone has won 2 rounds
                if (playerWins >= 2 || enemyWins >= 2) {
                    gameEnded = true;
                    break;  // End game immediately - someone won 2 rounds
                }
                roundEnded = false;
                currentRound++;
                // Check if we've reached max rounds
                if (currentRound > maxRounds) {
                    gameEnded = true;
                    break;
                } else {
                    // Reset for next round
                    playerHealth = 100.f;
                    enemyHealth = 100.f;
                    winNoted = false;
                    defeatNoted = false;
                    playerHitStunned = false;
                    enemyHitStunned = false;
                    playerJumping = false;
                    enemyJumping = false;
                    playerPosition = sf::Vector2f{120.f, groundY};
                    enemyPosition = sf::Vector2f{windowWidth - 250.f, groundY};  // Move enemy away from edge
                    playerSprites.setPosition(playerPosition);
                    enemySprites.setPosition(enemyPosition);
                    playerSprites.changeState(SpriteState::Walk);
                    enemySprites.changeState(SpriteState::Walk);
                    playerReloads = 2;  // Reset reloads for new round
                    reloadPlayer(true);  // Initial reload for new round (doesn't count)
                    enemyReloads = 2;  // Reset enemy reloads for new round
                    enemyAmmo = kMaxAmmo;
                    stageClock.restart();
                    playerDeadSoundPlayed = false;
                    enemyDeadSoundPlayed = false;
                }
            }
        }
    }
    
    // Stop game music when game ends (gameMusic is in scope here)
    if (gameMusicPlaying) {
        gameMusic.stop();
    }
    
    // Show final result and PlayAgain screen
    if (gameEnded) {
        // Determine winner
        bool playerWonGame = playerWins > enemyWins;
        
        // Display result text
        sf::Text resultText(context.font, playerWonGame ? "GANAS!" : "PIERDES!");
        resultText.setCharacterSize(80);
        resultText.setFillColor(playerWonGame ? sf::Color::Green : sf::Color::Red);
        resultText.setStyle(sf::Text::Bold);
        sf::FloatRect resultBounds = resultText.getLocalBounds();
        resultText.setPosition(sf::Vector2f{
            windowWidth / 2.f - resultBounds.size.x / 2.f,
            window.getSize().y / 2.f - resultBounds.size.y / 2.f
        });
        
        sf::Clock resultDisplayClock;
        const float resultDisplayTime = 3.0f;
        bool showingResult = true;
        
        while (window.isOpen() && showingResult) {
            while (auto eventOpt = window.pollEvent()) {
                const auto& event = *eventOpt;
                if (event.is<sf::Event::Closed>()) {
                    window.close();
                    return;
                }
            }
            
            if (resultDisplayClock.getElapsedTime().asSeconds() >= resultDisplayTime) {
                showingResult = false;
            }
            
            window.clear();
            if (context.hasBackground && context.backgroundSprite) {
                window.draw(*context.backgroundSprite);
            }
            window.draw(resultText);
            window.display();
        }
        
        // Show PlayAgain screen
        sf::Texture playAgainTexture;
        unique_ptr<sf::Sprite> playAgainSprite;
        if (playAgainTexture.loadFromFile("PlayAgain.png")) {
            playAgainSprite = make_unique<sf::Sprite>(playAgainTexture);
            const auto windowSize = window.getSize();
            const auto textureSize = playAgainTexture.getSize();
            const float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x);
            const float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
            playAgainSprite->setScale(sf::Vector2f{scaleX, scaleY});
        }
        
        // Load and play PlayAgain music
        sf::Music playAgainMusic;
        bool playAgainMusicPlaying = false;
        if (playAgainMusic.openFromFile("PlayAgain.mp3")) {
            playAgainMusic.setLooping(true);
            playAgainMusic.setVolume(70.f);
            playAgainMusic.play();
            playAgainMusicPlaying = true;
        }
        
        bool waitingForInput = true;
        while (window.isOpen() && waitingForInput) {
            while (auto eventOpt = window.pollEvent()) {
                const auto& event = *eventOpt;
                if (event.is<sf::Event::Closed>()) {
                    if (playAgainMusicPlaying) {
                        playAgainMusic.stop();
                    }
                    window.close();
                    return;
                }
                if (event.is<sf::Event::KeyPressed>()) {
                    const auto& keyEvent = event.getIf<sf::Event::KeyPressed>();
                    if (keyEvent) {
                        if (keyEvent->code == sf::Keyboard::Key::Enter) {
                            // Play again - stop music and restart the game
                            if (playAgainMusicPlaying) {
                                playAgainMusic.stop();
                            }
                            waitingForInput = false;
                            return; // Will restart from main
                        } else if (keyEvent->code == sf::Keyboard::Key::Escape) {
                            // Exit - stop music and play ending video
                            if (playAgainMusicPlaying) {
                                playAgainMusic.stop();
                            }
                            #ifdef __linux__
                            // Get ending video duration
                            FILE* pipe = popen("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 End/Ending.mp4 2>/dev/null", "r");
                            float endingDuration = 0.0f;
                            if (pipe) {
                                char buffer[128];
                                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                                    endingDuration = atof(buffer);
                                }
                                pclose(pipe);
                            }
                            
                            // Hide window and play ending video in fullscreen
                            window.setVisible(false);
                            if (system("which ffplay > /dev/null 2>&1") == 0 && endingDuration > 0.0f) {
                                string cmd = "ffplay -autoexit -fs -loglevel quiet End/Ending.mp4 2>/dev/null";
                                system(cmd.c_str());
                            }
                            #endif
                            window.close();
                            return;
                        }
                    }
                }
            }
            
            window.clear();
            if (playAgainSprite) {
                window.draw(*playAgainSprite);
            }
            window.display();
        }
        
        // Cleanup - stop music if still playing
        if (playAgainMusicPlaying) {
            playAgainMusic.stop();
        }
    }
}

