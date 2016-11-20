
class ModePromptInteger : public ModeBase
{
public:
    ModePromptInteger(Mode* base, Buffer& status, const COORD& pos, const wchar_t* p)
        : ModeBase(base)
        , buffer(status)
        , pos(pos)
        , p(p)
        , len(wcslen(p))
        , ret(0)
    {
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const
    {
        ModeBase::Draw(screen, scheme, false);

        WORD wAttribute = 0;
        scheme.get(EditScheme::STATUSFOCUS).apply(wAttribute);
        buffer.clear(wAttribute, L' ');

        WriteBegin(buffer, _COORD(1, 0), p);
        WriteBegin(buffer, _COORD(static_cast<SHORT>(len + 2), 0), pk.c_str());
        DrawBuffer(screen.hOut, buffer, pos);

        if (focus)
        {
            SetCursorVisible(screen.hOut, screen.cci, TRUE);
            COORD cursor = { static_cast<SHORT>(len + pk.size() + 2), 0 };
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
            else if (pk.size() < 5 && ker.wVirtualKeyCode >= L'0' && ker.wVirtualKeyCode <= L'9')
            {
                pk.push_back(ker.uChar.UnicodeChar);
            }
            else if (!pk.empty() && ker.wVirtualKeyCode == VK_BACK)
            {
                pk.pop_back();
            }
            else if (!pk.empty() && ker.wVirtualKeyCode == VK_RETURN)
            {
                ret = _wtoi(pk.c_str());
                cont = false;
            }
            else
                Beep();
        };
        return cont;
    }

    bool Do(const MOUSE_EVENT_RECORD& /*mer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }

    int Ret() const { return ret; }

private:
    Buffer& buffer;
    const COORD& pos;
    const wchar_t* p;
    size_t len;
    std::wstring pk;
    int ret;
};
