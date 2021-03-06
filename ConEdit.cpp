#include "stdafx.h"

#include "AboutMessage.H"
#include "FileIO.h"
#include "ConUtils.h"
#include "TextUtils.h"
#include "FileInfo.h"
#include "Mode.h"
#include "ModeInfoBox.h"
#include "ModePromptYesNo.h"
#include "ModePromptInteger.h"
#include "ModeEdit.h"
#include "ModeStatus.h"
#include "ModeFind.h"

#include <SyntaxHighlighterBrushes.h>

#include "resource.h"

#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008

// TODO
// Full editing in ModeFind and ModePromptInteger
// Use multi-char for unprintable characters ie ESC
// Search and replace
// Regular expression search
// Add wrapping?
// Tab insert space/tabs
// Add toolbar buttons for search case-sensitive
// Add toolbar buttons for show whitespace?
// Auto-save?
// Auto-revert if not editied?
// User defined scheme
// Syntax highlighting
// Caching offset to line number
// Virtual offset in anchor?
// Capture close by 'X' button

void AssertMsg(const char* test, const char* msg, const char* file, int line)
{
    char b[1024];
    if (msg)
        sprintf_s(b, "%s:%d %s: %s", file, line, test, msg);
    else
        sprintf_s(b, "%s:%d %s", file, line, test);
    throw std::exception(b);
}

std::wstring convert(const char* m)
{
    wchar_t msg[1024];
    MultiByteToWideChar(CP_UTF8, 0, m, -1, msg, ARRAYSIZE(msg));
    return msg;
}

class ModeConEdit : public Mode
{
public:
    ModeConEdit(HKEY hKey,
        FileInfo& fileInfo,
        COORD size,
        std::vector<wchar_t>& chars)
        : fileInfo(fileInfo)
        , me(size - _COORD(0, 1), !fileInfo.exists, fileInfo.exists && (fileInfo.stat.st_mode & _S_IWRITE) == 0, chars)
        , ms(_COORD(size.X, 1), _COORD(0, me.buffer.size.Y), me, fileInfo, chars)
        , mf(this, _COORD(20, 1), _COORD(me.buffer.size.X - 20, 0), FindCB, this)
    {
        me.LoadTheme(hKey);
        const wchar_t* ext = wcsrchr(fileInfo.filename, L'.');
        ext = (ext != nullptr) ? ext + 1 : L"";
        const Brush* b = brushes.getBrushByExtension(ext);
        me.SetBrush(b);
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const
    {
        bool invalid = me.invalid;
        me.Draw(screen, scheme, focus);

        if (invalid)
        {
            mf.Draw(screen, scheme, false);
            ms.Draw(screen, scheme, false);
        }
    }

    bool Do(const KEY_EVENT_RECORD& ker, Screen& screen, const EditScheme& scheme)
    {
        bool cont = true;
        if (ker.bKeyDown)
        {
            const bool readOnly = fileInfo.isreadonly();

            switch (ker.wVirtualKeyCode)
            {
            case VK_F1:
                {
                    wchar_t* pWData = nullptr;
                    {
                        HMODULE hModule = NULL;
                        Memory mem = GetResource(hModule, IDR_TEXT);
                        const char* pData = static_cast<const char*>(mem.data);

                        pWData = new wchar_t[mem.size + 1];
                        for (DWORD i = 0; i < mem.size; ++i)
                        {
                            switch (pData[i])
                            {
                            case '\r':
                                pWData[i] = L' ';
                                break;
                            case '\n':
                                pWData[i] = L'\0';
                                break;
                            default:
                                pWData[i] = pData[i];
                                break;
                            }
                        }
                        pWData[mem.size] = L'\0';
                    }

                    ModeInfoBox mmb(this, _COORD(4, 4), pWData);
                    DoMode(mmb, screen, scheme);

                    delete[] pWData;
                    me.invalid = true;
                }
                break;

            case VK_F3:
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    if (me.Selection().empty())
                    {
                        size_t p = MoveWordBegin(me.Chars(), me.Selection().end);

                        if (iswblank(me.Chars()[p])) // TODO Use locale?
                            break;  // Don't search

                        me.MoveCursor(p, false);
                        me.MoveCursor(MoveWordEnd(me.Chars(), me.Selection().end), true);
                    }

                    if (!me.Selection().empty())
                    {
                        mf.setFind(me.GetSelectedText());
                        mf.visible = true;
                        me.invalid = true;
                    }
                    Find(true, true);
                    break;

                case 0:
                    Find(true, true);
                    break;

                case SHIFT_PRESSED:
                    Find(false, true);
                    break;
                }
                break;

            case 'F':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    // TODO Set focus to find window to edit search text
                    mf.visible = true;
                    DoMode(mf, screen, scheme);
                    me.invalid = true;
                    break;
                }
                break;

            case 'G':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    {
                        ModePromptInteger mpi(this, ms.buffer, ms.pos, L"Goto line?");
                        DoMode(mpi, screen, scheme);
                        int line = mpi.Ret();
                        if (line > 0)
                        {
                            me.MoveCursor(GetLineOffset(me.Chars(), line), false);
                        }
                        me.invalid = true;
                    }
                    break;
                }
                break;

            case 'I':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    mf.ToggleCaseSensitive();
                    Find(true, false);
                    break;
                }
                break;

