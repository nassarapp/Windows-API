#include <windows.h>
#include <stdio.h>
#include <string.h>

int main()
{
    const char* fileName = "ReadFile-test.txt";
    const char* message = "Hello from ReadFile!";

    HANDLE hFile = CreateFileA(
        fileName,                   // Name of the file to create
        GENERIC_READ | GENERIC_WRITE, // Read and write access
        0,                          // Do not share
        NULL,                       // Default security attributes
        CREATE_ALWAYS,              // Always create a new file, overwrite existing
        FILE_ATTRIBUTE_NORMAL,      // Normal file attributes
        NULL                        // No template file
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileA failed with error: %lu\n", GetLastError());
        return 1;
    }

    DWORD bytesWritten = 0;
    BOOL writeResult = WriteFile(
        hFile,                      // Handle to the file
        message,                    // Buffer to write
        (DWORD)strlen(message),     // Number of bytes to write
        &bytesWritten,              // Receives number of bytes written
        NULL                        // Synchronous write
    );

    if (!writeResult)
    {
        printf("WriteFile failed with error: %lu\n", GetLastError());
        CloseHandle(hFile);
        DeleteFileA(fileName);
        return 1;
    }

    printf("Wrote %lu bytes to %s\n", bytesWritten, fileName);

    DWORD fileSize = SetFilePointer(hFile, 0, NULL, FILE_END);
    if (fileSize == INVALID_SET_FILE_POINTER)
    {
        printf("SetFilePointer failed with error: %lu\n", GetLastError());
        CloseHandle(hFile);
        DeleteFileA(fileName);
        return 1;
    }

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        printf("SetFilePointer failed with error: %lu\n", GetLastError());
        CloseHandle(hFile);
        DeleteFileA(fileName);
        return 1;
    }

    char buffer[256] = { 0 };
    DWORD bytesRead = 0;
    BOOL readResult = ReadFile(
        hFile,                      // Handle to the file
        buffer,                     // Buffer to receive data
        sizeof(buffer) - 1,         // Number of bytes to read (leave room for null terminator)
        &bytesRead,                 // Receives number of bytes read
        NULL                        // Synchronous read
    );

    if (!readResult)
    {
        printf("ReadFile failed with error: %lu\n", GetLastError());
        CloseHandle(hFile);
        DeleteFileA(fileName);
        return 1;
    }

    buffer[bytesRead] = '\0';
    printf("Read %lu bytes from %s: %s\n", bytesRead, fileName, buffer);

    BOOL closeResult = CloseHandle(hFile);
    if (!closeResult)
    {
        printf("CloseHandle failed with error: %lu\n", GetLastError());
        DeleteFileA(fileName);
        return 1;
    }

    printf("File handle closed successfully.\n");

    if (!DeleteFileA(fileName))
    {
        printf("DeleteFileA failed with error: %lu\n", GetLastError());
        return 1;
    }

    printf("Temporary file deleted successfully.\n");

    return 0;
}
