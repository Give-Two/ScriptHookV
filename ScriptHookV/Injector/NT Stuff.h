#pragma once

#ifndef NT_STUFF_H
#define NT_STUFF_H

#include <Windows.h>

struct UNICODE_STRING
{
	WORD		Length;
	WORD		MaxLength;
	wchar_t *	szBuffer;
};

struct LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY		InLoadOrder;
	LIST_ENTRY		InMemoryOrder;
	LIST_ENTRY		InInitOrder;
	void *			DllBase;
	void *			EntryPoint;
	ULONG			SizeOfImage;
	UNICODE_STRING	FullDllName;
	UNICODE_STRING	BaseDllName;
};

struct PEB_LDR_DATA
{
	ULONG		Length;
	BYTE		Initialized;
	HANDLE		SsHandle;
	LIST_ENTRY	InLoadOrderModuleListHead;
	LIST_ENTRY	InMemoryOrderModuleListHead;
	LIST_ENTRY	InInitializationOrderModuleListHead;
	void *		EntryInProgress;
	BYTE		ShutdownInProgress;
	HANDLE		ShutdownThreadId;
};

struct PEB
{
	void * Reserved[3];
	PEB_LDR_DATA * Ldr;
};

struct PROCESS_BASIC_INFORMATION
{
	NTSTATUS	ExitStatus;
	PEB *		pPEB;
	ULONG_PTR	AffinityMask;
	LONG		BasePriority;
	HANDLE		UniqueProcessId;
	HANDLE		InheritedFromUniqueProcessId;
};

enum _PROCESSINFOCLASS
{
	ProcessBasicInformation
};
typedef _PROCESSINFOCLASS PROCESSINFOCLASS;

typedef NTSTATUS(__stdcall * f_NtCreateThreadEx)(HANDLE * pHandle, ACCESS_MASK DesiredAccess, void * pAttr, HANDLE hProc, void * pFunc, void * pArg,
	ULONG Flags, SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaxStackSize, void * pAttrListOut);

typedef NTSTATUS(__stdcall * f_LdrLoadDll)(wchar_t * szOptPath, ULONG ulFlags, UNICODE_STRING * pModuleFileName, HANDLE * pOut);

typedef NTSTATUS(__stdcall * f_NtQueryInformationProcess)(HANDLE hProc, PROCESSINFOCLASS PIC, void * pBuffer, ULONG BufferSize, ULONG * SizeOut);

#endif