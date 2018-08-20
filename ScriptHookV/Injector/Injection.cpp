#include "Injection.h"

#pragma comment (lib, "Psapi.lib")

#ifdef ReCa
#undef ReCa
#endif
#define ReCa reinterpret_cast

#ifdef UNICODE
#undef Module32First
#undef Module32Next
#undef MODULEENTRY32
#endif

typedef HINSTANCE(__stdcall * f_LoadLibrary)(const char*);
typedef uintptr_t(__stdcall * f_GetProcAddress)(HINSTANCE, const char*);
typedef BOOL(WINAPI * f_DLL_ENTRY_POINT)(void*, DWORD, void*);

DWORD LastError = 0;

struct LDR_LOAD_DLL_DATA
{
	f_LdrLoadDll	pLdrLoadDll;
	HANDLE			Out;
	UNICODE_STRING	pModuleFileName;
	BYTE			Data[MAX_PATH * 2];
};

struct MANUAL_MAPPING_DATA
{
	f_LoadLibrary		pLoadLibrary;
	f_GetProcAddress	pGetProcAddress;
};

DWORD Inject(const char * szDllFile, HANDLE hProc, bool HijackThread);
DWORD ManualMap(const char * szDllFile, HANDLE hProc, bool HijackThread);
DWORD LdrLoadDllStub(const char * szDllFile, HANDLE hProc, bool HijackThread);
DWORD Cloaking(const char * szDllFile, HANDLE hProc, DWORD Flags);

UINT __forceinline _strlenA(const char * szString);
void __forceinline _ZeroMemory(BYTE * pMem, UINT Size);
void __stdcall ImportTlsExecute(MANUAL_MAPPING_DATA * pData);
void __stdcall LdrLoadDllShell(LDR_LOAD_DLL_DATA * pData);

bool FileExistsA(const char * szFile);
HANDLE StartRoutine(HANDLE hTargetProc, void * pRoutine, void * pArg, bool Hijack = false, bool Fastcall = true);
PEB * GetPEB(HANDLE hProc);

HMODULE GetModuleInProcess(HANDLE hProc, const char* moduleName)
{
	HMODULE	hModBuf[0xFF];
	DWORD	bN;
	UINT	i;
	HMODULE	r = nullptr;

	if (!K32EnumProcessModulesEx(hProc, hModBuf, sizeof(hModBuf), &bN, LIST_MODULES_64BIT))
		return r;

	for (i = 0; i < (bN / sizeof(HMODULE)); i++)
	{
		TCHAR szPath[MAX_PATH];
		if (!K32GetModuleFileNameExA(hProc, hModBuf[i], szPath, sizeof(szPath) / sizeof(TCHAR)))
			continue;
		std::string szName = szPath;
		if (szName.find(moduleName) != std::string::npos)
		{
			r = hModBuf[i];
			break;
		}
	}
	return r;
}

DWORD InjectDLL(const char * szDllFile, HANDLE hProc, INJECTION_MODE im, bool HijackThread, DWORD Postinjection, DWORD * ErrorCode)
{
	DWORD Ret = 0;

	switch (im)
	{
		case IM_LoadLibrary:
			Ret = Inject(szDllFile, hProc, HijackThread);
			break;

		case IM_LdrLoadDll:
			Ret = LdrLoadDllStub(szDllFile, hProc, HijackThread);
			break;

		case IM_ManualMap:
			return ManualMap(szDllFile, hProc, HijackThread);
	}

	if (!Ret)
		Ret = Cloaking(szDllFile, hProc, Postinjection);

	if (ErrorCode)
		*ErrorCode = LastError;

	return Ret;
}

