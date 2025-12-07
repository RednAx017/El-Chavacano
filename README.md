# El Chavacano 🎮

A 2D fighting game built with SFML featuring gangster characters, shooting mechanics, and round-based combat.

## 🎯 Features

- **Character Selection**: Choose between two gangster characters
- **Combat System**: Shooting, melee attacks, and reload mechanics
- **Round-Based**: First to 2 wins takes the match
- **Animated Sprites**: Smooth animations for all character actions
- **Sound Effects**: Immersive audio for actions and combat
- **Video Integration**: Intro and ending videos

## 🎮 Controls

- **Arrow Keys**: Move (Left/Right)
- **Down Arrow**: Run
- **Up Arrow**: Jump
- **A**: Shoot
- **S**: Melee Attack
- **R**: Reload
- **Enter**: Start/Continue
- **Escape**: Exit

## 📥 Download

### Windows (.exe)

Download the latest Windows executable from [Releases](https://github.com/RednAx017/El-Chavacano/releases).

The release package includes:
- ✅ `ElChavacano.exe` - The game executable
- ✅ All required DLLs
- ✅ All game assets (images, sounds, videos)
- ✅ README with instructions

**Just extract and run!**

### Building from Source

#### Linux
```bash
sudo apt-get install libsfml-dev
./build_linux.sh
```

#### Windows (Cross-compile from Linux)
```bash
sudo apt-get install mingw-w64
./build_windows_exe.sh
./package_windows_release.sh
```

## 🛠️ Requirements

- **SFML 2.6+** (for building)
- **C++17 compiler** (g++ or clang++)
- **FFmpeg** (for video playback, optional)

## 📁 Project Structure


El-Chavacano/
├── ElChavacano.cpp          # Main entry point
├── GameStage.cpp            # Core gameplay logic
├── IntroductionScene.cpp    # Intro video and start screen
├── CharacterSelectionScene.cpp  # Character selection
├── AssetPaths.hpp           # Asset file paths
├── GameContext.hpp          # Shared game context
└── .github/workflows/       # GitHub Actions for auto-build


## 🚀 Automatic Builds

This repository uses GitHub Actions to automatically build Windows executables when you:
- Push a tag (e.g., `v1.0`)
- Create a release
- Manually trigger the workflow

The built `.exe` will be automatically attached to releases!


## 🎨 Assets

Game assets include:
- Character sprite sheets (Gangster 1 & 3)
- Background images
- Sound effects (gunshots, melee, death)
- Background music
- Intro and ending videos

## 📄 License

This project uses assets from Craftpix.net. Please refer to their license terms.

## 🤝 Contributing

Feel free to fork, submit issues, or create pull requests!

---

**Enjoy the game!** 🎮

