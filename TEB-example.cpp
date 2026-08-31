#include <windows.h>
#include <winternl.h>
#include <intrin.h>
#include <stdio.h>

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;

// Well-known TEB fields that follow NT_TIB. Layout is architecture-specific.
#ifdef _WIN64
typedef struct _TEB_VIEW {
    NT_TIB NtTib;                       // 0x000
    PVOID EnvironmentPointer;           // 0x038
    CLIENT_ID ClientId;                 // 0x040
    PVOID ActiveRpcHandle;              // 0x050
    PVOID ThreadLocalStoragePointer;    // 0x058
    PPEB ProcessEnvironmentBlock;       // 0x060
    ULONG LastErrorValue;               // 0x068
} TEB_VIEW;
#else
typedef struct _TEB_VIEW {
    NT_TIB NtTib;                       // 0x000
    PVOID EnvironmentPointer;           // 0x01C
    CLIENT_ID ClientId;                 // 0x020
    PVOID ActiveRpcHandle;              // 0x028
    PVOID ThreadLocalStoragePointer;    // 0x02C
    PPEB ProcessEnvironmentBlock;       // 0x030
    ULONG LastErrorValue;               // 0x034
} TEB_VIEW;
#endif

// TEB is at GS:[0x30] on x64 and FS:[0x18] on x86 (NT_TIB.Self).
static TEB_VIEW* ReadTebFromSegmentRegister(void)
{
#ifdef _WIN64
    return (TEB_VIEW*)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
#else
    return (TEB_VIEW*)__readfsdword(FIELD_OFFSET(NT_TIB, Self));
#endif
}

int main(void)
{
    TEB_VIEW* tebNt = (TEB_VIEW*)NtCurrentTeb();
    TEB_VIEW* tebSeg = ReadTebFromSegmentRegister();
    DWORD lastError = GetLastError();

    printf("NtCurrentTeb():              %p\n", tebNt);
    printf("Segment register (GS/FS):    %p\n", tebSeg);
    printf("NT_TIB.Self:                 %p\n", tebNt->NtTib.Self);
    printf("\n");

    printf("Stack base:                  %p\n", tebNt->NtTib.StackBase);
    printf("Stack limit:                 %p\n", tebNt->NtTib.StackLimit);
    printf("Fiber data:                  %p\n", tebNt->NtTib.FiberData);
    printf("Arbitrary user pointer:      %p\n", tebNt->NtTib.ArbitraryUserPointer);
    printf("\n");

    printf("ClientId.UniqueProcess:      %p  (GetCurrentProcessId = %lu)\n",
           tebNt->ClientId.UniqueProcess, GetCurrentProcessId());
    printf("ClientId.UniqueThread:       %p  (GetCurrentThreadId  = %lu)\n",
           tebNt->ClientId.UniqueThread, GetCurrentThreadId());
    printf("\n");

    printf("PEB:                         %p\n", tebNt->ProcessEnvironmentBlock);
    printf("TLS pointer:                 %p\n", tebNt->ThreadLocalStoragePointer);
    printf("LastErrorValue:              %lu  (GetLastError = %lu)\n",
           tebNt->LastErrorValue, lastError);

    if (tebNt->ProcessEnvironmentBlock != NULL)
    {
        printf("PEB->BeingDebugged:          %u\n",
               tebNt->ProcessEnvironmentBlock->BeingDebugged);
    }

    return 0;
}
