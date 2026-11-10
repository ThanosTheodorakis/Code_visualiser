// configParser.cpp - Implementation of configParser.

#include "configParser.h"

#include <algorithm>
#include <fstream>
#include <sstream>

configParser::configParser(const std::string& filePath)
{
    loadDefaults();
    loadFromFile(filePath);
}

const ConfigData& configParser::getConfig() const
{
    return configData;
}

void configParser::loadDefaults()
{
    configData.flowLineColor = Ogre::ColourValue(0.60f, 0.66f, 0.76f);
    configData.branchLineColor = Ogre::ColourValue(1.00f, 0.68f, 0.15f);
    configData.loopLineColor = Ogre::ColourValue(0.25f, 0.85f, 0.45f);
    configData.functionLineColor = Ogre::ColourValue(0.35f, 0.62f, 1.00f);
    configData.connectorLineColor = Ogre::ColourValue(0.85f, 0.60f, 1.00f);
    configData.statementLineColor = Ogre::ColourValue(0.92f, 0.92f, 0.92f);
    configData.inputLineColor = Ogre::ColourValue(0.20f, 0.85f, 0.85f);
    configData.outputLineColor = Ogre::ColourValue(1.00f, 0.45f, 0.75f);
    configData.terminalLineColor = Ogre::ColourValue(0.95f, 0.42f, 0.42f);
    configData.switchLineColor = configData.branchLineColor;
    configData.decisionLineColor = configData.branchLineColor;
    configData.iterationLineColor = configData.loopLineColor;
    configData.cursorColor = Ogre::ColourValue(1.00f, 1.00f, 1.00f);
    configData.backgroundColor = Ogre::ColourValue(0.00f, 0.00f, 0.00f);
    configData.maxIterationsShown = 6;
}

void configParser::applyFallbacks(bool switchSet, bool decisionSet, bool iterationSet)
{
    if (!switchSet) {
        configData.switchLineColor = configData.branchLineColor;
    }
    if (!decisionSet) {
        configData.decisionLineColor = configData.branchLineColor;
    }
    if (!iterationSet) {
        configData.iterationLineColor = configData.loopLineColor;
    }
}

std::string configParser::trim(const std::string& value)
{
    const std::string whitespace = " \t\r\n";

    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

bool configParser::parseColor(const std::string& value, Ogre::ColourValue& color) const
{
    std::string normalizedValue = value;
    std::replace(normalizedValue.begin(), normalizedValue.end(), ',', ' ');

    std::stringstream stream(normalizedValue);

    float red;
    float green;
    float blue;
    float alpha = 1.0f;

    if (!(stream >> red >> green >> blue)) {
        return false;
    }

    if (stream >> alpha) {
        color = Ogre::ColourValue(red, green, blue, alpha);
    } else {
        color = Ogre::ColourValue(red, green, blue);
    }

    return true;
}

void configParser::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    bool switchSet = false;
    bool decisionSet = false;
    bool iterationSet = false;

    while (std::getline(file, line)) {
        const std::size_t commentPosition = line.find('#');
        if (commentPosition != std::string::npos) {
            line.erase(commentPosition);
        }

        const std::size_t separatorPosition = line.find('=');
        if (separatorPosition == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separatorPosition));
        const std::string value = trim(line.substr(separatorPosition + 1));

        if (key == "flowLineColor") {
            parseColor(value, configData.flowLineColor);
        } else if (key == "branchLineColor") {
            parseColor(value, configData.branchLineColor);
        } else if (key == "loopLineColor") {
            parseColor(value, configData.loopLineColor);
        } else if (key == "functionLineColor") {
            parseColor(value, configData.functionLineColor);
        } else if (key == "connectorLineColor") {
            parseColor(value, configData.connectorLineColor);
        } else if (key == "statementLineColor") {
            parseColor(value, configData.statementLineColor);
        } else if (key == "inputLineColor") {
            parseColor(value, configData.inputLineColor);
        } else if (key == "outputLineColor") {
            parseColor(value, configData.outputLineColor);
        } else if (key == "terminalLineColor") {
            parseColor(value, configData.terminalLineColor);
        } else if (key == "switchLineColor") {
            switchSet = parseColor(value, configData.switchLineColor);
        } else if (key == "decisionLineColor") {
            decisionSet = parseColor(value, configData.decisionLineColor);
        } else if (key == "iterationLineColor") {
            iterationSet = parseColor(value, configData.iterationLineColor);
        } else if (key == "cursorColor") {
            parseColor(value, configData.cursorColor);
        } else if (key == "backgroundColor") {
            parseColor(value, configData.backgroundColor);
        } else if (key == "maxIterationsShown") {
            try {
                configData.maxIterationsShown = std::max(0, std::stoi(value));
            } catch (...) {}
        }
    }

    applyFallbacks(switchSet, decisionSet, iterationSet);
}
