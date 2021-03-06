#include "stdafx.h"

#include "ModeEdit.h"

#include "Mode.h"
#include "FileInfo.h"
#include "BoyerMoore.h"

#include <Brush.h>
#include <SyntaxHighlighter.h>

// Note
// The attribute span is not updated in the undo/redo stack.
// This isn't an issue as the syntax highlighter recalculates it.

template<class V>
bool MapSpanNotExist(std::map<Span, V>& map, const Span& span)
{
    auto itPair = map.equal_range(span);
    return itPair.first == itPair.second;
}

template<class V>
void MapSpanAdd(std::map<Span, V>& map, const Span& span, const V& value)
{
    assert(span.end > span.begin);
    assert(MapSpanNotExist(map, span));
    map[span] = value;
}

template<class V>
typename std::map<Span, V>::iterator MapSpanMove(std::map<Span, V>& map, typename std::map<Span, V>::iterator e, const Span& span)
{
    assert(span.end > span.begin);
    V v = e->second;
    e = map.erase(e);
    assert(MapSpanNotExist(map, span));
    return map.insert(e, std::map<Span, V>::value_type(span, std::move(v)));
}

template<class V>
void MapSpanCollapse(std::map<Span, V>& map, const Span& span)
{
    std::map<Span, V>::iterator e = map.lower_bound(span);
    if (e != map.end() && e->first != span && e->first.begin < span.end && span.begin < e->first.end)
    {
        if (e->first.end > span.end)
        {
            if (span.begin > e->first.begin)
            {
                Span rn(e->first);
                rn.end -= span.length();
                assert(!rn.empty());
                e = std::next(MapSpanMove(map, e, rn));
            }
            else
            {
                Span rn(e->first);
                rn.begin = span.end;
                rn.begin -= span.length();
                rn.end -= span.length();
                assert(!rn.empty());
                e = std::next(MapSpanMove(map, e, rn));
            }
        }
        else if (e->first.end > span.begin)
        {
            Span rn(e->first);
            rn.end = span.begin;
            if (rn.empty())
                e = map.erase(e);
            else
                e = std::next(MapSpanMove(map, e, rn));
        }
    }
    while (e != map.end() && e->first.end <= span.end)
    {
        e = map.erase(e);
    }
    if (e != map.end() && e->first.begin < span.end)
    {
        Span rn(e->first);
        rn.begin = span.end;
        rn.begin -= span.length();
        rn.end -= span.length();
        assert(!rn.empty());
        e = std::next(MapSpanMove(map, e, rn));
    }
    while (e != map.end())
    {
        Span rn(e->first);
        rn.begin -= span.length();
        rn.end -= span.length();
        e = std::next(MapSpanMove(map, e, rn));
    }
}

template<class V>
void MapSpanInflate(std::map<Span, V>& map, const Span& span)
{
    const std::map<Span, V>::iterator eFound = map.lower_bound(span);
    if (eFound != map.end())
    {
        std::map<Span, V>::iterator e = std::prev(map.end());
        while (e != eFound && e->first.begin >= span.begin)
        {
            Span rn(e->first);
            rn.begin += span.length();
            rn.end += span.length();
            e = std::prev(MapSpanMove(map, e, rn));
        }

        Span rn(e->first);
        if (rn.begin >= span.begin)
            rn.begin += span.length();
        rn.end += span.length();
        e = std::prev(MapSpanMove(map, e, rn));
    }
}

std::vector<wchar_t> GetEOL(const std::vector<wchar_t>& chars)
{
    size_t e = GetEndOfLine(chars, 0);
    size_t len = IsEOL(chars, e);
    if (len > 0)
        return std::vector<wchar_t>(chars.begin() + e, chars.begin() + e + len);
    else
    {
        const wchar_t* eolDefault = L"\r\n";
        return std::vector<wchar_t>(eolDefault, eolDefault + wcslen(eolDefault));
    }
}

