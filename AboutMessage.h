#ifndef ABOUTMESSAGE_H
#define ABOUTMESSAGE_H

#include <Windows.H>

void DisplayAboutMessage(HINSTANCE hInstance);

struct Memory
{
    LPVOID data;
    DWORD size;
};
Memory GetResource(HMODULE hModule, WORD wRes);

void DisplayResource(HMODULE hModule, WORD wRes);

#endif
