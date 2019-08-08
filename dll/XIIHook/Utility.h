#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "Vector3.h"
#include "Input.h"

void Print(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

struct gameVars
{
	double cTime = 0;
	volatile double* targetFrameTime = (double*)minFrameTimePtr;
	volatile double* targetFrameTime1 = (double*)minFrameTime1Ptr;
	volatile double* realFrameTime = (double*)realFrameTimePtr;
	volatile float* timeScale = (float*)timeScalePtr;
	volatile float* framerateCoef = (float*)framerateCoefPtr;
	volatile float* worldTime = (float*)worldMPtr;
	volatile float* inGameMultiplier = (float*)igmPtr;
	volatile float* inGameMouseMultiplier = (float*)mouseCoefPtr;
	volatile float* gamma = (float*)gammaPtr;
	volatile float* fov = (float*)fovPtr;
	volatile float* animDummy = (float*)animDummyPtr;
	volatile uint8_t* titleState = (uint8_t*)inTitlePtr;
	volatile uint8_t* uiState = (uint8_t*)uiEnabledPtr;
	volatile uint8_t* cMenState = (uint8_t*)inCombatMenPtr;
	volatile uint8_t* inMovieState = (uint8_t*)inMoviePtr;
	volatile uint8_t* inCutscene = (uint8_t*)inCScenePtr;
	volatile uint8_t* igmState = (uint8_t*)igmStatePtr;
	volatile uint8_t* freeCamEnabled = (uint8_t*)freeCamEnabledPtr;
	volatile Vector3f* cameraPosition = (Vector3f*)cameraPositionPtr;
	volatile Vector3f* cameraLookAtPoint = (Vector3f*)cameraLookAtPointPtr;
	volatile HWND FFXIIWND;

	uint8_t gameStateEnum = 0, lastigm = 0, focusState = 0, lastFocusState = 0, lastUseMenuLimitState = 0;
	
	Interp::Interp igmInterp = Interp::Interp();
	UserConfig uConfig = UserConfig();
	InputManager IM;

	gameVars()
	{
		Config::UpdateUserConfig(uConfig);
		igmInterp.setType(uConfig.easingType);
		igmInterp.position = 1; igmInterp.position0 = 1; igmInterp.target = 1;
		focusState = 0; lastFocusState = 0; cTime = 0; gameStateEnum = 0; lastigm = 0; lastUseMenuLimitState = 0;

		Print("Normalizing config...\n");
		*framerateCoef = 30 / uConfig.requestedMinFramerate;
		uConfig.requestedMinFramerate = 1 / uConfig.requestedMinFramerate;
		uConfig.requestedMinFramerateMenus = 1 / uConfig.requestedMinFramerateMenus;
		uConfig.requestedMinFramerateNoFocus = 1 / uConfig.requestedMinFramerateNoFocus;
		uConfig.requestedMinFramerateMovies = 1 / uConfig.requestedMinFramerateMovies;
		uConfig.fov = uConfig.fov * Rad2Deg;
		Print("Config done... \n");
	}

};



// Clean up SetConsoleTextAttribute ... printf; shorthand; inline for optimization
__forceinline void SetConTAttribute(HANDLE h, WORD w, const char *fmt, ...)
{
	SetConsoleTextAttribute(h, w);
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

__forceinline float getIGM(gameVars& gVars, bool current) {
	switch (current) {
	case true:
		return	*gVars.igmState == 0 ? gVars.uConfig.igmState0Override
			:	*gVars.igmState == 1 ? gVars.uConfig.igmState1Override 
			:	gVars.uConfig.igmState2Override;
		break;

	case false:
		return	gVars.lastigm == 0 ? gVars.uConfig.igmState0Override
			:	gVars.lastigm == 1 ? gVars.uConfig.igmState1Override
			:	gVars.uConfig.igmState2Override;
		break;
	}
}

void tick_interp(gameVars& gVars)
{
	float nTime = (float)gVars.cTime;

	if (!gVars.gameStateEnum == 1 && !*gVars.inCutscene == 1 && gVars.lastigm != *gVars.igmState)
	{
		float base0 = getIGM(gVars, false);
		float base = getIGM(gVars, true);

		gVars.igmInterp.position = base0;
		gVars.igmInterp.position0 = base0;
		gVars.igmInterp.smoothTime = gVars.uConfig.easingTime;
		gVars.igmInterp.target = base;
		gVars.igmInterp.time0 = nTime;

		Print("Starting igm interp: %f -> %f\n", base0, base);
	}
	else if (!gVars.gameStateEnum == 1 && !*gVars.inCutscene == 1)
	{
		float base = 1;
		switch (gVars.uConfig.bEnableEasing)
		{
		case true:
			base = gVars.igmInterp.interp(nTime);

			if (nTime > gVars.igmInterp.time1) base = getIGM(gVars, true);
			if (base != base || base == 0) base = 1; //If NaN set to 1
			break;
		case false:
			base = getIGM(gVars, true);
			break;
		}

		*gVars.inGameMultiplier = base;
	}
	else *gVars.inGameMultiplier = 1; //Attempt to fix menu and loading slowdowns, as well as cutscene scaling
}


void tickf(gameVars & gVars)
{
	tick_interp(gVars);

	if (*gVars.worldTime > *gVars.inGameMultiplier * 2)
		*gVars.worldTime = *gVars.inGameMultiplier;

	gVars.lastigm = *gVars.igmState;

	*gVars.gamma = gVars.uConfig.gamma;
	*gVars.fov = gVars.uConfig.fov;
}


void updateGState(gameVars & gVars) {
	uint8_t inCutscene, uiState, cMenState, inMovieState;

	inCutscene = *gVars.inCutscene;
	uiState = *gVars.uiState;
	cMenState = *gVars.cMenState;
	inMovieState = *gVars.inMovieState;

	gVars.focusState = gVars.FFXIIWND == GetForegroundWindow() ? 0 : 1;
	
	gVars.gameStateEnum 
		= inCutscene == 1
		? 4 : inMovieState == 1 && uiState == 0
		? 3 : gVars.focusState == 1
		? 2 : (uiState == 1 || cMenState == 1) //TITLESTATE BREAKS REKS (Must find a more accurate value)
		? 1
		: 0;
}

void updateFPSCoef(gameVars& gVars) 
{
	switch (gVars.gameStateEnum)
	{
	case 0:
		//printf("User has exited a menu!\n");
		*gVars.targetFrameTime = gVars.uConfig.requestedMinFramerate;
		*gVars.targetFrameTime1 = gVars.uConfig.requestedMinFramerate;
		break;
	case 1:
		//printf("User has entered a menu!\n");
		*gVars.targetFrameTime = gVars.uConfig.requestedMinFramerateMenus;
		*gVars.targetFrameTime1 = gVars.uConfig.requestedMinFramerateMenus;
		break;
	case 2:
		//printf("Game window has lost focus!\n");
		*gVars.targetFrameTime = gVars.uConfig.requestedMinFramerateNoFocus;
		*gVars.targetFrameTime1 = gVars.uConfig.requestedMinFramerateNoFocus;
		break;
	case 3:
		//printf("User has entered a movie!\n");
		*gVars.targetFrameTime = gVars.uConfig.requestedMinFramerateMovies;
		*gVars.targetFrameTime1 = gVars.uConfig.requestedMinFramerateMovies;
		break;
	case 4:
		//printf("The user is in a cutscene; reducing framerate!\n");
		*gVars.targetFrameTime = gVars.uConfig.requestedMinFramerateMovies;
		*gVars.targetFrameTime1 = gVars.uConfig.requestedMinFramerateMovies;
		break;
	}
	gVars.lastUseMenuLimitState = gVars.gameStateEnum;
}

void updateMouse(gameVars& gVars) {
	*gVars.inGameMouseMultiplier = gVars.uConfig.bEnableAdaptiveMouse ?
		(gVars.uConfig.lockedMouseMulti * *gVars.inGameMultiplier) / *gVars.timeScale
		: gVars.uConfig.lockedMouseMulti;
}

void Step(gameVars& gVars) {
	//Fix page permissions
	DWORD protection;
	VirtualProtect((LPVOID)minFrameTimePtr, sizeof(double), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)minFrameTime1Ptr, sizeof(double), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)timeScalePtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)framerateCoefPtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)worldMPtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)igmPtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)mouseCoefPtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)gammaPtr, sizeof(float), PAGE_READWRITE, &protection);
	VirtualProtect((LPVOID)fovPtr, sizeof(float), PAGE_READWRITE, &protection);

	updateGState(gVars);
	updateFPSCoef(gVars);
	updateMouse(gVars);
	gVars.cTime = ((double)clock() / CLOCKS_PER_SEC);
	tickf(gVars);
	PollInput(gVars.IM);
}




#endif



