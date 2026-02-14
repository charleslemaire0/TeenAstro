#pragma once
/**
 * Command layer globals: MainUnit.h plus everything command handlers need
 * (Site, TelTimer, timerLoop, Coord_LO, etc.). Include this or Command.h when
 * implementing/dispatching commands; use MainUnit.h when you only need mount + MainUnitDecl.
 */
#include "MainUnit.h"

#include "timerLoop.hpp"
#include "TelTimer.hpp"
#include "Site.hpp"
#include "TeenAstroCoord_LO.hpp"