Anchor MakeVisible(Anchor a, COORD s, const std::vector<wchar_t>& chars, size_t tabSize, size_t p)
{
    Anchor pa(chars, tabSize, p);
    if (pa.startLine < a.startLine)
        a.startLine = pa.startLine;
    else
    {
        size_t charsStartOfLastLine = a.startLine;
        {
            for (int l = 0; l < s.Y; ++l)
                charsStartOfLastLine = GetStartOfNextLine(chars, charsStartOfLastLine);
        }
        if (pa.startLine >= charsStartOfLastLine)
        {
            a.startLine = pa.startLine;
            {
                for (int l = 1; l < s.Y; ++l)
                    a.startLine = GetStartOfPrevLine(chars, a.startLine);
            }
        }
    }
    if (pa.column < a.column)
        a.column = pa.column;
    else if (pa.column >(a.column + s.X - 1))
        a.column = pa.column - s.X + 1;
    return a;
}

ModeEdit::ModeEdit(const COORD& bufferSize, bool edited, bool readOnly, std::vector<wchar_t>& chars)
    : buffer(bufferSize)
    , pos(_COORD(0, 0))
    , readOnly(readOnly)
    , chars(chars)
    , selection(0, 0)
    , brush(nullptr)
    , showWhiteSpace(false)
    , tabSize(4)
    , undoCurrent(0)
    , invalid(true)
    , undoSaved(edited ? UINT_MAX : 0)
{
    eol = GetEOL(chars);

#if 0
    brushTheme[Brush::PREPROCESSOR] = Attribute(FOREGROUND_INTENSITY, FOREGROUND_MASK);
    brushTheme[Brush::COMMENTS] = Attribute(FOREGROUND_GREEN, FOREGROUND_MASK);
    brushTheme[Brush::KEYWORD] = Attribute(FOREGROUND_BLUE | FOREGROUND_INTENSITY, FOREGROUND_MASK);
    brushTheme[Brush::TYPE] = Attribute(FOREGROUND_BLUE | FOREGROUND_GREEN, FOREGROUND_MASK);
    brushTheme[Brush::FUNCTION] = Attribute(FOREGROUND_RED | FOREGROUND_GREEN, FOREGROUND_MASK);
    brushTheme[Brush::STRING] = Attribute(FOREGROUND_RED | FOREGROUND_BLUE, FOREGROUND_MASK);
    brushTheme[Brush::VALUE] = Attribute(FOREGROUND_RED | FOREGROUND_BLUE, FOREGROUND_MASK);
    //brushTheme[Brush::VARIABLE] = Attribute(   );
    //brushTheme[Brush::CONSTANT] = Attribute(   );
    //brushTheme[Brush::COLOR1] = Attribute(   );
    //brushTheme[Brush::COLOR2] = Attribute(   );
    //brushTheme[Brush::COLOR3] = Attribute(   );
#endif
}

