#pragma once
/**
 * Efinder - plate-solving finder (TeenAstroEfinder, Raspberry Pi) attached to
 * the Focuser or the GNSS port. The main unit is the master: it probes both
 * ports, pushes site and time, requests solves and applies the result
 * (sync, alignment reference, GoTo++). When the eFinder sits on the GNSS port
 * it may relay NMEA from its own GPS; those lines are fed to mount.gnss.
 *
 * Link protocol (main unit -> eFinder), all replies end with '#':
 *   :e?#                         -> TeenAstroEfinder,<version>#
 *   :eTyyyymmddhhmmss#           -> 1#        set UTC
 *   :eL<lat>,<lonEast>,<elev>#   -> 1#        set site (decimal degrees)
 *   :eC<raHours>,<decDeg>#       -> 1#        current mount pointing (JNow)
 *   :eS#                         -> 1# / 0#   start capture and solve
 *   :eO#                         -> 1# / 0#   start pixel-offset calibration on a bright star
 *   :eo#                         -> 1#        reset pixel offset
 * Host (not forwarded): :eK# plate-solves and stores two cone angles in the
 * telescope frame, from the telescope axis to the efinder axis. Hz Cone is
 * the horizontal cone and Ve Cone is the vertical cone. Later solves are
 * reported as the telescope axis.
 *   :eR#                         -> B# busy, N# nothing, F<code># failed,
 *                                   S<raHours>,<decDeg>,<stars>,<ms># solved (JNow),
 *                                   O<dxArcmin>,<dyArcmin>,<name># offset measured
 *   :eN1# :eN0#                  -> 1#        NMEA relay on/off
 *   :eN?#                        -> 1# / 0#   GPS present on the eFinder
 *   :eX...#                      -> ...#      camera parameters (forwarded from hosts)
 */
#include <Arduino.h>

enum EfinderPort : uint8_t { EF_PORT_NONE = 0, EF_PORT_FOCUSER, EF_PORT_GNSS };

enum EfinderJob : uint8_t {
  EF_JOB_IDLE = 0,
  EF_JOB_SOLVE,        // solve only, report result
  EF_JOB_SOLVE_SYNC,   // solve then sync the mount
  EF_JOB_OFFSET,       // calibrate finder pixel offset
  EF_JOB_GOTOPP,       // GoTo++ refinement after a goto
  EF_JOB_ALIGN,        // automatic alignment
  EF_JOB_AIM           // measure telescope-axis vs efinder-axis rotations
};

enum EfinderResult : uint8_t {
  EF_RES_NONE = 0,
  EF_RES_SOLVED,
  EF_RES_SYNCED,
  EF_RES_OFFSET,
  EF_RES_FAILED,
  EF_RES_ALIGNED
};

class EfinderManager {
public:
  void setup();
  void update();

  bool present() const { return m_port != EF_PORT_NONE; }
  EfinderPort port() const { return m_port; }
  bool ownsGnssPort() const { return m_port == EF_PORT_GNSS; }
  bool busy() const { return m_job != EF_JOB_IDLE; }
  EfinderJob job() const { return m_job; }
  const char* version() const { return m_version; }

  bool startSolve(bool sync);
  bool startOffset();
  bool startAimCal();
  /// \p stars is 2, 4, or 6. Every session starts from home. Six stars is 3+3 when the mount flips.
  bool startAlign(uint8_t stars = 6);
  void abort();
  /// 1-based star the automatic alignment is working on, and how many it will take.
  uint8_t alignIndex() const { return (uint8_t)(m_alignGot + 1); }
  uint8_t alignCount() const { return m_alignStars; }
  bool resetOffset();

  bool gotoPlusPlus() const { return m_gotoPP; }
  void setGotoPlusPlus(bool on);
  uint8_t thresholdArcmin() const { return m_thresholdArcmin; }
  void setThresholdArcmin(uint8_t v);

  /// Formats the last result for :eR# (host side).
  void formatResult(char* out, int len) const;
  /// Status for :e?# (host side): present, port, job, last result, GoTo++.
  void formatStatus(char* out, int len) const;
  /// Blocking pass-through used for :eX...# parameter commands (short replies only).
  bool passthrough(const char* cmd, char* reply, int len);

private:
  enum LinkState : uint8_t { LINK_IDLE = 0, LINK_WAIT };
  enum Step : uint8_t {
    STEP_NONE = 0,
    STEP_START,        // send :eS# or :eO#
    STEP_POLL,         // poll :eR#
    STEP_WAIT_GOTO,    // wait end of a goto started by the eFinder logic
    STEP_SETTLE
  };

