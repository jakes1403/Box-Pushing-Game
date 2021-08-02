/*
Copyright (c) 2019-2021 Jacob Allen
See LICENSE file at root of project for license
*/

/*
Taken from Thruster Game Engine v1: A retro 2D game engine

ThrusterWindowing.cpp - This is where the windowing system is handled. Can be used in other projects as well.

*/

#include "ThrusterWindowing.h"
#include "ThrusterConfig.h"
#include <iostream>

#ifndef EMSCRIPTEN
#include "TMessageBox.hpp"
#endif

#ifdef __PSP__
#include <pspdisplay.h>
#include <pspctrl.h>
#endif


#if COMPILE_WITH_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
#include "imgui/imgui_sdl.h"
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
#include "imgui/imgui_impl_opengl3.h"
#endif
#endif

bool UpArrow = false;
bool DownArrow = false;
bool LeftArrow = false;
bool RightArrow = false;

SDL_Window* TWin_Window = NULL;

int TWin_ScreenFPS = NULL;
int TWin_ScreenWidth = NULL;
int TWin_ScreenHeight = NULL;

double TWin_CurrentFPS = NULL;

double averageFPS = 0;

bool TWin_IsAudioEnabled = NULL;
bool TWin_IsJoyStickEnabled = NULL;

bool TWin_IsFullScreenEnabled = false;

SDL_Renderer* renderer = NULL;

float startFrameTicks;

float startclock = 0;

Uint64 NOW;
Uint64 LAST;

double deltaTime;

SDL_Joystick* joyStick = NULL;

InputQuery internalInputQuery;

SDL_GLContext gameContext;

#ifdef __PSP__
SceCtrlData pad;
int lastButtons = 0;
#endif

// NON-VSYNC FOR TESTING ONLY
int TWin_Init(int ScreenFPS, int ScreenWidth, int ScreenHeight, bool EnableAudio, bool EnableJoyStick, bool linearFiltering)
{
	TWin_ScreenFPS = ScreenFPS;
	TWin_ScreenWidth = ScreenWidth;
	TWin_ScreenHeight = ScreenHeight;
	TWin_IsAudioEnabled = EnableAudio;
	TWin_IsJoyStickEnabled = EnableJoyStick;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
#ifndef EMSCRIPTEN
		ThrusterNativeDialogs::showMessageBox("SDL video could not be initialized!", "Initialization Error", ThrusterNativeDialogs::Style::Error);
#endif
		return TWin_Fail;
	}
	if (EnableAudio)
	{
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		{
#ifndef EMSCRIPTEN
			ThrusterNativeDialogs::showMessageBox("SDL audio could not be initialized!", "Initialization Error", ThrusterNativeDialogs::Style::Error);
#endif
			return TWin_Fail;
		}
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
		{
#ifndef EMSCRIPTEN
			ThrusterNativeDialogs::showMessageBox("SDL mixer could not be initialized!", "Initialization Error", ThrusterNativeDialogs::Style::Error);
#endif
			return TWin_Fail;
		}
	}
	if (EnableJoyStick)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
#ifndef EMSCRIPTEN
			ThrusterNativeDialogs::showMessageBox("SDL joystick could not be initialized!", "Initialization Error", ThrusterNativeDialogs::Style::Error);
#endif
			return TWin_Fail;
		}
		if (SDL_NumJoysticks() < 1)
		{
			printf("Warning: No joysticks connected!\n");
		}
		else
		{
			//Load joystick
			joyStick = SDL_JoystickOpen(0);
			if (joyStick == NULL)
			{
				printf("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
			}
		}
	}

	if (linearFiltering)
	{
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}
	}
	
	#ifdef __PSP__
	// Setup Pad
    sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	#endif

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;

	return 0;
}

