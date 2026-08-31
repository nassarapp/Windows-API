#include <Windows.h>
#include <iostream>
#include <psapi.h>


int main()
{

	HANDLE hProc;
	HANDLE hModule;
	MODULEINFO mi;
	DWORD cd = sizeof(MODULEINFO);


		if ((hProc = OpenProcess(dwDesiredAccess, TRUE, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ)) == NULL)
		{
			std::cout << "Error in calling OpenProcess " << GetLastError() << std::endl;
			return -1;
	    }


		if ((hModule = GetModuleHandle(L"notepad.exe")) == NULL)
		{
			std::cout << "Error in calling GetModuleHandle " << GetLastError() << std::endl;
			return -1;
		}

		if (!GetModuleInformation(hProc, hModule, &mi, cb))
		{
			std::cout << "Error in calling GetModuleInformation " << GetLastError() << std::endl;
			return -1;
		}



	return 0;
}