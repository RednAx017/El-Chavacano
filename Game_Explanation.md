# El Chavacano – Code Explanation with File References

This document helps you **explain the game code in your video** by pointing to exact file names and line numbers.  
You can export this file to PDF from any editor (VS Code, browser, etc.).

---

## 1. Overall Structure & Main Entry Point

### Main Program Flow
**File: `ElChavacano.cpp`**  
- **Lines 12-60**: Main entry point
  - **Line 13**: Creates SFML window (960x540)
  - **Line 16**: Creates `GameContext` object (shared between scenes)
  - **Line 35-36**: Runs `IntroductionScene`
  - **Line 42-57**: Main game loop that runs:
    1. `CharacterSelectionScene` (line 43-44)
    2. `GameStage` (line 49-50)
    3. Loops back to character selection if player chooses "Play Again"

**In your video:**  
*"The main function sets up the window, creates a shared context, and runs each scene in order: intro, character selection, and the game stage."*

---

## 2. Introduction Scene

**File: `IntroductionScene.cpp`**  
- **Lines 13-135**: `IntroductionScene::run()` function
  - **Lines 19-41**: Plays intro video using `ffplay` (FFmpeg)
  - **Lines 55-63**: Loads and displays `Start.png` image
  - **Lines 80-88**: Waits for Enter key to proceed

**In your video:**  
*"The intro scene plays a video using FFmpeg's ffplay, then shows the Start screen and waits for the player to press Enter."*

---

## 3. Character Selection

**File: `CharacterSelectionScene.cpp`**  
- **Lines 35-114**: `CharacterSelectionScene::run()` function
  - **Lines 36-42**: Loads both character textures (Gangster 1 and Gangster 3)
  - **Lines 44-52**: Sets up sprites and positions
  - **Lines 82-95**: Handles keyboard input (Num1 for Gangster 1, Num3 for Gangster 3)
  - **Lines 84-86**: Stores selection in `GameContext.selectedCharacter`

**In your video:**  
*"The character selection screen displays both characters and lets the player choose with the number keys. The choice is stored in the game context."*

---

## 4. Sprite System & Animations

### AnimatedSprite Structure
**File: `GameStage.cpp`**  
- **Lines 42-93**: `AnimatedSprite` struct definition
  - **Lines 51-69**: `load()` function - loads sprite sheet and calculates frame dimensions
  - **Lines 82-92**: `update()` function - advances animation frames based on time

### CharacterSpriteManager
**File: `GameStage.cpp`**  
- **Lines 95-249**: `CharacterSpriteManager` struct definition
  - **Lines 95-100**: Contains all animation sprites (idle, walk, run, jump, shot, attack, hurt, dead)
  - **Lines 125-135**: `loadAll()` function - loads all sprite sheets for a character
  - **Lines 137-150**: `setFacingDirection()` - flips sprite horizontally
  - **Lines 152-155**: `setPosition()` - positions all sprites
  - **Lines 201-213**: `changeState()` - switches between animation states
  - **Lines 215-231**: `update()` - updates all animations
  - **Lines 233-248**: `getCurrentSprite()` - returns the currently active sprite

**In your video:**  
*"Each character has a sprite manager that holds all their animations. The manager switches between idle, walk, run, jump, shoot, attack, hurt, and dead animations based on what's happening in the game."*

---

## 5. GameStage Setup

**File: `GameStage.cpp`**  
- **Lines 254-465**: `GameStage::run()` function - Initialization
  - **Lines 255-256**: Sets window width (960) and ground Y position (300)
  - **Lines 258-264**: Creates and loads player/enemy sprite managers
  - **Lines 266-275**: Sets initial positions and scales
  - **Lines 277-292**: Loads bullet texture with transparency
  - **Lines 294-297**: Sets up UI bar positions
  - **Lines 299-340**: Creates health bars and UI text elements
  - **Lines 343-359**: Initializes ammo and reload system (2 reloads max)
  - **Lines 410-417**: Initializes round system (max 3 rounds, tracks wins)
  - **Lines 426-455**: Loads sound effects and game music

**In your video:**  
*"The game stage sets up everything: character positions, bullet textures, health bars, ammo system with 2 reloads, round tracking, and all the sound effects."*

