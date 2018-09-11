#pragma once

#include <windows.h>
#include <string>

#define IMPORT __declspec(dllimport)

/*Input*/
typedef void(*TWndProcFn)(UINT, WPARAM, LPARAM);
IMPORT void WndProcHandlerRegister(TWndProcFn handler);
IMPORT void WndProcHandlerUnregister(TWndProcFn handler);

/* keyboard */
typedef void(*KeyboardHandler)(DWORD, WORD, BYTE, BOOL, BOOL, BOOL, BOOL);
IMPORT void keyboardHandlerRegister(KeyboardHandler handler);
IMPORT void keyboardHandlerUnregister(KeyboardHandler handler);

/* D3d SwapChain */
typedef void(*PresentCallback)(void *);
IMPORT void presentCallbackRegister(PresentCallback cb);
IMPORT void presentCallbackUnregister(PresentCallback cb);

/* textures */
IMPORT int createTexture(char *texFileName);
IMPORT void drawTexture(int id, int index, int level, int time,
	float sizeX, float sizeY, float centerX, float centerY,
	float posX, float posY, float rotation, float screenHeightScaleFactor,
	float r, float g, float b, float a);

/* scripts */
IMPORT void changeScriptThread(UINT32 hash);
IMPORT void scriptWait(DWORD time);
IMPORT void scriptRegister(HMODULE module, void(*LP_SCRIPT_MAIN)());
IMPORT void scriptRegisterAdditionalThread(HMODULE module, void(*LP_SCRIPT_MAIN)());
IMPORT void scriptUnregister(HMODULE module);
IMPORT void scriptUnregister(void(*LP_SCRIPT_MAIN)()); // deprecated
IMPORT void nativeInit(UINT64 hash);
IMPORT void nativePush64(UINT64 val);
IMPORT PUINT64 nativeCall();

/* Functions */
static void WAIT(DWORD time) { scriptWait(time); }
static void TERMINATE() { WAIT(MAXDWORD); }

/* global variables */
IMPORT UINT64 *getGlobalPtr(int globalId);

/* World Pools */
IMPORT int worldGetAllVehicles(int *arr, int arrSize);
IMPORT int worldGetAllPeds(int *arr, int arrSize);
IMPORT int worldGetAllObjects(int *arr, int arrSize);
IMPORT int worldGetAllPickups(int *arr, int arrSize);

/* game version */
IMPORT int getGameVersion();

/* misc */
IMPORT BYTE *getScriptHandleBaseAddress(int handle);
IMPORT int registerRawStreamingFile(const std::string& fileName, const std::string& registerAs);