#include "stdafx.h"
#include "TextUtils.h"

bool IsEOL(wchar_t c)
{
    //return c == L'\r' || c == L'\n';
    return iswspace(c) && !iswblank(c);
}

size_t IsEOL(const std::vector<wchar_t>& chars, size_t p)
{
    if ((p + 1) < chars.size() && chars[p] == L'\r' && chars[p + 1] == L'\n')
        return 2;
    if ((p + 1) < chars.size() && chars[p] == L'\n' && chars[p + 1] == L'\r')
        return 2;
    if (p < chars.size() && chars[p] == L'\r')
        return 1;
    if (p < chars.size() && chars[p] == L'\n')
        return 1;
    return 0;
}

size_t IsPrevEOL(const std::vector<wchar_t>& chars, size_t p)
{
    if (p >= 2 && chars[p - 2] == L'\r' && chars[p - 1] == L'\n')
        return 2;
    if (p >= 2 && chars[p - 2] == L'\n' && chars[p - 1] == L'\r')
        return 2;
    if (p >= 1 && chars[p - 1] == L'\r')
        return 1;
    if (p >= 1 && chars[p - 1] == L'\n')
        return 1;
    return 0;
}

size_t GetStartOfLine(const std::vector<wchar_t>& chars, size_t p)
{
    while (p > 0 && !IsEOL(chars[p - 1]))
        --p;
    return p;
}

size_t GetEndOfLine(const std::vector<wchar_t>& chars, size_t p)
{
    while (p < chars.size() && !IsEOL(chars[p]))
        ++p;
    return p;
}

size_t GetStartOfNextLine(const std::vector<wchar_t>& chars, size_t p)
{
    while (p < chars.size() && !IsEOL(chars[p]))
        ++p;
    if (p >= chars.size())
        return UINT_MAX;
    p += IsEOL(chars, p);
    return p;
}

size_t GetStartOfPrevLine(const std::vector<wchar_t>& chars, size_t p)
{
    p = GetStartOfLine(chars, p);
    if (p == 0)
        return UINT_MAX;
    p -= IsPrevEOL(chars, p);
    p = GetStartOfLine(chars, p);
    return p;
}

size_t GetLine(const std::vector<wchar_t>& chars, size_t p)
{
    p = GetStartOfLine(chars, p);
    size_t l = 1;
    size_t s = 0;
    while (p > s)
    {
        s = GetStartOfNextLine(chars, s);
        ++l;
    }
    return l;
}

size_t GetLineOffset(const std::vector<wchar_t>& chars, size_t line)
{
    size_t s = UINT_MAX;
    if (line > 0)
    {
        s = 0;
        --line;
        while (s < UINT_MAX && line > 0)
        {
            s = GetStartOfNextLine(chars, s);
            --line;
        }
    }
    if (s == UINT_MAX)
        s = chars.size();
    return s;
}

size_t GetColumn(const std::vector<wchar_t>& chars, size_t startLine, size_t tabSize, size_t p)
{
#if 0
    return p - startLine;  // TODO Needs to be fixed for tabs
#else
    size_t column = 0;
    for (size_t i = startLine; i < chars.size() && i < p; ++i)
    {
        if (chars[i] == L'\t')
            column += tabSize - ((i - startLine) % tabSize);
        else
            ++column;
    }
    return column;
#endif
}

size_t GetOffset(const std::vector<wchar_t>& chars, size_t startLine, size_t tabSize, size_t column)
{
#if 0
    size_t p = startLine + column;  // TODO Needs to be fixed for tabs
    size_t end = GetEndOfLine(chars, startLine);
    if (p > end)
        p = end;
    return p;
#else
    size_t p = startLine;
    for (size_t i = 0; p < chars.size() && i < column; ++i)
    {
        if (chars[p] == L'\t')
        {
            size_t d = tabSize - (i % tabSize) - 1;
            if (i + d > column)
                break;
            i += d;
        }
        ++p;
    }
    return p;
#endif
}

size_t MoveUp(const std::vector<wchar_t>& chars, size_t tabSize, size_t p)
{
    size_t charsStartOfLine = GetStartOfLine(chars, p);
    size_t charsLineColumn = GetColumn(chars, charsStartOfLine, tabSize, p);
    size_t charsStartOfPrevLine = GetStartOfPrevLine(chars, charsStartOfLine);
    if (charsStartOfPrevLine != UINT_MAX)
    {
        size_t ret = GetOffset(chars, charsStartOfPrevLine, tabSize, charsLineColumn);
        size_t charsEndOfPrevLine = GetEndOfLine(chars, charsStartOfPrevLine);
        if (ret > charsEndOfPrevLine)
            ret = charsEndOfPrevLine;
        return ret;
    }
    else
        return p;
}