DWORD Inject(const char * szDllFile, HANDLE hProc, bool HijackThread)
{
	if (!hProc)
		return INJ_ERR_INVALID_PROC_HANDLE;
	if (!szDllFile || !FileExistsA(szDllFile))
		return INJ_ERR_FILE_DOESNT_EXIST;
	// Check if our dll is already injected
	if (GetModuleInProcess(hProc, szDllFile)) 
		return INJ_ERR_ALREADY_INJ;

	auto Len = lstrlenA(szDllFile);

	void * pArg = VirtualAllocEx(hProc, nullptr, Len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pArg)
		return INJ_ERR_OUT_OF_MEMORY;

	if (!WriteProcessMemory(hProc, pArg, szDllFile, Len, nullptr))
	{
		LastError = GetLastError();
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);
		return INJ_ERR_UNKNOWN;
	}

	DWORD dwExitCode = 0;
	HANDLE hThread = StartRoutine(hProc, LoadLibraryA, pArg, HijackThread, false);
	if (!hThread)
	{
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);
		return INJ_ERR_CANT_CREATE_THREAD;
	}
	else if (!HijackThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwExitCode);
		CloseHandle(hThread);
	}
	else
		dwExitCode = 1;

	VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);

	if (!dwExitCode)
		return INJ_ERR_UNKNOWN;

	return INJ_ERR_SUCCESS;
}