void ModeEdit::FillCharsWindow(const EditScheme& scheme, COORD& cursor) const
{
    Attribute wAttrDefault = scheme.get(EditScheme::DEFAULT);
    Attribute wAttrSelected = scheme.get(EditScheme::SELECTED);
    Attribute wAttrWhiteSpace = scheme.get(EditScheme::WHITESPACE);
    Attribute wAttrNonPrint = scheme.get(EditScheme::NONPRINT);

    std::vector<CHAR_INFO>::iterator itW = buffer.data.begin();
    Anchor a = GetAnchor();
    std::map<Span, Attribute>::const_iterator itAttribute = attributes.lower_bound(Span(a.startLine, a.startLine));
    for (SHORT Y = 0; Y < buffer.size.Y; ++Y)
    {
        size_t p = a.ToOffset(chars, tabSize);
        size_t charsEndOfLine = GetEndOfLine(chars, a.startLine);
        bool PastEOL = a.startLine == UINT_MAX || p > charsEndOfLine;
        size_t tab = 0;
        for (SHORT X = 0; X < buffer.size.X; ++X)
        {
            itW->Attributes = 0;
            wAttrDefault.apply(itW->Attributes);
            itW->Char.UnicodeChar = L' ';
            if (p == selection.end && !PastEOL)
            {
                cursor.X = X;
                cursor.Y = Y;
            }
            //PastEOL = PastEOL || p >= chars.size() || IsEOL(chars[p]) || chars[p] == '\0';
            PastEOL = PastEOL || p > charsEndOfLine;
            if (!PastEOL)
            {
                if (tab > 0)
                {
                    if (itAttribute != attributes.end() && itAttribute->first.isin(p))
                        itAttribute->second.apply(itW->Attributes);
                    if (selection.isin(p - 1))
                        wAttrSelected.apply(itW->Attributes);
                    --tab;
                }
                else
                {
                    while (itAttribute != attributes.end() && itAttribute->first.end <= p)
                        ++itAttribute;
                    if (itAttribute != attributes.end() && itAttribute->first.isin(p))
                        itAttribute->second.apply(itW->Attributes);
                    if (p >= chars.size())
                    {
                        itW->Char.UnicodeChar = L'~';
                        wAttrWhiteSpace.apply(itW->Attributes);
                    }
#if 1
                    else if (chars[p] == L'\t')
                    {
                        if (showWhiteSpace)
                        {
                            itW->Char.UnicodeChar = L'�';
                            wAttrWhiteSpace.apply(itW->Attributes);
                        }
                        tab = tabSize - 1 - ((a.column + X) % tabSize);
                    }
#endif
                    else if (iswblank(chars[p])) // TODO Use locale?
                    {
                        if (showWhiteSpace)
                        {
                            itW->Char.UnicodeChar = L'�';
                            wAttrWhiteSpace.apply(itW->Attributes);
                        }
                    }
                    else if (iswspace(chars[p]))
                    {
                        if (showWhiteSpace)
                        {
                            itW->Char.UnicodeChar = L'�';
                            wAttrWhiteSpace.apply(itW->Attributes);
                        }
                    }
                    else if (iswcntrl(chars[p])) // TODO Use locale?
                    {
                        switch (chars[p])
                        {
                        case 0x00: itW->Char.UnicodeChar = L'N'; break;   // Null
                        case 0x08: itW->Char.UnicodeChar = L'B'; break;   // Backspace
                        case 0x1B: itW->Char.UnicodeChar = L'E'; break;   // Escape
                        default: itW->Char.UnicodeChar = L'�'; break;
                        }
                        wAttrNonPrint.apply(itW->Attributes);
                    }
                    else
                        itW->Char.UnicodeChar = chars[p];
                    if (selection.isin(p))
                        wAttrSelected.apply(itW->Attributes);
                    ++p;
                }
            }
            else
            {
                itW->Char.UnicodeChar = L' ';
            }
            ++itW;
        }
        a.startLine = GetStartOfNextLine(chars, charsEndOfLine);
    }
}

void ModeEdit::Draw(Screen& screen, const EditScheme& scheme, bool focus) const
{
    if (invalid)
    {
        invalid = false;

        COORD cursor = { -1, -1 };
        FillCharsWindow(scheme, cursor);
        DrawBuffer(screen.hOut, buffer, pos);

        if (focus)
        {
            SetCursorVisible(screen.hOut, screen.cci, (cursor.X == -1 || cursor.Y == -1) ? FALSE : TRUE);
            if (screen.cci.bVisible)
                SetConsoleCursorPosition(screen.hOut, cursor + pos);
        }
    }
}

