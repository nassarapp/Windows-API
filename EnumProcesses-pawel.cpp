// EnumProcesses-pawel.cpp : This file contains the 'main' function. Program execution begins and ends there.
/*

BOOL EnumProcesses(
  [out] DWORD   *lpidProcess,
  [in]  DWORD   cb,
  [out] LPDWORD lpcbNeeded
);

https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocesses
*/

#include <Windows.h>
#include <stdio.h>
#include <Psapi.h>

int main()
{
	DWORD pid[5000];
	DWORD count;

	if (EnumProcesses(pid, sizeof(pid), &count))
	{
		for (int i = 0; i < count / sizeof(DWORD); i++)
		{
			printf("PID: %5u", pid[i]);


			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid[i]);

			if (hProcess)
			{
				WCHAR path[MAX_PATH];
				DWORD size = _countof(path);
				if (QueryFullProcessImageName(hProcess, 0, path, &size) > 0)
				{
					printf(" %ws\n", path);
				}

				CloseHandle(hProcess);
				
			}
		}
	}


	return 0;
}