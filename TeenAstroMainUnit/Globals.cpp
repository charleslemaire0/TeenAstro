// Global state definitions: mount, commandState, environment, and alignment/GNSS.
// See Global.h for layout; use mount.*, commandState.*, environment.* directly.
#include "Global.h"

// -----------------------------------------------------------------------------
// Alignment / GNSS
// -----------------------------------------------------------------------------
TinyGPSPlus gps;
CoordConv alignment;
bool hasStarAlignment = false;

// -----------------------------------------------------------------------------
// Environment
// -----------------------------------------------------------------------------
Environment environment = { 10.0, 110.0, {} };

// -----------------------------------------------------------------------------
// MountState constructor
// -----------------------------------------------------------------------------
MountState::MountState()
  : parkStatus(ParkState::PRK_UNPARKED),
    parkSaved(false),
    homeSaved(false),
    atHome(true),
    homeMount(false),
    slewSettleDuration(0U),
    lastSettleTime(0U),
    settling(false),
    backlashStatus(BacklashPhase::DONE),
    DecayModeTrack(false),
    meridianFlip(MeridianFlip::FLIP_NEVER),
    mountType(Mount::MOUNT_TYPE_GEM),
    isMountTypeFix(false),
    maxAlignNumStar(0),
    autoAlignmentBySync(false),
    PushtoStatus(Pushto::PT_OFF),
    hasFocuser(false),
    hasGNSS(true),
    trackComp(TrackingCompensation::TC_BOTH),
    siderealClockSpeed(997269.5625),
    reboot_unit(false),
    minInterval1(StepsMinInterval),
    maxInterval1(StepsMaxInterval),
    minInterval2(StepsMinInterval),
    maxInterval2(StepsMaxInterval),
    pulseGuideRate(0.25f),
    DegreesForAcceleration(3.0),
    enableMotor(false),
    enableEncoder(false),
    EncodeSyncMode(EncoderSync::ES_OFF),
    newTargetPoleSide(POLE_NOTVALID),
    newTargetAlt(0.0),
    newTargetAzm(0.0),
    newTargetDec(0.0),
    newTargetRA(0.0),
    currentAzm(0.0),
    currentAlt(45.0),
    lastError(ErrorsTraking::ERRT_NONE),
    movingTo(false),
    doSpiral(false),
    SpiralFOV(1.0),
    lastSideralTracking(false),
    sideralTracking(false),
    sideralMode(SID_Mode::SIDM_STAR),
    RequestedTrackingRateHA(TrackingStar),
    RequestedTrackingRateDEC(0.0),
    storedTrakingRateRA(0),
    storedTrakingRateDEC(0),
    GuidingState(Guiding::GuidingOFF),
    lastGuidingState(Guiding::GuidingOFF),
    lastSetTrakingEnable(0),
    lastSecurityCheck(0),
    abortSlew(false),
    activeGuideRate(static_cast<byte>(GuideRate::RX)),
    recenterGuideRate(static_cast<byte>(GuideRate::RX))
{
  memset(mountName, 0, sizeof(mountName));
  guideRates[0] = DefaultR0;
  guideRates[1] = DefaultR1;
  guideRates[2] = DefaultR2;
  guideRates[3] = DefaultR3;
  guideRates[4] = DefaultR4;
#ifdef RETICULE_LED_PINS
  reticuleBrightness = 255;
#endif
}

MountState mount;

// -----------------------------------------------------------------------------
// Command buffers and precision (global to avoid macro clash with parameter names)
// -----------------------------------------------------------------------------
char reply[REPLY_BUFFER_LEN];
char command[CMD_BUFFER_LEN];
bool highPrecision = true;

// -----------------------------------------------------------------------------
// CommandState constructor
// -----------------------------------------------------------------------------
CommandState::CommandState()
{
  baudRate_[0] = 115200;
  baudRate_[1] = 56700;
  baudRate_[2] = 38400;
  baudRate_[3] = 28800;
  baudRate_[4] = 19200;
  baudRate_[5] = 14400;
  baudRate_[6] = 9600;
  baudRate_[7] = 4800;
  baudRate_[8] = 2400;
  baudRate_[9] = 1200;
}

CommandState commandState;
