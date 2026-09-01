#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

std::string MakeHttpRequest(const std::wstring& userAgent,
                            const std::wstring& server,
                            const std::wstring& path)
{
    std::string response;

    HINTERNET hSession = WinHttpOpen(userAgent.c_str(),
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession)
    {
        std::cerr << "WinHttpOpen failed: " << GetLastError() << "\n";
        return response;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect)
    {
        std::cerr << "WinHttpConnect failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(hSession);
        return response;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                          L"GET",
                                          path.c_str(),
                                          NULL,
                                          WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          0);
    if (!hRequest)
    {
        std::cerr << "WinHttpOpenRequest failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
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
        std::cerr << "WinHttpSendRequest failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults)
    {
        std::cerr << "WinHttpReceiveResponse failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &dwStatusCode,
                        &dwSize,
                        WINHTTP_NO_HEADER_INDEX);

    std::cout << "HTTP Status Code: " << dwStatusCode << "\n";

    DWORD dwDownloaded = 0;
    do
    {
        DWORD dwAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwAvailable))
        {
            std::cerr << "WinHttpQueryDataAvailable failed: " << GetLastError() << "\n";
            break;
        }

        if (dwAvailable == 0)
            break;

        std::vector<char> buffer(dwAvailable + 1, 0);
        if (!WinHttpReadData(hRequest, buffer.data(), dwAvailable, &dwDownloaded))
        {
            std::cerr << "WinHttpReadData failed: " << GetLastError() << "\n";
            break;
        }

        response.append(buffer.data(), dwDownloaded);
    } while (dwDownloaded > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

int main()
{
    std::wstring userAgent = L"Windows-API-Example/1.0";
    std::wstring server = L"www.google.com";
    std::wstring path = L"/";

    std::string response = MakeHttpRequest(userAgent, server, path);

    std::cout << "Response length: " << response.size() << " bytes\n";
    std::cout << "Response preview:\n";
    std::cout << response.substr(0, 1024) << "\n";

    return 0;
}
