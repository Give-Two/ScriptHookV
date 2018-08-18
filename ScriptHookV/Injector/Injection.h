#pragma once

#ifndef INJECTION_H
#define INJECTION_H

#include <Windows.h>
#include <fstream>
#include <TlHelp32.h>
#include <Psapi.h>
#include "NT Stuff.h"
#include <array>

enum INJECTION_MODE
{
	IM_LoadLibrary,
	IM_LdrLoadDll,
	IM_ManualMap,
};

#define INJ_ERASE_HEADER 1
#define INJ_FAKE_HEADER 2
#define INJ_UNLINK_FROM_PEB 4
#define INJ_FLAGS_ALL (INJ_ERASE_HEADER | INJ_FAKE_HEADER | INJ_UNLINK_FROM_PEB)

HMODULE GetModuleInProcess(HANDLE hProc, const char* moduleName);
DWORD InjectDLL(const char * szDllFile, HANDLE hProc, INJECTION_MODE Mode, bool HijackThread = false, DWORD Postinjection = 0, DWORD * ErrorCode = nullptr);

#define InjectionErrors					\
X( INJ_ERR_SUCCESS )					\
X( INJ_ERR_INVALID_PROC_HANDLE )		\
X( INJ_ERR_FILE_DOESNT_EXIST )			\
X( INJ_ERR_OUT_OF_MEMORY )				\
X( INJ_ERR_INVALID_FILE	)				\
X( INJ_ERR_NO_X64FILE )					\
X( INJ_ERR_NO_X86FILE )					\
X( INJ_ERR_IMAGE_CANT_RELOC )			\
X( INJ_ERR_NTDLL_MISSING )				\
X( INJ_ERR_LDRLOADDLL_MISSING )			\
X( INJ_ERR_LDRPLOADDLL_MISSING )		\
X( INJ_ERR_INVALID_FLAGS )				\
X( INJ_ERR_CANT_FIND_MOD )				\
X( INJ_ERR_CANT_FIND_MOD_PEB )			\
X( INJ_ERR_UNKNOWN )					\
X( INJ_ERR_CANT_CREATE_THREAD )			\
X( INJ_ERR_CANT_ALLOC_MEM )				\
X( INJ_ERR_WPM_FAIL )					\
X( INJ_ERR_TH32_FAIL )					\
X( INJ_ERR_CANT_GET_PEB )				\
X( INJ_ERR_ALREADY_INJ )

#define X(x) x,
enum eInjectionErrors { InjectionErrors InjectionErrorCount };
#undef X

#define X(x) #x,    
constexpr std::array<char * const, eInjectionErrors::InjectionErrorCount> eInjectionErrorNames = { InjectionErrors };
#undef X

#define InjectionAdvErrors				\
X( INJ_ERR_ADV_UNKNOWN )				\
X( INJ_ERR_ADV_INV_PROC )				\
X( INJ_ERR_ADV_TH32_FAIL )				\
X( INJ_ERR_ADV_NO_THREADS )				\
X( INJ_ERR_ADV_CANT_OPEN_THREAD	)		\
X( INJ_ERR_ADV_SUSPEND_FAIL )			\
X( INJ_ERR_ADV_GET_CONTEXT_FAIL )		\
X( INJ_ERR_ADV_OUT_OF_MEMORY )			\
X( INJ_ERR_ADV_WPM_FAIL )				\
X( INJ_ERR_ADV_SET_CONTEXT_FAIL )		\
X( INJ_ERR_ADV_RESUME_FAIL )			\
X( INJ_ERR_ADV_QIP_MISSING )			\
X( INJ_ERR_ADV_QIP_FAIL )

#define X(x) x,
enum eInjectionAdvErrors { InjectionAdvErrors InjectionAdvErrorCount };
#undef X

#define X(x) #x,    
constexpr std::array<char * const, eInjectionAdvErrors::InjectionAdvErrorCount> eInjectionAdvErrorNames = { InjectionAdvErrors };
#undef X

#endif