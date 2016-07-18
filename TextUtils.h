
bool IsEOL(wchar_t c);

size_t IsEOL(const std::vector<wchar_t>& chars, size_t p);
size_t IsPrevEOL(const std::vector<wchar_t>& chars, size_t p);
size_t GetStartOfLine(const std::vector<wchar_t>& chars, size_t p);
size_t GetEndOfLine(const std::vector<wchar_t>& chars, size_t p);
size_t GetStartOfNextLine(const std::vector<wchar_t>& chars, size_t p);
size_t GetStartOfPrevLine(const std::vector<wchar_t>& chars, size_t p);

size_t GetLine(const std::vector<wchar_t>& chars, size_t p);
size_t GetLineOffset(const std::vector<wchar_t>& chars, size_t line);
size_t GetColumn(const std::vector<wchar_t>& chars, size_t startLine, size_t tabSize, size_t p);
size_t GetOffset(const std::vector<wchar_t>& chars, size_t startLine, size_t tabSize, size_t column);

size_t MoveUp(const std::vector<wchar_t>& chars, size_t tabSize, size_t p);
size_t MoveDown(const std::vector<wchar_t>& chars, size_t tabSize, size_t p);
size_t MoveLeft(const std::vector<wchar_t>& chars, size_t p);
size_t MoveRight(const std::vector<wchar_t>& chars, size_t p);
size_t MoveWordLeft(const std::vector<wchar_t>& chars, size_t p);
size_t MoveWordRight(const std::vector<wchar_t>& chars, size_t p);
size_t MoveWordBegin(const std::vector<wchar_t>& chars, size_t p);
size_t MoveWordEnd(const std::vector<wchar_t>& chars, size_t p);

size_t FindMatchingBrace(const std::vector<wchar_t>& chars, size_t p);
