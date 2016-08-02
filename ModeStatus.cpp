#include "stdafx.h"

#include "Mode.h"
#include "ConUtils.h"
#include "FileInfo.h"

#include "ModeStatus.h"

#include "ModeEdit.h"

const wchar_t* bomName[] = {
    { L"ANSI" }, // BOM_NONE
    { L"UTF8" }, // BOM_UTF8
    { L"UTF16LE" }, // BOM_UTF16LE
    { L"UTF16BE" }, // BOM_UTF16BE
};

const wchar_t* GetEOLName(const std::vector<wchar_t>& eol)
{
    if (eol.size() == 2 && eol[0] == L'\r' && eol[1] == L'\n')
        return L"CRLF";
    else if (eol.size() == 2 && eol[0] == L'\n' && eol[1] == L'\r')
        return L"LFCR";
    else if (eol.size() == 1 && eol[0] == L'\r')
        return L"CR";
    else if (eol.size() == 1 && eol[0] == L'\n')
        return L"LF";
    else
        return L"UNK";
}

void FillMessageWindow(Buffer& status, WORD wAttr, const std::wstring& msg)
{
    status.clear(wAttr, L' ');
    WriteBegin(status, _COORD(1, 0), msg.c_str());
}

void FillStatusWindow(Buffer& status, const ModeEdit& ev, const EditScheme& scheme, const FileInfo& fileInfo, const std::vector<wchar_t>& chars)
{
    WORD wAttribute = 0;
    scheme.get(EditScheme::STATUS).apply(wAttribute);
    status.clear(wAttribute, L' ');

    status.data[0].Char.UnicodeChar = L' ';
    if (fileInfo.exists && (fileInfo.stat.st_mode & _S_IWRITE) == 0)
        status.data[0].Char.UnicodeChar = L'§';
    if (ev.isEdited())
        status.data[0].Char.UnicodeChar = L'*';

    const wchar_t* n = wcsrchr(fileInfo.filename, L'\\');
    n = n != nullptr ? n + 1 : fileInfo.filename;
    WriteBegin(status, _COORD(1, 0), n);
    if (true)
    {
        const Anchor a(chars, ev.TabSize(), ev.Selection().end);
        const size_t line = GetLine(chars, a.startLine);
        const size_t len = ev.Selection().length();
        const wchar_t* eolName = GetEOLName(ev.Eol());
        const wchar_t* brushName = ev.GetBrushName();
        if (brushName == nullptr) brushName = L"";
        wchar_t msg[32];
        if (len == 0)
            swprintf_s(msg, L"%s %s %s %Iu,%Iu", brushName, bomName[fileInfo.bom], eolName, line, a.column + 1);
        else
            swprintf_s(msg, L"%s %s %s [%Iu] %Iu,%Iu", brushName, bomName[fileInfo.bom], eolName, len, line, a.column + 1);

        WriteEnd(status, _COORD(status.size.X - 2, 0), msg);
    }
}

void ModeStatus::Draw(Screen& screen, const EditScheme& scheme, bool /*focus*/) const
{
    if (!error.empty())
    {
        WORD wAttribute = 0;
        scheme.get(EditScheme::STATUSERROR).apply(wAttribute);
        FillMessageWindow(buffer, wAttribute, error);
        error.clear();
    }
    else if (!message.empty())
    {
        WORD wAttribute = 0;
        scheme.get(EditScheme::STATUSWARNING).apply(wAttribute);
        FillMessageWindow(buffer, wAttribute, message);
        message.clear();
    }
    else
        FillStatusWindow(buffer, ev, scheme, fileInfo, chars);
    DrawBuffer(screen.hOut, buffer, pos);
}
