#include "stdafx.h"
#include "FileIO.h"

namespace
{
    char bomBytes[][4] = {
        { 0 },  // BOM_NONE
        { '\xEF', '\xBB', '\xBF', 0 }, // BOM_UTF8
        { '\xFF', '\xFE', 0 }, // BOM_UTF16LE
        { '\xFE', '\xFF', 0 }, // BOM_UTF16BE
    };

    bool isbom(std::vector<char>& data, const char* c)
    {
        size_t len = strlen(c);
        bool is = memcmp(&data.front(), c, len) == 0;
        if (is)
            data.erase(data.begin(), data.begin() + len);
        return is;
    }

    void byteswap16(unsigned short* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            data[i] = _byteswap_ushort(data[i]);
    }

    void byteswap(std::vector<wchar_t>& chars)
    {
#if 0
        for (wchar_t& c : chars)
            c = _byteswap_ushort(c);
#else
        byteswap16(reinterpret_cast<unsigned short*>(&chars.front()), chars.size() - 1);
#endif
    }

    void byteswap(std::vector<char>& data)
    {
        byteswap16(reinterpret_cast<unsigned short*>(&data.front()), (data.size() - 1) * (sizeof(wchar_t) / sizeof(char)));
    }

    std::vector<char> loadfile(const wchar_t* filename, BOM& bom)
    {
        std::vector<char> data;

        FILE* f = nullptr;
        errno_t e = _wfopen_s(&f, filename, L"rb");
        if (e != 0)
            throw std::exception("Error opening file");

        _fseeki64(f, 0, SEEK_END);
        __int64 size = _ftelli64(f);
        _fseeki64(f, 0, SEEK_SET);

        data.resize(size);
        fread(&data.front(), 1, size, f);

        fclose(f);

        bom = BOM_NONE;
        if (isbom(data, bomBytes[BOM_UTF8]))
            bom = BOM_UTF8;
        else if (isbom(data, bomBytes[BOM_UTF16LE]))
            bom = BOM_UTF16LE;
        else if (isbom(data, bomBytes[BOM_UTF16BE]))
            bom = BOM_UTF16BE;

        return data;
    }

    void savefile(const wchar_t* filename, BOM bom, const std::vector<char>& data)
    {
        FILE* f = nullptr;
        errno_t e = _wfopen_s(&f, filename, L"wb");
        if (e != 0)
            throw std::exception("Error saving file");

        if (bom != BOM_NONE)
        {
            const char* c = bomBytes[bom];
            size_t len = strlen(c);
            fwrite(c, 1, len, f);
        }
        fwrite(&data.front(), 1, data.size(), f);

        fclose(f);
    }

    std::vector<wchar_t> convert(const std::vector<char>& data, BOM bom, _locale_t l)
    {
        std::vector<wchar_t> chars;

        switch (bom)
        {
        case BOM_UTF16BE:
        case BOM_UTF16LE:
            chars.resize((data.size() - 1) / (sizeof(wchar_t) / sizeof(char)) + 1);
            memcpy(&chars.front(), &data.front(), data.size() - 1);
            chars.back() = data.back();
            if (bom == BOM_UTF16BE)
                byteswap(chars);
            break;

        default:
            size_t s = 0;
            errno_t e = _mbstowcs_s_l(&s, nullptr, 0, &data.front(), 0, l);
            if (e != 0)
                throw std::exception("Error converting to wcs");
            chars.resize(s);
            e = _mbstowcs_s_l(&s, &chars.front(), s, &data.front(), s - 1, l);
            if (e != 0)
                throw std::exception("Error converting to wcs");
            break;
        }

        return chars;
    }

    std::vector<char> convert(const std::vector<wchar_t>& chars, BOM bom, _locale_t l)
    {
        std::vector<char> data;

        switch (bom)
        {
        case BOM_UTF16BE:
        case BOM_UTF16LE:
            data.resize((chars.size() - 1) * (sizeof(wchar_t) / sizeof(char)) + 1);
            memcpy(&data.front(), &chars.front(), data.size() - 1);
            data.back() = static_cast<char>(chars.back());
            if (bom == BOM_UTF16BE)
                byteswap(data);
            break;

        default:
            size_t s = 0;
            errno_t e = _wcstombs_s_l(&s, nullptr, 0, &chars.front(), 0, l);
            if (e != 0)
                throw std::exception("Error converting to mbs");
            data.resize(s);
            e = _wcstombs_s_l(&s, &data.front(), s, &chars.front(), s - 1, l);
            if (e != 0)
                throw std::exception("Error converting to mbs");
            break;
        }

        return data;
    }
}

std::vector<wchar_t> loadtextfile(const wchar_t* filename, BOM& bom, _locale_t l)
{
    std::vector<char> data = loadfile(filename, bom);
    data.push_back('\0');
    std::vector<wchar_t> chars = convert(data, bom, l);
#if 1
    std::vector<char> cmp = convert(chars, bom, l);
    if (data != cmp)
        throw std::exception("Unicode convert check failed");
#endif
    return chars;
}

void savetextfile(const wchar_t* filename, BOM bom, _locale_t l, const std::vector<wchar_t>& chars)
{
    std::vector<char> data = convert(chars, bom, l);
    data.pop_back();
    savefile(filename, bom, data);
}
