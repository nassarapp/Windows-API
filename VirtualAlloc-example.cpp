#include <windows.h>
#include <stdio.h>
#include <string.h>

int main()
{
    SIZE_T size = 4096;
    LPVOID address = VirtualAlloc(
        NULL,                   // Let the system choose the address
        size,                   // Size of the allocation (4 KB)
        MEM_COMMIT | MEM_RESERVE, // Allocate physical storage and reserve virtual address space
        PAGE_READWRITE          // Read/write access
    );

    if (address == NULL)
    {
        printf("VirtualAlloc failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Allocated memory at address: %p\n", address);

    const char* message = "Hello from VirtualAlloc!";
    memcpy(address, message, strlen(message) + 1);

    printf("Written to allocated memory: %s\n", (char*)address);

    DWORD oldProtect;
    BOOL protectResult = VirtualProtect(
        address,
        size,
        PAGE_READONLY,
        &oldProtect
    );

    if (!protectResult)
    {
        printf("VirtualProtect failed with error: %lu\n", GetLastError());
        VirtualFree(address, 0, MEM_RELEASE);
        return 1;
    }

    printf("Memory protection changed from PAGE_READWRITE to PAGE_READONLY\n");

    BOOL freeResult = VirtualFree(
        address,                // Base address of the allocation
        0,                      // dwSize must be 0 when using MEM_RELEASE
        MEM_RELEASE             // Release the entire region
    );

    if (!freeResult)
    {
        printf("VirtualFree failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Memory freed successfully.\n");

    return 0;
}
