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
        if (data.size() >= len)
        {
            bool is = memcmp(data.data(), c, len) == 0;
            if (is)
                data.erase(data.begin(), data.begin() + len);
            return is;
        }
        else
            return false;
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
        byteswap16(reinterpret_cast<unsigned short*>(chars.data()), chars.size() - 1);
#endif
    }

    void byteswap(std::vector<char>& data)
    {
        byteswap16(reinterpret_cast<unsigned short*>(data.data()), (data.size() - 1) * (sizeof(wchar_t) / sizeof(char)));
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
        if (!data.empty())
            fread(data.data(), 1, size, f);

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
        if (!data.empty())
            fwrite(data.data(), 1, data.size(), f);

        fclose(f);
    }

    std::vector<wchar_t> convert(const std::vector<char>& data, BOM bom)
    {
        std::vector<wchar_t> chars;

        switch (bom)
        {
        case BOM_UTF16BE:
        case BOM_UTF16LE:
            chars.resize((data.size() - 1) / (sizeof(wchar_t) / sizeof(char)) + 1);
            memcpy(chars.data(), data.data(), data.size() - 1);
            chars.back() = data.back();
            if (bom == BOM_UTF16BE)
                byteswap(chars);
            break;

        default:
            int s = MultiByteToWideChar(CP_UTF8, 0, data.data(), (int) data.size(), nullptr, 0);
            if (s == 0)
                throw std::exception("Error converting to wcs");
            chars.resize(s);
            MultiByteToWideChar(CP_UTF8, 0, data.data(), (int) data.size(), chars.data(), s);
            break;
        }

        return chars;
    }

    std::vector<char> convert(const std::vector<wchar_t>& chars, BOM bom)
    {
        std::vector<char> data;

        switch (bom)
        {
        case BOM_UTF16BE:
        case BOM_UTF16LE:
            data.resize((chars.size() - 1) * (sizeof(wchar_t) / sizeof(char)) + 1);
            memcpy(data.data(), chars.data(), data.size() - 1);
            data.back() = static_cast<char>(chars.back());
            if (bom == BOM_UTF16BE)
                byteswap(data);
            break;

        default:
            int s = WideCharToMultiByte(CP_UTF8, 0, chars.data(), (int) chars.size(), nullptr, 0, nullptr, nullptr);
            if (s == 0)
                throw std::exception("Error converting to wcs");
            data.resize(s);
            WideCharToMultiByte(CP_UTF8, 0, chars.data(), (int) chars.size(), data.data(), s, nullptr, nullptr);
        }

        return data;
    }
}

std::vector<wchar_t> loadtextfile(const wchar_t* filename, BOM& bom)
{
    std::vector<char> data = loadfile(filename, bom);
    data.push_back('\0');
    std::vector<wchar_t> chars = convert(data, bom);
#if 1
    std::vector<char> cmp = convert(chars, bom);
    if (data != cmp)
        throw std::exception("Unicode convert check failed");
#endif
    return chars;
}

void savetextfile(const wchar_t* filename, BOM bom, const std::vector<wchar_t>& chars)
{
    std::vector<char> data = convert(chars, bom);
    data.pop_back();
    savefile(filename, bom, data);
}
