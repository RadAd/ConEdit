#ifndef MODE_H
#define MODE_H

struct Screen;

class Attribute
{
public:
    Attribute()
        : wAttribute(0)
        , wMask(0)
    {

    }

    Attribute(WORD wAttribute, WORD wMask)
        : wAttribute(wAttribute & wMask)
        , wMask(~wMask)
    {

    }

    void apply(WORD& w) const
    {
        w = (w & wMask) | wAttribute;
    }

private:
    WORD wAttribute;
    WORD wMask;
};

struct EditScheme
{
    EditScheme(WORD wAttributes);

    Attribute wAttrDefault;
    Attribute wAttrSelected;
    Attribute wAttrWhiteSpace;
    Attribute wAttrNonPrint;
    Attribute wAttrStatus;
    Attribute wAttrStatusFocus;
    Attribute wAttrError;
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
