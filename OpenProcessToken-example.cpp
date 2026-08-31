#include <windows.h>
#include <stdio.h>

int main()
{
    HANDLE hToken = NULL;
    BOOL result = OpenProcessToken(
        GetCurrentProcess(),        // Handle to the process whose token is opened
        TOKEN_QUERY,                // Desired access rights to the token
        &hToken                     // Receives the handle to the token
    );

    if (!result)
    {
        printf("OpenProcessToken failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Successfully opened the current process token.\n");

    DWORD returnLength = 0;
    GetTokenInformation(
        hToken,                     // Handle to the access token
        TokenUser,                  // Type of information to retrieve
        NULL,                       // No buffer yet; request required size
        0,                          // Buffer size is 0
        &returnLength               // Receives the required buffer size
    );

    if (returnLength == 0)
    {
        printf("GetTokenInformation failed to return buffer size with error: %lu\n", GetLastError());
        CloseHandle(hToken);
        return 1;
    }

    PTOKEN_USER pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, returnLength);
    if (pTokenUser == NULL)
    {
        printf("LocalAlloc failed with error: %lu\n", GetLastError());
        CloseHandle(hToken);
        return 1;
    }

    result = GetTokenInformation(
        hToken,
        TokenUser,
        pTokenUser,
        returnLength,
        &returnLength
    );

    if (!result)
    {
        printf("GetTokenInformation failed with error: %lu\n", GetLastError());
        LocalFree(pTokenUser);
        CloseHandle(hToken);
        return 1;
    }

    LPWSTR sidString = NULL;
    if (ConvertSidToStringSidW(pTokenUser->User.Sid, &sidString) == 0)
    {
        printf("ConvertSidToStringSid failed with error: %lu\n", GetLastError());
        LocalFree(pTokenUser);
        CloseHandle(hToken);
        return 1;
    }

    printf("Token user SID: %ls\n", sidString);

    LocalFree(sidString);
    LocalFree(pTokenUser);

    BOOL closeResult = CloseHandle(hToken);
    if (!closeResult)
    {
        printf("CloseHandle failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Token handle closed successfully.\n");

    return 0;
}
