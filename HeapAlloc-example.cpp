#include <windows.h>
#include <stdio.h>
#include <string.h>

int main()
{
    HANDLE hHeap = GetProcessHeap();
    if (hHeap == NULL)
    {
        printf("GetProcessHeap failed with error: %lu\n", GetLastError());
        return 1;
    }

    SIZE_T size = 256;
    LPVOID buffer = HeapAlloc(
        hHeap,              // Handle to the process heap
        HEAP_ZERO_MEMORY, // Zero-initialize the allocated memory
        size                // Number of bytes to allocate
    );

    if (buffer == NULL)
    {
        printf("HeapAlloc failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Allocated %zu bytes on the process heap at address: %p\n", size, buffer);

    const char* message = "Hello from HeapAlloc!";
    size_t messageLen = strlen(message) + 1;

    if (messageLen <= size)
    {
        memcpy(buffer, message, messageLen);
        printf("Written to allocated memory: %s\n", (char*)buffer);
    }
    else
    {
        printf("Message too large for allocated buffer.\n");
    }

    BOOL freeResult = HeapFree(
        hHeap,              // Handle to the process heap
        0,                  // Flags (reserved, must be 0)
        buffer              // Pointer to the memory block to free
    );

    if (!freeResult)
    {
        printf("HeapFree failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Memory freed successfully.\n");

    return 0;
}
