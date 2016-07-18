
class ModeEdit;
struct FileInfo;

class ModeStatus //: public Mode
{
public:
    ModeStatus(COORD size, COORD pos, const ModeEdit& ev, const FileInfo& fileInfo, const std::vector<wchar_t>& chars)
        : buffer(size)
        , pos(pos)
        , ev(ev)
        , fileInfo(fileInfo)
        , chars(chars)
    {
    }

    void Draw(Screen& screen, const EditScheme& scheme, bool focus) const;

    mutable std::wstring error;
    mutable std::wstring message;

    mutable Buffer buffer;
    COORD pos;
private:
    const ModeEdit& ev;
    const FileInfo& fileInfo;
    const std::vector<wchar_t>& chars;
};
