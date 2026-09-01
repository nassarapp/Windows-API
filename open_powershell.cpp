#include <windows.h>
#include <shellapi.h>
#include <iostream>

int main() {
    HINSTANCE result = ShellExecuteA(NULL, "open", "powershell.exe", NULL, NULL, SW_SHOWDEFAULT);

    if ((INT_PTR)result <= 32) {
        std::cerr << "Failed to open PowerShell (error: " << (INT_PTR)result << ")" << std::endl;
        return 1;
    }

    std::cout << "PowerShell opened successfully" << std::endl;
    return 0;
}