size_t MoveDown(const std::vector<wchar_t>& chars, size_t tabSize, size_t p)
{
    size_t charsStartOfLine = GetStartOfLine(chars, p);
    size_t charsLineColumn = GetColumn(chars, charsStartOfLine, tabSize, p);
    size_t charsStartOfNextLine = GetStartOfNextLine(chars, p);
    if (charsStartOfNextLine != UINT_MAX)
    {
        size_t ret = GetOffset(chars, charsStartOfNextLine, tabSize, charsLineColumn);
        size_t charsEndOfNextLine = GetEndOfLine(chars, charsStartOfNextLine);
        if (ret > charsEndOfNextLine)
            ret = charsEndOfNextLine;
        return ret;
    }
    else
        return p;
}

size_t MoveLeft(const std::vector<wchar_t>& chars, size_t p)
{
    size_t d = IsPrevEOL(chars, p);
    if (d == 0)
    {
        if (p > 0)
            --p;
    }
    else
        p -= d;
    return p;
}

size_t MoveRight(const std::vector<wchar_t>& chars, size_t p)
{
    size_t d = IsEOL(chars, p);
    if (d == 0)
    {
        if (p < chars.size())
            ++p;
    }
    else
        p += d;
    return p;
}

size_t MoveWordLeft(const std::vector<wchar_t>& chars, size_t p)
{
    size_t d = IsPrevEOL(chars, p);
    if (d > 0)
        p -= d;

    while (p > 0 && isblank(chars[p - 1])) // TODO Use locale?
        --p;

    if (p > 0 && iswpunct(chars[p - 1])) // TODO Use locale?
        --p;
    else
        while (p > 0 && (iswalnum(chars[p - 1]) || chars[p] == L'_')) // TODO Use locale?
            --p;

    return p;
}

size_t MoveWordRight(const std::vector<wchar_t>& chars, size_t p)
{
    size_t d = IsEOL(chars, p);
    if (d > 0)
        p += d;
    else
    {
        if (iswpunct(chars[p])) // TODO Use locale?
            ++p;
        else
            while (p < chars.size() && (iswalnum(chars[p]) || chars[p] == L'_')) // TODO Use locale?
                ++p;
    }

    while (p < chars.size() && isblank(chars[p])) // TODO Use locale?
        ++p;

    return p;
}

size_t MoveWordBegin(const std::vector<wchar_t>& chars, size_t p)
{
    if (iswpunct(chars[p])) // TODO Use locale?
        --p;
    else if (iswblank(chars[p]) && (p == 0 || iswblank(chars[p - 1]))) // TODO Use locale?
        while (p > 0 && iswblank(chars[p - 1])) // TODO Use locale?
            --p;
    else
        while (p > 0 && (iswalnum(chars[p - 1]) || chars[p - 1] == L'_')) // TODO Use locale?
            --p;

    return p;
}

size_t MoveWordEnd(const std::vector<wchar_t>& chars, size_t p)
{
    if (iswpunct(chars[p])) // TODO Use locale?
        ++p;
    else if (iswblank(chars[p])) // TODO Use locale?
        while (p < chars.size() && iswblank(chars[p])) // TODO Use locale?
            ++p;
    else
        while (p < chars.size() && (iswalnum(chars[p]) || chars[p] == L'_')) // TODO Use locale?
            ++p;

    return p;
}

size_t FindMatchingBraceForward(const std::vector<wchar_t>& chars, size_t p, wchar_t open, wchar_t close)
{
    while (p < chars.size() && p != UINT_MAX)
    {
        ++p;
        if (chars[p] == close)
            return p;
        if (chars[p] == open)
            p = FindMatchingBraceForward(chars, p, open, close);
    }
    return UINT_MAX;
}

size_t FindMatchingBraceBackward(const std::vector<wchar_t>& chars, size_t p, wchar_t open, wchar_t close)
{
    while (p > 0 && p != UINT_MAX)
    {
        --p;
        if (chars[p] == open)
            return p;
        if (chars[p] == close)
            p = FindMatchingBraceBackward(chars, p, open, close);
    }
    return UINT_MAX;
}

size_t FindMatchingBrace(const std::vector<wchar_t>& chars, size_t p)
{
    switch (chars[p])
    {
    case L'(': return FindMatchingBraceForward(chars, p, L'(', L')');
    case L'[': return FindMatchingBraceForward(chars, p, L'[', L']');
    case L'{': return FindMatchingBraceForward(chars, p, L'{', L'}');

    case L')': return FindMatchingBraceBackward(chars, p, L'(', L')');
    case L']': return FindMatchingBraceBackward(chars, p, L'[', L']');
    case L'}': return FindMatchingBraceBackward(chars, p, L'{', L'}');

    default:
        return UINT_MAX;
    }
}