DWORD ManualMap(const char * szDllFile, HANDLE hProc, bool HijackThread)
{
	if (!hProc)
		return INJ_ERR_INVALID_PROC_HANDLE;
	if (!szDllFile || !FileExistsA(szDllFile))
		return INJ_ERR_FILE_DOESNT_EXIST;

	BYTE *					pSrcData;
	IMAGE_NT_HEADERS *		pOldNtHeader;
	IMAGE_OPTIONAL_HEADER * pOldOptHeader;
	IMAGE_FILE_HEADER *		pOldFileHeader;
	BYTE *					pLocalBase;
	BYTE *					pTargetBase;

	std::ifstream File(szDllFile, std::ios::binary | std::ios::ate);

	auto FileSize = File.tellg();
	if (FileSize <= 0x1000)
	{
		File.close();
		return INJ_ERR_INVALID_FILE;
	}

	pSrcData = new BYTE[static_cast<UINT_PTR>(FileSize)];

	if (!pSrcData)
	{
		File.close();
		return INJ_ERR_OUT_OF_MEMORY;
	}

	File.seekg(0, std::ios::beg);
	File.read(ReCa<char*>(pSrcData), FileSize);
	File.close();

	if (ReCa<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D)
	{
		delete[] pSrcData;
		return INJ_ERR_INVALID_FILE;
	}

	pOldNtHeader	= ReCa<IMAGE_NT_HEADERS*>(pSrcData + ReCa<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
	pOldOptHeader	= &pOldNtHeader->OptionalHeader;
	pOldFileHeader	= &pOldNtHeader->FileHeader;

	#ifdef _WIN64
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
	{
		delete[] pSrcData;
		return INJ_ERR_NO_X64FILE;
	}
	#else
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
	{
		delete[] pSrcData;
		return INJ_ERR_NO_X86FILE;
	}
	#endif

	pTargetBase = ReCa<BYTE*>(VirtualAllocEx(hProc, ReCa<void*>(pOldOptHeader->ImageBase), pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if (!pTargetBase)
		pTargetBase = ReCa<BYTE*>(VirtualAllocEx(hProc, nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	if (!pTargetBase)
	{
		LastError = GetLastError();
		delete[] pSrcData;
		return INJ_ERR_CANT_ALLOC_MEM;
	}
	
	pLocalBase = ReCa<BYTE*>(VirtualAlloc(nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	if (!pLocalBase)
	{
		delete[] pSrcData;
		VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
		return INJ_ERR_OUT_OF_MEMORY;
	}

	memset(pLocalBase, 0, pOldOptHeader->SizeOfImage);
	memcpy(pLocalBase, pSrcData, 0x1000);
	
	auto * pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
	for (UINT i = 0; i < pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
		if (pSectionHeader->SizeOfRawData)
			memcpy(pLocalBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData);

	BYTE * LocationDelta = pTargetBase - pOldOptHeader->ImageBase;
	if (LocationDelta)
	{
		if (!pOldOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		{
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			VirtualFree(pLocalBase, 0, MEM_RELEASE);
			delete[] pSrcData;
			return INJ_ERR_IMAGE_CANT_RELOC;
		}

		auto * pRelocData = ReCa<IMAGE_BASE_RELOCATION*>(pLocalBase + pOldOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		while (pRelocData->VirtualAddress)
		{
			WORD * pRelativeInfo = ReCa<WORD*>(pRelocData + 1);
			for (UINT i = 0; i < ((pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2); ++i, ++pRelativeInfo)
			{
				#ifndef _WIN64
				if ((*pRelativeInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
				{
					DWORD * pPatch = ReCa<DWORD*>(pLocalBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
					*pPatch += ReCa<DWORD>(LocationDelta);
				}
				#else
				if ((*pRelativeInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)
				{
					UINT_PTR * pPatch = ReCa<UINT_PTR*>(pLocalBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
					*pPatch += ReCa<UINT_PTR>(LocationDelta);
				}
				#endif
			}
			pRelocData = ReCa<IMAGE_BASE_RELOCATION*>(ReCa<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
		}
	}

	ReCa<MANUAL_MAPPING_DATA*>(pLocalBase)->pLoadLibrary	= LoadLibraryA;
	ReCa<MANUAL_MAPPING_DATA*>(pLocalBase)->pGetProcAddress = ReCa<f_GetProcAddress>(GetProcAddress);

	BOOL Ret = WriteProcessMemory(hProc, pTargetBase, pLocalBase, pOldOptHeader->SizeOfImage, nullptr);
	if (!Ret)
	{
		LastError = GetLastError();
		VirtualFree(pLocalBase, 0, MEM_RELEASE);
		delete[] pSrcData;
		return INJ_ERR_WPM_FAIL;
	}

	VirtualFree(pLocalBase, 0, MEM_RELEASE);
	delete[] pSrcData;

	ULONG_PTR FuncSize = 0x600;
	void * pFunc = VirtualAllocEx(hProc, nullptr, FuncSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pFunc)
	{
		LastError = GetLastError();
		return INJ_ERR_CANT_ALLOC_MEM;
	}

	if (!WriteProcessMemory(hProc, pFunc, ImportTlsExecute, FuncSize, nullptr))
	{
		LastError = GetLastError();
		VirtualFreeEx(hProc, pFunc, 0, MEM_RELEASE);
		return INJ_ERR_WPM_FAIL;
	}

	HANDLE hThread = StartRoutine(hProc, pFunc, pTargetBase, HijackThread, false);

	if (!hThread)
	{
		VirtualFreeEx(hProc, pFunc, 0, MEM_RELEASE);
		VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
		return INJ_ERR_CANT_CREATE_THREAD;
	}
	else if (!HijackThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}

	VirtualFreeEx(hProc, pFunc, 0, MEM_RELEASE);

	return INJ_ERR_SUCCESS;
}

DWORD LdrLoadDllStub(const char * szDllFile, HANDLE hProc, bool HijackThread)
{
	if (!hProc)
		return INJ_ERR_INVALID_PROC_HANDLE;
	if (!szDllFile || !FileExistsA(szDllFile))
		return INJ_ERR_FILE_DOESNT_EXIST;

	LDR_LOAD_DLL_DATA data{ 0 };
	data.pModuleFileName.szBuffer = ReCa<wchar_t*>(data.Data);
	data.pModuleFileName.MaxLength = MAX_PATH * 2;

	size_t len = _strlenA(szDllFile);
	mbstowcs_s(&len, data.pModuleFileName.szBuffer, len + 1, szDllFile, len);
	data.pModuleFileName.Length = (WORD)(len * 2) - 2;

	HINSTANCE hNTDLL = GetModuleHandleA("ntdll.dll");
	if (!hNTDLL)
		return INJ_ERR_NTDLL_MISSING;

	FARPROC pFunc = GetProcAddress(hNTDLL, "LdrLoadDll");
	if (!pFunc)
		return INJ_ERR_LDRLOADDLL_MISSING;

	data.pLdrLoadDll = ReCa<f_LdrLoadDll>(pFunc);

	BYTE * pArg = ReCa<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(LDR_LOAD_DLL_DATA) + 0x200, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if (!pArg)
	{
		LastError = GetLastError();
		return INJ_ERR_CANT_ALLOC_MEM;
	}

	if (!WriteProcessMemory(hProc, pArg, &data, sizeof(LDR_LOAD_DLL_DATA), nullptr))
	{
		LastError = GetLastError();
		VirtualFreeEx(hProc, pArg, MEM_RELEASE, 0);
		return INJ_ERR_WPM_FAIL;
	}

	if (!WriteProcessMemory(hProc,pArg + sizeof(LDR_LOAD_DLL_DATA), LdrLoadDllShell, 0x100, nullptr))
	{
		LastError = GetLastError();
		VirtualFreeEx(hProc, pArg, MEM_RELEASE, 0);
		return INJ_ERR_WPM_FAIL;
	}

	HANDLE hThread = StartRoutine(hProc, pArg + sizeof(LDR_LOAD_DLL_DATA), pArg, HijackThread, false);
	if (!hThread)
	{
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);
		return INJ_ERR_CANT_CREATE_THREAD;
	}
	else if (!HijackThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}

	VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);

	return INJ_ERR_SUCCESS;
}

DWORD Cloaking(const char * szDllFile, HANDLE hProc, DWORD Flags)
{
	if (!Flags)
		return INJ_ERR_SUCCESS;

	if (Flags > INJ_FLAGS_ALL)
		return INJ_ERR_INVALID_FLAGS;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProc));
	if (!hSnap)
	{
		LastError = GetLastError();
		return  INJ_ERR_TH32_FAIL;
	}

	MODULEENTRY32 ME32{ 0 };
	ME32.dwSize = sizeof(MODULEENTRY32);

	HINSTANCE hMod = 0;

	BOOL Ret = Module32First(hSnap, &ME32);
	while (Ret)
	{
		char Buffer[MAX_PATH]{ 0 };
		GetModuleFileNameExA(hProc, ReCa<HINSTANCE>(ME32.modBaseAddr), Buffer, MAX_PATH);
		if (Buffer[3] == szDllFile[3] && !lstrcmpA(szDllFile, Buffer))
		{
			hMod = ME32.hModule;
			break;
		}
		Ret = Module32Next(hSnap, &ME32);
	}
	CloseHandle(hSnap);

	if (!Ret || !hMod)
		return INJ_ERR_CANT_FIND_MOD;

	if (Flags & INJ_FAKE_HEADER)
	{
		void * pK32 = ReCa<void*>(GetModuleHandleA("kernel32.dll"));
		DWORD dwOld = 0;
		VirtualProtectEx(hProc, hMod, 0x1000, PAGE_EXECUTE_READWRITE, &dwOld);
		WriteProcessMemory(hProc, hMod, pK32, 0x1000, nullptr);
		VirtualProtectEx(hProc, hMod, 0x1000, dwOld, &dwOld);
	}
	else if (Flags & INJ_ERASE_HEADER)
	{
		BYTE Buffer[0x1000]{ 0 };
		DWORD dwOld = 0;
		VirtualProtectEx(hProc, hMod, 0x1000, PAGE_EXECUTE_READWRITE, &dwOld);
		WriteProcessMemory(hProc, hMod, Buffer, 0x1000, nullptr);
	}

	if (Flags & INJ_UNLINK_FROM_PEB)
	{
		PEB * ppeb;
		ppeb = GetPEB(hProc);
		if (!ppeb)
			return INJ_ERR_CANT_GET_PEB;

		PEB	peb;
		if (!ReadProcessMemory(hProc, ppeb, &peb, sizeof(PEB), nullptr))
			return INJ_ERR_CANT_GET_PEB;

		PEB_LDR_DATA ldrdata;
		if (!ReadProcessMemory(hProc, peb.Ldr, &ldrdata, sizeof(PEB_LDR_DATA), nullptr))
			return INJ_ERR_CANT_GET_PEB;

		LDR_DATA_TABLE_ENTRY * pCurrentEntry	= reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(ldrdata.InLoadOrderModuleListHead.Flink);
		LDR_DATA_TABLE_ENTRY * pLastEntry		= reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(ldrdata.InLoadOrderModuleListHead.Blink);

		while (true)
		{
			LDR_DATA_TABLE_ENTRY CurrentEntry;
			ReadProcessMemory(hProc, pCurrentEntry, &CurrentEntry, sizeof(LDR_DATA_TABLE_ENTRY), nullptr);

			if (CurrentEntry.DllBase == hMod)
			{
				LIST_ENTRY flink;
				LIST_ENTRY blink;

				ReadProcessMemory(hProc, CurrentEntry.InLoadOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				ReadProcessMemory(hProc, CurrentEntry.InLoadOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);
				flink.Blink = CurrentEntry.InLoadOrder.Blink;
				blink.Flink = CurrentEntry.InLoadOrder.Flink;
				WriteProcessMemory(hProc, CurrentEntry.InLoadOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				WriteProcessMemory(hProc, CurrentEntry.InLoadOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);

				ReadProcessMemory(hProc, CurrentEntry.InMemoryOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				ReadProcessMemory(hProc, CurrentEntry.InMemoryOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);
				flink.Blink = CurrentEntry.InMemoryOrder.Blink;
				blink.Flink = CurrentEntry.InMemoryOrder.Flink;
				WriteProcessMemory(hProc, CurrentEntry.InMemoryOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				WriteProcessMemory(hProc, CurrentEntry.InMemoryOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);

				ReadProcessMemory(hProc, CurrentEntry.InInitOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				ReadProcessMemory(hProc, CurrentEntry.InInitOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);
				flink.Blink = CurrentEntry.InInitOrder.Blink;
				blink.Flink = CurrentEntry.InInitOrder.Flink;
				WriteProcessMemory(hProc, CurrentEntry.InInitOrder.Flink, &flink, sizeof(LIST_ENTRY), nullptr);
				WriteProcessMemory(hProc, CurrentEntry.InInitOrder.Blink, &blink, sizeof(LIST_ENTRY), nullptr);

				BYTE Buffer[MAX_PATH * 2]{ 0 };
				WriteProcessMemory(hProc, CurrentEntry.BaseDllName.szBuffer, Buffer, CurrentEntry.BaseDllName.MaxLength, nullptr);
				WriteProcessMemory(hProc, CurrentEntry.FullDllName.szBuffer, Buffer, CurrentEntry.FullDllName.MaxLength, nullptr);
				WriteProcessMemory(hProc, pCurrentEntry, Buffer, sizeof(LDR_DATA_TABLE_ENTRY), nullptr);

				return INJ_ERR_SUCCESS;
			}

			if (pCurrentEntry == pLastEntry)
			{
				LastError = INJ_ERR_ADV_UNKNOWN;
				return INJ_ERR_CANT_FIND_MOD_PEB;
			}

			pCurrentEntry = ReCa<LDR_DATA_TABLE_ENTRY*>(CurrentEntry.InLoadOrder.Flink);
		}
	}
	
	return INJ_ERR_SUCCESS;
}

UINT __forceinline _strlenA(const char * szString)
{
	UINT Ret = 0;
	for (; *szString++; Ret++);
	return Ret;
}

void __forceinline _ZeroMemory(BYTE * pMem, UINT Size)
{
	for (BYTE * i = pMem; i < pMem + Size; ++i)
		*i = 0x00;
}

void __stdcall ImportTlsExecute(MANUAL_MAPPING_DATA * pData)
{
	BYTE * pBase			= reinterpret_cast<BYTE*>(pData);
	auto * pOp				= &ReCa<IMAGE_NT_HEADERS*>(pBase + ReCa<IMAGE_DOS_HEADER*>(pData)->e_lfanew)->OptionalHeader;
	auto _LoadLibraryA		= pData->pLoadLibrary;
	auto _GetProcAddress	= pData->pGetProcAddress;
	auto _DllMain			= ReCa<f_DLL_ENTRY_POINT>(pBase + pOp->AddressOfEntryPoint);

	if (pOp->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
	{
		auto * pImportDescr = ReCa<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOp->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		while (pImportDescr->Name)
		{
			HINSTANCE hDll = _LoadLibraryA(ReCa<const char*>(pBase + pImportDescr->Name));
			ULONG_PTR * pThunkRef	= ReCa<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
			ULONG_PTR * pFuncRef	= ReCa<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

			_ZeroMemory(pBase + pImportDescr->Name, _strlenA(ReCa<char*>(pBase + pImportDescr->Name)));

			if (!pImportDescr->OriginalFirstThunk)
				pThunkRef = pFuncRef;

			for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
			{
				if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
				{
					*pFuncRef = _GetProcAddress(hDll, ReCa<const char*>(*pThunkRef & 0xFFFF));
					_ZeroMemory(ReCa<BYTE*>(*pThunkRef & 0xFFFF), _strlenA(ReCa<char*>(*pThunkRef & 0xFFFF)));
				}
				else
				{
					auto * pImport = ReCa<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
					*pFuncRef = _GetProcAddress(hDll, pImport->Name);
					_ZeroMemory(ReCa<BYTE*>(pImport->Name), _strlenA(pImport->Name));
				}
			}
			++pImportDescr;
		}
	}

	if (pOp->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
	{
		auto * pTLS = ReCa<IMAGE_TLS_DIRECTORY*>(pBase + pOp->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto * pCallback = ReCa<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
		for (; pCallback && *pCallback; ++pCallback)
			(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
	}

	_DllMain(pBase, DLL_PROCESS_ATTACH, nullptr);

	for (UINT i = 0; i <= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR; ++i)
	{
		if (i == IMAGE_DIRECTORY_ENTRY_IAT)
			continue;

		DWORD Size = pOp->DataDirectory[i].Size;
		if (Size)
			_ZeroMemory(pBase + pOp->DataDirectory[i].VirtualAddress, Size);
	}

	for (UINT i = 0; i != 0x1000; i += sizeof(ULONG64))
		*ReCa<ULONG64*>(pBase + i) = 0;
}

void __stdcall LdrLoadDllShell(LDR_LOAD_DLL_DATA * pData)
{
	if (!pData)
		return;

	pData->pModuleFileName.szBuffer = ReCa<wchar_t*>(pData->Data);
	pData->pLdrLoadDll(nullptr, 0, &pData->pModuleFileName, &pData->Out);
}

bool FileExistsA(const char * szFile)
{
	return (GetFileAttributesA(szFile) != INVALID_FILE_ATTRIBUTES);
}

HANDLE StartRoutine(HANDLE hTargetProc, void * pRoutine, void * pArg, bool Hijack, bool Fastcall)
{
	if (!Hijack)
	{
		auto _NtCTE = reinterpret_cast<f_NtCreateThreadEx>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx"));
		if(!_NtCTE)
			return CreateRemoteThreadEx(hTargetProc, nullptr, 0, ReCa<LPTHREAD_START_ROUTINE>(pRoutine), pArg, 0, nullptr, nullptr);
		
		HANDLE hRet = nullptr;
		_NtCTE(&hRet, THREAD_ALL_ACCESS, nullptr, hTargetProc, pRoutine, pArg, 0, 0, 0, 0, nullptr);
		return hRet;
	}

	DWORD dwProcId = GetProcessId(hTargetProc);
	if (!dwProcId)
	{
		LastError = INJ_ERR_ADV_INV_PROC;
		return nullptr;
	}

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (!hSnap)
	{
		LastError = INJ_ERR_ADV_TH32_FAIL;
		return nullptr;
	}

	THREADENTRY32 TE32 = { 0 };
	TE32.dwSize = sizeof(THREADENTRY32);

	BOOL Ret = Thread32First(hSnap, &TE32);
	while (Ret)
	{
		if (TE32.th32OwnerProcessID == dwProcId && TE32.th32ThreadID != GetCurrentThreadId())
			break;
		Ret = Thread32Next(hSnap, &TE32);
	}
	CloseHandle(hSnap);

	if (!Ret)
	{
		LastError = INJ_ERR_ADV_NO_THREADS;
		return nullptr;
	}

	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, TE32.th32ThreadID);
	if (!hThread)
	{
		LastError = INJ_ERR_ADV_CANT_OPEN_THREAD;
		return nullptr;
	}

	if (SuspendThread(hThread) == (DWORD)-1)
	{
		LastError = INJ_ERR_ADV_SUSPEND_FAIL;
		CloseHandle(hThread);
		return nullptr;
	}

	CONTEXT OldContext;
	OldContext.ContextFlags = CONTEXT_CONTROL;
	if (!GetThreadContext(hThread, &OldContext))
	{
		LastError = INJ_ERR_ADV_GET_CONTEXT_FAIL;
		ResumeThread(hThread);
		CloseHandle(hThread);
		return nullptr;
	}

	void * pCodecave = VirtualAllocEx(hTargetProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pCodecave)
	{
		LastError = INJ_ERR_ADV_OUT_OF_MEMORY;
		ResumeThread(hThread);
		CloseHandle(hThread);
		return nullptr;
	}

	Fastcall = true;

	BYTE Shellcode[] =
	{
		0x48, 0x83, 0xEC, 0x08,														// + 0x00			-> sub rsp, 08

		0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,									// + 0x04 (+ 0x07)	-> mov [rsp], RipLowPart
		0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00,								// + 0x0B (+ 0x0F)	-> mov [rsp + 04], RipHighPart		

		0x50, 0x51, 0x52, 0x53, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53,		// + 0x13			-> push r(acdb)x r(8-11)

		0x48, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,					// + 0x1F (+ 0x21)	-> mov rbx, pFunc
		0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,					// + 0x29 (+ 0x2B)	-> mov rcx, pArg

		0x48, 0x83, 0xEC, 0x20,														// + 0x33			-> sub rsp, 0x20
		0xFF, 0xD3,																	// + 0x37			-> call rbx
		0x48, 0x83, 0xC4, 0x20,														// + 0x39			-> add rsp, 0x20

		0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5B, 0x5A, 0x59, 0x58,		// + 0x3D			-> pop r(11-8) r(bdca)x

		0xC6, 0x05, 0xB0, 0xFF, 0xFF, 0xFF, 0x00,									// + 0x49			-> mov byte ptr[pCodecave - 0x49], 0

		0xC3																		// + 0x50			-> ret
	}; // SIZE = 0x51
	
	DWORD dwLoRIP = (DWORD)(OldContext.Rip & 0xFFFFFFFF);
	DWORD dwHiRIP = (DWORD)((OldContext.Rip >> 0x20) & 0xFFFFFFFF);

	*ReCa<DWORD*>(Shellcode + 0x07) = dwLoRIP;
	*ReCa<DWORD*>(Shellcode + 0x0F) = dwHiRIP;
	*ReCa<void**>(Shellcode + 0x21) = pRoutine;
	*ReCa<void**>(Shellcode + 0x2B) = pArg;

	OldContext.Rip = ReCa<DWORD64>(pCodecave);

	if (!WriteProcessMemory(hTargetProc, pCodecave, Shellcode, sizeof(Shellcode), nullptr))
	{
		LastError = INJ_ERR_ADV_WPM_FAIL;
		VirtualFreeEx(hTargetProc, pCodecave, MEM_RELEASE, 0);
		ResumeThread(hThread);
		CloseHandle(hThread);
		return nullptr;
	}

	if (!SetThreadContext(hThread, &OldContext))
	{
		LastError = INJ_ERR_ADV_SET_CONTEXT_FAIL;
		VirtualFreeEx(hTargetProc, pCodecave, MEM_RELEASE, 0);
		ResumeThread(hThread);
		CloseHandle(hThread);
		return nullptr;
	}

	if (ResumeThread(hThread) == (DWORD)-1)
	{
		LastError = INJ_ERR_ADV_RESUME_FAIL;
		VirtualFreeEx(hTargetProc, pCodecave, MEM_RELEASE, 0);
		CloseHandle(hThread);
		return nullptr;
	}

	BYTE CheckByte = 1;
	while (CheckByte)
		ReadProcessMemory(hTargetProc, pCodecave, &CheckByte, 1, nullptr);
		
	CloseHandle(hThread);
	VirtualFreeEx(hTargetProc, pCodecave, MEM_RELEASE, 0);	
	
	return (HANDLE)1;
}

PEB * GetPEB(HANDLE hProc)
{
	auto _NtQIP = reinterpret_cast<f_NtQueryInformationProcess>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess"));
	if (!_NtQIP)
	{
		LastError = INJ_ERR_ADV_QIP_MISSING;
		return nullptr;
	}

	PROCESS_BASIC_INFORMATION PBI{ 0 };
	ULONG SizeOut = 0;
	if (_NtQIP(hProc, PROCESSINFOCLASS::ProcessBasicInformation, &PBI, sizeof(PROCESS_BASIC_INFORMATION), &SizeOut) < 0)
	{
		LastError = INJ_ERR_ADV_QIP_FAIL;
		return nullptr;
	}

	return PBI.pPEB;
}