// shapes_test.cpp - Tests for the drawing helpers and the stroke font.

#include <cmath>
#include <string>
#include <vector>

#include "ShapeGeometry.h"
#include "StrokeFont.h"
#include "test_util.h"

struct Colour
{
    float r, g, b;
};

struct Sink
{
    struct P
    {
        float x, y, z;
    };

    std::vector<P> points;
    std::vector<Colour> colours;
    std::vector<int> indices;

    void position(float x, float y, float z) { points.push_back({x, y, z}); }

    void colour(const Colour& c) { colours.push_back(c); }

    void triangle(int a, int b, int c)
    {
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    }

    int triangles() const { return static_cast<int>(indices.size() / 3); }

    bool indicesInRange() const
    {
        for (int i : indices) {
            if (i < 0 || i >= static_cast<int>(points.size())) {
                return false;
            }
        }
        return true;
    }
};

static const Colour kWhite{1, 1, 1};

static void rectTests()
{
    Sink s;
    int index = 0;
    shapes::rect(&s, index, 0.0f, 0.0f, 10.0f, 5.0f, 3.0f, kWhite);

    CHECK_EQ(s.points.size(), 4u);
    CHECK_EQ(index, 4);
    CHECK_EQ(s.triangles(), 4);
    CHECK(s.indicesInRange());
    for (const auto& p : s.points) {
        CHECK_EQ(p.z, 3.0f);
    }
    CHECK_EQ(s.colours.size(), s.points.size());
}

static void indexContinuesTests()
{
    Sink s;
    int index = 0;
    shapes::rect(&s, index, 0, 0, 1, 1, 0, kWhite);
    shapes::triangleFill(&s, index, 0, 0, 1, 0, 0, 1, 0, kWhite);
    CHECK_EQ(index, 7);
    CHECK_EQ(s.points.size(), 7u);
    CHECK(s.indicesInRange());

    int smallest = 100;
    for (std::size_t i = 12; i < s.indices.size(); ++i) {
        smallest = std::min(smallest, s.indices[i]);
    }
    CHECK_EQ(smallest, 4);
}

static void thickLineTests()
{
    {
        Sink s;
        int index = 0;
        const bool drawn = shapes::thickLine(&s, index, 0, 0, 10, 0, 2.0f, 0.0f, kWhite);
        CHECK(drawn);
        CHECK(s.triangles() > 0);
        CHECK(s.indicesInRange());

        for (const auto& p : s.points) {
            CHECK(std::fabs(p.y) <= 1.0001f);
        }
    }
    {
        Sink s;
        int index = 0;
        const bool drawn = shapes::thickLine(&s, index, 5, 5, 5, 5, 2.0f, 0.0f, kWhite);
        CHECK(!drawn);
    }
}

static void diamondAndEllipseTests()
{
    {
        Sink s;
        int index = 0;
        shapes::diamond(&s, index, 10.0f, 20.0f, 4.0f, 0.0f, kWhite);
        CHECK_EQ(s.points.size(), 4u);
        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        for (const auto& p : s.points) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }
        CHECK_EQ(minX, 6.0f);
        CHECK_EQ(maxX, 14.0f);
        CHECK_EQ(minY, 16.0f);
        CHECK_EQ(maxY, 24.0f);
    }
    {
        Sink s;
        int index = 0;
        shapes::ellipseBand(&s, index, 0, 0, 10.0f, 5.0f, 0.0f, 2.0f * shapes::kPi, 2.0f, 0.0f, kWhite);
        CHECK(s.triangles() > 0);
        CHECK(s.indicesInRange());
        CHECK_EQ(index, static_cast<int>(s.points.size()));

        for (const auto& p : s.points) {
            const float k = (p.x * p.x) / (10.0f * 10.0f) + (p.y * p.y) / (5.0f * 5.0f);
            CHECK(k > 0.5f && k < 1.6f);
        }
    }
    {
        Sink s;
        int index = 0;
        shapes::ellipseBand(&s, index, 0, 0, 10.0f, 5.0f, 1.0f, 1.0f, 2.0f, 0.0f, kWhite);
        CHECK(s.points.empty());
    }
}

static void arrowHeadTests()
{
    Sink s;
    int index = 0;
    const bool drawn = shapes::arrowHead(&s, index, 10.0f, 0.0f, 1.0f, 0.0f, 6.0f, 0.0f, kWhite);
    CHECK(drawn);
    CHECK(s.triangles() > 0);

    float maxX = -1e9f;
    for (const auto& p : s.points) {
        maxX = std::max(maxX, p.x);
    }
    CHECK(std::fabs(maxX - 10.0f) < 0.001f);

    Sink t;
    int i2 = 0;
    CHECK(!shapes::arrowHead(&t, i2, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, kWhite));
}

static void ensureNotEmptyTests()
{
    Sink s;
    int index = 0;
    shapes::ensureNotEmpty(&s, index, kWhite);
    CHECK(!s.points.empty());

    Sink t;
    int i2 = 0;
    shapes::rect(&t, i2, 0, 0, 1, 1, 0, kWhite);
    const std::size_t before = t.points.size();
    shapes::ensureNotEmpty(&t, i2, kWhite);
    CHECK_EQ(t.points.size(), before);
}

static void fontTests()
{
    using namespace strokefont;

    CHECK(layout("", 12.0f).empty());
    CHECK(layout(" ", 12.0f).empty());
    CHECK(!layout("A", 12.0f).empty());

    const auto upper = layout("A", 12.0f);
    const auto lower = layout("a", 12.0f);
    CHECK_EQ(upper.size(), lower.size());

    const std::string drawable = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-+=:.,!?";
    for (char c : drawable) {
        CHECK(!layout(std::string(1, c), 12.0f).empty());
    }

    CHECK(!layout("\x01", 12.0f).empty());

    auto width = [](const std::vector<Segment>& v) {
        float m = 0.0f;
        for (const auto& s : v) {
            m = std::max(m, std::max(s.x1, s.x2));
        }
        return m;
    };
    CHECK(width(layout("ABC", 12.0f)) > width(layout("AB", 12.0f)));
    CHECK(width(layout("AB", 24.0f)) > width(layout("AB", 12.0f)) * 1.9f);

    for (const auto& s : layout("Hello World 123", 12.0f)) {
        CHECK(s.y1 >= -0.001f && s.y2 >= -0.001f);
        CHECK(s.y1 <= 12.5f && s.y2 <= 12.5f);
    }
}

int main()
{
    rectTests();
    indexContinuesTests();
    thickLineTests();
    diamondAndEllipseTests();
    arrowHeadTests();
    ensureNotEmptyTests();
    fontTests();
    return finish("shapes_test");
}
