#pragma once

#include "..\ScriptHookV.h"

enum eThreadState
{
	ThreadStateIdle = 0x0,
	ThreadStateRunning = 0x1,
	ThreadStateKilled = 0x2,
	ThreadState3 = 0x3,
	ThreadState4 = 0x4, // Sets opsToExecute to 1100000, and state to Idle in CallNative
};

struct scrThreadContext
{
    int ThreadId;
    unsigned int ScriptHash;
    eThreadState State;
    int padC[39];
};

static_assert(sizeof(scrThreadContext) == 0xA8, "");

struct scrThread
{
	void *vTable;
	scrThreadContext ThreadContext;
	//ScriptArgument *pStack;
	void *pStack;
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