#include <Windows.h>
#include <iostream>
#include <psapi.h>
#include <stdio.h>


void PrintProcessNameAndID(DWORD procID);

void PrintProcessNameAndID(DWORD procID)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procID);
	HMODULE  lphModule; 
	LPDWORD lpcbNeeded = 0;

	DWORD cb = (DWORD)sizeof(lphModule);

	LPWSTR baseName[256];
	DWORD baseNameSize = (DWORD)sizeof(baseNameSize);

	if (!hProc)
	{
		std::cout << "Error with getting handle in OpenProcess " << GetLastError() << std::endl;
		return;
	}

	if (!EnumProcessModules(hProc, &lphModule, cb, lpcbNeeded))
	{
		std::cout << "Error in EnumProcessModules " << GetLastError() << std::endl;
		return;
	}

	
	if (GetModuleBaseName(hProc, lphModule, (LPWSTR)baseName, baseNameSize) == 0)
	{
		std::cout << "Error in GetModuleBaseName " << GetLastError() << std::endl;
		return;
	}
	
	printf("%s (PID: %u)\n", baseName, procID);

	return;

}


int main()

{

	DWORD lpidProcess[1024];
	DWORD cb = sizeof(lpidProcess);
	DWORD lpcbNeeded = 0;


	if (!EnumProcesses(lpidProcess, cb, &lpcbNeeded))
	{
		std::cout << "Error in EnumProcesses " << GetLastError() << std::endl;
		return -1;
	}

	//calculate how many processes were enumerated

	DWORD enumeratedProcs = lpcbNeeded / sizeof(DWORD);

	std::cout << "Number of enumerated processes: " << enumeratedProcs << std::endl;

	for (DWORD i = 0; i < sizeof(lpidProcess); ++i)
	{
		PrintProcessNameAndID(lpidProcess[i]);

	}


	
	return 0;
}