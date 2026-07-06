#ifndef DIALOG_SEQUENCES_HPP
#define DIALOG_SEQUENCES_HPP

#include "raylib.h"
#include <string>

// Dialog sequence entry
struct DialogEntry {
    std::string speaker;
    std::string text;
    Color speakerColor;
    Color portraitColor;
};

#endif
