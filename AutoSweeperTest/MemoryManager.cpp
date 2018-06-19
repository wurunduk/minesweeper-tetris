#include "MemoryManager.h"

#include <TlHelp32.h>


void MemoryManager::DetachFromProcess() const
{
	CloseHandle(processHandle);
}


void MemoryManager::AttachToProcess(const char* processName)
{
	const auto handleID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(ProcessEntry);

	do
	{
		if (!strcmp(ProcessEntry.szExeFile, processName))
		{
			processID = ProcessEntry.th32ProcessID;
			CloseHandle(handleID);

			processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
			return;
		}
		//we need a sleep so we dont use too much CPU while scanning
		Sleep(1);
	} while (Process32Next(handleID, &ProcessEntry));
}

DWORD MemoryManager::GetModuleBaseAddress(const char* moduleName) const
{
	const auto moduleHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
	MODULEENTRY32 ModuleEntry;
	ModuleEntry.dwSize = sizeof(ModuleEntry);

	do
	{
		if (!strcmp(ModuleEntry.szModule, moduleName))
		{
			CloseHandle(moduleHandle);
			return reinterpret_cast<DWORD>(ModuleEntry.modBaseAddr);
		}
		Sleep(1);
	} while (Module32Next(moduleHandle, &ModuleEntry));

	return 0;
}
