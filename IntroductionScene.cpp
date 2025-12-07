#include "IntroductionScene.hpp"

#include <SFML/Audio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace std;

void IntroductionScene::run(sf::RenderWindow& window, GameContext& context) {
    // Play intro video first
    bool videoPlaying = false;
    bool videoFinished = false;
    float videoDuration = 0.0f;
    
    #ifdef __linux__
    // Try to get video duration using ffprobe
    FILE* pipe = popen("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 Intro/Intro.mp4 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            videoDuration = atof(buffer);
        }
        pclose(pipe);
    }
    
    // Check if ffplay is available
    if (system("which ffplay > /dev/null 2>&1") == 0 && videoDuration > 0.0f) {
        // Start video playback in fullscreen mode
        // Hide the game window temporarily while video plays
        window.setVisible(false);
        string cmd = "ffplay -autoexit -fs -loglevel quiet Intro/Intro.mp4 2>/dev/null";
        system(cmd.c_str());
        window.setVisible(true);
        videoPlaying = true;
        videoFinished = true; // Video has finished playing
    }
    #endif
    
    sf::Clock videoClock;
    bool showingVideo = videoPlaying;
    
    sf::Texture texture;
    unique_ptr<sf::Sprite> sprite;
    bool waitingForEnter = false;
    
    // Load music (will play after video)
    sf::Music music;
    bool musicPlaying = false;
    
    // Load Start.png
    if (texture.loadFromFile("Intro/Start.png")) {
        sprite = make_unique<sf::Sprite>(texture);
        // Scale to fit window
        const auto windowSize = window.getSize();
        const auto textureSize = texture.getSize();
        const float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x);
        const float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
        sprite->setScale(sf::Vector2f{scaleX, scaleY});
    }

    while (window.isOpen()) {
        // Handle events
        while (auto eventOpt = window.pollEvent()) {
            const auto& event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                if (musicPlaying) {
                    music.stop();
                }
                window.close();
                return;
            }
            
            // Handle ENTER key press (only after video finishes)
            if (event.is<sf::Event::KeyPressed>()) {
                const auto& keyEvent = event.getIf<sf::Event::KeyPressed>();
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (waitingForEnter && !showingVideo) {
                        // ENTER pressed - stop music and proceed
                        if (musicPlaying) {
                            music.stop();
                        }
                        context.actionHistory.push("Intro finished");
                        return;
                    }
                }
            }
        }

        // Check if video has finished
        if (videoPlaying && videoFinished) {
            // Video has finished, show Start.png
            if (!waitingForEnter) {
                showingVideo = false;
                // Now start music and show Start.png
                if (music.openFromFile("Intro/GodfatherTheme.mp3")) {
                    music.setLooping(true);
                    music.play();
                    musicPlaying = true;
                }
                waitingForEnter = true;
            }
        } else if (!videoPlaying || videoDuration <= 0.0f) {
            // If video player not available or duration couldn't be determined, skip directly to Start.png
            if (!videoFinished) {
                showingVideo = false;
                videoFinished = true;
                if (music.openFromFile("Intro/GodfatherTheme.mp3")) {
                    music.setLooping(true);
                    music.play();
                    musicPlaying = true;
                }
                waitingForEnter = true;
            }
        }

        // Render
        window.clear();
        if (showingVideo) {
            // Show black screen while video plays (video is in fullscreen)
            // The video player handles its own display
        } else if (sprite && waitingForEnter) {
            window.draw(*sprite);
        }
        window.display();
    }
    
    // Cleanup
    if (musicPlaying) {
        music.stop();
    }
}


