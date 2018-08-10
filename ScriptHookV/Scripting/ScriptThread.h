#pragma once

#include "..\ScriptHookV.h"

enum eThreadState
{
    ThreadStateRunning = 0x0,
    ThreadStateIdle    = 0x1,
	ThreadStateKilled = 0x2,
	ThreadState3 = 0x3,
	ThreadState4 = 0x4, // Sets opsToExecute to 1100000, and state to Idle in CallNative
};

struct scrThreadContext
{
    int ThreadId;
	uint32_t ScriptHash;
	eThreadState State;
	int IP;
	int FrameSP;
	int SP;
	float TimerA;
	float TimerB;
	float WaitTimer;
	int _mUnk1;
	int _mUnk2;
	int _f2C;
	int _f30;
	int _f34;
	int _f38;
	int _f3C;
	int _f40;
	int _f44;
	int _f48;
	int _f4C;
	int StackSize;
	int CatchIP;
	int CatchFrame;
	int CatchSP;
	int _set1;
	int FunctionDepth;
	int FunctionReturns[16];
};

static_assert(sizeof(scrThreadContext) == 0xA8, "");

struct scrThread
{
	void *vTable;
	scrThreadContext m_ctx;
	void *m_pStack;
	int pad0;
	int ParameterSize;
	int StaticsSize;
	int pad3;
	const char *m_pszExitMessage;
	char Name[64];
};

inline scrThread* GetActiveThread()
{
	char* moduleTls = *(char**)__readgsqword(88);
	return *reinterpret_cast<scrThread**>(moduleTls + 2096);
}

inline void SetActiveThread(scrThread* thread)
{
	char* moduleTls = *(char**)__readgsqword(88);

	*reinterpret_cast<scrThread**>(moduleTls + 2096) = thread;
}