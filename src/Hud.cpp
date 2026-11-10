// Hud.cpp - Implementation of Hud.

#include "Hud.h"

void Hud::create(const HudContext& context)
{
    context_ = context;

    pause_.create(context_);
    legend_.create(context_);
    controls_.create(context_);
    status_.create(context_);
    callStack_.create(context_);
}