            case 'R':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    me.Revert(fileInfo);
                    break;
                }
                break;

            case 'S':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    if (!readOnly)
                    {
                        try
                        {
                            me.Save(fileInfo);
                            ms.message = L"Saved";
                        }
                        catch (const std::exception& e)
                        {
                            ms.error = convert(e.what());
                            me.invalid = true;
                        }
                    }
                    else
                        Beep();
                    break;
                }
                break;

            case 'X':
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_ALT_PRESSED:
                case LEFT_ALT_PRESSED:
                    {
                        if (me.isEdited())
                        {
                            ModePromptYesNo mpyn(this, ms.buffer, ms.pos, L"Save?");
                            DoMode(mpyn, screen, scheme);
                            WORD key = mpyn.Ret();
                            me.invalid = true;
                            if (key == L'Y')
                                me.Save(fileInfo);
                            if (key != 0)
                                cont = false;
                        }
                        else
                            cont = false;

                    }
                    break;
                }
                break;
            }
        }

        if (cont)
            cont = me.Do(ker, screen, scheme);

        return cont;
    }

    bool Do(const MOUSE_EVENT_RECORD& mer, Screen& screen, const EditScheme& scheme)
    {
        bool cont = true;
        COORD pos = mer.dwMousePosition - me.pos;
        if (pos.X >= 0 && pos.Y >= 0 && pos.X < me.buffer.size.X && pos.Y < me.buffer.size.Y)
        {
            MOUSE_EVENT_RECORD lmer = mer;
            lmer.dwMousePosition = pos;
            cont = me.Do(mer, screen, scheme);
        }
        return cont;
    }

    void Do(const FOCUS_EVENT_RECORD& fer, Screen& screen, const EditScheme& scheme)
    {
        if (fer.bSetFocus)
        {
            bool modeChanged = false;
            if (fileInfo.changed(modeChanged))
            {
                ModePromptYesNo mpyn(this, ms.buffer, ms.pos, L"File changed, Revert?");
                DoMode(mpyn, screen, scheme);
                WORD key = mpyn.Ret();
                me.readOnly = fileInfo.isreadonly();
                me.invalid = true;
                if (key == 'Y')
                    me.Revert(fileInfo);
            }
            if (modeChanged)
                me.invalid = true;
        }
    }

    void Do(const WINDOW_BUFFER_SIZE_RECORD& wbsr)
    {
        me.buffer.resize(wbsr.dwSize - _COORD(0, 1));
        ms.buffer.resize(_COORD(me.buffer.size.X, 1));
        ms.pos = _COORD(0, me.buffer.size.Y);
        mf.pos = _COORD(me.buffer.size.X - mf.buffer.size.X, 0);
        me.invalid = true;
    }

    static void FindCB(void* data, bool forward, bool next)
    {
        static_cast<ModeConEdit*>(data)->Find(forward, next);
    }

    void Find(bool forward, bool next)
    {
        if (!me.Find(mf.Find(), mf.CaseSensitive(), forward, next))
            ms.message = L"Not found: " + mf.Find();
    }

    void Assert() const
    {
        me.Assert();
    }

private:
    FileInfo& fileInfo;
    ModeEdit me;
    ModeStatus ms;
    ModeFind mf;
    SyntaxHighlighterBrushes brushes;
};

int wmain(int argc, const wchar_t* argv[])
{
    try
    {
        if (argc < 2)
        {
            DisplayAboutMessage(NULL);
            DisplayResource(NULL, IDR_TEXT);
            return 1;
        }

        FileInfo fileInfo(argv[1]);
        std::vector<wchar_t> chars;
        if (fileInfo.exists)
            chars = fileInfo.load();

        HWND hWnd = GetConsoleWindow();
        HICON hIcon = (HICON) SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON)));

        const Screen screenOrig;
        AutoRestoreBufferInfo arbi(screenOrig.hOut);
        AutoRestoreMode armIn(screenOrig.hIn, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT, 0xFFFF);
        AutoRestoreMode armOut(screenOrig.hOut, 0, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);

        Screen screen(screenOrig);
        screen.hOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);

        HKEY hKey = NULL;
        RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\RadSoft\\ConEdit", &hKey);

        CONSOLE_SCREEN_BUFFER_INFOEX csbi = arbi.get();
        csbi.dwSize = GetSize(screen.csbi.srWindow);
        EditScheme scheme(hKey, csbi);

        csbi.wAttributes = 0;
        scheme.get(EditScheme::DEFAULT).apply(csbi.wAttributes);
        SetConsoleScreenBufferInfoEx(screen.hOut, &csbi);

        AutoRestoreActiveScreenBuffer arasb(screenOrig.hOut, screen.hOut);

        ModeConEdit mce(hKey, fileInfo, GetSize(screen.csbi.srWindow), chars);

        RegCloseKey(hKey);
        hKey = NULL;

        DoMode(mce, screen, scheme);

        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

        return 0;
    }
    catch (const std::exception& e)
    {
        printf(e.what());
        printf("\n");
        return 1;
    }
}
