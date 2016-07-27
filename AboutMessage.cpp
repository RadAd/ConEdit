#include "stdafx.h"

#include "AboutMessage.H"
#include <Windows.h>

static WORD GetConsoleTextAttribute(HANDLE Console)
{
    CONSOLE_SCREEN_BUFFER_INFO	ConsoleInfo = { sizeof(CONSOLE_SCREEN_BUFFER_INFO ) };
    GetConsoleScreenBufferInfo(Console, &ConsoleInfo);
    return ConsoleInfo.wAttributes;
}

static void GetVersionString(void *Info, const TCHAR* s, const TCHAR*& r)
{
    TCHAR	*String = nullptr;
    UINT	Length = 0;
    VerQueryValue(Info, s, (LPVOID *) &String, &Length);
    if (String != nullptr)
        r = String;
}

#define LANGUAGE "0c0904b0"

void DisplayAboutMessage(HINSTANCE hInstance)
{
    const TCHAR*	FileVersion = L"Unknown";
    const TCHAR*	FileDescription = L"Unknown";
    const TCHAR*	CompanyName = L"Unknown";
    const TCHAR*	ProductName = L"Unknown";
    const TCHAR*	Copyright = L"Unknown";

	TCHAR	FileName[1024];
	GetModuleFileName(hInstance, FileName, 1024);

	DWORD	Dummy;
	DWORD	Size = GetFileVersionInfoSize(FileName, &Dummy);
    void	*Info = nullptr;

	if (Size > 0)
	{
		Info = malloc(Size);

		// VS_VERSION_INFO   VS_VERSIONINFO  VS_FIXEDFILEINFO

		GetFileVersionInfo(FileName, Dummy, Size, Info);

        GetVersionString(Info, TEXT("\\StringFileInfo\\" LANGUAGE "\\FileVersion"), FileVersion);
        GetVersionString(Info, TEXT("\\StringFileInfo\\" LANGUAGE "\\FileDescription"), FileDescription);
        GetVersionString(Info, TEXT("\\StringFileInfo\\" LANGUAGE "\\CompanyName"), CompanyName);
        GetVersionString(Info, TEXT("\\StringFileInfo\\" LANGUAGE "\\ProductName"), ProductName);
        GetVersionString(Info, TEXT("\\StringFileInfo\\" LANGUAGE "\\LegalCopyright"), Copyright);
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD wOriginalAttribute = GetConsoleTextAttribute(hOut);
    WORD wOriginalBackground= wOriginalAttribute & 0xF0;
    WORD wHighlight = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | wOriginalBackground;

    SetConsoleTextAttribute(hOut, wHighlight);
    WriteConsole(hOut, ProductName, static_cast<DWORD>(_tcslen(ProductName)), nullptr, nullptr);
    SetConsoleTextAttribute(hOut, wOriginalAttribute);
    TCHAR Version[] = L" Version ";
    WriteConsole(hOut, Version, ARRAYSIZE(Version) - 1, nullptr, nullptr);
    SetConsoleTextAttribute(hOut, wHighlight);
    WriteConsole(hOut, FileVersion, static_cast<DWORD>(_tcslen(FileVersion)), nullptr, nullptr);
    SetConsoleTextAttribute(hOut, wOriginalAttribute);
    TCHAR s[] = L" - ";
    WriteConsole(hOut, s, ARRAYSIZE(s) - 1, nullptr, nullptr);
    WriteConsole(hOut, Copyright, static_cast<DWORD>(_tcslen(Copyright)), nullptr, nullptr);
    TCHAR n[] = L"\n ";
    WriteConsole(hOut, n, ARRAYSIZE(n) - 1, nullptr, nullptr);
    WriteConsole(hOut, FileDescription, static_cast<DWORD>(_tcslen(FileDescription)), nullptr, nullptr);
    TCHAR By[] = L"\n By ";
    WriteConsole(hOut, By, ARRAYSIZE(By) - 1, nullptr, nullptr);
    SetConsoleTextAttribute(hOut, wHighlight);
    WriteConsole(hOut, CompanyName, static_cast<DWORD>(_tcslen(CompanyName)), nullptr, nullptr);
    SetConsoleTextAttribute(hOut, wOriginalAttribute);
    TCHAR Details[] = L" (adamgates84+software@gmail.com) - http://radsoft.boxathome.net/\n\n";
    WriteConsole(hOut, Details, ARRAYSIZE(Details) - 1, nullptr, nullptr);

    free(Info);
}

void DisplayResource(HMODULE hModule, WORD wRes)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(wRes), RT_RCDATA);
    if (hResource == NULL)
        throw std::exception("Can't find resource");
    DWORD size = SizeofResource(hModule, hResource);
    HGLOBAL hResourceData = LoadResource(hModule, hResource);
    const char* pData = static_cast<const char*>(LockResource(hResourceData));
    WriteConsoleA(hOut, pData, size, nullptr, nullptr);
}
