<img src="resource/logo.png" width="827" height="372"/>

The source code to the gameplay and engine behind Box Pushing Game.

# About
Box Pushing Game was a project I took on the side to learn C++ and how to make game engines. As a side effect the code is really bad. Feel free to do something about it :D

Most of this code is years old and there are private versions of this repo with way better code, I just havent gone around to putting them out and documenting them yet.

It is a game with a very straightforward title where you push boxes into holes. Nothing more to it than that. Its got decent art, shader effects, and custom lighting. I haven't gotten around to music but then again this is a very old project.

| Screenshots |
|:--:| 
| ![SixkQb](https://user-images.githubusercontent.com/45643741/131200536-caf8ca1b-dded-46da-affa-7da7581f0288.png) |
| ![MQN6ca](https://user-images.githubusercontent.com/45643741/131200522-4cc65c6c-67a6-4aec-aff4-5e56177fc8b0.png) |

## Supported Platforms
* Microsoft Windows 64 bit
* Linux (not buildable yet but soon)
* Sony PSP (Yes, the one from *2005*. Only works in an emulator currently)
* HTML5 (Runs with emscripten)

# About the "Engine"
Thruster (the name for the driver code of the game) is a library I created to make the creation of this game easier.

It works more as a software framework instead of an engine, and enables cross platform support easily

I wrote almost all of that code myself, and I was very new to C++ at the time so I am sure its trash.

# Compiling

Make sure CMake, SDL2, and SDl2-mixer are installed
If using the OpenGL build, make sure your system is capable of running OpenGL

```
git clone https://github.com/jakes1403/Box-Pushing-Game.git
```

```
git submodule update --init --recursive
```

```
mkdir build
cd build
```

If you are building on windows:

```
cmake -DSDL2MAIN_LIBRARY="PATH_TO_SDL2main.lib_HERE" -DSDL2_LIBRARY="PATH_TO_SDL2.lib_HERE" -DSDL2_INCLUDE_DIR="PATH_TO_SDL2_HEADERS_HERE" -DSDL2_MIXER_LIBRARY="PATH_TO_SDL2_mixer.lib_HERE" -DSDL2_MIXER_INCLUDE_DIR="PATH_TO_SDL2_MIXER_HEADERS_HERE" ..
```

Otherwise:

```
cmake ..
```

```
cmake --build . --config Release
```

*Copyright (C) 2019-2021 Jacob Allen*
