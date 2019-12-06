#include <fmt/printf.h>
#include <iostream>
#include <celengine/overlay.h>
#include <celutil/color.h>
#include "textblock.h"

using namespace std;

namespace
{
enum MarkupState
{
    Begin,
    End,
    OpeningBracket,
    ClosingBracket,
    Tag,
    TagEnd,
    ClosingTag,
    Span,
    Markup,
    Error
};

inline void SkipWS(const string &s, string::size_type &pos)
{
    auto p = s.find_first_not_of(" \t", pos);
    if (p != string::npos)
        pos = p;
}

bool ParseAttribute(const string &s, string::size_type &pos, string &key, string &value, MarkupState &state, const char **error)
{
    SkipWS(s, pos);
    if (s[pos] == '>')
    {
        state = TagEnd;
        return true;
    }

    auto p = s.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_", pos);
    if (p == string::npos)
    {
        *error = "unterminated tag";
        return false;
    }
    key = s.substr(pos, p-pos);
    pos = p;

    SkipWS(s, pos);
    if (s[pos++] != '=')
    {
        *error = "= expected";
        return false;
    }
    SkipWS(s, pos);
    if (s[pos++] != '"')
    {
        *error = "openning \" expected";
        return false;
    }
    p = s.find('"', pos);
    if (p == string::npos)
    {
        *error = "closing \" expected";
        return false;
    }
    value = s.substr(pos, p-pos);
    pos = p+1;
    return true;
}

}; // namespace

namespace celestia
{

TextBlock::TextBlock(const Overlay &overlay,
                     const Eigen::Vector3f &position,
                     const Color &color) :
    m_overlay(overlay),
    m_position(position),
    m_defaultColor(color)
{
}

bool TextBlock::add(const string &s)
{
    return ParseMarkup(s);
}

bool TextBlock::ParseMarkup(const string &s)
{
    MarkupState state = Begin;
    string::size_type pos = 0, p = 0;
    Color color = m_defaultColor;
    bool closingTag = false;

    for (;pos < s.length();)
    {
        switch (state)
        {
        case Begin:
            p = s.find('<', pos);
            if (p == string::npos)
            {
                m_blocks.emplace_back({ s.substr(pos), color });
                state = End;
            }
            else
            {
                m_blocks.emplace_back({ s.substr(pos, p-pos), color });
                state = OpeningBracket;
                pos = p;
            }
            break;
        case End:
            return true;
        case OpeningBracket:
            ++pos;
            state = closingTag ? ClosingTag : Tag;
            break;
        case Tag:
            if (s.compare(pos, 4, "span") == 0)
            {
                pos += 4;
                state = Span;
            }
            else if (s.compare(pos, 6, "markup") == 0)
            {
                pos += 6;
                state = Markup;
            }
            closingTag = true;
            break;
        case ClosingTag:
            closingTag = false;
            if (s[pos++] == '/')
            {
                if (s.compare(pos, 4, "span") == 0)
                {
                    pos += 4;
                    state = ClosingBracket;
                    color = m_defaultColor;
                }
                else if (s.compare(pos, 6, "markup") == 0)
                {
                    pos += 6;
                    state = ClosingBracket;
                }
                else
                {
                    state = Error;
                    m_error = "markup or span expected";
                }
            }
            else
            {
                state = Error;
                m_error = "/ expected";
            }
            break;
        case Span:
            for (;state != Error && state != TagEnd;)
            {
                string key, value;
                if (!ParseAttribute(s, pos, key, value, state, m_error))
                {
                    state = Error;
                }
                else if (state != TagEnd)
                {
                    if (key == "foreground")
                    {
                        if (!Color::parse(value, color))
                        {
                            m_error = "incorrect color value";
                            state = Error;
                        }
                    }
                    else
                    {
                        m_error = "unknown attribute";
                        state = Error;
                    }
                }
                else
                {
                    state = Error;
                }
            }
            break;
        case Markup:
            state = TagEnd;
        case TagEnd:
            SkipWS(s, pos);
            state = ClosingBracket;
            break;
        case ClosingBracket:
            if (s[pos] == '>')
            {
                pos++;
                state = Begin;
            }
            else
            {
                state = Error;
                error = "> expected";
            }
            break;
        case Error:
            m_errorPos = pos;
            return false;
        default:
            break;
        }
    }
    if (state != End && state != Begin)
    {
        m_error = "premature end of string";
        return false;
    }
    return true;
}

bool TextBlock::ParseAttribute(std::string &s, string::size_type &pos, string &key, string &value)
{
    SkipWS(s, pos);
    if (s[pos] == '>')
    {
        state = TagEnd;
        return true;
    }

    auto p = s.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_", pos);
    if (p == string::npos)
    {
        m_error = "unterminated tag";
        return false;
    }

    key = s.substr(pos, p-pos);

    SkipWS(s, pos);
    if (s[pos++] != '=')
    {
        m_error = "= expected";
        return false;
    }
    SkipWS(s, pos);
    if (s[pos++] != '"')
    {
        m_error = "openning \" expected";
        return false;
    }
    p = s.find('"', pos);
    if (p == string::npos)
    {
        m_error = "closing \" expected";
        return false;
    }
    value = s.substr(pos, p-pos);
    pos = p+1;
    return true;
}

const char* TextBlock::getError() const
{
    return m_error;
}
};
