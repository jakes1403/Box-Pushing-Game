/*
Copyright (c) 2019-2021 Jacob Allen
See LICENSE file at root of project for license
*/

#pragma once

#include "ThrusterConfig.h"

#ifdef __LINUX__
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif

#ifdef _WIN32
#include <SDL.h>
#include <SDL_mixer.h>
#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
#include "glad\glad.h"
#endif

#ifdef __PSP__
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif

#define TWin_Exit -1

#define TWin_Fail -2
#define TWin_Good 0

#define TWin_NoFPSLimit 0
#define TWin_UseVSync -1

#define BUTTON_UP 0 // Hat up
#define BUTTON_DOWN 1 // Hat down
#define BUTTON_LEFT 2 // Hat Left
#define BUTTON_RIGHT 3 // Hat Right

#define BUTTON_1 4 // A or Cross
#define BUTTON_2 5 // B or Circle
#define BUTTON_3 6 // X or Square
#define BUTTON_4 7 // Y or Triangle

#define controlSize 8

#define Axis1X 0
#define Axis1Y 1

#define axisSize 2

#define KEY_SPACE 0
//#define KEY_! 1
//#define KEY_" 2
//#define KEY_# 3
#define KEY_$ 4
//#define KEY_% 5
//#define KEY_& 6
//#define KEY_' 7
//#define KEY_( 8
//#define KEY_) 9
//#define KEY_* 10
//#define KEY_+ 11
//#define KEY_, 12
//#define KEY_- 13
//#define KEY_. 14
//#define KEY_/ 15
#define KEY_0 16
#define KEY_1 17
#define KEY_2 18
#define KEY_3 19
#define KEY_4 20
#define KEY_5 21
#define KEY_6 22
#define KEY_7 23
#define KEY_8 24
#define KEY_9 25
//#define KEY_: 26
//#define KEY_; 27
//#define KEY_< 28
//#define KEY_= 29
//#define KEY_> 30
//#define KEY_? 31
//#define KEY_@ 32
#define KEY_A 33
#define KEY_B 34
#define KEY_C 35
#define KEY_D 36
#define KEY_E 37
#define KEY_F 38
#define KEY_G 39
#define KEY_H 40
#define KEY_I 41
#define KEY_J 42
#define KEY_K 43
#define KEY_L 44
#define KEY_M 45
#define KEY_N 46
#define KEY_O 47
#define KEY_P 48
#define KEY_Q 49
#define KEY_R 50
#define KEY_S 51
#define KEY_T 52
#define KEY_U 53
#define KEY_V 54
#define KEY_W 55
#define KEY_X 56
#define KEY_Y 57
#define KEY_Z 58
//#define KEY_[ 59
//#define KEY_\ 60
//#define KEY_] 61
//#define KEY_^ 62
#define KEY__ 63
//#define KEY_` 64
#define KEY_a 65
#define KEY_b 66
#define KEY_c 67
#define KEY_d 68
#define KEY_e 69
#define KEY_f 70
#define KEY_g 71
#define KEY_h 72
#define KEY_i 73
#define KEY_j 74
#define KEY_k 75
#define KEY_l 76
#define KEY_m 77
#define KEY_n 78
#define KEY_o 79
#define KEY_p 80
#define KEY_q 81
#define KEY_r 82
#define KEY_s 83
#define KEY_t 84
#define KEY_u 85
#define KEY_v 86
#define KEY_w 87
#define KEY_x 88
#define KEY_y 89
#define KEY_z 90
//#define KEY_{ 91
//#define KEY_| 92
//#define KEY_} 93
//#define KEY_~ 94

extern float axis[axisSize];


extern bool UpArrow;
extern bool DownArrow;
extern bool LeftArrow;
extern bool RightArrow;

extern double deltaTime;

extern SDL_Window* TWin_Window;

extern int TWin_ScreenFPS;
extern int TWin_ScreenWidth;
extern int TWin_ScreenHeight;

extern double TWin_CurrentFPS;

extern bool TWin_IsAudioEnabled;
extern bool TWin_IsJoyStickEnabled;

extern bool TWin_IsFullScreenEnabled;

extern SDL_Renderer* renderer;

extern int TWin_Init(int ScreenFPS, int ScreenWidth, int ScreenHeight, bool EnableAudio, bool EnableJoyStick, bool linearFiltering);

extern int TWin_CreateWindow(const char* WindowName);

extern int TWin_BeginFrame(int frame);

extern int TWin_EndFrame();

extern int TWin_DestroyWindow();

extern int TWin_ToggleWindowed();

struct InputQuery
{
	bool buttons[controlSize];
	float axis[axisSize];

	int pointerX;
	int pointerY;

	bool pointerDown;
};

extern InputQuery GetInput();
