// CreateProcess-Pawel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
//	 BOOL CreateProcessA(
//	[in, optional]      LPCSTR                lpApplicationName,
//	[in, out, optional] LPSTR                 lpCommandLine,
//	[in, optional]      LPSECURITY_ATTRIBUTES lpProcessAttributes,
//	[in, optional]      LPSECURITY_ATTRIBUTES lpThreadAttributes,
//	[in]                BOOL                  bInheritHandles,
//	[in]                DWORD                 dwCreationFlags,
//	[in, optional]      LPVOID                lpEnvironment,
//	[in, optional]      LPCSTR                lpCurrentDirectory,
//	[in]                LPSTARTUPINFOA        lpStartupInfo,
//	[out]               LPPROCESS_INFORMATION lpProcessInformation
//	);
// 
// 

#include <Windows.h>
#include <iostream>

int main()
{

	using std::cout;
	using std::endl;

	//WCHAR name[] = L"notepad";
	const WCHAR name[] = L"c:\\windows\\system32\\notepad.exe";
	STARTUPINFO si = { sizeof(si) };

	PROCESS_INFORMATION pi;

	DWORD code;
	BOOL created = CreateProcess(name, nullptr /*name*/, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

	if (created)
	{
		cout << "Process created PID: " << pi.dwProcessId << endl;
		
		DWORD rv = WaitForSingleObject(pi.hProcess, 10000);
		if (rv == WAIT_OBJECT_0)
		{
			cout << "Process is terminated" << endl;
			GetExitCodeProcess(pi.hProcess, &code);
			cout << "Exit Code: " << code << endl;
		}

		else
		{
			cout << "Process is still running" << endl;
		}



		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

	}
	else
	{
		cout << "Error: " << GetLastError() << endl;
	}


	return 0;

}