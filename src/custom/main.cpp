#include <cstdio>
#include <windows.h>
#include <winuser.h>

#include "init.hpp"

// FUNCTION: CUSTOM 0x00401970
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DialogBoxParamA(hInstance, MAKEINTRESOURCEA(103), NULL, DialogProc, 0);
    return 0;
}
