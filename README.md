# TCoRFix
The Chronicles of Riddick: Escape from Butcher Bay OpenGL fixes\workarounds for modern GPUs

What this thing does:
- Only returning extensions game requires, this fixes crashes (including the crash related to compressed textures on RTX cards)
- By enabling "Enable 2.0++ on AMD/ATI" in TCoRFix.ini, shader mode 2.0++ becomes selectable on AMD GPUs ~~until amd decides to break their driver~~
- Fix GL_MAX_TEXTURE_UNITS error on RTX cards (optional fix, enable "Fix Misc Bugs" in TCoRFix.ini)

## How to Install
1. Download latest [release](https://github.com/Nezarn/TCoRFix/releases)
2. Extract to ```<path-to-game>\System\Win64_AMD64\```
3. Download latest [Ultimate-ASI-Loader_x64.zip](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
4. Extract dinput8.dll to ```<path-to-game>\System\Win64_AMD64\``` and rename it to winmm.dll
5. Play

## Building from source
```
git clone https://github.com/Nezarn/TCoRFix
cd TCoRFix
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

## NOTE
This was only tested on x64 version of the game (but technically it should work with x86 too)
