# TCoRFix
The Chronicles of Riddick: Escape from Butcher Bay OpenGL fixes\workarounds for modern GPUs

What this thing does:
- Only returning extensions game requires, this fixes crashes (including the crash related to compressed textures on RTX cards)
- By enabling "Enable 2.0++ on any GPU" in TCoRFix.ini, shader mode 2.0++ becomes selectable on ANY GPU
- Fix GL_MAX_TEXTURE_UNITS error on RTX cards (optional fix, enable "Fix Misc Bugs" in TCoRFix.ini)
- Adds a workaround for eyeshine flickering

## How to Install
1. Download latest [release](https://github.com/Nezarn/TCoRFix/releases)
2. Extract to ```<path-to-game>\System\Win64_AMD64\``` or ```<path-to-game>\System\Win32_x86<_SSE/SSE2>```
3. Download latest [Ultimate-ASI-Loader_x64.zip or Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
4. Extract dinput8.dll to ```<path-to-game>\System\Win64_AMD64\``` or ```<path-to-game>\System\Win32_x86<_SSE/SSE2>``` and rename it to winmm.dll
5. Play

## Building from source
x64:
```
git clone https://github.com/Nezarn/TCoRFix
cd TCoRFix
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```
x86:
```
git clone https://github.com/Nezarn/TCoRFix
cd TCoRFix
mkdir build && cd build
cmake .. -A Win32
cmake --build . --config Release
```