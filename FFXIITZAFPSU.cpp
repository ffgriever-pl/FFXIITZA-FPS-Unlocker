// FFXIITZAFPSU.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Config.h"
#include "Interp.h"
//#include "out/detours/detours.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <ctime>
#include <cstdlib>
//#include <d3d11.h>

#define NOP 0x90 //NOP instruction

//Variable pointers
#define tftPtr 0x0090D398 //Min Frametime 1
#define tft1Ptr 0x0090E0C0 //Min Frametime 2 (Doesn't seem to do anything)
#define rftPtr 0x01F97058  //Reported frametime 1
#define rft1Ptr 0x01F98170 //Reported frametime 2 (Not to be trusted)
#define igmPtr 0x0207D798 /*
In game multiplier
Generally becomes 1,2,4 though you can make it anything. Disabling instructions related to this can have some weird results.
*/
#define worldMPtr 0x0207D794 /*
Wolrd multiplier (Eg skies and grass anims) @ < 1 this freezes the game, but there is still camera control
This is equal to the igm * the global time multiplier
*/
#define mouseMPtr 0x01E1A820 /*
Mouse ~~multiplier~~ Min angle coef
Realative to all other multipliers
*/
#define tScalePtr 0x0207D79C /*
Global time multiplier
When I found this everything else made so much more sense, this is what we will be changing
*/
#define framerateScalePtr 0x01E160BC /*
1 at 30 fps, 0.5 at 60
This should fix everything.
*/
#define igmSPtr 0x01FED754 /*
Currently selected state of igmptr
*/
#define uiOPtr 0x01E160C0 //Menu enabled
#define titlPtr 0x020A3380 //0 In game 1 In title
//Instruction pointers
#define cMenPtr 0x01FED3A4 /*
Combat menu pointer.
*/
#define cScenePtr 0x020AB454 /*
In cutscene pointer, is 1 when in a cutscene or at the title.
*/
#define moviePtr 0x022C0C51 /*
In Move pointer, is 1 when in a movie.
*/
#define gammaPtr 0x00900150 /*
In game gamma
*/
#define mouseFPtr 0x01F111E0 /*
Chaning this does some weird stuff to the input
*/
#define fovPtr 0x020B0258 /*
The in game FOV. This does not effect cutscenes (They use different camera)
*/
#define gammaPtr 0x00900150 /*
The in game gamma. 
*/
#define shadowTypePtr 0x0090FF10 /*
One byte, 0-6 (modulus) changes quality of shadow.
*/

#define igmUnlockPtr 0x0022AC27 /*
First overwrite to fully control the igm*/
#define igmUnlockPtr0 0x0022AC1D /*
Second overwrite to fully control the igm*/
#define igmUnlockPtr1 0x0022AC1D /*
Third overwrite to fully control the igm*/
#define igmUnlockPtr2 0x04556E00
#define igmUnlockPtr3 0x0022AC3A
#define mouseUnlockPtr 0x0022AC58 /*
Instruction writes to the mouse multi.*/
#define mouseUnlockPtr0 0x04551428 /*
Instruction writes to the mouse multi.
*/
#define fullSpeedUnlockPtr 0x00622274 /*
Erasing this results in completely unlocked framerate*/
#define fovUnlockPtr 0x06E63EE0 /*
Fixes fullscreen problem
*/

#define D3D11EndScene 0x7FFAD2985070 /*DX11*/

HWND FindFFXII() {
	return FindWindow(0, L"FINAL FANTASY XII THE ZODIAC AGE");
}

__forceinline void writeMinFrametime(HANDLE&hProcess, double&targetFrameTime) 
{
	DWORD protection;
	VirtualProtectEx(hProcess, (LPVOID)tftPtr, sizeof(double), PAGE_READWRITE, &protection);
	VirtualProtectEx(hProcess, (LPVOID)tft1Ptr, sizeof(double), PAGE_READWRITE, &protection);

	WriteProcessMemory(hProcess, (LPVOID)tftPtr, &targetFrameTime, sizeof(double), NULL);
	WriteProcessMemory(hProcess, (LPVOID)tft1Ptr, &targetFrameTime, sizeof(double), NULL);
}