---

## 6. Main Game Loop

**File: `GameStage.cpp`**  
- **Line 467**: Main `while (window.isOpen())` loop starts

### Event Handling
- **Lines 468-596**: Event processing
  - **Lines 470-473**: Window close event
  - **Lines 474-595**: Keyboard input handling
    - **Lines 489-494**: Left/Right arrow keys (movement)
    - **Lines 495-497**: Down arrow (running)
    - **Lines 498-504**: Up arrow (jump)
    - **Lines 506-551**: A key (shoot)
    - **Lines 552-570**: S key (melee attack)
    - **Lines 572-574**: R key (reload)

**In your video:**  
*"The main loop runs every frame. First, it handles all keyboard input: arrows for movement, A to shoot, S for melee, R to reload."*

---

## 7. Player Movement & Physics

**File: `GameStage.cpp`**  
- **Lines 666-763**: Player movement and physics
  - **Lines 667-676**: Calculates horizontal movement based on input
  - **Lines 678-693**: Boundary clamping (allows dead characters to pass through)
  - **Lines 695-704**: Updates player facing direction
  - **Lines 706-758**: Jump physics and gravity
    - **Lines 706-720**: Jump velocity and gravity application
    - **Lines 722-758**: Ground collision and position correction
  - **Lines 760-763**: Ensures player is on ground when not jumping

**In your video:**  
*"Movement is simple: left/right arrows move horizontally, up arrow adds jump velocity, and gravity pulls the character down. We always snap the character back to the ground when not jumping."*

---

## 8. Player Shooting (Bullet Creation)

**File: `GameStage.cpp`**  
- **Lines 506-551**: Player shooting code
  - **Lines 507-510**: Checks if shooting is allowed (ammo, cooldown, alive, not stunned)
  - **Line 511**: Removes one ammo from queue
  - **Lines 513-545**: Creates bullet sprite
    - **Line 518**: Sets bullet scale to 0.05 (tiny)
    - **Line 524**: Determines direction based on player facing
    - **Lines 527-541**: Positions bullet at gun tip using sprite bounds
    - **Line 543**: Sets velocity (700 units/second)
    - **Line 545**: Adds bullet to bullets vector
  - **Line 547**: Changes sprite to "Shot" state
  - **Line 549**: Records action in history

**In your video:**  
*"When the player presses A, we check if they have ammo and aren't on cooldown. Then we create a tiny bullet sprite at the gun tip, set its direction based on where the player is facing, and give it a velocity to move across the screen."*

---

## 9. Enemy AI & Shooting

**File: `GameStage.cpp`**  
- **Lines 864-1080**: Enemy AI decision making
  - **Lines 864-872**: Calculates distance to player and determines if close/mid-range
  - **Lines 874-1008**: Enemy movement AI (decides direction based on distance)
  - **Lines 1009-1015**: Enemy facing direction (always faces player)
  - **Lines 1017-1080**: Enemy attack decisions
    - **Lines 1022-1033**: Melee attack when close
    - **Lines 1034-1079**: Shooting when mid-range
      - **Lines 1041-1067**: Creates enemy bullet (same as player bullet)
      - **Line 1048**: Direction based on player position relative to enemy
      - **Line 1078**: Records "Enemy fired" in action history

**In your video:**  
*"The enemy AI checks the distance to the player. If close, it does a melee attack. If mid-range, it shoots. The enemy always faces the player and creates bullets the same way the player does."*

---

## 10. Bullet Update & Collision Detection

**File: `GameStage.cpp`**  
- **Lines 765-845**: Bullet update and collision
  - **Lines 765-778**: Moves each bullet by velocity * delta time
  - **Lines 779-783**: Removes bullets that go off-screen
  - **Lines 785-813**: Player bullet hitting enemy
    - **Lines 790-800**: Checks overlap between bullet and enemy sprite
    - **Line 801**: Reduces enemy health by 6
    - **Lines 802-804**: Applies hit stun and hurt animation
    - **Lines 805-809**: Plays gun sound
    - **Line 810**: Deactivates bullet
  - **Lines 815-840**: Enemy bullet hitting player (same logic, reduces health by 5)