bool ModeEdit::Do(const KEY_EVENT_RECORD& ker, Screen& /*screen*/, const EditScheme& /*scheme*/)
{
    bool cont = true;
    if (ker.bKeyDown)
    {
        switch (ker.wVirtualKeyCode)
        {
        case VK_UP:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
            case SHIFT_PRESSED:
                MoveCursor(MoveUp(chars, tabSize, selection.end), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                MoveAnchorUp(1, false);
                break;
            }
            break;

        case VK_DOWN:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
            case SHIFT_PRESSED:
                MoveCursor(MoveDown(chars, tabSize, selection.end), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                MoveAnchorDown(1, false);
                break;
            }
            break;

        case VK_LEFT:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
                {
                    size_t p = selection.empty() ? MoveLeft(chars, selection.end) : selection.Begin();
                    MoveCursor(p, false);
                }
                break;

            case SHIFT_PRESSED:
                MoveCursor(MoveLeft(chars, selection.end), true);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
#if 0
                if (anchor.offset > 0)
                    --anchor.offset;
#else
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                MoveCursor(MoveWordLeft(chars, selection.end), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
#endif
                break;
            }
            break;

        case VK_RIGHT:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
                {
                    size_t p = selection.empty() ? MoveRight(chars, selection.end) : selection.End();
                    MoveCursor(p, false);
                }
                break;

            case SHIFT_PRESSED:
                MoveCursor(MoveRight(chars, selection.end), true);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
#if 0
                //if (anchor.offset < maxLineLength)
                ++anchor.offset;
#else
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                MoveCursor(MoveWordRight(chars, selection.end), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
#endif
                break;
            }
            break;

        case VK_HOME:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
            case SHIFT_PRESSED:
                {
                    size_t p = GetStartOfLine(chars, selection.end);
                    if (p == selection.end)
                        p = MoveWordRight(chars, p);
                    MoveCursor(p, (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                }
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                MoveCursor(0, (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                break;
            }
            break;

        case VK_PRIOR:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
                MoveAnchorUp(buffer.size.Y - 1, true);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                {
                    Anchor a(chars, tabSize, selection.end);
                    a.startLine = anchor.startLine;
                    size_t p = a.ToOffset(chars, tabSize);
                    size_t endOfLine = GetEndOfLine(chars, a.startLine);
                    if (p > endOfLine)
                        p = endOfLine;
                    MoveCursor(p, (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                }
                break;
            }
            break;

        case VK_NEXT:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
                MoveAnchorDown(buffer.size.Y - 1, true);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                {
                    Anchor a(chars, tabSize, selection.end);
                    a.startLine = anchor.startLine;
                    for (int i = 1; a.startLine < chars.size() && i < buffer.size.Y; ++i)
                        a.startLine = GetStartOfNextLine(chars, a.startLine);
                    size_t p = a.ToOffset(chars, tabSize);
                    size_t endOfLine = GetEndOfLine(chars, a.startLine);
                    if (p > endOfLine)
                        p = endOfLine;
                    MoveCursor(p, (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                }
                break;
            }
            break;

        case VK_END:
            switch (ker.dwControlKeyState & 0x1F)
            {
            case 0:
            case SHIFT_PRESSED:
                MoveCursor(GetEndOfLine(chars, selection.end), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                break;

            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                MoveCursor(GetStartOfLine(chars, chars.size()), (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                break;
            }
            break;

        case VK_DELETE:
            if (!readOnly)
            {
                Span span = selection;
                size_t p = UINT_MAX;
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    if (span.end >= span.begin)
                        span.end = MoveWordRight(chars, span.end);
                    break;

                case SHIFT_PRESSED:
                    if (span.empty())
                    {
                        span.begin = GetStartOfLine(chars, span.end);
                        span.end = GetStartOfNextLine(chars, span.end);
                        p = MoveDown(chars, tabSize, selection.end) - span.length();
                    }
                    break;

                case 0:
                    if (span.empty())
                        span.end = MoveRight(chars, span.end);
                    break;

                default:
                    span.end = span.begin; // Dont delete
                    break;
                }
                if (p != UINT_MAX)
                    Delete(span, p);
                else
                    Delete(span);
            }
            else
                Beep();
            break;

        case VK_BACK:
            if (!readOnly)
            {
                Span span = selection;
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    if (span.end <= span.begin)
                        span.end = MoveWordLeft(chars, span.end);
                    break;

                case 0:
                case SHIFT_PRESSED:
                    if (selection.empty())
                        span.end = MoveLeft(chars, span.end);
                    break;
                }
                Delete(span);
            }
            else
                Beep();
            break;

        case VK_RETURN:
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    {
                        size_t startOfLine = GetStartOfLine(chars, selection.end);
                        Insert(startOfLine, eol);
                        MoveCursor(MoveLeft(chars, selection.end), false);
                    }
                    break;

                case 0:
                    Insert(eol);
                    break;
                }
            }
            else
                Beep();
            break;

        case VK_TAB:
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case 0:
                    // TODO Indent lines
                    // TODO Indent spaces
                    // TODO Shift - remove tab spaces
                    Insert(L'\t');
                    break;
                }
            }
            else
                Beep();
            break;

        case VK_OEM_6: //  ']}' for US
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
            case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
            case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                {
                    size_t p = FindMatchingBrace(chars, selection.end);
                    if (p != UINT_MAX)
                        MoveCursor(p, (ker.dwControlKeyState & SHIFT_PRESSED) != 0);
                }
                break;
            }
            break;

        case 'A':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                MoveCursor(0, false);
                MoveCursor(chars.size(), true);
                break;
            }
            break;

        case 'C':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                CopyToClipboard();
                break;
            }
            break;

        case 'U':
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    {
                        if (selection.empty())
                            MoveCursor(MoveRight(chars, selection.end), true);
                        Transform(towlower);
                    }
                    break;

                case RIGHT_CTRL_PRESSED | SHIFT_PRESSED:
                case LEFT_CTRL_PRESSED | SHIFT_PRESSED:
                    {
                        if (selection.empty())
                            MoveCursor(MoveRight(chars, selection.end), true);
                        Transform(towupper);
                    }
                    break;
                }
            }
            else
                Beep();
            break;

        case 'V':
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    CopyFromClipboard();
                    break;
                }
            }
            else
                Beep();
            break;

        case 'W':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED | RIGHT_ALT_PRESSED:
            case RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED:
            case LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED:
            case LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED:
                showWhiteSpace = !showWhiteSpace;
                invalid = true;
                break;
            }
            break;

        case 'X':
            switch (ker.dwControlKeyState & 0x1F)
            {
            case RIGHT_CTRL_PRESSED:
            case LEFT_CTRL_PRESSED:
                if (!readOnly)
                    DeleteToClipboard();
                else
                    Beep();
                break;
            }
            break;

        case 'Y':
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    Redo();
                    break;
                }
            }
            else
                Beep();
            break;

        case 'Z':
            if (!readOnly)
            {
                switch (ker.dwControlKeyState & 0x1F)
                {
                case RIGHT_CTRL_PRESSED:
                case LEFT_CTRL_PRESSED:
                    Undo();
                    break;
                }
            }
            else
                Beep();
            break;
        }

        if (((ker.dwControlKeyState & 0x1F) == 0 || (ker.dwControlKeyState & 0x1F) == SHIFT_PRESSED)
            && iswprint(ker.uChar.UnicodeChar)) // TODO Use locale?
        {
            if (!readOnly)
                Insert(ker.uChar.UnicodeChar);
            else
                Beep();
        }
    }
    return cont;
}