__forceinline void tickf(HANDLE&hProcess, double&cTime, float&worldTime, float&inGameMouseMultiplier, float&inGameMultiplier, uint8_t&useMenuLimit, uint8_t&inCutscene, uint8_t&igmState, uint8_t&lastigm, Interp::Interp&igmInterp, UserConfig&uConfig)
{
	float nTime = (float)cTime;
	if (!useMenuLimit == 1 && !inCutscene == 1 && lastigm != igmState)
	{
		float base0 = (lastigm == 0 ? uConfig.igmState0Override
			: (lastigm == 1 ? uConfig.igmState1Override : uConfig.igmState2Override));
		float base = (igmState == 0 ? uConfig.igmState0Override
			: (igmState == 1 ? uConfig.igmState1Override : uConfig.igmState2Override));

		igmInterp.position = base0;
		igmInterp.position0 = base0;
		igmInterp.smoothTime = uConfig.easingTime;
		igmInterp.target = base;
		igmInterp.time0 = nTime;

		std::cout << "Starting igm interp: " << base << " " << base0 << "\n";		
	}
	else if (!useMenuLimit == 1 && !inCutscene == 1) 
	{
		float base = 1;
		switch (uConfig.bEnableEasing) 
		{
		case true:
			base = igmInterp.interp(nTime);

			if (nTime > igmInterp.time1) base = (igmState == 0 ? uConfig.igmState0Override
				: (igmState == 1 ? uConfig.igmState1Override : uConfig.igmState2Override));
			if (base != base || base == 0) base = 1; //If NaN set to 1
			break;
		case false:
			base = (igmState == 0 ? uConfig.igmState0Override
				: (igmState == 1 ? uConfig.igmState1Override : uConfig.igmState2Override));
			break;
		}

		inGameMultiplier = base;
	}
	else inGameMultiplier = 1; //Attempt to fix menu and loading slowdowns, as well as cutscene scaling

	uint8_t wtChanged = 0;
	if (worldTime > inGameMultiplier * 2)
	{
		worldTime = inGameMultiplier;
		wtChanged = 1;
	}

	WriteProcessMemory(hProcess, (LPVOID)igmPtr, &inGameMultiplier, sizeof(float), NULL);
	WriteProcessMemory(hProcess, (LPVOID)mouseMPtr, &inGameMouseMultiplier, sizeof(float), NULL);
	if (wtChanged==1) WriteProcessMemory(hProcess, (LPVOID)worldMPtr, &worldTime, sizeof(float), NULL);

	lastigm = igmState;
}

