#ifndef CONUTILS_H
#define CONUTILS_H

#define FOREGROUND_MASK (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)
#define BACKGROUND_MASK (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

#define FOREGROUND_BLACK (0)
#define FOREGROUND_DGREY (FOREGROUND_INTENSITY)
#define FOREGROUND_LGREY (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define FOREGROUND_WHITE (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)

#define BACKGROUND_BLACK (0)
#define BACKGROUND_DGREY (BACKGROUND_INTENSITY)
#define BACKGROUND_LGREY (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED)
#define BACKGROUND_WHITE (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

inline void Beep()
{
    //Beep(323, 200);
    MessageBeep(MB_ICONERROR);
}

void CopyToClipboard(const wchar_t* pSrc, size_t len);

struct Screen
{
    Screen();

    HANDLE hIn;
    HANDLE hOut;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    CONSOLE_CURSOR_INFO cci;
};

inline Screen::Screen()
{
    hIn = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hOut, &csbi);
    GetConsoleCursorInfo(hOut, &cci);
}

inline COORD _COORD(SHORT X, SHORT Y)
{
    COORD c = { X, Y };
    return c;
}

inline COORD operator+(COORD a, COORD b)
{
    a.X += b.X;
    a.Y += b.Y;
    return a;
}

inline COORD operator-(COORD a, COORD b)
{
    a.X -= b.X;
    a.Y -= b.Y;
    return a;
}

inline bool operator==(COORD a, COORD b)
{
    return a.X == b.X && a.Y == b.Y;
}

inline bool operator!=(COORD a, COORD b)
{
    return a.X != b.X || a.Y != b.Y;
}

inline COORD GetSize(const SMALL_RECT& r)
{
    COORD c;
    c.X = r.Right - r.Left + 1;
    c.Y = r.Bottom - r.Top + 1;
    return c;
}

inline SMALL_RECT Rectangle(COORD topleft, COORD size)
{
    SMALL_RECT r;
    r.Top = topleft.Y;
    r.Left = topleft.X;
    r.Bottom = r.Top + size.Y - 1;
    r.Right = r.Left + size.X - 1;
    return r;
}

void SetCursorVisible(HANDLE hScreen, CONSOLE_CURSOR_INFO& cci, BOOL bVisible);

struct Buffer
{
    Buffer(COORD s)
        : size(s)
        , data(s.X * s.Y)
    {
    }

    void clear(WORD wAttr, wchar_t c)
    {
        for (CHAR_INFO& ci : data)
        {
            ci.Attributes = wAttr;
            ci.Char.UnicodeChar = c;
        }
    }

    void resize(COORD s)
    {
        size = s;
        data.resize(s.X * s.Y);
    }

    size_t offset(COORD s)
    {
        return s.Y * size.X + s.X;
    }

    COORD size;
    std::vector<CHAR_INFO> data;
};

void DrawBuffer(HANDLE hScreen, const Buffer& buffer, COORD posBuffer);
void WriteBegin(Buffer& b, COORD o, const wchar_t* s);
void WriteEnd(Buffer& b, COORD o, const wchar_t* s);

class AutoRestoreMode
{
public:
    AutoRestoreMode(HANDLE h, WORD newModeSet, WORD newModeMask)
        : hHandle(h)
    {
        GetConsoleMode(hHandle, &mode);
        WORD newMode = (mode & newModeMask) | newModeSet;
        SetConsoleMode(hHandle, newMode);
    }

    ~AutoRestoreMode()
    {
        SetConsoleMode(hHandle, mode);
    }

private:
    HANDLE hHandle;
    DWORD mode;
};

class AutoRestoreBufferInfo
{
public:
    AutoRestoreBufferInfo(HANDLE h)
        : hHandle(h)
    {
        csbi.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        GetConsoleScreenBufferInfoEx(hHandle, &csbi);
    }

    ~AutoRestoreBufferInfo()
    {
        SetConsoleScreenBufferInfoEx(hHandle, &csbi);
    }

    const CONSOLE_SCREEN_BUFFER_INFOEX& get() const { return csbi; }

private:
    HANDLE hHandle;
    CONSOLE_SCREEN_BUFFER_INFOEX csbi;
};

class AutoRestoreActiveScreenBuffer
{
public:
    AutoRestoreActiveScreenBuffer(HANDLE hOrig, HANDLE hNew)
        : hHandle(hOrig)
    {
        SetConsoleActiveScreenBuffer(hNew);
    }

    ~AutoRestoreActiveScreenBuffer()
    {
        SetConsoleActiveScreenBuffer(hHandle);
    }

private:
    HANDLE hHandle;
};

#endif
