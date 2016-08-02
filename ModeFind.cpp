#include "stdafx.h"

#include "ModeFind.h"

#include "Mode.h"

void FillMessageWindow(Buffer& status, WORD wAttr, const std::wstring& msg); // #include "ModeStatus.h"

void ModeFind::Draw(Screen& screen, const EditScheme& scheme, bool focus) const
{
    if (focus)
        ModeBase::Draw(screen, scheme, false);

    if (visible)
    {
        WORD wAttribute = 0;
        scheme.get(focus ? EditScheme::STATUSFOCUS : EditScheme::STATUS).apply(wAttribute);
        FillMessageWindow(buffer, wAttribute, find);
        DrawBuffer(screen.hOut, buffer, pos);

        if (focus)
        {
            COORD cursor = { static_cast<SHORT>(editPos + 1), 0 };
            SetCursorVisible(screen.hOut, screen.cci, TRUE);
            if (screen.cci.bVisible)
                SetConsoleCursorPosition(screen.hOut, pos + cursor);
        }
    }
}

bool ModeFind::Do(const KEY_EVENT_RECORD& ker, Screen& /*screen*/, const EditScheme& /*scheme*/)
{
    bool cont = true;
    if (ker.bKeyDown)
    {
        switch (ker.wVirtualKeyCode)
        {
        case VK_LEFT:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                if (editPos > 0)
                    --editPos;
            }
            break;

        case VK_RIGHT:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                if (editPos < find.size())
                    ++editPos;
            }
            break;

        case VK_HOME:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                editPos = 0;
            }
            break;

        case VK_END:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                editPos = find.size();
            }
            break;

        case VK_ESCAPE:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                cont = false;
            }
            break;

        case VK_DELETE:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                if (editPos < find.size())
                {
                    find.erase(find.begin() + editPos);
                    FindCB(data, true, false);
                }
            }
            break;

        case VK_BACK:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                if (editPos > 0)
                {
                    --editPos;
                    find.erase(find.begin() + editPos);
                    FindCB(data, true, false);
                }
            }
            break;

        case VK_RETURN:
            if ((ker.dwControlKeyState & 0x1F) == 0)
            {
                FindCB(data, true, true);
            }
            break;

        case VK_F3:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
                FindCB(data, true, true);
                break;

            case SHIFT_PRESSED:
                FindCB(data, false, true);
                break;
            }
            break;

        case 'C':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                CopyToClipboard(find.c_str(), find.length());
                break;
            }
            break;

        case 'I':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                ToggleCaseSensitive();
                FindCB(data, true, false);
                break;
            }
            break;

        case 'V':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                {
                    OpenClipboard(0);
                    HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
                    if (hMem != NULL)
                    {
                        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                        size_t l = find.length();
                        find.insert(editPos, pMem);
                        editPos += find.length() - l;
                        GlobalUnlock(hMem);
                        FindCB(data, true, false);
                    }
                    CloseClipboard();
                }
                break;
            }
            break;
        };

        if (((ker.dwControlKeyState & 0x1F) == 0 || (ker.dwControlKeyState & 0x1F) == SHIFT_PRESSED)
            && iswprint(ker.uChar.UnicodeChar))
        {
            if (find.size() < (buffer.size.X - 2))
            {
                find.insert(find.begin() + editPos, ker.uChar.UnicodeChar);
                ++editPos;
                FindCB(data, true, false);
            }
            else
                Beep();
        }
    };
    return cont;
}
