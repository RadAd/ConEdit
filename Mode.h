#ifndef MODE_H
#define MODE_H

struct Screen;

struct EditScheme
{
    EditScheme(WORD wAttributes);

    WORD wAttrDefault;
    WORD wAttrSelected;
    WORD wAttrWhiteSpace;
    WORD wAttrNonPrint;
    WORD wAttrStatus;
    WORD wAttrStatusFocus;
    WORD wAttrError;
};

class Mode
{
public:
    virtual void Draw(Screen& screen, const EditScheme& scheme, bool focus) const = 0;
    virtual bool Do(const KEY_EVENT_RECORD& /*ker*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }
    virtual bool Do(const MOUSE_EVENT_RECORD& /*mer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { return true; }
    virtual void Do(const FOCUS_EVENT_RECORD& /*fer*/, Screen& /*screen*/, const EditScheme& /*scheme*/) { }
    virtual void Do(const WINDOW_BUFFER_SIZE_RECORD& /*wbsr*/) { }
    virtual void Assert() {}
};

void DoMode(Mode& m, Screen& screen, const EditScheme& scheme);

class ModeBase : public Mode
{
public:
    ModeBase(Mode* base)
        : base(base)
    {

    }
    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const { base->Draw(screen, scheme, focus); };
    bool Do(const KEY_EVENT_RECORD& ker, Screen& screen, const EditScheme& scheme) { return base->Do(ker, screen, scheme); }
    bool Do(const MOUSE_EVENT_RECORD& mer, Screen& screen, const EditScheme& scheme) { return base->Do(mer, screen, scheme); }
    void Do(const FOCUS_EVENT_RECORD& fer, Screen& screen, const EditScheme& scheme) { base->Do(fer, screen, scheme); }
    void Do(const WINDOW_BUFFER_SIZE_RECORD& wbsr) { base->Do(wbsr); }
    void Assert() { base->Assert(); }

private:
    Mode* base;
};

#endif
