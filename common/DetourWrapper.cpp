#include "DetourWrapper.h"

#include "RedtapeWindows.h"
#include <Psapi.h>
#include "3rdparty/detours/src/detours.h"
#include <string>
#include <vector>

using CreateThread_t = HANDLE(WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, __drv_aliasesMem LPVOID, DWORD, LPDWORD);

CreateThread_t pOriginalCreateThread = nullptr;

std::vector<std::wstring> blacklistedModules = {L"graphics-hook64.dll"};

HANDLE WINAPI CreateThread_Handler(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	__drv_aliasesMem LPVOID lpParameter,DWORD dwCreationFlags,LPDWORD lpThreadId)
{

	const HMODULE currentModule = DetourGetContainingModule(lpStartAddress);
	WCHAR moduleName[256];
	GetModuleBaseName(GetCurrentProcess(), currentModule, moduleName, 256);

	for (size_t i = 0; i < blacklistedModules.size(); i++)
	{
		if (moduleName == blacklistedModules[i])
			return nullptr;
	}

	return pOriginalCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
}

void DetourWrapper::Install()
{
	pOriginalCreateThread = CreateThread;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)pOriginalCreateThread, CreateThread_Handler);
	DetourTransactionCommit();
}
