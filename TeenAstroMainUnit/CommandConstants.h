#pragma once
/**
 * Command protocol constants for TeenAstro LX200-style serial protocol.
 * Centralises command letters and buffer sizes for clarity and maintainability.
 */

// ----- Command lead characters (first character after ':') -----
namespace Cmd {
  constexpr char RESET       = '$';   // :$x#  Reset / reboot / init
  constexpr char ACK          = 6;    // :<ACK>#  Mount type (LX200)
  constexpr char ALIGNMENT    = 'A';  // :Ax#  Alignment
  constexpr char RETICULE     = 'B';  // :B+#  Reticule brightness
  constexpr char SYNC         = 'C';  // :Cx#  Sync
  constexpr char DISTANCE     = 'D';  // :D#   Distance bars
  constexpr char ENCODER      = 'E';  // :Ex#  Encoder / push-to
  constexpr char FOCUSER      = 'F';  // :Fx#  Focuser
  constexpr char GET          = 'G';  // :Gx#  Get (LX200 + :GXnn# TeenAstro)
  constexpr char GNSS         = 'g';  // :gx#  GNSS sync
  constexpr char HOME_PARK    = 'h';  // :hx#  Home / park
  constexpr char MOVE         = 'M';  // :Mx#  Move / slew / goto
  constexpr char HALT         = 'Q';  // :Qx#  Halt
  constexpr char RATE         = 'R';  // :Rx#  Slew rate
  constexpr char SET          = 'S';  // :Sx#  Set (LX200 + :SXnnn# TeenAstro)
  constexpr char TRACKING     = 'T';  // :Tx#  Tracking
  constexpr char PRECISION    = 'U';  // :U#   Precision toggle
  constexpr char SITE         = 'W';  // :Wn#  Site select
}

// Command buffer limits (from protocol: : + cmd + #)
constexpr int CMD_BUFFER_LEN   = 28;
constexpr int REPLY_BUFFER_LEN = 50;
constexpr int CMD_MAX_PAYLOAD  = 22;  // max chars between : and #
