// configParser.h - Colours and settings read from config.cfg.

#pragma once

#include "Ogre.h"

#include <string>

// Colours and limits that config.cfg can set.
struct ConfigData
{
    Ogre::ColourValue flowLineColor;
    Ogre::ColourValue branchLineColor;
    Ogre::ColourValue loopLineColor;
    Ogre::ColourValue functionLineColor;
    Ogre::ColourValue connectorLineColor;
    Ogre::ColourValue statementLineColor;
    Ogre::ColourValue inputLineColor;
    Ogre::ColourValue outputLineColor;
    Ogre::ColourValue terminalLineColor;

    Ogre::ColourValue switchLineColor;
    Ogre::ColourValue decisionLineColor;
    Ogre::ColourValue iterationLineColor;

    Ogre::ColourValue cursorColor;
    Ogre::ColourValue backgroundColor;

    int maxIterationsShown = 6;
};

// Reads config.cfg; a missing line keeps its default.
class configParser
{
public:
    // Loads the defaults, then the file.
    explicit configParser(const std::string& filePath);

    // The loaded settings.
    const ConfigData& getConfig() const;

private:
    ConfigData configData;

    // Fills in the built-in values.
    void loadDefaults();
    // Switch, decision and iteration colours follow their group colour unless set.
    void applyFallbacks(bool switchSet, bool decisionSet, bool iterationSet);
    // Reads the settings from the file.
    void loadFromFile(const std::string& filePath);

    // Reads "r,g,b" into a colour; false if malformed.
    bool parseColor(const std::string& value, Ogre::ColourValue& color) const;

    // Removes leading and trailing spaces.
    static std::string trim(const std::string& value);
};
