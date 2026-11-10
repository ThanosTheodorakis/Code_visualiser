// config_test.cpp - Tests for the config file reader (needs the Ogre headers).

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include "configParser.h"
#include "test_util.h"

static bool same(const Ogre::ColourValue& c, float r, float g, float b, float a = 1.0f)
{
    return std::fabs(c.r - r) < 1e-4f && std::fabs(c.g - g) < 1e-4f && std::fabs(c.b - b) < 1e-4f &&
           std::fabs(c.a - a) < 1e-4f;
}

static std::string writeFile(const char* name, const std::string& text)
{
    std::ofstream out(name);
    out << text;
    return name;
}

static void defaultsTests()
{
    configParser missing("this_file_does_not_exist.cfg");
    const ConfigData& d = missing.getConfig();
    CHECK(same(d.flowLineColor, 0.60f, 0.66f, 0.76f));
    CHECK(same(d.branchLineColor, 1.00f, 0.68f, 0.15f));
    CHECK(same(d.loopLineColor, 0.25f, 0.85f, 0.45f));
    CHECK(same(d.functionLineColor, 0.35f, 0.62f, 1.00f));
    CHECK(same(d.backgroundColor, 0, 0, 0));
    CHECK(same(d.cursorColor, 1, 1, 1));
    CHECK_EQ(d.maxIterationsShown, 6);

    CHECK(same(d.switchLineColor, 1.00f, 0.68f, 0.15f));
    CHECK(same(d.decisionLineColor, 1.00f, 0.68f, 0.15f));
    CHECK(same(d.iterationLineColor, 0.25f, 0.85f, 0.45f));
}

static void readingTests()
{
    const std::string path = writeFile("cv_test_a.cfg", "# a comment line\n"
                                                        "flowLineColor = 0.1, 0.2, 0.3\n"
                                                        "branchLineColor=1 0 0   # trailing comment\n"
                                                        "   loopLineColor =   0,1,0   \n"
                                                        "backgroundColor = 0.5,0.5,0.5,0.25\n"
                                                        "maxIterationsShown = 3\n"
                                                        "unknownKey = 1,2,3\n"
                                                        "this line has no equals sign\n");
    configParser parser(path);
    const ConfigData& c = parser.getConfig();

    CHECK(same(c.flowLineColor, 0.1f, 0.2f, 0.3f));
    CHECK(same(c.branchLineColor, 1, 0, 0));
    CHECK(same(c.loopLineColor, 0, 1, 0));
    CHECK(same(c.backgroundColor, 0.5f, 0.5f, 0.5f, 0.25f));
    CHECK_EQ(c.maxIterationsShown, 3);

    CHECK(same(c.functionLineColor, 0.35f, 0.62f, 1.00f));
    std::remove(path.c_str());
}

static void fallbackTests()
{
    {
        const std::string path =
            writeFile("cv_test_b.cfg", "branchLineColor = 1,0,0\nloopLineColor = 0,0,1\n");
        configParser parser(path);
        const ConfigData& c = parser.getConfig();
        CHECK(same(c.switchLineColor, 1, 0, 0));
        CHECK(same(c.decisionLineColor, 1, 0, 0));
        CHECK(same(c.iterationLineColor, 0, 0, 1));
        std::remove(path.c_str());
    }

    {
        const std::string path = writeFile("cv_test_c.cfg", "branchLineColor = 1,0,0\n"
                                                            "switchLineColor = 0,1,1\n"
                                                            "iterationLineColor = 1,1,0\n");
        configParser parser(path);
        const ConfigData& c = parser.getConfig();
        CHECK(same(c.switchLineColor, 0, 1, 1));
        CHECK(same(c.decisionLineColor, 1, 0, 0));
        CHECK(same(c.iterationLineColor, 1, 1, 0));
        std::remove(path.c_str());
    }
}

static void badInputTests()
{
    const std::string path = writeFile("cv_test_d.cfg", "flowLineColor = banana\n"
                                                        "branchLineColor = 1,0\n"
                                                        "maxIterationsShown = lots\n"
                                                        "loopLineColor = 0,1,0\n");
    configParser parser(path);
    const ConfigData& c = parser.getConfig();
    CHECK(same(c.flowLineColor, 0.60f, 0.66f, 0.76f));
    CHECK(same(c.branchLineColor, 1.00f, 0.68f, 0.15f));
    CHECK_EQ(c.maxIterationsShown, 6);
    CHECK(same(c.loopLineColor, 0, 1, 0));
    std::remove(path.c_str());

    const std::string negative = writeFile("cv_test_e.cfg", "maxIterationsShown = -4\n");
    configParser parser2(negative);
    CHECK_EQ(parser2.getConfig().maxIterationsShown, 0);
    std::remove(negative.c_str());
}

static void shippedConfigTests()
{
    configParser shipped(CV_SHIPPED_CONFIG);
    configParser defaults("this_file_does_not_exist.cfg");
    const ConfigData& a = shipped.getConfig();
    const ConfigData& b = defaults.getConfig();
    CHECK(same(a.flowLineColor, b.flowLineColor.r, b.flowLineColor.g, b.flowLineColor.b));
    CHECK(same(a.branchLineColor, b.branchLineColor.r, b.branchLineColor.g, b.branchLineColor.b));
    CHECK(same(a.loopLineColor, b.loopLineColor.r, b.loopLineColor.g, b.loopLineColor.b));
    CHECK(same(a.functionLineColor, b.functionLineColor.r, b.functionLineColor.g, b.functionLineColor.b));
    CHECK(same(a.connectorLineColor, b.connectorLineColor.r, b.connectorLineColor.g, b.connectorLineColor.b));
    CHECK(same(a.statementLineColor, b.statementLineColor.r, b.statementLineColor.g, b.statementLineColor.b));
    CHECK(same(a.inputLineColor, b.inputLineColor.r, b.inputLineColor.g, b.inputLineColor.b));
    CHECK(same(a.outputLineColor, b.outputLineColor.r, b.outputLineColor.g, b.outputLineColor.b));
    CHECK(same(a.terminalLineColor, b.terminalLineColor.r, b.terminalLineColor.g, b.terminalLineColor.b));
    CHECK(same(a.cursorColor, b.cursorColor.r, b.cursorColor.g, b.cursorColor.b));
    CHECK(same(a.backgroundColor, b.backgroundColor.r, b.backgroundColor.g, b.backgroundColor.b));
    CHECK_EQ(a.maxIterationsShown, b.maxIterationsShown);
}

int main()
{
    defaultsTests();
    readingTests();
    fallbackTests();
    badInputTests();
    shippedConfigTests();
    return finish("config_test");
}
