/**
 * e — eFinder (plate solving). Host commands on USB/SHC; forwarded to the Pi
 * over the Focuser or GNSS port by EfinderManager.
 */
#include "Command.h"
#include "Efinder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
//   e - eFinder
// -----------------------------------------------------------------------------
void Command_e()
{
  switch (commandState.command[1])
  {
  case '?':
    // :e?#  present, port, version, job, last result, GoTo++, threshold
    efinder.formatStatus(commandState.reply, REPLY_BUFFER_LEN);
    break;

  case 'V':
    // :eV#  version string
    if (!efinder.present())
    {
      replyLongFalse();
      break;
    }
    snprintf(commandState.reply, REPLY_BUFFER_LEN, "%s#", efinder.version());
    break;

  case 'S':
    // :eS#  start solve (no sync)
    replyValueSetShort(efinder.startSolve(false));
    break;

  case 'Y':
    // :eY#  solve and sync (or add alignment reference in :AA# / :eA# mode)
    replyValueSetShort(efinder.startSolve(true));
    break;

  case 'R':
    // :eR#  last result / busy
    if (efinder.busy())
    {
      strcpy(commandState.reply, "B#");
    }
    else
    {
      efinder.formatResult(commandState.reply, REPLY_BUFFER_LEN);
    }
    break;

  case 'O':
    // :eO#  start offset calibration
    replyValueSetShort(efinder.startOffset());
    break;

  case 'o':
    // :eo#  reset offset
    replyValueSetShort(efinder.resetOffset());
    break;

  case 'A':
    // :eA#  start eFinder alignment session (alignment-by-sync + first solve)
    replyValueSetShort(efinder.startAlign());
    break;

  case 'B':
    // :eB#  abort current job
    efinder.abort();
    replyShortTrue();
    break;

  case 'P':
    // :eP#  GoTo++ settings: on,thresholdArcmin
    snprintf(commandState.reply, REPLY_BUFFER_LEN, "%u,%u#",
             efinder.gotoPlusPlus() ? 1u : 0u,
             (unsigned)efinder.thresholdArcmin());
    break;

  case 'T':
    // :eT0# / :eT1#  disable / enable GoTo++
    // :eTn#  n=2..60 set threshold arcmin (also leaves GoTo++ as-is)
    if (commandState.command[2] == '0')
    {
      efinder.setGotoPlusPlus(false);
      replyShortTrue();
    }
    else if (commandState.command[2] == '1' && commandState.command[3] == '#')
    {
      efinder.setGotoPlusPlus(true);
      replyShortTrue();
    }
    else if (commandState.command[2] >= '2' && commandState.command[2] <= '9')
    {
      int v = atoi(&commandState.command[2]);
      if (v >= 1 && v <= 60)
      {
        efinder.setThresholdArcmin((uint8_t)v);
        replyShortTrue();
      }
      else replyShortFalse();
    }
    else replyShortFalse();
    break;

  case 'X':
    // :eX...#  camera parameter passthrough to the Pi (:eX...# as-is)
    {
      if (!efinder.present())
      {
        replyLongFalse();
        break;
      }
      char out[40] = ":";
      strncat(out, commandState.command, sizeof(out) - 3);
      strcat(out, "#");
      if (!efinder.passthrough(out, commandState.reply, REPLY_BUFFER_LEN))
        replyLongFalse();
    }
    break;

  default:
    replyNothing();
    break;
  }
}