int main()
{
	//Hack stuff
	HWND FFXIIWND = FindFFXII();
	HANDLE hProcess;
	DWORD pId;

	//Prefabricated vars
	const double oneOverSixty = (double)1L / (double)60L;
	const double Rad2Deg = 0.0174532925199L;

	//Ingame vars
	double defaultTargetFrameTime = 0;
	double targetFrameTime = 0;
	double realFrametime = 0;
	float timeScale = 1;
	float framerateScale = 1;
	float inGameMouseMultiplier = 1;
	float inGameMultiplier = 1;
	float worldTime = 1;
	uint8_t igmState = 0;
	uint8_t titleState = 1;
	uint8_t uiState = 0;
	uint8_t cMenState = 0;
	uint8_t focusState = 0;
	uint8_t lastUseMenuLimitState = 0;
	uint8_t lastFocusState = 0;
	uint8_t lastigm = 0;
	uint8_t inMovie = 0;
	uint8_t inCutscene = 0;
	uint8_t useMenuLimit = 0;

	//Config stuff
	UserConfig uConfig;
	Config::UpdateUserConfig(uConfig);

	//Interp stuff
	Interp::Interp igmInterp;
	igmInterp.setType(uConfig.easingType);
	igmInterp.position = 1; igmInterp.position0 = 1; igmInterp.target = 1;

	//Do math once
	std::cout << "Normalizing config variables...\n";

	framerateScale = 30 / uConfig.requestedMinFramerate;
	uConfig.requestedMinFramerate = 1 / uConfig.requestedMinFramerate;
	uConfig.requestedMinFramerateMenus = 1 / uConfig.requestedMinFramerateMenus;
	uConfig.requestedMinFramerateNoFocus = 1 / uConfig.requestedMinFramerateNoFocus;
	uConfig.requestedMinFramerateMovies = 1 / uConfig.requestedMinFramerateMovies;
	uConfig.fov = uConfig.fov * Rad2Deg;
	defaultTargetFrameTime = uConfig.requestedMinFramerate;

	while (!FFXIIWND)
	{
		std::cout << "Could not find FFXII, make sure the game is open!\nChecking again...\n";
		FFXIIWND = FindFFXII();
		Sleep(1000);
	}

	GetWindowThreadProcessId(FFXIIWND, &pId);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
	std::cout << "Found FFXII Window at PID: " << pId << "!\n";


	if (!hProcess) std::cout << "Failed to open process...\n";
	else 
	{
		std::cout << "Target frame time=" << uConfig.requestedMinFramerate << "\n";
		std::cout << "Setting up... \n";

		//Overwriting instructions
		if (uConfig.bEnableLockedMouseMulti || uConfig.bEnableAdaptiveMouse) 
		{
			if (!WriteProcessMemory(hProcess, (LPVOID)mouseUnlockPtr, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
				std::cout << "Failed to erase instructions... \n";
			if (!WriteProcessMemory(hProcess, (LPVOID)mouseUnlockPtr0, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
				std::cout << "Failed to erase instructions... \n";
		}
		if (!WriteProcessMemory(hProcess, (LPVOID)igmUnlockPtr, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (!WriteProcessMemory(hProcess, (LPVOID)igmUnlockPtr0, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (!WriteProcessMemory(hProcess, (LPVOID)igmUnlockPtr1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (!WriteProcessMemory(hProcess, (LPVOID)igmUnlockPtr2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (!WriteProcessMemory(hProcess, (LPVOID)igmUnlockPtr3, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (!WriteProcessMemory(hProcess, (LPVOID)fovUnlockPtr, "\x90\x90\x90\x90\x90\x90\x90", 7, NULL))
			std::cout << "Failed to erase instructions... \n";
		if (uConfig.bEnableFullSpeedMode) 
			if (!WriteProcessMemory(hProcess, (LPVOID)fullSpeedUnlockPtr, "\x90\x90\x90\x90\x90\x90\x90\x90", 8, NULL))
				std::cout << "Failed to erase instructions... \n";


		
		//Setting default frametimes, loop until this works
		while (targetFrameTime != uConfig.requestedMinFramerate)
		{
			writeMinFrametime(hProcess, uConfig.requestedMinFramerate);
			ReadProcessMemory(hProcess, (LPVOID)tftPtr, (LPVOID)&targetFrameTime, sizeof(double), NULL);
		}

		std::cout << "Setting desired Gamma and FOV...\n";
		DWORD protection;
		VirtualProtectEx(hProcess, (LPVOID)gammaPtr, sizeof(float), PAGE_READWRITE, &protection);
		VirtualProtectEx(hProcess, (LPVOID)fovPtr, sizeof(float), PAGE_READWRITE, &protection);


		WriteProcessMemory(hProcess, (LPVOID)gammaPtr, &uConfig.gamma, sizeof(float), NULL);
		WriteProcessMemory(hProcess, (LPVOID)fovPtr, &uConfig.fov, sizeof(float), NULL);

		clock_t tick = clock() / CLOCKS_PER_SEC;

		for (;;)
		{
			if (!IsWindow(FFXIIWND)) 
			{
				std::cout << "Window closed, stopping...\n";
				break;
			}

			ReadProcessMemory(hProcess, (LPVOID)rftPtr, (LPVOID)&realFrametime, sizeof(double), NULL);
			ReadProcessMemory(hProcess, (LPVOID)worldMPtr, (LPVOID)&worldTime, sizeof(float), NULL);
			ReadProcessMemory(hProcess, (LPVOID)igmSPtr, (LPVOID)&igmState, sizeof(uint8_t), NULL);
			ReadProcessMemory(hProcess, (LPVOID)uiOPtr, (LPVOID)&uiState, sizeof(uint8_t), NULL);
			ReadProcessMemory(hProcess, (LPVOID)titlPtr, (LPVOID)&titleState, sizeof(uint8_t), NULL);
			ReadProcessMemory(hProcess, (LPVOID)cMenPtr, (LPVOID)&cMenState, sizeof(uint8_t), NULL);
			ReadProcessMemory(hProcess, (LPVOID)cScenePtr, (LPVOID)&inCutscene, sizeof(uint8_t), NULL);
			ReadProcessMemory(hProcess, (LPVOID)moviePtr, (LPVOID)&inMovie, sizeof(uint8_t), NULL);
			
			focusState = FFXIIWND != GetForegroundWindow();

			//Faster than previous method; should cause less bugs
			useMenuLimit =
				inCutscene == 1
				? 4 : inMovie == 1 && uiState == 0
				? 3 : focusState == 1 
				? 2 : (uiState == 1 || cMenState == 1) //(uiState == 1 || titleState == 1 || cMenState == 1) TITLESTATE BREAKS REKS (Must find a more accurate value)
				? 1 
				: 0; 

			if (useMenuLimit != lastUseMenuLimitState) 
			{
				switch (useMenuLimit) 
				{
				case 0:
					std::cout << "User has exited a menu!\n";
					targetFrameTime = uConfig.requestedMinFramerate;
					break;
				case 1:
					std::cout << "User has entered a menu!\n";
					targetFrameTime = uConfig.requestedMinFramerateMenus;
					break;
				case 2:
					std::cout << "Game window has lost focus!\n";
					targetFrameTime = uConfig.requestedMinFramerateNoFocus;
					break;
				case 3:
					std::cout << "User has entered a movie!\n";
					targetFrameTime = uConfig.requestedMinFramerateMovies;
					break;
				case 4:
					std::cout << "The user is in a cutscene; reducing framerate!\n";
					targetFrameTime = uConfig.requestedMinFramerateMovies;
					break;
				}
				writeMinFrametime(hProcess, targetFrameTime);
				lastUseMenuLimitState = useMenuLimit; //Only write back to the frametime when needed
			}

			timeScale = realFrametime / targetFrameTime;
			inGameMouseMultiplier = uConfig.bEnableAdaptiveMouse ?
				(uConfig.lockedMouseMulti * inGameMultiplier) / timeScale
				: uConfig.lockedMouseMulti;

			WriteProcessMemory(hProcess, (LPVOID)framerateScalePtr, &framerateScale, sizeof(float), NULL);
			WriteProcessMemory(hProcess, (LPVOID)tScalePtr, &timeScale, sizeof(float), NULL);

			double cTime = ((double)clock() / CLOCKS_PER_SEC);
			if (cTime - tick >= (realFrametime))
			{ 
				tickf(hProcess, cTime, worldTime, inGameMouseMultiplier, inGameMultiplier, useMenuLimit, inCutscene, igmState, lastigm, igmInterp, uConfig);
				tick = cTime;
			}

			//IN MS NOT SECONDS
			Sleep(1000 * (realFrametime / mainThreadUpdateCoef)); /*Improve CPU time
									  Also accounts for framerate fluctuations, with an effort to update x times per frame.*/
		}
	}

	CloseHandle(hProcess);
	std::cin.get();
	return 0;
}
