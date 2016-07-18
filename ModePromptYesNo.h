
class ModePromptYesNo : public ModeBase
{
public:
    ModePromptYesNo(Mode* base, Buffer& status, COORD& pos, const wchar_t* p)
        : ModeBase(base)
        , buffer(status)
        , pos(pos)
        , p(p)
        , len(wcslen(p))
        , pk(L"[YN]")
        , ret(0)
    {
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const
    {
        ModeBase::Draw(screen, scheme, false);

        buffer.clear(scheme.wAttrStatusFocus, L' ');

        WriteBegin(buffer, _COORD(1, 0), p);
        WriteBegin(buffer, _COORD(static_cast<SHORT>(len + 2), 0), pk.c_str());
        DrawBuffer(screen.hOut, buffer, pos);

        if (focus)
        {
            SetCursorVisible(screen.hOut, screen.cci, TRUE);
            COORD cursor = { static_cast<SHORT>(len + pk.size() + 3), 0 };
            SetConsoleCursorPosition(screen.hOut, cursor + pos);
        }
    }

    bool Do(const KEY_EVENT_RECORD& ker, Screen& /*screen*/, const EditScheme& /*scheme*/)
    {
        bool cont = true;
        if (ker.bKeyDown)
        {
            if (ker.wVirtualKeyCode == VK_ESCAPE)
            {
                ret = 0;
                cont = false;
            }
            else if (ker.wVirtualKeyCode == 'Y' || ker.wVirtualKeyCode == 'N')
            {
                ret = ker.wVirtualKeyCode;
                cont = false;
            }
            else
                Beep();
        };
        return cont;
    }

    bool Do(const MOUSE_EVENT_RECORD& /*mer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }

    WORD Ret() const { return ret; }

private:
    Buffer& buffer;
    const COORD& pos;
    const wchar_t* p;
    size_t len;
    const std::wstring pk;
    WORD ret;
};