bool ModeEdit::Do(const MOUSE_EVENT_RECORD& mer, Screen& /*screen*/, const EditScheme& /*scheme*/)
{
    switch (mer.dwEventFlags)
    {
    case 0:
    case DOUBLE_CLICK:
    case MOUSE_MOVED:
        if ((mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0)
        {
            Anchor a = GetAnchor();
            for (int i = 0; a.startLine < chars.size() && i < mer.dwMousePosition.Y; ++i)
            {
                a.startLine = GetStartOfNextLine(chars, a.startLine);
            }
            a.column += mer.dwMousePosition.X;
            size_t p = a.ToOffset(chars, tabSize);
            size_t endOfLine = GetEndOfLine(chars, a.startLine);
            if (p > endOfLine)
                p = endOfLine;
            if (mer.dwEventFlags == DOUBLE_CLICK || (mer.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) != 0)
            {
                // TODO Should be able to select the space between words as well
                // TODO if not at start of word
                MoveCursor(MoveWordBegin(chars, p), false);
                p = MoveWordEnd(chars, p);
            }
            MoveCursor(p, mer.dwEventFlags == MOUSE_MOVED || (mer.dwControlKeyState & SHIFT_PRESSED) != 0 || mer.dwEventFlags == DOUBLE_CLICK || (mer.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) != 0);
        }
        break;

    case MOUSE_WHEELED:
        WORD wscroll = static_cast<WORD>(mer.dwButtonState >> (sizeof(WORD) * 8));
        short scroll = reinterpret_cast<short&>(wscroll) / 40;
        if (scroll < 0)
            MoveAnchorDown(-scroll, false);
        if (scroll > 0)
            MoveAnchorUp(scroll, false);
        break;
    }
    return true;
}

void ModeEdit::Assert() const
{
    ASSERT_MSG(selection.begin <= chars.size(), "me.selection.begin out of bounds");
    ASSERT_MSG(selection.end <= chars.size(), "me.selection.end out of bounds");
    ASSERT_MSG(anchor.startLine <= chars.size(), "me.anchor.startLine out of bounds");
}

void ModeEdit::MoveCursor(size_t p, bool keepPivot)
{
    invalid = invalid || selection.end != p;
    selection.end = p;
    if (!keepPivot)
    {
        invalid = invalid || selection.begin != selection.end;
        selection.begin = selection.end;
    }
    MakeCursorVisible();
}

bool ModeEdit::Find(const std::wstring& find, bool caseSensitive, bool forward, bool next)
{
    if (find.empty())
        return true; // Special case

    size_t p = UINT_MAX;
    if (forward)
    {
        size_t start = selection.Begin();
        if (next)
        {
            if (start >= chars.size())
                start = 0;
            else
                ++start;
        }
        if (caseSensitive)
        {
            p = boyer_moore<true>(chars, start, chars.size(), find);
            if (p == UINT_MAX)
            {
                if (start >= (chars.size() - find.length()))
                    start = 0;
                else
                    start += find.length();
                // TODO Notify search wrapping to user
                p = boyer_moore<true>(chars, 0, start, find);
            }
        }
        else
        {
            p = boyer_moore<false>(chars, start, chars.size(), find);
            if (p == UINT_MAX)
            {
                if (start >= (chars.size() - find.length()))
                    start = 0;
                else
                    start += find.length();
                // TODO Notify search wrapping to user
                p = boyer_moore<false>(chars, 0, start, find);
            }
        }
    }
    else
    {
        size_t end = selection.End();
        if (next)
        {
            if (end == 0)
                end = chars.size();
            else
                --end;
        }
        if (caseSensitive)
        {
            p = boyer_moore_r<true>(chars, 0, end, find);
            if (p == UINT_MAX)
            {
                if (end < find.length())
                    end = chars.size();
                else
                    end -= find.length();
                // TODO Notify search wrapping to user
                p = boyer_moore_r<true>(chars, end, chars.size(), find);
            }
        }
        else
        {
            p = boyer_moore_r<false>(chars, 0, end, find);
            if (p == UINT_MAX)
            {
                if (end < find.length())
                    end = chars.size();
                else
                    end -= find.length();
                // TODO Notify search wrapping to user
                p = boyer_moore_r<false>(chars, end, chars.size(), find);
            }
        }
    }


    if (p != UINT_MAX)
    {
        MoveCursor(p, false);
        MoveCursor(p + find.length(), true);
        return true;
    }
    else
    {
        MoveCursor(selection.Begin(), false);
        return false;
    }
}

void ModeEdit::MakeCursorVisible()
{
    anchor = MakeVisible(anchor, buffer.size, chars, tabSize, selection.end);
    invalid = true;
}

void ModeEdit::LoadTheme(HKEY hKey)
{
    brushTheme.clear();
    if (hKey != NULL)
    {
        HKEY hThemeKey = NULL;
        RegOpenKey(hKey, L"Theme", &hThemeKey);
        if (hThemeKey != NULL)
        {
            wchar_t Theme[256];
            LONG size = ARRAYSIZE(Theme) * sizeof(wchar_t);
            if (RegQueryValue(hThemeKey, nullptr, Theme, &size) == ERROR_SUCCESS)
            {
                HKEY hSubThemeKey = NULL;
                RegOpenKey(hThemeKey, Theme, &hSubThemeKey);
                if (hSubThemeKey != NULL && hSubThemeKey != hThemeKey)
                {
                    const wchar_t* names[] = {
                        Brush::PREPROCESSOR,
                        Brush::COMMENTS,
                        Brush::KEYWORD,
                        Brush::TYPE,
                        Brush::FUNCTION,
                        Brush::STRING,
                        Brush::VALUE,
                        Brush::VARIABLE,
                        Brush::CONSTANT,
                        Brush::COLOR1,
                        Brush::COLOR2,
                        Brush::COLOR3,
                    };
                    for (const wchar_t* name : names)
                    {
                        DWORD Value = 0;
                        DWORD Size = sizeof(Value);
                        if (RegQueryValueEx(hSubThemeKey, name, nullptr, nullptr, (LPBYTE) &Value, &Size) == ERROR_SUCCESS)
                            brushTheme[name] = Attribute(Value & 0xFFFF, (Value >> (sizeof(WORD) * 8)) & 0xFFFF);
                    }

                    RegCloseKey(hSubThemeKey);
                }
            }

            RegCloseKey(hThemeKey);
        }
    }
}

void ModeEdit::SetBrush(const Brush* b)
{
    brush = b;
    ApplyBrush(Span(0, chars.size()));
}

const wchar_t* ModeEdit::GetBrushName() const
{
    return brush->getName();
}

void ModeEdit::ApplyBrush(Span s)
{
    //attributes.clear();
    if (s.begin < 80)
        s.begin = 0;
    else
        s.begin -= 80;
    s.end += 80;
    if (s.end > chars.size())
        s.end = chars.size();

    auto itPair = attributes.equal_range(s);
    if (itPair.first != attributes.end() && itPair.first->first.begin < s.begin)
        s.begin = itPair.first->first.begin;
    if (itPair.second != attributes.begin() && std::prev(itPair.second)->first.end > s.end)
        s.end = std::prev(itPair.second)->first.end;
    attributes.erase(itPair.first, itPair.second);

    if (brush != nullptr)
    {
        SyntaxHighlighterMapT hl = SyntaxHighlighter(brush, chars, s.begin, s.end);
        for (const auto& d : hl)
        {
            auto itScheme = brushTheme.find(d.second);
            if (itScheme != brushTheme.end())
                MapSpanAdd(attributes, Span(d.first.begin, d.first.end), itScheme->second);
            //else
                // MapSpanAdd(attributes, s, Attribute(BACKGROUND_RED | BACKGROUND_INTENSITY, BACKGROUND_MASK));
        }
    }
}

void ModeEdit::MoveAnchorUp(size_t count, bool bMoveSelection)
{
    for (int i = 0; anchor.startLine > 0 && i < count; ++i)
        anchor.startLine = GetStartOfPrevLine(chars, anchor.startLine);
    if (bMoveSelection)
    {
        Anchor a(chars, tabSize, selection.end);
        for (int i = 0; a.startLine > 0 && i < count; ++i)
            a.startLine = GetStartOfPrevLine(chars, a.startLine);
        selection.begin = selection.end = a.ToOffset(chars, tabSize);
    }
    invalid = true;
}

void ModeEdit::MoveAnchorDown(size_t count, bool bMoveSelection)
{
    for (int i = 0; anchor.startLine < UINT_MAX && i < count; ++i)
        anchor.startLine = GetStartOfNextLine(chars, anchor.startLine);
    if (anchor.startLine == UINT_MAX)
        anchor.startLine = GetStartOfLine(chars, chars.size());
    if (bMoveSelection)
    {
        Anchor a(chars, tabSize, selection.end);
        for (int i = 0; a.startLine < UINT_MAX && i < count; ++i)
            a.startLine = GetStartOfNextLine(chars, a.startLine);
        if (a.startLine == UINT_MAX)
            a.startLine = GetStartOfLine(chars, chars.size());
        selection.begin = selection.end = a.ToOffset(chars, tabSize);
    }
    invalid = true;
}

std::wstring ModeEdit::GetSelectedText() const
{
    std::vector<wchar_t>::const_iterator itBegin = chars.begin() + selection.Begin();
    std::vector<wchar_t>::const_iterator itEnd = chars.begin() + selection.End();
    return std::wstring(itBegin, itEnd);
}

bool ModeEdit::Delete(Span ospan, bool pauseUndo)
{
    if (!ospan.empty())
    {
        Span span(ospan.Begin(), ospan.End());

        bool moveStartLine = anchor.startLine > 0 && span.isin(anchor.startLine - 1);

        std::vector<wchar_t>::iterator itBegin = chars.begin() + span.begin;
        std::vector<wchar_t>::iterator itEnd = chars.begin() + span.end;
        if (!pauseUndo)
            AddUndo(UndoEntry(UndoEntry::U_DELETE, ospan, selection, itBegin, itEnd));

        chars.erase(itBegin, itEnd);
#if 1
        if (selection.begin >= span.end)
            selection.begin -= span.length();
        else if (selection.begin >= span.begin)
            selection.begin = span.begin;
        if (selection.end >= span.end)
            selection.end -= span.length();
        else if (selection.end >= span.begin)
            selection.end = span.begin;
#else
        selection.begin = selection.end = span.begin;
#endif
        MakeCursorVisible();

        MapSpanCollapse(attributes, span);
        ApplyBrush(Span(span.begin, span.begin));

        if (moveStartLine)
            anchor.startLine = GetStartOfLine(chars, selection.begin);
        invalid = true;
        return true;
    }
    else
        return false;
}

void ModeEdit::Delete(Span span, size_t p)
{
    if (Delete(span))
        undo[undoCurrent - 1].type = UndoEntry::U_DELETE_PRE;
    Insert(p, std::vector<wchar_t>());
}

void ModeEdit::Insert(wchar_t c)
{
    Insert(std::vector<wchar_t>(1, c));
}

void ModeEdit::Insert(const wchar_t* s)
{
    Insert(std::vector<wchar_t>(s, s + wcslen(s)));
}

void ModeEdit::Insert(const std::vector<wchar_t>& s)
{
    if (Delete(selection))
        undo[undoCurrent - 1].type = UndoEntry::U_DELETE_PRE;
    ASSERT(selection.empty());
    Insert(selection.end, s);
}

void ModeEdit::Insert(size_t p, const std::vector<wchar_t>& s, bool pauseUndo)
{
    if (!pauseUndo)
        AddUndo(UndoEntry(UndoEntry::U_INSERT, Span(p, p), selection, s));

    Span span(p, p + s.size());
    chars.insert(chars.begin() + span.begin, s.begin(), s.end());

    selection.end = span.end;
    selection.begin = selection.end;
    MakeCursorVisible();

    MapSpanInflate(attributes, span);
    ApplyBrush(span);

    invalid = true;
}

void ModeEdit::Transform(CharTransfomT f)
{
    if (!selection.empty())
    {
        Span sel = selection;
        std::wstring t = GetSelectedText();
        for (wchar_t& c : t)
            c = f(c); // TODO Use locale?
        Insert(std::vector<wchar_t>(t.begin(), t.end()));
        selection = sel;
    }
}

void ModeEdit::Undo()
{
    if (undoCurrent > 0)
    {
        const UndoEntry& u = undo[--undoCurrent];

        switch (u.type)
        {
        case UndoEntry::U_INSERT:
            Delete(u.span, true);
            selection = u.selection;
            break;

        case UndoEntry::U_DELETE:
            Insert(u.span.Begin(), u.chars, true);
            selection = u.selection;
            break;
        }

        if (undoCurrent > 0 && undo[undoCurrent - 1].type == UndoEntry::U_DELETE_PRE)
        {
            const UndoEntry& up = undo[--undoCurrent];
            Insert(up.span.Begin(), up.chars, true);
            selection = up.selection;
        }
    }
}

void ModeEdit::Redo()
{
    if (undoCurrent < undo.size())
    {
        const UndoEntry& u = undo[undoCurrent];

        switch (u.type)
        {
        case UndoEntry::U_DELETE:
            Delete(u.span, true);
            break;

        case UndoEntry::U_DELETE_PRE:
            {
                Delete(u.span, true);
                const UndoEntry& up = undo[++undoCurrent];
                ASSERT(up.type == UndoEntry::U_INSERT);
                ASSERT(selection.empty());
                Insert(up.span.Begin(), up.chars, true);
            }
            break;

        case UndoEntry::U_INSERT:
            Insert(u.span.Begin(), u.chars, true);
            break;
        }

        ++undoCurrent;
    }
}

void ModeEdit::Save(FileInfo& fileInfo)
{
    if (isEdited())
    {
        fileInfo.save(chars);
        markSaved();
        invalid = true;
    }
}

void ModeEdit::Revert(FileInfo& fileInfo)
{
    ClearUndo();
    chars = fileInfo.load();
    invalid = true;
    eol = GetEOL(chars);
    MoveCursor(0, false);
    markSaved();
}

Span ModeEdit::CopyToClipboard() const
{
    Span tocopy = selection;
    if (tocopy.empty())
    {
        tocopy.begin = GetStartOfLine(chars, selection.end);
        tocopy.end = GetStartOfNextLine(chars, selection.end);
    }

    ::CopyToClipboard(&chars[tocopy.Begin()], tocopy.length());

    return tocopy;
}

void ModeEdit::CopyFromClipboard()
{
    OpenClipboard(0);
    HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
    if (hMem != NULL)
    {
        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
        Insert(pMem);
        GlobalUnlock(hMem);
    }
    CloseClipboard();
}

void ModeEdit::DeleteToClipboard()
{
    if (selection.empty())
    {
        Span span = CopyToClipboard();
        size_t p = MoveDown(chars, tabSize, selection.end) - span.length();
        Delete(span);
        MoveCursor(p, false);
    }
    else
    {
        Span span = CopyToClipboard();
        ASSERT(span == selection);
        Delete(span);
    }
}

void ModeEdit::AddUndo(const UndoEntry&& ue)
{
    undo.erase(undo.begin() + undoCurrent, undo.end());
    undo.push_back(ue);
    undoCurrent = undo.size();
}
