// StrokeFont.h - Built-in font made of line segments.

#pragma once

#include <string>
#include <vector>

// A built-in font drawn with line segments.
namespace strokefont {
// One line of a letter.
struct Segment
{
    float x1, y1, x2, y2;
};

// Height of a capital letter in glyph units.
constexpr float kCapHeight = 6.0f;
// Distance between letters in glyph units.
constexpr float kAdvance = 5.6f;
// Size of lower case letters relative to capitals.
constexpr float kLowerScale = 0.78f;

// Stroke description of a character, or nullptr if unknown.
inline const char* glyphData(int c)
{
    switch (c) {
        case ' ':
            return "";
        case 'A':
            return "00 26 40|12 32";
        case 'B':
            return "00 06 36 45 44 33 03|33 42 41 30 00";
        case 'C':
            return "45 36 16 05 01 10 30 41";
        case 'D':
            return "00 06 26 44 42 20 00";
        case 'E':
            return "46 06 00 40|03 33";
        case 'F':
            return "46 06 00|03 33";
        case 'G':
            return "45 36 16 05 01 10 30 41 43 23";
        case 'H':
            return "00 06|40 46|03 43";
        case 'I':
            return "16 36|26 20|10 30";
        case 'J':
            return "46 41 30 10 01";
        case 'K':
            return "00 06|46 03 40";
        case 'L':
            return "06 00 40";
        case 'M':
            return "00 06 23 46 40";
        case 'N':
            return "00 06 40 46";
        case 'O':
            return "10 01 05 16 36 45 41 30 10";
        case 'P':
            return "00 06 36 45 44 33 03";
        case 'Q':
            return "10 01 05 16 36 45 41 30 10|22 40";
        case 'R':
            return "00 06 36 45 44 33 03|23 40";
        case 'S':
            return "45 36 16 05 04 13 33 42 41 30 10 01";
        case 'T':
            return "06 46|26 20";
        case 'U':
            return "06 01 10 30 41 46";
        case 'V':
            return "06 20 46";
        case 'W':
            return "06 10 23 30 46";
        case 'X':
            return "06 40|46 00";
        case 'Y':
            return "06 23 46|23 20";
        case 'Z':
            return "06 46 00 40";
        case '0':
            return "10 01 05 16 36 45 41 30 10|11 35";
        case '1':
            return "15 26 20|10 30";
        case '2':
            return "05 16 36 45 44 00 40";
        case '3':
            return "05 16 36 45 44 33 23|33 42 41 30 10 01";
        case '4':
            return "30 36 02 42";
        case '5':
            return "46 06 03 33 42 41 30 10 01";
        case '6':
            return "36 16 05 01 10 30 41 42 33 03";
        case '7':
            return "06 46 20";
        case '8':
            return "13 02 01 10 30 41 42 33 13 04 05 16 36 45 44 33";
        case '9':
            return "43 13 04 05 16 36 45 41 30 10";
        case '_':
            return "00 40";
        case '-':
            return "03 43";
        case '+':
            return "03 43|21 25";
        case '=':
            return "02 42|04 44";
        case ':':
            return "11 21 22 12 11|14 24 25 15 14";
        case '.':
            return "10 20 21 11 10";
        case ',':
            return "21 10";
        case '!':
            return "26 22|20 21";
        case '?':
            return "05 16 36 45 44 23 22|20 21";
        case '\'':
            return "36 35";
        case '/':
            return "00 46";
        case '(':
            return "36 14 12 30";
        case ')':
            return "16 34 32 10";
        case '[':
            return "36 16 10 30";
        case ']':
            return "16 36 30 10";
        case '*':
            return "03 43|24 22|14 32|12 34";
        case '%':
            return "00 46|15 16 26 25 15|31 30 40 41 31";
        case ';':
            return "14 24 25 15 14|21 10";
        case '"':
            return "16 15|36 35";
        case '{':
            return "36 26 24 13 22 20 30";
        case '}':
            return "16 26 24 33 22 20 10";
        case '<':
            return "36 13 30";
        case '>':
            return "16 33 10";
        case '&':
            return "40 04 15 26 35 24 02 11 20 30 42";
        case '|':
            return "26 20";
        case '\\':
            return "06 40";
        case '~':
            return "02 13 22 31 42";
        case '^':
            return "03 26 43";
        case '#':
            return "16 10|36 30|02 42|04 44";
        default:
            return nullptr;
    }
}

// Stroke description of the box shown for unknown characters.
inline const char* unknownGlyph()
{
    return "00 06 46 40 00";
}

// Adds the lines of one glyph at a position and scale.
inline void appendGlyph(std::vector<Segment>& out, const char* data, float ox, float oy, float scale)
{
    float px = 0.0f, py = 0.0f;
    bool havePrevious = false;
    for (const char* p = data; *p;) {
        if (*p == '|') {
            havePrevious = false;
            ++p;
            continue;
        }
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (!p[1]) {
            break;
        }
        const float x = ox + static_cast<float>(p[0] - '0') * scale;
        const float y = oy + static_cast<float>(p[1] - '0') * scale;
        p += 2;
        if (havePrevious) {
            out.push_back({px, py, x, y});
        }
        px = x;
        py = y;
        havePrevious = true;
    }
}

// Lines of a whole text at the given capital height, starting at the origin.
inline std::vector<Segment> layout(const std::string& text, float capHeight)
{
    std::vector<Segment> segments;
    const float unit = capHeight / kCapHeight;
    float x = 0.0f;
    for (char raw : text) {
        const unsigned char ch = static_cast<unsigned char>(raw);
        const bool lower = ch >= 'a' && ch <= 'z';
        const int key = lower ? ch - 'a' + 'A' : ch;
        const float scale = unit * (lower ? kLowerScale : 1.0f);

        const char* data = glyphData(key);
        appendGlyph(segments, data ? data : unknownGlyph(), x, 0.0f, scale);
        x += kAdvance * scale;
    }
    return segments;
}
}
