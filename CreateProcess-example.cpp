#include <Windows.h>
#include <iostream>


int main()
{
	LPCWSTR lpApplicationName= L"c:\\windows\\system32\\calc.exe";
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	BOOL success = FALSE;

	success = CreateProcess(lpApplicationName, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	if (!success)
	{
		std::cout << "Error creating process " << GetLastError() << std::endl;
		return -1;
	}



	return 0;
}
