#pragma once
#include "TeenAstroMath.h"

/// Alt-az mounts only use POLE_UNDER (axis 2 inside +/-90 deg). An equatorial
/// fork may use POLE_OVER so a goto can pass the pole and look under Polaris.
/// A German equatorial uses both sides as well, via the meridian flip.
inline bool poleSideSupported(bool altAz, PoleSide side)
{
  return !altAz || side != POLE_OVER;
}

/// Pole side named by :SmE# / :SmW#. East is POLE_UNDER on an alt-az or in the
/// northern hemisphere; the southern equatorial sky swaps the two.
inline PoleSide poleSideFromPierLetter(char letter, bool altAzOrNorth)
{
  return ((letter == 'E') == altAzOrNorth) ? POLE_UNDER : POLE_OVER;
}
