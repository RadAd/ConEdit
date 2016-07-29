#include "TextUtils.h"
#include "ConUtils.h"

#include <map>

struct EditScheme;
struct FileInfo;
class Attribute;

struct Anchor
{
    Anchor()
        : startLine(0)
        , column(0)
    {
    }

    Anchor(const std::vector<wchar_t>& chars, size_t tabSize, size_t p)
        : startLine(GetStartOfLine(chars, p))
        , column(GetColumn(chars, startLine, tabSize, p))
    {
    }

    size_t ToOffset(const std::vector<wchar_t>& chars, size_t tabSize) const;

    size_t startLine;
    size_t column;
};

inline bool operator==(const Anchor& l, const Anchor& r)
{
    return l.startLine == r.startLine && l.column == r.column;
}

inline bool operator!=(const Anchor& l, const Anchor& r)
{
    return !(l == r);
}

inline size_t Anchor::ToOffset(const std::vector<wchar_t>& chars, size_t tabSize) const
{
    return GetOffset(chars, startLine, tabSize, column);
}

struct Span
{
    Span(size_t b, size_t e)
        : begin(b)
        , end(e)
    {
    }

    bool isin(size_t p) const
    {
        return ((p >= begin) && (p < end) || (p >= end) && (p < begin));
    }

    bool empty() const { return begin == end; }
    size_t Begin() const { return (end > begin) ? begin : end; }
    size_t End() const { return (end > begin) ? end : begin; }
    size_t length() const { return (end > begin) ? end - begin : begin - end; }

    size_t begin;
    size_t end;
};

inline bool operator==(const Span& l, const Span& r)
{
    return l.begin == r.begin && l.end == r.end;
}

inline bool operator!=(const Span& l, const Span& r)
{
    return !(l == r);
}

inline bool operator<(const Span& l, const Span& r)
{
    if (l.begin == r.begin)
        return l.end < r.end;
    else
        return l.begin < r.begin;
}

struct UndoEntry
{
    enum Type { U_INSERT, U_DELETE, U_DELETE_PRE };

    UndoEntry(Type t, Span s, Span selection, std::vector<wchar_t>::const_iterator itBegin, std::vector<wchar_t>::const_iterator itEnd)
        : type(t)
        , span(s)
        , selection(selection)
        , chars(itBegin, itEnd)
    {
        ASSERT(span.length() == chars.size());
    }

    UndoEntry(Type t, Span s, Span selection, wchar_t c)
        : type(t)
        , span(s)
        , selection(selection)
        , chars(1, c)
    {
        span.end += chars.size();
        ASSERT(span.length() == chars.size());
    }

    UndoEntry(Type t, Span s, Span selection, const wchar_t* str)
        : type(t)
        , span(s)
        , selection(selection)
        , chars(str, str + wcslen(str))
    {
        span.end += chars.size();
        ASSERT(span.length() == chars.size());
    }

    UndoEntry(Type t, Span s, Span selection, const std::vector<wchar_t>& str)
        : type(t)
        , span(s)
        , selection(selection)
        , chars(str)
    {
        span.end += chars.size();
        ASSERT(span.length() == chars.size());
    }

    Type type;
    Span span;
    Span selection;
    std::vector<wchar_t> chars;
};

class ModeEdit // : public Mode
{
public:
    ModeEdit(const COORD& bufferSize, bool edited, bool readOnly, std::vector<wchar_t>& chars);

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const;
    bool Do(const KEY_EVENT_RECORD& ker, Screen& screen, const EditScheme& scheme);
    bool Do(const MOUSE_EVENT_RECORD& mer, Screen& screen, const EditScheme& scheme);

    void Assert() const;

    void MoveCursor(size_t p, bool keepPivot);
    bool Find(const std::wstring& find, bool caseSensitive, bool forward, bool next);

    void InsertAttribute(Span s, Attribute a);
    void ClearAttributes() { attributes.clear(); }

    void MoveAnchorUp(size_t count, bool bMoveSelection);
    void MoveAnchorDown(size_t count, bool bMoveSelection);

    std::wstring GetSelectedText() const;

    bool Delete(Span span, bool pauseUndo = false);
    void Delete(Span span, size_t p);
    void Insert(wchar_t c);
    void Insert(const wchar_t* s);
    void Insert(const std::vector<wchar_t>& s);
    void Insert(size_t p, const std::vector<wchar_t>& s, bool pauseUndo = false);

    typedef wint_t(*CharTransfomT)(wint_t);
    void Transform(CharTransfomT f);

    void Save(FileInfo& fileInfo, _locale_t l);
    void Revert(FileInfo& fileInfo, _locale_t l);

    Span CopyToClipboard() const;
    void CopyFromClipboard();
    void DeleteToClipboard();

    void Undo();
    void Redo();
    void ClearUndo() { undo.clear();  undoCurrent = undo.size(); }

    const std::vector<wchar_t>& Chars() const { return chars; }
    const Anchor& GetAnchor() const { return anchor; }
    const Span& Selection() const { return selection; }
    size_t TabSize() const { return tabSize; }
    const std::vector<wchar_t>& Eol() const { return eol; }

    mutable Buffer buffer;
    COORD pos;

    bool readOnly;

    mutable bool invalid;

    bool isEdited() const { return undoSaved != undoCurrent; }
    void markSaved() { undoSaved = undoCurrent; }

private:
    void FillCharsWindow(const EditScheme& scheme, COORD& cursor) const;
    void MakeCursorVisible();

    std::vector<wchar_t>& chars;
    std::vector<wchar_t> eol;
    Anchor anchor;
    Span selection;
    std::map<Span, Attribute> attributes;

    bool showWhiteSpace;
    size_t tabSize;

    size_t undoCurrent;
    std::vector<UndoEntry> undo;
    void AddUndo(const UndoEntry&& ue);

    size_t undoSaved;
};
