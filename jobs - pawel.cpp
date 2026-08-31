// jobs - pawel.cpp : This file contains the 'main' function. Program execution begins and ends there.


#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <VersionHelpers.h>		// to make sure its windows 8 and above



int main(int argc, char *argv[])
{

	if (!IsWindows8OrGreater())
	{
		printf("CPU rate limit only supported on Windows 8+\n");
		return -1;
	}
	if (argc < 3)
	{
		printf("Usage: cpulimit <pid> <percentage>\n");

		return 0;
	}

	HANDLE hJob = CreateJobObject(nullptr, L"CpuRateJob");
	if (!hJob)
	{
		printf("Error Opening Process\n");
	}


	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_SET_QUOTA, FALSE, atoi(argv[1]));
	if (!hProcess)
	{
		return 1;
	}
	
	if (!hProcess)
	{
		printf("Error opening process\n");
		return 1;
	}

	if (!AssignProcessToJobObject(hJob, hProcess))
	{
		return 1;
	}

	JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info;

	info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
	info.CpuRate = atoi(argv[2]) * 100;

	if(!SetInformationJobObject(hJob, JobObjectCpuRateControlInformation, &info, sizeof(info)));
	{
		return 1;
	}
	printf("Succeeded!\n");

	Sleep(INFINITE);

	return 0;
}