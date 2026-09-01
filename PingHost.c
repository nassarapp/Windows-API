/* PingHost.c
 *
 * A simple Windows C example that pings a remote host using the ICMP API.
 *
 * Build with MSVC:
 *   cl.exe PingHost.c
 *
 * Build with MinGW:
 *   x86_64-w64-mingw32-gcc PingHost.c -o PingHost.exe -lws2_32 -liphlpapi
 *
 * Usage:
 *   PingHost.exe google.com
 *   PingHost.exe 8.8.8.8
 */

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <hostname or IPv4 address>\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    struct addrinfo hints = {0};
    struct addrinfo* res = NULL;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], NULL, &hints, &res) != 0)
    {
        printf("Failed to resolve hostname: %s\n", argv[1]);
        WSACleanup();
        return 1;
    }

    DWORD ipAddr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE)
    {
        printf("IcmpCreateFile failed: %lu\n", GetLastError());
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }

    char sendData[32] = "WindowsAPICPingExample";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    LPVOID replyBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, replySize);
    if (!replyBuffer)
    {
        printf("HeapAlloc failed\n");
        IcmpCloseHandle(hIcmp);
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }

    DWORD dwRetVal = IcmpSendEcho(hIcmp,
                                  ipAddr,
                                  sendData,
                                  sizeof(sendData),
                                  NULL,
                                  replyBuffer,
                                  replySize,
                                  1000);

    if (dwRetVal != 0)
    {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
        struct in_addr replyAddr;
        replyAddr.S_un.S_addr = pEchoReply->Address;

        printf("Reply from %s (%s):\n", argv[1], inet_ntoa(replyAddr));
        printf("  Status: %lu\n", pEchoReply->Status);
        printf("  Round-trip time: %lu ms\n", pEchoReply->RoundTripTime);
        printf("  Data size: %lu bytes\n", pEchoReply->DataSize);
        printf("  TTL: %lu\n", pEchoReply->Options.Ttl);
    }
    else
    {
        printf("IcmpSendEcho failed: %lu\n", GetLastError());
    }

    HeapFree(GetProcessHeap(), 0, replyBuffer);
    IcmpCloseHandle(hIcmp);
    freeaddrinfo(res);
    WSACleanup();

    return 0;
}
