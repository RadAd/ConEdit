#include "ConUtils.h"
#include "Mode.h"

struct EditScheme;

typedef void(*findcb)(void* data, bool forward, bool next);

class ModeFind : public ModeBase
{
public:
    ModeFind(Mode* base, COORD size, COORD pos, findcb cb, void* data)
        : ModeBase(base)
        , buffer(size)
        , pos(pos)
        , editPos(0)
        , visible(false)
        , FindCB(cb)
        , data(data)
        , caseSensitive(false)
    {
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const;
    bool Do(const KEY_EVENT_RECORD& ker, Screen& screen, const EditScheme& scheme);
    bool Do(const MOUSE_EVENT_RECORD& /*mer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }

    const std::wstring& Find() const { return find; }
    bool CaseSensitive() const { return caseSensitive; }
    void ToggleCaseSensitive() { caseSensitive = !caseSensitive; }
    void setFind(const std::wstring& f)
    {
        find = f;
        editPos = find.length();
    }

    bool visible;

    mutable Buffer buffer;
    COORD pos;

private:
    std::wstring find;
    size_t editPos;
    findcb FindCB;
    void* data;

    bool caseSensitive;
};
