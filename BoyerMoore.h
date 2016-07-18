#include <string>
#include <map>
#include <vector>
#include <assert.h>

#ifndef max
#define max(a, b) ((a < b) ? b : a)
#endif

template<bool caseSensitive>
wchar_t make_case(wchar_t c);

template<>
wchar_t make_case<true>(wchar_t c) { return c; }

template<>
wchar_t make_case<false>(wchar_t c) { return towupper(c); }

// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurrence of c in pat.
// If c does not occur in pat, then delta1[c] = pat.size().
// If c is at string[i] and c != pat[pat.size()-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up
// with some character in pat.
// this algorithm runs in pat.size() time.
template<bool caseSensitive, class T>
std::map<wchar_t, size_t> make_delta1(const T& patBegin, const T& patEnd) {
    assert(patBegin != patEnd);
    std::map<wchar_t, size_t> delta1;
    size_t patSize = patEnd - patBegin;
    for (size_t i = 0; i < patSize - 1; i++) {
        delta1[make_case<caseSensitive>(patBegin[i])] = patSize - 1 - i;
    }
    return delta1;
}

// true if the suffix of word starting from word[pos] is a prefix
// of word
template<bool caseSensitive, class T>
bool is_prefix(const T& wordBegin, const T& wordEnd, size_t pos) {
    size_t suffixlen = (wordEnd - wordBegin) - pos;
    for (size_t i = 0; i < suffixlen; i++) {
        if (make_case<caseSensitive>(wordBegin[i]) != make_case<caseSensitive>(wordBegin[pos + i])) {
            return false;
        }
    }
    return true;
}

// length of the longest suffix of word ending on word[pos].
// suffix_length(L"dddbcabc", 4) = 2
template<bool caseSensitive, class T>
size_t suffix_length(const T& wordBegin, const T& wordEnd, size_t pos) {
    size_t i;
    size_t wordSize = (wordEnd - wordBegin);
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (make_case<caseSensitive>(wordBegin[pos - i]) == make_case<caseSensitive>(wordBegin[wordSize - 1 - i])) && (i < pos); i++);
    return i;
}

// delta2 table: given a mismatch at pat[pos], we want to align
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[pat.size()-1].
//
// In case 1:
// pat[pos+1] to pat[pat.size()-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. pat.size()-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with
// pat[pat.size()-1].
//
// In case 2:
// pat[pos+1] to pat[pat.size()-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
//
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDBYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
template<bool caseSensitive, class T>
std::vector<size_t> make_delta2(const T& patBegin, const T& patEnd) {
    assert(patBegin != patEnd);
    size_t patSize = patEnd - patBegin;
    std::vector<size_t> delta2(patSize);
    size_t last_prefix_index = patSize - 1;

    // first loop
    for (size_t p = patSize; p > 0; --p) {
        if (is_prefix<caseSensitive>(patBegin, patEnd, p)) {
            last_prefix_index = p;
        }
        delta2[p - 1] = last_prefix_index + (patSize - p);
    }

    // second loop
    for (size_t p = 0; p < patSize - 1; p++) {
        size_t slen = suffix_length<caseSensitive>(patBegin, patEnd, p);
        if (make_case<caseSensitive>(patBegin[p - slen]) != make_case<caseSensitive>(patBegin[patSize - 1 - slen])) {
            delta2[patSize - 1 - slen] = patSize - 1 - p + slen;
        }
    }

    return delta2;
}

template<bool caseSensitive, class T, class U>
size_t boyer_moore_it(const T& string, size_t begin, size_t end, const U& patBegin, const U& patEnd) {
    // The empty pattern must be considered specially
    size_t patSize = patEnd - patBegin;
    if (patSize == 0 || patSize > (end - begin)) {
        return UINT_MAX;
    }

    const std::map<wchar_t, size_t> delta1(make_delta1<caseSensitive>(patBegin, patEnd));
    const std::vector<size_t> delta2(make_delta2<caseSensitive>(patBegin, patEnd));

    size_t i = begin + patSize;
    while (i < end) {
        size_t j = patSize;
        while (j > 0 && (make_case<caseSensitive>(string[i - 1]) == make_case<caseSensitive>(patBegin[j - 1]))) {
            --i;
            --j;
        }
        if (j == 0) {
            return i;
        }

        std::map<wchar_t, size_t>::const_iterator it = delta1.find(make_case<caseSensitive>(string[i - 1]));
        size_t d1 = it != delta1.end() ? d1 = it->second : patSize;

        i += max(d1, delta2[j - 1]);
    }
    return UINT_MAX;
}

template<bool caseSensitive, class T>
size_t boyer_moore(const T& string, size_t begin, size_t end, std::wstring pat) {
    return boyer_moore_it<caseSensitive>(string.begin(), begin, end, pat.begin(), pat.end());
}

template<bool caseSensitive, class T>
size_t boyer_moore_r(const T& string, size_t begin, size_t end, const std::wstring& pat) {
    size_t p = boyer_moore_it<caseSensitive>(string.rbegin(), string.size() - end, string.size() - begin, pat.rbegin(), pat.rend());
    if (p != UINT_MAX)
        p = string.size() - pat.size() - p;
    return p;
}

template<bool caseSensitive>
size_t boyer_moore(const std::wstring& string, const std::wstring& pat) {
    return boyer_moore<caseSensitive>(string, 0, string.size(), pat);
}

template<bool caseSensitive>
size_t boyer_moore_r(const std::wstring& string, const std::wstring& pat) {
    return boyer_moore_r<caseSensitive>(string, 0, string.size(), pat);
}