// TODO: Fix error handling with new libs
int TWin_CreateWindow(const char* WindowName)
{
#ifndef EMSCRIPTEN
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	TWin_Window = SDL_CreateWindow(WindowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, TWin_ScreenWidth, TWin_ScreenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
#else
	TWin_Window = SDL_CreateWindow(WindowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, TWin_ScreenWidth, TWin_ScreenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
#endif
	if (TWin_Window == NULL)
	{
#ifndef EMSCRIPTEN
		ThrusterNativeDialogs::showMessageBox("SDL window could not be created!", "Initialization Error", ThrusterNativeDialogs::Style::Error);
#endif
		return TWin_Fail;
	}

#ifndef EMSCRIPTEN
	// Fix later
	SDL_SetWindowResizable(TWin_Window, SDL_TRUE);
	SDL_SetWindowMinimumSize(TWin_Window, TWin_ScreenWidth / 4, TWin_ScreenHeight / 4);
#endif

	#ifdef __PSP__
	renderer = SDL_CreateRenderer(TWin_Window, -1, SDL_RENDERER_ACCELERATED);
	#else
	Uint32 flags = SDL_RENDERER_ACCELERATED;
	if (TWin_ScreenFPS == TWin_UseVSync)
	{
		 flags |= SDL_RENDERER_PRESENTVSYNC;
	}
	
	renderer = SDL_CreateRenderer(TWin_Window, -1, flags);
	#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	gameContext = SDL_GL_CreateContext(TWin_Window);

	bool gladInitSuccess = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	if (!gladInitSuccess)
	{
		//Thruster::show("Could not initialize OpenGL loader (glad)", "Error");
		ThrusterNativeDialogs::showMessageBox("Could not initialize OpenGL loader (glad)", "Initialization Error", ThrusterNativeDialogs::Style::Error);
		return TWin_Fail;
	}

	if (SDL_GL_LoadLibrary(NULL) != 0)
	{
		ThrusterNativeDialogs::showMessageBox("Could not load OpenGL library", "Initialization Error", ThrusterNativeDialogs::Style::Error);
		return TWin_Fail;
	}

	if (TWin_ScreenFPS == TWin_UseVSync)
	{
		SDL_GL_SetSwapInterval(1); // Set to use TWin vsync options later
	}
#endif

	//backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, TWin_ScreenWidth, TWin_ScreenHeight);
	// TODO: Fix Imgui later on pi
	#if COMPILE_WITH_IMGUI
	ImGui::CreateContext();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	ImGuiSDL::Initialize(renderer, TWin_ScreenWidth, TWin_ScreenHeight);
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui_ImplSDL2_InitForOpenGL(TWin_Window, gameContext);
	const char* glsl_version = "#version 330 core";
	ImGui_ImplOpenGL3_Init(glsl_version);
#endif
	#endif

	//lineBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, TWin_ScreenWidth, TWin_ScreenHeight);
	return 0;
}

int TWin_BeginFrame(int frame)
{
	LAST = NOW;
	NOW = SDL_GetPerformanceCounter();

	deltaTime = (double)((NOW - LAST) * 1000.0f / (double)SDL_GetPerformanceFrequency());

	if (deltaTime != 0)
	{
		TWin_CurrentFPS = 1000.0f / deltaTime;
	}

	if (frame < 10 && frame != 0)
	{
		averageFPS = (averageFPS + TWin_CurrentFPS) / (frame + 1);
	}

	startFrameTicks = SDL_GetTicks();

	// Screen updating code goes here

	// Setup low-level inputs (e.g. on Win32, GetKeyboardState(), or write to those fields from your Windows message loop handlers, etc.)

	SDL_Event e;

	//Handle events on queue
    while (SDL_PollEvent(&e) != 0)
    {
		//User requests quit
		if (e.type == SDL_QUIT)
		{
			return TWin_Exit;
		}
		else if (e.type == SDL_KEYDOWN)
		{
			switch (e.key.keysym.sym)
			{
			case KEYBINDS_FULLSCREEN:
				TWin_ToggleWindowed();
				break;
			case SDLK_ESCAPE:
				return TWin_Exit;
				break;
			case SDLK_LEFT:
				internalInputQuery.buttons[BUTTON_LEFT] = true;
				break;
			case SDLK_UP:
				internalInputQuery.buttons[BUTTON_UP] = true;
				break;
			case SDLK_DOWN:
				internalInputQuery.buttons[BUTTON_DOWN] = true;
				break;
			case SDLK_RIGHT:
				internalInputQuery.buttons[BUTTON_RIGHT] = true;
				break;

			case SDLK_a:
				internalInputQuery.buttons[BUTTON_LEFT] = true;
				break;
			case SDLK_w:
				internalInputQuery.buttons[BUTTON_UP] = true;
				break;
			case SDLK_s:
				internalInputQuery.buttons[BUTTON_DOWN] = true;
				break;
			case SDLK_d:
				internalInputQuery.buttons[BUTTON_RIGHT] = true;
				break;

			case SDLK_SPACE:
				internalInputQuery.buttons[BUTTON_1] = true;
				break;
			case SDLK_z:
				internalInputQuery.buttons[BUTTON_2] = true;
				break;
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			switch (e.key.keysym.sym)
			{
			case SDLK_LEFT:
				internalInputQuery.buttons[BUTTON_LEFT] = false;
				break;
			case SDLK_UP:
				internalInputQuery.buttons[BUTTON_UP] = false;
				break;
			case SDLK_DOWN:
				internalInputQuery.buttons[BUTTON_DOWN] = false;
				break;
			case SDLK_RIGHT:
				internalInputQuery.buttons[BUTTON_RIGHT] = false;
				break;

			case SDLK_a:
				internalInputQuery.buttons[BUTTON_LEFT] = false;
				break;
			case SDLK_w:
				internalInputQuery.buttons[BUTTON_UP] = false;
				break;
			case SDLK_s:
				internalInputQuery.buttons[BUTTON_DOWN] = false;
				break;
			case SDLK_d:
				internalInputQuery.buttons[BUTTON_RIGHT] = false;
				break;

			case SDLK_SPACE:
				internalInputQuery.buttons[BUTTON_1] = false;
				break;
			case SDLK_z:
				internalInputQuery.buttons[BUTTON_2] = false;
				break;
			}
		}
		else if (e.type == SDL_JOYBUTTONDOWN)
		{

		}
		else if (e.type == SDL_MOUSEBUTTONDOWN)
		{

		}

		int mouseX, mouseY;
		const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

		internalInputQuery.pointerDown = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);

		#if COMPILE_WITH_IMGUI
		ImGuiIO& io = ImGui::GetIO();
        

        io.DeltaTime = 1.0f / 60.0f;
        io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
        io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
        io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
        // TODO: Fix mouse wheel
        //io.MouseWheel = static_cast<float>(0);

        io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
        io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
        io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
        io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);

        ImGui_ImplSDL2_ProcessEvent(&e);

        int w, h;
        int display_w, display_h;
        SDL_GetWindowSize(TWin_Window, &w, &h);
        if (SDL_GetWindowFlags(TWin_Window) & SDL_WINDOW_MINIMIZED)
            w = h = 0;
        SDL_GL_GetDrawableSize(TWin_Window, &display_w, &display_h);
        io.DisplaySize = ImVec2((float)w, (float)h);
        if (w > 0 && h > 0)
            io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
		#endif
    }

	#ifdef __PSP__
	sceCtrlPeekBufferPositive(&pad, 1);

	internalInputQuery.buttons[BUTTON_UP] = pad.Buttons & PSP_CTRL_UP;
	internalInputQuery.buttons[BUTTON_DOWN] = pad.Buttons & PSP_CTRL_DOWN;
	internalInputQuery.buttons[BUTTON_LEFT] = pad.Buttons & PSP_CTRL_LEFT;
	internalInputQuery.buttons[BUTTON_RIGHT] = pad.Buttons & PSP_CTRL_RIGHT;

	internalInputQuery.buttons[BUTTON_1] = pad.Buttons & PSP_CTRL_CROSS;
	internalInputQuery.buttons[BUTTON_2] = pad.Buttons & PSP_CTRL_CIRCLE;
	internalInputQuery.buttons[BUTTON_3] = pad.Buttons & PSP_CTRL_SQUARE;
	internalInputQuery.buttons[BUTTON_4] = pad.Buttons & PSP_CTRL_TRIANGLE;

	internalInputQuery.axis[Axis1X] = pad.Lx;
	internalInputQuery.axis[Axis1Y] = pad.Ly;


	#endif

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

	SDL_RenderClear(renderer);

	#if COMPILE_WITH_IMGUI
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(TWin_Window);
#endif
	
	ImGui::NewFrame();
	#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

	return TWin_Good;
}

int TWin_EndFrame()
{
	#if COMPILE_WITH_IMGUI
	ImGui::Render();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	ImGuiSDL::Render(ImGui::GetDrawData());
#endif
	#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	SDL_RenderPresent(renderer);
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	SDL_GL_SwapWindow(TWin_Window);
#endif
	float frameTicks = SDL_GetTicks() - startFrameTicks;
	#ifdef __PSP__
	if ((TWin_ScreenFPS != TWin_NoFPSLimit) && (TWin_ScreenFPS != TWin_UseVSync))
	{
		if (frameTicks < 1000 / TWin_ScreenFPS)
		{
			//Wait remaining time
			SDL_Delay((1000 / TWin_ScreenFPS) - frameTicks);
		}
	}
	else if (TWin_ScreenFPS == TWin_UseVSync)
	{
		sceDisplayWaitVblankStart();
	}
	#else
	if ((TWin_ScreenFPS != TWin_NoFPSLimit) && (TWin_ScreenFPS != TWin_UseVSync))
	{
		if (frameTicks < 1000 / TWin_ScreenFPS)
		{
			//Wait remaining time
			SDL_Delay((1000 / TWin_ScreenFPS) - frameTicks);
		}
	}
	#endif
	return TWin_Good;
}

int TWin_DestroyWindow()
{
	#if COMPILE_WITH_IMGUI
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	ImGuiSDL::Deinitialize();
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	ImGui_ImplOpenGL3_Shutdown();
#endif
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	#endif

#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	SDL_GL_DeleteContext(gameContext);
#endif
	//TLN_Deinit();

	SDL_DestroyRenderer(renderer);

	//Destroy window
	SDL_DestroyWindow(TWin_Window);

	Mix_Quit();
	SDL_Quit();

	return 0;
}

int TWin_ToggleWindowed()
{
#ifndef EMSCRIPTEN
	//Grab the mouse so that we don't end up with unexpected movement when the dimensions/position of the window changes.
	TWin_IsFullScreenEnabled = !TWin_IsFullScreenEnabled;
	if (!TWin_IsFullScreenEnabled)
	{
		//SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_SetWindowFullscreen(TWin_Window, 0);
	}
	else
	{
		//SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_SetWindowFullscreen(TWin_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
#endif
	return 0;
}

InputQuery GetInput()
{
	int x_move, y_move;
	x_move = SDL_JoystickGetAxis(joyStick, 0);
	y_move = SDL_JoystickGetAxis(joyStick, 1);
	
	internalInputQuery.axis[Axis1X] = float(x_move / 32767.0f);
	internalInputQuery.axis[Axis1Y] = float((y_move / 32767.0f) * -1);

	SDL_GetMouseState(&internalInputQuery.pointerX, &internalInputQuery.pointerY);

	return internalInputQuery;
}