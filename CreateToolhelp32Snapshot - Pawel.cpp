// CreateToolhelp32Snapshot - Pawel.cpp : This file contains the 'main' function. Program execution begins and ends there.


//HANDLE CreateToolhelp32Snapshot(
//[in] DWORD dwFlags,
//[in] DWORD th32ProcessID
//);
//
//
//
// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>



int main()
{

	using std::cout;
	using std::endl;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		cout << "Error" << endl;
		return -1;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (Process32First(hSnapshot, &pe))
	{
		do {
			printf("PID: %5u PPID: %5u  T: %3u (%ws)\n", pe.th32ProcessID, pe.th32ParentProcessID, pe.cntThreads, pe.szExeFile);

		} while (Process32Next(hSnapshot, &pe));
	}
	
	CloseHandle(hSnapshot);


	return 0;
}
