// CreateThread - Pawel.cpp : This file contains the 'main' function. Program execution begins and ends there.
/*

HANDLE CreateThread(
  [in, optional]  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
  [in]            SIZE_T                  dwStackSize,
  [in]            LPTHREAD_START_ROUTINE  lpStartAddress,
  [in, optional]  __drv_aliasesMem LPVOID lpParameter,
  [in]            DWORD                   dwCreationFlags,
  [out, optional] LPDWORD                 lpThreadId
);


https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread 

*/

#include <Windows.h>
#include <stdio.h>
#include <iostream>


struct Data {
    int x, y;
    int Result;
};

DWORD WINAPI MyFunction(PVOID param);



int main()
{
    printf("Main thread id: %u\n", GetCurrentThreadId());

    Data d, d2;

    d.x = 10; d.y = 20;
    d2.x = 12; d2.y = 30;

    DWORD id;

    HANDLE hThread = CreateThread(nullptr, 0, MyFunction, &d, 0, &id);
    HANDLE hThread2 = CreateThread(nullptr, 0, MyFunction, &d2, 0, &id);

    printf("Main thread running\n");
    HANDLE h[] = { hThread, hThread2 };

    //WaitForSingleObject(hThread, INFINITE);
    WaitForMultipleObjects(2, h, TRUE, INFINITE);
    printf("Thread done\n");

   // DWORD code;
    //GetExitCodeThread(hThread, &code);
    //printf("Result: %u\n", code);

    printf("Result: %u\n", d.Result + d2.Result);

    CloseHandle(hThread);

    return 0;
}

DWORD WINAPI MyFunction(PVOID param)
{

    /* Casting data back to Data instead of param */

    Data* d = (Data*)param; // down casting
    printf("Worker Thread: %u\n", GetCurrentThreadId());
    Sleep(3000);

    d->Result = d->x * d->y;

    return 1;
}
