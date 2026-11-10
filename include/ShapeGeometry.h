// ShapeGeometry.h - Helpers that build lines, arrows, ovals and other shapes from triangles.

#pragma once

#include <algorithm>
#include <cmath>

// Shape builders that write triangles to any sink with position, colour and triangle calls.
namespace shapes {
// The constant pi.
constexpr float kPi = 3.14159265358979f;

// Adds one coloured vertex.
template <class Sink, class Col> void vertex(Sink* s, float x, float y, float z, const Col& c)
{
    s->position(x, y, z);
    s->colour(c);
}

// Adds a triangle visible from both sides.
template <class Sink> void twoSided(Sink* s, int a, int b, int c)
{
    s->triangle(a, b, c);
    s->triangle(a, c, b);
}

// Adds a filled four-cornered shape.
template <class Sink, class Col>
void quad(Sink* s, int& index, float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
          float z, const Col& c)
{
    vertex(s, ax, ay, z, c);
    vertex(s, bx, by, z, c);
    vertex(s, cx, cy, z, c);
    vertex(s, dx, dy, z, c);
    twoSided(s, index, index + 1, index + 2);
    twoSided(s, index, index + 2, index + 3);
    index += 4;
}

// Adds a filled rectangle.
template <class Sink, class Col>
void rect(Sink* s, int& index, float x0, float y0, float x1, float y1, float z, const Col& c)
{
    quad(s, index, x0, y0, x1, y0, x1, y1, x0, y1, z, c);
}

// Adds a filled triangle.
template <class Sink, class Col>
void triangleFill(Sink* s, int& index, float ax, float ay, float bx, float by, float cx, float cy, float z,
                  const Col& c)
{
    vertex(s, ax, ay, z, c);
    vertex(s, bx, by, z, c);
    vertex(s, cx, cy, z, c);
    twoSided(s, index, index + 1, index + 2);
    index += 3;
}

// Adds a line of the given width; false if both ends coincide.
template <class Sink, class Col>
bool thickLine(Sink* s, int& index, float x1, float y1, float x2, float y2, float width, float z,
               const Col& c, bool cap = true)
{
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-5f) {
        return false;
    }
    const float ux = dx / len;
    const float uy = dy / len;
    const float h = width * 0.5f;
    const float e = cap ? h : 0.0f;
    const float nx = -uy * h;
    const float ny = ux * h;
    const float sx = x1 - ux * e;
    const float sy = y1 - uy * e;
    const float tx = x2 + ux * e;
    const float ty = y2 + uy * e;
    quad(s, index, sx + nx, sy + ny, tx + nx, ty + ny, tx - nx, ty - ny, sx - nx, sy - ny, z, c);
    return true;
}

// Adds a dashed line and returns the number of dashes.
template <class Sink, class Col>
int dashedLine(Sink* s, int& index, float x1, float y1, float x2, float y2, float width, float dash,
               float gap, float z, const Col& c)
{
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-5f || dash <= 0.0f) {
        return 0;
    }
    const float ux = dx / len;
    const float uy = dy / len;
    int count = 0;
    for (float t = 0.0f; t < len; t += dash + gap) {
        const float t2 = std::min(len, t + dash);
        thickLine(s, index, x1 + ux * t, y1 + uy * t, x1 + ux * t2, y1 + uy * t2, width, z, c, false);
        ++count;
    }
    return count;
}

// Adds an arrow head at a tip pointing along a direction; false for no direction.
template <class Sink, class Col>
bool arrowHead(Sink* s, int& index, float tx, float ty, float dx, float dy, float size, float z, const Col& c)
{
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) {
        return false;
    }
    const float ux = dx / len;
    const float uy = dy / len;
    const float bx = tx - ux * size;
    const float by = ty - uy * size;
    const float hw = size * 0.55f;
    triangleFill(s, index, tx, ty, bx - uy * hw, by + ux * hw, bx + uy * hw, by - ux * hw, z, c);
    return true;
}

// Adds a filled diamond.
template <class Sink, class Col>
void diamond(Sink* s, int& index, float x, float y, float radius, float z, const Col& c)
{
    quad(s, index, x, y + radius, x + radius, y, x, y - radius, x - radius, y, z, c);
}

// Adds an arc of an ellipse with the given line width.
template <class Sink, class Col>
void ellipseBand(Sink* s, int& index, float cx, float cy, float rx, float ry, float a0, float a1, float width,
                 float z, const Col& c, int segments = 64)
{
    const float sweep = a1 - a0;
    if (std::fabs(sweep) < 1e-6f) {
        return;
    }
    const int n = std::max(1, static_cast<int>(std::ceil(std::fabs(sweep) / (2.0f * kPi) * segments)));
    const float h = width * 0.5f;
    const int first = index;
    for (int i = 0; i <= n; ++i) {
        const float a = a0 + sweep * static_cast<float>(i) / n;
        const float ca = std::cos(a);
        const float sa = std::sin(a);

        float nx = ca / rx;
        float ny = sa / ry;
        const float nl = std::sqrt(nx * nx + ny * ny);
        nx /= nl;
        ny /= nl;
        const float px = cx + rx * ca;
        const float py = cy + ry * sa;
        vertex(s, px - nx * h, py - ny * h, z, c);
        vertex(s, px + nx * h, py + ny * h, z, c);
    }
    for (int i = 0; i < n; ++i) {
        const int a = first + i * 2;
        twoSided(s, a, a + 1, a + 3);
        twoSided(s, a, a + 3, a + 2);
    }
    index += (n + 1) * 2;
}

// Adds a filled elliptical sector.
template <class Sink, class Col>
void ellipseFan(Sink* s, int& index, float cx, float cy, float rx, float ry, float a0, float a1, float z,
                const Col& c, int segments = 64)
{
    const float sweep = a1 - a0;
    if (std::fabs(sweep) < 1e-6f) {
        return;
    }
    const int n = std::max(1, static_cast<int>(std::ceil(std::fabs(sweep) / (2.0f * kPi) * segments)));
    const int centre = index;
    vertex(s, cx, cy, z, c);
    for (int i = 0; i <= n; ++i) {
        const float a = a0 + sweep * static_cast<float>(i) / n;
        vertex(s, cx + rx * std::cos(a), cy + ry * std::sin(a), z, c);
    }
    for (int i = 0; i < n; ++i) {
        twoSided(s, centre, centre + 1 + i, centre + 2 + i);
    }
    index += n + 2;
}

// Adds an invisible triangle if nothing was drawn, so the object is valid.
template <class Sink, class Col> void ensureNotEmpty(Sink* s, int& index, const Col& c)
{
    if (index == 0) {
        triangleFill(s, index, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c);
    }
}

// Multiplies a colour by a factor, limited to 1.
template <class Col> Col scaled(const Col& c, float f)
{
    return Col(std::min(1.0f, c.r * f), std::min(1.0f, c.g * f), std::min(1.0f, c.b * f));
}

// Mixes a colour toward white.
template <class Col> Col towardWhite(const Col& c, float t)
{
    return Col(c.r + (1.0f - c.r) * t, c.g + (1.0f - c.g) * t, c.b + (1.0f - c.b) * t);
}
}
