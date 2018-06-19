#ifndef _MEMORYMANAGER_H_
#define _MEMORYMANAGER_H_

#define WIN32_LEAN_AND_MEAN //Exclude not needed content of Win32API

#include <Windows.h>

class MemoryManager 
{
protected:
	HANDLE processHandle = nullptr;
	DWORD processID = 0;

public:
	template <typename  T>
	T Read(const DWORD address)
	{
		T returnValue;
		if(processHandle != nullptr)
			ReadProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), &returnValue, sizeof(T), nullptr);

		return returnValue;
	}

	template <typename T>
	void Write(const DWORD address, T value)
	{
		if(processHandle != nullptr)
			WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
	}

	void AttachToProcess(const char* processName);
	//call this when the program terminates to avoid memory leaking
	void DetachFromProcess() const;
	DWORD GetModuleBaseAddress(const char* moduleName) const;
};
#endif