// GetSystemTime.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>

void main()
{
    SYSTEMTIME lt;

 
    GetLocalTime(&lt);
    
    printf(" The local time is: %02d:%02d\n", lt.wHour, lt.wMinute);
}