#include "stdafx.h"
#include "ConUtils.h"

void CopyToClipboard(const wchar_t* pSrc, size_t len)
{
    if (pSrc != nullptr && len > 0)
    {
        // TODO Convert EOL to '\r\n'
        HGLOBAL hMem = NULL;
        hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
        memcpy(pMem, pSrc, len * sizeof(wchar_t));
        pMem[len] = L'\0';
        GlobalUnlock(hMem);

        OpenClipboard(0);
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
}

void SetCursorVisible(HANDLE hScreen, CONSOLE_CURSOR_INFO& cci, BOOL bVisible)
{
    if (cci.bVisible != bVisible)
    {
        cci.bVisible = bVisible;
        SetConsoleCursorInfo(hScreen, &cci);
    }
}

void DrawBuffer(HANDLE hScreen, const Buffer& buffer, COORD posBuffer)
{
    SMALL_RECT wr = Rectangle(posBuffer, buffer.size);
    WriteConsoleOutput(hScreen, &buffer.data.front(), buffer.size, _COORD(0, 0), &wr);
}

void Write(std::vector<CHAR_INFO>::iterator b, std::vector<CHAR_INFO>::iterator e, const wchar_t* s)
{
    auto it = b;
    const wchar_t* t = s;
    while (it != e && *t != L'\0')
    {
        if (*t == L'\t')
        {
            for (int i = 0; i < 4; ++i) // TODO Should really be x - x%4
            {
                it->Char.UnicodeChar = L' ';
                ++it;
            }
        }
        else
        {
            it->Char.UnicodeChar = *t;
            ++it;
        }
        ++t;
    }
}

void WriteBegin(Buffer& b, COORD o, const wchar_t* s)
{
    // TODO Check bounds
    auto it = b.data.begin() + b.offset(o);
    Write(it, b.data.end(), s);
}

void WriteEnd(Buffer& b, COORD o, const wchar_t* s)
{
    // TODO Check bounds
    size_t offset = wcslen(s) < b.offset(o) ? b.offset(o) - wcslen(s) : 0;
    auto it = b.data.begin() + offset;
    Write(it, b.data.end(), s);
}
