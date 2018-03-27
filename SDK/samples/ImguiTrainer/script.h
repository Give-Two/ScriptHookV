/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2015
*/

#pragma once

#include "..\..\inc\natives.h"
#include "..\..\inc\types.h"
#include "..\..\inc\enums.h"

#include "..\..\inc\main.h"

#include <windows.h>

#include <string>
#include <ctime>
#include <vector>
#include <iostream>
#include <functional>
#include <deque>

#include <DXGI.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>

void ScriptMain();
void PresentHook(void *swapChain);
void OnWndProcMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

extern std::deque<std::function<void()>> g_nativeQueue;

#define NQ_ g_nativeQueue.push_back([&] {
#define _NQ });