**In your video:**  
*"Every frame, we move each bullet. Then we check if the bullet's bounding box overlaps with the enemy or player sprite. If it does, we reduce their health, play a hurt animation, play a sound, and remove the bullet."*

---

## 11. Ammo & Reload System

**File: `GameStage.cpp`**  
- **Lines 343-359**: Player reload system initialization
  - **Line 343**: `playerReloads = 2` (starts with 2 reloads)
  - **Lines 344-356**: `reloadPlayer()` lambda function
    - Checks if reloads are available
    - Waits for reload time
    - Refills ammo to 5
    - Decrements reload count
  - **Line 357**: Initial reload (doesn't count)
  - **Line 360**: Enemy also has 2 reloads
- **Lines 572-574**: R key triggers reload
- **Lines 847-862**: Enemy auto-reload when ammo is 0
- **Lines 1106-1107**: UI displays "Ammo: X | Reloads: Y"

**In your video:**  
*"Both characters start with 2 reloads. When you press R or the enemy runs out of ammo, they reload. Each reload refills ammo to 5 and uses up one of the 2 reloads. The UI shows both ammo count and remaining reloads."*

---

## 12. Health System & Death

**File: `GameStage.cpp`**  
- **Lines 1103-1104**: Health bar size calculation (scaled by health/100)
- **Lines 1127-1175**: Death detection and animation
  - **Lines 1127-1150**: Player death (when health <= 0)
    - Changes sprite to Dead state
    - Stops all movement
    - Plays death sound
  - **Lines 1152-1175**: Enemy death (same logic)
- **Lines 1177-1208**: Dead sprite animation (plays through frames then stops)

**In your video:**  
*"Health is stored as a float from 0 to 100. The health bar width is scaled by health divided by 100. When health reaches 0, we switch to the dead animation, stop all movement, and play a death sound."*

---

## 13. Rounds & Win Condition

**File: `GameStage.cpp`**  
- **Lines 410-417**: Round system initialization
  - `currentRound = 1`, `maxRounds = 3`
  - `playerWins = 0`, `enemyWins = 0`
- **Lines 1212-1242**: Win/loss detection
  - **Lines 1216-1228**: Player wins round (enemy health <= 0)
    - Increments `playerWins`
    - Sets `roundEnded = true`
  - **Lines 1229-1241**: Player loses round (player health <= 0 or time runs out)
    - Increments `enemyWins`
    - Sets `roundEnded = true`
- **Lines 1288-1326**: Round end handling
  - **Lines 1291-1293**: Checks if someone won 2 rounds → ends game
  - **Lines 1295-1324**: Resets for next round (health, positions, reloads)

**In your video:**  
*"The game tracks wins for each round. When someone's health hits 0, they lose that round. After the death animation plays, if someone has 2 wins, the game ends. Otherwise, we reset everything and start the next round."*

---

## 14. Action Stack (Last Action Display)

**File: `GameStage.cpp`**  
- **Line 338**: Creates `actionLabel` text element
- **Lines 1109-1119**: Updates action label each frame
  - **Line 1111**: Gets top item from `context.actionHistory` stack
  - **Line 1112**: Sets text to "Last: [action]"
  - **Lines 1116-1118**: Positions it centered under timer
- **Action pushes throughout code:**
  - **Line 503**: "Player jumped"
  - **Line 549**: "Player fired"
  - **Line 569**: "Player melee attack"
  - **Line 354**: "Reloaded ammo"
  - **Line 851**: "Enemy reloading"
  - **Line 858**: "Enemy reloaded"
  - **Line 1033**: "Enemy melee attack"
  - **Line 1078**: "Enemy fired"
  - **Line 1217**: "Player victory"
  - **Line 1230**: "Player down"

**In your video:**  
*"We keep a stack of action strings. Every important event like shooting, jumping, or reloading pushes a message onto this stack. Each frame, we display the top item as 'Last: [action]' centered under the timer."*

---

## 15. UI Updates

**File: `GameStage.cpp`**  
- **Lines 1082-1119**: UI text updates each frame
  - **Lines 1091-1101**: Timer countdown calculation and positioning
  - **Lines 1103-1104**: Health bar sizes
  - **Lines 1106-1107**: Ammo and reloads text
  - **Lines 1109-1119**: Action label (last action)
- **Lines 1251-1285**: Drawing everything
  - **Lines 1251-1256**: Background
  - **Lines 1258-1262**: Health bars
  - **Lines 1264-1267**: UI text (ammo, timer, action)
  - **Lines 1268-1273**: Bullets
  - **Lines 1275-1281**: Player and enemy sprites

**In your video:**  
*"Each frame, we update all the UI text: the timer countdown, health bar sizes, ammo and reload counts, and the last action. Then we draw everything in order: background, health bars, text, bullets, and finally the characters."*

---

## 16. Play Again Screen & Ending

**File: `GameStage.cpp`**  
- **Lines 1334-1460**: Game end and Play Again screen
  - **Lines 1335-1373**: Shows result text ("GANAS!" or "PIERDES!")
  - **Lines 1375-1385**: Loads PlayAgain.png image
  - **Lines 1387-1395**: Loads and plays PlayAgain.mp3 music
  - **Lines 1398-1454**: Waits for player input
    - **Lines 1411-1417**: Enter key → returns to character selection
    - **Lines 1418-1443**: Escape key → plays ending video and exits
      - **Lines 1423-1441**: Uses ffplay to show ending video

**In your video:**  
*"After the game ends, we show the result, then the Play Again screen with music. If the player presses Enter, we go back to character selection. If they press Escape, we play the ending video and exit."*

---

## 17. Sound Effects & Music

**File: `GameStage.cpp`**  
- **Lines 426-455**: Sound loading
  - **Lines 431-445**: Loads sound buffers (gun, tommy gun, melee hit, swing, death)
  - **Lines 448-455**: Loads and starts game music (70% volume, looping)
- **Lines 805-809**: Player gun sound on hit
- **Lines 832-836**: Enemy gun sound on hit
- **Lines 1031**: Melee hit sound
- **Lines 1146-1149**: Death sound
- **Lines 1330-1332**: Stops game music when game ends
- **Lines 1387-1395**: PlayAgain music
- **Lines 1413-1414, 1420-1421**: Stops PlayAgain music on choice

**In your video:**  
*"We load all sound effects at the start: gunshots, melee hits, death sounds. The game music plays during the match at 70% volume. When the game ends, we stop the game music and play the Play Again music."*

---

## 18. Key Code Sections Summary

For quick reference while recording your video:

1. **Main Entry**: `ElChavacano.cpp` lines 12-60
2. **Intro Scene**: `IntroductionScene.cpp` lines 13-135
3. **Character Selection**: `CharacterSelectionScene.cpp` lines 35-114
4. **Sprite System**: `GameStage.cpp` lines 42-249
5. **Game Setup**: `GameStage.cpp` lines 254-465
6. **Main Loop**: `GameStage.cpp` line 467
7. **Player Input**: `GameStage.cpp` lines 468-596
8. **Movement**: `GameStage.cpp` lines 666-763
9. **Player Shooting**: `GameStage.cpp` lines 506-551
10. **Enemy AI**: `GameStage.cpp` lines 864-1080
11. **Bullet Collision**: `GameStage.cpp` lines 765-845
12. **Ammo/Reload**: `GameStage.cpp` lines 343-359, 847-862
13. **Health/Death**: `GameStage.cpp` lines 1127-1208
14. **Rounds**: `GameStage.cpp` lines 1212-1326
15. **Action Stack**: `GameStage.cpp` lines 1109-1119
16. **UI Drawing**: `GameStage.cpp` lines 1251-1285
17. **Play Again**: `GameStage.cpp` lines 1334-1460

---

## Tips for Your Video Explanation

1. **Start with the big picture**: Show the main function flow (ElChavacano.cpp)
2. **Scene by scene**: Walk through intro → character select → game stage
3. **Game loop**: Explain the main while loop and what happens each frame
4. **Key features**: Focus on movement, shooting, collisions, rounds
5. **Use line numbers**: Reference specific lines when showing code
6. **Keep it simple**: Don't dive too deep into C++ syntax, focus on the logic

Good luck with your video! 🎮
