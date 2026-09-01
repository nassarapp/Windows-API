#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

std::string FormatLuid(const LUID& luid)
{
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << luid.HighPart
	   << ":" << std::dec << luid.LowPart;
	return ss.str();
}

std::string GetPrivilegeStatusString(DWORD attributes)
{
	std::string status;

	if (attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
	{
		status = "Enabled (Default)";
	}
	else if (attributes & SE_PRIVILEGE_ENABLED)
	{
		status = "Enabled";
	}
	else
	{
		status = "Disabled";
	}

	if (attributes & SE_PRIVILEGE_REMOVED)
	{
		status += " | Removed";
	}
	if (attributes & SE_PRIVILEGE_USED_FOR_ACCESS)
	{
		status += " | Used for access";
	}

	return status;
}

int main()
{
	HANDLE hToken = nullptr;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
		return 1;
	}

	DWORD cbSize = 0;
	GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &cbSize);

	if (cbSize == 0)
	{
		std::cerr << "GetTokenInformation size query failed: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		return 1;
	}

	std::vector<BYTE> buffer(cbSize);
	PTOKEN_PRIVILEGES pPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(buffer.data());

	if (!GetTokenInformation(hToken, TokenPrivileges, pPrivileges, cbSize, &cbSize))
	{
		std::cerr << "GetTokenInformation failed: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		return 1;
	}

	std::cout << "Current process token privileges (" << pPrivileges->PrivilegeCount << " total):\n";
	std::cout << std::string(95, '-') << std::endl;
	std::cout << std::left
	          << std::setw(5)  << "#"
	          << std::setw(45) << "Privilege Name"
	          << std::setw(20) << "LUID"
	          << "Status" << std::endl;
	std::cout << std::string(95, '-') << std::endl;

	for (DWORD i = 0; i < pPrivileges->PrivilegeCount; ++i)
	{
		LUID_AND_ATTRIBUTES& laa = pPrivileges->Privileges[i];

		char privilegeName[256] = { 0 };
		DWORD nameSize = sizeof(privilegeName);

		std::string nameStr;
		if (LookupPrivilegeNameA(nullptr, &laa.Luid, privilegeName, &nameSize))
		{
			nameStr = privilegeName;
		}
		else
		{
			nameStr = "<unknown privilege>";
		}

		std::cout << std::left
		          << std::setw(5)  << (i + 1)
		          << std::setw(45) << nameStr
		          << std::setw(20) << FormatLuid(laa.Luid)
		          << GetPrivilegeStatusString(laa.Attributes) << std::endl;
	}

	std::cout << std::string(95, '-') << std::endl;
	CloseHandle(hToken);
	return 0;
}
