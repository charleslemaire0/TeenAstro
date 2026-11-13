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
#ifdef EFINDER_OFF
  replyNothing();
  return;
#endif
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
      if (efinder.job() == EF_JOB_ALIGN)
        snprintf(commandState.reply, REPLY_BUFFER_LEN, "B%u,%u#",
                 (unsigned)efinder.alignIndex(), (unsigned)efinder.alignCount());
      else
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
    // :eo#  reset pixel offset and the stored axis rotations
    replyValueSetShort(efinder.resetOffset());
    break;

  case 'K':
    // :eK#  plate-solve and store the Hz cone and the Ve cone in the
    // telescope frame. Hz is the horizontal cone, Ve the vertical cone.
    // Result via :eR# is O<hzArcmin>,<veArcmin>,cone#
    replyValueSetShort(efinder.startAimCal());
    break;

  case 'A':
  {
    // :eA#   six places from home (3+3 when the mount flips)
    // :eA2#  two stars from home, any mount
    // :eA4#  four stars from home, any mount
    // :eA6#  same as :eA#
    uint8_t stars = 6;
    const char n = commandState.command[2];
    if (n == '2' && commandState.command[3] == '\0')
      stars = 2;
    else if (n == '4' && commandState.command[3] == '\0')
      stars = 4;
    else if (n == '6' && commandState.command[3] == '\0')
      stars = 6;
    else if (n != '\0')
    {
      replyNothing();
      break;
    }
    replyValueSetShort(efinder.startAlign(stars));
    break;
  }

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
    // :eTn#  n=2..60 set threshold arcmin (GoTo++ left as-is)
    // The frame parser strips ':' and '#', so :eT1# arrives as "eT1"
    // and :eT15# as "eT15". A lone '1' is enable; a longer number is
    // the threshold, including 10..19.
    if (commandState.command[2] == '0' && commandState.command[3] == '\0')
    {
      efinder.setGotoPlusPlus(false);
      replyShortTrue();
    }
    else if (commandState.command[2] == '1' && commandState.command[3] == '\0')
    {
      efinder.setGotoPlusPlus(true);
      replyShortTrue();
    }
    else if (commandState.command[2] >= '1' && commandState.command[2] <= '9')
    {
      int v = atoi(&commandState.command[2]);
      if (v >= 2 && v <= 60)
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
      else if (strchr(commandState.reply, '#') == NULL)
      {
        // The link parser strips the Pi's trailing '#'. Host replies must
        // end with one or the client waits until its timeout.
        size_t n = strlen(commandState.reply);
        if (n + 1 < REPLY_BUFFER_LEN)
        {
          commandState.reply[n] = '#';
          commandState.reply[n + 1] = '\0';
        }
      }
    }
    break;

  default:
    replyNothing();
    break;
  }
}
