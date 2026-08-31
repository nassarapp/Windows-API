// CredProtectUnprotect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credprotectw 


/*

BOOL CredProtectA(
  [in]      BOOL                 fAsSelf,
  [in]      LPSTR                pszCredentials,
  [in]      DWORD                cchCredentials,
  [out]     LPSTR                pszProtectedCredentials,
  [in, out] DWORD                *pcchMaxChars,
  [out]     CRED_PROTECTION_TYPE *ProtectionType
);

*/


#include <Windows.h>
#include <iostream>
#include <wincred.h>
#pragma comment (lib, "advapi32")



int main(void)

{
	 WCHAR  plainText[] = L"Wutang for ever";
	
	WCHAR encrypted;

	
	


	if (!CredProtect(FALSE, &plainText, wcslen(plaiText), &encrypted, wcslen(encrypted), NULL))
	{
	
		std::cout << "Error in encyprtion " << GetLastError() << std::endl;
		return -1;
	}


		return 0;
)