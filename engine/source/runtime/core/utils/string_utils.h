#pragma once

#include <regex>
#include <string>

namespace Piccolo
{
    namespace string_utils
    {
        // case-insensitive string comparison

        // note: strcasecmp is a POSIX function and there is no standardized
        // equivalent as of C++17
        template<typename T>
        bool strcasecmp(T const& a, T const& b)
        {
            return a.size() == b.size() &&
                   std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        }

        template<typename T>
        bool strcasencmp(T const& a, T const& b, size_t n)
        {
            return a.size() >= n && b.size() >= n &&
                   std::equal(a.begin(), a.begin() + n, b.begin(), b.begin() + n, [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        }

        inline bool starts_with(std::string_view const& value, std::string_view const& beginning)
        {
            if (beginning.size() > value.size())
                return false;

            return std::equal(beginning.begin(), beginning.end(), value.begin());
        }

        inline bool ends_with(std::string_view const& value, std::string_view const& ending)
        {
            if (ending.size() > value.size())
                return false;

            return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        }

        // Trim white space

        // for "C" locale characters trimmed are ' ', '\f', '\n', '\r', '\t', '\v'
        // see: https://en.cppreference.com/w/cpp/string/byte/isspace

        inline void ltrim(std::string_view& s)
        {
            s.remove_prefix(std::distance(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); })));
        }

        inline void ltrim(std::string& s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
        }

        inline void rtrim(std::string_view& s)
        {
            s.remove_suffix(std::distance(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(), s.end()));
        }

        inline void rtrim(std::string& s)
        {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
        }

        template<typename T>
        void trim(T& s)
        {
            ltrim(s);
            rtrim(s);
        }

        // trim specific characters

        inline void ltrim(std::string_view& s, int c)
        {
            s.remove_prefix(std::distance(s.begin(), std::find_if(s.begin(), s.end(), [&](int ch) { return ch != c; })));
        }

        inline void ltrim(std::string& s, int c)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](int ch) { return ch != c; }));
        }

        inline void rtrim(std::string_view& s, int c)
        {
            s.remove_suffix(std::distance(std::find_if(s.rbegin(), s.rend(), [&](int ch) { return ch != c; }).base(), s.end()));
        }

        inline void rtrim(std::string& s, int c)
        {
            s.erase(std::find_if(s.rbegin(), s.rend(), [&](int ch) { return ch != c; }).base(), s.end());
        }

        template<typename T>
        void trim(T& s, int c)
        {
            ltrim(s, c);
            rtrim(s, c);
        }

        // upper/lower case conversions

        inline void tolower(std::string& s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(static_cast<int>(c))); });
        }

        inline void toupper(std::string& s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(static_cast<int>(c))); });
        }

        // regex tokens split (default delimiter are space, comma, pipe and semi-colon characters)
        // note : std::regex throws runt_time exceptions on invalid expressions
        inline std::vector<std::string> split(std::string const& s, char const* regex = "[\\s+,|:]")
        {
            std::regex                 rx(regex);
            std::sregex_token_iterator it {s.begin(), s.end(), rx, -1}, end;

            std::vector<std::string> tokens;
            for (; it != end; ++it)
                if (!it->str().empty())
                    tokens.push_back(*it);
            return tokens;
        }

        inline std::vector<std::string_view> split(std::string_view const s, char const* regex = "[\\s+,|:]")
        {
            std::regex                 rx(regex);
            std::cregex_token_iterator it {s.data(), s.data() + s.size(), rx, -1}, end;

            std::vector<std::string_view> tokens;
            for (; it != end; ++it)
                if (it->length() > 0)
                    tokens.push_back({it->first, (size_t)it->length()});
            return tokens;
        }

    } // namespace string_utils
} // namespace Piccolo
