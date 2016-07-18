#include "FileIO.h"

struct FileInfo
{
    FileInfo(const wchar_t* fn)
        : filename(fn)
        , exists(_wstati64(filename, &stat) == 0)
        , bom(BOM_NONE)
    {
        if (exists && (stat.st_mode & S_IFDIR) != 0)
            throw std::exception("Can't edit directory");
        if (exists && (stat.st_mode & S_IFREG) == 0)
            throw std::exception("Can't edit non-file");
    }

    bool changed(bool &modeChanged)
    {
        struct _stat64 newStat;
        bool newExists(_wstati64(filename, &newStat) == 0);

        modeChanged = stat.st_mode != newStat.st_mode;
        stat.st_mode = newStat.st_mode;

        return (newExists && (!exists || stat.st_mtime != newStat.st_mtime));
    }

    bool isreadonly() const
    {
        return exists && (stat.st_mode & _S_IWRITE) == 0;
    }

    std::vector<wchar_t> load(_locale_t l)
    {
        exists = _wstati64(filename, &stat) == 0;

        std::vector<wchar_t> chars = loadtextfile(filename, bom, l);
        chars.pop_back(); // Remove null terminator
        return chars;
    }

    void save(std::vector<wchar_t>& chars, _locale_t l)
    {
        struct TempPushBack
        {
            TempPushBack(std::vector<wchar_t>& v, wchar_t c)
                : v(v)
            {
                v.push_back(c);
            }
            ~TempPushBack()
            {
                v.pop_back();
            }
            std::vector<wchar_t>& v;
        };

        TempPushBack tpb(chars, L'\0');
        savetextfile(filename, bom, l, chars);

        exists = _wstati64(filename, &stat) == 0;
    }

    const wchar_t* filename;
    struct _stat64 stat;
    bool exists;
    BOM bom;
};
