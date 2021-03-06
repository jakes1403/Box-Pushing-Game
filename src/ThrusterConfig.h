/*
Copyright (c) 2019-2021 Jacob Allen
See LICENSE file at root of project for license
*/

#define G_IMPL_SDL2 0
#define G_IMPL_OPENGL2 1
#define G_IMPL_OPENGL3 2
#define G_IMPL_PSP 3
#define MODE_EDITOR 0
#define MODE_GAME 1
#define TYPE_RELEASE 0
#define TYPE_DEBUG 1

// Config here

#define THRUSTER_MODE MODE_GAME

#ifdef BUILD_GAME_DEBUG
#define TBUILD_TYPE TYPE_DEBUG
#endif
#ifdef BUILD_GAME_RELEASE
#define TBUILD_TYPE TYPE_RELEASE
#endif

#ifdef BUILD_GAME_SDL2
#define GRAPHICS_IMPLEMENTATION G_IMPL_SDL2
#endif
#ifdef BUILD_GAME_OPENGL3
#define GRAPHICS_IMPLEMENTATION G_IMPL_OPENGL3
#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
#define PLATFORM_NO_FRAMEBUFFER
#endif

#define KEYBINDS_FULLSCREEN SDLK_F11

#define TFLOAT_DATATYPE float