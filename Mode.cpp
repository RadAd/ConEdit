#include "stdafx.h"

#include "Mode.h"

#include "ConUtils.h"

const wchar_t* EditScheme::DEFAULT = L"Default";
const wchar_t* EditScheme::SELECTED = L"Selected";
const wchar_t* EditScheme::WHITESPACE = L"Whitespace";
const wchar_t* EditScheme::NONPRINT = L"NonPrint";
const wchar_t* EditScheme::STATUS = L"Status";
const wchar_t* EditScheme::STATUSFOCUS = L"StatusFocus";
const wchar_t* EditScheme::ERROR_ = L"Error";

EditScheme::EditScheme(HKEY hKey, CONSOLE_SCREEN_BUFFER_INFOEX& csbi)
{
    attr[DEFAULT] = Attribute(csbi.wAttributes, FOREGROUND_MASK | BACKGROUND_MASK);
    attr[SELECTED] = Attribute(BACKGROUND_BLUE | BACKGROUND_INTENSITY, BACKGROUND_MASK);
    attr[WHITESPACE] = Attribute(FOREGROUND_DGREY, FOREGROUND_MASK);
    attr[NONPRINT] = Attribute(BACKGROUND_BLACK | FOREGROUND_WHITE | COMMON_LVB_REVERSE_VIDEO, FOREGROUND_MASK | BACKGROUND_MASK | COMMON_LVB_REVERSE_VIDEO);
    attr[STATUS] = Attribute(BACKGROUND_LGREY | FOREGROUND_BLACK, FOREGROUND_MASK | BACKGROUND_MASK);
    attr[STATUSFOCUS] = Attribute(BACKGROUND_BLUE | FOREGROUND_WHITE, FOREGROUND_MASK | BACKGROUND_MASK);
    attr[ERROR_] = Attribute(BACKGROUND_RED | FOREGROUND_WHITE, FOREGROUND_MASK | BACKGROUND_MASK);

    if (hKey != NULL)
    {
        HKEY hSchemeKey = NULL;
        RegOpenKey(hKey, L"Scheme", &hSchemeKey);
        if (hSchemeKey != NULL)
        {
            wchar_t Scheme[256];
            LONG size = ARRAYSIZE(Scheme) * sizeof(wchar_t);
            if (RegQueryValue(hSchemeKey, nullptr, Scheme, &size) == ERROR_SUCCESS)
            {
                HKEY hSubSchemeKey = NULL;
                RegOpenKey(hSchemeKey, Scheme, &hSubSchemeKey);
                if (hSubSchemeKey != NULL && hSubSchemeKey != hSchemeKey)
                {
                    for (int i = 0; i < 16; ++i)
                    {
                        wchar_t Entry[256];
                        wsprintf(Entry, L"ColorTable%02d", i);
                        DWORD Value = 0;
                        DWORD Size = sizeof(Value);
                        if (RegQueryValueEx(hSubSchemeKey, Entry, nullptr, nullptr, (LPBYTE) &Value, &Size) == ERROR_SUCCESS)
                            csbi.ColorTable[i] = Value;
                    }

                    const wchar_t* names[] = {
                        DEFAULT,
                        SELECTED,
                        WHITESPACE,
                        NONPRINT,
                        STATUS,
                        STATUSFOCUS,
                        ERROR_,
                    };
                    for (const wchar_t* name : names)
                    {
                        DWORD Value = 0;
                        DWORD Size = sizeof(Value);
                        if (RegQueryValueEx(hSubSchemeKey, name, nullptr, nullptr, (LPBYTE) &Value, &Size) == ERROR_SUCCESS)
                            attr[name] = Attribute(Value & 0xFFFF, (Value >> (sizeof(WORD) * 8)) & 0xFFFF);
                    }
                    RegCloseKey(hSubSchemeKey);
                }
            }
            RegCloseKey(hSchemeKey);
        }
    }
}

Attribute EditScheme::get(const wchar_t* s) const
{
    auto it = attr.find(s);
    if (it == attr.end())
        it = attr.find(DEFAULT);
    if (it != attr.end())
        return it->second;
    else
        return Attribute(0x0F, FOREGROUND_MASK | BACKGROUND_MASK);;
}

void DoMode(Mode& m, Screen& screen, const EditScheme& scheme)
{
    bool cont = true;
    while (cont)
    {
        m.Draw(screen, scheme, true);

        INPUT_RECORD ir = {};
        DWORD er = 0;
        ReadConsoleInput(screen.hIn, &ir, 1, &er);

        switch (ir.EventType)
        {
        case KEY_EVENT:
            if (ir.Event.KeyEvent.wVirtualKeyCode == VK_F12 && (ir.Event.KeyEvent.dwControlKeyState & 0x1F) == 0)
            {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(screen.hOut, &csbi);
                if (csbi.dwSize != GetSize(csbi.srWindow))
                    SetConsoleScreenBufferSize(screen.hOut, GetSize(csbi.srWindow));
            }
            else
                cont = m.Do(ir.Event.KeyEvent, screen, scheme);
            break;

        case MOUSE_EVENT:
            cont = m.Do(ir.Event.MouseEvent, screen, scheme);
            break;

        case FOCUS_EVENT:
            m.Do(ir.Event.FocusEvent, screen, scheme);
            break;

        case WINDOW_BUFFER_SIZE_EVENT:
            m.Do(ir.Event.WindowBufferSizeEvent);
            break;
        }
#if 1
        m.Assert();
#endif
    }
}