  Stream* stream() const;
  /// Look for an eFinder without stalling loop(). The search stops once the
  /// startup window ends, or as soon as a port is obviously something else.
  void pollProbe();
  bool probeWindowOpen() const;
  void beginProbe(EfinderPort p);
  /// Read whatever is already buffered. True when the attempt finished
  /// (identity accepted, or a frame that was not an eFinder).
  bool collectProbe();
  void advanceProbe(EfinderPort failed);
  void retirePort(EfinderPort p);
  bool acceptProbe(EfinderPort p, const char* buf);
  void pump();
  void handleLine(const char* line);
  bool sendRequest(const char* cmd);
  bool transact(const char* cmd, char* reply, int len, unsigned long timeoutMs);
  void pushSiteAndTime();
  void pushPointing();
  void runJob();
  void onSolved(double raH, double decD);
  void onFailed(char code);
  void finishJob(EfinderResult r);
  void watchGoto();
  void loadSettings();
  void saveSettings();
  bool applySync(double raH, double decD);
  bool buildAlignPlan();
  bool skyReaches(double raDeg, double decDeg, PoleSide side) const;
  bool fillAlignSide(PoleSide side, uint8_t slot0, uint8_t count, double preferSign);
  bool fillAlignSpread(uint8_t count);
  bool slewToPlan(uint8_t index);
  bool beginAlignFlip();
  void snapshotAlignModel();
  void restoreAlignModel();
  void failAlign(char code);
  void closeAlignSession();
  void telescopePointing(double& raH, double& decD) const;
  bool mountIsAltAz() const;
  /// Subtract the stored cones so ra/dec is the telescope axis.
  void finderToTelescope(double& raH, double& decD) const;
  /// Store efinder-minus-telescope Hz and Ve cones from a synced star and a raw solve.
  void measureAim(double scopeRaH, double scopeDecD, double finderRaH, double finderDecD);
  void equToHor(double raH, double decD, double& altRad, double& azRad) const;
  void horToEqu(double altRad, double azRad, double& raH, double& decD) const;

  EfinderPort m_port = EF_PORT_NONE;
  char m_version[24] = "";
  unsigned long m_lastProbe = 0;
  unsigned long m_lastSeen = 0;
  // Port waiting for a :e?# reply. EF_PORT_NONE means discovery is idle.
  EfinderPort m_probe = EF_PORT_NONE;
  unsigned long m_probeStart = 0;
  unsigned long m_probeUntil = 0;
  char m_probeBuf[48] = {};
  uint8_t m_probeLen = 0;
  bool m_probeStopped = false;
  bool m_skipFocuser = false;
  bool m_skipGnss = false;

  // incoming frame assembly
  char m_rx[64];
  uint8_t m_rxLen = 0;
  bool m_inNmea = false;
  char m_reply[64];
  bool m_replyReady = false;
  LinkState m_link = LINK_IDLE;
  unsigned long m_reqTime = 0;

  EfinderJob m_job = EF_JOB_IDLE;
  Step m_step = STEP_NONE;
  unsigned long m_stepTime = 0;
  uint8_t m_pass = 0;
  uint8_t m_failures = 0;

  EfinderResult m_lastResult = EF_RES_NONE;
  char m_failCode = 0;
  double m_solvedRaH = 0;
  double m_solvedDecD = 0;
  double m_errorArcmin = 0;
  char m_offsetInfo[40] = "";

  // GoTo++ target captured at the end of the user goto (RA in degrees)
  double m_targetRaH = 0;
  double m_targetDecD = 0;
  uint8_t m_prevGotoState = 0;
  bool m_suppressGotoWatch = false;

  bool m_gotoPP = false;
  uint8_t m_thresholdArcmin = 2;
  bool m_nmea = false;

  // Telescope-frame cones, radians: efinder axis relative to the telescope axis.
  // m_aimHz is the horizontal cone, m_aimVe the vertical cone.
  float m_aimHz = 0;
  float m_aimVe = 0;
  bool m_aimHor = false;
  // Telescope pointing last pushed to the efinder. Aim calibration compares
  // the plate solve to this, not to where tracking has moved since.
  double m_pushedRaH = 0;
  double m_pushedDecD = 0;

  // Automatic alignment: six solved places. A mount that can change pier side
  // takes three, flips, then three more. The previous model is restored if
  // the run stops before the fit is saved.
  struct AlignStop {
    double raDeg = 0;
    double decDeg = 0;
    PoleSide side = POLE_UNDER;
  };
  AlignStop m_plan[6];
  uint8_t m_alignStars = 0;
  uint8_t m_alignGot = 0;
  bool m_alignFlip = false;
  bool m_alignFlipped = false;
  bool m_pendingFlip = false;
  bool m_alignActive = false;
  bool m_hadModel = false;
  bool m_hadHead = false;
  float m_savedT[9] = {};
  float m_savedHead[3] = {};
};

extern EfinderManager efinder;
