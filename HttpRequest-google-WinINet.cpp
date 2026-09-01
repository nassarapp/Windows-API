#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "wininet.lib")

std::string MakeHttpRequestWithWinINet(const std::wstring& userAgent,
                                       const std::wstring& url)
{
    std::string response;

    HINTERNET hInternet = InternetOpen(userAgent.c_str(),
                                       INTERNET_OPEN_TYPE_PRECONFIG,
                                       NULL,
                                       NULL,
                                       0);
    if (!hInternet)
    {
        std::cerr << "InternetOpen failed: " << GetLastError() << "\n";
        return response;
    }

    HINTERNET hUrl = InternetOpenUrl(hInternet,
                                     url.c_str(),
                                     NULL,
                                     0,
                                     INTERNET_FLAG_RELOAD,
                                     0);
    if (!hUrl)
    {
        std::cerr << "InternetOpenUrl failed: " << GetLastError() << "\n";
        InternetCloseHandle(hInternet);
        return response;
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    DWORD dwIndex = 0;
    HttpQueryInfo(hUrl,
                  HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                  &dwStatusCode,
                  &dwSize,
                  &dwIndex);

    std::cout << "HTTP Status Code: " << dwStatusCode << "\n";

    const DWORD bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    DWORD dwRead = 0;

    while (InternetReadFile(hUrl, buffer.data(), bufferSize, &dwRead) && dwRead > 0)
    {
        response.append(buffer.data(), dwRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    return response;
}

int main()
{
    std::wstring userAgent = L"Windows-API-WinINet-Example/1.0";
    std::wstring url = L"http://www.google.com/";

    std::string response = MakeHttpRequestWithWinINet(userAgent, url);

    std::cout << "Response length: " << response.size() << " bytes\n";
    std::cout << "Response preview:\n";
    std::cout << response.substr(0, 1024) << "\n";

    return 0;
}
