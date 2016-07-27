#include "stdafx.h"

#include "Mode.h"

#include "ConUtils.h"

EditScheme::EditScheme(WORD wAttributes)
{
    wAttrDefault = Attribute(wAttributes, FOREGROUND_MASK | BACKGROUND_MASK);
    wAttrSelected = Attribute(BACKGROUND_BLUE | BACKGROUND_INTENSITY, BACKGROUND_MASK);
    wAttrWhiteSpace = Attribute(FOREGROUND_DGREY, FOREGROUND_MASK);
    //wAttrNonPrint = _halfbyteswap_word(wAttrDefault);
    wAttrNonPrint = Attribute(BACKGROUND_BLACK | FOREGROUND_WHITE | COMMON_LVB_REVERSE_VIDEO, FOREGROUND_MASK | BACKGROUND_MASK | COMMON_LVB_REVERSE_VIDEO);
    wAttrStatus = Attribute(BACKGROUND_LGREY | FOREGROUND_BLACK, FOREGROUND_MASK | BACKGROUND_MASK);
    wAttrStatusFocus = Attribute(BACKGROUND_BLUE | FOREGROUND_WHITE, FOREGROUND_MASK | BACKGROUND_MASK);
    wAttrError = Attribute(BACKGROUND_RED | FOREGROUND_WHITE, FOREGROUND_MASK | BACKGROUND_MASK);
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
