/* QueryMissingHttpHeaders.c
 *
 * A simple Windows C utility that checks a remote host for common missing
 * HTTP security response headers.
 *
 * Build with MSVC:
 *   cl.exe QueryMissingHttpHeaders.c
 *
 * Build with MinGW:
 *   x86_64-w64-mingw32-gcc QueryMissingHttpHeaders.c -o QueryMissingHttpHeaders.exe -lwinhttp
 *
 * Usage:
 *   QueryMissingHttpHeaders.exe https://www.google.com/
 */

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <wchar.h>

#pragma comment(lib, "winhttp.lib")

typedef struct
{
    const wchar_t* name;
    const char* description;
} SecurityHeader;

SecurityHeader g_securityHeaders[] = {
    { L"Strict-Transport-Security", "HTTPS strict transport security (HSTS)" },
    { L"X-Frame-Options", "Clickjacking / framing protection" },
    { L"X-Content-Type-Options", "MIME-sniffing protection" },
    { L"Content-Security-Policy", "Content injection / XSS protection" },
    { L"Referrer-Policy", "Referrer leakage control" },
    { L"Permissions-Policy", "Browser feature restrictions" },
    { L"X-XSS-Protection", "Legacy XSS filter" },
    { NULL, NULL }
};

// Returns TRUE if the header exists, FALSE if it is missing.
BOOL HeaderExists(HINTERNET hRequest, const wchar_t* headerName)
{
    DWORD dwSize = 0;
    BOOL result = WinHttpQueryHeaders(hRequest,
                                      WINHTTP_QUERY_CUSTOM,
                                      headerName,
                                      WINHTTP_NO_OUTPUT_BUFFER,
                                      &dwSize,
                                      WINHTTP_NO_HEADER_INDEX);
    if (!result)
    {
        DWORD error = GetLastError();
        if (error == ERROR_WINHTTP_HEADER_NOT_FOUND)
        {
            return FALSE;
        }
        else if (error == ERROR_WINHTTP_INSUFFICIENT_BUFFER)
        {
            return TRUE;
        }
    }
    return TRUE;
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 2)
    {
        wprintf(L"Usage: %s <url>\n", argv[0]);
        wprintf(L"Example: %s https://www.google.com/\n", argv[0]);
        return 1;
    }

    wchar_t url[2048];
    if (wcsncmp(argv[1], L"http://", 7) != 0 && wcsncmp(argv[1], L"https://", 8) != 0)
    {
        swprintf_s(url, sizeof(url) / sizeof(url[0]), L"https://%s", argv[1]);
    }
    else
    {
        wcscpy_s(url, sizeof(url) / sizeof(url[0]), argv[1]);
    }

    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    if (!WinHttpCrackUrl(url, 0, 0, &urlComp))
    {
        wprintf(L"Failed to parse URL: %s (error %lu)\n", url, GetLastError());
        return 1;
    }

    HINTERNET hSession = WinHttpOpen(L"MissingHttpHeadersChecker/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession)
    {
        wprintf(L"WinHttpOpen failed: %lu\n", GetLastError());
        return 1;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect)
    {
        wprintf(L"WinHttpConnect failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return 1;
    }

    DWORD dwFlags = 0;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
    {
        dwFlags = WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"GET",
                                            urlComp.lpszUrlPath,
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            dwFlags);
    if (!hRequest)
    {
        wprintf(L"WinHttpOpenRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS,
                                       0,
                                       WINHTTP_NO_REQUEST_DATA,
                                       0,
                                       0,
                                       0);
    if (!bResults)
    {
        wprintf(L"WinHttpSendRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults)
    {
        wprintf(L"WinHttpReceiveResponse failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &dwStatusCode,
                        &dwSize,
                        WINHTTP_NO_HEADER_INDEX);

    wprintf(L"URL: %s\n", url);
    wprintf(L"Status code: %lu\n\n", dwStatusCode);
    wprintf(L"Missing security headers:\n");

    int missingCount = 0;
    for (int i = 0; g_securityHeaders[i].name != NULL; i++)
    {
        if (!HeaderExists(hRequest, g_securityHeaders[i].name))
        {
            wprintf(L"  %s (%hs)\n", g_securityHeaders[i].name, g_securityHeaders[i].description);
            missingCount++;
        }
    }

    if (missingCount == 0)
    {
        wprintf(L"  None - all checked headers are present.\n");
    }
    else
    {
        wprintf(L"\nTotal missing: %d\n", missingCount);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return 0;
}
