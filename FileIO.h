#ifndef FILEIO_H
#define FILEIO_H

enum BOM { BOM_NONE, BOM_UTF8, BOM_UTF16LE, BOM_UTF16BE };

std::vector<wchar_t> loadtextfile(const wchar_t* filename, BOM& bom);
void savetextfile(const wchar_t* filename, BOM bom, const std::vector<wchar_t>& chars);

#endif
