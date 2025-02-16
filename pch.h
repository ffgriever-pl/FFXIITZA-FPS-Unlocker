#ifndef PCH_H
#define PCH_H

#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <Windows.h>
#include <psapi.h>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <d3dx9core.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dwmapi.h>
#include <tlhelp32.h> 

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "detours.lib")

constexpr double Rad2Deg = 0.0174532925199L;
constexpr double oneOverSixty = 0.01666666666666667L;

#endif
