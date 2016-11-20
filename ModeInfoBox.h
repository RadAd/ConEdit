

// TODO
// Support scroll??

class ModeInfoBox : public ModeBase
{
public:
    ModeInfoBox(Mode* base, COORD padding, const wchar_t* p)
        : ModeBase(base)
        , buffer(padding)   // padding isnt the right size but it will get fixed up on the first draw call
        , padding(padding)
        , p(p)
        , len(wcslen(p))
    {
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const
    {
        ModeBase::Draw(screen, scheme, false);

        COORD size = GetSize(screen.csbi.srWindow);
        if (buffer.size != (size - padding))
            buffer.resize(size - padding);

        WORD wAttribute = 0;
        scheme.get(EditScheme::STATUSFOCUS).apply(wAttribute);
        buffer.clear(wAttribute, L' ');

        SHORT y = 0;
        const wchar_t* e = p;
        while (*e != L'\0' && y < buffer.size.Y)
        {
            WriteBegin(buffer, _COORD(0, y), e);
            e += wcslen(e) + 1;
            ++y;
        }
        COORD pos = { (size.X - buffer.size.X) / 2,  (size.Y - buffer.size.Y) / 2 };
        DrawBuffer(screen.hOut, buffer, pos);

        if (focus)
        {
            SetCursorVisible(screen.hOut, screen.cci, FALSE);
        }
    }

    bool Do(const KEY_EVENT_RECORD& ker, Screen& /*screen*/, const EditScheme& /*scheme*/)
    {
        bool cont = true;
        if (ker.bKeyDown)
        {
            if (ker.wVirtualKeyCode == VK_ESCAPE || ker.wVirtualKeyCode == VK_RETURN)
            {
                cont = false;
            }
            else
                Beep();
        };
        return cont;
    }

    bool Do(const MOUSE_EVENT_RECORD& /*mer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }
    void Do(const WINDOW_BUFFER_SIZE_RECORD& wbsr)
    {
        ModeBase::Do(wbsr);
    }

private:
    mutable Buffer buffer;
    COORD padding;
    const wchar_t* p;
    size_t len;
};
